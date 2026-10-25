#include "xTextureManager.h"

#include "xDebug.h"
#include "xstransvc.h"
#include "xbinio.h"
#include "iFile.h"
#include "xFMV.h"
#include "zAssetTypes.h"

#include <string.h>

namespace {

inline U32 DiskSize(U32 fileSize)
{
    U32 sectorSize = iFileGetSectorSize(XFILE_DRIVE_CDVD);
    return sectorSize * ((fileSize + (sectorSize - 1)) / sectorSize);
}

}

void xInternalTextureNode::FreeMemory()
{
    if (Ready()) {
        xTextureManager::Get().freeNode(this);
    } else if (Loading()) {
        S32 zeroIndex = -1;
        for (U32 i = 0; i < 4; i++) {
            if (xTextureManager::Get().m_clearIDs[i] == 0) {
                zeroIndex = i;
            } else if (xTextureManager::Get().m_clearIDs[i] == info.aid) {
                return;
            }
        }
        xASSERT_FMT_L(44, zeroIndex != -1, "Programming error: more textures loading than is possible...");
        xTextureManager::Get().m_clearIDs[zeroIndex] = info.aid;
    }
}

xTextureHandle::xTextureHandle()
{
    m_pNode = NULL;
    m_pPrevNode = NULL;
    m_pTexture = NULL;
    m_pMovie = NULL;
}

void xTextureHandle::StopMovie()
{
    dereference(true);
}

void xTextureHandle::dereference(bool clearPrev)
{
    if (m_pNode) {
        xASSERT_FMT_L(77, m_pNode->refCount > 0, "Should not have a refcount that is 0 or below when dereferencing");
        m_pNode->refCount--;
        if (clearPrev) {
            m_pPrevNode = NULL;
        } else {
            m_pPrevNode = m_pNode;
            if (m_pPrevNode) {
                m_pPrevNode->LRU = xTextureManager::Get().GetLRU();
            }
        }
    }

    if (m_pMovie) {
        xTextureManager::Get().freeNode(m_pMovie);
    }

    m_pNode = NULL;
}

void xTextureHandle::Reference(U32 assetID, bool clearPrev)
{
    if (assetID) {
        RwTexture* pTexture = (RwTexture*)xSTFindAsset(assetID, NULL);
        if (!pTexture) {
            dereference(clearPrev);

            PKRAssetTOCInfo info;
            S32 found = xSTGetAssetInfo(assetID, &info);
            if (!found) {
                m_pNode = NULL;
                m_pTexture = NULL;
            } else {
                xTextureManager::Get().AddTexture(this, &info);
            }
        } else {
            dereference(true);
            m_pTexture = pTexture;
        }
    }
}

bool xTextureHandle::IsMoviePlaying() { return m_pMovie != NULL; }

void xTextureHandle::PlayMovie(U32 assetID, U32 looping, U32 freezeframed, U32 startframe, U32 endframe)
{
    if (assetID) {
        dereference(false);
        xASSERT_FMT_L(142, 0==xSTFindAsset(assetID, NULL), "movie assets should NOT be in memory %X", assetID);

        PKRAssetTOCInfo info;
        S32 found = xSTGetAssetInfo(assetID, &info);
        if (found) {
            xTextureManager::Get().AddMovie(this, &info);
            if (m_pMovie) {
                m_pMovie->m_looping = looping;
                m_pMovie->m_freezeframed = freezeframed;
                m_pMovie->m_startframe = startframe;
                m_pMovie->m_endframe = endframe;
                xASSERT_L(157, !this->m_pMovie->pMovie);
                m_pMovie->pMovie = iFMVOpen(m_pMovie->info);
            }
        }
    }
}

void xTextureHandle::Reference(RwTexture* pTexture)
{
    dereference(true);
    m_pNode = NULL;
    m_pTexture = pTexture;
}

xTextureHandle::~xTextureHandle()
{
    dereference(true);
}

RwTexture* xTextureHandle::GetTexture()
{
    if (IsReady()) {
        if (m_pMovie) {
            return m_pMovie->pTexture;
        } else if (m_pNode) {
            return m_pNode->pTexture;
        }
    }

    if (m_pPrevNode && m_pPrevNode->Ready()) {
        return m_pPrevNode->pTexture;
    }

    return m_pTexture;
}

bool xTextureHandle::IsReady()
{
    if (m_pMovie) {
        m_pMovie->pTexture = xFMVGetTexture(m_pMovie->pMovie, m_pMovie->m_looping, m_pMovie->m_freezeframed,
                                            m_pMovie->m_startframe, m_pMovie->m_endframe, (U32)this);
        return true;
    }

    if (!m_pNode) {
        return true;
    }

    if (m_pNode->Unloaded()) {
        xTextureManager::Get().reload(this);
        return false;
    }

    m_pNode->LRU = xTextureManager::Get().GetLRU();
    return m_pNode->Ready();
}

xTextureHandle& xTextureHandle::operator=(const xTextureHandle& handle)
{
    m_pNode = handle.m_pNode;
    m_pTexture = handle.m_pTexture;
    if (handle.m_pNode) {
        handle.m_pNode->refCount++;
    }

    return *this;
}

void fileCallback(xFile* file)
{
    xInternalTextureNode* pNode = (xInternalTextureNode*)xFileGetUserData(file);
    xFileSetUserData(file, NULL);
    iFileClose(file);

    for (U32 i = 0; i < 4; i++) {
        if (pNode->info.aid == xTextureManager::Get().m_clearIDs[i]) {
            xTextureManager::Get().m_clearIDs[i] = 0;
            RwFree(pNode->pAssetMemory);
            xTextureManager::Get().clearNode(pNode);
            return;
        }
    }

    U32 size;
    void* assetMem = pNode->pAssetMemory;
    pNode->pTexture = (RwTexture*)RWTX_Read(NULL, pNode->info.aid, pNode->pAssetMemory, pNode->info.size, &size);
    RwFree(assetMem);
    pNode->state = xInternalTextureNode::LOADED;
}

xTextureManager& xTextureManager::Get()
{
    static xTextureManager mgr;
    return mgr;
}

xTextureManager::xTextureManager()
{
    m_lru = 0;
    m_currentPoolSize = 0;
    m_maxPoolSize = 0x60000;

    for (U32 i = 0; i < 4; i++) {
        xFileSetUserData(&m_files[i], NULL);
    }
    memset(m_texture_nodes, 0, sizeof(m_texture_nodes));
    memset(m_movie_nodes, 0, sizeof(m_movie_nodes));
}

void xTextureManager::AddTexture(xTextureHandle* handle, PKRAssetTOCInfo* pInfo)
{
    if (!pInfo || !pInfo->aid) {
        return;
    }

    xInternalTextureNode* pNode = NULL;
    for (U32 i = 0; i < maxStreamingTextures; i++) {
        if (m_texture_nodes[i].used && m_texture_nodes[i].info.aid == pInfo->aid) {
#if DEBUG
            for (U32 j = i + 1; j < maxStreamingTextures; j++) {
                xASSERT_FMT_L(319, !m_texture_nodes[j].used || m_texture_nodes[i].info.aid != m_texture_nodes[j].info.aid, "Streaming texture node list is corrupt. Blame a programmer");
            }
#endif

            handle->m_pNode = &m_texture_nodes[i];
            m_texture_nodes[i].refCount++;
            return;
        }
    }

    for (U32 i = 0; i < maxStreamingTextures; i++) {
        if (!m_texture_nodes[i].used) {
            pNode = &m_texture_nodes[i];
            memset(pNode, 0, sizeof(xInternalTextureNode));
            pNode->used = true;
            break;
        }
    }
    xASSERT_FMT_L(344, pNode, "More streaming textures than maxStreamingTextures");
    pNode->refCount = 1;
    memcpy(&pNode->info, pInfo, sizeof(PKRAssetTOCInfo));
    handle->m_pNode = pNode;
}

void xTextureManager::AddMovie(xTextureHandle* handle, PKRAssetTOCInfo* pInfo)
{
    if (!pInfo || !pInfo->aid) {
        return;
    }

    xInternalMovieNode* pNode = NULL;
    for (U32 i = 0; i < maxStreamingMovies; i++) {
        if (m_movie_nodes[i].used && m_movie_nodes[i].info.aid == pInfo->aid) {
            handle->m_pMovie = &m_movie_nodes[i];
            return;
        }
    }

    for (U32 i = 0; i < maxStreamingTextures; i++) {
        if (!m_movie_nodes[i].used) {
            pNode = &m_movie_nodes[i];
            memset(pNode, 0, sizeof(xInternalMovieNode));
            pNode->used = true;
            break;
        }
    }
    xASSERT_FMT_L(374, pNode, "More streaming movie than maxStreamingMovies");
    memcpy(&pNode->info, pInfo, sizeof(PKRAssetTOCInfo));
    handle->m_pMovie = pNode;
}

void xTextureManager::reload(xTextureHandle* pHandle)
{
    xASSERT_FMT_L(419, pHandle && pHandle->m_pNode && !pHandle->m_pNode->pTexture && pHandle->m_pNode->state != xInternalTextureNode::LOADED, "This node is ready or not dynamic, so why reload it?");

    xFile* pFile = NULL;
    for (U32 i = 0; i < 4; i++) {
        if (!xFileGetUserData(&m_files[i])) {
            pFile = &m_files[i];
            break;
        }
    }
    if (!pFile) {
        return;
    }

    xInternalTextureNode* pNode = pHandle->m_pNode;
    U32 size = DiskSize(pNode->info.size);
    U32 doubleSize = size * 2;
    xASSERT_FMT_L(438, doubleSize <= m_maxPoolSize, "This image can't fit in the shared pool even if it kicked out all the other images");
    while (doubleSize + m_currentPoolSize > m_maxPoolSize) {
        if (!freeElement(pHandle->m_pPrevNode)) {
            return;
        }
    }

    pNode->state = xInternalTextureNode::LOADING;
    pNode->pAssetMemory = RwMallocL(size, rwMEMHINTDUR_NADURATION, 450);
    xASSERT_L(452, pNode->pAssetMemory);

    m_currentPoolSize += size;
    pNode->LRU = GetLRU();
    xFileSetUserData(pFile, pNode);

    const char* pHipFilename = xST_xAssetID_HIPFullPath(pNode->info.aid);
    xASSERT_L(464, pHipFilename);
    U32 res = iFileOpen(pHipFilename, 0x41, pFile);
    xASSERT_L(469, !res &&"error opening file");
    iFileSeek(pFile, pNode->info.sector * iFileGetSectorSize(XFILE_DRIVE_CDVD), 0);

    S32 result = iFileReadAsync(pFile, pNode->pAssetMemory, size, fileCallback, 0);
    if (result == -1) {
        xFAIL_FMT_L(476, "%s", "Async read has failed probably because there are too many streams at once.");
        pNode->state = xInternalTextureNode::UNLOADED;
        RwFree(pNode->pAssetMemory);
        m_currentPoolSize -= size;
    }
}

void xTextureManager::freeNode(xInternalTextureNode* pNode)
{
    TextureRW3_Unload(pNode->pTexture, pNode->info.aid);
    clearNode(pNode);
}

void xTextureManager::clearNode(xInternalTextureNode* pNode)
{
    m_currentPoolSize -= DiskSize(pNode->info.size);
    pNode->pAssetMemory = NULL;
    pNode->state = xInternalTextureNode::UNLOADED;
}

void xTextureManager::freeNode(xInternalMovieNode*& pNode)
{
    if (pNode->pMovie) {
        iFMVClose(pNode->pMovie);
    }

    pNode->used = false;
    pNode = NULL;
}

bool xTextureManager::freeElement(xInternalTextureNode* pPrevNode)
{
    for (U32 i = 0; i < maxStreamingTextures; i++) {
        if (&m_texture_nodes[i] == pPrevNode) {
            continue;
        }

        if (!m_texture_nodes[i].refCount && m_texture_nodes[i].Ready()) {
            freeNode(&m_texture_nodes[i]);
            return true;
        }
    }

    S32 index = -1, lru = m_lru + 1;
    for (U32 i = 0; i < maxStreamingTextures; i++) {
        if (&m_texture_nodes[i] == pPrevNode) {
            continue;
        }

        if (m_texture_nodes[i].LRU < lru && m_texture_nodes[i].Ready()) {
            lru = m_texture_nodes[i].LRU;
            index = i;
        }
    }

    if (index == -1) {
        if (pPrevNode->Ready()) {
            freeNode(pPrevNode);
            return true;
        }

        return false;
    }

    freeNode(&m_texture_nodes[index]);
    return true;
}

void xTextureManager::FreeMemory()
{
    for (U32 i = 0; i < maxStreamingTextures; i++) {
        m_texture_nodes[i].FreeMemory();
    }

    for (U32 i = 0; i < maxStreamingMovies; i++) {
        xInternalMovieNode* pNode = &m_movie_nodes[i];
        if (pNode->used) {
            freeNode(pNode);
        }
    }
}

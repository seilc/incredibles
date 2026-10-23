#ifndef XTEXTUREMANAGER_H
#define XTEXTUREMANAGER_H

#include "xpkrsvc.h"
#include "xFile.h"
#include "iFMV.h"
#include <rwcore.h>

struct xInternalMovieNode
{
    U32 hash_id;
    PKRAssetTOCInfo info;
    MOVIE* pMovie;
    RwTexture* pTexture;
    U8 used;
    U8 m_looping;
    U16 m_freezeframed;
    U32 m_startframe;
    U32 m_endframe;
};

struct xInternalTextureNode
{
    PKRAssetTOCInfo info;
    void* pAssetMemory;
    RwTexture* pTexture;
    U16 LRU;
    U8 refCount;
    S8 used;
};

class xTextureHandle
{
private:
    RwTexture* m_pTexture;
    xInternalMovieNode* m_pMovie;
    xInternalTextureNode* m_pNode;
    xInternalTextureNode* m_pPrevNode;
};

class xTextureManager
{
public:
    static xTextureManager& Get();

    void FreeMemory();

private:
    U32 m_maxPoolSize;
    U32 m_currentPoolSize;
    S32 m_lru;
    xInternalTextureNode m_texture_nodes[128];
    xInternalMovieNode m_movie_nodes[4];
    xFile m_files[8];
    U32 m_clearIDs[8];
};

#endif
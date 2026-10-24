#ifndef XTEXTUREMANAGER_H
#define XTEXTUREMANAGER_H

#include "types.h"
#include "xpkrsvc.h"
#include "xFile.h"
#include "iFMV.h"

#include <rwcore.h>

const U32 maxStreamingTextures = 192;
const U32 maxStreamingMovies = 4;

struct xInternalTextureNode
{
    enum
    {
        UNLOADED,
        LOADED,
        LOADING
    };

    PKRAssetTOCInfo info;
    union
    {
        void* pAssetMemory;
        RwTexture* pTexture;
    };
    U16 LRU;
    U8 refCount;
    S8 state;
    bool used;

    bool Ready() { return state == LOADED; }
    bool Loading() { return state == LOADING; }
    bool Unloaded() { return state == UNLOADED; }
    void FreeMemory();
};

struct xInternalMovieNode
{
    U32 hash_id;
    PKRAssetTOCInfo info;
    MOVIE* pMovie;
    RwTexture* pTexture;
    bool used;
    U8 m_looping;
    U16 m_freezeframed;
    U32 m_startframe;
    U32 m_endframe;
};

class xTextureHandle
{
    RwTexture* m_pTexture;
    xInternalMovieNode* m_pMovie;
    xInternalTextureNode* m_pNode;
    xInternalTextureNode* m_pPrevNode;

    friend class xTextureManager;

public:
    xTextureHandle();
    ~xTextureHandle();
    xTextureHandle& operator=(const xTextureHandle& handle);

    void Reference(U32 assetID, bool clearPrev);
    void Reference(RwTexture* pTexture);
    void dereference(bool clearPrev);
    RwTexture* GetTexture();
    bool IsReady();

    void PlayMovie(U32 assetID, U32 looping, U32 freezeframed, U32 startframe, U32 endframe);
    void StopMovie();
    bool IsMoviePlaying();
};

class xTextureManager
{
    U32 m_maxPoolSize;
    U32 m_currentPoolSize;
    S32 m_lru;
    xInternalTextureNode m_texture_nodes[maxStreamingTextures];
    xInternalMovieNode m_movie_nodes[maxStreamingMovies];
    xFile m_files[4];
    U32 m_clearIDs[4];

    friend struct xInternalTextureNode;
    friend void fileCallback(xFile* file);

public:
    static xTextureManager& Get();

    xTextureManager();
    void AddTexture(xTextureHandle* handle, PKRAssetTOCInfo* pInfo);
    void AddMovie(xTextureHandle* handle, PKRAssetTOCInfo* pInfo);
    void reload(xTextureHandle* pHandle);
    void freeNode(xInternalTextureNode* pNode);
    void clearNode(xInternalTextureNode* pNode);
    void freeNode(xInternalMovieNode*& pNode);
    bool freeElement(xInternalTextureNode* pPrevNode);
    void FreeMemory();

    S32 GetLRU() { return ++m_lru; }
};

#endif

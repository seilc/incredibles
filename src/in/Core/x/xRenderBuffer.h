#ifndef XRENDERBUFFER_H
#define XRENDERBUFFER_H

#include "xMath3.h"

#define IRENDER_NUM_INDEX 960
#define IRENDER_NUM_VERTEX 480
#define IRENDER_NUM_VERTEXTZ 480

struct tagiRenderArrays
{
    U16 m_index[IRENDER_NUM_INDEX];
    RwIm3DVertex m_vertex[IRENDER_NUM_VERTEX];
    F32 m_vertexTZ[IRENDER_NUM_VERTEXTZ];
};

struct tagiRenderInput
{
    U16* m_index;
    RwIm3DVertex* m_vertex;
    F32* m_vertexTZ;
    U32 m_mode;
    S32 m_vertexType;
    S32 m_vertexTypeSize;
    S32 m_indexCount;
    S32 m_vertexCount;
    xMat4x3 m_camViewMatrix;
    xVec4 m_camViewR;
    xVec4 m_camViewU;
};

extern tagiRenderArrays gRenderArr;
extern tagiRenderInput gRenderBuffer;

void xRenderBufferInit();

#endif
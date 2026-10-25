#include "xIM.h"

#include "xDebug.h"
#include "xMemMgr.h"

#include <rwcore.h>

void xIMLock(xIMLockContext& context, xIMFormat format, S32 vertCount)
{
    xASSERT_L(9, vertCount > 0);
    xASSERT_L(10, !((format == XIM_TRILIST) && ((vertCount % 3) != 0)));

    S32 pos_qwords = (vertCount * sizeof(xVec3) + (16-1)) / 16;
    S32 uv_qwords = (vertCount * sizeof(xVec2) + (16-1)) / 16;
    S32 color_qwords = (vertCount * sizeof(xColor) + (16-1)) / 16;
    S32 alloc_qwords = pos_qwords + uv_qwords + color_qwords;

    context.data = xMEMPUSHTEMP_L((alloc_qwords*16), 0, 18);

    U8* data_buffer = (U8*)context.data;

    context.pos = (xVec3*)data_buffer;
    data_buffer += pos_qwords*16;

    context.uv = (xVec2*)data_buffer;
    data_buffer += uv_qwords*16;

    context.color = (xColor*)data_buffer;
    data_buffer += color_qwords*16;

    xASSERT_L(28, ((U32)data_buffer - (U32)context.data) == (alloc_qwords*16));

    context.vertCount = vertCount;
    context.format = format;
}

void xIMLock(xIMLockContext& context, xIMFormat format, S32 vertCount, const xIMLockParameters& params)
{
    xASSERT(vertCount > 0);
    xASSERT(!((format == XIM_TRILIST) && ((vertCount % 3) != 0)));

    S32 pos_qwords = (vertCount * sizeof(xVec3) + (16-1)) / 16;
    S32 uv_qwords = (vertCount * sizeof(xVec2) + (16-1)) / 16;
    S32 color_qwords = (vertCount * sizeof(xColor) + (16-1)) / 16;
    S32 alloc_qwords = 0;

    if (!(params.flags & 0x1)) {
        alloc_qwords += pos_qwords;
    }

    if (!(params.flags & 0x4)) {
        alloc_qwords += uv_qwords;
    }

    if (!(params.flags & 0x10)) {
        alloc_qwords += color_qwords;
    }

    context.data = xMEMPUSHTEMP((alloc_qwords*16), 0);

    U8* data_buffer = (U8*)context.data;

    if (params.flags & 0x1) {
        xASSERT(params.cpos != 0);
        context.pos = (xVec3*)params.cpos;
    } else {
        context.pos = (xVec3*)data_buffer;
        data_buffer += pos_qwords*16;
    }

    if (params.flags & 0x4) {
        xASSERT(params.cuv != 0);
        context.uv = (xVec2*)params.cuv;
    } else {
        context.uv = (xVec2*)data_buffer;
        data_buffer += uv_qwords*16;
    }

    if (params.flags & 0x10) {
        xASSERT(params.ccolor != 0);
        context.color = (xColor*)params.ccolor;
    } else {
        context.color = (xColor*)data_buffer;
        data_buffer += color_qwords*16;
    }

    xASSERT(((U32)data_buffer - (U32)context.data) == (alloc_qwords*16));

    context.vertCount = vertCount;
    context.format = format;
}

void xIMUnlock(xIMLockContext& context)
{
    if (context.data) {
        xMEMPOPTEMP(context.data);
    }
}

void xIMRender(xIMLockContext& context, const xMat4x3* mat)
{
    S32 verts_size = context.vertCount;
    const xVec3* pos = context.pos;
    const xVec2* uv = context.uv;
    const xColor* color = context.color;

    RwIm3DVertex* vert_buffer = (RwIm3DVertex*)xMEMPUSHTEMP(verts_size * sizeof(RwIm3DVertex), 0);

    RwIm3DVertex* vert = vert_buffer;
    RwIm3DVertex* end_vert = vert + verts_size;
    for (; vert != end_vert; color++, pos++, uv++, vert++) {
        RwIm3DVertexSetPos(vert, pos->x, pos->y, pos->z);
        RwIm3DVertexSetUV(vert, uv->x, uv->y);
        RwIm3DVertexSetRGBA(vert, color->r, color->g, color->b, color->a);
    }

    RwIm3DTransform(vert_buffer, verts_size, (RwMatrix*)mat, (rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA));
    RwIm3DRenderPrimitive((RwPrimitiveType)context.format);
    RwIm3DEnd();

    xMEMPOPTEMP(vert_buffer);
}

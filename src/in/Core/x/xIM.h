#ifndef XIM_H
#define XIM_H

#include "xMath3.h"
#include "xMath2.h"
#include "xColor.h"

#include <rwcore.h>

enum xIMFormat
{
    XIM_TRISTRIP = 4,
    XIM_TRILIST = 3
};

struct xIMLockContext
{
    void* data;
    xVec3* pos;
    xVec2* uv;
    xColor* color;
    S32 vertCount;
    xIMFormat format;
};

struct xIMLockParameters
{
    S32 flags;
    const xVec3* cpos;
    const xVec2* cuv;
    const xColor* ccolor;
};

void xIMLock(xIMLockContext& context, xIMFormat format, S32 vertCount);
void xIMLock(xIMLockContext& context, xIMFormat format, S32 vertCount, const xIMLockParameters& params);
void xIMUnlock(xIMLockContext& context);
void xIMRender(xIMLockContext& context, const xMat4x3* mat);

void xIMRenderLegacy(const RwIm3DVertex* verts, U32 count, const RwMatrix* ltm, RwPrimitiveType primType);

#endif

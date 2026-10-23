#include "zActionLine.h"

#include "xstransvc.h"

#include <rwcore.h>

#define ACTION_LINE_COUNT 8

static ActionLine* sActionLine[ACTION_LINE_COUNT];
static RwRaster* sActionLineRaster;

void zActionLineInit()
{
    for (S32 i = 0; i < ACTION_LINE_COUNT; i++)
    {
        sActionLine[i] = NULL;
    }

    sActionLineRaster = NULL;

    RwTexture* tex = (RwTexture*)xSTFindAsset(0x7830BF69);

    if (tex)
    {
        sActionLineRaster = RwTextureGetRaster(tex);
    }
}

void zActionLineUpdate(F32 seconds)
{
    for (S32 i = 0; i < ACTION_LINE_COUNT; i++)
    {
        ActionLine* l = sActionLine[i];

        if (l && (l->flags & ZACTIONLINE_ENABLED))
        {
            l->time_left -= seconds;

            if (l->time_left <= 0.0f)
            {
                l->flags &= ~ZACTIONLINE_ENABLED;
            }
        }
    }
}

static void RenderActionLine(ActionLine* l)
{
    static RwIm3DVertex sStripVert[4];

    RwIm3DVertex* vert = &sStripVert[0];

    for (S32 i = 0; i < 4; i++, vert++)
    {
        RwIm3DVertexSetPos(vert, l->pos[i].x, l->pos[i].y, l->pos[i].z);
        RwIm3DVertexSetUV(vert, 0.0f, 0.0f);
        RwIm3DVertexSetRGBA(vert, 255, 255, 255, 128);
    }

    sStripVert[2].u = 1.0f;

    sStripVert[3].u = 1.0f;
    sStripVert[3].v = 1.0f;

    if (RwIm3DTransform(sStripVert, 4, NULL,
                        rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
    {
        RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
        RwIm3DEnd();
    }
}

void zActionLineRender()
{
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)sActionLineRaster);

    for (S32 i = 0; i < ACTION_LINE_COUNT; i++)
    {
        if (sActionLine[i] && (sActionLine[i]->flags & ZACTIONLINE_ENABLED))
        {
            RenderActionLine(sActionLine[i]);
        }
    }
}
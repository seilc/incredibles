#include "zGrapple.h"

#include "zScene.h"
#include "xGroup.h"
#include "xstransvc.h"
#include "xMarkerAsset.h"
#include "xEvent.h"

static zGrapplePoint* sGrapplePoints = NULL;
static S32 sNumGrapplePoints = 0;

void zGrapple_Init(xBase& data, xDynAsset& asset, size_t)
{
    zGrapple_Init((zGrapple*)&data, (zGrappleAsset*)&asset);
}

void zGrapple_Init(zGrapple* grapple, zGrappleAsset* asset)
{
    xBaseInit(grapple, asset);

    grapple->grappleAsset = asset;
    grapple->eventFunc = zGrapple_EventCB;

    if (grapple->linkCount)
    {
        grapple->link = (xLinkAsset*)(asset + 1);
    }
    else
    {
        grapple->link = NULL;
    }
}

S32 zGrapple_Recurse(U32 id, zGrapplePoint* gpList, zGrapple* grapple)
{
    U32 i;
    S32 result = 0;
    xBase* base = zSceneFindObject(id);

    if (base)
    {
        if (base->baseType == eBaseTypeGroup)
        {
            xGroup* group = (xGroup*)base;
            U32 count = xGroupGetCount(group);

            for (i = 0; i < count; i++)
            {
                U32 memberID = xGroupGetItem(group, i);
                S32 incr = gpList ? zGrapple_Recurse(memberID, &gpList[result], grapple) :
                                      zGrapple_Recurse(memberID, NULL, NULL);

                result += incr;
            }
        }
        else if (base->baseFlags & 0x20)
        {
            if (gpList)
            {
                gpList[0].pos = (xVec3*)&((xEnt*)base)->model->Mat->pos;
                gpList[0].local = (xMat3x3*)((xEnt*)base)->model->Mat;
                gpList[0].offset = &grapple->grappleAsset->offset;
                gpList[0].optr = base;
                gpList[0].grap = grapple;
            }

            result = 1;
        }
    }
    else
    {
        U32 size;
        void* data = xSTFindAsset(id, &size);

        if (size == sizeof(xMarkerAsset))
        {
            if (gpList)
            {
                gpList[0].pos = &((xMarkerAsset*)data)->pos;
                gpList[0].local = NULL;
                gpList[0].offset = NULL;
                gpList[0].optr = data;
                gpList[0].grap = grapple;
            }

            result = 1;
        }
    }

    return result;
}

void zGrapple_SceneInit(zScene* zsc)
{
    S32 i;
    S32 count = zsc->baseCount[eBaseTypeGrapple];
    S32 numGrapples = 0;
    zGrapple* grapple;

    for (i = 0; i < count; i++)
    {
        grapple = (zGrapple*)zsc->baseList[eBaseTypeGrapple] + i;
        numGrapples += zGrapple_Recurse(grapple->grappleAsset->object, NULL, NULL);
    }

    sGrapplePoints = (zGrapplePoint*)xMEMALLOC(numGrapples * sizeof(zGrapplePoint));
    sNumGrapplePoints = numGrapples;

    numGrapples = 0;

    for (i = 0; i < count; i++)
    {
        grapple = (zGrapple*)zsc->baseList[eBaseTypeGrapple] + i;
        numGrapples +=
            zGrapple_Recurse(grapple->grappleAsset->object, sGrapplePoints + numGrapples, grapple);
    }

    for (i = 0; i < sNumGrapplePoints; i++)
    {
        grapple = sGrapplePoints[i].grap;
        grapple->flags = grapple->grappleAsset->grapFlags;
    }
}

void zGrapple_Reset()
{
    S32 i;
    zGrapple* grapple;

    for (i = 0; i < sNumGrapplePoints; i++)
    {
        grapple = sGrapplePoints[i].grap;
        grapple->flags = grapple->grappleAsset->grapFlags;
    }
}

void zGrapple_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    zGrapple* grap = (zGrapple*)to;

    switch (toEvent)
    {
    case eEventOn:
        grap->flags |= 0x1;
        break;
    case eEventOff:
        grap->flags &= ~0x1;
        break;
    }
}

S32 zGrapple_NumGrapplePoints()
{
    return sNumGrapplePoints;
}

zGrapplePoint* zGrapple_GetGrapplePoint(S32 i)
{
    return &sGrapplePoints[i];
}

void zGrapple_GetPosition(zGrapplePoint* point, xVec3* pos)
{
    xVec3Copy(pos, point->pos);

    if (point->offset && point->local)
    {
        xVec3AddScaled(pos, &point->local->right, point->offset->x);
        xVec3AddScaled(pos, &point->local->up, point->offset->y);
        xVec3AddScaled(pos, &point->local->at, point->offset->z);
    }
}
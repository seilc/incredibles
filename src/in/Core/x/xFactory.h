#ifndef XFACTORY_H
#define XFACTORY_H

#include "xRMemData.h"
#include "xordarray.h"

struct xFactoryInst : RyzMemData
{
    S32 itemType;
    xFactoryInst* nextprod;
    xFactoryInst* prevprod;

    xFactoryInst()
    {
        itemType = 0;
        prevprod = NULL;
        nextprod = NULL;
    }

    ~xFactoryInst()
    {
    }
};

typedef xFactoryInst* (*xFactoryCreateCallback)(S32, RyzMemGrow*, void*);
typedef void (*xFactoryDestroyCallback)(xFactoryInst*);

struct XGOFTypeInfo
{
    S32 tid;
    xFactoryCreateCallback creator;
    xFactoryDestroyCallback destroyer;
};

struct xFactory : RyzMemData
{
    XGOFTypeInfo* infopool;
    st_XORDEREDARRAY infolist;
    xFactoryInst* products;
    RyzMemGrow growContextData;

    xFactory(S32 maxTypes);
    ~xFactory();

    S32 RegItemType(XGOFTypeInfo* info);
    S32 RegItemType(S32 tid, xFactoryCreateCallback create, xFactoryDestroyCallback destroy); // inferred from TSSM GC, Rat MAP
    void GrowDataEnable(xBase* user, S32 isResume); // inferred from TSSM GC, Rat MAP
    void GrowDataDisable();
    xFactoryInst* CreateItem(S32 typeID, void* userdata, RyzMemGrow* callerzgrow);
    void DestroyAll();
    void DestroyItem(xFactoryInst* item);
};

#endif
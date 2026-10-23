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

struct XGOFTypeInfo
{
    S32 tid;
    xFactoryInst* (*creator)(S32, RyzMemGrow*, void*);
    void (*destroyer)(xFactoryInst*);
};

struct xFactory : RyzMemData
{
    XGOFTypeInfo* infopool;
    XORDEREDARRAY infolist;
    xFactoryInst* products;
    RyzMemGrow growContextData;

    xFactory(S32 maxTypes);
    ~xFactory();

    S32 RegItemType(XGOFTypeInfo* info);
    xFactoryInst* CreateItem(S32 typeID, void* userdata, RyzMemGrow* callerzgrow);
    void DestroyAll();
    void DestroyItem(xFactoryInst* item);
};

#endif
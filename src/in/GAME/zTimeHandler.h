#ifndef ZTIMEHANDLER_H
#define ZTIMEHANDLER_H

#include "zQueue.h"
#include "xMath3.h"

struct zTimeHandlerStruct
{
    F32 elapsed;
    F32 period;
    void (*cb)(F32, zTimeHandlerStruct*);
    xVec3 pos;
    xVec3 normal;
    F32 num_particles;
};

class zTimeHandlerMgr : public zQueue<zTimeHandlerStruct, 64>
{
public:
    void update(F32 dt);
};

extern zTimeHandlerMgr timehandler_mgr;

#endif
#include "zTimeHandler.h"

zTimeHandlerMgr timehandler_mgr;

void zTimeHandlerMgr::update(F32 dt)
{
    for (S32 i = get_size_queue() - 1; i >= 0; i--)
    {
        zTimeHandlerStruct* s = fetch_queue_ptr(i);

        if (s)
        {
            s->cb(dt, s);
            s->elapsed += dt;

            if (s->elapsed > s->period)
            {
                get_queue(*s);
            }
        }
    }
}
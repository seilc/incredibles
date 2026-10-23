#ifndef XCSNNOSEY_H
#define XCSNNOSEY_H

#include <types.h>
#include <rwcore.h>
#include <rpworld.h>

class XCSNNosey
{
public:
    XCSNNosey();

    virtual void CanRenderNow();
    virtual void UpdatedAnimated(RpAtomic* model, RwMatrix* animMat, U32 animIndex,
                                 U32 dataIndex, U32&);

private:
    void* userdata;
    S32 flg_nosey;
};

#endif
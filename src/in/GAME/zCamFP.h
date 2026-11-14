#ifndef ZCAMFP_H
#define ZCAMFP_H

#include "xCam.h"

struct xEnt;

class zCamFP : public xCam
{
public:
    U32 fpFlags;
    F32 yawSpeed;
    F32 pitchSpeed;
    F32 minPitch;
    F32 maxPitch;
    S32 boneIndex;
    xVec3 boneOffset;

    void CopyPosFromPlayer(xEnt* ent);
    void CopyYawFromPlayer(xEnt* ent);
    void CopyToPlayer(xEnt* ent);

    virtual void create();
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, F32 dt);
};

#endif

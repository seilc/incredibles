#ifndef ZCAMSWING_H
#define ZCAMSWING_H

#include "xCam.h"
#include "xCamSupport.h"

struct zCamSwingPrefs
{
    F32 height;
    F32 dist;
    F32 pitchCorrect;
    F32 targetHeight;
};

// zCamSwing::camSwingFlags
#define k_ZCAMSWING_UNLOCKED 0x1

class zCamSwing : public xCam
{
public:
    S32 camSwingFlags;
    F32 prefFactor;
    F32 yawSpeed;
    F32 unlockedLag;
    zCamSwingPrefs pref[3];
    F32 distOffset;
    F32 distSpeed;
    F32 heightOffset;
    F32 heightSpeed;
    F32 lockFactorSpeed;
    F32 unlockFactorSpeed;
    xCamSupportLOSBar losbar;

    void get_target(xVec3* target);
    void refresh_from_mat();
    void refresh_from_my_mat();
    void refresh_from_other_mat();
    void set_pref_factor(xVec3* hangVec, F32 dt);
    void smooth_pref_factor(F32 factor, F32 dt);
    void get_height_and_dist(zCamSwingPrefs* prefs);
    F32 get_targetHeight();
    F32 get_pitchCorrect();

    virtual void create();
    virtual void start();
    virtual void update(xScene& scene, F32 dt);
};

#endif

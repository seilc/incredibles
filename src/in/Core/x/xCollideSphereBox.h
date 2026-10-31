#ifndef XCOLLIDESPHEREBOX_H
#define XCOLLIDESPHEREBOX_H

#include "xCollideSweptSphere.h"

S32 xSweptSphereToBox(xSweptSphere& sws, const xBox& box);
S32 xSweptSphereToOBB(xSweptSphere* sws, const xBox* box, const xMat4x3* mat);

inline S32 xSweptSphereToBox(xSweptSphere* sws, const xBox* box)
{
    return xSweptSphereToBox(*sws, *box);
}

#endif

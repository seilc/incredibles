#ifndef XMODELWARP_H
#define XMODELWARP_H

#include "xMath2.h"
#include "xMath3.h"
#include "xColor.h"

struct RpAtomic;
class xFXCameraTexture;

void xModelWarpSceneEnter();
void xModelWarpSceneExit();
void xModelWarpSetEffect(RpAtomic* atomic, const xBox* box, const xFColor* color);
void xModelWarpSphereMapUVsByNormals(xVec2* uv, S32 verts_size, const xVec3* norm);
void xModelWarpSphereMapUVs(RpAtomic* atomic, bool use_normals);
xFXCameraTexture* xModelWarpGrabScreen(const xSphere& sphere, const xFColor& color, xMat4x3* subview_mat);

#endif
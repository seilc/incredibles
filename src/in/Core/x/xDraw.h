#ifndef XDRAW_H
#define XDRAW_H

#include "xMath3.h"

inline void xDrawCard(RwRaster* raster, const xFRect& src_rect, const xFRect& dst_rect, bool zwrite) {}
inline void xDrawSetColor(xColor color) {}
inline void xDrawLine(const xVec3* a, const xVec3* b) {}
inline void xDrawArrow(const xVec3* from, const xVec3* dir, bool asRay, F32 length) {}
inline void xDrawSphere(const xSphere* sph, U32 flags) {}
inline void xDrawSphere(const xVec3* center, F32 r, U32 flags) {}

#endif

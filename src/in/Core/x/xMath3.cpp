#include "xCommon.h"
#include "xMath3.h"

#include "xMath.h"

const xVec3 g_O3 = { 0, 0, 0 };
const xVec3 g_X3 = { 1, 0, 0 };
const xVec3 g_Y3 = { 0, 1, 0 };
const xVec3 g_Z3 = { 0, 0, 1 };
const xVec3 g_NX3 = { -1, 0, 0 };
const xVec3 g_NY3 = { 0, -1, 0 };
const xVec3 g_NZ3 = { 0, 0, -1 };
xMat4x3 g_I3;
const xQuat g_IQ = { { 0, 0, 0 }, 1 };

void xMath3Init()
{
    iMath3Init();

    g_I3.right.x = g_X3.x, g_I3.right.y = g_X3.y, g_I3.right.z = g_X3.z;
    g_I3.up.x = g_Y3.x, g_I3.up.y = g_Y3.y, g_I3.up.z = g_Y3.z;
    g_I3.at.x = g_Z3.x, g_I3.at.y = g_Z3.y, g_I3.at.z = g_Z3.z;
    g_I3.pos.x = g_O3.x, g_I3.pos.y = g_O3.y, g_I3.pos.z = g_O3.z;
}

void xLine3VecDist2(const xVec3* p1, const xVec3* p2, const xVec3* v, xIsect* isx)
{
    xVec3 ldir;
    xVec3Sub(&ldir, p2, p1);

    xVec3Sub(&isx->norm, v, p1);

    F32 ldirdotlv = xVec3Dot(&ldir, &isx->norm);
    if (ldirdotlv <= 0.0f) {
        isx->dist = xVec3Length2(&isx->norm);
    } else {
        F32 ldirlen2 = xVec3Length2(&ldir);
        if (ldirdotlv >= ldirlen2) {
            xVec3Sub(&isx->norm, v, p2);
            isx->dist = xVec3Length2(&isx->norm);
        } else {
            F32 lvlen2 = xVec3Length2(&isx->norm);
            isx->dist = lvlen2 - ldirdotlv * ldirdotlv / ldirlen2;
        }
    }
}

static F32 gs_fTolerance = 1e-05f;

void xLine3LineMinDist(const xVec3* p1, const xVec3* p2, const xVec3* q1, const xVec3* q2, F32& t, F32& u)
{
    F32 kDiffx, kDiffy, kDiffz, pDirx, pDiry, pDirz, qDirx, qDiry, qDirz;

    kDiffx = p1->x - q1->x, kDiffy = p1->y - q1->y, kDiffz = p1->z - q1->z;
    pDirx = p2->x - p1->x, pDiry = p2->y - p1->y, pDirz = p2->z - p1->z;
    qDirx = q2->x - q1->x, qDiry = q2->y - q1->y, qDirz = q2->z - q1->z;
    F32 fA00 = pDirx * pDirx + pDiry * pDiry + pDirz * pDirz;
    F32 fA01 = -(pDirx * qDirx + pDiry * qDiry + pDirz * qDirz);
    F32 fA11 = qDirx * qDirx + qDiry * qDiry + qDirz * qDirz;
    F32 fB0 = kDiffx * pDirx + kDiffy * pDiry + kDiffz * pDirz;
    F32 fDet = xabs(fA00 * fA11 - fA01 * fA01), fB1, fS, fT, fTmp;

    if (fDet >= gs_fTolerance) {
        fB1 = -(kDiffx * qDirx + kDiffy * qDiry + kDiffz * qDirz);
        fS = fA01 * fB1 - fA11 * fB0;
        fT = fA01 * fB0 - fA00 * fB1;

        if (fS >= 0.0f) {
            if (fS <= fDet) {
                if (fT >= 0.0f) {
                    if (fT <= fDet) {
                        F32 fInvDet = 1.0f / fDet;
                        fS *= fInvDet;
                        fT *= fInvDet;
                    } else {
                        fT = 1.0f;
                        fTmp = fA01 + fB0;
                        if (fTmp >= 0.0f) {
                            fS = 0.0f;
                        }
                        else if (-fTmp >= fA00) {
                            fS = 1.0f;
                        } else {
                            fS = -fTmp / fA00;
                        }
                    }
                } else {
                    fT = 0.0f;
                    if (fB0 >= 0.0f) {
                        fS = 0.0f;
                    }
                    else if (-fB0 >= fA00) {
                        fS = 1.0f;
                    } else {
                        fS = -fB0 / fA00;
                    }
                }
            } else {
                if (fT >= 0.0f) {
                    if (fT <= fDet) {
                        fS = 1.0f;
                        fTmp = fA01 + fB1;
                        if (fTmp >= 0.0f) {
                            fT = 0.0f;
                        }
                        else if (-fTmp >= fA11) {
                            fT = 1.0f;
                        } else {
                            fT = -fTmp / fA11;
                        }
                    } else {
                        fTmp = fA01 + fB0;
                        if (-fTmp <= fA00) {
                            fT = 1.0f;
                            if (fTmp >= 0.0f) {
                                fS = 0.0f;
                            } else {
                                fS = -fTmp / fA00;
                            }
                        } else {
                            fS = 1.0f;
                            fTmp = fA01 + fB1;
                            if (fTmp >= 0.0f) {
                                fT = 0.0f;
                            }
                            else if (-fTmp >= fA11) {
                                fT = 1.0f;
                            } else {
                                fT = -fTmp / fA11;
                            }
                        }
                    }
                } else {
                    if (-fB0 < fA00) {
                        fT = 0.0f;
                        if (fB0 >= 0.0f) {
                            fS = 0.0f;
                        } else {
                            fS = -fB0 / fA00;
                        }
                    } else {
                        fS = 1.0f;
                        fTmp = fA01 + fB1;
                        if (fTmp >= 0.0f) {
                            fT = 0.0f;
                        }
                        else if (-fTmp >= fA11) {
                            fT = 1.0f;
                        } else {
                            fT = -fTmp / fA11;
                        }
                    }
                }
            }
        } else {
            if (fT >= 0.0f) {
                if (fT <= fDet) {
                    fS = 0.0f;
                    if (fB1 >= 0.0f) {
                        fT = 0.0f;
                    }
                    else if (-fB1 >= fA11) {
                        fT = 1.0f;
                    } else {
                        fT = -fB1 / fA11;
                    }
                } else {
                    fTmp = fA01 + fB0;
                    if (fTmp < 0.0f) {
                        fT = 1.0f;
                        if (-fTmp >= fA00) {
                            fS = 1.0f;
                        } else {
                            fS = -fTmp / fA00;
                        }
                    } else {
                        fS = 0.0f;
                        if (fB1 >= 0.0f) {
                            fT = 0.0f;
                        }
                        else if (-fB1 >= fA11) {
                            fT = 1.0f;
                        } else {
                            fT = -fB1 / fA11;
                        }
                    }
                }
            } else {
                if (fB0 < 0.0f) {
                    fT = 0.0f;
                    if (-fB0 >= fA00) {
                        fS = 1.0f;
                    } else {
                        fS = -fB0 / fA00;
                    }
                } else {
                    fS = 0.0f;
                    if (fB1 >= 0.0f) {
                        fT = 0.0f;
                    }
                    else if (-fB1 >= fA11) {
                        fT = 1.0f;
                    } else {
                        fT = -fB1 / fA11;
                    }
                }
            }
        }
    } else {
        if (fA01 > 0.0f) {
            if (fB0 >= 0.0f) {
                fS = 0.0f;
                fT = 0.0f;
            }
            else if (-fB0 <= fA00) {
                fS = -fB0 / fA00;
                fT = 0.0f;
            } else {
                fB1 = -(kDiffx * qDirx + kDiffy * qDiry + kDiffz * qDirz);
                fS = 1.0f;
                fTmp = fA00 + fB0;
                if (-fTmp >= fA01) {
                    fT = 1.0f;
                } else {
                    fT = -fTmp / fA01;
                }
            }
        } else {
            if (-fB0 >= fA00) {
                fS = 1.0f;
                fT = 0.0f;
            }
            else if (fB0 <= 0.0f) {
                fS = -fB0 / fA00;
                fT = 0.0f;
            } else {
                fB1 = -(kDiffx * qDirx + kDiffy * qDiry + kDiffz * qDirz);
                fS = 0.0f;
                if (fB0 >= -fA01) {
                    fT = 1.0f;
                } else {
                    fT = -fB0 / fA01;
                }
            }
        }
    }

    t = fS;
    u = fT;
}

#define xDIST2(ax, ay, az, bx, by, bz, outlen)                                                    \
do {                                                                                              \
    F32 dx__ = (ax) - (bx);                                                                       \
    F32 dy__ = (ay) - (by);                                                                       \
    F32 dz__ = (az) - (bz);                                                                       \
    (outlen) = dx__ * dx__ + dy__ * dy__ + dz__ * dz__;                                           \
} while (0)

void xLine3TriMinDist(const xVec3* p1, const xVec3* p2, const xVec3* V0, const xVec3* V1, const xVec3* V2, F32& t,
                      F32& u, F32& v)
{
    F32 kDiffx, kDiffy, kDiffz, pDirx, pDiry, pDirz, edge0x, edge0y, edge0z, edge1x, edge1y, edge1z;

    kDiffx = V0->x - p1->x, kDiffy = V0->y - p1->y, kDiffz = V0->z - p1->z;
    pDirx = p2->x - p1->x, pDiry = p2->y - p1->y, pDirz = p2->z - p1->z;
    edge0x = V1->x - V0->x, edge0y = V1->y - V0->y, edge0z = V1->z - V0->z;
    edge1x = V2->x - V0->x, edge1y = V2->y - V0->y, edge1z = V2->z - V0->z;
    F32 fA00 = pDirx * pDirx + pDiry * pDiry + pDirz * pDirz;
    F32 fA01 = -(pDirx * edge0x + pDiry * edge0y + pDirz * edge0z);
    F32 fA02 = -(pDirx * edge1x + pDiry * edge1y + pDirz * edge1z);
    F32 fA11 = edge0x * edge0x + edge0y * edge0y + edge0z * edge0z;
    F32 fA12 = edge0x * edge1x + edge0y * edge1y + edge0z * edge1z;
    F32 fA22 = edge1x * edge1x + edge1y * edge1y + edge1z * edge1z;
    F32 fB0 = -(kDiffx * pDirx + kDiffy * pDiry + kDiffz * pDirz);
    F32 fB1 = kDiffx * edge0x + kDiffy * edge0y + kDiffz * edge0z;
    F32 fB2 = kDiffx * edge1x + kDiffy * edge1y + kDiffz * edge1z;

    F32 fSqrDist, fSqrDist0, fR, fS, fT, fR0, fS0, fT0;

    F32 kNx, kNy, kNz;
    kNx = edge0y * edge1z - edge1y * edge0z, kNy = edge0z * edge1x - edge1z * edge0x, kNz = edge0x * edge1y - edge1x * edge0y;
    F32 fNSqrLen = kNx * kNx + kNy * kNy + kNz * kNz;
    F32 fDot = pDirx * kNx + pDiry * kNy + pDirz * kNz;
    S32 bNotParallel = fDot * fDot >= gs_fTolerance * fA00 * fNSqrLen;
    F32 tp1x, tp1y, tp1z, tp2x, tp2y, tp2z;

    if (bNotParallel) {
        F32 fCof00 = fA11 * fA22 - fA12 * fA12;
        F32 fCof01 = fA02 * fA12 - fA01 * fA22;
        F32 fCof02 = fA01 * fA12 - fA02 * fA11;
        F32 fCof11 = fA00 * fA22 - fA02 * fA02;
        F32 fCof12 = fA02 * fA01 - fA00 * fA12;
        F32 fCof22 = fA00 * fA11 - fA01 * fA01;
        F32 fInvDet = 1.0f / (fA00 * fCof00 + fA01 * fCof01 + fA02 * fCof02);
        F32 fRhs0 = -fB0 * fInvDet;
        F32 fRhs1 = -fB1 * fInvDet;
        F32 fRhs2 = -fB2 * fInvDet;

        fR = fCof00 * fRhs0 + fCof01 * fRhs1 + fCof02 * fRhs2;
        fS = fCof01 * fRhs0 + fCof11 * fRhs1 + fCof12 * fRhs2;
        fT = fCof02 * fRhs0 + fCof12 * fRhs1 + fCof22 * fRhs2;

        if (fR < 0.0f) {
            if (fS + fT <= 1.0f) {
                if (fS < 0.0f) {
                    if (fT < 0.0f) {
                        xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                        tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                            tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                        tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                            tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                        fS = 0.0f;

                        xLine3LineMinDist(p1, p2, V0, V1, fR0, fS0);
                        tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                            tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                        tp2x = V0->x * (1.0f - fS0) + V1->x * fS0, tp2y = V0->y * (1.0f - fS0) + V1->y * fS0,
                            tp2z = V0->z * (1.0f - fS0) + V1->z * fS0;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fR = fR0;
                            fS = fS0;
                            fT = fT0;
                        }

                        xTri3VecMinDist(V0, V1, V2, p1, fS0, fT0);
                        tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                            tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                        tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                        tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                        xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fR = fR0;
                            fS = fS0;
                            fT = fT0;
                        }
                    } else {
                        xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                        tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                            tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                        tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                            tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                        fS = 0.0f;

                        xTri3VecMinDist(V0, V1, V2, p1, fS0, fT0);
                        tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                            tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                        tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                        tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                        xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fR = fR0;
                            fS = fS0;
                            fT = fT0;
                        }
                    }
                }
                else if (fT < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V1, fR, fS);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fS) + V1->x * fS, tp2y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp2z = V0->z * (1.0f - fS) + V1->z * fS;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fT = 0.0f;

                    xTri3VecMinDist(V0, V1, V2, p1, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 0.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                } else {
                    xTri3VecMinDist(V0, V1, V2, p1, fS, fT);
                    tp1x = V0->x * (1.0f - fS) + V1->x * fS, tp1y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp1z = V0->z * (1.0f - fS) + V1->z * fS;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT, tp1y += tp2y * fT, tp1z += tp2z * fT;
                    xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist);
                    fR = 0.0f;
                }
            } else {
                if (fS < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                        tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fS = 0.0f;

                    xLine3LineMinDist(p1, p2, V1, V2, fR0, fT0);
                    tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                        tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                    tp2x = V1->x * (1.0f - fT0) + V2->x * fT0, tp2y = V1->y * (1.0f - fT0) + V2->y * fT0,
                        tp2z = V1->z * (1.0f - fT0) + V2->z * fT0;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                    fS0 = 1.0f - fT0;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }

                    xTri3VecMinDist(V0, V1, V2, p1, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 0.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                }
                else if (fT < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V1, fR, fS);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fS) + V1->x * fS, tp2y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp2z = V0->z * (1.0f - fS) + V1->z * fS;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fT = 0.0f;

                    xLine3LineMinDist(p1, p2, V1, V2, fR0, fT0);
                    tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                        tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                    tp2x = V1->x * (1.0f - fT0) + V2->x * fT0, tp2y = V1->y * (1.0f - fT0) + V2->y * fT0,
                        tp2z = V1->z * (1.0f - fT0) + V2->z * fT0;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                    fS0 = 1.0f - fT0;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }

                    xTri3VecMinDist(V0, V1, V2, p1, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 0.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                } else {
                    xLine3LineMinDist(p1, p2, V1, V2, fR, fT);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V1->x * (1.0f - fT) + V2->x * fT, tp2y = V1->y * (1.0f - fT) + V2->y * fT,
                        tp2z = V1->z * (1.0f - fT) + V2->z * fT;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fS = 1.0f - fT;

                    xTri3VecMinDist(V0, V1, V2, p1, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 0.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                }
            }
        }
        else if (fR <= 1.0f) {
            if (fS + fT <= 1.0f) {
                if (fS < 0.0f) {
                    if (fT < 0.0f) {
                        xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                        tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                            tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                        tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                            tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                        fS = 0.0f;

                        xLine3LineMinDist(p1, p2, V0, V1, fR0, fS0);
                        tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                            tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                        tp2x = V0->x * (1.0f - fS0) + V1->x * fS0, tp2y = V0->y * (1.0f - fS0) + V1->y * fS0,
                            tp2z = V0->z * (1.0f - fS0) + V1->z * fS0;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fR = fR0;
                            fS = fS0;
                            fT = fT0;
                        }
                    } else {
                        xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                        tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                            tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                        tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                            tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                        fS = 0.0f;
                    }
                }
                else if (fT < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V1, fR, fS);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fS) + V1->x * fS, tp2y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp2z = V0->z * (1.0f - fS) + V1->z * fS;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fT = 0.0f;
                } else {
                    fSqrDist = 0.0f;
                }
            } else {
                if (fS < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                        tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fS = 0.0f;

                    xLine3LineMinDist(p1, p2, V1, V2, fR0, fT0);
                    tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                        tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                    tp2x = V1->x * (1.0f - fT0) + V2->x * fT0, tp2y = V1->y * (1.0f - fT0) + V2->y * fT0,
                        tp2z = V1->z * (1.0f - fT0) + V2->z * fT0;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                    fS0 = 1.0f - fT0;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                }
                else if (fT < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V1, fR, fS);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fS) + V1->x * fS, tp2y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp2z = V0->z * (1.0f - fS) + V1->z * fS;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fT = 0.0f;

                    xLine3LineMinDist(p1, p2, V1, V2, fR0, fT0);
                    tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                        tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                    tp2x = V1->x * (1.0f - fT0) + V2->x * fT0, tp2y = V1->y * (1.0f - fT0) + V2->y * fT0,
                        tp2z = V1->z * (1.0f - fT0) + V2->z * fT0;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                    fS0 = 1.0f - fT0;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                } else {
                    xLine3LineMinDist(p1, p2, V1, V2, fR, fT);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V1->x * (1.0f - fT) + V2->x * fT, tp2y = V1->y * (1.0f - fT) + V2->y * fT,
                        tp2z = V1->z * (1.0f - fT) + V2->z * fT;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fS = 1.0f - fT;
                }
            }
        } else {
            if (fS + fT <= 1.0f) {
                if (fS < 0.0f) {
                    if (fT < 0.0f) {
                        xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                        tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                            tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                        tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                            tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                        fS = 0.0f;

                        xLine3LineMinDist(p1, p2, V0, V1, fR0, fS0);
                        tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                            tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                        tp2x = V0->x * (1.0f - fS0) + V1->x * fS0, tp2y = V0->y * (1.0f - fS0) + V1->y * fS0,
                            tp2z = V0->z * (1.0f - fS0) + V1->z * fS0;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fR = fR0;
                            fS = fS0;
                            fT = fT0;
                        }

                        xTri3VecMinDist(V0, V1, V2, p2, fS0, fT0);
                        tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                            tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                        tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                        tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                        xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fR = fR0;
                            fS = fS0;
                            fT = fT0;
                        }
                    } else {
                        xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                        tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                            tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                        tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                            tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                        fS = 0.0f;

                        xTri3VecMinDist(V0, V1, V2, p2, fS0, fT0);
                        tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                            tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                        tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                        tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                        xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fR = fR0;
                            fS = fS0;
                            fT = fT0;
                        }
                    }
                }
                else if (fT < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V1, fR, fS);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fS) + V1->x * fS, tp2y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp2z = V0->z * (1.0f - fS) + V1->z * fS;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fT = 0.0f;

                    xTri3VecMinDist(V0, V1, V2, p2, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 1.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                } else {
                    xTri3VecMinDist(V0, V1, V2, p2, fS, fT);
                    tp1x = V0->x * (1.0f - fS) + V1->x * fS, tp1y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp1z = V0->z * (1.0f - fS) + V1->z * fS;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT, tp1y += tp2y * fT, tp1z += tp2z * fT;
                    xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist);
                    fR = 1.0f;
                }
            } else {
                if (fS < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V2, fR, fT);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fT) + V2->x * fT, tp2y = V0->y * (1.0f - fT) + V2->y * fT,
                        tp2z = V0->z * (1.0f - fT) + V2->z * fT;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fS = 0.0f;

                    xLine3LineMinDist(p1, p2, V1, V2, fR0, fT0);
                    tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                        tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                    tp2x = V1->x * (1.0f - fT0) + V2->x * fT0, tp2y = V1->y * (1.0f - fT0) + V2->y * fT0,
                        tp2z = V1->z * (1.0f - fT0) + V2->z * fT0;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                    fS0 = 1.0f - fT0;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }

                    xTri3VecMinDist(V0, V1, V2, p2, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 1.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                }
                else if (fT < 0.0f) {
                    xLine3LineMinDist(p1, p2, V0, V1, fR, fS);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V0->x * (1.0f - fS) + V1->x * fS, tp2y = V0->y * (1.0f - fS) + V1->y * fS,
                        tp2z = V0->z * (1.0f - fS) + V1->z * fS;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fT = 0.0f;

                    xLine3LineMinDist(p1, p2, V1, V2, fR0, fT0);
                    tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0,
                        tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
                    tp2x = V1->x * (1.0f - fT0) + V2->x * fT0, tp2y = V1->y * (1.0f - fT0) + V2->y * fT0,
                        tp2z = V1->z * (1.0f - fT0) + V2->z * fT0;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
                    fS0 = 1.0f - fT0;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }

                    xTri3VecMinDist(V0, V1, V2, p2, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 1.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                } else {
                    xLine3LineMinDist(p1, p2, V1, V2, fR, fT);
                    tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR,
                        tp1z = p1->z * (1.0f - fR) + p2->z * fR;
                    tp2x = V1->x * (1.0f - fT) + V2->x * fT, tp2y = V1->y * (1.0f - fT) + V2->y * fT,
                        tp2z = V1->z * (1.0f - fT) + V2->z * fT;
                    xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
                    fS = 1.0f - fT;

                    xTri3VecMinDist(V0, V1, V2, p2, fS0, fT0);
                    tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0,
                        tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
                    tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
                    tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
                    xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist0);
                    fR0 = 1.0f;
                    if (fSqrDist0 < fSqrDist) {
                        fSqrDist = fSqrDist0;
                        fR = fR0;
                        fS = fS0;
                        fT = fT0;
                    }
                }
            }
        }
    } else {
        xLine3LineMinDist(p1, p2, V0, V1, fR, fS);
        tp1x = p1->x * (1.0f - fR) + p2->x * fR, tp1y = p1->y * (1.0f - fR) + p2->y * fR, tp1z = p1->z * (1.0f - fR) + p2->z * fR;
        tp2x = V0->x * (1.0f - fS) + V1->x * fS, tp2y = V0->y * (1.0f - fS) + V1->y * fS, tp2z = V0->z * (1.0f - fS) + V1->z * fS;
        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist);
        fT = 0.0f;

        xLine3LineMinDist(p1, p2, V0, V2, fR0, fT0);
        tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0, tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
        tp2x = V0->x * (1.0f - fT0) + V2->x * fT0, tp2y = V0->y * (1.0f - fT0) + V2->y * fT0, tp2z = V0->z * (1.0f - fT0) + V2->z * fT0;
        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
        fS0 = 0.0f;
        if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
        }

        xLine3LineMinDist(p1, p2, V1, V2, fR0, fT0);
        tp1x = p1->x * (1.0f - fR0) + p2->x * fR0, tp1y = p1->y * (1.0f - fR0) + p2->y * fR0, tp1z = p1->z * (1.0f - fR0) + p2->z * fR0;
        tp2x = V1->x * (1.0f - fT0) + V2->x * fT0, tp2y = V1->y * (1.0f - fT0) + V2->y * fT0, tp2z = V1->z * (1.0f - fT0) + V2->z * fT0;
        xDIST2(tp1x, tp1y, tp1z, tp2x, tp2y, tp2z, fSqrDist0);
        fS0 = 1.0f - fT0;
        if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
        }

        xTri3VecMinDist(V0, V1, V2, p1, fS0, fT0);
        tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0, tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
        tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
        tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
        xDIST2(p1->x, p1->y, p1->z, tp1x, tp1y, tp1z, fSqrDist0);
        fR0 = 0.0f;
        if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
        }

        xTri3VecMinDist(V0, V1, V2, p2, fS0, fT0);
        tp1x = V0->x * (1.0f - fS0) + V1->x * fS0, tp1y = V0->y * (1.0f - fS0) + V1->y * fS0, tp1z = V0->z * (1.0f - fS0) + V1->z * fS0;
        tp2x = V2->x - V0->x, tp2y = V2->y - V0->y, tp2z = V2->z - V0->z;
        tp1x += tp2x * fT0, tp1y += tp2y * fT0, tp1z += tp2z * fT0;
        xDIST2(p2->x, p2->y, p2->z, tp1x, tp1y, tp1z, fSqrDist0);
        fR0 = 1.0f;
        if (fSqrDist0 < fSqrDist) {
            fSqrDist = fSqrDist0;
            fR = fR0;
            fS = fS0;
            fT = fT0;
        }
    }

    t = fR;
    u = fS;
    v = fT;
}

void xTri3VecMinDist(const xVec3* V0, const xVec3* V1, const xVec3* V2, const xVec3* p, F32& s, F32& t)
{
    F32 kDiffx, kDiffy, kDiffz, edge0x, edge0y, edge0z, edge1x, edge1y, edge1z;

    kDiffx = V0->x - p->x, kDiffy = V0->y - p->y, kDiffz = V0->z - p->z;
    edge0x = V1->x - V0->x, edge0y = V1->y - V0->y, edge0z = V1->z - V0->z;
    edge1x = V2->x - V0->x, edge1y = V2->y - V0->y, edge1z = V2->z - V0->z;
    F32 fA00 = edge0x * edge0x + edge0y * edge0y + edge0z * edge0z;
    F32 fA01 = edge0x * edge1x + edge0y * edge1y + edge0z * edge1z;
    F32 fA11 = edge1x * edge1x + edge1y * edge1y + edge1z * edge1z;
    F32 fB0 = kDiffx * edge0x + kDiffy * edge0y + kDiffz * edge0z;
    F32 fB1 = kDiffx * edge1x + kDiffy * edge1y + kDiffz * edge1z;

    F32 fDet = xabs(fA00 * fA11 - fA01 * fA01);
    F32 fS = fA01 * fB1 - fA11 * fB0;
    F32 fT = fA01 * fB0 - fA00 * fB1;

    if (fS + fT <= fDet) {
        if (fS < 0.0f) {
            if (fT < 0.0f) {
                if (fB0 < 0.0f) {
                    fT = 0.0f;
                    if (-fB0 >= fA00) {
                        fS = 1.0f;
                    } else {
                        fS = -fB0 / fA00;
                    }
                } else {
                    fS = 0.0f;
                    if (fB1 >= 0.0f) {
                        fT = 0.0f;
                    }
                    else if (-fB1 >= fA11) {
                        fT = 1.0f;
                    } else {
                        fT = -fB1 / fA11;
                    }
                }
            } else {
                fS = 0.0f;
                if (fB1 >= 0.0f) {
                    fT = 0.0f;
                }
                else if (-fB1 >= fA11) {
                    fT = 1.0f;
                } else {
                    fT = -fB1 / fA11;
                }
            }
        }
        else if (fT < 0.0f) {
            fT = 0.0f;
            if (fB0 >= 0.0f) {
                fS = 0.0f;
            }
            else if (-fB0 >= fA00) {
                fS = 1.0f;
            } else {
                fS = -fB0 / fA00;
            }
        } else {
            F32 fInvDet = 1.0f / fDet;
            fS *= fInvDet;
            fT *= fInvDet;
        }
    } else {
        F32 fTmp0, fTmp1, fNumer, fDenom;

        if (fS < 0.0f) {
            fTmp0 = fA01 + fB0;
            fTmp1 = fA11 + fB1;
            if (fTmp1 > fTmp0) {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom) {
                    fS = 1.0f;
                    fT = 0.0f;
                } else {
                    fS = fNumer / fDenom;
                    fT = 1.0f - fS;
                }
            } else {
                fS = 0.0f;
                if (fTmp1 <= 0.0f) {
                    fT = 1.0f;
                }
                else if (fB1 >= 0.0f) {
                    fT = 0.0f;
                } else {
                    fT = -fB1 / fA11;
                }
            }
        }
        else if (fT < 0.0f) {
            fTmp0 = fA01 + fB1;
            fTmp1 = fA00 + fB0;
            if (fTmp1 > fTmp0) {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom) {
                    fT = 1.0f;
                    fS = 0.0f;
                } else {
                    fT = fNumer / fDenom;
                    fS = 1.0f - fT;
                }
            } else {
                fT = 0.0f;
                if (fTmp1 <= 0.0f) {
                    fS = 1.0f;
                }
                else if (fB0 >= 0.0f) {
                    fS = 0.0f;
                } else {
                    fS = -fB0 / fA00;
                }
            }
        } else {
            fNumer = fA11 + fB1 - fA01 - fB0;
            if (fNumer <= 0.0f) {
                fS = 0.0f;
                fT = 1.0f;
            } else {
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom) {
                    fS = 1.0f;
                    fT = 0.0f;
                } else {
                    fS = fNumer / fDenom;
                    fT = 1.0f - fS;
                }
            }
        }
    }

    s = fS;
    t = fT;
}

S32 xPointInBox(const xBox* b, const xVec3* p)
{
    return (p->x >= b->lower.x && p->x <= b->upper.x) && (p->y >= b->lower.y && p->y <= b->upper.y) &&
           (p->z >= b->lower.z && p->z <= b->upper.z);
}

void xBoxInitBoundOBB(xBox* o, const xBox* b, const xMat4x3* m)
{
    xVec3 boxcent;
    boxcent.x = 0.5f * (b->lower.x + b->upper.x);
    boxcent.y = 0.5f * (b->lower.y + b->upper.y);
    boxcent.z = 0.5f * (b->lower.z + b->upper.z);

    F32 xmax, ymax, zmax;
    xmax = xabs(m->right.x * (b->upper.x - boxcent.x)), ymax = xabs(m->right.y * (b->upper.x - boxcent.x)),
        zmax = xabs(m->right.z * (b->upper.x - boxcent.x));
    xmax += xabs(m->up.x * (b->upper.y - boxcent.y)), ymax += xabs(m->up.y * (b->upper.y - boxcent.y)),
        zmax += xabs(m->up.z * (b->upper.y - boxcent.y));
    xmax += xabs(m->at.x * (b->upper.z - boxcent.z)), ymax += xabs(m->at.y * (b->upper.z - boxcent.z)),
        zmax += xabs(m->at.z * (b->upper.z - boxcent.z));

    xMat4x3Toworld(&boxcent, m, &boxcent);

    o->lower.x = boxcent.x - xmax;
    o->lower.y = boxcent.y - ymax;
    o->lower.z = boxcent.z - zmax;
    o->upper.x = boxcent.x + xmax;
    o->upper.y = boxcent.y + ymax;
    o->upper.z = boxcent.z + zmax;
}

void xBoxInitBoundCapsule(xBox* b, const xCapsule* c)
{
    if (c->start.x < c->end.x) {
        b->lower.x = c->start.x - c->r;
        b->upper.x = c->end.x + c->r;
    } else {
        b->lower.x = c->end.x - c->r;
        b->upper.x = c->start.x + c->r;
    }

    if (c->start.y < c->end.y) {
        b->lower.y = c->start.y - c->r;
        b->upper.y = c->end.y + c->r;
    } else {
        b->lower.y = c->end.y - c->r;
        b->upper.y = c->start.y + c->r;
    }

    if (c->start.z < c->end.z) {
        b->lower.z = c->start.z - c->r;
        b->upper.z = c->end.z + c->r;
    } else {
        b->lower.z = c->end.z - c->r;
        b->upper.z = c->start.z + c->r;
    }
}

// UNUSED: needed to match (code from TSSM GC, BFBB GC)
void xBoxFromCone(xBox& box, const xVec3& center, const xVec3& dir, F32 dist, F32 r1, F32 r2)
{
    xBox temp;
    xVec3 circleCenter;

    xBoxFromCircle(box, center, dir, r1);
    xVec3AddScaled(circleCenter, center, dir, dist);
    xBoxFromCircle(temp, circleCenter, dir, r2);
    xBoxUnion(box, box, temp);
}

void xMat3x3Init(xMat3x3* m, const xVec3* _right, const xVec3* _up, const xVec3* _at)
{
    xVec3Copy(&m->right, _right);
    xVec3Copy(&m->up, _up);
    xVec3Copy(&m->at, _at);
    m->flags = 0;
}

void xMat3x3Normalize(xMat3x3* o, const xMat3x3* m)
{
    xVec3Normalize(&o->right, &m->right);
    xVec3Normalize(&o->up, &m->up);
    xVec3Normalize(&o->at, &m->at);
}

void xMat3x3GetScale(const xMat3x3* m, xVec3* s)
{
    s->x = xVec3Length(&m->right);
    s->y = xVec3Length(&m->up);
    s->z = xVec3Length(&m->at);
}

void xMat3x3GetEuler(const xMat3x3* m, xVec3* a)
{
    F32 pitch = -xasin(m->at.y), yaw, roll;

    if (pitch < PI / 2) {
        if (pitch > -PI / 2) {
            yaw = xatan2(m->at.x, m->at.z);
            roll = xatan2(m->right.y, m->up.y);
        } else {
            yaw = -xatan2(-m->up.x, m->right.x);
            roll = 0.0f;
        }
    } else {
        yaw = xatan2(-m->up.x, m->right.x);
        roll = 0.0f;
    }

    a->x = yaw;
    a->y = pitch;
    a->z = roll;
}

void xMat4x3MoveLocalRight(xMat4x3* m, F32 mag)
{
    m->pos.x += m->right.x * mag;
    m->pos.y += m->right.y * mag;
    m->pos.z += m->right.z * mag;
}

void xMat4x3MoveLocalUp(xMat4x3* m, F32 mag)
{
    m->pos.x += m->up.x * mag;
    m->pos.y += m->up.y * mag;
    m->pos.z += m->up.z * mag;
}

void xMat4x3MoveLocalAt(xMat4x3* m, F32 mag)
{
    m->pos.x += m->at.x * mag;
    m->pos.y += m->at.y * mag;
    m->pos.z += m->at.z * mag;
}

F32 xMat3x3LookVec(xMat3x3* m, const xVec3* at)
{
    F32 vec_len = xVec3Normalize(&m->at, at);

    xVec3Inv(&m->at, &m->at);

    if (xabs(1.0f - m->at.y) < 0.00001f) {
        m->right.x = 1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = 0.0f;
        m->up.y = 0.0f;
        m->up.z = 1.0f;
        m->at.x = 0.0f;
        m->at.y = -1.0f;
        m->at.z = 0.0f;
        return vec_len;
    }

    if (xabs(1.0f + m->at.y) < 0.00001f) {
        m->right.x = -1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = 0.0f;
        m->up.y = 0.0f;
        m->up.z = -1.0f;
        m->at.x = 0.0f;
        m->at.y = 1.0f;
        m->at.z = 0.0f;
        return vec_len;
    }

    if (xabs(at->z) < 0.00001f && xabs(at->x) < 0.00001f) {
        m->right.x = 1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = 0.0f;
        m->up.y = 1.0f;
        m->up.z = 0.0f;
        m->at.x = 0.0f;
        m->at.y = 0.0f;
        m->at.z = 1.0f;
        return 0.0f;
    }

    m->right.x = m->at.z;
    m->right.y = 0.0f;
    m->right.z = -m->at.x;

    xVec3Normalize(&m->right, &m->right);

    xVec3Cross(&m->up, &m->at, &m->right);

    xVec3Cross(&m->right, &m->up, &m->at);

    m->flags = 0;

    return vec_len;
}

void xMat3x3LookVec2(xMat3x3* m, const xVec3* at)
{
    xMat3x3LookVec(m, at);

    if (xabs(m->right.x) < 0.00001f) {
        m->right.x = 0.0f;
    } else if (xabs(m->right.x - 1.0f) < 0.00001f) {
        m->right.x = 1.0f;
    }

    if (xabs(m->right.y) < 0.00001f) {
        m->right.y = 0.0f;
    } else if (xabs(m->right.y - 1.0f) < 0.00001f) {
        m->right.y = 1.0f;
    }

    if (xabs(m->right.z) < 0.00001f) {
        m->right.z = 0.0f;
    } else if (xabs(m->right.z - 1.0f) < 0.00001f) {
        m->right.z = 1.0f;
    }

    if (xabs(m->up.x) < 0.00001f) {
        m->up.x = 0.0f;
    } else if (xabs(m->up.x - 1.0f) < 0.00001f) {
        m->up.x = 1.0f;
    }

    if (xabs(m->up.y) < 0.00001f) {
        m->up.y = 0.0f;
    } else if (xabs(m->up.y - 1.0f) < 0.00001f) {
        m->up.y = 1.0f;
    }

    if (xabs(m->up.z) < 0.00001f) {
        m->up.z = 0.0f;
    } else if (xabs(m->up.z - 1.0f) < 0.00001f) {
        m->up.z = 1.0f;
    }

    if (xabs(m->at.x) < 0.00001f) {
        m->at.x = 0.0f;
    } else if (xabs(m->at.x - 1.0f) < 0.00001f) {
        m->at.x = 1.0f;
    }

    if (xabs(m->at.y) < 0.00001f) {
        m->at.y = 0.0f;
    } else if (xabs(m->at.y - 1.0f) < 0.00001f) {
        m->at.y = 1.0f;
    }

    if (xabs(m->at.z) < 0.00001f) {
        m->at.z = 0.0f;
    } else if (xabs(m->at.z - 1.0f) < 0.00001f) {
        m->at.z = 1.0f;
    }
}

F32 xMat3x3LookVec4(xMat3x3& mat, F32 at_x, F32 at_y, F32 at_z, const xVec3& hint_right)
{
    F32 mag2 = at_x * at_x + at_y * at_y + at_z * at_z;
    if (mag2 < 0.0001f * 0.0001f)
    { mat = g_I3; return 0.0f; }
    F32 mag = xsqrt(mag2), imag = 1.0f / mag;
    mat.at.assign(at_x * imag, at_y * imag, at_z * imag);

    if (xabs(mat.at.dot(hint_right)) > 0.99999f) {
        if (xabs(mat.at.y) > 0.99999f)
            mat.right.assign(0.0f, mat.at.z, -mat.at.y);
        else
            mat.right.assign(-mat.at.z, 0.0f, mat.at.x);
        mat.right.normalize();
        mat.up = mat.right.cross(mat.at);
    } else {
        mat.up.cross(mat.at, hint_right);
        mat.up.normalize();
        mat.right.cross(mat.up, mat.at);
    }

    return mag;
}

F32 xMat3x3LookVec5(xMat3x3& mat, F32 at_x, F32 at_y, F32 at_z, const xVec3& hint_up)
{
    F32 mag2 = at_x * at_x + at_y * at_y + at_z * at_z;
    if (mag2 < 0.0001f * 0.0001f)
    { mat = g_I3; return 0.0f; }
    F32 mag = xsqrt(mag2), imag = 1.0f / mag;
    mat.at.assign(at_x * imag, at_y * imag, at_z * imag);

    if (xabs(mat.at.dot(hint_up)) > 0.99999f) {
        if (xabs(mat.at.y) > 0.99999f)
            mat.right.assign(0.0f, mat.at.z, -mat.at.y);
        else
            mat.right.assign(-mat.at.z, 0.0f, mat.at.x);
        mat.right.normalize();
        mat.up = mat.right.cross(mat.at);
    } else {
        mat.right.cross(mat.at, hint_up);
        mat.right.normalize();
        mat.up.cross(mat.at, mat.right);
    }

    return mag;
}

void xMat3x3Euler(xMat3x3* m, const xVec3* ypr)
{
    xMat3x3Euler(m, ypr->x, ypr->y, ypr->z);
}

void xMat3x3Euler(xMat3x3* m, F32 yaw, F32 pitch, F32 roll)
{
    F32 sy = isin(yaw);
    F32 cy = icos(yaw);
    F32 sp = isin(pitch);
    F32 cp = icos(pitch);
    F32 sr = isin(roll);
    F32 cr = icos(roll);

    m->right.x = cy * cr + sy * sp * sr;
    m->right.y = cp * sr;
    m->right.z = -sy * cr + cy * sp * sr;
    m->up.x = -cy * sr + sy * sp * cr;
    m->up.y = cp * cr;
    m->up.z = sy * sr + cy * sp * cr;
    m->at.x = sy * cp;
    m->at.y = -sp;
    m->at.z = cy * cp;
    m->flags = 0;
}

void xMat3x3RotC(xMat3x3* m, F32 _x, F32 _y, F32 _z, F32 t)
{
    if (t == 0.0f) {
        xMat3x3Identity(m);
        return;
    }

    F32 a = icos(t);
    F32 b = isin(t);
    F32 c = 1.0f - a;

    m->right.x = c * _x * _x + a;
    m->right.y = b * _z + c * _x * _y;
    m->right.z = -b * _y + c * _z * _x;
    m->up.x = -b * _z + c * _x * _y;
    m->up.y = c * _y * _y + a;
    m->up.z = b * _x + c * _y * _z;
    m->at.x = b * _y + c * _z * _x;
    m->at.y = -b * _x + c * _y * _z;
    m->at.z = c * _z * _z + a;
    m->flags = 0;
}

void xMat3x3RotX(xMat3x3* m, F32 t)
{
    F32 a = icos(t);
    F32 b = isin(t);
    xVec3Copy(&m->right, &g_X3);
    xVec3Init(&m->up, 0.0f, a, b);
    xVec3Init(&m->at, 0.0f, -b, a);
    m->flags = 0;
}

void xMat3x3RotY(xMat3x3* m, F32 t)
{
    F32 a = icos(t);
    F32 b = isin(t);
    xVec3Init(&m->right, a, 0.0f, -b);
    xVec3Copy(&m->up, &g_Y3);
    xVec3Init(&m->at, b, 0.0f, a);
    m->flags = 0;
}

void xMat3x3RotZ(xMat3x3* m, F32 t)
{
    F32 a = icos(t);
    F32 b = isin(t);
    xVec3Init(&m->right, a, b, 0.0f);
    xVec3Init(&m->up, -b, a, 0.0f);
    xVec3Copy(&m->at, &g_Z3);
    m->flags = 0;
}

void xMat3x3ScaleC(xMat3x3* m, F32 x, F32 y, F32 z)
{
    xVec3Init(&m->right, x, 0.0f, 0.0f);
    xVec3Init(&m->up, 0.0f, y, 0.0f);
    xVec3Init(&m->at, 0.0f, 0.0f, z);
    m->flags = 0;
}

void xMat3x3LMulRotY(xMat3x3* o, const xMat3x3* m, F32 t)
{
    F32 a = icos(t);
    F32 b = isin(t);

    if (o == m) {
        F32 temp = a * o->right.x - b * o->at.x;
        o->at.x = a * o->at.x + b * o->right.x;
        o->right.x = temp;

        temp = a * o->right.y - b * o->at.y;
        o->at.y = a * o->at.y + b * o->right.y;
        o->right.y = temp;

        temp = a * o->right.z - b * o->at.z;
        o->at.z = a * o->at.z + b * o->right.z;
        o->right.z = temp;
    } else {
        xVec3Copy(&o->up, &m->up);
        o->right.x = a * m->right.x - b * m->at.x;
        o->at.x = a * m->at.x + b * m->right.x;
        o->right.y = a * m->right.y - b * m->at.y;
        o->at.y = a * m->at.y + b * m->right.y;
        o->right.z = a * m->right.z - b * m->at.z;
        o->at.z = a * m->at.z + b * m->right.z;
        o->flags = 0;
    }
}

void xMat3x3RMulRotY(xMat3x3* o, const xMat3x3* m, F32 t)
{
    F32 a = icos(t);
    F32 b = isin(t);

    if (o == m) {
        F32 temp = a * o->right.x + b * o->right.z;
        o->right.z = a * o->right.z - b * o->right.x;
        o->right.x = temp;

        temp = a * o->up.x + b * o->up.z;
        o->up.z = a * o->up.z - b * o->up.x;
        o->up.x = temp;

        temp = a * o->at.x + b * o->at.z;
        o->at.z = a * o->at.z - b * o->at.x;
        o->at.x = temp;
    } else {
        o->right.x = a * m->right.x + b * m->right.z;
        o->right.y = m->right.y;
        o->right.z = a * m->right.z - b * m->right.x;
        o->up.x = a * m->up.x + b * m->up.z;
        o->up.y = m->up.y;
        o->up.z = a * m->up.z - b * m->up.x;
        o->at.x = a * m->at.x + b * m->at.z;
        o->at.y = m->at.y;
        o->at.z = a * m->at.z - b * m->at.x;
        o->flags = 0;
    }
}

#define xSWAPF(a, b)                            \
do {                                            \
    F32 temp = (a);                             \
    (a) = (b);                                  \
    (b) = temp;                                 \
} while (0)

void xMat3x3Transpose(xMat3x3* o, const xMat3x3* m)
{
    if (o == m) {
        xSWAPF(o->right.y, o->up.x);
        xSWAPF(o->right.z, o->at.x);
        xSWAPF(o->up.z, o->at.y);
    } else {
        o->right.x = m->right.x;
        o->right.y = m->up.x;
        o->right.z = m->at.x;
        o->up.x = m->right.y;
        o->up.y = m->up.y;
        o->up.z = m->at.y;
        o->at.x = m->right.z;
        o->at.y = m->up.z;
        o->at.z = m->at.z;
        o->flags = 0;
    }
}

void xMat3x3Mul(xMat3x3* o, const xMat3x3* a, const xMat3x3* b)
{
    xMat3x3 temp;
    xMat3x3* tp;
    U32 usetemp = (o == a || o == b);

    if (usetemp) {
        tp = &temp;
    } else {
        tp = o;
    }

    tp->right.x = a->right.x * b->right.x + a->right.y * b->up.x + a->right.z * b->at.x;
    tp->right.y = a->right.x * b->right.y + a->right.y * b->up.y + a->right.z * b->at.y;
    tp->right.z = a->right.x * b->right.z + a->right.y * b->up.z + a->right.z * b->at.z;
    tp->up.x = a->up.x * b->right.x + a->up.y * b->up.x + a->up.z * b->at.x;
    tp->up.y = a->up.x * b->right.y + a->up.y * b->up.y + a->up.z * b->at.y;
    tp->up.z = a->up.x * b->right.z + a->up.y * b->up.z + a->up.z * b->at.z;
    tp->at.x = a->at.x * b->right.x + a->at.y * b->up.x + a->at.z * b->at.x;
    tp->at.y = a->at.x * b->right.y + a->at.y * b->up.y + a->at.z * b->at.y;
    tp->at.z = a->at.x * b->right.z + a->at.y * b->up.z + a->at.z * b->at.z;
    tp->flags = 0;

    if (usetemp) {
        xMat3x3Copy(o, tp);
    }
}

void xMat3x3LMulVec(xVec3* o, const xMat3x3* m, const xVec3* v)
{
    F32 x = m->right.x * v->x + m->right.y * v->y + m->right.z * v->z;
    F32 y = m->up.x * v->x + m->up.y * v->y + m->up.z * v->z;
    F32 z = m->at.x * v->x + m->at.y * v->y + m->at.z * v->z;
    o->x = x, o->y = y, o->z = z;
}

void xMat3x3Tolocal(xVec3* o, const xMat3x3* m, const xVec3* v)
{
    F32 sx2 = m->right.x * m->right.x + m->right.y * m->right.y + m->right.z * m->right.z;
    F32 sy2 = m->up.x * m->up.x + m->up.y * m->up.y + m->up.z * m->up.z;
    F32 sz2 = m->at.x * m->at.x + m->at.y * m->at.y + m->at.z * m->at.z;

    xMat3x3LMulVec(o, m, v);

    o->x /= sx2;
    o->y /= sy2;
    o->z /= sz2;
}

void xMat4x3Rot(xMat4x3* m, const xVec3* a, F32 t, const xVec3* p)
{
    xMat4x3 temp;

    xMat3x3RotC(m, a->x, a->y, a->z, t);
    xVec3Copy(&m->pos, p);

    xMat3x3Identity(&temp);
    xVec3Inv(&temp.pos, p);
    xMat4x3Mul(m, &temp, m);
}

void xMat4x3Mul(xMat4x3* o, const xMat4x3* a, const xMat4x3* b)
{
    xVec3 pos;

    xMat4x3Toworld(&pos, b, &a->pos);
    xMat3x3Mul(o, a, b);
    xVec3Copy(&o->pos, &pos);
}

void xQuatInit(xQuat* q, F32 s, const xVec3* v)
{
    q->s = s;
    xVec3Copy(&q->v, v);
}

void xQuatFromMat(xQuat* q, const xMat3x3* m)
{
    F32 *mp = (F32*)m, *qvp = (F32*)q;
    F32 tr = m->right.x + m->up.y + m->at.z, root;
    static S32 nxt[3] = { 1, 2, 0 };

    if (tr > 0.0f) {
        root = xsqrt(1.0f + tr);
        q->s = 0.5f * root;
        root = 0.5f / root;
        q->v.x = root * (m->at.y - m->up.z);
        q->v.y = root * (m->right.z - m->at.x);
        q->v.z = root * (m->up.x - m->right.y);
    } else {
        S32 i = 0;
        if (mp[5] > mp[0]) {
            i = 1;
        }
        if (mp[10] > mp[i * 5]) {
            i = 2;
        }
        S32 j = nxt[i], k = nxt[j];

        root = xsqrt(1.0f + (mp[i * 5] - mp[j * 5] - mp[k * 5]));
        if (xabs(root) < 0.00001f) {
            xQuatCopy(q, &g_IQ);
            return;
        }

        qvp[i] = 0.5f * root;
        root = 0.5f / root;
        q->s = root * (mp[k * 4 + j] - mp[j * 4 + k]);
        qvp[j] = root * (mp[j * 4 + i] + mp[i * 4 + j]);
        qvp[k] = root * (mp[k * 4 + i] + mp[i * 4 + k]);

        if (q->s < 0.0f) {
            xQuatFlip(q, q);
        }
    }
}

void xQuatFromAxisAngle(xQuat* q, const xVec3* a, F32 t)
{
    if (t == 0.0f) {
        xQuatCopy(q, &g_IQ);
    } else {
        F32 t_2 = 0.5f * t;
        F32 st_2 = isin(t_2);
        q->s = icos(t_2);
        xVec3SMul(&q->v, a, st_2);
    }
}

void xQuatToMat(const xQuat* q, xMat3x3* m)
{
    F32 tx = 2.0f * q->v.x;
    F32 ty = 2.0f * q->v.y;
    F32 tz = 2.0f * q->v.z;
    F32 tsx = tx * q->s;
    F32 tsy = ty * q->s;
    F32 tsz = tz * q->s;
    F32 txx = tx * q->v.x;
    F32 txy = ty * q->v.x;
    F32 txz = tz * q->v.x;
    F32 tyy = ty * q->v.y;
    F32 tyz = tz * q->v.y;
    F32 tzz = tz * q->v.z;

    m->right.x = 1.0f - tyy - tzz;
    m->right.y = txy - tsz;
    m->right.z = txz + tsy;
    m->up.x = txy + tsz;
    m->up.y = 1.0f - tzz - txx;
    m->up.z = tyz - tsx;
    m->at.x = txz - tsy;
    m->at.y = tyz + tsx;
    m->at.z = 1.0f - txx - tyy;
    m->flags = 0;
}

void xQuatToAxisAngle(const xQuat* q, xVec3* a, F32* t)
{
    *t = 2.0f * xacos(q->s);
    xVec3Normalize(a, &q->v);
}

F32 xQuatNormalize(xQuat* o, const xQuat* q)
{
    F32 one_len, len, len2 = xQuatLength2(q);

    if (len2 == 1.0f) {
        if (o != q) {
            xQuatCopy(o, q);
        }
        return 1.0f;
    }

    if (len2 == 0.0f) {
        if (o != q) {
            xQuatCopy(o, &g_IQ);
        }
        return 0.0f;
    }

    len = xsqrt(len2);
    one_len = 1.0f / len;
    xQuatSMul(o, q, one_len);
    return len;
}

// UNUSED: needed to match (speculative code)
void xQuatInv(xQuat* o, const xQuat* q)
{
    F32 len2 = xQuatLength2(q);
    F32 one_len = 1.0f / len2;
    xQuatConj(o, q);
    xQuatSMul(o, o, one_len);
}

void xQuatSlerp(xQuat* o, const xQuat* a, const xQuat* b, F32 t)
{
    F32 asph, bsph, one_sintheta, theta, abdot;

    xQuat aerp, berp, b2;
    abdot = xQuatDot(a, b);

    if (abdot < 0.0f) {
        abdot = -abdot;
        b2.v.x = -b->v.x, b2.v.y = -b->v.y, b2.v.z = -b->v.z, b2.s = -b->s;
        b = &b2;
    }

    if (abdot >= 0.999f) {
        asph = 1.0f - t;
        bsph = t;
    } else {
        theta = xacos(abdot);
        one_sintheta = 1.0f / isin(theta);
        asph = one_sintheta * isin((1.0f - t) * theta);
        bsph = one_sintheta * isin(t * theta);
    }

    xQuatSMul(&aerp, a, asph);
    xQuatSMul(&berp, b, bsph);
    xQuatAdd(o, &aerp, &berp);

    xQuatNormalize(o, o);
}

void xQuatMul(xQuat* o, const xQuat* a, const xQuat* b)
{
    F32 _s = a->s * b->s - a->v.x * b->v.x - a->v.y * b->v.y - a->v.z * b->v.z;
    F32 _x = a->s * b->v.x + a->v.x * b->s + a->v.y * b->v.z - a->v.z * b->v.y;
    F32 _y = a->s * b->v.y + a->v.y * b->s + a->v.z * b->v.x - a->v.x * b->v.z;
    F32 _z = a->s * b->v.z + a->v.z * b->s + a->v.x * b->v.y - a->v.y * b->v.x;
    o->v.x = _x;
    o->v.y = _y;
    o->v.z = _z;
    o->s = _s;

    xQuatNormalize(o, o);
}

void xQuatDiff(xQuat* o, const xQuat* a, const xQuat* b)
{
    xQuatConj(o, a);
    xQuatMul(o, o, b);
    if (o->s < 0.0f) {
        xQuatFlip(o, o);
    }
}

bool xVec3Rotate(xMat3x3* o, const xVec3* a, const xVec3* b, F32 l)
{
    F32 dot = xVec3Dot(a, b);
    if (dot >= 0.9999f) {
        return true;
    }

    xVec3 axis;
    xVec3Cross(&axis, a, b);
    xVec3Normalize(&axis, &axis);

    F32 angle = xacos(dot);
    if (angle > l) {
        angle = l;
    }

    xMat3x3Rot(o, &axis, angle);

    return false;
}

bool xVec3Eq(const xVec3& a, const xVec3& b, F32 tol)
{
    const F32 *fa = (const F32*)&a, *fb = (const F32*)&b;

    for (S32 i = 0; i < 3; i++) {
        if (!xeq(fa[i], fb[i], tol))
            return false;
    }

    return true;
}

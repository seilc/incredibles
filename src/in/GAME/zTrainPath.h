#ifndef ZTRAINPATH_H
#define ZTRAINPATH_H

#include "xDynAsset.h"
#include "xSplineAsset.h"

struct zTrainPathJunctionAsset : xDynAsset
{
    U32 in_spline;
    bool in_is_from_forward;
    U32 out1_spline;
    bool out1_is_forward;
    U32 out2_spline;
    bool out2_is_forward;
};

struct zTrainPathJunction : xBase
{
    zTrainPathJunctionAsset* junct_asset;
    x::NURBS* in_spline;
    x::NURBS* out1_spline;
    x::NURBS* out2_spline;
    bool use_out1;
    zTrainPathJunction* next;

    static zTrainPathJunction* all_junctions;

    static void AddJunction(zTrainPathJunction* junction);
    static void InitDone();
    static zTrainPathJunction* GetPathEnd(x::NURBS* curve, bool along_increasing_u);
    static void Init(xBase& data, xDynAsset& asset, size_t);
    static void Event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                      xBase* toParamWidget, U32 toParamWidgetID);
    static void Reset();

    void Init(zTrainPathJunctionAsset* asset);
    void GetCurrentOut(x::NURBS*& out_curve, bool& travel_increasing_u);
    void Event(xBase*, U32 toEvent, const F32*, xBase*, U32);
};

#endif
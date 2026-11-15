#ifndef ZDISPATCHER_H
#define ZDISPATCHER_H

#include "xBase.h"

void ZDSP_elcb_event(xBase* from, xBase* xb, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                     U32 toParamWidgetID);

#endif

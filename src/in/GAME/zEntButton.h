#ifndef ZENTBUTTON_H
#define ZENTBUTTON_H

#include "types.h"

typedef struct _zEntButton zEntButton;

void zEntButton_Press(zEntButton* ent, U32 mask);
void zEntButton_Hold(zEntButton* ent, U32 mask);

#endif

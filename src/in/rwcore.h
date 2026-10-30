#include "types.h"
#include "../rwsdk/include/rwcore.h"

RwCamera* RwCameraCreateWrapper();
#define RwCameraCreate RwCameraCreateWrapper

RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera);
#define RwCameraBeginUpdate RwCameraBeginUpdateWrapper

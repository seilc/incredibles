#ifndef RPUSRDAT_H
#define RPUSRDAT_H

#include "rwcore.h"
#include "rpworld.h"

enum RpUserDataFormat
{
    rpNAUSERDATAFORMAT = 0,
    rpINTUSERDATA,
    rpREALUSERDATA,
    rpSTRINGUSERDATA,
    rpUSERDATAFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RpUserDataFormat RpUserDataFormat;

typedef struct RpUserDataArray RpUserDataArray;

struct RpUserDataArray
{
    RwChar* name;
    RpUserDataFormat format;
    RwInt32 numElements;
    void* data;
};

#ifdef __cplusplus
extern "C" {
#endif

RwInt32 RpGeometryAddUserDataArray(RpGeometry* geometry, RwChar* name, RpUserDataFormat format, RwInt32 numElements);
RpUserDataArray* RpGeometryGetUserDataArray(const RpGeometry* geometry, RwInt32 data);
RwInt32 RpGeometryGetUserDataArrayCount(const RpGeometry* geometry);

#ifdef __cplusplus
}
#endif

#endif

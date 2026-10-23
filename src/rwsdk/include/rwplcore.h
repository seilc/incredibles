#ifndef RWPLCORE_H
#define RWPLCORE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef long RwFixed;
typedef int RwInt32;
typedef unsigned int RwUInt32;
typedef short RwInt16;
typedef unsigned short RwUInt16;
typedef unsigned char RwUInt8;
typedef signed char RwInt8;
typedef char RwChar;
typedef float RwReal;
typedef RwInt32 RwBool;
typedef long long RwInt64;
typedef unsigned long long RwUInt64;

#define RWFORCEENUMSIZEINT ((RwInt32)((~((RwUInt32)0)) >> 1))

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !FALSE
#endif

#define MACRO_START do
#define MACRO_STOP while (0)

typedef struct RwDevice RwDevice;
typedef struct RwFreeList RwFreeList;
typedef struct RwGlobals RwGlobals;
typedef struct RwLLLink RwLLLink;
typedef struct RwLine RwLine;
typedef struct RwLinkList RwLinkList;
typedef struct RwMemoryFunctions RwMemoryFunctions;
typedef struct RwMetrics RwMetrics;
typedef struct RwObject RwObject;
typedef struct RwPlane RwPlane;
typedef struct RwRGBA RwRGBA;
typedef struct RwRGBAReal RwRGBAReal;
typedef struct RwResEntry RwResEntry;
typedef struct RwSphere RwSphere;
typedef struct RwStream RwStream;
typedef struct RwStreamCustom RwStreamCustom;
typedef union RwStreamFile RwStreamFile;
typedef struct RwStreamMemory RwStreamMemory;
typedef union RwStreamUnion RwStreamUnion;
typedef struct RwStringFunctions RwStringFunctions;
typedef struct RwSurfaceProperties RwSurfaceProperties;
typedef struct RwTexCoords RwTexCoords;
typedef struct RwV2d RwV2d;
typedef struct RwV3d RwV3d;
typedef struct RwVideoMode RwVideoMode;
typedef struct rwGameCube2DVertex rwGameCube2DVertex;
typedef struct rxReq rxReq;

typedef struct RwMatrixTag RwMatrix;
typedef rwGameCube2DVertex RwIm2DVertex;
typedef RwUInt16 RxVertexIndex;
typedef RxVertexIndex RwImVertexIndex;

enum RwMemoryHintDuration
{
    rwMEMHINTDUR_NADURATION = 0x0,
    rwMEMHINTDUR_FUNCTION = 0x10000,
    rwMEMHINTDUR_FRAME = 0x20000,
    rwMEMHINTDUR_EVENT = 0x30000,
    rwMEMHINTDUR_GLOBAL = 0x40000,
    rwMEMHINTDUR_MASK = 0xFF0000,
    rwMEMHINTDURFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwMemoryHintDuration RwMemoryHintDuration;

enum RwMatrixType
{
    rwMATRIXTYPENORMAL = 0x1,
    rwMATRIXTYPEORTHOGONAL = 0x2,
    rwMATRIXTYPEORTHONORMAL = 0x3,
    rwMATRIXTYPEMASK = 0x3,
    rwMATRIXTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwMatrixType RwMatrixType;

enum RwMatrixFlag
{
    rwMATRIXINTERNALIDENTITY = 0x20000,
    rwMATRIXFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwMatrixFlag RwMatrixFlag;

struct RwV2d
{
    RwReal x;
    RwReal y;
};

struct RwV3d
{
    RwReal x;
    RwReal y;
    RwReal z;
};

struct RwSphere
{
    RwV3d center;
    RwReal radius;
};

struct RwLine
{
    RwV3d start;
    RwV3d end;
};

enum RwTextureCoordinateIndex
{
    rwNARWTEXTURECOORDINATEINDEX,
    rwTEXTURECOORDINATEINDEX0,
    rwTEXTURECOORDINATEINDEX1,
    rwTEXTURECOORDINATEINDEX2,
    rwTEXTURECOORDINATEINDEX3,
    rwTEXTURECOORDINATEINDEX4,
    rwTEXTURECOORDINATEINDEX5,
    rwTEXTURECOORDINATEINDEX6,
    rwTEXTURECOORDINATEINDEX7,
    rwTEXTURECOORDINATEINDEXFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwTextureCoordinateIndex RwTextureCoordinateIndex;

struct RwTexCoords
{
    RwReal u;
    RwReal v;
};

struct RwLLLink
{
    RwLLLink* next;
    RwLLLink* prev;
};

struct RwLinkList
{
    RwLLLink link;
};

struct RwSurfaceProperties
{
    RwReal ambient;
    RwReal specular;
    RwReal diffuse;
};

struct RwPlane
{
    RwV3d normal;
    RwReal distance;
};

struct RwMemoryFunctions
{
    void* (*rwmalloc)(unsigned long, RwUInt32);
    void (*rwfree)(void*);
    void* (*rwrealloc)(void*, unsigned long, RwUInt32);
    void* (*rwcalloc)(unsigned long, unsigned long, RwUInt32);
};

enum RwStreamType
{
    rwNASTREAM,
    rwSTREAMFILE,
    rwSTREAMFILENAME,
    rwSTREAMMEMORY,
    rwSTREAMCUSTOM,
    rwSTREAMTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwStreamType RwStreamType;

enum RwStreamAccessType
{
    rwNASTREAMACCESS,
    rwSTREAMREAD,
    rwSTREAMWRITE,
    rwSTREAMAPPEND,
    rwSTREAMACCESSTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwStreamAccessType RwStreamAccessType;

struct RwStreamMemory
{
    RwUInt32 position;
    RwUInt32 nSize;
    RwUInt8* memBlock;
};

union RwStreamFile
{
    void* fpFile;
    void* constfpFile;
};

struct RwStreamCustom
{
    RwInt32 (*sfnclose)(void*);
    RwUInt32 (*sfnread)(void*, void*, RwUInt32);
    RwInt32 (*sfnwrite)(void*, void*, RwUInt32);
    RwInt32 (*sfnskip)(void*, RwUInt32);
    void* data;
};

union RwStreamUnion
{
    RwStreamMemory memory;
    RwStreamFile file;
    RwStreamCustom custom;
};

struct RwStream
{
    RwStreamType type;
    RwStreamAccessType accessType;
    RwInt32 position;
    RwStreamUnion Type;
    RwBool rwOwned;
};

enum RwOpCombineType
{
    rwCOMBINEREPLACE,
    rwCOMBINEPRECONCAT,
    rwCOMBINEPOSTCONCAT,
    rwOPCOMBINETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwOpCombineType RwOpCombineType;

struct RwMatrixTag
{
    RwV3d right;
    RwUInt32 flags;
    RwV3d up;
    RwUInt32 pad1;
    RwV3d at;
    RwUInt32 pad2;
    RwV3d pos;
    RwUInt32 pad3;
};

struct RwRGBAReal
{
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal alpha;
};

struct RwRGBA
{
    RwUInt8 red;
    RwUInt8 green;
    RwUInt8 blue;
    RwUInt8 alpha;
};

enum RwDebugType
{
    rwNADEBUGTYPE,
    rwDEBUGASSERT,
    rwDEBUGERROR,
    rwDEBUGMESSAGE,
    rwDEBUGTRACE,
    rwDEBUGTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwDebugType RwDebugType;

struct rwGameCube2DVertex
{
    RwReal x;
    RwReal y;
    RwReal z;
    RwUInt8 r;
    RwUInt8 g;
    RwUInt8 b;
    RwUInt8 a;
    RwReal u;
    RwReal v;
};

enum RwRenderState
{
    rwRENDERSTATENARENDERSTATE,
    rwRENDERSTATETEXTURERASTER,
    rwRENDERSTATETEXTUREADDRESS,
    rwRENDERSTATETEXTUREADDRESSU,
    rwRENDERSTATETEXTUREADDRESSV,
    rwRENDERSTATETEXTUREPERSPECTIVE,
    rwRENDERSTATEZTESTENABLE,
    rwRENDERSTATESHADEMODE,
    rwRENDERSTATEZWRITEENABLE,
    rwRENDERSTATETEXTUREFILTER,
    rwRENDERSTATESRCBLEND,
    rwRENDERSTATEDESTBLEND,
    rwRENDERSTATEVERTEXALPHAENABLE,
    rwRENDERSTATEBORDERCOLOR,
    rwRENDERSTATEFOGENABLE,
    rwRENDERSTATEFOGCOLOR,
    rwRENDERSTATEFOGTYPE,
    rwRENDERSTATEFOGDENSITY,
    rwRENDERSTATECULLMODE = 20,
    rwRENDERSTATESTENCILENABLE,
    rwRENDERSTATESTENCILFAIL,
    rwRENDERSTATESTENCILZFAIL,
    rwRENDERSTATESTENCILPASS,
    rwRENDERSTATESTENCILFUNCTION,
    rwRENDERSTATESTENCILFUNCTIONREF,
    rwRENDERSTATESTENCILFUNCTIONMASK,
    rwRENDERSTATESTENCILFUNCTIONWRITEMASK,
    rwRENDERSTATEALPHATESTFUNCTION,
    rwRENDERSTATEALPHATESTFUNCTIONREF,
    rwRENDERSTATEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwRenderState RwRenderState;

enum RwShadeMode
{
    rwSHADEMODENASHADEMODE,
    rwSHADEMODEFLAT,
    rwSHADEMODEGOURAUD,
    rwSHADEMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwShadeMode RwShadeMode;

enum RwTextureFilterMode
{
    rwFILTERNAFILTERMODE,
    rwFILTERNEAREST,
    rwFILTERLINEAR,
    rwFILTERMIPNEAREST,
    rwFILTERMIPLINEAR,
    rwFILTERLINEARMIPNEAREST,
    rwFILTERLINEARMIPLINEAR,
    rwTEXTUREFILTERMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwTextureFilterMode RwTextureFilterMode;

enum RwFogType
{
    rwFOGTYPENAFOGTYPE,
    rwFOGTYPELINEAR,
    rwFOGTYPEEXPONENTIAL,
    rwFOGTYPEEXPONENTIAL2,
    rwFOGTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwFogType RwFogType;

enum RwBlendFunction
{
    rwBLENDNABLEND,
    rwBLENDZERO,
    rwBLENDONE,
    rwBLENDSRCCOLOR,
    rwBLENDINVSRCCOLOR,
    rwBLENDSRCALPHA,
    rwBLENDINVSRCALPHA,
    rwBLENDDESTALPHA,
    rwBLENDINVDESTALPHA,
    rwBLENDDESTCOLOR,
    rwBLENDINVDESTCOLOR,
    rwBLENDSRCALPHASAT,
    rwBLENDFUNCTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwBlendFunction RwBlendFunction;

enum RwTextureAddressMode
{
    rwTEXTUREADDRESSNATEXTUREADDRESS,
    rwTEXTUREADDRESSWRAP,
    rwTEXTUREADDRESSMIRROR,
    rwTEXTUREADDRESSCLAMP,
    rwTEXTUREADDRESSBORDER,
    rwTEXTUREADDRESSMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwTextureAddressMode RwTextureAddressMode;

enum RwPrimitiveType
{
    rwPRIMTYPENAPRIMTYPE,
    rwPRIMTYPELINELIST,
    rwPRIMTYPEPOLYLINE,
    rwPRIMTYPETRILIST,
    rwPRIMTYPETRISTRIP,
    rwPRIMTYPETRIFAN,
    rwPRIMTYPEPOINTLIST,
    rwPRIMITIVETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwPrimitiveType RwPrimitiveType;

typedef RwBool (*RwSystemFunc)(RwInt32 nOption, void* pOut, void* pInOut, RwInt32 nIn);
typedef RwBool (*RwRenderStateSetFunction)(RwRenderState nState, void* pParam);
typedef RwBool (*RwRenderStateGetFunction)(RwRenderState nState, void* pParam);
typedef RwBool (*RwIm2DRenderLineFunction)(RwIm2DVertex* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2);
typedef RwBool (*RwIm2DRenderTriangleFunction)(RwIm2DVertex* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2, RwInt32 vert3);
typedef RwBool (*RwIm2DRenderPrimitiveFunction)(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices);
typedef RwBool (*RwIm2DRenderIndexedPrimitiveFunction)(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices, RwImVertexIndex* indices, RwInt32 numIndices);
typedef RwBool (*RwIm3DRenderLineFunction)(RwInt32 vert1, RwInt32 vert2);
typedef RwBool (*RwIm3DRenderTriangleFunction)(RwInt32 vert1, RwInt32 vert2, RwInt32 vert3);
typedef RwBool (*RwIm3DRenderPrimitiveFunction)(RwPrimitiveType primType);
typedef RwBool (*RwIm3DRenderIndexedPrimitiveFunction)(RwPrimitiveType primtype, RwImVertexIndex* indices, RwInt32 numIndices);

struct RwDevice
{
    RwReal gammaCorrection;
    RwSystemFunc fpSystem;
    RwReal zBufferNear;
    RwReal zBufferFar;
    RwRenderStateSetFunction fpRenderStateSet;
    RwRenderStateGetFunction fpRenderStateGet;
    RwIm2DRenderLineFunction fpIm2DRenderLine;
    RwIm2DRenderTriangleFunction fpIm2DRenderTriangle;
    RwIm2DRenderPrimitiveFunction fpIm2DRenderPrimitive;
    RwIm2DRenderIndexedPrimitiveFunction fpIm2DRenderIndexedPrimitive;
    RwIm3DRenderLineFunction fpIm3DRenderLine;
    RwIm3DRenderTriangleFunction fpIm3DRenderTriangle;
    RwIm3DRenderPrimitiveFunction fpIm3DRenderPrimitive;
    RwIm3DRenderIndexedPrimitiveFunction fpIm3DRenderIndexedPrimitive;
};

enum RwVideoModeFlag
{
    rwVIDEOMODEEXCLUSIVE = 0x1,
    rwVIDEOMODEINTERLACE = 0x2,
    rwVIDEOMODEFFINTERLACE = 0x4,
    rwVIDEOMODE_PS2_FSAASHRINKBLIT = 0x100,
    rwVIDEOMODE_PS2_FSAAREADCIRCUIT = 0x200,
    rwVIDEOMODE_XBOX_WIDESCREEN = 0x100,
    rwVIDEOMODE_XBOX_PROGRESSIVE = 0x200,
    rwVIDEOMODE_XBOX_FIELD = 0x400,
    rwVIDEOMODE_XBOX_10X11PIXELASPECT = 0x800,
    rwVIDEOMODEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwVideoModeFlag RwVideoModeFlag;

struct RwVideoMode
{
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwVideoModeFlag flags;
    RwInt32 refRate;
    RwInt32 format;
};

typedef int (*vecSprintfFunc)(RwChar* buffer, const RwChar* format, ...);
typedef int (*vecVsprintfFunc)(RwChar* buffer, const RwChar* format, va_list argptr);
typedef RwChar* (*vecStrcpyFunc)(RwChar* dest, const RwChar* srce);
typedef RwChar* (*vecStrncpyFunc)(RwChar* dest, const RwChar* srce, size_t size);
typedef RwChar* (*vecStrcatFunc)(RwChar* dest, const RwChar* srce);
typedef RwChar* (*vecStrncatFunc)(RwChar* dest, const RwChar* srce, size_t size);
typedef RwChar* (*vecStrrchrFunc)(const RwChar* string, int findThis);
typedef RwChar* (*vecStrchrFunc)(const RwChar* string, int findThis);
typedef RwChar* (*vecStrstrFunc)(const RwChar* string, const RwChar* findThis);
typedef int (*vecStrcmpFunc)(const RwChar* string1, const RwChar* string2);
typedef int (*vecStrncmpFunc)(const RwChar* string1, const RwChar* string2, size_t max_size);
typedef int (*vecStricmpFunc)(const RwChar* string1, const RwChar* string2);
typedef size_t (*vecStrlenFunc)(const RwChar* string);
typedef RwChar* (*vecStruprFunc)(RwChar* string);
typedef RwChar* (*vecStrlwrFunc)(RwChar* string);
typedef RwChar* (*vecStrtokFunc)(RwChar* string, const RwChar* delimit);
typedef int (*vecSscanfFunc)(const RwChar* buffer, const RwChar* format, ...);

struct RwStringFunctions
{
    vecSprintfFunc vecSprintf;
    vecVsprintfFunc vecVsprintf;
    vecStrcpyFunc vecStrcpy;
    vecStrncpyFunc vecStrncpy;
    vecStrcatFunc vecStrcat;
    vecStrncatFunc vecStrncat;
    vecStrrchrFunc vecStrrchr;
    vecStrchrFunc vecStrchr;
    vecStrstrFunc vecStrstr;
    vecStrcmpFunc vecStrcmp;
    vecStrncmpFunc vecStrncmp;
    vecStricmpFunc vecStricmp;
    vecStrlenFunc vecStrlen;
    vecStruprFunc vecStrupr;
    vecStrlwrFunc vecStrlwr;
    vecStrtokFunc vecStrtok;
    vecSscanfFunc vecSscanf;
};

enum RwEngineStatus
{
    rwENGINESTATUSIDLE,
    rwENGINESTATUSINITED,
    rwENGINESTATUSOPENED,
    rwENGINESTATUSSTARTED,
    rwENGINESTATUSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT,
};
typedef enum RwEngineStatus RwEngineStatus;

typedef void (*RwDebugHandler)(RwDebugType type, const RwChar* string);
typedef RwBool (*RwStandardFunc)(void* pOut, void* pInOut, RwInt32 nI);
typedef void* (*RwMemoryAllocFn)(RwFreeList* fl, RwUInt32 hint);
typedef RwFreeList* (*RwMemoryFreeFn)(RwFreeList* fl, void* pData);

struct RwGlobals
{
#ifdef RWDEBUG
    RwDebugHandler debugFunction;
    RwInt32 debugStackDepth;
    RwBool debugTrace;
#endif
    void* curCamera;
    void* curWorld;
    RwUInt16 renderFrame;
    RwUInt16 lightFrame;
    RwUInt16 pad[2];
    RwDevice dOpenDevice;
    RwStandardFunc stdFunc[29];
    RwLinkList dirtyFrameList;
    RwStringFunctions stringFuncs;
    RwMemoryFunctions memoryFuncs;
#ifdef RWDEBUG
    RwBool freeListExtraDebug;
#endif
    RwMemoryAllocFn memoryAlloc;
    RwMemoryFreeFn memoryFree;
    RwMetrics* metrics;
    RwEngineStatus engineStatus;
    RwUInt32 resArenaInitSize;
};

typedef void (*RwResEntryDestroyNotify)(RwResEntry* resEntry);

struct RwResEntry
{
    RwLLLink link;
    RwInt32 size;
    void* owner;
    RwResEntry** ownerRef;
    RwResEntryDestroyNotify destroyNotify;
};

struct RwObject
{
    RwUInt8 type;
    RwUInt8 subType;
    RwUInt8 flags;
    RwUInt8 privateFlags;
    void* parent;
};

#ifdef RWDEBUG
#define RwMallocL(size, hint, line) \
    (_rwMemoryLastAllocFile = __FILE__, _rwMemoryLastAllocLine = line, (RWSRCGLOBAL(memoryFuncs).rwmalloc)((size), (hint)))
#define RwMalloc(size, hint) \
    (_rwMemoryLastAllocFile = __FILE__, _rwMemoryLastAllocLine = __LINE__, (RWSRCGLOBAL(memoryFuncs).rwmalloc)((size), (hint)))
#else
#define RwMallocL(size, hint, line) ((RWSRCGLOBAL(memoryFuncs).rwmalloc)((size), (hint)))
#define RwMalloc(size, hint) ((RWSRCGLOBAL(memoryFuncs).rwmalloc)((size), (hint)))
#endif
#define RwFree(p) ((RWSRCGLOBAL(memoryFuncs).rwfree)((p)))

#define RWRGBALONG(r,g,b,a) ((RwUInt32) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#define RwIm2DVertexSetIntRGBA(vert, red, green, blue, alpha)  \
    MACRO_START                                                \
    {                                                          \
        ((vert)->r = (red));                                   \
        ((vert)->g = (green));                                 \
        ((vert)->b = (blue));                                  \
        ((vert)->a = (alpha));                                 \
    }                                                          \
    MACRO_STOP

#define RwIm2DVertexSetScreenX(vert, scrnx) ((vert)->x = (scrnx))
#define RwIm2DVertexSetScreenY(vert, scrny) ((vert)->y = (scrny))
#define RwIm2DVertexSetScreenZ(vert, scrnz) ((vert)->z = (scrnz))
#define RwIm2DVertexSetU(vert, texU, recipz) ((vert)->u = (texU))
#define RwIm2DVertexSetV(vert, texV, recipz) ((vert)->v = (texV))
#define RWSRCGLOBAL(variable) (((RwGlobals *)RwEngineInstance)->variable)
#define RwRenderStateGetMacro(_state, _value) (RWSRCGLOBAL(dOpenDevice).fpRenderStateGet(_state, _value))
#define RwRenderStateSetMacro(_state, _value) (RWSRCGLOBAL(dOpenDevice).fpRenderStateSet(_state, _value))
#define RwIm2DGetNearScreenZMacro() (RWSRCGLOBAL(dOpenDevice).zBufferNear)

#define RwIm2DRenderPrimitiveMacro(_primType, _vertices, _numVertices) \
    (RWSRCGLOBAL(dOpenDevice).fpIm2DRenderPrimitive(_primType, _vertices, _numVertices))

#define rwMatrixSetFlags(m, flagsbit) ((m)->flags = (flagsbit))
#define rwMatrixGetFlags(m) ((m)->flags)

#define RwMatrixSetIdentityMacro(m)                                                                         \
    MACRO_START                                                                                             \
    {                                                                                                       \
        (m)->right.x = (m)->up.y = (m)->at.z = (RwReal)((1.0));                                             \
        (m)->right.y = (m)->right.z = (m)->up.x = (RwReal)((0.0));                                          \
        (m)->up.z = (m)->at.x = (m)->at.y = (RwReal)((0.0));                                                \
        (m)->pos.x = (m)->pos.y = (m)->pos.z = (RwReal)((0.0));                                             \
        rwMatrixSetFlags((m), rwMatrixGetFlags(m) | (rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL));  \
    }                                                                                                       \
    MACRO_STOP

#define rwObjectGetParent(object) (((const RwObject *)(object))->parent)
#define _RW_S2 ( (float) 8.3333337680e-03 )
#define _RW_S3 ( (float)-1.9841270114e-04 )
#define _RW_S4 ( (float) 2.7557314297e-06 )
#define _RW_S5 ( (float)-2.5050759689e-08 )
#define _RW_S6 ( (float) 1.5896910177e-10 )
#define _RW_S1 ( (float)-1.6666667163e-01 )

#define RwSinMinusPiToPiMacro(result, x)                                                       \
    do                                                                                         \
    {                                                                                          \
        const float z = x * x;                                                                 \
        const float v = z * x;                                                                 \
        const float r = ( _RW_S2 + z * (_RW_S3 + z * (_RW_S4 + z * (_RW_S5 + z * _RW_S6))) );  \
        result = x + v * (_RW_S1 + z * r);                                                     \
    }                                                                                          \
    while(0)

#define RwV3dScaleMacro(o, a, s)       \
    MACRO_START                        \
    {                                  \
        (o)->x = (((a)->x) * ( (s)));  \
        (o)->y = (((a)->y) * ( (s)));  \
        (o)->z = (((a)->z) * ( (s)));  \
    }                                  \
    MACRO_STOP

#define RwV3dIncrementScaledMacro(o, a, s)  \
    MACRO_START                             \
    {                                       \
        (o)->x += (((a)->x) * ( (s)));      \
        (o)->y += (((a)->y) * ( (s)));      \
        (o)->z += (((a)->z) * ( (s)));      \
    }                                       \
    MACRO_STOP

#define RWPLUGINOFFSET(_type, _base, _offset) ((_type *)((RwUInt8 *)(_base) + (_offset)))

#ifdef __cplusplus
extern "C" {
#endif

extern void* RwEngineInstance;
#ifdef RWDEBUG
extern const RwChar* _rwMemoryLastAllocFile;
extern RwUInt32 _rwMemoryLastAllocLine;
#endif

RwInt32 RwEngineGetCurrentVideoMode(void);
RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeinfo, RwInt32 modeIndex);
RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in);

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
RwReal RwIm2DGetNearScreenZ(void);
RwBool RwIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices);
void RwMatrixSetIdentity(RwMatrix* matrix);
RwBool RwRenderStateGet(RwRenderState state, void* value);
RwBool RwRenderStateSet(RwRenderState state, void* value);
#endif

#ifdef __cplusplus
}
#endif

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
#define RwIm2DGetNearScreenZ() RwIm2DGetNearScreenZMacro()

#define RwIm2DRenderPrimitive(_primType, _vertices, _numVertices) \
    RwIm2DRenderPrimitiveMacro(_primType, _vertices, _numVertices)

#define RwMatrixSetIdentity(m) RwMatrixSetIdentityMacro(m)
#define RwRenderStateGet(_state, _value) RwRenderStateGetMacro(_state, _value)
#define RwRenderStateSet(_state, _value) RwRenderStateSetMacro(_state, _value)
#endif

#endif

#include "xCommon.h"
#include "water/textures.h"

#include "xstransvc.h"

#include <rwcore.h>

#define GC_WBUMP_ID 0xBE266DB6 // xStrHash("gc_wbump")
#define EXPLOSIONFLASH_SMOKE_TAN_64_ID 0x453C0740 // xStrHash("ExplosionFlash_Smoke_tan_64")

RwTexture* iWaterBumpTex;
RwTexture* iWaterEnvTex;
U32 iWaterBumpTexID = GC_WBUMP_ID;
U32 iWaterEnvTexID = EXPLOSIONFLASH_SMOKE_TAN_64_ID;

S32 TexturesOpen()
{
    RwTextureSetAutoMipmapping(TRUE);
    RwTextureSetMipmapping(TRUE);

    iWaterBumpTex = (RwTexture*)xSTFindAsset(iWaterBumpTexID, NULL);
    if (iWaterBumpTex) {
        RwTextureSetFilterMode(iWaterBumpTex, rwFILTERLINEARMIPLINEAR);
        RwTextureSetAddressing(iWaterBumpTex, rwTEXTUREADDRESSWRAP);
    }

    iWaterEnvTex = (RwTexture*)xSTFindAsset(iWaterEnvTexID, NULL);
    if (iWaterEnvTex) {
        RwTextureSetFilterMode(iWaterEnvTex, rwFILTERLINEARMIPLINEAR);
        RwTextureSetAddressing(iWaterEnvTex, rwTEXTUREADDRESSCLAMP);
    }

    return TRUE;
}

void TexturesClose()
{
    RwTextureSetAutoMipmapping(TRUE);
    RwTextureSetMipmapping(TRUE);

    iWaterBumpTex = NULL;
    iWaterEnvTex = NULL;
}

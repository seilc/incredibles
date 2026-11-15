#include "xCommon.h"
#include "water/textures.h"

#include "xstransvc.h"
#include "zHashes.h"

#include <rwcore.h>

RwTexture* iWaterBumpTex;
RwTexture* iWaterEnvTex;
U32 iWaterBumpTexID = HASH_gc_wbump;
U32 iWaterEnvTexID = HASH_ExplosionFlash_Smoke_tan_64;

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

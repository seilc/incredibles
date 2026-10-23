#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GICE78",  # 0
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-2"
config.compilers_tag = "20251118"
config.dtk_tag = "v1.8.3"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.2"
config.wibo_tag = "1.0.3"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
config.scratch_preset_id = None

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i include",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Warning flags
if args.warn == "all":
    cflags_base.append("-W all")
elif args.warn == "off":
    cflags_base.append("-W off")
elif args.warn == "error":
    cflags_base.append("-W error")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
]

# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
]

# Game-specific flags
cflags_incredibles = [
    *cflags_base,
    "-lang=C++",
    "-common on",
    "-str reuse,pool,readonly",
    "-use_lmw_stmw on",
    '-pragma "cpp_extensions on"',
    "-O4,s",
    "-inline off",
    "-gccinc",
    "-ir src",
    "-i include",
    "-i include/bink",
    "-i include/CodeWarrior",
    "-i include/dolphin",
    "-i include/rwsdk",
    "-DGAMECUBE",
    "-sdata 64",
    "-sdata2 64",
    "-common off",
]

cflags_incredibles_dbg = [
    *cflags_incredibles,
    "-sym on",    
]

config.linker_version = "GC/2.6"


# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_base,
        "progress_category": "sdk",
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_rel,
        "progress_category": "game",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    {
        "lib": "IN",
        "mw_version": config.linker_version,
        "cflags": cflags_incredibles,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "in/Core/x/iCamera.cpp"),
            Object(NonMatching, "in/Core/x/xAnim.cpp"),
            Object(NonMatching, "in/Core/x/xBallPhysics.cpp"),
            Object(Matching, "in/Core/x/xBase.cpp"),
            Object(Matching, "in/Core/x/xBehaveGoalSimple.cpp"),
            Object(Matching, "in/Core/x/xBehaveMgr.cpp"),
            Object(NonMatching, "in/Core/x/xbinio.cpp"),
            Object(Matching, "in/Core/x/xBound.cpp"),
            Object(NonMatching, "in/Core/x/xCam.cpp"),
            Object(NonMatching, "in/Core/x/xCamFollow.cpp"),
            Object(NonMatching, "in/Core/x/xCamSupport.cpp"),
            Object(NonMatching, "in/Core/x/xClimate.cpp"),
            Object(NonMatching, "in/Core/x/xClumpColl.cpp"),
            Object(NonMatching, "in/Core/x/xCM.cpp"),
            Object(NonMatching, "in/Core/x/xCollide.cpp"),
            Object(Matching, "in/Core/x/xCollideFast.cpp"),
            Object(Matching, "in/Core/x/xColor.cpp"),
            Object(Matching, "in/Core/x/xCounter.cpp"),
            Object(Matching, "in/Core/x/xCurveAsset.cpp"),
            Object(NonMatching, "in/Core/x/xCutscene.cpp"),
            Object(NonMatching, "in/Core/x/xDebug.cpp"),
            Object(NonMatching, "in/Core/x/xDecal.cpp"),
            Object(NonMatching, "in/Core/x/xEnt.cpp"),
            Object(NonMatching, "in/Core/x/xEntBoulder.cpp"),
            Object(NonMatching, "in/Core/x/xEntDrive.cpp"),
            Object(NonMatching, "in/Core/x/xEntMotion.cpp"),
            Object(Matching, "in/Core/x/xEnv.cpp"),
            Object(Matching, "in/Core/x/xEvent.cpp"),
            Object(Matching, "in/Core/x/xFactory.cpp"),
            Object(NonMatching, "in/Core/x/xFFX.cpp"),
            Object(Matching, "in/Core/x/xFMV.cpp"),
            Object(Matching, "in/Core/x/xFog.cpp"),
            Object(NonMatching, "in/Core/x/xFont.cpp"),
            Object(NonMatching, "in/Core/x/xFX.cpp"),
            Object(NonMatching, "in/Core/x/xGrid.cpp"),
            Object(Matching, "in/Core/x/xGroup.cpp"),
            Object(Matching, "in/Core/x/xHierarchyBound.cpp"),
            Object(Matching, "in/Core/x/xhipio.cpp"),
            Object(NonMatching, "in/Core/x/xHud.cpp"),
            Object(NonMatching, "in/Core/x/xHudFontMeter.cpp"),
            Object(NonMatching, "in/Core/x/xHudMeter.cpp"),
            Object(Matching, "in/Core/x/xHudModel.cpp"),
            Object(NonMatching, "in/Core/x/xHudUnitMeter.cpp"),
            Object(NonMatching, "in/Core/x/xIni.cpp"),
            Object(Matching, "in/Core/x/xJaw.cpp"),
            Object(NonMatching, "in/Core/x/xJSP.cpp"),
            Object(NonMatching, "in/Core/x/xLaserBolt.cpp"),
            Object(Matching, "in/Core/x/xLightKit.cpp"),
            Object(NonMatching, "in/Core/x/xMath.cpp"),
            Object(Matching, "in/Core/x/xMath2.cpp"),
            Object(NonMatching, "in/Core/x/xMath3.cpp"),
            Object(NonMatching, "in/Core/x/xMemMgr.cpp"),
            Object(NonMatching, "in/Core/x/xModel.cpp"),
            Object(NonMatching, "in/Core/x/xModelBucket.cpp"),
            Object(NonMatching, "in/Core/x/xMorph.cpp"),
            Object(NonMatching, "in/Core/x/xMovePoint.cpp"),
            Object(NonMatching, "in/Core/x/xNavigationMeshAsset.cpp"),
            Object(NonMatching, "in/Core/x/xOneLinerManager.cpp"),
            Object(NonMatching, "in/Core/x/xOneLinerPlayer.cpp"),
            Object(Matching, "in/Core/x/xordarray.cpp"),
            Object(NonMatching, "in/Core/x/xPad.cpp"),
            Object(Matching, "in/Core/x/xParEmitter.cpp"),
            Object(Matching, "in/Core/x/xParSys.cpp"),
            Object(NonMatching, "in/Core/x/xPartition.cpp"),
            Object(Matching, "in/Core/x/xpkrsvc.cpp"),
            Object(NonMatching, "in/Core/x/xPtankPool.cpp"),
            Object(NonMatching, "in/Core/x/xQuickCull.cpp"),
            Object(NonMatching, "in/Core/x/xRand.cpp"),
            Object(Matching, "in/Core/x/xRegionSupport.cpp"),
            Object(Matching, "in/Core/x/xRenderState.cpp"),
            Object(Matching, "in/Core/x/xRMemData.cpp"),
            Object(NonMatching, "in/Core/x/xRumbleEmitter.cpp"),
            Object(NonMatching, "in/Core/x/xRumbleManager.cpp"),
            Object(NonMatching, "in/Core/x/xsavegame.cpp"),
            Object(NonMatching, "in/Core/x/xScene.cpp"),
            Object(NonMatching, "in/Core/x/xScreenWarp.cpp"),
            Object(NonMatching, "in/Core/x/xScrFx.cpp"),
            Object(Matching, "in/Core/x/xserializer.cpp"),
            Object(NonMatching, "in/Core/x/xShadow.cpp"),
            Object(NonMatching, "in/Core/x/xShadowSimple.cpp"),
            Object(NonMatching, "in/Core/x/xSkyDome.cpp"),
            Object(NonMatching, "in/Core/x/xSndMgr.cpp"),
            Object(Matching, "in/Core/x/xSoundFX.cpp"),
            Object(NonMatching, "in/Core/x/xSpacePartitionTree.cpp"),
            Object(NonMatching, "in/Core/x/xSpline.cpp"),
            Object(NonMatching, "in/Core/x/xSplineAsset.cpp"),
            Object(Matching, "in/Core/x/xstransvc.cpp"),
            Object(NonMatching, "in/Core/x/xString.cpp"),
            Object(Matching, "in/Core/x/xSubTitles.cpp"),
            Object(Matching, "in/Core/x/xSurface.cpp"),
            Object(Matching, "in/Core/x/xTextAsset.cpp"),
            Object(NonMatching, "in/Core/x/xTextureManager.cpp"),
            Object(Matching, "in/Core/x/xTimer.cpp"),
            Object(Matching, "in/Core/x/xTransient.cpp"),
            Object(NonMatching, "in/Core/x/xTRC.cpp"),
            Object(NonMatching, "in/Core/x/xUpdateCull.cpp"),
            Object(NonMatching, "in/Core/x/xutil.cpp"),
            Object(Matching, "in/Core/x/xVec3.cpp"),
            Object(Matching, "in/Core/x/xVolume.cpp"),
            Object(NonMatching, "in/Core/gc/iAnimSKB.cpp"),
            Object(NonMatching, "in/Core/gc/iAsync.cpp"),
            Object(NonMatching, "in/Core/gc/iCutscene.cpp"),
            Object(NonMatching, "in/Core/gc/iDraw.cpp"),
            Object(NonMatching, "in/Core/gc/iEnv.cpp"),
            Object(NonMatching, "in/Core/gc/iFile.cpp"),
            Object(NonMatching, "in/Core/gc/iFMV.cpp"),
            Object(NonMatching, "in/Core/gc/iFX.cpp"),
            Object(NonMatching, "in/Core/gc/iLight.cpp"),
            Object(Matching, "in/Core/gc/iMath.cpp"),
            Object(NonMatching, "in/Core/gc/iMath3.cpp"),
            Object(NonMatching, "in/Core/gc/iMemMgr.cpp"),
            Object(NonMatching, "in/Core/gc/iModel.cpp"),
            Object(NonMatching, "in/Core/gc/iMorph.cpp"),
            Object(NonMatching, "in/Core/gc/iPad.cpp"),
            Object(NonMatching, "in/Core/gc/isavegame.cpp"),
            Object(NonMatching, "in/Core/gc/iScrFX.cpp"),
            Object(NonMatching, "in/Core/gc/iSnd.cpp"),
            Object(NonMatching, "in/Core/gc/iSystem.cpp"),
            Object(NonMatching, "in/Core/gc/iTime.cpp"),
            Object(NonMatching, "in/Core/gc/iTRC.cpp"),
            Object(NonMatching, "in/Core/gc/iAnim.cpp"),
            Object(NonMatching, "in/GAME/zZipLine.cpp"),
            Object(NonMatching, "in/GAME/zAnimCache.cpp"),
            Object(NonMatching, "in/GAME/zAnimList.cpp"),
            Object(NonMatching, "in/GAME/zAssetTypes.cpp"),
            Object(NonMatching, "in/GAME/zBulletTime.cpp"),
            Object(Matching, "in/GAME/zCam.cpp"),
            Object(Matching, "in/GAME/zCameraFly.cpp"),
            Object(Matching, "in/GAME/zCameraTweak.cpp"),
            Object(NonMatching, "in/GAME/zCamFP.cpp"),
            Object(NonMatching, "in/GAME/zCamMarker.cpp"),
            Object(NonMatching, "in/GAME/zCamPlayer.cpp"),
            Object(NonMatching, "in/GAME/zCamSwing.cpp"),
            Object(NonMatching, "in/GAME/zCheat.cpp"),
            Object(NonMatching, "in/GAME/zCollGeom.cpp"),
            Object(NonMatching, "in/GAME/zCombat.cpp"),
            Object(NonMatching, "in/GAME/zCombatCombo.cpp"),
            Object(NonMatching, "in/GAME/zCombatEffects.cpp"),
            Object(NonMatching, "in/GAME/zCombatLaser.cpp"),
            Object(NonMatching, "in/GAME/zCombatSystem.cpp"),
            Object(NonMatching, "in/GAME/zCommonPlayer.cpp"),
            Object(Matching, "in/GAME/zConditional.cpp"),
            Object(Matching, "in/GAME/zConfiguration.cpp"),
            Object(NonMatching, "in/GAME/zCutsceneMgr.cpp"),
            Object(NonMatching, "in/GAME/zDashPlayer.cpp"),
            Object(NonMatching, "in/GAME/zDestructible.cpp"),
            Object(NonMatching, "in/GAME/zDispatcher.cpp"),
            Object(NonMatching, "in/GAME/zDuplicator.cpp"),
            Object(NonMatching, "in/GAME/zEGenerator.cpp"),
            Object(NonMatching, "in/GAME/zElastiGirlPlayer.cpp"),
            Object(NonMatching, "in/GAME/zEnt.cpp"),
            Object(NonMatching, "in/GAME/zEntButton.cpp"),
            Object(NonMatching, "in/GAME/zEntHangable.cpp"),
            Object(NonMatching, "in/GAME/zEntSimpleObj.cpp"),
            Object(NonMatching, "in/GAME/zEntTrigger.cpp"),
            Object(Matching, "in/GAME/zEnv.cpp"),
            Object(NonMatching, "in/GAME/zExplosion.cpp"),
            Object(NonMatching, "in/GAME/zFire.cpp"),
            Object(NonMatching, "in/GAME/zFlameEmitter.cpp"),
            Object(Matching, "in/GAME/zFMV.cpp"),
            Object(Matching, "in/GAME/zFX.cpp"),
            Object(NonMatching, "in/GAME/zFXRibbonPool.cpp"),
            Object(NonMatching, "in/GAME/zGame.cpp"),
            Object(Matching, "in/GAME/zGameExtras.cpp"),
            Object(Matching, "in/GAME/zGameState.cpp"),
            Object(Matching, "in/GAME/zGrapple.cpp"),
            Object(NonMatching, "in/GAME/zGrid.cpp"),
            Object(NonMatching, "in/GAME/zHangable.cpp"),
            Object(Matching, "in/GAME/zHitParameters.cpp"),
            Object(Matching, "in/GAME/zHud.cpp"),
            Object(NonMatching, "in/GAME/zHudIncredimeter.cpp"),
            Object(Matching, "in/GAME/zHudTextureFlasher.cpp"),
            Object(NonMatching, "in/GAME/zIncrediBallPlayer.cpp"),
            Object(NonMatching, "in/GAME/zKaboomMinigame.cpp"),
            Object(NonMatching, "in/GAME/zLaserBeam.cpp"),
            Object(NonMatching, "in/GAME/zLight.cpp"),
            Object(NonMatching, "in/GAME/zLightEffect.cpp"),
            Object(NonMatching, "in/GAME/zLightning.cpp"),
            Object(Matching, "in/GAME/zLightWeight.cpp"),
            Object(NonMatching, "in/GAME/zLOD.cpp"),
            Object(NonMatching, "in/GAME/zMain.cpp"),
            Object(NonMatching, "in/GAME/zMenu.cpp"),
            Object(Matching, "in/GAME/zMovePoint.cpp"),
            Object(NonMatching, "in/GAME/zMrIncrediblePlayer.cpp"),
            Object(NonMatching, "in/GAME/zMusic.cpp"),
            Object(Matching, "in/GAME/zNPC.cpp"),
            Object(NonMatching, "in/GAME/zNPCArmor.cpp"),
            Object(NonMatching, "in/GAME/zNPCBehavior.cpp"),
            Object(NonMatching, "in/GAME/zNPCBehaviorFX.cpp"),
            Object(NonMatching, "in/GAME/zNPCBossOmnidroid.cpp"),
            Object(NonMatching, "in/GAME/zNPCCar.cpp"),
            Object(NonMatching, "in/GAME/zNPCCombat.cpp"),
            Object(NonMatching, "in/GAME/zNPCFriendly.cpp"),
            Object(NonMatching, "in/GAME/zNPCLobber.cpp"),
            Object(NonMatching, "in/GAME/zNPCMelee.cpp"),
            Object(NonMatching, "in/GAME/zNPCMeleeShielded.cpp"),
            Object(NonMatching, "in/GAME/zNPCMgr.cpp"),
            Object(NonMatching, "in/GAME/zNPCMine.cpp"),
            Object(NonMatching, "in/GAME/zNPCNavigation.cpp"),
            Object(NonMatching, "in/GAME/zNPCNavigationMesh.cpp"),
            Object(NonMatching, "in/GAME/zNPCOracle.cpp"),
            Object(NonMatching, "in/GAME/zNPCShooter.cpp"),
            Object(NonMatching, "in/GAME/zNPCTank.cpp"),
            Object(NonMatching, "in/GAME/zNPCTurret.cpp"),
            Object(NonMatching, "in/GAME/zNPCTypes.cpp"),
            Object(NonMatching, "in/GAME/zNPCVelocipod.cpp"),
            Object(NonMatching, "in/GAME/zNPCWaterhose.cpp"),
            Object(NonMatching, "in/GAME/zPendulum.cpp"),
            Object(NonMatching, "in/GAME/zPlatform.cpp"),
            Object(NonMatching, "in/GAME/zPlayer.cpp"),
            Object(Matching, "in/GAME/zPlayerBase.cpp"),
            Object(Matching, "in/GAME/zPortal.cpp"),
            Object(Matching, "in/GAME/zReference.cpp"),
            Object(NonMatching, "in/GAME/zRenderState.cpp"),
            Object(NonMatching, "in/GAME/zSaveLoad.cpp"),
            Object(NonMatching, "in/GAME/zScene.cpp"),
            Object(Matching, "in/GAME/zScript.cpp"),
            Object(NonMatching, "in/GAME/zshell.cpp"),
            Object(NonMatching, "in/GAME/zShrapnel.cpp"),
            Object(Matching, "in/GAME/zSlideCamera.cpp"),
            Object(NonMatching, "in/GAME/zSurface.cpp"),
            Object(Matching, "in/GAME/zSymbol.cpp"),
            Object(NonMatching, "in/GAME/zTalkBox.cpp"),
            Object(NonMatching, "in/GAME/zTaskBox.cpp"),
            Object(NonMatching, "in/GAME/zTextBox.cpp"),
            Object(NonMatching, "in/GAME/zTrainCar.cpp"),
            Object(Matching, "in/GAME/zTrainPath.cpp"),
            Object(NonMatching, "in/GAME/zTransient.cpp"),
            Object(NonMatching, "in/GAME/zTurret.cpp"),
            Object(NonMatching, "in/GAME/zUI.cpp"),
            Object(NonMatching, "in/GAME/zUIBox.cpp"),
            Object(NonMatching, "in/GAME/zUIImage.cpp"),
            Object(NonMatching, "in/GAME/zUIMgr.cpp"),
            Object(NonMatching, "in/GAME/zUIModel.cpp"),
            Object(NonMatching, "in/GAME/zUIText.cpp"),
            Object(Matching, "in/GAME/zUIUserString.cpp"),
            Object(NonMatching, "in/GAME/zVar.cpp"),
            Object(NonMatching, "in/GAME/zVioletPlayer.cpp"),
            Object(Matching, "in/GAME/zVolume.cpp"),
            Object(NonMatching, "in/GAME/zWaterhose.cpp"),
        ],
    },
    {  # Compiled with -sym on and not alphabetically sorted
        "lib": "IN_DBG",
        "mw_version": config.linker_version,
        "cflags": cflags_incredibles_dbg,
        "progress_category": "game",
        "objects": [
            Object(Matching, "in/GAME/zActionLine.cpp"),
            Object(NonMatching, "in/Core/x/xCollideSpherePolygon.cpp"),
            Object(NonMatching, "in/GAME/zAsync.cpp"),
            Object(NonMatching, "in/Core/x/xParticleBatch.cpp"),
            Object(NonMatching, "in/GAME/zSmoke.cpp"),
            Object(NonMatching, "in/GAME/zSplash.cpp"),
            Object(NonMatching, "in/GAME/zNPCNavigationMeshDriven.cpp"),
            Object(NonMatching, "in/GAME/zDashCam.cpp"),
            Object(NonMatching, "in/GAME/zNPCRobotBehavior.cpp"),
            Object(NonMatching, "in/GAME/zIncrediSlam.cpp"),
            Object(NonMatching, "in/Core/x/xScrPulseGlare.cpp"),
            Object(Matching, "in/GAME/zBehavior.cpp"),
            Object(Matching, "in/GAME/zWater.cpp"),
            Object(NonMatching, "in/GAME/zIncrediBallCam.cpp"),
            Object(Matching, "in/GAME/zTimeHandler.cpp"),
            Object(Matching, "in/GAME/zBulletMarkAsset.cpp"),
            Object(NonMatching, "in/Core/x/xCollideSweptSphere.cpp"),
            Object(Matching, "in/GAME/zNPCCommon.cpp"),
            Object(Matching, "in/Core/x/xScreenFade.cpp"),
            Object(NonMatching, "in/GAME/zNPCWeapon.cpp"),
            Object(NonMatching, "in/Core/x/xCollideSphereBox.cpp"),
            Object(NonMatching, "in/GAME/zNPCInvisAware.cpp"),
            Object(NonMatching, "in/GAME/zNPCBotLob.cpp"),
            Object(NonMatching, "in/GAME/zDashTrack.cpp"),
            Object(NonMatching, "in/GAME/zReticle.cpp"),
            Object(NonMatching, "in/GAME/zGrass.cpp"),
            Object(NonMatching, "in/Core/x/xFXCameraTexture.cpp"),
            Object(NonMatching, "in/Core/x/xFXHighDynamicRange.cpp"),
            Object(NonMatching, "in/Core/x/xCollideSpherePolygonUnprocessed.cpp"),
            Object(NonMatching, "in/GAME/zInteraction.cpp"),
            Object(NonMatching, "in/GAME/zNPCSenses.cpp"),
            Object(NonMatching, "in/GAME/zBomb.cpp"),
            Object(Matching, "in/GAME/zProgressScript.cpp"),
            Object(NonMatching, "in/Core/x/xHudText.cpp"),
            Object(NonMatching, "in/GAME/zFlameThrower.cpp"),
            Object(NonMatching, "in/GAME/zNPCFlyShooter.cpp"),
            Object(NonMatching, "in/GAME/zRocket.cpp"),
            Object(NonMatching, "in/Core/x/xHudImage.cpp"),
            Object(NonMatching, "in/GAME/zNPCMeleeBot.cpp"),
            Object(NonMatching, "in/GAME/zReactiveAnimation.cpp"),
            Object(NonMatching, "in/GAME/zDust.cpp"),
            Object(Matching, "in/GAME/zHudDiskIndicator.cpp"),
            Object(Matching, "in/Core/x/xProjectedTexture.cpp"),
            Object(NonMatching, "in/GAME/zSpotlight.cpp"),
            Object(NonMatching, "in/GAME/zPickup.cpp"),
            Object(Matching, "in/GAME/zNPCGroup.cpp"),
            Object(NonMatching, "in/GAME/zThrowableSystem.cpp"),
            Object(NonMatching, "in/GAME/zNPCHelibot.cpp"),
            Object(NonMatching, "in/GAME/zHudDashMeter.cpp"),
            Object(NonMatching, "in/GAME/zNPCBossOmnidroid10.cpp"),
            Object(NonMatching, "in/GAME/zNPCBotLeap.cpp"),
            Object(Matching, "in/GAME/zUIController.cpp"),
            Object(Matching, "in/Core/x/xLightVolume.cpp"),
            Object(NonMatching, "in/GAME/zUICustom.cpp"),
            Object(NonMatching, "in/GAME/zUIIncredibles.cpp"),
            Object(NonMatching, "in/Core/x/xMemoryManager.cpp"),
            Object(Matching, "in/Core/x/xMemoryManagerFixed.cpp"),
            Object(NonMatching, "in/Core/x/xMemoryManagerGeneral.cpp"),
            Object(Matching, "in/Core/gc/iException.cpp"),
            Object(NonMatching, "in/GAME/zParticleGenerator.cpp"),
            Object(NonMatching, "in/GAME/zParticleLocator.cpp"),
            Object(Matching, "in/GAME/zParticleSystems.cpp"),
            Object(NonMatching, "in/GAME/zParticleSystemWaterfall.cpp"),
            Object(NonMatching, "in/GAME/zTurretSmoke.cpp"),
            Object(NonMatching, "in/GAME/zUberLaser.cpp"),
            Object(Matching, "in/GAME/zLoadingScreen.cpp"),
            Object(NonMatching, "in/Core/x/xModelBlur.cpp"),
            Object(NonMatching, "in/Core/gc/iWater.cpp"),
            Object(NonMatching, "in/Core/gc/fxembm.c"),
            Object(NonMatching, "in/Core/gc/fxmain.c"),
            Object(NonMatching, "in/Core/gc/texcache.c"),
            Object(NonMatching, "in/Core/gc/texconv.c"),
            Object(NonMatching, "in/Core/gc/textures.c"),
            Object(NonMatching, "in/GAME/zEnergyStream.cpp"),
            Object(NonMatching, "in/GAME/zNPCSmoke.cpp"),
            Object(NonMatching, "in/GAME/zNPCStreamer.cpp"),
            Object(NonMatching, "in/GAME/zNPCBossBombChopper.cpp"),
            Object(Matching, "in/GAME/zCameraRBand.cpp"),
            Object(Matching, "in/GAME/zWireFrameAsset.cpp"),
            Object(Matching, "in/GAME/zJSPExtraData.cpp"),
            Object(NonMatching, "in/GAME/zRubbleGenerator.cpp"),
            Object(NonMatching, "in/GAME/zParticleFXCommon.cpp"),
            Object(NonMatching, "in/GAME/zHudBossMeter.cpp"),
            Object(Matching, "in/Core/x/containers.cpp"),
            Object(Matching, "in/Core/x/xFunctionGenerator.cpp"),
            Object(Matching, "in/Core/x/xModelWarp.cpp"),
            Object(NonMatching, "in/GAME/zCombo.cpp"),
            Object(NonMatching, "in/GAME/zRubbleSystem.cpp"),
            Object(NonMatching, "in/GAME/zNPCPlayers.cpp"),
            Object(Matching, "in/GAME/zCineFXShared.cpp"),
            Object(NonMatching, "in/GAME/zCineFX.cpp"),
            Object(NonMatching, "in/GAME/zFrozoneIce.cpp"),
            Object(Matching, "in/Core/x/xRenderBuffer.cpp"),
            Object(NonMatching, "in/GAME/zFXFireTrail.cpp"),
            Object(NonMatching, "in/GAME/zFXEnergySystem.cpp"),
            Object(NonMatching, "in/GAME/zUICustomBattleModeSelectScene.cpp"),
            Object(Matching, "in/GAME/zUICustomSysMessage.cpp"),
            Object(NonMatching, "in/GAME/zUICustomBattleModeSelectPlayer.cpp"),
            Object(NonMatching, "in/Core/gc/iARAMTmp.cpp"),
            Object(Matching, "in/GAME/zCamPreset.cpp"),
            Object(NonMatching, "in/GAME/zCineFXIN.cpp"),
            Object(NonMatching, "in/GAME/zSparkEmitter.cpp"),
            Object(NonMatching, "in/GAME/zToss.cpp"),
            Object(NonMatching, "in/GAME/zHudFamilyMeter.cpp"),
            Object(NonMatching, "in/Core/x/xIM.cpp"),
            Object(NonMatching, "in/GAME/zParticleCustom.cpp"),
            Object(NonMatching, "in/GAME/zShrapnelEmitter.cpp"),
            Object(NonMatching, "in/GAME/zSmashRing.cpp"),
            Object(NonMatching, "in/GAME/zTransientModel.cpp"),
            Object(NonMatching, "in/GAME/zIncrediBallEffect.cpp"),
        ],
    },
    {
        "lib": "Runtime",
        "mw_version": config.linker_version,
        "cflags": cflags_runtime,
        "progress_category": "sdk",  # str | List[str]
        "objects": [
            Object(NonMatching, "Runtime/global_destructor_chain.c"),
            Object(NonMatching, "Runtime/__init_cpp_exceptions.cpp"),
        ],
    },
]


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects


# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
]
config.progress_each_module = args.verbose
# Optional extra arguments to `objdiff-cli report generate`
config.progress_report_args = [
    # Marks relocations as mismatching if the target value is different
    # Default is "functionRelocDiffs=none", which is most lenient
    # "--config functionRelocDiffs=data_value",
]

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)

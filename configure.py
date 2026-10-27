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
    "-i src",
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

# Library flags (Rat's base flags; the libraries come from Rat, via RotU)
cflags_lib_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    # "-W all",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i src",
    f"-i build/{config.version}/include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/Include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C++/MSL_Common/Include",
]
cflags_lib_base.append("-DNDEBUG=1")

# Metrowerks library flags
cflags_runtime = [
    *cflags_lib_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
    "-O4,p",
]

# Dolphin SDK flags (dolsdk2004's Makefile), release libraries.
cflags_sdk_base = [
    "-char unsigned",
    "-nodefaults",
    "-proc gekko",
    "-fp hard",
    "-Cpp_exceptions off",
    "-enum int",
    "-warn pragmas",
    "-requireprotos",
    '-pragma "cats off"',
    "-D__GEKKO__",
    "-nosyspath",
    "-i src/dolphin/include",
    "-i src/dolphin/include/libc",
    "-ir src/dolphin/src",
]
cflags_sdk_debug = [
    "-opt level=0",
    "-inline off",
    "-schedule off",
    "-sym on",
    *cflags_sdk_base,
    "-DDEBUG",
]
cflags_sdk_release = [
    "-O4,p",
    "-inline auto",
    "-sym on",
    *cflags_sdk_base,
    "-DRELEASE",
]
cflags_sdk = cflags_sdk_release
sdk_char_signed = ["-char signed"]

# EXIBios.c is built at -O3,p instead of -O4,p in the release library
cflags_sdk_exibios = [("-O3,p" if f == "-O4,p" else f) for f in cflags_sdk]

# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
]

# RenderWare flags
cflags_rwcore_base = [
    *cflags_lib_base,
    "-sym on",
    "-DDOLPHIN",
    "-DRW_USE_SPF",
    "-DRWNOVECMULTFUNCS",
    "-gccinc",
    "-i src/dolphin/include",
    "-i src/rwsdk/include/gcn",
    "-i src/rwsdk/driver/common",
    "-i src/rwsdk/driver/gcn",
    "-i src/rwsdk/os/gcn",
    "-i src/rwsdk/src",
    "-i src/rwsdk/src/pipe/p2",
    "-i src/rwsdk/src/pipe/p2/gcn",
    "-i src/rwsdk/src/plcore",
]
cflags_rwcore = [
    *cflags_rwcore_base,
    "-O4,p",
]

cflags_rpcollis = [
    "-i src/rwsdk/plugin/collis",
    *cflags_rwcore,
]
cflags_rphanim = [
    "-i src/rwsdk/plugin/hanim",
    *cflags_rwcore,
]
cflags_rpmatfx = [
    "-i src/rwsdk/plugin/matfx",
    "-i src/rwsdk/plugin/matfx/gcn",
    *cflags_rwcore,
]
cflags_rpptank = [
    "-i src/rwsdk/plugin/ptank",
    "-i src/rwsdk/plugin/ptank/gcn",
    "-i src/rwsdk/plugin/ptank/generic",
    *cflags_rwcore,
]
cflags_rpskinmatfx = [
    "-i src/rwsdk/plugin/skin2",
    "-i src/rwsdk/plugin/skin2/gcn",
    *cflags_rwcore,
]
cflags_rpusrdat = [
    "-i src/rwsdk/plugin/userdata",
    *cflags_rwcore,
]
cflags_rpworld = [
    "-i src/rwsdk/world",
    "-i src/rwsdk/world/pipe",
    "-i src/rwsdk/world/pipe/p2",
    "-i src/rwsdk/world/pipe/p2/gcn",
    *cflags_rwcore,
]
cflags_rtanim = [
    "-i src/rwsdk/tool/anim",
    *cflags_rwcore,
]
cflags_rtcharse = [
    "-i src/rwsdk/tool/charse",
    *cflags_rwcore,
]
cflags_rtfsyst = [
    "-i src/rwsdk/tool/fsyst",
    "-i src/rwsdk/tool/fsyst/gcn",
    *cflags_rwcore,
]
cflags_rtintsec = [
    "-i src/rwsdk/tool/intsec",
    *cflags_rwcore,
]
cflags_rtquat = [
    "-i src/rwsdk/tool/quat",
    *cflags_rwcore,
]
cflags_rtskinsp = [
    "-i src/rwsdk/tool/sknsplit",
    *cflags_rwcore,
]
cflags_rtslerp = [
    "-i src/rwsdk/tool/slerp",
    *cflags_rwcore,
]
cflags_rttilerd = [
    "-i src/rwsdk/tool/tilerd",
    *cflags_rwcore,
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
    "-ir src/in",
    "-i src/bink/bink",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C++/MSL_Common/Include",
    "-i src/dolphin/include",
    "-i src/rwsdk/include",
    "-DGAMECUBE",
    "-DDEBUG=0",
    "-DRELEASE=0",
    "-DMASTERDEBUG=0",
    "-DMASTER=1",
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
        "cflags": cflags_sdk,
        "progress_category": "sdk",
        "objects": objects,
    }

# Helper function for RenderWare libraries
def RenderWareLib(lib_name: str, cflags: List[str], objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/2.5",
        "cflags": cflags,
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
            Object(Matching, "in/Core/x/xbinio.cpp"),
            Object(Matching, "in/Core/x/xBound.cpp"),
            Object(NonMatching, "in/Core/x/xCam.cpp"),
            Object(NonMatching, "in/Core/x/xCamFollow.cpp"),
            Object(NonMatching, "in/Core/x/xCamSupport.cpp"),
            Object(Matching, "in/Core/x/xClimate.cpp"),
            Object(NonMatching, "in/Core/x/xClumpColl.cpp"),
            Object(Matching, "in/Core/x/xCM.cpp"),
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
            Object(Matching, "in/Core/x/xFFX.cpp"),
            Object(Matching, "in/Core/x/xFMV.cpp"),
            Object(Matching, "in/Core/x/xFog.cpp"),
            Object(NonMatching, "in/Core/x/xFont.cpp"),
            Object(NonMatching, "in/Core/x/xFX.cpp"),
            Object(NonMatching, "in/Core/x/xGrid.cpp"),
            Object(Matching, "in/Core/x/xGroup.cpp"),
            Object(Matching, "in/Core/x/xHierarchyBound.cpp"),
            Object(Matching, "in/Core/x/xhipio.cpp"),
            Object(NonMatching, "in/Core/x/xHud.cpp"),
            Object(Matching, "in/Core/x/xHudFontMeter.cpp"),
            Object(Matching, "in/Core/x/xHudMeter.cpp"),
            Object(Matching, "in/Core/x/xHudModel.cpp"),
            Object(Matching, "in/Core/x/xHudUnitMeter.cpp"),
            Object(Matching, "in/Core/x/xIni.cpp"),
            Object(Matching, "in/Core/x/xJaw.cpp"),
            Object(Matching, "in/Core/x/xJSP.cpp"),
            Object(NonMatching, "in/Core/x/xLaserBolt.cpp"),
            Object(Matching, "in/Core/x/xLightKit.cpp"),
            Object(NonMatching, "in/Core/x/xMath.cpp"),
            Object(Matching, "in/Core/x/xMath2.cpp"),
            Object(NonMatching, "in/Core/x/xMath3.cpp"),
            Object(NonMatching, "in/Core/x/xMemMgr.cpp"),
            Object(NonMatching, "in/Core/x/xModel.cpp"),
            Object(NonMatching, "in/Core/x/xModelBucket.cpp"),
            Object(Matching, "in/Core/x/xMorph.cpp"),
            Object(Matching, "in/Core/x/xMovePoint.cpp"),
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
            Object(Matching, "in/Core/x/xQuickCull.cpp"),
            Object(Matching, "in/Core/x/xRand.cpp"),
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
            Object(Matching, "in/Core/x/xSpline.cpp"),
            Object(NonMatching, "in/Core/x/xSplineAsset.cpp"),
            Object(Matching, "in/Core/x/xstransvc.cpp"),
            Object(Matching, "in/Core/x/xString.cpp"),
            Object(Matching, "in/Core/x/xSubTitles.cpp"),
            Object(Matching, "in/Core/x/xSurface.cpp"),
            Object(Matching, "in/Core/x/xTextAsset.cpp"),
            Object(Matching, "in/Core/x/xTextureManager.cpp"),
            Object(Matching, "in/Core/x/xTimer.cpp"),
            Object(Matching, "in/Core/x/xTransient.cpp"),
            Object(NonMatching, "in/Core/x/xTRC.cpp"),
            Object(Matching, "in/Core/x/xUpdateCull.cpp"),
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
            Object(Matching, "in/Core/gc/iAnim.cpp"),
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
            Object(Matching, "in/Core/x/xScrPulseGlare.cpp"),
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
            Object(Matching, "in/Core/x/xCollideSpherePolygonUnprocessed.cpp"),
            Object(NonMatching, "in/GAME/zInteraction.cpp"),
            Object(NonMatching, "in/GAME/zNPCSenses.cpp"),
            Object(NonMatching, "in/GAME/zBomb.cpp"),
            Object(Matching, "in/GAME/zProgressScript.cpp"),
            Object(Matching, "in/Core/x/xHudText.cpp"),
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
            Object(Matching, "in/Core/x/xMemoryManager.cpp"),
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
            Object(Matching, "in/Core/x/xIM.cpp"),
            Object(NonMatching, "in/GAME/zParticleCustom.cpp"),
            Object(NonMatching, "in/GAME/zShrapnelEmitter.cpp"),
            Object(NonMatching, "in/GAME/zSmashRing.cpp"),
            Object(NonMatching, "in/GAME/zTransientModel.cpp"),
            Object(NonMatching, "in/GAME/zIncrediBallEffect.cpp"),
        ],
    },
    {
        "lib": "fmod",
        "mw_version": "GC/1.3.2",
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "fmod/fmusic.c"),
            Object(NonMatching, "fmod/format_fsb.c"),
            Object(NonMatching, "fmod/format_gcadpcm.c"),
            Object(NonMatching, "fmod/fsound.c"),
            Object(NonMatching, "fmod/fsound_3d.c"),
            Object(NonMatching, "fmod/fsound_async.c"),
            Object(NonMatching, "fmod/fsound_channel.c"),
            Object(NonMatching, "fmod/fsound_device.c"),
            Object(NonMatching, "fmod/fsound_dsp.c"),
            Object(NonMatching, "fmod/fsound_gc.c"),
            Object(NonMatching, "fmod/fsound_sample.c"),
            Object(NonMatching, "fmod/fsound_stream.c"),
            Object(NonMatching, "fmod/mixer.c"),
            Object(NonMatching, "fmod/fsound_stream_decode.c"),
            Object(NonMatching, "fmod/mixer_clipcopy.c"),
            Object(NonMatching, "fmod/music_formatfsb.c"),
            Object(NonMatching, "fmod/output_gc.c"),
            Object(NonMatching, "fmod/output_nosound.c"),
            Object(NonMatching, "fmod/sound_software.c"),
            Object(NonMatching, "fmod/sound_software_3d.c"),
            Object(NonMatching, "fmod/system_file.c"),
            Object(NonMatching, "fmod/system_memory.c"),
            Object(NonMatching, "fmod/system_misc_gc.c"),
            Object(NonMatching, "fmod/system_thread.c"),
            Object(NonMatching, "fmod/system_time.c"),
            Object(NonMatching, "fmod/system_string.c"),
        ],
    },
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": config.linker_version,
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/__va_arg.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/global_destructor_chain.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/__mem.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/CPlusLibPPC.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/New.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/NMWException.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/runtime.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/__init_cpp_exceptions.cpp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/Gecko_ExceptionPPC.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/GCN_mem_alloc.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/abort_exit.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/alloc.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/errno.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/ansi_files.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Src/ansi_fp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/arith.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/bsearch.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/buffer_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/SRC/critical_regions.gamecube.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/ctype.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/locale.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/direct_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/file_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/FILE_POS.C"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mbstring.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mem.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mem_funcs.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/misc_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/printf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/qsort.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/rand.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/scanf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/signal.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/string.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/float.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/strtold.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/strtoul.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/wchar_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/SRC/uart_console_io_gcn.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_acos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_asin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_atan2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_exp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_fmod.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_log.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_pow.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_rem_pio2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_cos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_rem_pio2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_sin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_tan.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_atan.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ceil.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_copysign.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_cos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_floor.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_frexp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ldexp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_modf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_sin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_tan.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_acos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_asin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_atan2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_exp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_fmod.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_log.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_pow.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_sqrt.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/SRC/math_ppc.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_sqrt.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/extras.c"),
        ],
    },
    {
        "lib": "TRK_Minnow_Dolphin",
        "mw_version": config.linker_version,
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/mainloop.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/nubevent.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/nubinit.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/msg.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/msgbuf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/serpoll.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/usr_put.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/dispatch.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/msghndlr.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/support.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/mutex_TRK.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/notify.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/flush_cache.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/mem_TRK.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/targimpl.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Export/targsupp.s"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/mpc_7xx_603e.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/__exception.s"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/main_TRK.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk_glue.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/targcont.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/target_options.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Export/mslsupp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/UDP_Stubs.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/cc/exi2/GCN/EXI2_DDH_GCN/main.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/utils/common/CircleBuffer.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/cc/exi2/GCN/EXI2_GDEV_GCN/main.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/utils/common/MWTrace.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/utils/gc/MWCriticalSection_gc.cpp"),
        ],
    },
    DolphinLib(
        "ai",
        [
            Object(Matching, "dolphin/src/ai/ai.c"),
        ],
    ),
    DolphinLib(
        "amcstubs",
        [
            Object(Matching, "dolphin/src/amcstubs/AmcExi2Stubs.c"),
        ],
    ),
    DolphinLib(
        "ar",
        [
            Object(Matching, "dolphin/src/ar/ar.c"),
            Object(Matching, "dolphin/src/ar/arq.c"),
        ],
    ),
    DolphinLib(
        "ax",
        [
            Object(Matching, "dolphin/src/ax/AX.c"),
            Object(Matching, "dolphin/src/ax/AXAlloc.c"),
            Object(Matching, "dolphin/src/ax/AXAux.c"),
            Object(Matching, "dolphin/src/ax/AXCL.c"),
            Object(Matching, "dolphin/src/ax/AXOut.c"),
            Object(Matching, "dolphin/src/ax/AXSPB.c"),
            Object(Matching, "dolphin/src/ax/AXVPB.c"),
            Object(Matching, "dolphin/src/ax/AXComp.c"),
            Object(Matching, "dolphin/src/ax/DSPCode.c"),
            Object(Matching, "dolphin/src/ax/AXProf.c"),
        ],
    ),
    DolphinLib(
        "axfx",
        [
            Object(Matching, "dolphin/src/axfx/reverb_hi.c"),
            Object(Matching, "dolphin/src/axfx/reverb_std.c"),
            Object(Matching, "dolphin/src/axfx/chorus.c"),
            Object(Matching, "dolphin/src/axfx/delay.c"),
            Object(Matching, "dolphin/src/axfx/axfx.c"),
            Object(Matching, "dolphin/src/axfx/reverb_hi_4ch.c"),
        ],
    ),
    DolphinLib(
        "base",
        [
            Object(Matching, "dolphin/src/base/PPCArch.c"),
        ],
    ),
    DolphinLib(
        "card",
        [
            Object(NonMatching, "dolphin/src/card/CARDBios.c"),
            Object(Matching, "dolphin/src/card/CARDUnlock.c"),
            Object(NonMatching, "dolphin/src/card/CARDRdwr.c"),
            Object(Matching, "dolphin/src/card/CARDBlock.c"),
            Object(Matching, "dolphin/src/card/CARDDir.c"),
            Object(Matching, "dolphin/src/card/CARDCheck.c"),
            Object(NonMatching, "dolphin/src/card/CARDMount.c"),
            Object(Matching, "dolphin/src/card/CARDFormat.c"),
            Object(Matching, "dolphin/src/card/CARDOpen.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/card/CARDCreate.c"),
            Object(Matching, "dolphin/src/card/CARDRead.c"),
            Object(Matching, "dolphin/src/card/CARDWrite.c"),
            Object(Matching, "dolphin/src/card/CARDDelete.c"),
            Object(Matching, "dolphin/src/card/CARDStat.c"),
            Object(Matching, "dolphin/src/card/CARDStatEx.c"),
            Object(Matching, "dolphin/src/card/CARDNet.c"),
        ],
    ),
    DolphinLib(
        "db",
        [
            Object(Matching, "dolphin/src/db/db.c"),
        ],
    ),
    DolphinLib(
        "dsp",
        [
            Object(Matching, "dolphin/src/dsp/dsp.c"),
            Object(Matching, "dolphin/src/dsp/dsp_debug.c"),
            Object(Matching, "dolphin/src/dsp/dsp_task.c"),
        ],
    ),
    DolphinLib(
        "dvd",
        [
            Object(Matching, "dolphin/src/dvd/dvdlow.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdfs.c", extra_cflags=sdk_char_signed),
            Object(NonMatching, "dolphin/src/dvd/dvd.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdqueue.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvderror.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdidutils.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdFatal.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/fstload.c", extra_cflags=sdk_char_signed),
        ],
    ),
    DolphinLib(
        "exi",
        [
            Object(Matching, "dolphin/src/exi/EXIBios.c", cflags=cflags_sdk_exibios),
            Object(NonMatching, "dolphin/src/exi/EXIUart.c"),
        ],
    ),
    DolphinLib(
        "gx",
        [
            Object(Matching, "dolphin/src/gx/GXInit.c"),
            Object(Matching, "dolphin/src/gx/GXFifo.c"),
            Object(Matching, "dolphin/src/gx/GXAttr.c"),
            Object(Matching, "dolphin/src/gx/GXMisc.c"),
            Object(Matching, "dolphin/src/gx/GXGeometry.c"),
            Object(Matching, "dolphin/src/gx/GXFrameBuf.c"),
            Object(Matching, "dolphin/src/gx/GXLight.c"),
            Object(Matching, "dolphin/src/gx/GXTexture.c"),
            Object(Matching, "dolphin/src/gx/GXBump.c"),
            Object(Matching, "dolphin/src/gx/GXTev.c"),
            Object(Matching, "dolphin/src/gx/GXPixel.c"),
            Object(Matching, "dolphin/src/gx/GXDisplayList.c"),
            Object(Matching, "dolphin/src/gx/GXTransform.c"),
            Object(Matching, "dolphin/src/gx/GXPerf.c"),
        ],
    ),
    DolphinLib(
        "mix",
        [
            Object(Matching, "dolphin/src/mix/mix.c"),
        ],
    ),
    DolphinLib(
        "mtx",
        [
            Object(Matching, "dolphin/src/mtx/mtx.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/mtx/mtx44.c", extra_cflags=sdk_char_signed),
        ],
    ),
    DolphinLib(
        "odenotstub",
        [
            Object(Matching, "dolphin/src/odenotstub/odenotstub.c"),
        ],
    ),
    DolphinLib(
        "os",
        [
            Object(NonMatching, "dolphin/src/os/OS.c"),
            Object(Matching, "dolphin/src/os/OSAlarm.c"),
            Object(Matching, "dolphin/src/os/OSAlloc.c"),
            Object(Matching, "dolphin/src/os/OSArena.c"),
            Object(Matching, "dolphin/src/os/OSAudioSystem.c"),
            Object(Matching, "dolphin/src/os/OSCache.c"),
            Object(Matching, "dolphin/src/os/OSContext.c"),
            Object(Matching, "dolphin/src/os/OSError.c"),
            Object(NonMatching, "dolphin/src/os/OSFont.c"),
            Object(Matching, "dolphin/src/os/OSInterrupt.c"),
            Object(Matching, "dolphin/src/os/OSLink.c"),
            Object(Matching, "dolphin/src/os/OSMemory.c"),
            Object(Matching, "dolphin/src/os/OSMutex.c"),
            Object(NonMatching, "dolphin/src/os/OSReboot.c"),
            Object(NonMatching, "dolphin/src/os/OSReset.c"),
            Object(Matching, "dolphin/src/os/OSResetSW.c"),
            Object(Matching, "dolphin/src/os/OSRtc.c"),
            Object(Matching, "dolphin/src/os/OSSemaphore.c"),
            Object(Matching, "dolphin/src/os/OSSync.c"),
            Object(Matching, "dolphin/src/os/OSThread.c"),
            Object(Matching, "dolphin/src/os/OSTime.c"),
            Object(Matching, "dolphin/src/os/__start.c"),
            Object(Matching, "dolphin/src/os/__ppc_eabi_init.cpp"),
        ],
    ),
    DolphinLib(
        "pad",
        [
            Object(Matching, "dolphin/src/pad/Padclamp.c"),
            Object(Matching, "dolphin/src/pad/Pad.c"),
        ],
    ),
    DolphinLib(
        "si",
        [
            Object(Matching, "dolphin/src/si/SIBios.c"),
            Object(NonMatching, "dolphin/src/si/SISamplingRate.c"),
        ],
    ),
    DolphinLib(
        "vi",
        [
            Object(NonMatching, "dolphin/src/vi/vi.c"),
        ],
    ),
    {
        "lib": "OdemuExi2",
        "mw_version": "GC/1.3.2",
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "OdemuExi2/DebuggerDriver.c"),
        ],
    },
    RenderWareLib(
        "rpcollis",
        cflags_rpcollis,
        [
            Object(NonMatching, "rwsdk/plugin/collis/ctdata.c"),
            Object(NonMatching, "rwsdk/plugin/collis/colldata.c"),
            Object(NonMatching, "rwsdk/plugin/collis/collquery.c"),
        ],
    ),
    RenderWareLib(
        "rphanim",
        cflags_rphanim,
        [
            Object(NonMatching, "rwsdk/plugin/hanim/stdkey.c"),
            Object(NonMatching, "rwsdk/plugin/hanim/rphanim.c"),
        ],
    ),
    RenderWareLib(
        "rpmatfx",
        cflags_rpmatfx,
        [
            Object(NonMatching, "rwsdk/plugin/matfx/rpmatfx.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/effectPipesGcn.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcnData.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcnPipe.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcn.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/nbtGen.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/multiTex.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/multiTexEffect.c"),
        ],
    ),
    RenderWareLib(
        "rpptank",
        cflags_rpptank,
        [
            Object(NonMatching, "rwsdk/plugin/ptank/rpptank.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcncallbacks.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcnrender.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcntransforms.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_ppm.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_ppm.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_cs_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_cs_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_pps_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_pps_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_cs_ppr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_cs_ppr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_pps_ppr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_pps_ppr.c"),
        ],
    ),
    RenderWareLib(
        "rpskinmatfx",
        cflags_rpskinmatfx,
        [
            Object(NonMatching, "rwsdk/plugin/skin2/bsplit.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/rpskin.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skingcn.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skinstream.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/instance/instanceskin.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skinmatrixblend.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skingcnasm.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skinmatfxgcn.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skingcnm.c"),
        ],
    ),
    RenderWareLib(
        "rpusrdat",
        cflags_rpusrdat,
        [
            # Release seems to have an older version of rpusrdat for some reason.
            Object(NonMatching, "rwsdk/plugin/userdata/release/rpusrdat.c"),
        ],
    ),
    RenderWareLib(
        "rpworld",
        cflags_rpworld,
        [
            Object(NonMatching, "rwsdk/world/babinwor.c"),
            Object(NonMatching, "rwsdk/world/baclump.c"),
            Object(NonMatching, "rwsdk/world/bageomet.c"),
            Object(NonMatching, "rwsdk/world/balight.c"),
            Object(NonMatching, "rwsdk/world/bamateri.c"),
            Object(NonMatching, "rwsdk/world/bamatlst.c"),
            Object(NonMatching, "rwsdk/world/bamesh.c"),
            Object(NonMatching, "rwsdk/world/bameshop.c"),
            Object(NonMatching, "rwsdk/world/basector.c"),
            Object(NonMatching, "rwsdk/world/baworld.c"),
            Object(NonMatching, "rwsdk/world/baworobj.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/bapipew.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/gcpipe.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/vtxfmt.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/wrldpipe.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/nodeGameCubeAtomicAllInOne.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/nodeGameCubeWorldSectorAllInOne.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/gclights.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/gcmorph.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/native.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/setup.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/geomcond.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/geominst.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/ibuffer.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/instancegeom.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/instanceworld.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/itools.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/vbuffer.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/vtools.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/vtxdesc.c"),
        ],
    ),
    RenderWareLib(
        "rtanim",
        cflags_rtanim,
        [
            Object(NonMatching, "rwsdk/tool/anim/rtanim.c"),
        ],
    ),
    RenderWareLib(
        "rwcore",
        cflags_rwcore,
        [
            Object(NonMatching, "rwsdk/src/plcore/babinary.c"),
            Object(NonMatching, "rwsdk/src/plcore/bacolor.c"),
            Object(NonMatching, "rwsdk/src/plcore/baerr.c"),
            Object(NonMatching, "rwsdk/src/plcore/bamatrix.c"),
            Object(NonMatching, "rwsdk/src/plcore/bamemory.c"),
            Object(NonMatching, "rwsdk/src/plcore/baresour.c"),
            Object(NonMatching, "rwsdk/src/plcore/bastream.c"),
            Object(NonMatching, "rwsdk/src/plcore/batkbin.c"),
            Object(NonMatching, "rwsdk/src/plcore/batkreg.c"),
            Object(NonMatching, "rwsdk/src/plcore/bavector.c"),
            Object(NonMatching, "rwsdk/src/plcore/resmem.c"),
            Object(NonMatching, "rwsdk/src/plcore/rwgrp.c"),
            Object(NonMatching, "rwsdk/src/plcore/rwstring.c"),
            Object(NonMatching, "rwsdk/os/gcn/osintf.c"),
            Object(NonMatching, "rwsdk/src/babbox.c"),
            Object(NonMatching, "rwsdk/src/babincam.c"),
            Object(NonMatching, "rwsdk/src/babinfrm.c"),
            Object(NonMatching, "rwsdk/src/babintex.c"),
            Object(NonMatching, "rwsdk/src/bacamera.c"),
            Object(NonMatching, "rwsdk/src/badevice.c"),
            Object(NonMatching, "rwsdk/src/baframe.c"),
            Object(NonMatching, "rwsdk/src/baimage.c"),
            Object(NonMatching, "rwsdk/src/baimras.c"),
            Object(NonMatching, "rwsdk/src/baraster.c"),
            Object(NonMatching, "rwsdk/src/baresamp.c"),
            Object(NonMatching, "rwsdk/src/basync.c"),
            Object(NonMatching, "rwsdk/src/batextur.c"),
            Object(NonMatching, "rwsdk/src/batypehf.c"),
            Object(NonMatching, "rwsdk/driver/common/palquant.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dl2drend.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlconvrt.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dldevice.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlraster.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlrendst.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlsprite.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dltexdic.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dltextur.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dltoken.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/baim3d.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/bapipe.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2altmdl.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2core.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2define.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2dep.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2heap.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2renderstate.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2resort.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/gcn/im3dpipe.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/gcn/nodeDolphinSubmitNoLight.c"),
        ],
    ),
    RenderWareLib(
        "rtfsyst",
        cflags_rtfsyst,
        [
            # Release seems to have a different (older?) version of rtfsyst for some reason.
            Object(NonMatching, "rwsdk/tool/fsyst/release/rtfsmgr.c"),
        ],
    ),
    RenderWareLib(
        "rtintsec",
        cflags_rtintsec,
        [
            Object(NonMatching, "rwsdk/tool/intsec/rtintsec.c"),
        ],
    ),
    RenderWareLib(
        "rtskinsp",
        cflags_rtskinsp,
        [
            Object(NonMatching, "rwsdk/tool/sknsplit/skinsplit.c"),
            Object(NonMatching, "rwsdk/tool/sknsplit/rtskinsp.c"),
        ],
    ),
    RenderWareLib(
        "rtslerp",
        cflags_rtslerp,
        [
            Object(NonMatching, "rwsdk/tool/slerp/rtslerp.c"),
        ],
    ),

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

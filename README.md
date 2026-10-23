The Incredibles (GameCube)  
[![Build Status]][actions] <!--[![Code Progress]][progress] [![Data Progress]][progress]--> [![Discord Badge]][discord]
=============

[Build Status]: https://github.com/seilc/incredibles/actions/workflows/build.yml/badge.svg
[actions]: https://github.com/seilc/incredibles/actions/workflows/build.yml
<!--
decomp.dev progress badges (hidden until the project is registered on decomp.dev)
See https://decomp.dev/api for an API overview.
-->
[Code Progress]: https://decomp.dev/seilc/incredibles.svg?mode=shield&measure=code&label=Code
[Data Progress]: https://decomp.dev/seilc/incredibles.svg?mode=shield&measure=data&label=Data
[progress]: https://decomp.dev/seilc/incredibles
[Discord Badge]: https://img.shields.io/discord/829152115322257436?color=%237289DA&logo=discord&logoColor=%23FFFFFF
[discord]: https://discord.gg/dVbGFdYU6A

A work-in-progress matching decompilation of *The Incredibles* for the GameCube (USA, `GICE78`), built on Heavy
Iron's EvilEngine. Much of the engine is shared with the [Ratatouille prototype decomp](https://github.com/seilc/rat)
and the [Rise of the Underminer decomp](https://github.com/seilc/rotu), which this project ports matched code from.

This repository does **not** contain any game assets or assembly whatsoever. An existing copy of the game is required.

Full disclosure: this project is AI-assisted since October 23, 2026. The code matched before then was decompiled by
hand by the contributors below. Since then, an LLM has finished the move to decomp-toolkit, and new code is ported
or decompiled with its help, then reviewed and checked against the original binary. Matching code is
byte-identical to the original, but names, types and source structure beyond what the binary and its debug info
prove may be guesses.

Supported versions:

- `GICE78` - The Incredibles (USA), retail `main.dol`

Building
========

- Install Python 3 and ninja (on Linux/macOS also [wibo](https://github.com/decompals/wibo) or wine; `configure.py`
  downloads wibo automatically).
- Copy `sys/main.dol` from the disc to `orig/GICE78/sys/main.dol` (SHA-1 `c7413be2fba5b8cc47b7506ecb7785677a4a1c12`).
- Run `python configure.py`, then `ninja`.

The project is built with [decomp-toolkit](https://github.com/encounter/decomp-toolkit) on the
[dtk-template](https://github.com/encounter/dtk-template) layout; [objdiff](https://github.com/encounter/objdiff)
reads the generated `objdiff.json`.

Special Thanks
==============

**[@seilc](https://github.com/seilc)** -
Starting the project, working on tools, decompilation.

**[@bluisblu](https://github.com/bluisblu)** -
Porting the project to decomp-toolkit.

**[@mattbruv](https://github.com/mattbruv)** -
The original project tracking website and inlineasm tool, decompilation.

**[@DarkRTA](https://github.com/DarkRTA)** -
Creating the formatting rules, improving the build process, decompilation.

**[@stravant](https://github.com/stravant)** -
Writing [documentation](https://github.com/bfbbdecomp/bfbb/blob/master/docs/WalkthroughAndTips.md).

**[@mkst](https://github.com/mkst)** ([conker](https://github.com/mkst/conker)) -
Continuous Integration for the original build.

And to [everyone else](https://github.com/seilc/incredibles/graphs/contributors) who has contributed to the project
in any way.

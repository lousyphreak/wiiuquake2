# wiiuQuake2

This port is based on yquake2, and the gl3 renderer. The main game and xatrix/rogue
addons run stable (startup demo loop for hours).
Runs (mostly) at 60 fps, except if there are too many dynamic lights active, and
sometimes has short hangs (probably reading music from the sd-card).

Built with 100% open source tools, tested with Cemu and on real hardware.

## Installing

Install the Quake 2 data files to your sdcard "quake2" folder, and the wuhb to "wiiu/apps":

```
sdcard
| - quake2
| | - baseq2
| | | - pak0.pak
| | | - <other files>
| | - rogue (optional)
| | | - pak0.pak
| | | - <other files>
| | - xatrix (optional)
| | | - pak0.pak
| | | - <other files>
| - wiiu
  | - apps
    | - quake2.wuhb
```

## Building

You need:

- [devkitPro wiiu toolchain](https://github.com/devkitPro/wut)
- (optional, for music playback) ogg libraries for PPC (from the devkiPro pacman)
- [CafeGLSL](https://github.com/Exzap/CafeGLSL) with [PR #3](https://github.com/Exzap/CafeGLSL/pull/3)
  or directly from the [fork](https://github.com/Crementif/CafeGLSL/tree/main)
  to build the offline GLSL compiler, and installed to `${DEVKITPRO}/tools/bin/glslcompiler.elf`

```sh
mkdir build
cd build
${DEVKITPRO}/portlibs/wiiu/bin/powerpc-eabi-cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

The resulting `quake2.wuhb` can be installed on the sd-card for use in the WiiU, or launched directly by Cemu.
The only other data required is the quake2 data files on the sd-card.

## Bugs

- Currently none known

### Cemu Bug

There is a bug in the Cemu PPC recompiler (i think), that causes endian conversion for floats
to fail, if you run into this you can compile with a define `__FLOAT_HACK__`, or run Cemu with
`--force-interpreter`. This also does not affect real hardware.

## Added CVars

- **r_lightmaps_unfiltered, r_3D_unfiltered**

    similar to r_videos_unfiltered and r_2D_unfiltered, but applied to lightmaps and all 3d content

- **wiiu_debugmips**

    replace all textures with a color representing their mip level. can only be set (reliably) before
    startup because it does not reload textures

- **wiiu_drc**

    copy TV image to DRC (make game playable on WiiU gamepad), has small performance hit

- **wiiu_lightcap**

    cap the maximum number of dynamic lights, currently > 4 may cause slowdowns. Defaults to 32 (the maximum).

## TODO

- optimize the lightmap shader (3d_lm(flow)_frag), currently it is split into multiple shaders
  each supporting different lights.
- exiting from ingame, exit via HOME button or console shutdown works
- FBO (post effects), has partial implementation
- multisampling, requires reimplementation of WUT gfx implementation
- stencil & scissor (shadows?), requires reimplementation of WUT gfx implementation
- round particles, gl_PointCoord always zero, might be some broken state

## (maybe interesting?) things for WiiU devs

Some things in here that i have not yet seen in any other demo, so feel free to "steal".

- Mip-Mapped textures (src/client/refresh/wiiu/gl3_image.c in WiiU_Upload32)

  the generation of them is meh, but getting the offsets right was a pain (Cemu + dumping was a great help)

- Copy rendered screen from TV to DRC (src/client/refresh/wiiu/gl3_sdl.c in WiiU_EndFrame)

  using a VERY simple shader for downsampling (even with hardcoded resolution)

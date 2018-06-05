<p align="center">
  <a href="https://prilik.com/ANESE">
    <img height="128px" src="resources/icons/anese.ico" alt="ANESE Logo">
  </a>
</p>
<p align="center">
  <a href="https://ci.appveyor.com/project/daniel5151/anese">
    <img src="https://ci.appveyor.com/api/projects/status/qgy19m8us3ss6ilt?svg=true" alt="Build Status Windows">
  </a>
  <a href="https://travis-ci.org/daniel5151/ANESE">
    <img src="https://travis-ci.org/daniel5151/ANESE.svg?branch=master" alt="Build Status macOS/Linux">
  </a>
</p>

**ANESE** (**A**nother **NES** **E**mulator) is a Nintendo Entertainment System
Emulator being written for fun and learning.

Accuracy is a long-term goal, but the primary goal is simply getting ANESE to
play some of the more popular titles. As of now, most basic Mappers have been
implemented, so popular titles should be working! :smile:

ANESE is built with _cross-platform_ in mind, and should build on all major
platforms (macOS, Windows, and Linux). ANESE doesn't use any vendor-specific
language extensions, and is compiled with strict compiler flags.
It is also linted (fairly) regularly.

ANESE strives for clean and _interesting_ C++11 code, with an emphasis on
readability and maintainability. Performance is important, but it's not a
primary focus.

## Downloads

Right now, there are no proper binary releases.

[AppVeyor](https://ci.appveyor.com/project/daniel5151/anese) does create build
artifacts for every commit of ANESE though, so you could grab Windows binaries
from there.

## Building

ANESE uses **CMake**, so make sure it is installed.

ANESE's emulation core (src/nes) doesn't have any major dependencies, (aside
from the c standard library), but there are a couple of libraries for the UI.
Most of these dependencies are bundled with ANESE (see: /thirdparty), although
some require additional installation:

- **SDL2** (video/audio/controls)
  - _Linux_: `apt-get install libsdl2-dev` (on Ubuntu)
  - _MacOS_: `brew install SDL2`
  - _Windows_:
    - Download dev libs from [here](https://www.libsdl.org/download-2.0.php)
    - EITHER:
      - Set the `SDL` environment variable to point to the dev libs
      - Modify the `SDL2_MORE_INCLUDE_DIR` variable in `CMakeLists.txt` to point
        to the SDL2 dev libs (or just plop them down into `C:\sdl2\`)

Once that's installed, it's a standard CMake install
```bash
# in ANESE root
mkdir build
cd build
cmake ..
make
make install # on macOS: creates ANESE.app in ANESE/bin/
```

On Windows, building is very similar:
```bat
mkdir build
cd build
cmake ..
msbuild anese.sln /p:Configuration=Release
```

If you're interested in looking under the hood of the PPU, you can pass the
`-DDEBUG_PPU` flag to cmake and get an `anese` build with some neat debug info.

## Running

ANESE can run from the shell using `anese [rom.nes]` syntax.

If no ROM is provided, a simple dialog window pops-up prompting the user to
select a valid NES rom.

For a full list of switches, run `anese -h`

**Windows Users:** make sure the executable can find `SDL2.dll`! Download the
runtime DLLs from the SDL website, and plop them in the same directory as
anese.exe

## Controls

Currently hard-coded to the following:

Button | Key         | Controller
-------|-------------|------------
A      | Z           | X
B      | X           | A
Start  | Enter       | Start
Select | Right Shift | Select
Up     | Up arrow    | D-Pad
Down   | Down arrow  | D-Pad
Left   | Left arrow  | D-Pad
Right  | Right arrow | D-Pad

Any xbox-compatible controller should work.

There are also a couple of emulator actions:

Action             | Keys
-------------------|--------
Exit               | Esc
Reset              | Ctrl - R
Power Cycle        | Ctrl - P
Toggle CPU logging | Ctrl - C
Speed++            | Ctrl - =
Speed--            | Ctrl - -
Fast-Forward       | Space
Make Save-State    | Ctrl - \`
Load Save-State    | Ctrl - 1

## DISCLAIMERS

- The CPU is _instruction-cycle_ accurate, but not _sub-instruction cycle_
accurate. While this inaccuracy doesn't affect most games, there are some that
that rely on sub-instruction level timings (eg: Solomon's Key).
  - The `--alt-nmi-timing` flag might fix some of these games.
- The APU is _not my code_. I wanted to get ANESE partially up and
running before new-years 2018, so I'm using Blargg's venerable `nes_snd_emu`
library to handle sound (for now). Once I polish up some of the other aspects
of the emulator, I'll try to revisit my own APU implementation (which is
currently a stub)

## Attributions

- A big shout-out to [LaiNES](https://github.com/AndreaOrru/LaiNES) and
[fogleman/nes](https://github.com/fogleman/nes), two solid NES emulators that I
referenced while implementing some particularly tricky parts of the PPU). While
I actively avoided looking at the source codes of other NES emulators
as I set about writing my initial implementations of the CPU and PPU, I did
sneak a peek at how others did some things when I got very stuck.
- Blargg's [nes_snd_emu](http://www.slack.net/~ant/libs/audio.html) makes ANESE
  sound as good as it does :smile:
- These awesome libraries make ANESE a lot nicer to use:
  - [sdl2](https://www.libsdl.org/)
  - [args](https://github.com/Taywee/args)
  - [miniz](https://github.com/richgel999/miniz)
  - [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/)

## TODO

- Key Milestones
  - [x] Parse iNES files
  - [x] Create Cartridges (iNES + Mapper interface)
  - [x] CPU
    - [x] Set Up Memory Map
    - [x] Hardware Structures (registers)
    - [x] Core Loop / Basic Functionality
      - [x] Read / Write RAM
      - [x] Addressing Modes
      - [x] Fetch - Decode - Execute
    - [x] Official Opcodes Implemented
    - [x] Handle Interrupts
  - [x] PPU
    - [x] Set Up Basic Rendering Context (SDL)
    - [x] Implement Registers + Memory Map them
    - [x] Implement DMA
    - [x] Generate NMI -> CPU
    - [x] Core rendering loop
      - [x] Background Rendering
      - [x] Sprite Rendering - _currently not hardware accurate_
      - [x] Proper Background / Foreground blending
    - [x] Sprite Zero Hit
    - [ ] Misc PPU flags (emphasize RGB, Greyscale, etc...)
  - [ ] APU - ***Uses `nes_snd_emu` by Blargg***
    - [x] Implement Registers + Memory Map them
    - [ ] Frame Timer IRQ
    - [ ] Set Up Basic Sound Output Context (SDL)
    - [ ] Channels
      - [ ] PCM 1
      - [ ] PCM 2
      - [ ] Triangle
      - [ ] Noise
      - [ ] DMC
    - [ ] DMC DMA
  - [ ] Joypads
    - [x] Basic Controller
    - [ ] Zapper
    - [ ] NES Four Score

- Ongoing Tasks
  - Better Usability
    - [x] Loading Files with picker
    - [x] Reset / Power-cycle
    - [x] Fast Forward
    - [ ] Run / Pause / Step
    - Saving
      - [x] Battery Backed RAM - Saves to `.sav`
      - [x] Save-states
        - [ ] Dump to file
    - [ ] Config File
      - [ ] Remap controls
    - [x] Running NESTEST (behind a flag)
    - [x] Controller support - _currently very basic_
    - Full-fledged in application GUI
      - [ ] imgui maybe?
  - Accuracy & Compatibility improvements
    - More Mappers
      - [x] 000
      - [x] 001
      - [x] 002
      - [x] 003
      - [x] 004
      - [x] 007
      - [?] 009 - _(Punch Out!)_ Relies on accurate PPU sprite fetches
      - ...
    - CPU
      - [ ] Implement Unofficial Opcodes
      - [ ] Pass More Tests yo
      - [ ] _\(Stretch\)_ Switch to sub-instruction level cycle-based emulation
            (vs instruction level)
    - PPU
      - [ ] Make the sprite rendering pipeline more accurate (fetch-timings)
        - This _should_ fix _Punch Out!!_
      - [ ] Pass More Tests yo
      - [ ] Make value in PPU <-> CPU bus decay
  - Cleanup Code
    - [ ] Modularize `main.cc`
    - Movies
      - [ ] Conform to the `.fm2` movie format better (in parsing and recording)
    - Better file parsing
      - [ ] Actually bounds-check stuff lol
    - Better error handling and logging
      - [ ] Remove fatal asserts (?)
      - [ ] Switch to a better logging system (\*cough\* not fprintf \*cough\*)
    - [ ] Unify naming conventions (either camelCase or snake_case)
    - Better cmake config

- Fun Bonuses
  - Features!
    - [x] Zipped ROM support
    - [ ] Rewind
    - [ ] Game Genie
    - [x] Movie recording and playback
    - [ ] Proper PAL handling?
  - [ ] Debugger!
    - [ ] CPU
      - [x] Step through instructions - _super jank, no external flags_
    - [ ] PPU Views
      - [x] Static Palette
      - [x] Palette Memory
      - [x] Pattern Tables
      - [x] Nametables
      - [ ] OAM memory
  - [ ] PPU
    - [ ] Proper NTSC artifacting
  - [ ] Write a NES rom to simulate TV static, and run it if no ROM inserted
  - Multiple Frontends
    - [x] SDL Standalone
    - [ ] LibRetro
    - ...
  - [ ] Add support for more ROM formats (not just iNES)

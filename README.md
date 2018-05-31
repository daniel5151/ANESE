<h1 align="center">
  <img height="128px" src="resources/icons/anese.ico" alt="ANESE Logo">
</h1>

**ANESE** (**A**nother **NES** **E**mulator) is a Nintendo Entertainment System
Emulator being written for fun and learning.

Accuracy is a long-term goal, but the primary goal is simply getting ANESE to
play some of the more popular titles :smile:

I'm aiming for clean and _interesting_ C++11 code, with a emphasis on keeping
the source readable and maintainable. Performance is important, but it's not a
primary focus.

ANESE is built with _cross-platform_ in mind, and building should work on macOS,
Linux (Ubuntu), and Windows. The C++ doesn't rely on any vendor-specific
extentions, and is compiled with relatively strict compiler flags. The code is
linted regularly.

Lastly, I actively avoided looking at the source codes of other NES emulators
while writing initial implementations of the CPU and PPU, since I thought it
would be fun to figure things out myself :D

That said, a big shout-out to [LaiNES](https://github.com/AndreaOrru/LaiNES) and
[fogleman/nes](https://github.com/fogleman/nes), two solid NES emulators that I
referenced while implementing some particularly tricky parts of the PPU)

## Building

ANESE uses **CMake**, so make sure it is installed.

ANESE's emualtion core doesn't have any major dependencies (aside from clib),
but there are a couple used for the UI.
Most of these dependencies are bundled with ANESE (see: /thirdparty), although
some require additional installation:

- **SDL2** (video/audio/controls)
  - _Linux_: `apt-get install libsdl2-dev` (on Ubuntu)
  - _MacOS_: `brew install SDL2`
  - _Windows_:
    - Download dev libs from [here](https://www.libsdl.org/download-2.0.php)
    - Modfiy the `SDL2_MORE_INCLUDE_DIR` variable in `CMakeLists.txt` to point
      to the SDL2 dev libs (or just plop them down into `C:\sdl2\`)

```bash
# in ANESE root
mkdir build
cd build
cmake ..
make
```

Building on Windows has been tested with VS 2017 using MSVC.

If you're interested in looking under the hood of the PPU, you can pass the
`-DDEBUG_PPU` flag to cmake and get an `anese` build with some neat debug info.

## Running

ANESE can run from the shell using `anese [rom.nes]` syntax.

If no ROM is provided, a simple dialog window pops-up prompting the user to
select a valid NES rom.

For a full list of switches, run `anese -h`

**Windows Users:** make sure the executable can find `SDL2.dll`!

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

There are also a couple emulator actions:

Action             | Keys
-------------------|--------
Exit               | Esc
Reset              | Ctrl - R
Power Cycle        | Ctrl - P
Toggle CPU logging | Ctrl - C
Speed++            | Ctrl - =
Speed--            | Ctrl - -
Fast-Forward       | Space

## DISCLAIMERS

I wrote my CPU emulator to be _instruction-length cycle_ accurate, but not
_sub-instruction cycle_ accurate. This doesn't affect most games, but there are
a couple that rely on sub-instruction level timings (eg: Solomon's Key). Use the
`--alt-nmi-timing` flag to turn on a hack that fixes some of these games.

**NOTE:** The APU is _not my code_. I wanted to get ANESE partially up and
running before new-years 2018, so I've used Blargg's venerable `nes_snd_emu`
library to handle sound (for now). Once I polish up some of the other aspects
of the emulator, I will revisit my own APU implementation (which is currently
stubbed)

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
      - [x] Battery Backed RAM
      - [ ] Save-states
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
      - ...
    - [ ] Proper PAL handling
    - CPU
      - [ ] Implement Unofficial Opcodes
      - [ ] Pass More Tests yo
      - [ ] _\(Stretch\)_ Switch to sub-instruction level cycle-based emulation
            (vs instruction level)
    - PPU
      - [ ] Make value in PPU <-> CPU bus decay
      - [ ] Pass More Tests yo
  - Cleanup Code
    - [ ] OH GOD DON'T LOOK AT `main.cc` I'M SORRY IN ADVANCE
      - [ ] Modularize EVERYTHING and get it out of main!
    - Movies
      - [ ] Confrom to the `.fm2` movie format better (in parsing and recording)
      - [ ] Make movie recording a seperate class (and not a bunch of ad-hoc f
            printfs scattered around the codebase)
    - Better file parsing
      - [ ] Actually bounds-check stuff
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

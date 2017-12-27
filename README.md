<h1 align="center">
  <img height="128px" src="resources/icons/anese.ico" alt="ANESE Logo">
</h1>

**ANESE** (**A**nother **NES** **E**mulator) is a Nintendo Entertainment System
Emulator being written for fun and learning.

Strong Accuracy is a long-term goal, but the primary goal is simply getting
ANESE to play some of the more popular titles :smile:

I am aiming for clean and _interesting_ C++11 code, with a emphasis on keeping
the source readable, maintainable, and comprehensible. Performance is important,
but not a primary focus.

**NOTE:** The APU is _not my code_. I wanted to get ANESE up and running before
new-years 2018, so i've decided to use Blargg's venerable `nes_snd_emu` library
to handle sound (for now). Once I polish up some of the other aspects of the
emulator, I will revisit my own APU implementation (which is currently just a
stub)

It is being built with _cross-platform_ in mind, with builds being tested on
MacOS, Linux (Ubuntu), and Windows regularly, with strict compiler flags, and
regular `cppcheck` linting.


Lastly, I am trying to avoid looking at the source codes of other NES emulators,
since IMO, half the fun of writing a emulator is figuring things out yourself :D

(That said, big shoutout to [LaiNES](https://github.com/AndreaOrru/LaiNES) and
[fogleman/nes](https://github.com/fogleman/nes), two solid projects who's code I
referenced when implementing some particularly tricky parts of the PPU)

## Building

ANESE uses **CMake**, so make sure it is installed.

ANESE's core doesn't have any hard dependencies, but there are a couple used by
the UI. Most of these dependencies are either bundled with ANESE
(see: /thirdparty), although some do require additional installation:

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
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

Building on Windows has been tested with VS 2017 using MSVC.

**NOTE:** make sure to build ANESE in _release_ configuration, since without
the compiler optimizations, it's bloody slow. This is because I wrote the code
to be clean and neat, without worrying about performance.

## Running

Just open `anese`, and a file select dialog should prompt you for a rom file.

Alternatively, it can be called from the shell using standard `anese [rom.nes]`
syntax.

For a full list of switches, run `anese -h`

**Windows Users:** make sure the executable can find SDL2.dll.

## Controls

Currently hardcoded to the following:

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
  - [ ] PPU
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
    - [ ] Set Up Basic Sound Ouput Context (SDL)
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
    - [ ] Multitap

- Ongoing Tasks
  - Better Usability
    - [x] Loading Files with picker
    - [x] Reset / Power-cycle
    - [ ] Run / Pause / Stop
    - [x] Fast Forward
    - [ ] Config File
      - [ ] Remap controls
    - [x] Running NESTEST (behind a flag)
    - [x] Controller support - _currently very basic_
    - Saving
      - [ ] Battery Backed RAM
      - [ ] Save-states
  - Accuracy & Compatibility improvements
    - More Mappers
      - [x] 000
      - [x] 001
      - [ ] 002
      - [ ] 003
      - [ ] 004
      - [ ] 005
      - [ ] 006
      - ...
    - [ ] Proper PAL handling
    - CPU
      - [ ] Implement Unofficial Opcodes
      - [ ] Pass More Tests yo
      - [ ] _\(Stretch\)_ Switch to cycle-based emulation (vs instruction level)
    - PPU
      - [ ] Make value in PPU <-> CPU bus decay
      - [ ] Pass More Tests yo
  - Cleanup Code
    - [ ] OH GOD DON'T LOOK AT `main.cc` I'M SORRY IN ADVANCE
      - [ ] Modularize UI code (i.e: get it out of main.cc)
    - [ ] Unify naming conventions (either camelCase or snake_case)
    - Better error handling and logging
      - [ ] Remove fatal asserts (?)
      - [ ] Switch to a better logging system (\*cough\* not fprintf \*cough\*)
    - Better cmake config

- Fun Bonuses
  - Features!
    - [x] Zipped ROM support
    - [ ] Rewind
    - [ ] Game Genie
  - [ ] Debugger!
    - [ ] CPU
      - [ ] Serialize state
      - [ ] Step through instructions
    - [ ] PPU
      - [ ] Proper NTSC artifacting
  - [ ] Write a NES rom to simulate TV static, and run it if no ROM inserted
  - Multiple Frontends
    - [x] SDL (current default)
    - [ ] LibRetro
    - ...
  - [ ] Add support for more ROM formats (not just iNES)

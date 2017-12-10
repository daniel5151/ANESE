# ANESE

ANESE (**A**nother **NES** **E**mulator) is a Nintendo Entertainment System
Emulator being written for fun and learning.

I am aiming for accuracy in the long run, but the primary goal is simply getting
it to play some of the more popular titles :smile:

I am aiming for clean and _interesting_ C++11 code, with a emphasis on keeping
the source readable, maintainable, and comprehensible. Performance is important,
but not a primary focus.

It is being built with _cross-platform_ in mind, with builds being tested on
MacOS, Linux (Ubuntu), and Windows regularly, with strict compiler flags, and
regular `cppcheck` linting.


Lastly, I am trying to avoid looking at the source codes of other NES emulators,
since IMO, half the fun of writing a emulator is figuring things out yourself :D

## Building

ANESE tries to keep external dependencies to a minimum

- **SDL2** (rendering layer)
  - _Linux_: `apt-get install libsdl2-dev` (on Ubuntu)
  - _MacOS_: `brew install SDL2`
  - _Windows_:
    - Download dev libs from [here](https://www.libsdl.org/download-2.0.php)
    - Modfiy the `SDL2_MORE_INCLUDE_DIR` variable in `CMakeLists.txt` to point
      to the SDL2 dev libs

ANESE uses **CMake**, so make sure you have it installed.

```bash
# in ANESE root
mkdir build
cd build
cmake ..
make
```

Building on Windows has been tested with VS 2017 using both MSVC and Clang.
At the moment, it is reccomended to build with Clang, as unlike MSVC, it can
optimize ANESE code well enough to hit 60 fps.

## Running

Until I get around to writing a proper UI for it, ANESE needs to be called from
the commandline: `anese [rom_file]`

On Windows, make sure the executable can find SDL2.dll.

## Controls

Currently hardcoded to the following:

Button | Key
-------|-----
A      | Z
B      | X
Start  | Enter
Select | Right Shift
Up     | Up arrow
Down   | Down arrow
Left   | Left arrow
Right  | Right arrow

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
    - [x] Set Up Memory Map
    - [x] Basic Hardware Structures
      - [x] Framebuffer (technically not in hardware)
      - [x] Registers
        - [x] Memory Map them
    - [x] Implement DMA
    - [x] Generate NMI -> CPU
    - [ ] Core rendering loop
      - [ ] Background Rendering
      - [x] Sprite Rendering - _not hardware accurate_
      - [ ] Proper Background / Foreground blending
    - [ ] Sprite Zero Hit
  - [ ] APU
    - [x] Basic Register Layout
    - [ ] Frame Timer IRQ
  - [ ] Joypads
    - [x] Basic Controller
    - [ ] Zapper
    - [ ] Multitap

- Ongoing Tasks
  - Better Usability
    - [ ] Menus
      - [ ] Loading Files
      - [ ] Run / Pause / Stop
      - [ ] Remap controls
    - [ ] Config File
    - [ ] Running NESTEST (behind a flag)
    - [ ] Xbox Controller support
    - Saving
      - [ ] Battery Backed RAM
      - [ ] Save-states
  - Accuracy & Compatibility improvements
    - More Mappers
      - [x] 000
      - [ ] 001
      - [ ] 002
      - [ ] 003
      - [ ] 004
      - [ ] 005
      - [ ] 006
      - ...
    - [ ] Proper PAL handling
    - CPU
      - [ ] Implement Unofficial Opcodes
      - [ ] Better handling of simultaneous interrupts
      - [ ] _\(Stretch\)_ Switch to cycle-based emulation (vs instruction level)
    - PPU
      - [ ] Make value in PPU <-> CPU bus decay
    - JOY
      - [ ] Multitap support
      - [ ] More nes controller types
  - Cleanup
    - [ ] Get SDL out of the main function!
    - [ ] Unify method naming (either camelCase or snake_case)
    - [ ] Be more explicit with copy / move ctors
    - [ ] Add `const` throughout the codebase (?)
    - Better error handling & logging
      - [ ] Remove asserts
      - [ ] Switch to a better logging system (\*cough\* not fprintf \*cough\*)
    - Better cmake scripts

- Fun Bonuses
  - [ ] Debugger!
    - [ ] CPU
      - [ ] Serialize state
      - [ ] Step through instructions
    - [ ] PPU
      - [ ] proper NTSC artifacting
  - [ ] Write a NES rom to simulate TV static, and have that run if no ROM is
        chosen
  - Multiple Frontends
    - [x] SDL (current default)
    - [ ] LibRetro
    - ...
  - [ ] Add support for more ROM formats (not just iNES)

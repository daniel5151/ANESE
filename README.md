# ANESE

ANESE (**A**nother **NES** **E**mulator) is a _cycle_ accurate Nintendo
Entertainment System Emulator written for fun and learning.

Hopefully, it will support most popular titles :smile:

I am aiming for clean, performant, and _interesting_ C++11 code, with a emphasis
on keeping the source readable, maintainable, and comprehensible.

It is being built with _cross-platform_ in mind, with builds being tested on
MacOS, Linux (Ubuntu), and Windows regularly, with strict compiler flags, and
regular `cppcheck` linting.


Lastly, I actively avoiding looking at the source codes of other NES emulators
during early development, since IMO, half the fun of writing a emulator is
figuring things out yourself :grin:

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

Building on Windows has been tested with VS 2017

## Running

Until I get around to writing a proper UI for it, ANESE needs to be called from
the commandline: `anese [rom_file]`

On Windows, make sure the executable can find SDL2.dll.

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
    - [ ] Hardware Structures
      - [x] Framebuffer
      - [x] Registers
        - [x] Memory Map them
      - ...
    - [x] Implement DMA
    - [ ] Generate NMI -> CPU
    - [ ] Core rendering loop
      - ...
    - ...
  - [ ] APU
    - ...

- Ongoing Tasks
  - Better Usability
    - [ ] Menus
      - [ ] Loading Files
      - [ ] Run / Pause / Stop
      - [ ] Remap controls
    - [ ] Config File
    - [ ] Running NESTEST (behind a flag)
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
      - TBD
  - [ ] Write a NES rom to simulate TV static, and have that run if no ROM is
        chosen
  - Multiple Frontends
    - [x] SDL (current default)
    - [ ] LibRetro
    - ...
  - [ ] Add support for more ROM formats (not just iNES)

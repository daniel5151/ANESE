# ANESE

ANESE (Another NES Emulator) is a Nintendo Entertainment System Emulator written
for fun and learning.

ANESE strives to be a accurate NES emulator, even at the expense of performance.

I am aiming for clean, performant C++11 code, with a emphasis on readability.

Also, for the sake of future cross-platform support, I am trying to keep the
code portable across compilers, and not reliant on any gcc / clang specific
features.

## Building

ANESE uses CMake, so make sure you have it installed.

```bash
# in ANESE root
mkdir build
cd build
cmake ..
make
```

Building on Windows has been tested with VS 2017.

## TODO

- Key Milestones
  - [x] Parse iNES files
  - [x] Create Cartridges (iNES + Mapper interface)
  - [ ] CPU
    - [x] Set Up Memory Map
    - [x] Core Loop / Basic Functionality
      - Read / Write RAM
      - Addressing Modes
      - Fetch - Decode - Execute
    - [x] Official Opcodes Implemented
    - [ ] Handle Interrupts
    - [ ] Unofficial Opcodes Implemented (?)
  - [ ] PPU
    - [ ] Set Up Memory Map
    - TBD
  - [ ] APU
    - TBD

- Ongoing Tasks
  - Better error handling (something like Result in Rust maybe?)
    - [ ] Remove asserts
  - Implement more Mappers
    - [ ] 000
    - [ ] 001
    - [ ] 002
    - [ ] 003
    - [ ] 004
    - [ ] 005
    - [ ] 006
    - ...
  - Add `const` throughout the codebase?

- Fun Bonuses
  - [ ] Write a NES rom to simulate TV static, and have that run if no ROM is
        chosen
  - [ ] LibRetro support
  - [ ] Add support for more ROM formats (not just iNES)

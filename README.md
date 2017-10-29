# ANESE

ANESE (Another NES Emulator) is a Nintendo Entertainment System Emulator written
for fun and learning.

ANESE strives to be a accurate NES emulator, but not _too_ accurate. i.e: cycle
accuracy yes, but things like bus-conflicts and other hardware minutia... no.

I am aiming for clean, performant C++11 code, with a emphasis on readability.

Also, for the sake of _cross-platform_ support, I am trying to keep the code
portable and standard conforming.

Lastly, I actively avoiding looking at the source codes of other NES emulators,
since IMO, doing so would make the challenge of writing your own a lot less fun
:smile:

## Building

ANESE has some external dependencies

- **SDL2** (rendering layer)
  - _Linux_: Varies, but for Ubuntu it's `apt-get install libsdl2-dev`
  - _MacOS_: `brew install SDL2`
  - _Windows_:
    - Download dev libs from [here](https://www.libsdl.org/download-2.0.php)
    - Modfiy the `SDL2_MORE_INCLUDE_DIR` variable in `CMakeLists.txt` to point
      to the SDL2 dev libs

ANESE uses CMake, so make sure you have it installed.

```bash
# in ANESE root
mkdir build
cd build
cmake ..
make
```

Building on Windows has been tested with VS 2017.

## Running

Simple as `anese [rom_file]`

On Windows, make sure the executable can find SDL2.dll.

## TODO

- Key Milestones
  - [x] Parse iNES files
  - [x] Create Cartridges (iNES + Mapper interface)
  - [x] CPU
    - [x] Set Up Memory Map
    - [x] Core Loop / Basic Functionality
      - [x] Read / Write RAM
      - [x] Addressing Modes
      - [x] Fetch - Decode - Execute
    - [x] Official Opcodes Implemented
    - [x] Handle Interrupts
  - [ ] PPU
    - [x] Set Up Memory Map
    - [ ] Set Up Basic Rendering Context (SDL)
    - TBD
  - [ ] APU
    - TBD

- Ongoing Tasks
  - Pass more tests (i.e: Accuracy)
    - CPU
      - [ ] Implement Unofficial Opcodes (?)
      - [ ] Better handling of simultaneous interrupts
      - [ ] _\(Stretch\)_ Switch to cycle-based emulation (vs instruction level)
  - Better error handling (but without c++ exceptions)
    - [ ] Remove asserts
  - Implement more Mappers
    - [x] 000
    - [ ] 001
    - [ ] 002
    - [ ] 003
    - [ ] 004
    - [ ] 005
    - [ ] 006
    - ...
  - Add `const` throughout the codebase?
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
  - [ ] LibRetro support
  - [ ] Add support for more ROM formats (not just iNES)

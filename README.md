# ANESE

ANESE (Another NES Emulator) is a Nintendo Entertainment System Emulator written
for fun and learning.

I am aiming for clean, readable code, even if that means taking a performance
hit here and there.

## TODO

- Key Milestones
  - [x] Parse iNES files
  - [x] Create Cartridges (iNES + Mapper interface)
  - [ ] CPU
    - [x] Set Up Memory Map
    - [ ] Core Loop / Basic Functionality
      - Read / Write RAM
      - Addressing Modes
      - Fetch - Decode - Execute
    - [ ] Opcodes
    - [ ] Handle Interrupts
  - [ ] PPU
    - [ ] Set Up Memory Map
    - TBD
  - [ ] APU
    - TBD

- Ongoing Tasks
  - Better error handling (something like Result in Rust)
    - [ ] Remove asserts
  - Implement more Mappers
    - [ ] 000
    - [ ] 001
    - [ ] 002
    - [ ] 003
    - [ ] 004
    - [ ] 005
    - [ ] 006
  - Add `const` throughout the codebase?

- Fun Bonuses
  - [ ] Write a NES rom to simulate TV static, and have that run if no ROM is
        chosen
  - [ ] LibRetro support

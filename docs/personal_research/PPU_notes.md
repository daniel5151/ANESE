# PPU Notes

- Writes to the following registers are ignored if earlier than ~29658 CPU
  clocks after reset:
  - PPUCTRL
  - PPUMASK
  - PPUSCROLL
  - PPUADDR
  - This also means that the PPUSCROLL/PPUADDR latch will not toggle.
  - The other registers work immediately
- If the NES is powered on after having been off for less than 20 seconds,
  register writes are ignored as if it were a reset, and register starting
  values differ:
  - PPUSTATUS = $80 (VBlank flag set)
  - OAMADDR = $2F or $01
  - PPUADDR = $0001
  - **i'm probably _not_ handling this case**
- The Reset button on the Control Deck resets the PPU only on the front-loading
  NES (NES-001). On top-loaders (Famicom, NES-101), the Reset button resets only
  the CPU.

#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"

#include "nes/wiring/dma.h"
#include "nes/wiring/interrupt_lines.h"

namespace PPURegisters {
  enum Reg {
    PPUCTRL   = 0x2000,
    PPUMASK   = 0x2001,
    PPUSTATUS = 0x2002,
    OAMADDR   = 0x2003,
    OAMDATA   = 0x2004,
    PPUSCROLL = 0x2005,
    PPUADDR   = 0x2006,
    PPUDATA   = 0x2007,
    OAMDMA    = 0x4014,
  };
} // PPURegisters

// http://wiki.nesdev.com/w/index.php/PPU_programmer_reference
class PPU final : public Memory {
private:

  /*----------  "Hardware"  ----------*/
  // In quotes because technically, these things aren't located on the PPU, but
  // by coupling them with the PPU, it makes the emulator code cleaner

  // CPU WRAM -> PPU OAM Direct Memory Access (DMA) Unit
  DMA& dma;

  /*-----------  Hardware  -----------*/

  InterruptLines& interrupts;

  Memory& mem; // PPU 16 bit address space (should be wired to ppu_mmu)
  Memory& oam; // PPU Object Attribute Memory

  u8 cpu_data_bus; // PPU <-> CPU data bus (filled on any register write)

  bool latch; // Controls which byte to write to in PPUADDR and PPUSCROLL
              // 0 = write to hi, 1 = write to lo


  struct { // Registers
    // ------ Internal Registers ------- //

    u8 ppudata_read_buffer;

    // ---- Memory Mapped Registers ---- //

    union {     // PPUCTRL   - 0x2000 - PPU control register
      u8 raw;
      BitField<7> V; // NMI enable
      BitField<6> P; // PPU master/slave
      BitField<5> H; // sprite height
      BitField<4> B; // background tile select
      BitField<3> S; // sprite tile select
      BitField<2> I; // increment mode
      BitField<0, 2> N; // nametable select
    } ppuctrl;

    union {     // PPUMASK   - 0x2001 - PPU mask register
      u8 raw;
      BitField<7> B; // color emphasis Blue
      BitField<6> G; // color emphasis Green
      BitField<5> R; // color emphasis Red
      BitField<4> s; // sprite enable
      BitField<3> b; // background enable
      BitField<2> M; // sprite left column enable
      BitField<1> m; // background left column enable
      BitField<0> g; // greyscale
    } ppumask;

    union {     // PPUSTATUS - 0x2002 - PPU status register
      u8 raw;
      BitField<7> V; // vblank
      BitField<6> S; // sprite 0 hit
      BitField<5> O; // sprite overflow
      // the rest are irrelevant
    } ppustatus;

    u8 oamaddr; // OAMADDR   - 0x2003 - OAM address port
    u8 oamdata; // OAMDATA   - 0x2004 - OAM data port

    union {     // PPUSCROLL - 0x2005 - PPU scrolling position register
      u16 val;
      BitField<8, 8> x;
      BitField<0, 8> y;
    } ppuscroll;

    union {     // PPUADDR   - 0x2006 - PPU VRAM address register
      u16 val;            // 16 bit address
      BitField<8, 8> hi;  // hi byte of addr
      BitField<0, 8> lo;  // lo byte of addr
    } ppuaddr;

    u8 ppudata; // PPUDATA   - 0x2007 - PPU VRAM data port
  } reg;

  // What about OAMDMA - 0x4014 - PPU DMA register?
  //
  // Well, it's not a register... hell, it's not even located on the PPU!
  //
  // DMA is handled by an external unit (this->dma), an object that has
  // references to both CPU WRAM and PPU OAM.
  // To make the emulator code simpler, the PPU object handles the 0x4014 call,
  // but all that actually happens is that it calls this->dma.write(...), and
  // cycles itself for the 513/514 cycles that it takes to do this

  /*----------  Emulation Vars  ----------*/

  uint cycles;

  // framebuffer
  u8 frame [240 * 256 * 4];

  // current pixel
  struct {
    uint x;
    uint y;
  } scan;

  /*---------------  Debug  --------------*/

#ifdef DEBUG_PPU
  void   init_debug_windows();
  void update_debug_windows();
#endif // DEBUG_PPU

public:
  struct Color {
    u8 r;
    u8 g;
    u8 b;

    Color(u8 r, u8 g, u8 b);
    Color(u32 color);

    inline operator u32() const {
      return (u32(this->r) << 16)
           | (u32(this->g) << 8)
           | (u32(this->b) << 0);
    }
  };

  static Color palette [64]; // NES color palette (static, for now)

public:
  ~PPU();
  PPU(Memory& mem, Memory& oam, DMA& dma, InterruptLines& interrupts);

  // <Memory>
  u8 read(u16 addr) override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void power_cycle();
  void reset();

  void cycle();

  const u8* getFrame() const;
};

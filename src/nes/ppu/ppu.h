#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"

#include "color.h"
#include "dma.h"
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

// Picture Processing Unit
// This guy is NOT cycle-accurate at the moment!
// http://wiki.nesdev.com/w/index.php/PPU_programmer_reference
class PPU final : public Memory {
private:

  /*----------  "Hardware"  ----------*/
  // In quotes because technically, these things aren't located on the PPU, but
  // by coupling them with the PPU, it makes the emulator code cleaner

  // CPU WRAM -> PPU OAM Direct Memory Access (DMA) Unit
  DMA& dma;

  /*-----------  Hardware  -----------*/

  // ---- Core Hardware ---- //

  InterruptLines& interrupts;
  Memory& mem; // PPU 16 bit address space (should be wired to ppu_mmu)

  // ---- Sprite Hardware ---- //

  Memory& oam;  // Primary OAM - 256 bytes (Object Attribute Memory)
  Memory& oam2; // Secondary OAM - 32 bytes (8 sprites to render on scanline)

  // ---- Background Hardware ---- //

  // TBD

  // ---- Memory Mapped Registers ---- //

  u8 cpu_data_bus; // PPU <-> CPU data bus (filled on any register write)

  bool latch; // Controls which byte to write to in PPUADDR and PPUSCROLL
              // 0 = write to hi, 1 = write to lo

  struct { // Registers
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

      // https://wiki.nesdev.com/w/index.php/PPU_scrolling
      BitField<0,  5> x_scroll;
      BitField<5,  5> y_scroll;
      BitField<10, 2> nametable;
      BitField<12, 3> y_scroll_fine;
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
  // but all that actually happens is that it calls this->dma.transfer(), pushes
  // that data to OAMDATA, and cycles itself for the requisite number of cycles

  /*---- Helper Functions ----*/

  void get_bgr_pixel(bool& has_spr, u8& color);
  void get_spr_pixel(bool& has_spr, u8& color, bool& priority);

  void sprite_eval();

  /*----  Emulation Vars and Methods  ----*/

  uint cycles; // total PPU cycles
  uint frames; // total frames rendered

  // framebuffer
  u8 framebuff [240 * 256 * 4];
  void draw_dot(Color color);

  // scanline tracker
  struct {
    uint line;  // 0 - 261
    uint cycle; // 0 - 340
  } scan;

  // State related to sprite evaluation
  struct {
    u8 oam2addr; // pointer into oam2
  } spr_eval;

  /*---------------  Debug  --------------*/

#ifdef DEBUG_PPU
  void   init_debug_windows();
  void update_debug_windows();
#endif // DEBUG_PPU

public:
  ~PPU();
  PPU(
    Memory& mem,
    Memory& oam,
    Memory& oam2,
    DMA& dma,
    InterruptLines& interrupts
  );

  // <Memory>
  u8 read(u16 addr) override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void power_cycle();
  void reset();

  void cycle();

  const u8* getFramebuff() const;
  uint getFrames() const;

  // NES color palette (static, for the time being)
  static const Color palette [64];
};

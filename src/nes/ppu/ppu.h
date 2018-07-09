#pragma once

#include "common/bitfield.h"
#include "common/serializable.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "color.h"
#include "dma.h"
#include "nes/generic/ram/ram.h"
#include "nes/wiring/interrupt_lines.h"

#include "nes/params.h"

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
// _Should_ be cycle-accurate
// http://wiki.nesdev.com/w/index.php/PPU_programmer_reference
class PPU final : public Memory, public Serializable {
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

  RAM oam;  // Primary OAM - 256 bytes (Object Attribute Memory)
  RAM oam2; // Secondary OAM - 32 bytes (8 sprites to render on scanline)

  struct {
    struct {
      u8 lo;
      u8 hi;
    } sliver [8]; // 8 pairs of 8-bit shift registers (for bitmaps)
    u8 at    [8]; // Attribute byte (per sprite)
    u8 xpos  [8]; // x-position of sprites

    bool spr_zero_on_line; // temp, not in hardware (just a workaround for now)
  } spr;

  // ---- Background Hardware ---- //

  struct {
    // Temporary Registers
    u8 nt_byte;
    u8 at_byte;
    u8 tile_lo;
    u8 tile_hi;
    // Shift Registers
    struct {
      u8   at       [2];
      bool at_latch [2];
      u16  tile     [2];
    } shift;
  } bgr;

  // ---- Misc Hardware ---- //

  u8 cpu_data_bus; // PPU <-> CPU data bus (filled on any register write)

  bool odd_frame_latch;
  bool latch; // Controls which byte to write to in PPUADDR and PPUSCROLL
              // 0 = write to hi, 1 = write to lo

  // ---- Memory Mapped Registers ---- //

  struct { // Registers
    // PPUCTRL - 0x2000 - PPU control register
    union {
      u8 raw;
      BitField<7> V; // NMI enable
      BitField<6> P; // PPU master/slave
      BitField<5> H; // sprite height
      BitField<4> B; // background tile select
      BitField<3> S; // sprite tile select (ignored in 8x16 sprite mode)
      BitField<2> I; // increment mode
      BitField<0, 2> N; // nametable select
    } ppuctrl;

    // PPUMASK - 0x2001 - PPU mask register
    union {
      u8 raw;
      BitField<7> B; // color emphasis Blue
      BitField<6> G; // color emphasis Green
      BitField<5> R; // color emphasis Red
      BitField<4> s; // sprite enable
      BitField<3> b; // background enable
      BitField<2> M; // sprite left column enable
      BitField<1> m; // background left column enable
      BitField<0> g; // greyscale

      BitField<3, 2> is_rendering;
    } ppumask;

    // PPUSTATUS - 0x2002 - PPU status register
    union {
      u8 raw;
      BitField<7> V; // vblank
      BitField<6> S; // sprite 0 hit
      BitField<5> O; // sprite overflow
      // the rest are irrelevant
    } ppustatus;

    u8 oamaddr; // OAMADDR - 0x2003 - OAM address port
    u8 oamdata; // OAMDATA - 0x2004 - OAM data port

    // PPUSCROLL - 0x2005 - PPU scrolling position register
    // PPUADDR   - 0x2006 - PPU VRAM address register
    //     Note: Both 0x2005 and 0x2006 map to the same internal register, v
    //     0x2005 is simply a convenience used to set scroll more easily!

    u8 ppudata; // PPUDATA - 0x2007 - PPU VRAM data port

    union {
      u16 val;            // the actual address
      BitField<8, 8> hi;  // hi byte of addr
      BitField<0, 8> lo;  // lo byte of addr

      // https://wiki.nesdev.com/w/index.php/PPU_scrolling
      BitField<0,  5> coarse_x;
      BitField<5,  5> coarse_y;
      BitField<10, 2> nametable;
      BitField<12, 3> fine_y;
    } v, t;

    // v is the true vram address
    // t is the temp vram address

    union {
      u8 _x_val;
      BitField<0, 3> x; // fine x-scroll register
    };
  } reg;

  // What about OAMDMA - 0x4014 - PPU DMA register?
  //
  // Well, it's not a register... hell, it's not even located on the PPU!
  //
  // DMA is handled by an external unit (this->dma), an object that has
  // references to both CPU WRAM and PPU OAM.
  //
  // To make the emulator code simpler, the PPU object handles the 0x4014 call,
  // but all that actually happens is that it calls this->dma.transfer(), pushes
  // that data to OAMDATA, and cycles itself for the requisite number of cycles

  /*----------- PPU Functions -----------*/

  struct Pixel {
    bool is_on;
    u8   palette;  // pixel color - by palette
    bool priority; // sprite priority bit (always 0 for bgr pixels)
  };

  Pixel get_bgr_pixel();
  Pixel get_spr_pixel(Pixel& bgr_pixel);

  void bgr_fetch();
  void spr_fetch();

  /*----  Emulation Vars and Methods  ----*/

  // fogleman NMI hack - gets some games to boot (eg: Bad Dudes)
  int nmi_delay;
  int nmi_previous;
  void nmiChange();

  // framebuffers
  u8 framebuffer     [240 * 256 * 4] = {0};
  u8 framebuffer_spr [240 * 256 * 4] = {0};
  u8 framebuffer_bgr [240 * 256 * 4] = {0};

  // scanline tracker
  struct {
    uint line;  // 0 - 261
    uint cycle; // 0 - 340
  } scan;

  uint cycles; // total PPU cycles
  uint frames; // total frames rendered

  SERIALIZE_START(11, "PPU")
    SERIALIZE_SERIALIZABLE(oam)
    SERIALIZE_SERIALIZABLE(oam2)
    SERIALIZE_POD(spr)
    SERIALIZE_POD(bgr)
    SERIALIZE_POD(cpu_data_bus)
    SERIALIZE_POD(odd_frame_latch)
    SERIALIZE_POD(latch)
    SERIALIZE_POD(reg)
    SERIALIZE_POD(scan)
    SERIALIZE_POD(cycles)
    SERIALIZE_POD(frames)
  SERIALIZE_END(11)

  /*---------------  Hacks  --------------*/

  const bool& fogleman_nmi_hack;

#ifdef DEBUG_PPU
  // Implementation in debug.cc
  void   init_debug_windows();
  void update_debug_windows();
#endif // DEBUG_PPU

  /*---------------  Public  --------------*/

public:
  PPU() = delete;
  PPU(const NES_Params& params,
    Memory& mem,
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

  void getFramebuffSpr(const u8*& framebuffer) const;
  void getFramebuffBgr(const u8*& framebuffer) const;

  void getFramebuff(const u8*& framebuffer) const;
  uint getNumFrames() const;

  // NES color palette (static, for the time being)
  static const Color palette [64];

  /*---------------  wideNES  --------------*/

public:
  struct Scroll { u8 x; u8 y; };
private:
  Scroll last_scroll { 0, 0 };

  struct ZeroFilter {
    uint bias = 3;
    void set_bias(uint bias);
    u8 past_few [10] = {1};
    void add_val(u8 val);
    bool is_true_0();
  } zf_x, zf_y;
public:
  Scroll get_scroll() const;
};

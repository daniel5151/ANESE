#include "ppu.h"

#include <cassert>
#include <cstdio>

#define DEBUG_PPU

#ifdef DEBUG_PPU
  #include <SDL.h>
  #include "common/debug.h"
  static DebugPixelbuffWindow* patt_t;
#endif

PPU::~PPU() {}

PPU::PPU(Memory& mem, Memory& oam, DMA& dma)
: dma(dma),
  mem(mem),
  oam(oam)
{
  for (uint i = 0; i < 256 * 240 * 4; i++)
    this->frame[i] = 0;

  this->scan.x = 0;
  this->scan.y = 0;

  this->power_cycle();

#ifdef DEBUG_PPU
  patt_t = new DebugPixelbuffWindow(
    "Pattern Table",
    0x80 * 2 + 16, 0x80,
    32, 32
  );
#endif
}

void PPU::power_cycle() {
  this->cycles = 0;

  this->cpu_data_bus = 0x00;

  // http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  this->reg.ppuctrl.raw = 0x00;
  this->reg.ppumask.raw = 0x00;

  this->reg.ppustatus.V = 1; // "often" set
  this->reg.ppustatus.S = 0;
  this->reg.ppustatus.O = 1; // "often" set

  this->reg.oamaddr = 0x00;
  this->reg.oamdata = 0x00; // (just a guess)

  this->latch = 0;
  this->reg.ppuscroll.val = 0x0000;
  this->reg.ppuaddr.val = 0x0000;

  this->reg.ppudata = 0x00;
  this->reg.ppudata_read_buffer = 0x00; // (just a guess)
}

void PPU::reset() {
  this->cycles = 0;

  // http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  this->reg.ppuctrl.raw = 0x00;
  this->reg.ppumask.raw = 0x00;

  // this->reg.ppustatus is unchanged

  // this->reg.oamaddr   is unchanged
  // this->reg.oamdata   is unchanged (just a guess)

  this->latch = 0;
  this->reg.ppuscroll.val = 0x0000;
  // this->reg.ppuaddr.val is unchanged

  this->reg.ppudata = 0x00;
  this->reg.ppudata_read_buffer = 0x00; // (just a guess)
}

u8 PPU::read(u16 addr) {
  assert((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014);

  using namespace PPURegisters;

  u8 retval;

  switch(addr) {
  case PPUSTATUS: { // Bottom 5 bits are the values happen to be present
                    // on the cpu data line
                    retval = (this->reg.ppustatus.raw & 0xE0)
                           | (this->cpu_data_bus      & 0x1F);
                    this->reg.ppustatus.V = false;
                    this->latch = false;
                  } break;
  case OAMDATA:   { retval = this->oam[this->reg.oamaddr++];
                  } break;
  case PPUDATA:   { // PPUDATA read buffer (post-fetch) logic
                    if (this->reg.ppuaddr.val <= 0x3EFF) {
                      // Reading Nametables
                      // retval = from internal buffer
                      retval = this->reg.ppudata_read_buffer;
                      // Fill read buffer with acutal data
                      u8 val = this->mem[this->reg.ppuaddr.val % 0x4000];
                      this->reg.ppudata_read_buffer = val;
                    } else {
                      // Reading Pallete
                      // retval = directly from memory
                      retval = this->mem[this->reg.ppuaddr.val % 0x4000];
                      // Fill read buffer with the mirrored nametable data
                      // Why? Because the wiki said so!
                      u8 val = this->mem[this->reg.ppuaddr.val % 0x2000];
                      this->reg.ppudata_read_buffer = val;
                    }

                    // (0: add 1, going across; 1: add 32, going down)
                    if (this->reg.ppuctrl.I == 0) this->reg.ppuaddr.val += 1;
                    if (this->reg.ppuctrl.I == 1) this->reg.ppuaddr.val += 32;
                  } break;
  case OAMDMA:    { // This is not a valid operation...
                    // And it's not like this would return the cpu_data_bus val
                    // So, uh, screw it, just return 0 I guess?
                    fprintf(stderr, "[DMA] Reading DMA is undefined!\n");
                    retval = 0x00;
                  } break;
  default:        { retval = this->cpu_data_bus;
                    fprintf(stderr,
                      "[PPU] Reading from a Write-Only register: 0x%04X\n",
                      addr
                    );
                  } break;
  }

  return retval;
}

// Should match PPU::read, just without any side-effects
u8 PPU::peek(u16 addr) const {
  assert((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014);

  using namespace PPURegisters;

  u8 retval;

  switch(addr) {
  case PPUSTATUS: { retval = (this->reg.ppustatus.raw & 0xE0)
                           | (this->cpu_data_bus      & 0x1F);
                  } break;
  case OAMDATA:   { retval = this->oam.peek(this->reg.oamaddr);
                  } break;
  case PPUDATA:   { if (this->reg.ppuaddr.val <= 0x3EFF) {
                      retval = this->reg.ppudata_read_buffer;
                    } else {
                      retval = this->mem.peek(this->reg.ppuaddr.val % 0x4000);
                    }
                  } break;
  case OAMDMA:    { // This is not a valid operation...
                    // And it's not like this would return the cpu_data_bus val
                    // So, uh, screw it, just return 0 I guess?
                    fprintf(stderr, "[DMA] Peeking DMA is undefined!\n");
                    retval = 0x00;
                  } break;
  default:        { retval = this->cpu_data_bus;
                    fprintf(stderr,
                      "[PPU] Peeking from a Write-Only register: 0x%04X\n",
                      addr
                    );
                  } break;
  }

  return retval;
}

void PPU::write(u16 addr, u8 val) {
  assert((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014);

  using namespace PPURegisters;

  this->cpu_data_bus = val; // fill up data bus

  // According to http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  // Writes to these registers are ignored if done earlier than ~29658 CPU
  // cycles after reset...
  if (this->cycles < 29658 * 3) {
    switch(addr) {
    case PPUCTRL:   return;
    case PPUMASK:   return;
    case PPUSCROLL: return;
    case PPUADDR:   return;
    default: break;
    }
  }

  switch(addr) {
  case PPUCTRL:   { this->reg.ppuctrl.raw = val;
                    // SHOULD CAUSE NMI IF SET WHILE PPU IS IN VBLANK!
                  } return;
  case PPUMASK:   { this->reg.ppumask.raw = val;
                  } return;
  case OAMADDR:   { this->reg.oamaddr = val;
                  } return;
  case OAMDATA:   { this->oam[this->reg.oamaddr++] = val;
                  } return;
  case PPUSCROLL: { if (this->latch == 0) this->reg.ppuscroll.x = val;
                    if (this->latch == 1) this->reg.ppuscroll.y = val;
                    this->latch = !this->latch;
                  } return;
  case PPUADDR:   { if (this->latch == 0) this->reg.ppuaddr.hi = val;
                    if (this->latch == 1) this->reg.ppuaddr.lo = val;
                    this->latch = !this->latch;
                  } return;
  case PPUDATA:   { u16 true_addr = this->reg.ppuaddr.val % 0x4000;
                    this->mem[true_addr] = val;
                    // (0: add 1, going across; 1: add 32, going down)
                    bool mode = this->reg.ppuctrl.I;
                    if (mode == 0) this->reg.ppuaddr.val += 1;
                    if (mode == 1) this->reg.ppuaddr.val += 32;
                  } return;
  case OAMDMA:    { // The CPU is paused during DMA, but the PPU is not!
                    // DMA takes 513 / 514 CPU cycles:
                    // 1 dummy cycle
                    this->cycle();
                    // +1 cycle if starting on odd CPU cycle
                    if ((this->cycles / 3) % 2)
                      this->cycle();
                    // 512 cycles of reading & writing
                    this->dma.start(val);
                    while (this->dma.isActive()) {
                      this->dma.transfer();
                      this->cycle();
                    }
                  } return;
  default:        { fprintf(stderr,
                      "[PPU] Writing to a Read-Only register: 0x%04X\n <- 0x%02X",
                      addr,
                      val
                    );
                  } return;
  }
}

// Just dicking around rn
#include <cmath>

void PPU::cycle() {
#ifdef DEBUG_PPU
  if (this->cycles % 200000 == 0) {
    // Don't update this info every frame, that's sooper pooper slow

    // Left pattern table
    for (uint row = 0; row < 16; row++) {
      for (uint col = 0; col < 16; col++) {
        for (uint y = 0; y < 8; y++) {
          u16 addr = (row << 8) + (col << 4) + y;
          u8 lo_bp = this->mem[addr + 0];
          u8 hi_bp = this->mem[addr + 8];
          for (uint x = 0; x < 8; x++) {
            u8 pixel_type = nth_bit(lo_bp, x) + 2 * nth_bit(hi_bp, x);
            patt_t->set_pixel(
              col * 8 + (7 - x),
              row * 8 + y,
              pixel_type * (256 / 4),
              pixel_type * (256 / 4),
              pixel_type * (256 / 4),
              255
            );
          }
        }
      }
    }

    // Right pattern table
    for (uint row = 0; row < 16; row++) {
      for (uint col = 0; col < 16; col++) {
        for (uint y = 0; y < 8; y++) {
          u16 addr = (row << 8) + (col << 4) + y;

          addr += 0x1000;

          u8 lo_bp = this->mem[addr + 0];
          u8 hi_bp = this->mem[addr + 8];
          for (uint x = 0; x < 8; x++) {
            u8 pixel_type = nth_bit(lo_bp, x) + 2 * nth_bit(hi_bp, x);
            patt_t->set_pixel(
              col * 8 + (7 - x) + 0x90, // <--- draw to the right
              row * 8 + y,
              pixel_type * (256 / 4),
              pixel_type * (256 / 4),
              pixel_type * (256 / 4),
              255
            );
          }
        }
      }
    }

    patt_t->render();
  }
#endif
  this->cycles += 1;

  const u32 offset = (256 * 4 * this->scan.y) + this->scan.x * 4;
  /* b */ this->frame[offset + 0] = (sin(this->cycles / 10.0) + 1) * 125;
  /* g */ this->frame[offset + 1] = (this->scan.y / double(240)) * 255;
  /* r */ this->frame[offset + 2] = (this->scan.x / double(256)) * 255;
  /* a */ this->frame[offset + 3] = 255;

  // Always increment x
  this->scan.x += 1;
  this->scan.x %= 256;

  // Only increment y when x is back at 0
  this->scan.y += (this->scan.x == 0 ? 1 : 0);
  this->scan.y %= 240;
}

const u8* PPU::getFrame() const { return this->frame; }

/*----------------------------------------------------------------------------*/

PPU::Color::Color(u8 r, u8 g, u8 b) {
  this->r = r;
  this->g = g;
  this->b = b;
}

PPU::Color::Color(u32 color) {
  this->r = color & 0xFF0000 >> 16;
  this->g = color & 0x00FF00 >> 8;
  this->b = color & 0x0000FF >> 0;
}

PPU::Color PPU::palette [64] = {
  0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
  0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000,
  0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00,
  0x6B6D00, 0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000, 0x000000, 0x000000,
  0xFFFEFF, 0x64B0FF, 0x9290FF, 0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
  0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000,
  0xFFFEFF, 0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
  0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000,
};

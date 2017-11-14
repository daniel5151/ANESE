#include "ppu.h"

#include <cassert>
#include <cstdio>

PPU::~PPU() {}

PPU::PPU(Memory& mem, Memory& oam, DMA& dma, InterruptLines& interrupts)
: dma(dma),
  interrupts(interrupts),
  mem(mem),
  oam(oam)
{
  for (uint i = 0; i < 256 * 240 * 4; i++)
    this->framebuff[i] = 0;

  this->dot.x = 0;
  this->dot.y = 261; // start on pre-render scanline

  this->power_cycle();

#ifdef DEBUG_PPU
  this->init_debug_windows();
#endif
}

void PPU::power_cycle() {
  this->cycles = 0;

  this->cpu_data_bus = 0x00;

  this->latch = 0;

  // http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  this->reg.ppuctrl.raw = 0x00;
  this->reg.ppumask.raw = 0x00;

  this->reg.ppustatus.V = 1; // "often" set
  this->reg.ppustatus.S = 0;
  this->reg.ppustatus.O = 1; // "often" set

  this->reg.oamaddr = 0x00;
  this->reg.oamdata = 0x00; // (just a guess)

  this->reg.ppuscroll.val = 0x0000;
  this->reg.ppuaddr.val = 0x0000;

  this->reg.ppudata = 0x00; // (just a guess)
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

  this->reg.ppudata = 0x00; // (just a guess)
}

/*--------------------------  Framebuffer Methods  ---------------------------*/

uint PPU::getFrames() const { return this->frames; }

const u8* PPU::getFramebuff() const { return this->framebuff; }

void PPU::draw_dot(Color color) {
  ((u32*)this->framebuff)[(256 * this->dot.y) + this->dot.x] = color;
}

/*----------------------------  Memory Interface  ----------------------------*/

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
                      retval = this->reg.ppudata;
                      // Fill read buffer with acutal data
                      u8 val = this->mem[this->reg.ppuaddr.val];
                      this->reg.ppudata = val;
                    } else {
                      // Reading Pallete
                      // retval = directly from memory
                      retval = this->mem[this->reg.ppuaddr.val];
                      // Fill read buffer with the mirrored nametable data
                      // Why? Because the wiki said so!
                      u8 val = this->mem[this->reg.ppuaddr.val % 0x2000];
                      this->reg.ppudata = val;
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
                      "[PPU] Reading from Write-Only register: 0x%04X\n",
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
                      retval = this->reg.ppudata;
                    } else {
                      retval = this->mem.peek(this->reg.ppuaddr.val);
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
                      "[PPU] Peeking from Write-Only register: 0x%04X\n",
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
                    if (this->reg.ppustatus.V)
                      this->interrupts.request(Interrupts::Type::NMI);
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
  case PPUDATA:   { this->mem[this->reg.ppuaddr.val] = val;
                    // (0: add 1, going across; 1: add 32, going down)
                    if (this->reg.ppuctrl.I == 0) this->reg.ppuaddr.val += 1;
                    if (this->reg.ppuctrl.I == 1) this->reg.ppuaddr.val += 32;
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
                      this->mem[OAMDATA] = this->dma.transfer();
                      this->cycle();
                      this->cycle();
                    }
                  } return;
  default:        { fprintf(stderr,
                      "[PPU] Writing to Read-Only register: 0x%04X\n <- 0x%02X",
                      addr,
                      val
                    );
                  } return;
  }
}

/*----------------------------  Core Render Loop  ----------------------------*/

void PPU::cycle() {
  this->cycles += 1;

#ifdef DEBUG_PPU
  this->update_debug_windows();
#endif

  const bool start_of_line = (this->cycles % 342 == 0);

  if (start_of_line) {
    // Update current dot position
    this->dot.y += 1;
    this->dot.x = 0;

    if (this->dot.y > 261) {
      this->dot.y = 0;
      this->frames += 1;
    }
  }

  // Check if there is anything to render
  // (i.e: the show sprites flag is on, or show backgrounds flag is on)
  if (this->reg.ppumask.s || this->reg.ppumask.b) {

    // this is just me messing around
    if (this->dot.y < 240) {
      this->draw_dot(
        this->palette[
          (this->cycles / (0x3FFF) + this->dot.x + this->dot.y) / 128 % 64
        ]
      );

      this->dot.x += 1;
      this->dot.x %= 256;
    }
  }

  // Enable / Disable vblank
  if (start_of_line) {
    // vblank start on line 241...
    if (this->dot.y == 241 && this->reg.ppuctrl.V) {
      this->reg.ppustatus.V = true;
      this->interrupts.request(Interrupts::NMI);
    }

    // ...and ends after line 260
    if (this->dot.y == 261) {
      this->reg.ppustatus.V = false;
    }
  }
}

/*-------------------------  Palette + Color Struct  -------------------------*/

Color::Color(u8 r, u8 g, u8 b) {
  this->color.r = r;
  this->color.g = g;
  this->color.b = b;
  this->color.a = 0xFF;
}

Color::Color(u32 rgb) {
  this->color.r = (rgb & 0xFF0000) >> 16;
  this->color.g = (rgb & 0x00FF00) >> 8;
  this->color.b = (rgb & 0x0000FF) >> 0;
  this->color.a = 0xFF;
}

Color::operator u32() const  {
  return this->color.val;
}

Color PPU::palette [64] = {
  0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
  0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000,
  0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00,
  0x6B6D00, 0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000, 0x000000, 0x000000,
  0xFFFEFF, 0x64B0FF, 0x9290FF, 0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
  0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000,
  0xFFFEFF, 0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
  0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000,
};

/*=====================================
=            DEBUG WINDOWS            =
=====================================*/

#ifdef DEBUG_PPU

#include <SDL.h>
#include "common/debug.h"

constexpr uint UPDATE_EVERY_X_FRAMES = 10;

static_assert(UPDATE_EVERY_X_FRAMES > 1, "causes badness. pls no do.");

static DebugPixelbuffWindow* patt_t;
static DebugPixelbuffWindow* palette_t;
static DebugPixelbuffWindow* nes_palette;
static DebugPixelbuffWindow* name_t;

void PPU::init_debug_windows() {
  // Pattern Table
  patt_t = new DebugPixelbuffWindow(
    "Pattern Table",
    (0x80 * 2 + 16) * 2, (0x80) * 2,
     0x80 * 2 + 16,       0x80,
    0, 32
  );

  // Palette Table
  palette_t = new DebugPixelbuffWindow(
    "Palette Table",
    (8 + 1) * 20, (4) * 20,
     8 + 1,        4,
    0x110 * 2, 4*16*2
  );

  // the static NES palette
  nes_palette = new DebugPixelbuffWindow(
    "Static NES Palette",
    (16) * 16, (4) * 16,
     16,        4,
    0x110 * 2, 0
  );

  // nametables
  name_t = new DebugPixelbuffWindow(
    "Nametables",
    (256 * 2 + 16), (240 * 2 + 16),
     256 * 2 + 16,   240 * 2 + 16,
    0, 324
  );
}

void PPU::update_debug_windows() {
  if (this->cycles < 10) {
    // load in a debug palette
    this->mem[0x3F00 + 0] = 0x1D;
    this->mem[0x3F00 + 1] = 0x2D;
    this->mem[0x3F00 + 2] = 0x3D;
    this->mem[0x3F00 + 3] = 0x30;
  }

  // i'm making it update every second right now
  static bool should_update = true;
  if (
    should_update == false &&
    this->frames % UPDATE_EVERY_X_FRAMES == UPDATE_EVERY_X_FRAMES - 1
  ) should_update = true;

  if (should_update && this->frames % UPDATE_EVERY_X_FRAMES == 0) {
    should_update = false;

    auto paint_tile = [=](
      u16 tile_addr,
      uint tl_x, uint tl_y,
      uint palette, // from 0 - 4
      DebugPixelbuffWindow* window
    ) {
      for (uint y = 0; y < 8; y++) {
        u8 lo_bp = this->mem.peek(tile_addr + y + 0);
        u8 hi_bp = this->mem.peek(tile_addr + y + 8);

        for (uint x = 0; x < 8; x++) {
          u8 pixel_type = nth_bit(lo_bp, x) + 2 * nth_bit(hi_bp, x);

          Color color = this->palette[
            this->mem.peek(0x3F00 + palette * 4 + pixel_type)
          ];

          window->set_pixel(
            tl_x + (7 - x),
            tl_y + y,

            color
          );
        }
      }
    };

    // Pattern Tables
    // There are two sets of 256 8x8 pixel tiles
    // Every 16 bytes represents a single 8x8 pixel tile
    for (uint addr = 0x0000; addr < 0x2000; addr += 16) {
      const uint tl_x = ((addr % 0x1000) % 256) / 2
                      + ((addr >= 0x1000) ? 0x90 : 0);
      const uint tl_y = ((addr % 0x1000) / 256) * 8;

      paint_tile(addr, tl_x, tl_y, 0, patt_t);
    }

    patt_t->render();

    // Nametables
    auto paint_nametable = [=](
      u16 base_addr,
      uint offset_x, uint offset_y
    ){
      for (uint addr = base_addr; addr < base_addr + 0x400 - 64; addr++) {
        // Getting which tile to render is easy...

        u16 tile_addr = (this->reg.ppuctrl.B * 0x1000) // bg palette selector
                      + this->mem.peek(addr) * 16;

        // ...The hard part is figuring out the palette for it :)
        // http://wiki.nesdev.com/w/index.php/PPU_attribute_tables

        // What 8x8 tile are we on?
        // There are 32 tiles per row, and a total of 30 rows
        const uint tile_no = addr - base_addr;
        const uint tile_row = tile_no % 32;
        const uint tile_col = tile_no / 32;

        // Supertiles are 32x32 collections of pixels, where each 16x16 corner
        // of the supertile (4 8x8 tiles) can be assigned a palette
        const uint supertile_no = tile_row / 4
                                + tile_col / 4 * 8;

        // Nice! Now we can pull data from the attribute table!
        // Now, to descifer which of the 4 palettes to use...
        const u8 attribute = this->mem[base_addr + 0x3C0 + supertile_no];

        // What corner is this particular 8x8 tile in?
        //
        // top left = 0
        // top right = 1
        // bottom left = 2
        // bottom right = 3
        const uint corner = (tile_row % 4) / 2
                          + (tile_col % 4) / 2 * 2;

        // Recall that the attribute byte stores the palette assignment data
        // formatted as follows:
        //
        // attribute = (topleft << 0)
        //           | (topright << 2)
        //           | (bottomleft << 4)
        //           | (bottomright << 6)
        //
        // thus, we can reverse the process with some clever bitmasking!
        // 0x03 == 0b00000011
        const uint mask = 0x03 << (corner * 2);
        const uint palette = (attribute & mask) >> (corner * 2);

        // And with that, we can go ahead and render it!

        // 32 tiles per row, each is 8x8 pixels
        const uint tl_x = (tile_no % 32) * 8 + offset_x;
        const uint tl_y = (tile_no / 32) * 8 + offset_y;

        paint_tile(tile_addr, tl_x, tl_y, palette, name_t);
      }
    };

    paint_nametable(0x2000, 0,        0       );
    paint_nametable(0x2400, 256 + 16, 0       );
    paint_nametable(0x2800, 0,        240 + 16);
    paint_nametable(0x2C00, 256 + 16, 240 + 16);

    name_t->render();

    // nes palette
    for (uint i = 0; i < 64; i++) {
      nes_palette->set_pixel(i % 16, i / 16, this->palette[i]);
    }

    nes_palette->render();

    // Palette Tables
    // Background palette - from 0x3F00 to 0x3F0F
    // Sprite palette     - from 0x3F10 to 0x3F1F
    for (u16 addr = 0x3F00; addr < 0x3F20; addr++) {
      Color color = this->palette[this->mem.peek(addr)];
      // printf("0x%04X\n", this->mem.peek(addr));
      palette_t->set_pixel(
        (addr % 4) + ((addr >= 0x3F10) ? 5 : 0),
        (addr - ((addr >= 0x3F10) ? 0x3F10 : 0x3F00)) / 4,
        color
      );
    }

    palette_t->render();
  }
}

#endif // DEBUG_PPU

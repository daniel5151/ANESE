#include "ppu.h"

#include <cassert>
#include <cstdio>

PPU::~PPU() {}

PPU::PPU(
  Memory& mem,
  Memory& oam,
  Memory& oam2,
  DMA& dma,
  InterruptLines& interrupts
) :
  dma(dma),
  interrupts(interrupts),
  mem(mem),
  oam(oam),
  oam2(oam2)
{
  this->spr_eval = {0};

  for (uint i = 0; i < 256 * 240 * 4; i++)
    this->framebuff[i] = 0;

  this->scan.line = 261; // start on pre-render scanline
  this->scan.cycle = 0;

  this->power_cycle();

#ifdef DEBUG_PPU
  this->init_debug_windows();
#endif
}

void PPU::power_cycle() {
  this->cycles = 0;
  this->frames = 0;

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
  this->frames = 0;

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
  if (color.a == 0) return;

  if (this->scan.line >= 240 || !in_range(this->scan.cycle, 0, 255)) {
    // these pixels are not actually rendered...
    return;
  }

  u32* pixels = reinterpret_cast<u32*>(this->framebuff);
  pixels[(256 * this->scan.line) + this->scan.cycle] = color;
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
  case OAMDATA:   { // Extra logic for handling reads during sprite evaluation
                    // https://wiki.nesdev.com/w/index.php/PPU_sprite_evaluation
                    bool on_visible_line = this->scan.line < 240;
                    bool is_clearing_oam2 = in_range(this->scan.cycle, 1, 64);
                    if (on_visible_line && is_clearing_oam2) {
                      retval = 0xFF;
                    } else {
                      retval = this->oam[this->reg.oamaddr++];
                    }
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
  case OAMDATA:   { // Extra logic for handling reads during sprite evaluation
                    // https://wiki.nesdev.com/w/index.php/PPU_sprite_evaluation
                    bool on_visible_line = this->scan.line < 240;
                    bool is_clearing_oam2 = in_range(this->scan.cycle, 1, 64);
                    if (on_visible_line && is_clearing_oam2) {
                      retval = 0xFF;
                    } else {
                      retval = this->oam.peek(this->reg.oamaddr);
                    }
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
  //
  // That said though, it seems to be causing some issues right now, so i'm
  // commenting it out for the time being.
  //
  // if (this->cycles < 29658 * 3) {
  //   switch(addr) {
  //   case PPUCTRL:   return;
  //   case PPUMASK:   return;
  //   case PPUSCROLL: return;
  //   case PPUADDR:   return;
  //   default: break;
  //   }
  // }

  switch(addr) {
  case PPUCTRL:   { this->reg.ppuctrl.raw = val;
                    if (this->reg.ppuctrl.V && this->reg.ppustatus.V)
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

/*----------------------------  Helper Functions  ----------------------------*/

// TODO: Make sure this thing actually works!
//
// Implemented verbaitem as documented on nesdev wiki
// https://wiki.nesdev.com/w/index.php/PPU_sprite_evaluation
void PPU::sprite_eval() {
  // Cycles 1-64: Secondary OAM is initialized to $FF
  // NOTE: attempting to read $2004 (OAMDATA) will return $FF
  if (in_range(this->scan.cycle, 1, 64)) {
    // reset relevant state
    if (this->scan.cycle == 1) {
      this->spr_eval.oam2addr = 0;
    }

    // Each write takes 2 cycles, so simulate this by only writing on every
    // other cycle
    if ((this->scan.cycle - 1) % 2 == 0)
      this->oam2[this->spr_eval.oam2addr++] = 0xFF;

    return;
  } else

  // Cycles 65-256: Sprite evaluation
  if (in_range(this->scan.cycle, 65, 256)) {
    const uint sprite_height = this->reg.ppuctrl.H ? 16 : 8;

    // This implementation isn't actually cycle-accurate, since it just does all
    // the calculations on cycle 65.
    // Ideally, c++ would have co-routines and yeild functionality, since that
    // would make this a whole hell of a lot easier, but ah well...
    //
    // Maybe i'll get around to implementing this properly later, but for now,
    // i'm just leaving it innaccurate

    if (this->scan.cycle != 65) return;

    // reset state
    this->reg.oamaddr = 0;
    this->spr_eval.oam2addr = 0;
    bool disable_oam2_write = false;

    u8   y_coord;
    bool y_in_range;
    // I use this->reg.oamaddr as the `n` var as described on nesdev

    // Okay, I know that gotos are generally a bad idea.
    // But in this case, holy cow, to they help a ton!
    // The alternative is adding a whole mess of helper functions that clutter
    // things up, and IMHO, make it complicated to see control flow.

  step1:
    // 1.
    // Starting at n = 0, read a sprite's Y-coordinate (OAM[n][0]),
    // copying it to the next open slot in secondary OAM
    y_coord = this->oam[this->reg.oamaddr];

    if (!disable_oam2_write)
      this->oam2[this->spr_eval.oam2addr] = y_coord;

    // 1a.
    // If Y-coordinate is in range, copy remaining bytes of sprite data
    // (OAM[n][1] thru OAM[n][3]) into secondary OAM.
    y_in_range = in_range(
      y_coord,
      this->scan.line + 1 - (sprite_height - 1),
      this->scan.line + 1
    );

    if (y_in_range && !disable_oam2_write) {
      this->spr_eval.oam2addr++;
      this->reg.oamaddr++;

      this->oam2[this->spr_eval.oam2addr++] = this->oam[this->reg.oamaddr++];
      this->oam2[this->spr_eval.oam2addr++] = this->oam[this->reg.oamaddr++];
      this->oam2[this->spr_eval.oam2addr++] = this->oam[this->reg.oamaddr++];
    } else {
      this->reg.oamaddr += 4;
    }

    // 2a. if n has overflowed back to 0 (i.e: all 64 sprites eval'd), go to 4
    if (this->reg.oamaddr == 0) goto step4;
    // 2b. if less than 8 sprites have been found, go to 1
    if (this->spr_eval.oam2addr < 32) goto step1;
    // 2c. If exactly 8 sprites have been found, disable writes to secondary OAM
    //     This causes sprites in back to drop out.
    if (this->spr_eval.oam2addr >= 32) disable_oam2_write = true;

  step3:
    // 3.
    // Starting at Starting at m = 0, evaluate OAM[n][m] as a Y-coordinate.
    y_coord = this->oam[this->reg.oamaddr];

    // 3a.
    // If the value is in range, set the sprite overflow flag in $2002 and
    // read the next 3 entries of OAM (incrementing 'm' after each byte and
    // incrementing 'n' when 'm' overflows)
    y_in_range = in_range(
      y_coord,
      this->scan.line + 1 - (sprite_height - 1),
      this->scan.line + 1
    );
    if (y_in_range) {
      this->reg.ppustatus.O = true;
      // not sure if these reads are used anywhere...
      this->oam.read(this->reg.oamaddr++);
      this->oam.read(this->reg.oamaddr++);
      this->oam.read(this->reg.oamaddr++);
    }
    // 3b.
    // If the value is not in range, increment n and m (without carry).
    // If n overflows to 0, go to 4; otherwise go to 3
    else {
      // NOTE: the m increment is a hardware bug
      this->reg.oamaddr += 4 + 1;

      if (this->reg.oamaddr == 0) goto step4;
      else                        goto step3;
    }

  step4:
    // 4.
    // Attempt (and fail) to copy OAM[n][0] into the next free slot in secondary
    // OAM, and increment n
    // (repeat until HBLANK is reached)

    // since i'm not cycle-accurate, just return here...
    return;
  } else

  // Cycles 257-320: Sprite fetches (8 sprites total, 8 cycles per sprite)
  if (in_range(this->scan.cycle, 257, 320)) {
    // reset state
    if (this->scan.cycle == 257) {
      this->spr_eval.oam2addr = 0;
    }

    const uint sprite = (this->scan.cycle - 257) / 8;
    const uint step = (this->scan.cycle - 257) % 8;

    // i can't tell if these reads are being used, but i'm going to follow what
    // the wiki says.
    if (in_range(step, 0, 3)) this->oam2.read(sprite * 4 + step);
    if (in_range(step, 4, 7)) this->oam2.read(sprite * 4 + 3);
  } else

  // Cycles 321-340+0: Background render pipeline initialization
  if (in_range(this->scan.cycle, 321, 340) || this->scan.cycle == 0) {
    // Read the first byte in secondary OAM

    // again, this read doesn't seem to be used anywhere...
    this->oam2.read(0x00);
  }
}

void PPU::get_bgr_pixel(bool& has_spr, u8& color) {
  // TODO: implement me
  has_spr = 0;
  color = 0x00;
}

void PPU::get_spr_pixel(bool& has_spr, u8& color, bool& priority) {
  // This is super ugly and not cycle-accurate at all.
  // BUT GOD DAMN IT I JUST WANT TO PLAY SOME DANKEY KANG!

  // huge optimization
  static bool skipme [256] = {false};
  if (this->scan.cycle == 0) {
    for (uint i = 0; i < 256; i++) skipme[i] = false;
  }

  const uint sprite_height = this->reg.ppuctrl.H ? 16 : 8;

  for (uint sprite = 0; sprite < 64; sprite++) {
    if (skipme[sprite]) continue;

    // Read sprite data
    u8 y_pos      = this->oam[sprite * 4 + 0] + 1;
    u8 tile_index = this->oam[sprite * 4 + 1];
    union {
      u8 val;
      BitField<0, 2> palette;
      // BitField<2, 3> unimplemented;
      BitField<5> priority;
      BitField<6> flip_horizontal;
      BitField<7> flip_vertical;
    } attributes  { this->oam[sprite * 4 + 2] };
    u8 x_pos      = this->oam[sprite * 4 + 3];

    bool on_this_line = in_range(
      y_pos,
      this->scan.line - sprite_height + 1,
      this->scan.line
    );
    if (!on_this_line) {
      skipme[sprite] = true;
      continue;
    }

    // We know that the sprite is present on this scanline (otherwise it would
    // not be in the secondary OAM!)
    // All we need to check is if it has a pixel at the current x addr
    if (in_range(x_pos, this->scan.cycle - 7, this->scan.cycle)) {
      // Cool! There is a sprite here!
      has_spr = true;
      priority = attributes.priority;

      // Now we just need to get the pixel data for this sprite...
      // First, which pixel of the sprite are we rendering?
      uint spr_row =      this->scan.line  - y_pos;
      uint spr_col = 7 - (this->scan.cycle - x_pos);

      if (attributes.flip_vertical)   spr_row = sprite_height - 1 - spr_row;
      if (attributes.flip_horizontal) spr_col =             8 - 1 - spr_col;

      // Get that pixel yo!
      const u16 tile_addr = (0x1000 * this->reg.ppuctrl.S) + (tile_index * 16);
      u8 lo_bp = this->mem[tile_addr + spr_row + 0];
      u8 hi_bp = this->mem[tile_addr + spr_row + 8];
      u2 pixel_type = nth_bit(lo_bp, spr_col) + 2 * nth_bit(hi_bp, spr_col);

      // Finally, we can set it's color!
      color = this->mem.peek(0x3F10 + attributes.palette * 4 + pixel_type);

      // no need to keep looping :)
      return;
    }

    // Otherwise, keep looking
    continue;
  }
}

/*----------------------------  Core Render Loop  ----------------------------*/

void PPU::cycle() {
#ifdef DEBUG_PPU
  this->update_debug_windows();
#endif

  const bool start_new_line = this->scan.cycle == 342;

  if (start_new_line) {
    // update scanline tracking vars
    this->scan.line += 1;
    this->scan.cycle = 0;
    // check for rollover
    if (this->scan.line > 261) {
      this->scan.line = 0;

      this->frames += 1;
    }
  }

  assert(in_range(this->scan.line,  0, 261));
  assert(in_range(this->scan.cycle, 0, 341));

  Color render_color;

  /* TEMP */ bool renderme = false;

  // Check if there is anything to render
  // (i.e: the show sprites flag is on, or show backgrounds flag is on)
  if (this->reg.ppumask.s || this->reg.ppumask.b) {
    // Visible Scanlines
    if (this->scan.line < 240) {
      bool has_bgr = false;
      u8 bgr_color = 0x00;
      // Get Background data
      if (this->reg.ppumask.b) {
        this->get_bgr_pixel(has_bgr, bgr_color);
      }

      // Get Sprite data
      bool has_spr  = false;
      u8 spr_color  = 0x00;
      bool priority = false;
      if (this->reg.ppumask.s) {
        this->get_spr_pixel(has_spr, spr_color, priority);
      }

      // Priority Multiplexer decision table
      // https://wiki.nesdev.com/w/index.php/PPU_rendering#Preface
      u8 color = 0;
      /**/ if (has_bgr == 0 && has_spr == 0) color = this->mem[0x3F00];
      else if (has_bgr == 0 && has_spr != 0) color = spr_color;
      else if (has_bgr != 0 && has_spr == 0) color = bgr_color;
      else if (has_bgr != 0 && has_spr != 0) color = priority
                                                      ? bgr_color
                                                      : spr_color;


      /* TEMP */ if (color == spr_color && spr_color != this->mem[0x3F00]) renderme = true;
      /* TEMP */ if (color == spr_color)

      render_color = this->palette[color];
    }

    /* TEMP */ if (renderme)

    this->draw_dot(render_color);
  }



  // Sprite Evaluation
  if (this->scan.line < 240) { // only on visible scanlines
    this->sprite_eval();
  }

  // Enable / Disable vblank
  if (start_new_line) {
    // vblank start on line 241...
    if (this->scan.line == 241 && this->reg.ppuctrl.V) {
      this->reg.ppustatus.V = true;
      this->interrupts.request(Interrupts::NMI);
    }

    // ...and ends after line 260
    if (this->scan.line == 261) {
      this->reg.ppustatus.O = false;
      this->reg.ppustatus.V = false;
    }
  }

  // Update cycle counts
  this->scan.cycle += 1;
  this->cycles += 1;
}

/*---------------------------------  Palette  --------------------------------*/

const Color PPU::palette [64] = {
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
      DebugPixelbuffWindow* window,
      bool render_to_main_window = false
    ) {
      for (uint y = 0; y < 8; y++) {
        u8 lo_bp = this->mem.peek(tile_addr + y + 0);
        u8 hi_bp = this->mem.peek(tile_addr + y + 8);

        for (uint x = 0; x < 8; x++) {
          u2 pixel_type = nth_bit(lo_bp, x) + 2 * nth_bit(hi_bp, x);

          Color color = this->palette[
            this->mem.peek(0x3F00 + palette * 4 + pixel_type)
          ];

          // Render to main window too?
          if (render_to_main_window) {
            u32* pixels = reinterpret_cast<u32*>(this->framebuff);
            u8 main_x = (tl_x % 256) + 7 - x;
            u8 main_y = (tl_y % 240) + y;
            pixels[(256 * main_y) + main_x] = color;
          }

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
      uint offset_x, uint offset_y,
      bool render_to_main_window = false
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

        paint_tile(tile_addr, tl_x, tl_y, palette, name_t, render_to_main_window);
      }
    };

    paint_nametable(0x2000, 0,        0 , true); // TEMP: render to main window
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

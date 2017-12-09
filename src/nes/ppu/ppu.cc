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
  this->reg.oamdata = 0x00; // ?

  this->reg.v = 0x0000;
  this->reg.t = 0x0000;
  this->reg.x = 0;

  this->reg.ppudata = 0x00; // ?

  this->bgr = {0, 0, 0, 0};
}

void PPU::reset() {
  this->cycles = 0;
  this->frames = 0;

  // http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  this->reg.ppuctrl.raw = 0x00;
  this->reg.ppumask.raw = 0x00;

  // this->reg.ppustatus is unchanged

  // this->reg.oamaddr   is unchanged
  // this->reg.oamdata   is unchanged?

  this->latch = 0;
  // this->reg.v is unchanged
  // this->reg.t is unchanged?
  // this->reg.x is unchanged?

  this->reg.ppudata = 0x00; // ?
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

  // This array caused me a lot of heartache and headache.
  //
  // I spent ~1h trying to debug why this->scan.line and this->scan.cycle were
  // being overwritten with bogus values, and in the end, I figured out why...
  //
  // I was indexing into this pixels array incorrectly, writing past the end of
  // it, but lucky for me, the way that the PPU class is laid out in memory,
  // this->scan immediately followed the pixels array, and was writable.
  //
  // Why the hell did I decide to write this in C++ again?

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
                    if (this->reg.v <= 0x3EFF) {
                      // Reading Nametables
                      // retval = from internal buffer
                      retval = this->reg.ppudata;
                      // Fill read buffer with acutal data
                      u8 val = this->mem[this->reg.v];
                      this->reg.ppudata = val;
                    } else {
                      // Reading Pallete
                      // retval = directly from memory
                      retval = this->mem[this->reg.v];
                      // Fill read buffer with the mirrored nametable data
                      // Why? Because the wiki said so!
                      u8 val = this->mem[this->reg.v % 0x2000];
                      this->reg.ppudata = val;
                    }

                    // (0: add 1, going across; 1: add 32, going down)
                    if (this->reg.ppuctrl.I == 0) this->reg.v.val += 1;
                    if (this->reg.ppuctrl.I == 1) this->reg.v.val += 32;
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
  case PPUDATA:   { if (this->reg.v <= 0x3EFF) {
                      retval = this->reg.ppudata;
                    } else {
                      retval = this->mem.peek(this->reg.v);
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
                    // t: ....BA.. ........ = d: ......BA
                    this->reg.t = (this->reg.t & 0xF3FF) | (u16(val & 0x03) << 10);
                  } return;
  case PPUMASK:   { this->reg.ppumask.raw = val;
                  } return;
  case OAMADDR:   { this->reg.oamaddr = val;
                  } return;
  case OAMDATA:   { this->oam[this->reg.oamaddr++] = val;
                  } return;
  case PPUSCROLL: { if (this->latch == 0) {
                      // t: ........ ...HGFED = d: HGFED...
                      this->reg.t.coarse_x = val >> 3;
                      // x:               CBA = d: .....CBA
                      this->reg.x = val & 0x07;
                    }
                    if (this->latch == 1) {
                      // t: .CBA..HG FED..... = d: HGFEDCBA
                      this->reg.t.coarse_y = val >> 3;
                      this->reg.t.fine_y   = val & 0x07;
                    }
                    this->latch = !this->latch;
                  } return;
  case PPUADDR:   { if (this->latch == 0) {
                      // t: ..FEDCBA ........ = d: ..FEDCBA
                      // t: .X...... ........ = 0
                      this->reg.t.hi = val & 0x3F;
                    }
                    if (this->latch == 1) {
                      // t: ........ HGFEDCBA = d: HGFEDCBA
                      this->reg.t.lo = val;
                      // v                    = t
                      this->reg.v = this->reg.t;
                    }
                    this->latch = !this->latch;
                  } return;
  case PPUDATA:   { this->mem[this->reg.v] = val;
                    // (0: add 1, going across; 1: add 32, going down)
                    if (this->reg.ppuctrl.I == 0) this->reg.v.val += 1;
                    if (this->reg.ppuctrl.I == 1) this->reg.v.val += 32;
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

// TODO: cycle accurate
// For now, it just performs all the calculations on cycle 0 of a scanline
void PPU::sprite_eval() {
  if (this->scan.cycle != 0)
    return;

  // (This will break games that switch sprite_height mid-scanline)
  const uint sprite_height = this->reg.ppuctrl.H ? 16 : 8;

  // Fill OAM2 memory with 0xFF
  for (uint addr = 0; addr < 32; addr++)
    this->oam2[addr] = 0xFF;

  // Pointer into OAM2 RAM
  uint oam2_addr = 0; // from 0 - 32

  for (uint sprite = 0; sprite < 64; sprite++) {
    // Check if sprite is on current line
    u8 y_pos = this->oam[sprite * 4 + 0];
    bool on_this_line = in_range(
      y_pos,
      this->scan.line - sprite_height,
      this->scan.line - 1
    );

    if (!on_this_line)
      continue;

    // Otherwise, copy this sprite into OAM2 (room permitting)
    if (oam2_addr < 32) {
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 0];
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 1];
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 2];
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 3];
    } else {
      this->reg.ppustatus.O = true; // Sprite Overflow
      // Also, early return too
      return;
    }
  }
}

/*-----------------------  Pixel Evaluation Functions  -----------------------*/

// https://wiki.nesdev.com/w/index.php/PPU_rendering
// TODO: make this more "hardware" accurate
PPU::Pixel PPU::get_bgr_pixel() {
  // First, evaluate the current pixel
  // TODO: make this more "hardware" accurate

  // What corner is this particular 8x8 tile in?
  //
  // top left = 0
  // top right = 1
  // bottom left = 2
  // bottom right = 3
  const u2 corner = (this->reg.v.coarse_x % 4) / 2
                  + (this->reg.v.coarse_y % 4) / 2 * 2;

  // Recall that the attribute byte stores the palette assignment data
  // formatted as follows:
  //
  // attribute = (topleft << 0)
  //           | (topright << 2)
  //           | (bottomleft << 4)
  //           | (bottomright << 6)
  //
  // thus, we can reverse the process with some clever bitmasking!
  const u8 mask = 0x03 << (corner * 2);
  const u2 palette = (this->bgr.at_byte & mask) >> (corner * 2);

  const u3 x = 7 - ((this->scan.cycle + this->reg.x) % 8);

  const u2 pixel_type = nth_bit(this->bgr.tile_lo, x)
                      + nth_bit(this->bgr.tile_hi, x) * 2;

  Color color = this->palette[this->mem[0x3F00 + palette * 4 + pixel_type]];

  // Set the pixel to be returned
  Pixel ret_pixel = Pixel { color, 0 };


  // If background rendering is disabled, return a empty pixel
  if (this->reg.ppumask.b == false)
    ret_pixel = Pixel();

  // now, perform memory accesses

  /**/ if (in_range(this->scan.cycle, 0,   0  )) { /* idle cycle */ }
  else if (in_range(this->scan.cycle, 1,   256) ||
           in_range(this->scan.cycle, 321, 336)) {
    // Each mem-access takes 2 cycles to perform
    switch (this->scan.cycle % 8) {
    // 1) Fetch Nametable Byte
    // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Tile_and_attribute_fetching
    case 1: {
      u16 nt_addr = 0x2000
                  | (this->reg.v & 0x0FFF);
      this->bgr.nt_byte = this->mem[nt_addr];
    } break;

    // 2) Fetch Attribute Table Byte
    // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Tile_and_attribute_fetching
    case 3: {
      u16 at_addr = 0x23C0
                  | ((this->reg.v >> 0) & 0x0C00)
                  | ((this->reg.v >> 4) & 0x38)
                  | ((this->reg.v >> 2) & 0x07);
      this->bgr.at_byte = this->mem[at_addr];
    } break;

    // 3) Fetch lo tile bitmap
    case 5: {
      u16 tile_addr = this->reg.ppuctrl.B * 0x1000
                    + this->bgr.nt_byte * 16
                    + this->reg.v.fine_y;

      this->bgr.tile_lo = this->mem[tile_addr + 0];
    } break;

    // 4) Fetch hi tile bitmap
    case 7: {
      u16 tile_addr = this->reg.ppuctrl.B * 0x1000
                    + this->bgr.nt_byte * 16
                    + this->reg.v.fine_y;

      this->bgr.tile_hi = this->mem[tile_addr + 8];

      // increment Coarse X
      // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Wrapping_around
      if (this->reg.v.coarse_x == 31) {
        this->reg.v = this->reg.v ^ 0x0400; // switch horizontal nametable
      }
      this->reg.v.coarse_x++;
    } break;

    default: break; // do nothing on odd-cycles
    }
  }
  else if (in_range(this->scan.cycle, 257, 320)) { /* sprite data fetches */ }
  else if (in_range(this->scan.cycle, 337, 340)) {
    // Two bytes are fetched, but the purpose for this is unknown.
    // These fetches are 2 PPU cycles each.
    // Both of the bytes fetched here are the same nametable byte that will be
    // fetched at the beginning of the next scanline (tile 3, in other words).
    if (this->scan.cycle % 2) {
      this->bgr.nt_byte = this->mem[this->reg.v];
    }
    // This is used for timing by some mappers.
  }
  else { /* idle */ }

  // Y increment
  // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Wrapping_around
  if (this->scan.cycle == 256 && this->reg.ppumask.b) {
    if (this->reg.v.fine_y == 7) {
      if (this->reg.v.coarse_y == 29) {
        this->reg.v.coarse_y = 0;
        this->reg.v = this->reg.v ^ 0x0800; // switch vertical nametable
      }
      else {
        this->reg.v.coarse_y++;
      }
    }
    this->reg.v.fine_y++;
  }

  // Check for any copyX / copyY
    if (this->scan.cycle == 257) {
    // copyX
    // v: .....F.. ...EDCBA = t: .....F.. ...EDCBA
    this->reg.v = (this->reg.v & 0xFBE0) | (this->reg.t & 0x041F);
  }
  if (this->scan.line == 261 && in_range(this->scan.cycle, 268, 304)) {
    // copyY
    // v: .IHGF.ED CBA..... = t: .IHGF.ED CBA.....
    this->reg.v = (this->reg.v & 0x841F) | (this->reg.t & 0x7BE0);
  }



  // finally, return pixel
  return ret_pixel;
}

// TODO: make this more "hardware" accurate
// https://wiki.nesdev.com/w/index.php/PPU_rendering
PPU::Pixel PPU::get_spr_pixel() {
  if (this->reg.ppumask.s == false)
    return Pixel();

  const uint sprite_height = this->reg.ppuctrl.H ? 16 : 8;

  // Scan through OAM2 to see if there is a sprite to draw at this scan.cycle
  for (uint sprite = 0; sprite < 8; sprite++) {
    // Read sprite data
    u8 y_pos      = this->oam2[sprite * 4 + 0];
    u8 tile_index = this->oam2[sprite * 4 + 1];
    union {
      u8 val;
      BitField<0, 2> palette;
    //BitField<2, 3> unimplemented;
      BitField<5> priority;
      BitField<6> flip_horizontal;
      BitField<7> flip_vertical;
    } attributes  { this->oam2[sprite * 4 + 2] };
    u8 x_pos      = this->oam2[sprite * 4 + 3];

    // If the sprite is invalid (i.e: all 0xFF), early return, since there won't
    // be any more sprites after this
    if (
      y_pos == 0xFF &&
      y_pos == x_pos &&
      y_pos == tile_index &&
      y_pos == attributes.val
    ) return Pixel();

    // Does this sprite have a pixel at the current x addr?
    if (in_range(x_pos, this->scan.cycle - 7, this->scan.cycle)) {
      // Cool! There is a sprite here!

      // Now we just need to get the actual pixel data for this sprite...
      // First, which pixel of the sprite are we rendering?
      uint spr_row = this->scan.line - y_pos - 1;
      uint spr_col = 7 - (this->scan.cycle - x_pos);

      if (attributes.flip_vertical)   spr_row = sprite_height - 1 - spr_row;
      if (attributes.flip_horizontal) spr_col =             8 - 1 - spr_col;

      // Time to get the pixel type yo!
      u16 tile_addr = (0x1000 * this->reg.ppuctrl.S) + (tile_index * 16);
      u8 lo_bp = this->mem[tile_addr + spr_row + 0];
      u8 hi_bp = this->mem[tile_addr + spr_row + 8];
      u2 pixel_type = nth_bit(lo_bp, spr_col) + 2 * nth_bit(hi_bp, spr_col);

      // If the pixel is transparent, continue looking for a valid sprite...
      if (pixel_type == 0)
        continue;

      // Otherwise, fetch which pallete color to use, and return the pixel!
      u8 palette = this->mem.peek(0x3F10 + attributes.palette * 4 + pixel_type);
      return Pixel { this->palette[palette], attributes.priority };
    }

    // Otherwise, keep looking
    continue;
  }

  // if nothing was found, return a empty pixel
  return Pixel();
}

/*----------------------------  Core Render Loop  ----------------------------*/

void PPU::cycle() {
#ifdef DEBUG_PPU
  this->update_debug_windows();
#endif

  const bool start_new_line = this->scan.cycle == 341;

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

  // If there is stuff to render:
  if (this->reg.ppumask.s || this->reg.ppumask.b) {
    // Get pixel data
    PPU::Pixel bgr_pixel = this->get_bgr_pixel();
    PPU::Pixel spr_pixel = this->get_spr_pixel();

    // Alpha channel of 0 means that pixel is not on
    bool bgr_on = bgr_pixel.color.a;
    bool spr_on = spr_pixel.color.a;

    // Priority Multiplexer decision table
    // https://wiki.nesdev.com/w/index.php/PPU_rendering#Preface
    Color dot_color = Color();
    /**/ if (!bgr_on && !spr_on) dot_color = Color(); // this->palette[this->mem[0x3F00]];
    else if (!bgr_on &&  spr_on) dot_color = spr_pixel.color;
    else if ( bgr_on && !spr_on) dot_color = bgr_pixel.color;
    else if ( bgr_on &&  spr_on) dot_color = spr_pixel.priority
                                              ? bgr_pixel.color
                                              : spr_pixel.color;

    // Doesn't actually draw anything when this->scan.cycle >= 255
    this->draw_dot(dot_color);
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

#include "ppu.h"

#include <cassert>
#include <cstdio>
#include <cstring>

PPU::~PPU() {
  delete this->framebuff;
}

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
  this->framebuff = new u8[240 * 256 * 4]();

  this->power_cycle();

#ifdef DEBUG_PPU
  this->init_debug_windows();
#endif
}

void PPU::power_cycle() {
  this->cycles = 0;
  this->frames = 0;

  this->scan.line = 0;
  this->scan.cycle = 0;

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

  this->reg.v.val = 0x0000;
  this->reg.t.val = 0x0000;
  this->reg.x = 0;

  this->reg.ppudata = 0x00; // ?

  // This is probably the cleanest way to do this tbh
  memset(&this->bgr, 0, sizeof this->bgr);
  memset(&this->spr, 0, sizeof this->spr);
}

void PPU::reset() {
  this->cycles = 0;
  this->frames = 0;

  this->scan.line = 0;
  this->scan.cycle = 0;

  this->cpu_data_bus = 0x00; // ?

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

void PPU::draw_dot(Color color, uint x, uint y) {
  assert(x < 256 && y < 240);

  const uint offset = (256 * 4 * y) + x * 4;
  assert(offset + 3 < 240 * 256 * 4);
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

  /* b */ this->framebuff[offset + 0] = color.b;
  /* g */ this->framebuff[offset + 1] = color.g;
  /* r */ this->framebuff[offset + 2] = color.r;
  /* a */ this->framebuff[offset + 3] = color.a;
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
                    // THIS BREAKS cpu_dummy_writes_oam.nes
                    bool on_visible_line = this->scan.line < 240;
                    bool is_clearing_oam2 = in_range(this->scan.cycle, 1, 64);
                    if (on_visible_line && is_clearing_oam2) {
                      retval = 0xFF;
                    } else {
                      retval = this->oam[this->reg.oamaddr];
                    }
                  } break;
  case PPUDATA:   { // PPUDATA read buffer (post-fetch) logic
                    if (this->reg.v <= 0x3EFF) {
                      // Reading Nametables
                      // retval = from internal buffer
                      retval = this->reg.ppudata;
                      // Fill read buffer with acutal data
                      this->reg.ppudata = this->mem[this->reg.v];
                    } else {
                      // Reading Pallete
                      // retval = directly from memory
                      retval = this->mem[this->reg.v];
                      // Fill read buffer with the mirrored nametable data
                      this->reg.ppudata = retval;
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
                    // fprintf(stderr,
                    //   "[PPU] Reading from Write-Only register: 0x%04X\n",
                    //   addr
                    // );
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
                    // fprintf(stderr, "[DMA] Peeking DMA is undefined!\n");
                    retval = 0x00;
                  } break;
  default:        { retval = this->cpu_data_bus;
                    // fprintf(stderr,
                    //   "[PPU] Peeking from Write-Only register: 0x%04X\n",
                    //   addr
                    // );
                  } break;
  }

  return retval;
}

void PPU::write(u16 addr, u8 val) {
   //if (!this->reg.ppustatus.V) {
   //  fprintf(stderr, "wr to 0x%04X outside of vblank, at %d : %d\n", addr, this->scan.line, this->scan.cycle);
   //  this->draw_dot(Color(0x00FF00));
   //}

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
  case PPUCTRL:   { // There is a chance an NMI will fire here
                    decltype(this->reg.ppuctrl) old_ppuctrl = this->reg.ppuctrl;
                    this->reg.ppuctrl.raw = val;
                    bool toggled_V_on = !old_ppuctrl.V && this->reg.ppuctrl.V;
                    if (toggled_V_on && this->reg.ppustatus.V)
                      this->interrupts.request(Interrupts::Type::NMI);
                    // t: ....BA.. ........ = d: ......BA
                    this->reg.t.nametable = val & 0x03;
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
                      this->reg.v.val = this->reg.t.val;
                    }
                    this->latch = !this->latch;
                  } return;
  case PPUDATA:   { this->mem[this->reg.v] = val;
                    // (0: add 1, going across; 1: add 32, going down)
                    if (this->reg.ppuctrl.I == 0) this->reg.v.val += 1;
                    if (this->reg.ppuctrl.I == 1) this->reg.v.val += 32;
                  } return;
  case OAMDMA:    { // The CPU stalls during DMA, but the PPU keeps running!
                    // DMA takes 513 / 514 CPU cycles:
                    #define CPU_CYCLE() \
                      do {this->cycle();this->cycle();this->cycle();} while(0);

                    // 1 dummy cycle
                    CPU_CYCLE();

                    // +1 cycle if starting on odd CPU cycle
                    if ((this->cycles / 3) % 2)
                      CPU_CYCLE();

                    // 512 cycles of reading & writing
                    this->dma.start(val);
                    for (uint i = 0; i < 256; i++) {
                      u8 cpu_val = this->dma.transfer();        CPU_CYCLE();
                      this->oam[this->reg.oamaddr++] = cpu_val; CPU_CYCLE();
                    }
                    #undef CPU_CYCLE
                  } return;
  // default:        { fprintf(stderr,
  //                     "[PPU] Writing to Read-Only register: 0x%04X\n <- 0x%02X",
  //                     addr,
  //                     val
  //                   );
  //                 } return;
  }
}

/*----------------------------  Helper Functions  ----------------------------*/

// TODO: cycle accurate
// For now, it just performs all the calculations on cycle 0 of a scanline
void PPU::spr_fetch() {
  if (this->scan.cycle != 0)
    return;

  // Fill OAM2 memory with 0xFF
  for (uint addr = 0; addr < 32; addr++)
    this->oam2[addr] = 0xFF;

  this->spr.spr_zero_on_line = false;

  // Pointer into OAM2 RAM
  uint oam2_addr = 0; // from 0 - 32

  for (uint sprite = 0; sprite < 64; sprite++) {
    // Check if sprite is on current line
    u8 y_pos = this->oam[sprite * 4 + 0];
    bool on_this_line = in_range(
      int(y_pos),
      int(this->scan.line) - (this->reg.ppuctrl.H ? 16 : 8),
      int(this->scan.line) - 1
    );

    if (!on_this_line) {
      continue;
    }

    if (sprite == 0) {
      this->spr.spr_zero_on_line = true;
    }

    // Otherwise, copy this sprite into OAM2 (room permitting)
    if (oam2_addr < 32) {
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 0];
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 1];
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 2];
      this->oam2[oam2_addr++] = this->oam[sprite * 4 + 3];
    } else {
      if (this->reg.ppumask.is_rendering) {
        this->reg.ppustatus.O = true; // Sprite Overflow
      }
    }
  }
}

// https://wiki.nesdev.com/w/index.php/PPU_rendering
// using ntsc_timing.png as a visual guide
void PPU::bgr_fetch() {
  assert(this->scan.line < 240 || this->scan.line == 261);

  /**/ if (in_range(this->scan.cycle, 0,   0  )) { /* idle cycle */ }
  else if (in_range(this->scan.cycle, 1,   256) ||
           in_range(this->scan.cycle, 321, 336)) {
    // Each mem-access takes 2 cycles to perform
    switch (this->scan.cycle % 8) {
    case 1: {
      this->bgr.shift.tile[0] &= 0xFF00;
      this->bgr.shift.tile[0] |= this->bgr.tile_lo;

      this->bgr.shift.tile[1] &= 0xFF00;
      this->bgr.shift.tile[1] |= this->bgr.tile_hi;

      this->bgr.shift.at_latch[0] = this->bgr.at_byte & 1;
      this->bgr.shift.at_latch[1] = this->bgr.at_byte & 2;
    }
    // 1) Fetch Nametable Byte
    // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Tile_and_attribute_fetching
    case 2: {
      u16 nt_addr = 0x2000
                  | (this->reg.v & 0x0FFF);
      this->bgr.nt_byte = this->mem[nt_addr];
    } break;

    // 2) Fetch Attribute Table Byte
    // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Tile_and_attribute_fetching
    case 4: {
      u16 at_addr = 0x23C0
                  | ((this->reg.v >> 0) & 0x0C00)
                  | ((this->reg.v >> 4) & 0x38)
                  | ((this->reg.v >> 2) & 0x07);
      this->bgr.at_byte = this->mem[at_addr];

      if (this->reg.v.coarse_y & 2) this->bgr.at_byte >>= 4;
      if (this->reg.v.coarse_x & 2) this->bgr.at_byte >>= 2;

    } break;

    // 3) Fetch lo tile bitmap
    case 6: {
      u16 tile_addr = this->reg.ppuctrl.B * 0x1000
                    + this->bgr.nt_byte * 16
                    + this->reg.v.fine_y;

      this->bgr.tile_lo = this->mem[tile_addr + 0];
    } break;

    // 4) Fetch hi tile bitmap
    case 0: {
      u16 tile_addr = this->reg.ppuctrl.B * 0x1000
                    + this->bgr.nt_byte * 16
                    + this->reg.v.fine_y;

      this->bgr.tile_hi = this->mem[tile_addr + 8];

      // increment Coarse X
      // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Wrapping_around
      if (this->reg.ppumask.is_rendering) {
        if (this->reg.v.coarse_x == 31) {
          this->reg.v.val ^= 0x0400; // switch horizontal nametable
        }
        this->reg.v.coarse_x++;
      }
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
    if (this->scan.cycle % 2 == 0) {
      this->bgr.nt_byte = this->mem[this->reg.v];
    }
    // This is used for timing by some mappers.
  }
  else { assert(false); /* This should not happen. */ }

  // Y increment
  // https://wiki.nesdev.com/w/index.php/PPU_scrolling#Wrapping_around
  if (this->scan.cycle == 256 && this->reg.ppumask.is_rendering) {
    if (this->reg.v.fine_y == 7) {
      if (this->reg.v.coarse_y == 29) {
        this->reg.v.coarse_y = 0;
        this->reg.v.val ^= 0x0800; // switch vertical nametable
      } else {
        this->reg.v.coarse_y++;
      }
    }
    this->reg.v.fine_y++;
  }

  // ---- Copy X / Copy Y ---- //
  if (this->reg.ppumask.is_rendering) {
    if (this->scan.cycle == 257) {
      // copyX
      // v: .....F.. ...EDCBA = t: .....F.. ...EDCBA
      this->reg.v.coarse_x = this->reg.t.coarse_x;
      this->reg.v.nametable = (this->reg.v.nametable & 2)
                            | (this->reg.t.nametable & 1);
    }
    if (this->scan.line == 261 && in_range(this->scan.cycle, 280, 304)) {
      // copyY
      // v: .IHGF.ED CBA..... = t: .IHGF.ED CBA.....
      this->reg.v.coarse_y = this->reg.t.coarse_y;
      this->reg.v.fine_y = this->reg.t.fine_y;
      this->reg.v.nametable = (this->reg.v.nametable & 1)
                            | (this->reg.t.nametable & 2);
    }
  }
}


/*-----------------------  Pixel Evaluation Functions  -----------------------*/

// https://wiki.nesdev.com/w/index.php/PPU_rendering
PPU::Pixel PPU::get_bgr_pixel() {
  assert(this->scan.line < 240 || this->scan.line == 261);

  uint pixel_type = (nth_bit(this->bgr.shift.tile[1], 15 - this->reg.x) << 1)
                | (nth_bit(this->bgr.shift.tile[0], 15 - this->reg.x) << 0);

  uint palette = (nth_bit(this->bgr.shift.at[1], 7 - this->reg.x) << 1)
               | (nth_bit(this->bgr.shift.at[0], 7 - this->reg.x) << 0);

  // Shift Background registers
  if (in_range(this->scan.cycle, 1,   256) ||
      in_range(this->scan.cycle, 321, 336))
  {
    this->bgr.shift.tile[0] <<= 1;
    this->bgr.shift.tile[1] <<= 1;

    this->bgr.shift.at[0] <<= 1;
    this->bgr.shift.at[0] |= this->bgr.shift.at_latch[0];
    this->bgr.shift.at[1] <<= 1;
    this->bgr.shift.at[1] |= this->bgr.shift.at_latch[1];
  }

  // Perform data fetches
  this->bgr_fetch();

  // Check for background mask disable
  if (!this->reg.ppumask.m && (this->scan.cycle - 2) < 8)
    return Pixel();

  // If background rendering is disabled, return a empty pixel
  if (this->reg.ppumask.b == false || this->scan.line >= 240)
    return Pixel();

  return Pixel { pixel_type != 0, this->mem[0x3F00 + palette * 4 + pixel_type], 0 };
}

// TODO: make this more "hardware" accurate.
//       this should make all the dumb -2 go away
// https://wiki.nesdev.com/w/index.php/PPU_rendering
PPU::Pixel PPU::get_spr_pixel(PPU::Pixel& bgr_pixel) {
  const uint x = this->scan.cycle - 2;

  // Fetch data
  this->spr_fetch();

  // Check to see if sprite rendering is even enabled
  if (this->reg.ppumask.s == false)
    return Pixel();

  // Check for sprite mask disable
  if (!this->reg.ppumask.M && x < 8) {
    return Pixel();
  }

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
      0xFF == y_pos &&
      0xFF == x_pos &&
      0xFF == tile_index &&
      0xFF == attributes.val
    ) return Pixel();

    // Does this sprite have a pixel at the current x addr?
    bool x_in_range = in_range(int(x_pos), int(x) - 7, int(x));
    if (x_in_range) {
      // Cool! There is a sprite here!

      // Now we just need to get the actual pixel data for this sprite...
      // First, which pixel of the sprite are we rendering?
      uint spr_row = this->scan.line - y_pos - 1;
      uint spr_col = 7 - (x - x_pos);

      const uint sprite_height = this->reg.ppuctrl.H ? 16 : 8;
      if (attributes.flip_vertical)   spr_row = sprite_height - 1 - spr_row;
      if (attributes.flip_horizontal) spr_col =             8 - 1 - spr_col;

      // Time to get the pixel type yo!

      // NOTE: I completely overlooked handling 8x16 sprites for a very long
      //       time. I did manage to figure it out though!

      // When in 8x16 sprite mode, the sprite_table to use is solely dependant
      // on odd/eveness of the tile_index
      bool sprite_table = !this->reg.ppuctrl.H
        ? this->reg.ppuctrl.S
        : tile_index & 1;

      if (this->reg.ppuctrl.H) {
        tile_index &= 0xFE;
        // And if we have finished the top half, read from adjacent tile
        if (spr_row > 7) {
          tile_index++;
          spr_row -= 8;
        }
      }

      u16 tile_addr = (0x1000 * sprite_table) + (tile_index * 16) + spr_row;
      u8 lo_bp = this->mem[tile_addr + 0];
      u8 hi_bp = this->mem[tile_addr + 8];
      u8 pixel_type = nth_bit(lo_bp, spr_col) + (nth_bit(hi_bp, spr_col) << 1);

      // If the pixel is transparent, continue looking for a valid sprite...
      if (pixel_type == 0)
        continue;

      // The rules for Sprite0 hit are fairly obtuse...
      // You can only get a sprite 0 hit when:
      if (
        this->spr.spr_zero_on_line &&     // Sprite 0 is on the line
        this->reg.ppumask.is_rendering && // And rendering is enabled
        this->reg.ppustatus.S == 0 &&     // And there has not been a spr hit
        (x_pos != 0xFF && x < 0xFF) &&   // And not when dot/spr is at 255
        bgr_pixel.is_on                   // And the bgr pixel is on
      ) this->reg.ppustatus.S = 1; // Only then does sprite hit occur

      // Otherwise, fetch which pallete color to use, and return the pixel!
      u8 palette = this->mem[0x3F10 + attributes.palette * 4 + pixel_type];
      return Pixel { true, palette, bool(attributes.priority) };
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

  // ---- Core Loop ---- //

  if (this->scan.line < 240 || this->scan.line == 261) {
    // Fetch Pixels
    PPU::Pixel bgr_pixel = this->get_bgr_pixel();
    PPU::Pixel spr_pixel = this->get_spr_pixel(bgr_pixel);

    // Priority Multiplexer decision table
    // https://wiki.nesdev.com/w/index.php/PPU_rendering#Preface
    // BG pixel | Sprite pixel | Priority | Output
    // --------------------------------------------
    // 0        | 0            | X        | BG ($3F00)
    // 0        | 1-3          | X        | Sprite
    // 1-3      | 0            | X        | BG
    // 1-3      | 1-3          | 0        | Sprite
    // 1-3      | 1-3          | 1        | BG

    const bool bgr_on = bgr_pixel.is_on;
    const bool spr_on = spr_pixel.is_on;

    u8 palette = 0x00;
    /**/ if (!bgr_on && !spr_on) palette = this->mem[0x3F00];
    else if (!bgr_on &&  spr_on) palette = spr_pixel.palette;
    else if ( bgr_on && !spr_on) palette = bgr_pixel.palette;
    else if ( bgr_on &&  spr_on) palette = spr_pixel.priority
                                              ? bgr_pixel.palette
                                              : spr_pixel.palette;

    uint x = (this->scan.cycle - 2);
    if (x < 256 && this->scan.line != 261) {
      this->draw_dot(this->palette[palette], x, this->scan.line);
    }
  }

  // ---- Enable / Disable vblank ---- //

  if (this->scan.cycle == 1) {
    // vblank start on line 241...
    if (this->scan.line == 241) {
      // MAJOR KEY: The vblank flag is _always_ set!
      this->reg.ppustatus.V = true;
      // Only the interrupt is affected by ppuctrl.V (not the flag!)
      if (this->reg.ppuctrl.V) {
        this->interrupts.request(Interrupts::NMI);
      }
    }

    // ...and is cleared in the pre-render line
    if (this->scan.line == 261) {
      this->reg.ppustatus.V = false;
      this->reg.ppustatus.S = false;
      this->reg.ppustatus.O = false;
    }
  }

  // ---- Update Counter ---- //

  // Update cycle counts
  this->scan.cycle += 1;
  this->cycles += 1;

  // Odd-Frame skip cycle
  if (
    this->frames % 2 == 1 &&
    this->scan.cycle == 340 &&
    this->scan.line == 261 &&
    this->reg.ppumask.is_rendering
  ) {
    this->cycles += 1;
    this->scan.cycle += 1;
  }

  // Check to see if the cycle has finished
  if (this->scan.cycle == 341) {
    // update scanline tracking vars
    this->scan.cycle = 0;
    this->scan.line += 1;
    // check for rollover
    if (this->scan.line == 262) {
      this->scan.line = 0;
      this->frames += 1;
    }
  }
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

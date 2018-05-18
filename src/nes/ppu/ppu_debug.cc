#ifdef DEBUG_PPU

#include "ppu.h"

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
  if (this->cycles == 0) {
      // load in a debug palette
      this->mem[0x3F00 + 0] = 0x1D;
      this->mem[0x3F00 + 1] = 0x2D;
      this->mem[0x3F00 + 2] = 0x3D;
      this->mem[0x3F00 + 3] = 0x30;
  }
  // i'm making it update every second right now
  static bool should_update = true;
  static u16 offset = 0;
  if (
    should_update == false &&
    this->frames % UPDATE_EVERY_X_FRAMES == UPDATE_EVERY_X_FRAMES - 1
  ) should_update = true;

  if ((this->cycles % 0xFFFF) == offset && should_update && this->frames % UPDATE_EVERY_X_FRAMES == 0) {
    should_update = false;
    offset++;

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
          uint pixel_type = nth_bit(lo_bp, x) + (nth_bit(hi_bp, x) << 1);

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
      union { // PPUADDR   - 0x2006 - PPU VRAM address register
        u16 val;
        // https://wiki.nesdev.com/w/index.php/PPU_scrolling
        BitField<0,  5> x_scroll;
        BitField<5,  5> y_scroll;
        BitField<10, 2> nametable;
        BitField<12, 3> y_scroll_fine;
      } addr;

      for (addr.val = base_addr; addr.val < base_addr + 0x400 - 64; addr.val++) {
        // Getting which tile to render is easy...

        u16 tile_addr = (this->reg.ppuctrl.B * 0x1000) // bg palette selector
                      + this->mem.peek(addr.val) * 16;

        // ...The hard part is figuring out the palette for it :)
        // http://wiki.nesdev.com/w/index.php/PPU_attribute_tables

        // What is the base address of the nametable we are reading from?
        const uint nt_base_addr = 0x2000 + (addr.nametable << 10);

        // Supertiles are 32x32 collections of pixels, where each 16x16 corner
        // of the supertile (4 8x8 tiles) can be assigned a palette
        const uint supertile_no = (addr.x_scroll / 4)
                                + (addr.y_scroll / 4) * 8;

        // Nice! Now we can pull data from the attribute table!
        // Now, to decipher which of the 4 palettes to use...
        const u8 attribute = this->mem[nt_base_addr + 0x3C0 + supertile_no];

        // What corner is this particular 8x8 tile in?
        //
        // top left = 0
        // top right = 1
        // bottom left = 2
        // bottom right = 3
        const uint corner = (addr.x_scroll % 4) / 2
                          + (addr.y_scroll % 4) / 2 * 2;

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
        const uint tile_no = addr.x_scroll + addr.y_scroll * 32;
        const uint tl_x = (tile_no % 32) * 8 + offset_x;
        const uint tl_y = (tile_no / 32) * 8 + offset_y;

        paint_tile(tile_addr, tl_x, tl_y, palette, name_t, render_to_main_window);
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

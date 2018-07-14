#include "ppu_debug.h"

#include <SDL.h>

// Simple SDL debug window that is pixel-addressable
struct PPUDebugModule::DebugPixelbuffWindow {
  SDL_Window*   window;
  SDL_Renderer* renderer;
  SDL_Texture*  texture;

  u8* frame;

  uint window_w, window_h;
  uint texture_w, texture_h;
  uint x, y;

  ~DebugPixelbuffWindow() {
    delete this->frame;

    SDL_DestroyTexture(this->texture);
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);
  }

  DebugPixelbuffWindow(
    const char* title,
    uint window_w, uint window_h,
    uint texture_w, uint texture_h,
    uint x, uint y
  ) {
    SDL_Init(SDL_INIT_EVERYTHING);

    this->window_w = window_w;
    this->window_h = window_h;
    this->texture_w = texture_w;
    this->texture_h = texture_h;
    this->x = x;
    this->y = y;

    this->window = SDL_CreateWindow(
        title,
        x, y,
        window_w, window_h,
        SDL_WINDOW_RESIZABLE
    );

    this->renderer = SDL_CreateRenderer(
      this->window,
      -1,
      SDL_RENDERER_ACCELERATED
    );

    this->texture = SDL_CreateTexture(
      this->renderer,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      texture_w, texture_h
    );

    this->frame = new u8 [texture_w * texture_h * 4]();
  }

  DebugPixelbuffWindow(const DebugPixelbuffWindow&) = delete;

  void set_pixel(uint x, uint y, u8 r, u8 g, u8 b, u8 a) {
    const uint offset = ((this->texture_w * y) + x) * 4;
    this->frame[offset + 0] = b;
    this->frame[offset + 1] = g;
    this->frame[offset + 2] = r;
    this->frame[offset + 3] = a;
  }

  void set_pixel(uint x, uint y, u32 color) {
    ((u32*)this->frame)[(this->texture_w * y) + x] = color;
  }

  void render() {
    // Update the screen texture
    SDL_UpdateTexture(this->texture, nullptr, this->frame, this->texture_w * 4);

    // Render everything
    SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);

    // Reveal out the composited image
    SDL_RenderPresent(this->renderer);
  }
};

PPUDebugModule::PPUDebugModule(SharedState& gui) : GUIModule(gui) {
  fprintf(stderr, "[GUI][PPU Debug] Initializing...\n");

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

  this->gui.nes._ppu()._callbacks.scanline.add_cb(PPUDebugModule::cb_scanline, this);
}

PPUDebugModule::~PPUDebugModule() {
  fprintf(stderr, "[GUI][PPU Debug] Shutting down...\n");

  delete name_t;
  delete nes_palette;
  delete palette_t;
  delete patt_t;
}

void PPUDebugModule::input(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
    case SDLK_SPACE: this->nametable = !this->nametable;          break;
    case SDLK_DOWN:  if (this->scanline != 239) this->scanline++; break;
    case SDLK_UP:    if (this->scanline !=   0) this->scanline--; break;
    }
  }
}

void PPUDebugModule::update() {
  // updates happen on callbacks
}

void PPUDebugModule::cb_scanline(void* self) {
  ((PPUDebugModule*)self)->sample_ppu();
}

void PPUDebugModule::sample_ppu() {
  const PPU& ppu = this->gui.nes._ppu();

  if (ppu._scanline() != this->scanline) return;

  if (this->sample_which_nt_counter == 1) {
    this->nametable = ppu._reg().ppuctrl.B;
    this->sample_which_nt_counter--;
  }
  else if (this->sample_which_nt_counter) this->sample_which_nt_counter--;

  auto paint_tile = [&](
    u16 tile_addr,
    uint tl_x, uint tl_y,
    uint palette, // from 0 - 4
    DebugPixelbuffWindow* window
  ) {
    for (uint y = 0; y < 8; y++) {
      u8 lo_bp = ppu._mem().peek(tile_addr + y + 0);
      u8 hi_bp = ppu._mem().peek(tile_addr + y + 8);

      for (uint x = 0; x < 8; x++) {
        uint pixel_type = nth_bit(lo_bp, x) + (nth_bit(hi_bp, x) << 1);

        Color color = ppu.palette[
          ppu._mem().peek(0x3F00 + palette * 4 + pixel_type) % 64
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

  // Nametables
  auto paint_nametable = [&](
    u16 base_addr,
    uint offset_x, uint offset_y
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

      u16 tile_addr = (this->nametable * 0x1000)
                    + ppu._mem().peek(addr.val) * 16;

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
      const u8 attribute = ppu._mem()[nt_base_addr + 0x3C0 + supertile_no];

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

      paint_tile(tile_addr, tl_x, tl_y, palette, name_t);
    }

    // draw scanline
    for (uint x = 0; x < 256; x++) {
      name_t->set_pixel(x + offset_x, this->scanline + offset_y, 0xff0000);
    }
  };

  paint_nametable(0x2000, 0,        0       );
  paint_nametable(0x2400, 256 + 16, 0       );
  paint_nametable(0x2800, 0,        240 + 16);
  paint_nametable(0x2C00, 256 + 16, 240 + 16);


  // nes palette
  for (uint i = 0; i < 64; i++) {
    nes_palette->set_pixel(i % 16, i / 16, ppu.palette[i % 64]);
  }


  // Palette Tables
  // Background palette - from 0x3F00 to 0x3F0F
  // Sprite palette     - from 0x3F10 to 0x3F1F
  for (u16 addr = 0x3F00; addr < 0x3F20; addr++) {
    Color color = ppu.palette[ppu._mem().peek(addr) % 64];
    // printf("0x%04X\n", ppu._mem().peek(addr));
    palette_t->set_pixel(
      (addr % 4) + ((addr >= 0x3F10) ? 5 : 0),
      (addr - ((addr >= 0x3F10) ? 0x3F10 : 0x3F00)) / 4,
      color
    );
  }
}

void PPUDebugModule::output() {
  name_t->render();
  nes_palette->render();
  palette_t->render();
  patt_t->render();
}

uint PPUDebugModule::get_window_id() {
  return SDL_GetWindowID(this->name_t->window);
}

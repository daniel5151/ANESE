#include "widenes.h"

#include <cmath>
#include <cstdio>

#define ALT_SCROLL_VALS true

WideNESModule::WideNESModule(const SDLCommon& sdl_common, Config& config, EmuModule& emu)
: GUIModule(sdl_common, config)
{
  emu.register_frame_callback(WideNESModule::frame_callback, this);

  /*-------------------------------  SDL init  -------------------------------*/

  fprintf(stderr, "[SDL2] Initializing wideNES GUI\n");

  // make window
  this->sdl.window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    256 * this->config.window_scale * 2.25,
    240 * this->config.window_scale * 2.25,
    SDL_WINDOW_RESIZABLE
  );

  // make renderer
  this->sdl.renderer = SDL_CreateRenderer(
    this->sdl.window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  // Allow opacity
  SDL_SetRenderDrawBlendMode(this->sdl.renderer, SDL_BLENDMODE_BLEND);

  // Make base NES screen
  this->nes_screen = new Tile(this->sdl.renderer, 0, 0);
}

WideNESModule::~WideNESModule() {
  for (auto col : this->tiles) {
    for (auto row : col.second) {
      const Tile* tile = row.second;
      delete tile;
    }
  }

  SDL_DestroyTexture(this->nes_screen->texture);

  /*------------------------------  SDL Cleanup  -----------------------------*/

  SDL_DestroyRenderer(this->sdl.renderer);
  SDL_DestroyWindow(this->sdl.window);
}

void WideNESModule::input(const SDL_Event& event) {
  // Update from Mouse
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    this->pan.last_mouse_pos.x = event.button.x;
    this->pan.last_mouse_pos.y = event.button.y;
    this->pan.active = true;
  }

  if (event.type == SDL_MOUSEBUTTONUP) {
    this->pan.active = false;
  }

  if (event.type == SDL_MOUSEMOTION) {
    if (this->pan.active) {
      this->pan.dx += event.motion.x - this->pan.last_mouse_pos.x;
      this->pan.dy += event.motion.y - this->pan.last_mouse_pos.y;
      this->pan.last_mouse_pos.x = event.motion.x;
      this->pan.last_mouse_pos.y = event.motion.y;
    }
  }

  if (event.type == SDL_MOUSEWHEEL) {
    int scroll = event.wheel.y;
    if (scroll > 0) { // scroll up
      while (scroll--) this->pan.zoom *= 1.25;
    } else if(scroll < 0) { // scroll down
      while (scroll++) this->pan.zoom /= 1.25;
    }
  }
}

void WideNESModule::update() {
  // nothing
  // interesting updates happen in the frame callback
}

WideNESModule::Tile::Tile(SDL_Renderer* renderer, int x, int y) {
  this->pos.x = x;
  this->pos.y = y;
  this->texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    256, 240
  );
}

WideNESModule::Tile::~Tile() {
  SDL_DestroyTexture(this->texture);
}

void WideNESModule::frame_callback(void* self, EmuModule& emu) {
  ((WideNESModule*)self)->samplePPU(emu);
}


void WideNESModule::samplePPU(EmuModule& emu) {
  // First, calculate the new scroll position

  PPU::Scroll curr_scroll = emu.nes.get_PPU().get_scroll();

  int dx = curr_scroll.x - this->last_scroll.x;
  int dy = curr_scroll.y - this->last_scroll.y;

  // 255 -> 0 | dx is negative | means we are going right
  // 0 -> 255 | dx is positive | means we are going left
  if (::abs(dx) > 100) {
    if (dx < 0) dx += 256; // going right
    else        dx -= 256; // going left
  }
  // 239 -> 0 | dy is negative | means we are going down
  // 0 -> 239 | dy is positive | means we are going up
  if (::abs(dy) > 100) {
    if (dy < 0) dy += 240; // going down
    else        dy -= 240; // going up
  }

  struct { int x; int y; } oldscroll { this->scroll.x, this->scroll.y };

  this->scroll.x += dx;
  this->scroll.y += dy;
  this->scroll.dx = dx;
  this->scroll.dy = dy;

  this->last_scroll.x = curr_scroll.x;
  this->last_scroll.y = curr_scroll.y;

  // TODO: what the hell?
#if ALT_SCROLL_VALS
  auto true_scroll = oldscroll;
#else
  auto true_scroll = this->scroll;
  (void)oldscroll;
#endif

  // Next, update the textures / framebuffers of the affected tiles

  const int pos_x = ::floor(true_scroll.x / 256.0);
  const int pos_y = ::floor(true_scroll.y / 240.0);

#define mk_tile(x,y)                                                  \
  if ((this->tiles.count(x) && this->tiles[x].count(y)) == false) {   \
    this->tiles[x][y] = new Tile (this->sdl.renderer, x, y);          \
  }

  mk_tile(pos_x + 0, pos_y + 0);
  mk_tile(pos_x + 1, pos_y + 0);
  mk_tile(pos_x + 0, pos_y + 1);
  mk_tile(pos_x + 1, pos_y + 1);
#undef mk_tile


  // save copy of OG screen
  const u8* framebuffer_true;
  emu.nes.get_PPU().getFramebuff(framebuffer_true);
  SDL_UpdateTexture(nes_screen->texture, nullptr, framebuffer_true, 256 * 4);

  // then get a copy of only the bg
  const u8* framebuffer;
  emu.nes.get_PPU().getFramebuffBgr(framebuffer);

#define update_tile(px,py,w,h,dx,dy,sx,sy)                                     \
  for (int x = 0; x < w; x++) {                                                \
    for (int y = 0; y < h; y++) {                                              \
      Tile* tile = this->tiles[px][py];                                        \
      const uint src_px_i = (256 * 4 * (y + sy)) + (4 * (x + sx));             \
      const uint dst_px_i = (256 * 4 * (y + dy)) + (4 * (x + dx));             \
      tile->framebuffer[dst_px_i + 0] = framebuffer[src_px_i + 0];             \
      tile->framebuffer[dst_px_i + 1] = framebuffer[src_px_i + 1];             \
      tile->framebuffer[dst_px_i + 2] = framebuffer[src_px_i + 2];             \
      tile->framebuffer[dst_px_i + 3] = framebuffer[src_px_i + 3];             \
    }                                                                          \
  }                                                                            \
  SDL_UpdateTexture(                                                           \
    this->tiles[px][py]->texture,                                              \
    nullptr,                                                                   \
    this->tiles[px][py]->framebuffer,                                          \
    256 * 4                                                                    \
  );

  const int tl_w = true_scroll.x - pos_x * 256;
  const int tl_h = true_scroll.y - pos_y * 240;
  const int br_w = 256 - tl_w;
  const int br_h = 240 - tl_h;

  // fprintf(stderr, "%d %d | %d %d | %d %d | %d %d\n", pos_x + 0, pos_y + 0, /**/ br_w, br_h, /**/ tl_w, tl_h, /**/ 0,    0);
  // fprintf(stderr, "%d %d | %d %d | %d %d | %d %d\n", pos_x + 1, pos_y + 0, /**/ tl_w, br_h, /**/    0, tl_h, /**/ br_w, 0);
  // fprintf(stderr, "%d %d | %d %d | %d %d | %d %d\n", pos_x + 0, pos_y + 1, /**/ br_w, tl_h, /**/ tl_w,    0, /**/ 0,    br_h);
  // fprintf(stderr, "%d %d | %d %d | %d %d | %d %d\n", pos_x + 1, pos_y + 1, /**/ tl_w, tl_h, /**/    0,    0, /**/ br_w, br_h);
  // fprintf(stderr, "\n");

  // see diagram...
  update_tile(pos_x + 0, pos_y + 0, /**/ br_w, br_h, /**/ tl_w, tl_h, /**/ 0,    0);
  update_tile(pos_x + 1, pos_y + 0, /**/ tl_w, br_h, /**/    0, tl_h, /**/ br_w, 0);
  update_tile(pos_x + 0, pos_y + 1, /**/ br_w, tl_h, /**/ tl_w,    0, /**/ 0,    br_h);
  update_tile(pos_x + 1, pos_y + 1, /**/ tl_w, tl_h, /**/    0,    0, /**/ br_w, br_h);
#undef update_tile
}

void WideNESModule::output() {
  // calculate origin (tile (0,0) / actual NES screen)
  const int nes_w = 256 * this->pan.zoom;
  const int nes_h = 240 * this->pan.zoom;
  int window_w, window_h;
  SDL_GetWindowSize(this->sdl.window, &window_w, &window_h);

  const SDL_Rect origin {
    (window_w - nes_w) / 2 + this->pan.dx,
    (window_h - nes_h) / 2 + this->pan.dy,
    nes_w, nes_h
  };

  SDL_SetRenderDrawColor(this->sdl.renderer, 0, 0, 0, 0xff);
  SDL_RenderClear(this->sdl.renderer);

  // wideNES tiles
  for (auto col : this->tiles) {
    for (auto row : col.second) {
      const Tile* tile = row.second;

      SDL_Rect offset = origin;
      offset.x -= this->pan.zoom * (this->scroll.x - tile->pos.x * 256);
      offset.y -= this->pan.zoom * (this->scroll.y - tile->pos.y * 240);

      // TODO: what the hell? When using oldscroll + these added offsets, there
      // is no banding in some games, but causes it in others.
#if ALT_SCROLL_VALS
      offset.x += this->pan.zoom * this->scroll.dx;
      offset.y += this->pan.zoom * this->scroll.dy;
#endif

      SDL_RenderCopy(this->sdl.renderer, tile->texture, nullptr, &offset);
      SDL_SetRenderDrawColor(this->sdl.renderer, 0, 0xff, 0, 0xff);
      SDL_RenderDrawRect(this->sdl.renderer, &offset);
    }
  }

  // actual NES screen
  SDL_RenderCopy(this->sdl.renderer, nes_screen->texture, nullptr, &origin);
  SDL_SetRenderDrawColor(this->sdl.renderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderDrawRect(this->sdl.renderer, &origin);

  SDL_RenderPresent(this->sdl.renderer);
}

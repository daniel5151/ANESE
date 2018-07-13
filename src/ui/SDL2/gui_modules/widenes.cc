#include "widenes.h"

#include <cmath>
#include <cstdio>

WideNESModule::WideNESModule(SharedState& gui)
: GUIModule(gui)
{
  // register callbacks
  gui.nes.cart_changed_callbacks.add_cb(WideNESModule::cb_mapper_changed, this);
  gui.nes.debug_get.ppu().callbacks.frame_start.add_cb(WideNESModule::cb_frame_start, this);
  gui.nes.debug_get.ppu().callbacks.frame_end.add_cb(WideNESModule::cb_frame_end, this);
  gui.nes.debug_get.ppu().callbacks.scrollx.add_cb(WideNESModule::cb_scrollx_changed, this);
  gui.nes.debug_get.ppu().callbacks.scrolly.add_cb(WideNESModule::cb_scrolly_changed, this);

  /*-------------------------------  SDL init  -------------------------------*/

  fprintf(stderr, "[GUI][wideNES] Initializing...\n");

  // make window
  this->sdl.window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    256 * this->gui.config.window_scale * 2.25,
    240 * this->gui.config.window_scale * 2.25,
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

  this->sdl.inprint = new SDL2_inprint(this->sdl.renderer);

  // Make base NES screen
  this->nes_screen = new Tile(this->sdl.renderer, 0, 0);
}

WideNESModule::~WideNESModule() {
  fprintf(stderr, "[GUI][wideNES] Shutting down...\n");

  for (auto col : this->tiles) {
    for (auto row : col.second) {
      const Tile* tile = row.second;
      delete tile;
    }
  }

  /*------------------------------  SDL Cleanup  -----------------------------*/

  SDL_DestroyTexture(this->nes_screen->texture);

  delete this->sdl.inprint;

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

  if (event.type == SDL_KEYDOWN) {
    bool mod_shift = event.key.keysym.mod & KMOD_SHIFT;
    switch (event.key.keysym.sym) {
    case SDLK_e: this->pad.offset.t += mod_shift ? 1 : 8; break;
    case SDLK_3: this->pad.offset.t -= mod_shift ? 1 : 8; break;
    case SDLK_d: this->pad.offset.b += mod_shift ? 1 : 8; break;
    case SDLK_c: this->pad.offset.b -= mod_shift ? 1 : 8; break;
    case SDLK_s: this->pad.offset.l += mod_shift ? 1 : 8; break;
    case SDLK_a: this->pad.offset.l -= mod_shift ? 1 : 8; break;
    case SDLK_f: this->pad.offset.r += mod_shift ? 1 : 8; break;
    case SDLK_g: this->pad.offset.r -= mod_shift ? 1 : 8; break;
    }
  }
}

void WideNESModule::update() {
  // nothing.
  // updates happen in callbacks
}

/*----------------------------  Tile Definitions  ----------------------------*/

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

/*-------------------------------  Callbacks  --------------------------------*/

void WideNESModule::cb_scrollx_changed(void* self, u8 val) {
  ((WideNESModule*)self)->scrollx_handler(val);
}

void WideNESModule::cb_scrolly_changed(void* self, u8 val) {
  ((WideNESModule*)self)->scrolly_handler(val);
}

void WideNESModule::cb_mapper_changed(void* self, Mapper* mapper) {
  if (mapper && mapper->mapper_number() == 4) {
    Mapper_004* mmc3 = static_cast<Mapper_004*>(mapper);
    mmc3->did_irq_callbacks.add_cb(WideNESModule::cb_mmc3_irq, self);
  }
}

void WideNESModule::cb_mmc3_irq(void* self, Mapper_004* mmc3, bool active) {
  ((WideNESModule*)self)->mmc3_irq_handler(mmc3, active);
}

void WideNESModule::cb_frame_end(void* self) {
  ((WideNESModule*)self)->frame_end_handler();
}

void WideNESModule::cb_frame_start(void* self) {
  ((WideNESModule*)self)->frame_start_handler();
}

/*----------------------------  Callback Handlers  ---------------------------*/

void WideNESModule::scrollx_handler(u8 val) {
  this->curr_scroll.x = val;
}

void WideNESModule::scrolly_handler(u8 val) {
  this->curr_scroll.y = val;
}

void WideNESModule::mmc3_irq_handler(Mapper_004* mmc3, bool active) {
  this->heuristics.mmc3_irq.curr_scroll_pre_irq = this->curr_scroll;

  this->heuristics.mmc3_irq.happened = true;
  this->heuristics.mmc3_irq.on_scanline = active
    ? mmc3->peek_irq_latch()
    : 239; // cancels out later, give 0 padding
}

void WideNESModule::frame_start_handler() {
  // nothing for now...
}

void WideNESModule::frame_end_handler() {
  const PPU& ppu = this->gui.nes.debug_get.ppu();

  // save copy of OG screen
  const u8* framebuffer_true;
  ppu.getFramebuff(framebuffer_true);
  SDL_UpdateTexture(nes_screen->texture, nullptr, framebuffer_true, 256 * 4);

  // update padding / scroll registers (based on heuristics)

  // 1) if the left-column bit is enabled, odds are the game is hiding visual
  //    artifacts, so we can slice that bit off.
  const u8 ppumask = ppu.peek(0x2001);
  bool bgr_left_col_mask_disabled = ppumask & 0x02;
  if (!bgr_left_col_mask_disabled) {
    this->pad.guess.l = 8;
    this->pad.guess.r = 8;
  } else {
    this->pad.guess.l = 0;
    this->pad.guess.r = 0;
  }

  // 2) Mappers sometimes use a scanline IRQ to split the screen, many times for
  // making a static status bar at the bottom of the screen (kirby, smb3)
  // TODO: make this more robust, i.e: get rid of false positives (Megaman IV)
  if (this->heuristics.mmc3_irq.happened) {
    this->pad.guess.b = 239 - this->heuristics.mmc3_irq.on_scanline;
    // depending on if the menu is at the top / bottom of the screen, different
    // scroll values should be used
    if (this->heuristics.mmc3_irq.on_scanline < 240 / 2) {
      // top of screen
      this->curr_scroll = this->curr_scroll; // stays the same
    } else {
      // bottom of screen
      this->curr_scroll = this->heuristics.mmc3_irq.curr_scroll_pre_irq;
    }
  }
  this->heuristics.mmc3_irq.happened = false; // reset

  // 3) vertically scrolling + vertical mirroring usually leads to artifacting
  //    at the top of the screen
  // ??? not sure about this one...

  // calculate final padding
  this->pad.total.l = this->pad.guess.l + this->pad.offset.l;
  this->pad.total.r = this->pad.guess.r + this->pad.offset.r;
  this->pad.total.t = this->pad.guess.t + this->pad.offset.t;
  this->pad.total.b = this->pad.guess.b + this->pad.offset.b;

  // calculate the new scroll position

  int dx = this->curr_scroll.x - this->last_scroll.x;
  int dy = this->curr_scroll.y - this->last_scroll.y;

  const int fuzz = 10;
  const int thresh_w = (256 - this->pad.total.l - this->pad.total.r) - fuzz;
  const int thresh_h = (240 - this->pad.total.t - this->pad.total.b) - fuzz;

  // 255 -> 0 | dx is negative | means we are going right
  // 0 -> 255 | dx is positive | means we are going left
  if (::abs(dx) > thresh_w) {
    if (dx < 0) dx += 256; // going right
    else        dx -= 256; // going left
  }
  // 239 -> 0 | dy is negative | means we are going down
  // 0 -> 239 | dy is positive | means we are going up
  if (::abs(dy) > thresh_h) {
    if (dy < 0) dy += 240; // going down
    else        dy -= 240; // going up
  }

  this->scroll.x += dx;
  this->scroll.y += dy;
  this->scroll.dx = dx;
  this->scroll.dy = dy;

  this->last_scroll.x = this->curr_scroll.x;
  this->last_scroll.y = this->curr_scroll.y;

  // static int last_sample_x = 0;
  // static int last_sample_y = 0;
  // last_sample_x += dx;
  // last_sample_y += dy;

  // static bool do_sample = true;
  // if (last_sample_x >=  255) { last_sample_x -= 255; do_sample = true; }
  // if (last_sample_x <= -255) { last_sample_x += 255; do_sample = true; }
  // if (last_sample_y >=  239) { last_sample_y -= 239; do_sample = true; }
  // if (last_sample_y <= -239) { last_sample_y += 239; do_sample = true; }

  // if (!do_sample) return;
  // do_sample = false;

  // Next, update the textures / framebuffers of the affected tiles

  // use the background framebuffer (sprites leave artifacts)
  const u8* framebuffer;
  ppu.getFramebuffBgr(framebuffer);

#define update_tile(px,py,w,h,dx,dy,sx,sy)                                \
  if ((this->tiles.count(px) && this->tiles[px].count(py)) == false) {    \
    this->tiles[px][py] = new Tile (this->sdl.renderer, px, py);          \
  }                                                                       \
  for (int x = 0; x < w; x++) {                                           \
    for (int y = 0; y < h; y++) {                                         \
      if (                                                                \
        x + sx > 255 - this->pad.total.r || x + sx < this->pad.total.l || \
        y + sy > 239 - this->pad.total.b || y + sy < this->pad.total.t    \
      ) continue;                                                         \
      Tile* tile = this->tiles[px][py];                                   \
      const uint src_px_i = (256 * 4 * (y + sy)) + (4 * (x + sx));        \
      const uint dst_px_i = (256 * 4 * (y + dy)) + (4 * (x + dx));        \
      tile->framebuffer[dst_px_i + 0] = framebuffer[src_px_i + 0];        \
      tile->framebuffer[dst_px_i + 1] = framebuffer[src_px_i + 1];        \
      tile->framebuffer[dst_px_i + 2] = framebuffer[src_px_i + 2];        \
      tile->framebuffer[dst_px_i + 3] = framebuffer[src_px_i + 3];        \
    }                                                                     \
  }                                                                       \
  SDL_UpdateTexture(                                                      \
    this->tiles[px][py]->texture,                                         \
    nullptr,                                                              \
    this->tiles[px][py]->framebuffer,                                     \
    256 * 4                                                               \
  );

  const int pos_x = ::floor(this->scroll.x / 256.0);
  const int pos_y = ::floor(this->scroll.y / 240.0);
  const int tl_w = this->scroll.x - pos_x * 256;
  const int tl_h = this->scroll.y - pos_y * 240;
  const int br_w = 256 - tl_w;
  const int br_h = 240 - tl_h;

  // see diagram...
  update_tile(pos_x + 0, pos_y + 0, /**/ br_w, br_h, /**/ tl_w, tl_h, /**/ 0,    0);
  if (this->scroll.x % 256 != 0) {
    update_tile(pos_x + 1, pos_y + 0, /**/ tl_w, br_h, /**/    0, tl_h, /**/ br_w, 0);
  }
  if (this->scroll.y % 240 != 0) {
    update_tile(pos_x + 0, pos_y + 1, /**/ br_w, tl_h, /**/ tl_w,    0, /**/ 0,    br_h);
  }
  if (this->scroll.x % 256 != 0 && this->scroll.y % 240 != 0) {
    update_tile(pos_x + 1, pos_y + 1, /**/ tl_w, tl_h, /**/    0,    0, /**/ br_w, br_h);
  }
#undef update_tile
}

/*---------------------------------  Output  ---------------------------------*/

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

      SDL_RenderCopy(this->sdl.renderer, tile->texture, nullptr, &offset);
      SDL_SetRenderDrawColor(this->sdl.renderer, 0x60, 0x60, 0x60, 0xff);
      SDL_RenderDrawRect(this->sdl.renderer, &offset);

      this->sdl.inprint->set_color(0xff0000);
      char buf [64];
      sprintf(buf, "(%d, %d)", tile->pos.x, tile->pos.y);
      this->sdl.inprint->print(buf, offset.x + 8, offset.y + 8);
    }
  }

  // actual NES screen

  // draw most of the screen (minus left-col) transparently
  SDL_Rect col_origin = origin;
  col_origin.x += this->pan.zoom * (this->pad.total.l);
  col_origin.w -= this->pan.zoom * (this->pad.total.l + this->pad.total.r);

  SDL_Rect col_clip;
  col_clip.x = (this->pad.total.l);
  col_clip.y = 0;
  col_clip.w = 256 - (this->pad.total.l + this->pad.total.r);
  col_clip.h = 240;
  SDL_SetTextureBlendMode(nes_screen->texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(nes_screen->texture, 100);
  SDL_RenderCopy(this->sdl.renderer, nes_screen->texture, &col_clip, &col_origin);

  // draw clipped part of the screen with no transparency
  SDL_Rect padded_origin = origin;
  padded_origin.x += this->pan.zoom * (this->pad.total.l);
  padded_origin.y += this->pan.zoom * (this->pad.total.t);
  padded_origin.w -= this->pan.zoom * (this->pad.total.l + this->pad.total.r);
  padded_origin.h -= this->pan.zoom * (this->pad.total.t + this->pad.total.b);

  SDL_Rect padded_clip;
  padded_clip.x = (this->pad.total.l);
  padded_clip.y = (this->pad.total.t);
  padded_clip.w = 256 - (this->pad.total.l + this->pad.total.r);
  padded_clip.h = 240 - (this->pad.total.t + this->pad.total.b);

  SDL_SetTextureAlphaMod(nes_screen->texture, 255);
  SDL_RenderCopy(this->sdl.renderer, nes_screen->texture, &padded_clip, &padded_origin);

  // full-screen box
  // SDL_SetRenderDrawColor(this->sdl.renderer, 0xff, 0, 0, 0xff);
  // SDL_RenderDrawRect(this->sdl.renderer, &origin);

  // clipped-screen box
  SDL_SetRenderDrawColor(this->sdl.renderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderDrawRect(this->sdl.renderer, &padded_origin);

  // debug values
  this->sdl.inprint->set_color(0xff0000);
  char buf [128];
  sprintf(buf,
    "total scroll: %-3d %-3d\n"
    " last scroll: %-3d %-3d\n"
    "       dx/dy: %-3d %-3d\n",
    this->scroll.x, this->scroll.y,
    this->last_scroll.x, this->last_scroll.y,
    this->scroll.dx, this->scroll.dy
  );
  this->sdl.inprint->print(buf, 8, 8);

  SDL_RenderPresent(this->sdl.renderer);
}

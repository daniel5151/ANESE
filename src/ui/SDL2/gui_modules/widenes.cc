#include "widenes.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <stb_image_write.h>
#include <stb_image.h>
#include <cute_files.h>

WideNESModule::WideNESModule(SharedState& gui)
: GUIModule(gui)
{
  // register callbacks
  gui.nes._callbacks.savestate_loaded.add_cb(WideNESModule::cb_savestate_loaded, this);
  gui.nes._callbacks.cart_changed.add_cb(WideNESModule::cb_mapper_changed, this);
  gui.nes._ppu()._callbacks.frame_end.add_cb(WideNESModule::cb_ppu_frame_end, this);
  gui.nes._ppu()._callbacks.write_start.add_cb(WideNESModule::cb_ppu_write_start, this);
  gui.nes._ppu()._callbacks.write_end.add_cb(WideNESModule::cb_ppu_write_end, this);

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

  this->nes_screen = SDL_CreateTexture(
    this->sdl.renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    256, 240
  );

  // Allow opacity
  SDL_SetRenderDrawBlendMode(this->sdl.renderer, SDL_BLENDMODE_BLEND);

  this->sdl.inprint = new SDL2_inprint(this->sdl.renderer);

  // make menu submodule
  this->menu_submodule = new MenuSubModule(gui, this->sdl.window, this->sdl.renderer);
}

WideNESModule::~WideNESModule() {
  fprintf(stderr, "[GUI][wideNES] Shutting down...\n");

  this->save_scenes();
  this->clear_scenes();

  /*------------------------------  SDL Cleanup  -----------------------------*/

  SDL_DestroyTexture(this->nes_screen);

  delete this->sdl.inprint;

  SDL_DestroyRenderer(this->sdl.renderer);
  SDL_DestroyWindow(this->sdl.window);
}

void WideNESModule::clear_scenes() {
  // delete all saved-tiles
  for (auto p : this->scenes) {
    Scene& scene = p.second;
    for (auto col : scene.tiles) {
      for (auto row : col.second) {
        if (!row.second) continue;
        delete row.second;
      }
    }
  }
  this->scenes.clear();
}

#include "../fs/util.h"

void WideNESModule::save_scenes() {
  ANESE_fs::util::create_directory(this->scenes_path.c_str());

  FILE* hashes = fopen((this->scenes_path + "/hashes.txt").c_str(), "wb");
  for (auto p : this->scenes) {
    int scene_id = p.first;
    Scene& scene = p.second;

    // save all tiles
    for (auto col : scene.tiles) {
      for (auto row : col.second) {
        if (!row.second) continue;
        Tile* tile = row.second;

        u8* png_fb = new u8 [256 * 240 * 4];
        for (uint i = 0; i < 256 * 240; i++) {
          png_fb[i * 4 + 0] = tile->fb_new[i * 4 + 2];
          png_fb[i * 4 + 1] = tile->fb_new[i * 4 + 1];
          png_fb[i * 4 + 2] = tile->fb_new[i * 4 + 0];
          png_fb[i * 4 + 3] = tile->fb_new[i * 4 + 3];
        }

        char buf [256];
        sprintf(buf, "%s/scene_%d_%d_%d.png", this->scenes_path.c_str(),
          scene_id, tile->x, tile->y);

        stbi_write_png(buf, 256, 240, 4, png_fb, 256 * 4);

        delete[] png_fb;
      }
    }

    // save hashes
    for (auto p : scene.scroll_hash) {
      int hash = p.first;
      auto data = p.second;

      fprintf(hashes, "%d : %d | %d %d | %d %d\n",
        scene_id,
        hash,
        data.scroll.x, data.scroll.y,
        data.last_scroll.x, data.last_scroll.y);
    }
  }
  fclose(hashes);
}

#include <climits>

void WideNESModule::load_scenes() {
  int max_scene_id = INT_MIN;

  this->scenes_path = this->gui.current_rom_file + "_scenes";

  cf_dir_t dir;
  int success = cf_dir_open(&dir, this->scenes_path.c_str());
  if (!success) return;

  while (dir.has_next) {
    cf_file_t file;
    cf_read_file(&dir, &file);

    std::string fullpath = this->scenes_path + "/" + file.name;

    if (!strcmp(file.name, "hashes.txt")) {
      FILE* hashes = fopen(fullpath.c_str(), "r");
      int scene_id, hash, scroll_x, scroll_y, last_x, last_y;

      while(fscanf(hashes, "%d : %d | %d %d | %d %d\n",
        &scene_id,
        &hash,
        &scroll_x, &scroll_y,
        &last_x, &last_y) != EOF
      ) {
        this->scenes[scene_id].scroll_hash[hash].scroll.x = scroll_x;
        this->scenes[scene_id].scroll_hash[hash].scroll.y = scroll_y;
        this->scenes[scene_id].scroll_hash[hash].last_scroll.x = last_x;
        this->scenes[scene_id].scroll_hash[hash].last_scroll.y = last_y;
      };
    }

    if (!strcmp(file.ext, "png")) {
      // janky way to extract info from string lel
      int i = 0;
      int a [3];
      char* p = file.name;
      while (*p) {
        if (::isdigit(*p) || *p == '-') {
          a[i++] = ::strtol(p, &p, 10);
        } else {
          p++;
        }
      }
      int scene_id = a[0];
      int tx = a[1];
      int ty = a[2];

      max_scene_id = std::max(max_scene_id, scene_id);

      Tile* tile = this->scenes[scene_id].tiles[tx][ty];
      if (!tile) {
        tile = new WideNESModule::Tile (this->sdl.renderer, tx, ty);
        this->scenes[scene_id].tiles[tx][ty] = tile;
      }

      // Load PNG using stb_image
      int x, y, comp;
      u8* data = stbi_load(fullpath.c_str(), &x, &y, &comp, 4);
      if (!data) {
        fprintf(stderr, "%s\n", stbi_failure_reason());
        assert(false);
      }

      // a long, long time ago in development, I decided to use
      // SDL_PIXELFORMAT_ARGB8888 as the standard pixel ordering in ANESE.
      // Why? No reason. Totally arbitrary.
      // As long as it remained consistent, it didn't bother me.
      // Except stb_image doesn't support anything but RGBA, so instead of this
      // being a straigt memcpy, we need to switch some bytes around...
      for (uint i = 0; i < 256 * 240; i++) {
        tile->fb_new[i * 4 + 0] = data[i * 4 + 2];
        tile->fb_new[i * 4 + 1] = data[i * 4 + 1];
        tile->fb_new[i * 4 + 2] = data[i * 4 + 0];
        tile->fb_new[i * 4 + 3] = data[i * 4 + 3];
      }
      SDL_UpdateTexture(tile->texture_curr, nullptr, tile->fb_new, 256 * 4);

      // no need to update tile->fb (?)

      stbi_image_free(data);
    }

    cf_dir_next(&dir);
  }

  // this prevents new scene_ids from overlapping with loaded one.
  // it's not great, but it gets the job done.
  this->gen_scene_id.next_id = max_scene_id + 1;

  cf_dir_close(&dir);
}

void WideNESModule::input(const SDL_Event& event) {
  this->menu_submodule->input(event);
  if (this->gui.status.in_menu) return;

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

  bool forward_to_emu_module = true;

  if (event.type == SDL_KEYDOWN) {
    bool mod_shift = event.key.keysym.mod & KMOD_SHIFT;
    switch (event.key.keysym.sym) {
    case SDLK_ESCAPE: forward_to_emu_module = false; break;
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

  if (forward_to_emu_module)
    this->gui.modules.at("emu")->input(event);
}

void WideNESModule::update() {
  this->menu_submodule->update();
  // aside from that, nothing.
  // updates happen in callbacks
}

/*----------------------------  Tile Definitions  ----------------------------*/

WideNESModule::Tile::Tile(SDL_Renderer* renderer, int x, int y) {
  this->x = x;
  this->y = y;
  this->texture_done = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    256, 240
  );
  this->texture_curr = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    256, 240
  );

  SDL_SetTextureBlendMode(this->texture_done, SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(this->texture_curr, SDL_BLENDMODE_BLEND);
}

WideNESModule::Tile::~Tile() {
  SDL_DestroyTexture(this->texture_done);
  SDL_DestroyTexture(this->texture_curr);
}

/*------------------------------  Frame-Hashes  ------------------------------*/

int WideNESModule::frame_hash_unique(const u8* nescolor_fb) const {
  uint hash = 0;
  for (int x = this->pad.total.l; x < 256 - this->pad.total.r; x++) {
    // hash individual columns
    uint chash = 0;
    for (int y = this->pad.total.t; y < 240 - this->pad.total.b; y++)
      chash = (chash + (324723947 + nescolor_fb[y * 256 + x] * 2)) ^ 93485734985;

    // use column hashes to make a full-frame hash
    hash = (hash + (324723947 + chash)) ^ 93485734985;
  }

  return hash;
}

int WideNESModule::frame_hash_percept(const u8* nescolor_fb) const {
  // TODO: make this less bad lol

  int hash = 0;
  for (int x = this->pad.total.l; x < 256 - this->pad.total.r; x++) {
    for (int y = this->pad.total.t; y < 240 - this->pad.total.b; y++) {
      hash += nescolor_fb[y * 256 + x];
    }
  }

  return hash;
}

/*-------------------------------  Callbacks  --------------------------------*/

void WideNESModule::cb_ppu_write_start(void* self, u16 addr, u8 val) {
  ((WideNESModule*)self)->ppu_write_start_handler(addr, val);
}

void WideNESModule::cb_ppu_write_end(void* self, u16 addr, u8 val) {
  ((WideNESModule*)self)->ppu_write_end_handler(addr, val);
}

void WideNESModule::cb_savestate_loaded(void* self) {
  ((WideNESModule*)self)->savestate_loaded_handler();
}

void WideNESModule::cb_mapper_changed(void* self, Mapper* mapper) {
  if (mapper) ((WideNESModule*)self)->load_scenes();

  if (mapper && mapper->mapper_number() == 4) {
    Mapper_004* mmc3 = static_cast<Mapper_004*>(mapper);
    mmc3->_did_irq_callbacks.add_cb(WideNESModule::cb_mmc3_irq, self);
  }
}

void WideNESModule::cb_mmc3_irq(void* self, Mapper_004* mmc3, bool irq_enabled) {
  ((WideNESModule*)self)->mmc3_irq_handler(mmc3, irq_enabled);
}

void WideNESModule::cb_ppu_frame_end(void* self) {
  ((WideNESModule*)self)->ppu_frame_end_handler();
}

/*----------------------------  Callback Handlers  ---------------------------*/

#include "nes/ppu/ppu.h"

void WideNESModule::savestate_loaded_handler() {
  fprintf(stderr, "savestate loaded...\n");

  this->scroll = {0,0,0,0};
  this->last_scroll = {0,0};

  this->scene_id = this->gen_scene_id();
  this->stitch_timer = 10;
}

void WideNESModule::ppu_write_start_handler(u16 addr, u8 val) {
  (void)addr;
  (void)val;
  // nothing... for now
}

void WideNESModule::ppu_write_end_handler(u16 addr, u8 val) {
  const PPU& ppu = this->gui.nes._ppu();
  const PPU::Registers& ppu_reg = ppu._reg();

  using namespace PPURegisters;

  switch (addr) {
  case PPUSCROLL: {
    this->h.ppuscroll.did_change = true;
    this->h.ppuscroll.changed.while_rendering = ppu_reg.ppumask.is_rendering;
    this->h.ppuscroll.changed.on_scanline = ppu._scanline();
    this->h.ppuscroll.changed.on_scancycle = ppu._scancycle();

    if (ppu_reg.scroll_latch == 1) this->h.ppuscroll.new_scroll.x = val;
    if (ppu_reg.scroll_latch == 0) this->h.ppuscroll.new_scroll.y = val;
  } break;
  case PPUADDR: {
    this->h.ppuaddr.did_change = true;
    this->h.ppuaddr.changed.while_rendering = ppu_reg.ppumask.is_rendering;
    this->h.ppuaddr.changed.on_scanline = ppu._scanline();
    this->h.ppuaddr.changed.on_scancycle = ppu._scancycle();

    if (ppu_reg.scroll_latch == 1) this->h.ppuaddr.new_scroll.x = 8 * ppu_reg.t.coarse_x;
    if (ppu_reg.scroll_latch == 0) this->h.ppuaddr.new_scroll.y = 8 * ppu_reg.t.coarse_y;
  } break;
  }
}

void WideNESModule::mmc3_irq_handler(Mapper_004* mmc3, bool irq_enabled) {
  this->h.mmc3_irq.active = true;

  // taking value from ppuscroll
  // TODO: investigate if some games use ppuaddr value instead?
  this->h.mmc3_irq.scroll_pre_irq = this->h.ppuscroll.new_scroll;
  this->h.mmc3_irq.on_scanline = irq_enabled
    ? mmc3->peek_irq_latch()
    : 239; // cancels out later, give 0 padding
}

void WideNESModule::fix_padding() {
  const PPU& ppu = this->gui.nes._ppu();

  // - if the left-column bit is enabled, odds are the game is hiding visual
  // artifacts, so we can slice that bit off.
  this->pad.guess.l = ppu._reg().ppumask.m ? 0 : 8;

  // - If the PPUSCROLL register changes mid-frame, note where that happened,
  // since that probably indicates a status-bar?
  if (this->h.ppuscroll.did_change) {
    if (this->h.ppuscroll.changed.on_scanline < 241 &&
        this->h.ppuscroll.changed.while_rendering) {
      this->h.ppuaddr.active = true;

      this->h.ppuscroll.cut_scanline = this->h.ppuscroll.changed.on_scanline;
      if (this->h.ppuscroll.changed.on_scancycle > 256 / 3)
        this->h.ppuscroll.cut_scanline++;

      // lob off the chunk of the screen not being scrolled...
      if (this->h.ppuscroll.cut_scanline < 240 / 2) {
        // top of screen
        this->pad.guess.t = this->h.ppuscroll.cut_scanline;
      } else {
        // bottom of screen
        this->pad.guess.b = 239 - this->h.ppuscroll.cut_scanline;
      }
    }
  }

  // - most games use PPUSCROLL to scroll mid-frame, but some games (like TLOZ)
  // are sneakybois and use PPUADDR instead
  if (this->h.ppuaddr.did_change) {
    // ppuaddr is written to a lot, but we only care about writes that occur
    // mid-frame, since those are the one that lead to scrolling / status bars.
    if (this->h.ppuaddr.changed.on_scanline < 241 &&
        this->h.ppuaddr.changed.while_rendering) {
      this->h.ppuaddr.active = true;

      this->h.ppuaddr.cut_scanline = this->h.ppuaddr.changed.on_scanline;
      if (this->h.ppuaddr.changed.on_scancycle > 256 / 3)
        this->h.ppuaddr.cut_scanline++;

      // lob off the chunk of the screen not being scrolled...
      if (this->h.ppuaddr.cut_scanline < 240 / 2) {
        // top of screen
        this->pad.guess.t = this->h.ppuaddr.cut_scanline;
      } else {
        // bottom of screen
        this->pad.guess.b = 239 - this->h.ppuaddr.cut_scanline;
      }
    }
  }

  // - Mappers sometimes use a scanline IRQ to split the screen, many times for
  // making a static status bar at the bottom of the screen (kirby, smb3)
  // TODO: make this more robust, i.e: get rid of false positives (Megaman IV)
  if (this->h.mmc3_irq.active) {
    // depending on if the menu is at the top / bottom of the screen, different
    // scroll values should be used, and different parts of the screen should be
    // cut-off
    if (this->h.mmc3_irq.on_scanline < 240 / 2) {
      // top of screen
      this->pad.guess.t = this->h.mmc3_irq.on_scanline;
      // this->curr_scroll stays the same
    } else {
      // bottom of screen
      this->pad.guess.b = 239 - this->h.mmc3_irq.on_scanline;
    }
  }

  // - vertically scrolling + vertical mirroring usually leads to artifacting
  // at the top of the screen, so temporarilly shift the padding over...
  // TODO: implement me
  // TODO: implement inverse
}

void WideNESModule::fix_scrolling() {
  // - the OG - set current scroll values based off of the PPUSCROLL register
  this->curr_scroll.x = this->h.ppuscroll.new_scroll.x;
  this->curr_scroll.y = this->h.ppuscroll.new_scroll.y;

  // - Zelda-like scrolling
  if (this->h.ppuaddr.active) {
    this->curr_scroll.y = this->h.ppuaddr.new_scroll.y;
    // this->curr_scroll.x = this->h.ppuaddr.new_scroll.x;
    // TODO: ^ breaks zelda for some reason... investigate
  }

  // - MMC3 screen-split
  if (this->h.mmc3_irq.active) {
    // if the status-bar is at the bottom of the screen, the current scroll vals
    // are not repreesntative of the game-scene. instead, the values that should
    // be used are those from _before_ the IRQ
    if (this->h.mmc3_irq.on_scanline > 240 / 2) {
      this->curr_scroll = this->h.mmc3_irq.scroll_pre_irq;
    }
  }
}

void WideNESModule::ppu_frame_end_handler() {
  const PPU& ppu = this->gui.nes._ppu();

  const u8* framebuffer;

  // save copy of OG screen
  ppu.getFramebuff(&framebuffer);
  SDL_UpdateTexture(this->nes_screen, nullptr, framebuffer, 256 * 4);

  // but use the background framebuffer for all other calculations
  ppu.getFramebuffBgr(&framebuffer);

  /*-----------------------------  Heuristics  -----------------------------*/

  this->fix_padding();
  this->fix_scrolling();

  /*------------------  Padding / Scrolling Calculations  ------------------*/

  // calculate final padding
  this->pad.total.l = std::max(0, this->pad.guess.l + this->pad.offset.l);
  this->pad.total.r = std::max(0, this->pad.guess.r + this->pad.offset.r);
  this->pad.total.t = std::max(0, this->pad.guess.t + this->pad.offset.t);
  this->pad.total.b = std::max(0, this->pad.guess.b + this->pad.offset.b);

  // calculate the new scroll position

  int scroll_dx = this->curr_scroll.x - this->last_scroll.x;
  int scroll_dy = this->curr_scroll.y - this->last_scroll.y;

  const int fuzz = 10;
  const int thresh_w = (256 - this->pad.total.l - this->pad.total.r) - fuzz;
  const int thresh_h = (240 - this->pad.total.t - this->pad.total.b) - fuzz;

  // 255 -> 0 | scroll_dx is negative | means we are going right
  // 0 -> 255 | scroll_dx is positive | means we are going left
  if (::abs(scroll_dx) > thresh_w) {
    if (scroll_dx < 0) scroll_dx += 256; // going right
    else               scroll_dx -= 256; // going left
  }
  // 239 -> 0 | scroll_dy is negative | means we are going down
  // 0 -> 239 | scroll_dy is positive | means we are going up
  if (::abs(scroll_dy) > thresh_h) {
    if (scroll_dy < 0) scroll_dy += 240; // going down
    else               scroll_dy -= 240; // going up
  }

  // Zelda scroll heuristic
  // not entirely sure why zelda does this jumping behavior, but this seems to
  // fix it... whatever works right?
  if (this->h.ppuaddr.active && this->h.mmc3_irq.active == false) {
    if (::abs(scroll_dy) >= (int)this->h.ppuaddr.cut_scanline) {
      scroll_dy = 0;
    }
  }

  this->scroll.x += scroll_dx;
  this->scroll.y += scroll_dy;
  this->scroll.dx = scroll_dx;
  this->scroll.dy = scroll_dy;

  this->last_scroll.x = this->curr_scroll.x;
  this->last_scroll.y = this->curr_scroll.y;

  /*----------------------------  Frame Hashing  ---------------------------*/

  // Instead of using the full RGB framebuffer, we use the smaller raw NES color
  // framebuffer, since it's less data to hash
  const u8* nes_color_framebuffer;
  ppu.getFramebuffNESColorBgr(&nes_color_framebuffer);

  // We want to calculate 2 different hashes for each frame
  // 1) a true, unique hash of the frame
  //     - used to detect identical-frames, for re-entering scenes
  // 2) a perceptual hash of the frame
  //     - used to detect scene-changes

  int hash  = frame_hash_unique(nes_color_framebuffer);
  int phash = frame_hash_percept(nes_color_framebuffer);

  // printf("%d\n", hash);
  // printf("%d\n", phash);

  // Why do we have this "stitch timer"?
  // TL;DR: re-entry into the same scene isn't always obvious from one frame
  //
  // Consider SMB2 level 1.
  //
  // There is a point where you enter a door (scene 1), do some stuff (scene 2),
  // and exit _back into_ scene 1! This shouldn't be a problem, except for the
  // fact that there are animated elements in the background...
  //
  // The tricky about animated backgrounds is that there is a good chance the
  // that hash taken when entering a scene won't match-up exactly with any
  // hashes of an existing scene, unless you there was a frame in the original
  // scene that has the _exact same animation cycle_
  // To compensate for this, a couple of frames "grace period" is enabled
  // upon scene transition where every frame has it's hash checked, and tries
  // to stitch together scenes.
  //
  // Consider The Legend of Zelda
  //
  // There are times when zelda "fades in" in such a way that the perceptual
  // hash doesn't detect the fade, and as such, doesn't trigger a scene change.
  // Once it fades in though, the frame matches perfectly with a previously seen
  // frame, and everything works great.
  //
  // This brings up a question though: why not do this stitching on _every_
  // frame? Why even have a grace period in the first place?
  // Well, because some games *cough* Metroid *cough* like to reuse the _exact
  // same screens_ in different parts of the level, which means having a
  // contiuous check would keep "clipping back" to earlier parts of the map
  if (this->stitch_timer) {
    this->stitch_timer--;

    // check if scene already exists
    // not that slow tbh, basically linear wrt #scenes (since unordered_map is
    // O(1) lookup)
    Scene* match = nullptr;
    int match_id = -1;
    int match_hash = -1;
    for (auto& p : this->scenes) {
      int scene_id = p.first;
      Scene& scene = p.second;

      if (scene_id == this->scene_id) continue;

      if (scene.scroll_hash.find(hash) != scene.scroll_hash.end()) {
        fprintf(stderr, "found match in existing scene: %d - %d %d\n",
          scene_id,
          scene.scroll_hash[hash].scroll.x,
          scene.scroll_hash[hash].scroll.y);
        match = &scene;
        match_id = scene_id;
        match_hash = hash;
      }
    }

    if (match) {
      // Discard anything we've learned so far...
      for (auto col : this->scenes[this->scene_id].tiles) {
        for (auto row : col.second) {
          const Tile* tile = row.second;
          delete tile;
        }
      }
      this->scenes.erase(this->scene_id);

      // Set the scene
      this->scene_id = match_id;
      this->scroll = match->scroll_hash[match_hash].scroll;
      this->last_scroll = match->scroll_hash[match_hash].last_scroll;

      this->stitch_timer = 0; // clear the stitch timer
    }
  }

  // TODO: find more robust method of detecting difference phash_dx threshold
  // current value is pulled out of thing air (based off observation)
  // it works alright for games that fade to black, but it fails when games
  // fade to another straight-color
  int phash_dx = ::abs(this->phash.last - phash);
  // fprintf(stderr, "%d\n", phash_dx);
  if (phash_dx > 100000) {
    this->stitch_timer = 60 * 3;

    // fprintf(stderr, "\nSCENE CHANGE:\n"
    //   "  avg: %d\n"
    //   "  min: %d\n"
    //   "  max: %d\n"
    //   "\n",
    //   this->phash.total / 60,
    //   this->phash.min,
    //   this->phash.max);

    // reset phash
    this->phash.vals_i = 0;
    this->phash.total = phash * 60;
    this->phash.min = INT_MAX;
    this->phash.max = INT_MIN;

    // reset heuristics
    memset((char*)&this->h, 0, sizeof this->h);

    // check if scene already exists
    // not that slow tbh, basically linear wrt #scenes (since unordered_map is
    // O(1) lookup)
    Scene* match = nullptr;
    int match_id = -1;
    int match_hash = -1;
    for (auto& p : this->scenes) {
      int scene_id = p.first;
      Scene& scene = p.second;

      if (scene.scroll_hash.find(hash) != scene.scroll_hash.end()) {
        fprintf(stderr, "found match in existing scene: %d - %d %d\n",
          scene_id,
          scene.scroll_hash[hash].scroll.x,
          scene.scroll_hash[hash].scroll.y);
        match = &scene;
        match_id = scene_id;
        match_hash = hash;
      }
    }

    if (match) {
      this->scene_id = match_id; // set current scene to that scene
      this->scroll = match->scroll_hash[match_hash].scroll;
      this->last_scroll = match->scroll_hash[match_hash].last_scroll;

      // TODO: save padding guesses with scene data
      memset(&this->pad.guess, 0, sizeof this->pad.guess);
    } else { // new scene
      this->scene_id = this->gen_scene_id(); // fresh scene id
      this->scroll = {0,0,0,0};
      this->last_scroll = {0,0};

      memset(&this->pad.guess, 0, sizeof this->pad.guess);
    }
  }

  // TEMP: check for duplicate uniqe hashes
  auto& scroll_hashh = this->scenes[this->scene_id].scroll_hash;
  if (scroll_hashh.find(hash) != scroll_hashh.end()) {
    if (scroll_hashh[hash].scroll.x != this->scroll.x ||
        scroll_hashh[hash].scroll.y != this->scroll.y) {
      fprintf(stderr, " hash collision | %3d %3d | %3d %3d\n",
        scroll_hashh[hash].scroll.x, scroll_hashh[hash].scroll.y,
        this->scroll.x, this->scroll.y);
    }
  }

  // update scroll-hash map
  auto& scroll_hash = this->scenes[this->scene_id].scroll_hash;
  scroll_hash[hash].scroll = this->scroll;
  scroll_hash[hash].last_scroll = this->last_scroll;

  // update perceptual hash values
  this->phash.last = phash;

  this->phash.min = this->phash.min < phash ? this->phash.min : phash;
  this->phash.max = this->phash.max > phash ? this->phash.max : phash;

  this->phash.total -= this->phash.vals[this->phash.vals_i];
  this->phash.total += phash;
  this->phash.vals[this->phash.vals_i] = phash;
  this->phash.vals_i = (this->phash.vals_i + 1) % 60;

  /*----------------------------  Tile Updates  ----------------------------*/

  // 1) For every source-pixel on the NES screen, update the associated pixel
  //    within the appropriate tile
  // - also keep track of how many pixels are written to every 16x16 segment of
  //   the screen, as we want to sample on a 16x16 tile basis. This prevents
  //   smearing on animated tiles

  // sx/sy = soruce pixel (from NES screen)
  for (int sx = this->pad.total.l; sx < 256 - this->pad.total.r; sx++) {
    for (int sy = this->pad.total.t; sy < 240 - this->pad.total.b; sy++) {
      // tx/ty = "big tile" that sx/sy is currently in
      const int tx = ::floor((this->scroll.x + sx) / 256.0);
      const int ty = ::floor((this->scroll.y + sy) / 240.0);

      Tile* tile = this->scenes[this->scene_id].tiles[tx][ty];
      if (!tile) {
        tile = new WideNESModule::Tile (this->sdl.renderer, tx, ty);
        this->scenes[this->scene_id].tiles[tx][ty] = tile;
      }

      // dx/dy = destination pixel within big-tile
      const uint dx = (this->scroll.x - tx * 256) + sx;
      const uint dy = (this->scroll.y - ty * 240) + sy;

      // bx/by = block index dx/dy is currently in
      // i.e: NES screen is made of 16x16 blocks, 16 wide 15 tall
      const int bx = ::floor(dx / 16.0);
      const int by = ::floor(dy / 16.0);

      // There are 2 ways to recrod the screen: sample the _first_ thing that
      //  appears, or sample the _last_ thing that appears.
      // Sampling the _first_ thing that appears is cool beacause the generated
      //  scene is representative of the "initial state" of the map
      // Sampling the _last_ thing that appears is cool because that matches the
      //  progress you (as a player) have made.
      // There is no clear answer which is "better," so i'm leaving both options
      // TODO: make this toggleable with a flag

      // if (tile->done[bx][by]) continue; // comment out to sample _last_ thing

      tile->fill[bx][by] += 1;

      const uint spx_i = (256 * 4 * sy) + (4 * sx);
      const uint dpx_i = (256 * 4 * dy) + (4 * dx);
      tile->fb_new[dpx_i + 0] = framebuffer[spx_i + 0];
      tile->fb_new[dpx_i + 1] = framebuffer[spx_i + 1];
      tile->fb_new[dpx_i + 2] = framebuffer[spx_i + 2];
      tile->fb_new[dpx_i + 3] = framebuffer[spx_i + 3];
    }
  }

  // 2) Save any 16x16 blocks that have been captured this frame
  // Note: there can be 4 big tiles that intersect the screen at any given time,
  // so all of them should be updated.
  const int tx = ::floor(this->scroll.x / 256.0);
  const int ty = ::floor(this->scroll.y / 240.0);
  for (int dx : {0, 1}) {
    for (int dy : {0, 1}) {
      Tile* tile = this->scenes[this->scene_id].tiles[tx + dx][ty + dy];
      if (!tile) continue;

      for (int bx = 0; bx < 16; bx++) {
        for (int by = 0; by < 15; by++) {
          if (tile->fill[bx][by] == 256) {
            tile->done[bx][by] = true; // mark block as fully filled-in

            // update the framebuffer with the captured 16x16 tile
            for (int x = 0; x < 16; x++) {
              for (int y = 0; y < 16; y++) {
                const uint px_i = (256 * 4 * (by * 16 + y))
                                +       (4 * (bx * 16 + x));
                tile->fb[px_i + 0] = tile->fb_new[px_i + 0];
                tile->fb[px_i + 1] = tile->fb_new[px_i + 1];
                tile->fb[px_i + 2] = tile->fb_new[px_i + 2];
                tile->fb[px_i + 3] = tile->fb_new[px_i + 3];
              }
            }
          }

          tile->fill[bx][by] = 0; // clear fill state on every iter
        }
      }
    }
  }

  // 3) Actually update the textures
  for (int dx : {0, 1}) {
    for (int dy : {0, 1}) {
      Tile* tile = this->scenes[this->scene_id].tiles[tx + dx][ty + dy];
      if (!tile) continue;

      SDL_UpdateTexture(tile->texture_curr, nullptr, tile->fb_new, 256 * 4);
      SDL_UpdateTexture(tile->texture_done, nullptr, tile->fb,     256 * 4);
    }
  }
}

/*---------------------------------  Output  ---------------------------------*/

void WideNESModule::output() {
  // calculate origin (where to render NES screen / where to offset tiles from)
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
  for (auto col : this->scenes[this->scene_id].tiles) {
    for (auto row : col.second) {
      const Tile* tile = row.second;
      if (!tile) continue;

      SDL_Rect offset = origin;
      offset.x -= this->pan.zoom * (this->scroll.x - tile->x * 256);
      offset.y -= this->pan.zoom * (this->scroll.y - tile->y * 240);

      // render the latest state for the tile first, but then overlay the 16x16
      // block version over that.
      // thus, we get "smooth" edges, but also clean blocks (except at edges)
      SDL_RenderCopy(this->sdl.renderer, tile->texture_curr, nullptr, &offset);
      SDL_RenderCopy(this->sdl.renderer, tile->texture_done, nullptr, &offset);

      SDL_SetRenderDrawColor(this->sdl.renderer, 0x60, 0x60, 0x60, 0xff);
      SDL_RenderDrawRect(this->sdl.renderer, &offset);

      this->sdl.inprint->set_color(0xff0000);
      char buf [64];
      sprintf(buf, "(%d, %d)", tile->x, tile->y);
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
  SDL_SetTextureBlendMode(this->nes_screen, SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(this->nes_screen, 100);
  SDL_RenderCopy(this->sdl.renderer, this->nes_screen, &col_clip, &col_origin);

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

  SDL_SetTextureAlphaMod(this->nes_screen, 255);
  SDL_RenderCopy(this->sdl.renderer, this->nes_screen, &padded_clip, &padded_origin);

  // full-screen box
  // SDL_SetRenderDrawColor(this->sdl.renderer, 0xff, 0, 0, 0xff);
  // SDL_RenderDrawRect(this->sdl.renderer, &origin);

  // clipped-screen box
  SDL_SetRenderDrawColor(this->sdl.renderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderDrawRect(this->sdl.renderer, &padded_origin);

  // debug values
  this->sdl.inprint->set_color(0xff0000);
  char buf [256];
  sprintf(buf,
    "    scene id: %d\n\n"
    "total scroll: %-3d %-3d\n"
    " last scroll: %-3d %-3d\n"
    "       dx dy: %-3d %-3d\n",
    this->scene_id,
    this->scroll.x, this->scroll.y,
    this->last_scroll.x, this->last_scroll.y,
    this->scroll.dx, this->scroll.dy
  );
  this->sdl.inprint->print(buf, 8, 8);

  this->menu_submodule->output();

  SDL_RenderPresent(this->sdl.renderer);
}

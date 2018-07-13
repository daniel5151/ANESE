#pragma once

#include <map>

#include <SDL.h>
#include <SDL_inprint2.h>

#include "../config.h"
#include "module.h"

#include "submodules/menu.h"

#include "nes/cartridge/mappers/mapper_004.h"
#include "nes/ppu/ppu.h"

class WideNESModule : public GUIModule {
private:
  struct {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL2_inprint* inprint  = nullptr;
  } sdl;

  struct Tile {
    struct {
      int x, y;
    } pos; // position within tile-grid

    SDL_Texture* texture = nullptr;
    u8 framebuffer [256 * 240 * 4] = {0};

    // misc: not initializing the framebuffer leads to a really cool effect
    // where any raw memory is visualized as a texture!

    ~Tile();
    Tile(SDL_Renderer* renderer, int x, int y);
  };

  // tilemap
  std::map<int, std::map<int, Tile*>> tiles;

  // not part of the tilemap, it's just convenient to have a copy of the current
  // nes screen
  Tile* nes_screen;

  // used to calculate dx and dy b/w frames
  struct nes_scroll {
    u8 x; u8 y;
  } last_scroll { 0, 0 }
  , curr_scroll { 0, 0 };

  // total scroll (offset from origin)
  struct { int x; int y; int dx; int dy; } scroll { 0, 0, 0, 0 };

  // there tend to be graphical artifacts at the edge of the screen, so it's
  // prudent to sample sliglty away from the edge.
  // moreover, some games have static menus on screen that impair sampling
  // (eg: smb3, mc kids)
  struct {
    struct {
      int l, r, t, b;
    } guess  { 0, 0, 0, 0 }  // intelligent guess
    , offset { 0, 0, 0, 0 }  // manual offset
    , total  { 0, 0, 0, 0 }; // sum of the two
  } pad;

  struct {
    struct {
      bool happened = false;
      uint on_scanline = 239;
      nes_scroll curr_scroll_pre_irq = { 0, 0 };
    } mmc3_irq;
  } heuristics;

  // zoom/pan info
  struct {
    bool active = false;
    struct { int x; int y; } last_mouse_pos { 0, 0 };
    int dx = 0;
    int dy = 0;
    float zoom = 2.0;
  } pan;

  void frame_start_handler();
  void frame_end_handler();

  void mmc3_irq_handler(Mapper_004* mapper, bool active);

  void scrollx_handler(u8 val);
  void scrolly_handler(u8 val);

  static void cb_frame_start(void* self);
  static void cb_frame_end(void* self);

  static void cb_mapper_changed(void* self, Mapper* cart);

  static void cb_scrollx_changed(void* self, u8 val);
  static void cb_scrolly_changed(void* self, u8 val);

  static void cb_mmc3_irq(void* self, Mapper_004* mapper, bool active);

  MenuSubModule* menu_submodule;

public:
  virtual ~WideNESModule();
  WideNESModule(SharedState& gui);

  void input(const SDL_Event&) override;
  void update() override;
  void output() override;

  uint get_window_id() override { return SDL_GetWindowID(this->sdl.window); }
};

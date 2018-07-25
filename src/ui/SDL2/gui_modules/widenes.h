#pragma once

#include <unordered_map>

#include <SDL.h>
#include <SDL_inprint2.h>

#include "../config.h"
#include "module.h"

#include "submodules/menu.h"

#include "nes/cartridge/mappers/mapper_004.h"

class WideNESModule : public GUIModule {
private:
  /*--------------------------  SDL / GUI stuff  ---------------------------*/

  struct {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL2_inprint* inprint  = nullptr;
  } sdl;

  SDL_Texture* nes_screen; // copy of actual NES screen

  // zoom/pan info
  struct {
    bool active = false;
    struct { int x; int y; } last_mouse_pos { 0, 0 };
    int dx = 0;
    int dy = 0;
    float zoom = 2.0;
  } pan;

  /*---------------------------  Tile Rendering  ---------------------------*/

  struct Tile {
    int x, y;

    bool done [16][15] = {{false}};
    int  fill [16][15] = {{0}};

    SDL_Texture* texture_done = nullptr;
    SDL_Texture* texture_curr = nullptr;

    u8 fb     [256 * 240 * 4] = {0};
    u8 fb_new [256 * 240 * 4] = {0};

    // misc: not initializing the framebuffer leads to a really cool effect
    // where any raw memory is visualized as a texture!

    ~Tile();
    Tile(SDL_Renderer* renderer, int x, int y);
  };

  /*------------------------  Current Recording State  -----------------------*/

  // Many games restrict the play-area to a limited subset of the screen, and
  //   as such, parts of the screen unrelated to the map should be masked-off
  // Moreover, there are times when the limitations of the NES hardware lead to
  //   nasty artifacting near the edges of the screen, and it would be nice to
  //   mask those off too.
  struct {
    struct {
      int l, r, t, b;
    } guess  { 0, 0, 0, 0 }  //   intelligent guess
    , offset { 0, 0, 0, 0 }  // + user offset
                             // -------------------
    , total  { 0, 0, 0, 0 }; //   total
  } pad;

  struct nes_scroll { u8 x; u8 y; };
  nes_scroll last_scroll { 0, 0 };
  nes_scroll curr_scroll { 0, 0 };

  // total scroll (offset from origin)
  struct total_scroll {
    int x, y;
    int dx, dy;
  } scroll { 0, 0, 0, 0 };

  /*------------------------------  Scene Data  ------------------------------*/

  int frame_hash_unique(const u8* fb) const;
  int frame_hash_percept(const u8* fb) const;


  struct Scene {
    struct _scroll_data {
      total_scroll scroll;
      nes_scroll last_scroll;
    };

    std::unordered_map<int, _scroll_data> scroll_hash;

    std::unordered_map<int, std::unordered_map<int, Tile*>> tiles;
  };

  std::string scenes_path;
  void save_scenes();
  void load_scenes();
  void clear_scenes();

  // TODO: make this less-bad
  struct {
    int next_id;
    int operator()() { return this->next_id++; }
  } gen_scene_id;

  int scene_id = 0;
  std::unordered_map<int, Scene> scenes; // <- this is what gets saved / loaded

  uint stitch_timer = 0;

  struct {
    int last;

    int vals [60];
    int vals_i = 0;
    int total = 0;

    int min;
    int max;
  } phash;

  /*-----------------------------  Heuristics  -----------------------------*/

  void fix_padding();
  void fix_scrolling();

  struct {
    // The OG heuristic: Sniffing the PPUSCROLL/PPUADDR register for changes
    struct {
      bool active;

      bool did_change = false;
      struct {
        uint on_scanline = 0;
        uint on_scancycle = 0;
        bool while_rendering = 0;
      } changed;

      uint cut_scanline = 0;
      nes_scroll new_scroll { 0, 0 };
    } ppuscroll, ppuaddr;

    // MMC3 Interrupt handling (i.e: intelligently chopping the screen)
    // (SMB3, M.C Kids)
    struct {
      bool active = false;

      uint on_scanline = 0;
      nes_scroll scroll_pre_irq = { 0, 0 };
    } mmc3_irq;
  } h;

  /*--------------------------  Callback Handlers  -------------------------*/

  void savestate_loaded_handler();

  void ppu_frame_end_handler();
  void ppu_write_start_handler(u16 addr, u8 val);
  void ppu_write_end_handler(u16 addr, u8 val);

  void mmc3_irq_handler(Mapper_004* mapper, bool active);

  static void cb_savestate_loaded(void* self);
  static void cb_mapper_changed(void* self, Mapper* cart);

  static void cb_ppu_frame_end(void* self);
  static void cb_ppu_write_start(void* self, u16 addr, u8 val);
  static void cb_ppu_write_end(void* self, u16 addr, u8 val);

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

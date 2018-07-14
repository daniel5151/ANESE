#pragma once

#include <SDL.h>

#include "module.h"

class PPUDebugModule : public GUIModule {
private:
  struct DebugPixelbuffWindow;
  DebugPixelbuffWindow* patt_t;
  DebugPixelbuffWindow* palette_t;
  DebugPixelbuffWindow* nes_palette;
  DebugPixelbuffWindow* name_t;

  uint sample_which_nt_counter = 60;

  bool nametable = 0;
  uint scanline = 0;

public:
  PPUDebugModule(SharedState& gui);
  virtual ~PPUDebugModule();

  void input(const SDL_Event&) override;
  void update() override;
  void output() override;

  void sample_ppu();

  static void cb_scanline(void* self);

  uint get_window_id() override;
};

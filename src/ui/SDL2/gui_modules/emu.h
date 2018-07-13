#pragma once

#include <SDL.h>
#include "../config.h"

#include "module.h"

#include "submodules/menu.h"

#include "common/callback_manager.h"

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/joy/controllers/zapper.h"
#include "nes/nes.h"

#include "../movies/fm2/record.h"
#include "../movies/fm2/replay.h"

#include "../util/Sound_Queue.h"

class EmuModule : public GUIModule {
private:
  struct {
    SDL_Renderer* renderer = nullptr;
    SDL_Window*   window   = nullptr;

    SDL_Rect screen_rect;
    SDL_Texture* screen_texture = nullptr;
    // SDL_AudioDeviceID nes_audiodev;
    Sound_Queue  sound_queue;
  } sdl;

  int speed_counter = 0;

  JOY_Standard joy_1 { "P1" };
  JOY_Standard joy_2 { "P2" };
  JOY_Zapper   zap_2 { "Z2" };

  // Movie Controllers
  FM2_Replay fm2_replay;
  FM2_Record fm2_record;

  MenuSubModule* menu_submodule;

public:
  virtual ~EmuModule();
  EmuModule(SharedState& gui);

  void input(const SDL_Event&) override;
  void update() override;
  void output() override;

  uint get_window_id() override { return SDL_GetWindowID(this->sdl.window); }
};

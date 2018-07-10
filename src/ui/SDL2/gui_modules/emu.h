#pragma once

#include <SDL.h>
#include "../config.h"

#include "module.h"

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/joy/controllers/zapper.h"
#include "nes/nes.h"

#include "../movies/fm2/record.h"
#include "../movies/fm2/replay.h"

#include "../util/Sound_Queue.h"

#include <vector>


class EmuModule : public GUIModule {
public:
  typedef void (*frame_callback)(void* userdata, EmuModule& self);

  struct {
    const uint SAMPLE_RATE = 96000;

    SDL_Renderer* renderer = nullptr;
    SDL_Window*   window   = nullptr;

    SDL_Rect screen_rect;
    SDL_Texture* screen_texture = nullptr;
    // SDL_AudioDeviceID nes_audiodev;
    Sound_Queue  sound_queue;
  } sdl;

private:
  int speed_counter = 0;

  struct cb {
    frame_callback f;
    void* userdata;
  };

  std::vector<cb> frame_callbacks;

public:
  NES_Params params;
  NES nes;
  char current_rom_file [256] = { 0 };
  Cartridge* cart = nullptr;

  JOY_Standard joy_1 { "P1" };
  JOY_Standard joy_2 { "P2" };
  JOY_Zapper   zap_2 { "Z2" };

  // Movie Controllers
  FM2_Replay fm2_replay;
  FM2_Record fm2_record;

  const Serializable::Chunk* savestate [4] = { nullptr };

public:
  virtual ~EmuModule();
  EmuModule(const SDLCommon& sdl_common, Config& config);

  void input(const SDL_Event&) override;
  void update() override;
  void output() override;

  int load_rom(const char* rompath);
  int unload_rom(Cartridge* cart);

  void register_frame_callback(frame_callback cb, void* userdata);
};

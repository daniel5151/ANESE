#pragma once

#include <string>

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "movies/fm2/record.h"
#include "movies/fm2/replay.h"

#include <SDL.h>
#include "util/Sound_Queue.h"

struct ANESE_Args {
  bool log_cpu;
  bool reset_sav;
  bool ppu_timing_hack;

  std::string record_fm2_path;
  std::string replay_fm2_path;

  std::string rom;
};

class SDL_GUI final {
private:
  // Command-line args
  ANESE_Args args;

  // SDL Things
  struct {
    SDL_Renderer* renderer    = nullptr;
    SDL_Window*   window      = nullptr;
    SDL_Texture*  nes_texture = nullptr;
    SDL_Rect      nes_screen;

    SDL_GameController* controller = nullptr;

    Sound_Queue sound_queue;
  } sdl;

  // Movie Controllers
  FM2_Replay fm2_replay;
  FM2_Record fm2_record;

  // Savestates
  const Serializable::Chunk* savestate [4] = { nullptr };

  // Speed-Control
  uint speedup = 100;
  int  speed_counter = 0;

  // NES Things
  NES nes;
  Cartridge* cart = nullptr;
  JOY_Standard joy_1 = JOY_Standard("P1");
  JOY_Standard joy_2 = JOY_Standard("P2");

  // SDL_GUI is running?
  bool running = true;

  int init_error_val = 0;

private:
  int set_up_cartridge(const char* rompath);

  void update_joypads(const SDL_Event&);
  void check_misc_actions(const SDL_Event& event);

public:
  ~SDL_GUI();
  SDL_GUI(int argc, char* argv[]);

  int init_error() { return this->init_error_val; }

  int run();
};

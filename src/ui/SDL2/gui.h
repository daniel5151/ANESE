#pragma once

#include <string>
#include <vector>

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "movies/fm2/record.h"
#include "movies/fm2/replay.h"

#include <SDL.h>
#include "util/Sound_Queue.h"

#include <cute_files.h>

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

  /*------------------------------  SDL Things  ------------------------------*/
  bool sdl_running = true;

  struct {
    SDL_Renderer* renderer    = nullptr;
    SDL_Window*   window      = nullptr;

    SDL_GameController* controller = nullptr;

    // NES render things
    SDL_Texture* nes_texture = nullptr;
    SDL_Rect     nes_screen;
    Sound_Queue  nes_sound_queue;

    // UI render things
    SDL_Rect bg;
  } sdl;

  /*------------------------------  UI Things  -------------------------------*/

  char current_rom_file [256] = "\0";

  bool in_menu = true;
  struct {
    std::vector<cf_file_t> files;
    char directory [256] = ".";
    bool should_update_dir = true;
    uint selected_i = 0;
    struct {
      bool enter, up, down, left, right;
      char last_ascii;
    } hit = {0, 0, 0, 0, 0, 0};
  } menu;

  /*------------------------------  NES Things  ------------------------------*/
  NES nes;
  Cartridge* cart = nullptr;
  JOY_Standard joy_1 = JOY_Standard("P1");
  JOY_Standard joy_2 = JOY_Standard("P2");

  // Movie Controllers
  FM2_Replay fm2_replay;
  FM2_Record fm2_record;

  // Savestates
  const Serializable::Chunk* savestate [4] = { nullptr };

  // Speed-Control
  uint speedup = 100;
  int  speed_counter = 0;

private:
  int load_rom(const char* rompath);
  int unload_rom(Cartridge* cart);

  void update_input_global(const SDL_Event&);
  void update_input_nes_joypads(const SDL_Event&);
  void update_input_nes_misc(const SDL_Event&);
  void update_input_ui(const SDL_Event&);

  void step_nes();

  void output_nes();
  void output_menu();

public:
  ~SDL_GUI();

  int init(int argc, char* argv[]);
  int run();
};

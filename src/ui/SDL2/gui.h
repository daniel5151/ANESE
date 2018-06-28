#pragma once

#include <string>
#include <vector>

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/joy/controllers/zapper.h"
#include "nes/nes.h"

#include "movies/fm2/record.h"
#include "movies/fm2/replay.h"

#include <SDL.h>
#include "util/Sound_Queue.h"

#include <cute_files.h>

/**
 * Implementations are strewn-across multiple files, as having everything in a
 * single file became unwieldy.
 */

class SDL_GUI final {
private:
  // Command-line args
  struct {
    bool log_cpu;
    bool reset_sav;
    bool ppu_timing_hack;

    std::string record_fm2_path;
    std::string replay_fm2_path;

    std::string rom;
  } args;

  static constexpr uint SAMPLE_RATE = 96000;

  /*------------------------------  SDL Things  ------------------------------*/
  bool sdl_running = true;

  struct {
    SDL_Renderer* renderer    = nullptr;
    SDL_Window*   window      = nullptr;

    SDL_GameController* controller = nullptr;

    // NES render things
    SDL_Texture*      nes_texture = nullptr;
    SDL_Rect          nes_screen;
    SDL_AudioDeviceID nes_audiodev;
    Sound_Queue  nes_sound_queue;

    // UI render things
    SDL_Rect bg;
  } sdl;

  /*------------------------------  UI Things  -------------------------------*/

  struct {
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
  } ui;

  uint RES_X = 256;
  uint RES_Y = 240;

  uint SCREEN_SCALE = 2;

  /*------------------------------  NES Things  ------------------------------*/

  struct {
    NES console { SDL_GUI::SAMPLE_RATE };
    Cartridge* cart = nullptr;

    JOY_Standard joy_1 { "P1" };
    JOY_Standard joy_2 { "P2" };
    JOY_Zapper   zap_2 { "Z2" };

    // Movie Controllers
    FM2_Replay fm2_replay;
    FM2_Record fm2_record;

    // Savestates
    const Serializable::Chunk* savestate [4] = { nullptr };

    // Speed-Control
    uint speedup = 100;
    int  speed_counter = 0;
  } nes;

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

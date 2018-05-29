// WARNING: This file is currently super messy!
// I'll clean it up (eventually)

#include "common/util.h"
#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "common/debug.h"

#include <iostream>
#include <string>

#include <args.hxx>
#include <SDL.h>
#include <tinyfiledialogs.h>

#include "ui/sdl/Sound_Queue.h"
#include "ui/movies/fm2/common.h"
#include "ui/movies/fm2/replay.h"
#include "ui/movies/fm2/record.h"
#include "ui/fs/load.h"

int main(int argc, char* argv[]) {
  // --------------------------- Argument Parsing --------------------------- //

  // use `args` to parse arguments
  args::ArgumentParser parser("ANESE - A Nintendo Entertainment System Emulator", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  // Debug
  args::Flag log_cpu(parser, "log-cpu",
    "Output CPU execution over STDOUT",
    {'c', "log-cpu"});

  // Hacks
  args::Flag ppu_timing_hack(parser, "alt-nmi-timing",
    "Enable NMI timing fix "
    "(needed to boot some games, eg: Bad Dudes, Solomon's Key)",
    {"alt-nmi-timing"});

  // Movies
  args::ValueFlag<std::string> arg_log_movie(parser, "log-movie",
    "log input in fm2 format",
    {"log-movie"});
  args::ValueFlag<std::string> arg_fm2(parser, "fm2",
    "path to fm2 movie",
    {"fm2"});

  // Essential
  args::Positional<std::string> rom(parser, "rom",
    "Valid iNES rom");

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (args::ValidationError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  if (log_cpu) { DEBUG_VARS::Get()->print_nestest = 1; }
  if (ppu_timing_hack) { DEBUG_VARS::Get()->fogleman_hack = 1; }

  std::string rom_path;

  if (!rom) {
    // use `tinyfiledialogs` to open file-select dialog (if no rom provided)
    const char* rom_formats[] = { "*.nes", "*.zip" };
    const char* file = tinyfd_openFileDialog(
      "Select ROM",
      nullptr,
      2, rom_formats,
      "NES roms",
      0
    );

    if (!file) {
      printf("[File Select] Canceled File Select. Closing...\n");
      return 1;
    } else {
      rom_path = std::string(file);
    }
  } else {
    rom_path = args::get(rom);
  }

  fprintf(stderr, "[Load] Loading '%s'\n", rom_path.c_str());

  // ---------------------------- Movie Support ----------------------------- //

  FM2_Replay fm2_replay;
  if (arg_fm2) {
    bool did_load = fm2_replay.init(args::get(arg_fm2).c_str());
    if (!did_load) {
      fprintf(stderr, "[Replay][fm2] Movie loading failed!\n");
      return 1;
    }
    fprintf(stderr, "[Replay][fm2] Movie successfully loaded!\n");
  }

  FM2_Record fm2_record;
  if (arg_log_movie) {
    bool did_load = fm2_record.init(args::get(arg_log_movie).c_str());
    if (!did_load) {
      fprintf(stderr, "[Record][fm2] Failed to setup Movie recording!\n");
      return 1;
    }
    fprintf(stderr, "[Record][fm2] Movie recording is setup!\n");

  }

  // -------------------------- Construct Cartridge ------------------------- //

  Cartridge cart (load_rom_file(rom_path.c_str()));
  switch (cart.status()) {
  case Cartridge::Status::BAD_DATA:
    fprintf(stderr, "[Cart] ROM file could not be parsed!\n");
    return 1;
  case Cartridge::Status::BAD_MAPPER:
    fprintf(stderr, "[Cart] Mapper %u has not been implemented yet!\n",
      cart.get_rom_file()->meta.mapper);
    return 1;
  case Cartridge::Status::NO_ERROR:
    fprintf(stderr, "[Cart] ROM file loaded successfully!\n");
    break;
  }

  // -------------------------- NES Initialization -------------------------- //

  // Create a NES
  NES nes;

  // Create some controllers...
  JOY_Standard joy_1 ("P1");
  JOY_Standard joy_2 ("P2");

  // pass controllers to fm2_record if needed
  if (fm2_record.is_enabled()) {
    fm2_record.set_joy(0, FM2_Controller::SI_GAMEPAD, &joy_1);
    fm2_record.set_joy(1, FM2_Controller::SI_GAMEPAD, &joy_2);
  }

  // Check if there is fm2 to replay
  if (fm2_replay.is_enabled()) {
    // plug in fm2 controllers
    nes.attach_joy(0, fm2_replay.get_joy(0));
    nes.attach_joy(1, fm2_replay.get_joy(1));
  } else {
    // plug in physical nes controllers
    nes.attach_joy(0, &joy_1);
    nes.attach_joy(1, &joy_2);

  }

  // Slap that cartridge in!
  nes.loadCartridge(cart.get_mapper());

  // Power up the NES
  nes.power_cycle();

  // --------------------------- UI and Core Loop --------------------------- //

  /*===========================
  =            SDL            =
  ===========================*/
  // This needs to be moved out of main!

  typedef u32 time_ms;

  SDL_Event event;
  SDL_Renderer* renderer;
  SDL_Window* window;

  constexpr u32 RES_X = 256;
  constexpr u32 RES_Y = 240;
  constexpr double RATIO_XY = double(RES_X) / double(RES_Y);
  constexpr double RATIO_YX = double(RES_Y) / double(RES_X);

  int window_w = RES_X * 2;
  int window_h = RES_Y * 2;
  char window_title [64] = "anese";

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
  window = SDL_CreateWindow(
    window_title,
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    window_w, window_h,
    SDL_WINDOW_RESIZABLE
  );

  renderer = SDL_CreateRenderer(
    window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  // nes screen texture
  SDL_Texture* texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    RES_X, RES_Y
  );

  // where within the window to render the screen texture
  SDL_Rect screen;

  // Frame Counter
  u32 total_frames = 0;

  /* Open the first available controller. */
  SDL_GameController* controller = nullptr;
  for (int i = 0; i < SDL_NumJoysticks(); ++i) {
    if (SDL_IsGameController(i)) {
      controller = SDL_GameControllerOpen(i);
      if (controller) {
        break;
      }
      else {
        fprintf(stderr, "[SDL] Could not open gamecontroller %i: %s\n",
          i, SDL_GetError());
      }
    }
  }

  Sound_Queue sound_queue;
  sound_queue.init(44100);

  uint speedup = 100;
  int  speed_counter = 0;

  // Main Loop
  bool quit = false;
  while (!quit) {
    time_ms frame_start_time = SDL_GetTicks();
    total_frames++;

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT)
        quit = true;

      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE)
        quit = true;

      // ------ Joypad controls ------ //
      if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
        bool new_state = (event.type == SDL_CONTROLLERBUTTONDOWN) ? true : false;
        using namespace JOY_Standard_Button;
        switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:          joy_1.set_button(A,      new_state); break;
        case SDL_CONTROLLER_BUTTON_X:          joy_1.set_button(B,      new_state); break;
        case SDL_CONTROLLER_BUTTON_START:      joy_1.set_button(Start,  new_state); break;
        case SDL_CONTROLLER_BUTTON_BACK:       joy_1.set_button(Select, new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    joy_1.set_button(Up,     new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  joy_1.set_button(Down,   new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  joy_1.set_button(Left,   new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: joy_1.set_button(Right,  new_state); break;
        }
      }

      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        // ------ Keyboard controls ------ //
        bool new_state = (event.type == SDL_KEYDOWN) ? true : false;
        using namespace JOY_Standard_Button;
        switch (event.key.keysym.sym) {
        case SDLK_z:      joy_1.set_button(A,      new_state); break;
        case SDLK_x:      joy_1.set_button(B,      new_state); break;
        case SDLK_RETURN: joy_1.set_button(Start,  new_state); break;
        case SDLK_RSHIFT: joy_1.set_button(Select, new_state); break;
        case SDLK_UP:     joy_1.set_button(Up,     new_state); break;
        case SDLK_DOWN:   joy_1.set_button(Down,   new_state); break;
        case SDLK_LEFT:   joy_1.set_button(Left,   new_state); break;
        case SDLK_RIGHT:  joy_1.set_button(Right,  new_state); break;
        }

        // Use CMD on macOS, and CTRL on windows / linux
        bool did_hit_meta = false;
        if (strcmp(SDL_GetPlatform(), "Mac OS X") == 0) {
          did_hit_meta = event.key.keysym.mod & (KMOD_LGUI | KMOD_RGUI);
        } else {
          did_hit_meta = event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);
        }

        // ------ Misc controls ------ //

        // Quit
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          quit = true;
        }

        // 2x speed
        if (event.key.keysym.sym == SDLK_SPACE) {
          speedup = (event.type == SDL_KEYDOWN) ? 200 : 100;
          speed_counter = 0;
          nes.set_speed(speedup);
        }

        // Meta Modified keys
        if (event.type == SDL_KEYDOWN && did_hit_meta) {
          switch (event.key.keysym.sym) {
          case SDLK_r:      nes.reset();                  break;
          case SDLK_p:      nes.power_cycle();            break;
          case SDLK_EQUALS: {
            nes.set_speed(speedup += 25);
            speed_counter = 0;
          } break;
          case SDLK_MINUS: {
            if (speedup - 25) {
              nes.set_speed(speedup -= 25);
              speed_counter = 0;
            }
          } break;
          case SDLK_c: {
            bool log = DEBUG_VARS::Get()->print_nestest ^= 1;
            fprintf(stderr, "NESTEST CPU logging: %s\n", log ? "ON" : "OFF");
          } break;
          default: break;
          }
        }
      }
    }

    // Calculate the number of frames to render, as speedup values that are not
    // multiples of 100 cannot be represented by rendering a consistent number
    // of nes frames / actual frame.
    uint numframes = 0;
    speed_counter += speedup;
    while (speed_counter > 0) {
      speed_counter -= 100;
      numframes++;
    }

    for (uint i = 0; i < numframes; i++) {
      // log to movie file
      if (fm2_record.is_enabled()) {
        fm2_record.step_frame();
      }

      // update fm2 inputs
      if (fm2_replay.is_enabled()) {
        fm2_replay.step_frame();
      }

      // run the NES for a frame
      nes.step_frame();
    }

    if (nes.isRunning() == false) {
      // quit = true;
    }

    // output audio!
    short* samples = nullptr;
    uint count = 0;
    nes.getAudiobuff(samples, count);
    if (count) sound_queue.write(samples, count);

    // output!
    SDL_GetWindowSize(window, &window_w, &window_h);

    // Letterbox the screen within the window
    if (window_w / double(RES_X) > window_h / double(RES_Y)) {
      // fat window
      screen.h = static_cast<int>(window_h);
      screen.w = static_cast<int>(window_h * RATIO_XY);
      screen.x = -(screen.w - window_w) / 2;
      screen.y = 0;
    } else {
      // tall window
      screen.h = static_cast<int>(window_w * RATIO_YX);
      screen.w = static_cast<int>(window_w);
      screen.x = 0;
      screen.y = -(screen.h - window_h) / 2;
    }

    // Update the screen texture
    SDL_UpdateTexture(texture, nullptr, nes.getFramebuff(), RES_X * 4);

    // Render everything
    SDL_RenderCopy(renderer, texture, nullptr, &screen);

    // Reveal out the composited image
    SDL_RenderPresent(renderer);

    time_ms frame_end_time = SDL_GetTicks();

    // ---- Limit Framerate ---- //
    // NOTE: This is currently disabled, since it doesn't work as well as just
    //       enabling vsync.
    // NES runs as 60 fups, so don't run faster!
    // time_ms TARGET_FRAME_DT = static_cast<time_ms>(1000.0 / 60.0);
    // time_ms frame_dt = SDL_GetTicks() - frame_start_time;
    // if (frame_dt < TARGET_FRAME_DT)
    //   SDL_Delay(TARGET_FRAME_DT - frame_dt);

    // ---- Count Framerate ---- //
    static double past_fps [20] = {60.0}; // more samples == less FPS jutter

    // Get current FPS
    past_fps[total_frames % 20] = 1000.0 / (frame_end_time - frame_start_time);

    double avg_fps = 0;
    for(unsigned i = 0; i < 20; i++)
      avg_fps += past_fps[i];
    avg_fps /= 20;

    sprintf(window_title, "anese - %u fups - %u%% speed", uint(avg_fps), speedup);
    SDL_SetWindowTitle(window, window_title);
  }

  // SDL Cleanup
  SDL_GameControllerClose(controller);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  printf("\nANESE closed successfully\n");

  return 0;
}

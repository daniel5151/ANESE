// WARNING: This file is currently super messy!
// I'll clean it up (eventually)

#include "common/util.h"
#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "common/debug.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <args.hxx>
#include <SDL.h>
#include <tinyfiledialogs.h>
#include <miniz_zip.h>

#include "ui/sdl/Sound_Queue.h"
#include "ui/movies/playback/fm2.h"

#include <algorithm>
static inline std::string get_file_ext(std::string filename) {
  std::string ext = filename.substr(filename.find_last_of("."));
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return ext;
}

int main(int argc, char* argv[]) {
  // --------------------------- Argument Parsing --------------------------- //

  // use `args` to parse arguments
  args::ArgumentParser parser("ANESE - A Nintendo Entertainment System Emulator", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Flag log_cpu(parser, "log-cpu", "Output CPU execution over STDOUT", {"log-cpu"});
  args::ValueFlag<std::string> arg_log_movie(parser, "log-movie", "log input in fm2 format", {"log-movie"});
  args::ValueFlag<std::string> arg_fm2(parser, "fm2", "path to fm2 movie", {"fm2"});

  args::Positional<std::string> rom(parser, "rom", "Valid iNES rom");

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

  // Some pretty low-tier movie support
  // TODO: make a FM2_Recording_Controller...
  bool load_fm2 = false;
  std::string fm2_path = "";
  if (arg_fm2) {
    load_fm2 = true;
    fm2_path = args::get(arg_fm2);
  }

  FILE* movie = nullptr;
  if (arg_log_movie) {
    std::string movie_path = args::get(arg_log_movie);
    movie = fopen(movie_path.c_str(), "w");
  }

  if (log_cpu) { DEBUG_VARS::Get()->print_nestest = 1; }

  // ----------------------------- Read ROM File ---------------------------- //

  // We need to fill up this pointer with some data:
  u8* data = nullptr;
  uint data_len = 0;

  std::string rom_ext = get_file_ext(rom_path);

  // Read data in .nes file directly
  if (rom_ext == ".nes") {
    std::ifstream rom_file (rom_path, std::ios::binary);

    if (!rom_file.is_open()) {
      fprintf(stderr, "[Open][.nes] Could not open '%s'\n", rom_path.c_str());
      return 1;
    }

    // get length of file
    rom_file.seekg(0, rom_file.end);
    std::streamoff rom_file_size = rom_file.tellg();
    rom_file.seekg(0, rom_file.beg);

    if (rom_file_size == -1) {
      fprintf(stderr, "[Open][.nes] Could not read '%s'\n", rom_path.c_str());
      return 1;
    }

    data_len = rom_file_size;
    data = new u8 [data_len];

    rom_file.read((char*) data, data_len);

    fprintf(stderr, "[Open][.nes] Successfully read '%s'\n", rom_path.c_str());

  }
  // If given a zip, try to decompress it
  else if (rom_ext == ".zip") {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof zip_archive);
    mz_bool status = mz_zip_reader_init_file(
      &zip_archive,
      rom_path.c_str(),
      0
    );
    if (!status) {
      fprintf(stderr, "[Open][.zip] Could not read '%s'\n", rom_path.c_str());
      return 1;
    }

    // Try to find a .nes file in the archive
    for (uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
      mz_zip_archive_file_stat file_stat;
      mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

      std::string file_name = std::string(file_stat.m_filename);
      std::string file_ext = get_file_ext(file_name);

      if (file_ext == ".nes") {
        printf("[Open][.zip][UnZip] Found .nes file in archive: '%s'\n", file_stat.m_filename);

        size_t uncomp_size;
        void* p = mz_zip_reader_extract_file_to_heap(
          &zip_archive,
          file_stat.m_filename,
          &uncomp_size,
          0
        );

        if (!p) {
          printf("[Open][.zip][UnZip] Could not decompress '%s'\n", rom_path.c_str());
          return 1;
        }

        // Nice! We got data!
        data_len = uncomp_size;
        data = new u8 [data_len];
        memcpy(data, p, data_len);

        fprintf(stderr, "[Open][.zip] Successfully read '%s'\n", rom_path.c_str());

        // Free the redundant decompressed file mem
        mz_free(p);

        // Close the archive, freeing any resources it was using
        mz_zip_reader_end(&zip_archive);
      }
    }
  } else {
    fprintf(stderr, "[Open] Invalid file extension.\n");
    return 1;
  }

  // ---------------------------- Read fm2 file ----------------------------- //

  FM2_Playback_Controller* fm2_playback_controller = nullptr;

  if (load_fm2) {
    std::ifstream fm2_file (fm2_path, std::ios::binary);

    if (!fm2_file.is_open()) {
      fprintf(stderr, "[Open][.fm2] Could not open '%s'\n", fm2_path.c_str());
      return 1;
    }

    // get length of file
    fm2_file.seekg(0, fm2_file.end);
    std::streamoff fm2_file_size = fm2_file.tellg();
    fm2_file.seekg(0, fm2_file.beg);

    if (fm2_file_size == -1) {
      fprintf(stderr, "[Open][.fm2] Could not read '%s'\n", fm2_path.c_str());
      return 1;
    }

    // Nice, we got fm2 data!
    const char* fm2 = new char [fm2_file_size];
    fm2_file.read((char*) fm2, fm2_file_size);

    fprintf(stderr, "[Open][.fm2] Successfully read '%s'\n", fm2_path.c_str());

    // create a fm2 controller
    fm2_playback_controller = new FM2_Playback_Controller(fm2, fm2_file_size);

    // cleanup
    delete[] fm2;
  }

  // -------------------------- NES Initialization -------------------------- //

  // Generate cartridge from data
  Cartridge rom_cart (data, data_len);
  delete[] data;

  Cartridge::Error error = rom_cart.getError();
  switch (error) {
  case Cartridge::Error::NO_ERROR:
    fprintf(stderr, "[Cart] Cartridge created successfully!\n");
    break;
  case Cartridge::Error::BAD_MAPPER:
    fprintf(stderr, "[Cart] Mapper %u has not been implemented yet!\n",
      rom_cart.getMapper());
    return 1;
  case Cartridge::Error::BAD_DATA:
    fprintf(stderr, "[Cart] ROM file could not be parsed!\n");
    return 1;
  }

  // Create a NES
  NES nes;

  // Create some controllers...
  JOY_Standard joy_1 ("P1");
  JOY_Standard joy_2 ("P2");

  // Update movie header (if needed)
  if (movie) {
    // very incomplete fm2 header...
    fprintf(movie, "port0 1\n");
    fprintf(movie, "port1 1\n");
    fprintf(movie, "port2 0\n");
  }

  // Check if there is a fm2 to playback
  if (fm2_playback_controller) {
    // plug in fm2 controllers
    nes.attach_joy(0, fm2_playback_controller->get_joy(0));
    nes.attach_joy(1, fm2_playback_controller->get_joy(1));
  } else {
    // plug in physical nes controllers
    nes.attach_joy(0, &joy_1);
    nes.attach_joy(1, &joy_2);
  }

  // Slap that cartridge in!
  // (don't forget to blow on it a few times though)
  rom_cart.blowOnContacts();
  rom_cart.blowOnContacts();
  nes.loadCartridge(&rom_cart);

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
      if (movie) {
        fprintf(movie, "|0|%s|%s||\n",
          joy_1.get_movie_frame(),
          joy_2.get_movie_frame()
        );
      }

      // update fm2 inputs
      if (fm2_playback_controller) {
        fm2_playback_controller->step_frame();
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

    sprintf(window_title, "anese - %d fups - %d%% speed", int(avg_fps), speedup);
    SDL_SetWindowTitle(window, window_title);
  }

  // SDL Cleanup
  SDL_GameControllerClose(controller);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  delete fm2_playback_controller;

  printf("\nANESE closed successfully\n");

  return 0;
}

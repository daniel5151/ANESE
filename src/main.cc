#include "common/util.h"
#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/nes.h"

#include "common/debug.h"

#include <string>
#include <iostream>
#include <fstream>

#include <args.hxx>
#include <SDL.h>
#include <tinyfiledialogs.h>
#include <miniz_zip.h>

int main(int argc, char* argv[]) {
  // --------------------------- Argument Parsing --------------------------- //

  // use `args` to parse arguments
  args::ArgumentParser parser("ANESE - A Nintendo Entertainment System Emulator", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::Flag log_cpu(parser, "log_cpu", "Output CPU execution over STDOUT", {"log-cpu"});
  args::Positional<std::string> rom(parser, "rom", "Valid iNES rom");

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (args::ValidationError e) {
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
      printf("Canceled File Select. Closing...\n");
      return -1;
    } else {
      rom_path = std::string(file);
    }
  } else {
    rom_path = args::get(rom);
  }

  if (log_cpu) { DEBUG_VARS::Get()->print_nestest = 1; }

  // ----------------------------- Read ROM File ---------------------------- //

  // We need to fill up this pointer with some data:
  u8* data = nullptr;
  uint data_len = 0;

  std::string rom_ext = rom_path.substr(rom_path.find_last_of("."));

  // Read data in .nes file directly
  if (rom_ext == ".nes") {
    std::ifstream rom_file (rom_path, std::ios::binary);

    if (!rom_file.is_open()) {
      std::cerr << "could not open '" << rom_path << "'\n";
      return 1;
    }

    // get length of file
    rom_file.seekg(0, rom_file.end);
    std::streamoff rom_file_size = rom_file.tellg();
    rom_file.seekg(0, rom_file.beg);

    if (rom_file_size == -1) {
      std::cerr << "could not read '" << rom_path << "'\n";
      return 1;
    }

    data_len = static_cast<uint>(rom_file_size);
    data = new u8 [data_len];

    rom_file.read((char*) data, data_len);
  }
  // If given a zip, try to decompress it
  else if (rom_ext == ".zip") {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    mz_bool status = mz_zip_reader_init_file(
      &zip_archive,
      rom_path.c_str(),
      0
    );
    if (!status) {
      std::cerr << "could not read zip file'" << rom_path << "'\n";
      return 1;
    }

    // Try to find a .nes file in the archive
    for (uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
      mz_zip_archive_file_stat file_stat;
      mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

      std::string file_name = std::string(file_stat.m_filename);
      std::string file_ext = file_name.substr(file_name.find_last_of("."));

      if (file_ext == ".nes") {
        printf("Found .nes file in archive: '%s'\n", file_stat.m_filename);

        size_t uncomp_size;
        void* p = mz_zip_reader_extract_file_to_heap(
          &zip_archive,
          file_stat.m_filename,
          &uncomp_size,
          0
        );

        if (!p) {
          std::cerr << "Could not decompress zip file'" << rom_path << "'\n";
          return 1;
        }

        // Nice! We got data!
        data_len = uncomp_size;
        data = new u8 [data_len];
        memcpy(data, p, data_len);

        // Free the redundant decompressed file mem
        mz_free(p);

        // Close the archive, freeing any resources it was using
        mz_zip_reader_end(&zip_archive);
      }
    }
  }

  // -------------------------- NES Initialization -------------------------- //

  // Generate cartridge from data
  // Note: cartridge now owns data
  Cartridge rom_cart (data, data_len);

  Cartridge::Error error = rom_cart.getError();
  switch (error) {
  case Cartridge::Error::NO_ERROR:
    std::cerr << "ROM successfully loaded successfully!\n";
    break;
  case Cartridge::Error::BAD_MAPPER:
    std::cerr << "Mapper " << rom_cart.getMapper() << " isn't implemented!\n";
    return 1;
  case Cartridge::Error::BAD_DATA:
    std::cerr << "ROM file format could not be parsed as iNES!\n";
    return 1;
  }

  // Create a NES
  NES nes;

  // Create some controllers
  JOY_Standard joy_1 ("P1");
  JOY_Standard joy_2 ("P2");

  // And plug them in too!
  nes.attach_joy(0, &joy_1);
  nes.attach_joy(1, &joy_2);

  // Slap in a cartridge
  // (don't forget to blow on it)
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

  SDL_Init(SDL_INIT_EVERYTHING);
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
        fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
      }
    }
  }

  uint speedup = 1;

  // Main Loop
  bool quit = false;
  while (!quit) {
    time_ms frame_start_time = SDL_GetTicks();
    total_frames++;

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        quit = true;
        continue;
      }

      // for now
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE) {
        quit = true;
        continue;
      }

      if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
        bool new_state = (event.type == SDL_CONTROLLERBUTTONDOWN) ? true : false;
        switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:          joy_1.set_button("A",      new_state); break;
        case SDL_CONTROLLER_BUTTON_X:          joy_1.set_button("B",      new_state); break;
        case SDL_CONTROLLER_BUTTON_START:      joy_1.set_button("Start",  new_state); break;
        case SDL_CONTROLLER_BUTTON_BACK:       joy_1.set_button("Select", new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    joy_1.set_button("Up",     new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  joy_1.set_button("Down",   new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  joy_1.set_button("Left",   new_state); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: joy_1.set_button("Right",  new_state); break;
        }
      }

      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        // ------ Joypad controls ------ //

        bool new_state = (event.type == SDL_KEYDOWN) ? true : false;
        switch (event.key.keysym.sym) {
        case SDLK_z:      joy_1.set_button("A",      new_state); break;
        case SDLK_x:      joy_1.set_button("B",      new_state); break;
        case SDLK_RETURN: joy_1.set_button("Start",  new_state); break;
        case SDLK_RSHIFT: joy_1.set_button("Select", new_state); break;
        case SDLK_UP:     joy_1.set_button("Up",     new_state); break;
        case SDLK_DOWN:   joy_1.set_button("Down",   new_state); break;
        case SDLK_LEFT:   joy_1.set_button("Left",   new_state); break;
        case SDLK_RIGHT:  joy_1.set_button("Right",  new_state); break;
        }

        bool did_hit_ctrl = false;
        if (std::string(SDL_GetPlatform()) == "Mac OS X") {
          did_hit_ctrl = event.key.keysym.mod & (KMOD_LGUI | KMOD_RGUI);
        } else {
          did_hit_ctrl = event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);
        }

        // ------ Misc controls ------ //

        // Quit
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          quit = true;
        }

        if (event.key.keysym.sym == SDLK_SPACE) {
          if (event.type == SDL_KEYDOWN) {
            if (speedup != 2)
              fprintf(stderr, "Fast Forward: ON\n");

            speedup = 2;
          } else {
            fprintf(stderr, "Fast Forward: OFF\n");
            speedup = 1;
          }
        }

        if (event.type == SDL_KEYDOWN && did_hit_ctrl) {
          if (event.key.keysym.sym == SDLK_r) {
            fprintf(stderr, "NES Reset!\n");
            nes.reset();
          }

          if (event.key.keysym.sym == SDLK_p) {
            fprintf(stderr, "NES Power Cycled!\n");
            nes.power_cycle();
          }

          if (event.key.keysym.sym == SDLK_c) {
            bool log = DEBUG_VARS::Get()->print_nestest ^= 1;
            fprintf(stderr, "NESTEST CPU logging: %s\n", log ? "ON" : "OFF");
          }

          if (event.key.keysym.sym == SDLK_EQUALS) {
            speedup++;
            fprintf(stderr, "Speed: %dx\n", speedup);
          }

          if (event.key.keysym.sym == SDLK_MINUS) {
            speedup--;
            if (speedup == 0) speedup = 1;
            fprintf(stderr, "Speed: %dx\n", speedup);
          }

        }
        continue;
      }
    }

    // run the NES
    for (uint i = 0; i < speedup; i++)
      nes.step_frame();

    if (nes.isRunning() == false) {
      // quit = true;
    }

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

    sprintf(window_title, "anese - %d fups", int(avg_fps));
    SDL_SetWindowTitle(window, window_title);
  }

  // SDL Cleanup
  SDL_GameControllerClose(controller);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

#include "common/util.h"
#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"

#include <iostream>
#include <fstream>

#include <SDL.h>

typedef u32 time_ms;

int main(int argc, char* argv[]) {
  // Parse args
  if (argc < 2) {
    std::cerr << "usage: anese [.nes]\n";
    return -1;
  }

  // open ROM from file
  std::ifstream rom_file (argv[1], std::ios::binary);

  if (!rom_file.is_open()) {
    std::cerr << "could not open '" << argv[1] << "'\n";
    return -1;
  }

  // get length of file
  rom_file.seekg(0, rom_file.end);
  std::streamoff rom_file_size = rom_file.tellg();
  rom_file.seekg(0, rom_file.beg);

  if (rom_file_size == -1) {
    std::cerr << "could not read '" << argv[1] << "'\n";
    return -1;
  }

  uint data_len = static_cast<uint>(rom_file_size);

  u8* data = new u8 [data_len];

  rom_file.read((char*) data, data_len);

  // Generate cartridge from data
  Cartridge* rom_cart = new Cartridge (data, data_len);

  if (rom_cart->isValid()) {
    std::cerr << "iNES file loaded successfully!\n";
  } else {
    std::cerr << "Given file was not an iNES file!\n";
    return -1;
  }

  // Create a NES
  NES nes;

  // Slap in a cartridge
  // (don't forget to blow on it)
  rom_cart->blowOnContacts();
  rom_cart->blowOnContacts();
  nes.loadCartridge(rom_cart);

  // Power up the NES
  nes.power_cycle();

  /*===========================
  =            SDL            =
  ===========================*/
  // This needs to be moved out of main!

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

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED
                                          // | SDL_RENDERER_PRESENTVSYNC
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

  // Main Loop
  bool quit = false;
  while (!quit) {
    time_ms frame_start_time = SDL_GetTicks();
    total_frames++;

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }

    // run the NES for a frame
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
    SDL_UpdateTexture(texture, nullptr, nes.getFrame(), RES_X * 4);

    // Render everything
    SDL_RenderCopy(renderer, texture, nullptr, &screen);

    // Reveal out the composited image
    SDL_RenderPresent(renderer);

    // ---- Limit Framerate ---- //
    // NES runs as 60 fups, so don't run faster!
    constexpr time_ms TARGET_FPS = static_cast<time_ms>(1000.0 / 60.0);
    time_ms frame_dt = SDL_GetTicks() - frame_start_time;
    if (frame_dt < TARGET_FPS)
      SDL_Delay(TARGET_FPS - frame_dt);

    time_ms frame_end_time = SDL_GetTicks();

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

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  // Throw away the Cartridge
  delete rom_cart;

  return 0;
}

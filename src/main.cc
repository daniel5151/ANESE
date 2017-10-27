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
  u32 data_len = rom_file.tellg();
  rom_file.seekg(0, rom_file.beg);

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
  NES nes = NES ();

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
  constexpr float RATIO_XY = float(RES_X) / float(RES_Y);
  constexpr float RATIO_YX = float(RES_Y) / float(RES_X);

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

  // Main NES pixelbuffer
  u8* pixelbuff = new u8 [RES_X * RES_Y * 4];

  // Frame Counting
  const time_ms start_time = SDL_GetTicks();

  time_ms frame_start_time;
  time_ms frame_end_time;
  u32 total_frames = 0;

  // Main Loop
  bool quit = false;
  while (!quit) {
    time_ms frame_start_time = SDL_GetTicks();

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }

    // run the NES for a frame
    nes.step_frame();

    if (nes.isRunning() == false) {
      quit = true;
      break;
    }

    // output!
    SDL_GetWindowSize(window, &window_w, &window_h);

    // Letterbox the screen within the window
    if (window_w / float(RES_X) > window_h / float(RES_Y)) {
      // fat window
      screen.h = window_h;
      screen.w = window_h * RATIO_XY;
      screen.x = -(screen.w - window_w) / 2;
      screen.y = 0;
    } else {
      // tall window
      screen.h = window_w * RATIO_YX;
      screen.w = window_w;
      screen.x = 0;
      screen.y = -(screen.h - window_h) / 2;
    }

    // Update the screen texture

    // lmao this looks neato
    for (u32 x = 0; x < RES_X; x++) {
      for (u32 y = 0; y < RES_Y; y++) {
        const u32 offset = (RES_X * 4 * y) + x * 4;
        pixelbuff[offset + 0] = (sin(frame_start_time / 1000.0) + 1) * 126; // b
        pixelbuff[offset + 1] = (y / float(RES_Y)) * RES_X; // g
        pixelbuff[offset + 2] = (x / float(RES_X)) * RES_X; // r
        pixelbuff[offset + 3] = SDL_ALPHA_OPAQUE;  // a
      }
    }
    SDL_UpdateTexture(texture, nullptr, pixelbuff, RES_X * 4);

    // Render everything
    SDL_RenderCopy(renderer, texture, nullptr, &screen);

    // Reveal out the composited image
    SDL_RenderPresent(renderer);

    time_ms frame_end_time = SDL_GetTicks();

    // ---- Count Framerate ---- //
    // atm, this is a running avg.
    // it really should be sample-based (to reflect fluctuating performance)
    total_frames++;
    float total_dt = frame_end_time - start_time;
    float fps = total_frames / (total_dt / 1000.0);
    sprintf(window_title, "anese - %d fups", int(fps));
    SDL_SetWindowTitle(window, window_title);

    // ---- Limit Framerate ---- //
    constexpr time_ms TARGET_FPS = 1000.0 / 60.0;
    // Don't run faster than 60 fups!
    time_ms frame_dt = frame_end_time - frame_start_time;
    if (frame_dt < TARGET_FPS) {
      SDL_Delay(TARGET_FPS - frame_dt);
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  // Throw away the Cartridge
  delete rom_cart;

  return 0;
}

#include "common/util.h"
#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"

#include <iostream>
#include <fstream>

#include <SDL.h>

int main(int argc, const char* argv[]) {
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
  SDL_Renderer *renderer;
  SDL_Window *window;

  int window_w = 640;
  int window_h = 480;

  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    window_w, window_h,
    SDL_WINDOW_RESIZABLE
  );

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

  bool quit = false;
  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }

    // TODO: ADD FRAME LIMITING

    // run the NES for a frame
    nes.step();

    if (nes.isRunning() == false) {
      quit = true;
      break;
    }

    // output!

    // This is just me dicking around...
    // By the way, this is a stupidly inneficient way of updating pixels on a
    // screen!
    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    for (float x = 0; x < window_w; x++) {
      for (float y = 0; y < window_h; y++) {
        SDL_SetRenderDrawColor(renderer, (x / window_w) * 255, (y / window_h) * 255, 0, 255);
        SDL_RenderDrawPoint(renderer, x, y);
      }
    }

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  // Throw away the Cartridge
  delete rom_cart;

  return 0;
}

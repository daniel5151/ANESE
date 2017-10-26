#include "common/util.h"
#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"

#include <iostream>
#include <fstream>

#include <SDL.h>

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
  SDL_Renderer *renderer;
  SDL_Window *window;

  int window_w = 256 * 2;
  int window_h = 240 * 2;

  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    window_w, window_h,
    SDL_WINDOW_RESIZABLE
  );

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // nes screen texture
  const unsigned int texWidth = 256;
  const unsigned int texHeight = 240;
  SDL_Texture* texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    texWidth, texHeight
  );

  SDL_Rect screen;

  u8* pixelbuff = new u8 [texWidth * texHeight * 4];


  bool quit = false;
  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }

    // TODO: FRAME LIMITING

    // run the NES for a frame
    nes.step();

    if (nes.isRunning() == false) {
      quit = true;
      break;
    }

    // output!
    SDL_GetWindowSize(window, &window_w, &window_h);

    float skew_w = window_w / 256.0;
    float skew_h = window_h / 240.0;

    if (skew_w > skew_h) {
      // i.e: fat window
      screen.h = window_h;
      screen.w = window_h * (256.0 / 240.0);
      screen.x = -(screen.w - window_w) / 2;
      screen.y = 0;
    } else {
      // i.e: tall window
      screen.h = window_w * (240.0 / 256.0);
      screen.w = window_w;
      screen.x = 0;
      screen.y = -(screen.h - window_h) / 2;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // lmao this looks neato
    for (int x = 0; x < texWidth; x++) {
      for (int y = 0; y < texHeight; y++) {
        const unsigned int offset = (texWidth * 4 * y) + x * 4;
        pixelbuff[offset + 0] = (sin(SDL_GetTicks() / 1000.0) + 1) * 126; // b
        pixelbuff[offset + 1] = (float(y) / texHeight) * 256; // g
        pixelbuff[offset + 2] = (float(x) / texWidth)  * 256; // r
        pixelbuff[offset + 3] = SDL_ALPHA_OPAQUE;             // a
      }
    }

    SDL_UpdateTexture(
      texture,
      nullptr,
      &pixelbuff[0],
      texWidth * 4
    );

    SDL_RenderCopy(renderer, texture, nullptr, &screen);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  // Throw away the Cartridge
  delete rom_cart;

  return 0;
}

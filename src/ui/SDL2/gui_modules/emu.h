#pragma once

#include <SDL.h>
#include "../config.h"

#include "module.h"

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/joy/controllers/zapper.h"
#include "nes/nes.h"

#include "../movies/fm2/record.h"
#include "../movies/fm2/replay.h"

#include "../util/Sound_Queue.h"

#include <map>

class EmuModule : public GUIModule {
public:
  struct {
    // regular NES
    const uint RES_X = 256;
    const uint RES_Y = 240;
    const uint SAMPLE_RATE = 96000;

    SDL_Renderer* renderer = nullptr;
    SDL_Window*   window   = nullptr;

    SDL_Texture* screen_texture = nullptr;
    // SDL_AudioDeviceID nes_audiodev;
    Sound_Queue  sound_queue;
  } sdl;

private:
  int speed_counter = 0;

  // wideNES
  friend class WideNES;
  class WideNES {
    friend EmuModule;

    struct Tile {
      struct {
        int x, y;
      } pos; // position within tile-grid

      SDL_Texture* texture;
      u8 framebuffer [256 * 240 * 4];
      Tile(SDL_Renderer* renderer, int x, int y);
    };

    // used to calculate dx and dy
    struct {
      u8 x, y;
    } last_scroll { 0, 0 };

    // total scroll (offset from origin)
    struct {
      int x, y;
    } scroll { 0, 0 };

    // tilemap
    std::map<int, std::map<int, Tile*>> tiles;

    EmuModule* self;

  public:
    WideNES(EmuModule& self);

    void samplePPU();

  };
  WideNES* wideNES;


public:
  NES_Params params;
  NES nes;
  char current_rom_file [256] = { 0 };
  Cartridge* cart = nullptr;

  JOY_Standard joy_1 { "P1" };
  JOY_Standard joy_2 { "P2" };
  JOY_Zapper   zap_2 { "Z2" };

  // Movie Controllers
  FM2_Replay fm2_replay;
  FM2_Record fm2_record;

  const Serializable::Chunk* savestate [4] = { nullptr };

public:
  virtual ~EmuModule();
  EmuModule(const SDLCommon& sdl_common, Config& config);
  void input(const SDL_Event&) override;
  void update() override;
  void output() override;

  int load_rom(const char* rompath);
  int unload_rom(Cartridge* cart);
};

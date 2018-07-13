#include "gui.h"

#include <cstdio>

#include "common/util.h"

#include "gui_modules/emu.h"
#include "gui_modules/widenes.h"
#include "gui_modules/ppu_debug.h"

SDL_GUI::SDL_GUI(int argc, char* argv[]) {
  // Init config
  this->config.load(argc, argv);

  // Init NES params
  this->nes_params.log_cpu         = this->config.cli.log_cpu;
  this->nes_params.ppu_timing_hack = this->config.cli.ppu_timing_hack;
  this->nes_params.apu_sample_rate = 96000;
  this->nes_params.speed           = 100;

  // Init NES
  this->nes = new NES(this->nes_params);

  // Init SDL_Common
  fprintf(stderr, "[SDL2] Initializing SDL2 GUI\n");
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
  /* Open the first available controller. */
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      this->sdl_common.controller = SDL_GameControllerOpen(i);
    }
  }

  /*----------  Init GUI modules  ----------*/
  this->shared = new SharedState(
    this->modules,
    this->status,
    this->sdl_common,
    this->config,
    this->nes_params,
    *this->nes
  );

  this->modules["emu"] = (GUIModule*)new EmuModule(*this->shared);

  if (this->config.cli.ppu_debug)
    this->modules["ppu_debug"] = (GUIModule*)new PPUDebugModule(*this->shared);

  if (this->config.cli.widenes)
    this->modules["widenes"] = (GUIModule*)new WideNESModule(*this->shared);
}

SDL_GUI::~SDL_GUI() {
  fprintf(stderr, "[SDL2] Stopping SDL2 GUI\n");

  for (auto& p : this->modules)
    delete p.second;

  this->config.save();

  // SDL Cleanup
  SDL_GameControllerClose(this->sdl_common.controller);
  SDL_Quit();

  delete this->shared;

  delete this->nes;

  printf("\nANESE closed successfully\n");
}

void SDL_GUI::input_global(const SDL_Event& event) {
  if (
    (event.type == SDL_QUIT) ||
    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
  ) this->running = false;
}

int SDL_GUI::run() {
  fprintf(stderr, "[SDL2] Running SDL2 GUI\n");

  // Load ROM if one has been passed as param
  if (this->config.cli.rom != "") {
    this->status.in_menu = false;
    int error = this->shared->load_rom(this->config.cli.rom.c_str());
    if (error)
      return error;
  }

  double past_fups [20] = {60.0}; // more samples == less value jitter
  uint past_fups_i = 0;

  while (this->running) {
    typedef uint time_ms;
    time_ms frame_start_time = SDL_GetTicks();
    past_fups_i++;

    // Check for new events
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      this->input_global(event);

      for (auto& p : this->modules) {
        if (event.window.windowID == p.second->get_window_id())
          p.second->input(event);
      }
    }

    // Calculate the number of frames to render
    // Speedup values that are not multiples of 100 cause every-other frame to
    // render 1 more/less frame than usual
    uint numframes = 0;
    this->speed_counter += this->nes_params.speed;
    while (this->speed_counter > 0) {
      this->speed_counter -= 100;
      numframes++;
    }

    // Run ANESE for some number of frames
    for (uint i = 0; i < numframes; i++) {
      if (!this->status.in_menu)
        this->nes->step_frame();

      // Update modules
      for (auto& p : this->modules)
        p.second->update();
    }

    // Render stuff!
    for (auto& p : this->modules)
      p.second->output();

    // time how long all-that took
    time_ms frame_end_time = SDL_GetTicks();

    // ---- Count Framerate ---- //
    // Update fups for this frame
    past_fups[past_fups_i % 20] = 1000.0 / (frame_end_time - frame_start_time);

    // Get the average fups over the past 20 frames
    double avg_fps = 0;
    for(unsigned i = 0; i < 20; i++)
      avg_fps += past_fups[i];
    avg_fps /= 20;

    this->status.avg_fps = avg_fps;
  }

  return 0;
}

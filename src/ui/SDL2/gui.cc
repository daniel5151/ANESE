#include "gui.h"

#include <cstdio>

#include "common/util.h"

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
    this->sdl_common,
    this->config,
    this->nes_params,
    *this->nes,
    this->cart
  );

  this->emu  = new EmuModule(*this->shared);
  this->menu = new MenuModule(*this->shared, *this->emu);

  if (this->config.cli.ppu_debug)
    this->ppu_debug = new PPUDebugModule(*this->shared);

  if (this->config.cli.widenes)
    this->widenes = new WideNESModule(*this->shared);
}

SDL_GUI::~SDL_GUI() {
  fprintf(stderr, "[SDL2] Stopping SDL2 GUI\n");

  // order matters (menu has ref to emu)
  delete this->menu;
  delete this->widenes;
  delete this->emu;

  this->config.save();

  // SDL Cleanup
  // SDL_CloseAudioDevice(this->sdl_common.nes_audiodev);
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
  // TODO: this should probably go somewhere else...
  if (this->config.cli.rom != "") {
    this->menu->in_menu = false;
    int error = this->menu->load_rom(this->config.cli.rom.c_str());
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

      if (event.window.windowID == this->emu->get_window_id()) {
        this->emu->input(event);
        this->menu->input(event);
      }

      if (this->widenes && event.window.windowID == this->widenes->get_window_id()) {
        this->widenes->input(event);
        // send emulator actions too
        if (!this->menu->in_menu)
          this->emu->input(event);
      }

      if (this->ppu_debug && event.window.windowID == this->ppu_debug->get_window_id())
        this->ppu_debug->input(event);
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
      if (!this->menu->in_menu)
        this->nes->step_frame();

      // Update modules

      if (!this->menu->in_menu)
        this->emu->update();
      else
        this->menu->update();

      if (this->ppu_debug) this->ppu_debug->update();
      if (this->widenes) this->widenes->update();
    }

    // Render stuff!
    this->emu->output();
    this->menu->output();
    SDL_RenderPresent(this->emu->sdl.renderer);
    if (this->ppu_debug) this->ppu_debug->output(); // presents for itself
    if (this->widenes) this->widenes->output(); // presents for itself

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

    // Present fups though the title of the main window
    char window_title [64];
    sprintf(window_title, "anese - %u fups - %u%% speed",
      uint(avg_fps), this->nes_params.speed);
    SDL_SetWindowTitle(this->emu->sdl.window, window_title);
  }

  return 0;
}

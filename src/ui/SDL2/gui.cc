#include "gui.h"

#include <cstdio>

#include <SDL2_inprint.h>

#include "common/util.h"

int SDL_GUI::init(int argc, char* argv[]) {
  this->config.load(argc, argv);

  // --------------------------- Init SDL2 Common --------------------------- //

  fprintf(stderr, "[SDL2] Initializing SDL2 GUI\n");

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

  /* Open the first available controller. */
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      this->sdl_common.controller = SDL_GameControllerOpen(i);
    }
  }

  /*----------  Init GUI modules  ----------*/
  this->emu  = new EmuModule(this->sdl_common, this->config);
  this->widenes = new WideNESModule(this->sdl_common, this->config, *this->emu);
  this->menu = new MenuModule(this->sdl_common, this->config, *this->emu);

  // Load ROM if one has been passed as param
  // TODO: this really should go somewhere else...
  if (this->config.cli.rom != "") {
    this->menu->in_menu = false;
    int error = this->emu->load_rom(this->config.cli.rom.c_str());
    if (error)
      return error;
  }

  return 0;
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

  printf("\nANESE closed successfully\n");
}

void SDL_GUI::input_global(const SDL_Event& event) {
  if (
    (event.type == SDL_QUIT) ||
    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
  ) this->sdl_common.running = false;
}

int SDL_GUI::run() {
  fprintf(stderr, "[SDL2] Running SDL2 GUI\n");

  double past_fups [20] = {60.0}; // more samples == less value jitter
  uint past_fups_i = 0;

  while (this->sdl_common.running) {
    typedef uint time_ms;
    time_ms frame_start_time = SDL_GetTicks();
    past_fups_i++;

    // Check for new events
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      this->input_global(event);

      if (event.window.windowID == this->emu->get_window_id()) {
        if (!this->menu->in_menu)
          this->emu->input(event);
        else
          this->menu->input(event);
      }

      if (event.window.windowID == this->widenes->get_window_id()) {
        this->widenes->input(event);
        // send emulator actions too
        if (!this->menu->in_menu)
          this->emu->input(event);
      }
    }

    // Update the NES when not in menu
    if (!this->menu->in_menu)
      this->emu->update();
    else
      this->menu->update();

    this->widenes->update();

    // Render something
    this->emu->output(); // keep showing NES in the background
    if (this->menu->in_menu) {
      this->menu->output();
    }

    this->widenes->output(); // presents for itself

    // SHOW ME WHAT YOU GOT
    SDL_RenderPresent(this->emu->sdl.renderer);

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
      uint(avg_fps), this->emu->params.speed);
    SDL_SetWindowTitle(this->emu->sdl.window, window_title);
  }

  return 0;
}

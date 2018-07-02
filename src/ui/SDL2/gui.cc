#include "gui.h"

#include <cstdio>

#include <SDL2_inprint.h>

#include "common/util.h"

int SDL_GUI::init(int argc, char* argv[]) {
  this->config.load(argc, argv);

  // --------------------------- Init SDL2 Common --------------------------- //

  fprintf(stderr, "[SDL2] Initializing SDL2 GUI\n");

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

  this->sdl_common.window = SDL_CreateWindow(
    "anese",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    this->sdl_common.RES_X * this->config.window_scale,
    this->sdl_common.RES_Y * this->config.window_scale,
    SDL_WINDOW_RESIZABLE
  );

  this->sdl_common.renderer = SDL_CreateRenderer(
    this->sdl_common.window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  // Letterbox the screen in the window
  SDL_RenderSetLogicalSize(this->sdl_common.renderer,
    this->sdl_common.RES_X * this->sdl_common.SCREEN_SCALE,
    this->sdl_common.RES_Y * this->sdl_common.SCREEN_SCALE);
  // Allow opacity when drawing menu
  SDL_SetRenderDrawBlendMode(this->sdl_common.renderer, SDL_BLENDMODE_BLEND);

  /* Open the first available controller. */
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      this->sdl_common.controller = SDL_GameControllerOpen(i);
    }
  }

  // SDL_AudioSpec as, have;
  // as.freq = SDL_GUI::SAMPLE_RATE;
  // as.format = AUDIO_F32SYS;
  // as.channels = 1;
  // as.samples = 4096;
  // as.callback = nullptr; // use SDL_QueueAudio
  // this->sdl_common.nes_audiodev = SDL_OpenAudioDevice(NULL, 0, &as, &have, 0);
  // SDL_PauseAudioDevice(this->sdl_common.nes_audiodev, 0);

  // Setup SDL2_inprint font
  SDL2_inprint::inrenderer(this->sdl_common.renderer);
  SDL2_inprint::prepare_inline_font();

  /*----------  Init GUI modules  ----------*/
  this->emu  = new EmuModule(this->sdl_common, this->config);
  this->menu = new MenuModule(this->sdl_common, this->config, *this->emu);

  // Load ROM if one has been passed as param
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
  delete this->emu;

  this->config.save();

  // SDL Cleanup
  // SDL_CloseAudioDevice(this->sdl_common.nes_audiodev);
  SDL_GameControllerClose(this->sdl_common.controller);
  SDL_DestroyRenderer(this->sdl_common.renderer);
  SDL_DestroyWindow(this->sdl_common.window);
  SDL_Quit();

  SDL2_inprint::kill_inline_font();

  printf("\nANESE closed successfully\n");
}

void SDL_GUI::input_global(const SDL_Event& event) {
  if (
    (event.type == SDL_QUIT) ||
    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
  ) this->sdl_common.running = false;

  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        this->menu->in_menu = !this->menu->in_menu; break;
    }
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN ||
      event.type == SDL_CONTROLLERBUTTONUP) {
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
      this->menu->in_menu = !this->menu->in_menu; break;
    }
  }
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

      if (!this->menu->in_menu)
        this->emu->input(event);
      else
        this->menu->input(event);
    }

    // Update the NES when not in menu
    if (!this->menu->in_menu)
      this->emu->update();
    else
      this->menu->update();

    // Render something
    this->emu->output(); // keep showing NES in the background
    if (this->menu->in_menu) {
      this->menu->output();
    }

    // SHOW ME WHAT YOU GOT
    SDL_RenderPresent(this->sdl_common.renderer);

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
    SDL_SetWindowTitle(this->sdl_common.window, window_title);
  }

  return 0;
}

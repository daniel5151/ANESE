#include "gui.h"

int main(int argc, char* argv[]) {
  SDL_GUI gui;
  if (int error = gui.init(argc, argv))
    return error;
  return gui.run();
}

int SDL_GUI::run() {
  fprintf(stderr, "[SDL2] Running SDL2 GUI\n");

  double past_fups [20] = {60.0}; // more samples == less value jitter
  uint past_fups_i = 0;

  while (this->sdl_running) {
    typedef uint time_ms;
    time_ms frame_start_time = SDL_GetTicks();
    past_fups_i++;

    // Check for new events
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      this->update_input_global(event);

      if (!this->ui.in_menu) {
        this->update_input_nes_misc(event);
        this->update_input_nes_joypads(event);
      } else {
        this->update_input_ui(event);
      }
    }

    // Update the NES when not in menu
    if (!this->ui.in_menu)
      this->step_nes();

    // Render something
    this->output_nes();
    if (this->ui.in_menu) {
      this->output_menu();
    }

    // SHOW ME WHAT YOU GOT
    SDL_RenderPresent(this->sdl.renderer);

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
      uint(avg_fps), this->nes.speedup);
    SDL_SetWindowTitle(this->sdl.window, window_title);
  }

  return 0;
}

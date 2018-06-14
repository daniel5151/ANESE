#include "gui.h"

/**
 * @brief Run NES for appropriate number of frames
 * @details Actual number of frames emulated depends on speed-multiplier
 */
void SDL_GUI::step_nes() {
  // Calculate the number of frames to render
  // (speedup values that are not clean multiples of 100 imply that some the
  //  number of nes frames / real frame will vary)
  uint numframes = 0;
  this->nes.speed_counter += this->nes.speedup;
  while (this->nes.speed_counter > 0) {
    this->nes.speed_counter -= 100;
    numframes++;
  }

  // Run ANESE core for some number of frames
  for (uint i = 0; i < numframes; i++) {
    // log frame to fm2
    if (this->nes.fm2_record.is_enabled())
      this->nes.fm2_record.step_frame();

    // set input from fm2
    if (this->nes.fm2_replay.is_enabled())
      this->nes.fm2_replay.step_frame();

    // run the NES for a frame
    this->nes.console.step_frame();
  }

  if (this->nes.console.isRunning() == false) {
    // this->sdl_running = true;
  }
}

/**
 * @brief Render NES Framebuffer
 */
void SDL_GUI::output_nes() {
  // output audio!
  short* samples = nullptr;
  uint   count = 0;
  this->nes.console.getAudiobuff(samples, count);
  if (count) this->sdl.nes_sound_queue.write(samples, count);

  // output video!
  const u8* framebuffer = this->nes.console.getFramebuff();
  SDL_UpdateTexture(this->sdl.nes_texture, nullptr, framebuffer, RES_X * 4);
  SDL_RenderCopy(this->sdl.renderer, this->sdl.nes_texture, nullptr, &this->sdl.nes_screen);
}

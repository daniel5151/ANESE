#include "gui.h"

namespace SDL2_inprint {
extern "C" {
#include <SDL2_inprint.h>
}
}

/**
 * @brief Render Menu
 */
void SDL_GUI::output_menu() {
  // Paint transparent bg
  this->sdl.bg.x = this->sdl.bg.y = 0;
  SDL_RenderGetLogicalSize(this->sdl.renderer, &this->sdl.bg.w, &this->sdl.bg.h);
  SDL_SetRenderDrawColor(this->sdl.renderer, 0, 0, 0, 200);
  SDL_RenderFillRect(this->sdl.renderer, &this->sdl.bg);

  // Paint menu
  for (uint i = 0; i < this->ui.menu.files.size(); i++) {
    const cf_file_t& file = this->ui.menu.files[i];

    u32 color;
    if (this->ui.menu.selected_i == i)     color = 0xff0000; // red   - selected
    else if (strcmp("nes", file.ext) == 0) color = 0x00ff00; // green - .nes
    else if (strcmp("zip", file.ext) == 0) color = 0x00ffff; // cyan  - .zip
    else                                   color = 0xffffff; // white - folder

    SDL2_inprint::incolor(color, /* unused */ 0);
    SDL2_inprint::inprint(this->sdl.renderer, file.name,
      10, this->sdl.bg.h / SCREEN_SCALE + (i - this->ui.menu.selected_i) * 12
    );
  }
}

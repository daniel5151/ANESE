#ifndef SDL2_inprint_h
#define SDL2_inprint_h

#include <SDL.h>

namespace SDL2_inprint {
extern "C" {

void prepare_inline_font(void);
void kill_inline_font(void);

void inrenderer(SDL_Renderer *renderer);
void infont(SDL_Texture *font);
void incolor1(SDL_Color *color);
void incolor(Uint32 color, Uint32 unused); /* Color must be in 0x00RRGGBB format ! */
void inprint(SDL_Renderer *dst, const char *str, Uint32 x, Uint32 y);

SDL_Texture *get_inline_font(void);

}
}

#endif /* SDL2_inprint_h */

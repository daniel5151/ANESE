#include "SDL_inprint2.h"

#include <SDL.h>

#include "inline_font.h" /* Actual font data */

#define CHARACTERS_PER_ROW    16 /* I like 16 x 8 fontsets. */
#define CHARACTERS_PER_COLUMN 8  /* 128 x 1 is another popular format. */

SDL2_inprint::SDL2_inprint(SDL_Renderer *renderer) {
  selected_renderer = renderer;

  Uint32 *pix_ptr, tmp;
  int i, len, j;
  SDL_Surface *surface;
  Uint32 colors[2];

  selected_font_w = inline_font_width;
  selected_font_h = inline_font_height;

  if (inline_font != NULL) { selected_font = inline_font; return; }

  surface = SDL_CreateRGBSurface(0, inline_font_width, inline_font_height, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
  0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
  );
  colors[0] = SDL_MapRGBA(surface->format, 0xFF, 0xFF, 0xFF, 0xFF);
  colors[1] = SDL_MapRGBA(surface->format, 0x00, 0x00, 0x00, 0x00 /* or 0xFF, to have bg-color */);

  /* Get pointer to pixels and array length */
  pix_ptr = (Uint32 *)surface->pixels;
  len = surface->h * surface->w / 8;

  /* Copy */
  for (i = 0; i < len; i++)
  {
    tmp = (Uint8)inline_font_bits[i];
    for (j = 0; j < 8; j++)
    {
      Uint8 mask = (0x01 << j);
      pix_ptr[i * 8 + j] = colors[(tmp & mask) >> j];
    }
  }

  inline_font = SDL_CreateTextureFromSurface(selected_renderer, surface);
  SDL_FreeSurface(surface);

  selected_font = inline_font;
}

SDL2_inprint::~SDL2_inprint() {
  SDL_DestroyTexture(inline_font);
}

void SDL2_inprint::set_color(Uint32 color) {
  SDL_Color pal[1];
  pal[0].r = (Uint8)((color & 0x00FF0000) >> 16);
  pal[0].g = (Uint8)((color & 0x0000FF00) >> 8);
  pal[0].b = (Uint8)((color & 0x000000FF));
  SDL_SetTextureColorMod(selected_font, pal[0].r, pal[0].g, pal[0].b);
}

void SDL2_inprint::print(const char* str, Uint32 x, Uint32 y) {
  SDL_Rect s_rect;
  SDL_Rect d_rect;

  d_rect.x = x;
  d_rect.y = y;
  s_rect.w = selected_font_w / CHARACTERS_PER_ROW;
  s_rect.h = selected_font_h / CHARACTERS_PER_COLUMN;
  d_rect.w = s_rect.w;
  d_rect.h = s_rect.h;

  for (; *str; str++)
  {
    int id = (int)*str;
#if (CHARACTERS_PER_COLUMN != 1)
    int row = id / CHARACTERS_PER_ROW;
    int col = id % CHARACTERS_PER_ROW;
    s_rect.x = col * s_rect.w;
    s_rect.y = row * s_rect.h;
#else
    s_rect.x = id * s_rect.w;
    s_rect.y = 0;
#endif
    if (id == '\n')
    {
      d_rect.x = x;
      d_rect.y += s_rect.h;
      continue;
    }
    SDL_RenderCopy(selected_renderer, selected_font, &s_rect, &d_rect);
    d_rect.x += s_rect.w;
  }
}

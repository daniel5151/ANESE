#include "debug.h"

/*----------------------------------------------------------------------------*/

u8 Void_Memory::read(u16 addr)       { (void)addr; return 0x00; };
u8 Void_Memory::peek(u16 addr) const { (void)addr; return 0x00; };
void Void_Memory::write(u16 addr, u8 val) { (void)addr; (void)val; };

Void_Memory* Void_Memory::Get() {
  static Void_Memory* the_void = nullptr;

  if (!the_void) the_void = new Void_Memory ();
  return the_void;
}

/*----------------------------------------------------------------------------*/

Memory_Sniffer::Memory_Sniffer(const char* label, Memory* mem)
: label(label),
  mem(mem)
{}

u8 Memory_Sniffer::read(u16 addr) {
  if (this->mem == nullptr) {
    printf(
      "[%s] Underlying Memory is nullptr!\n",
      this->label
    );
    return 0x00;
  }

  // only read once, to prevent side effects
  u8 val = this->mem->read(addr);
  printf("[%s] R 0x%04X -> 0x%02X\n", this->label, addr, val);
  return val;
}

u8 Memory_Sniffer::peek(u16 addr) const {
  if (this->mem == nullptr) {
    printf(
      "[%s] Underlying Memory is nullptr!\n",
      this->label
    );
    return 0x00;
  }

  u8 val = this->mem->peek(addr);
  printf("[%s] P 0x%04X -> 0x%02X\n", this->label, addr, val);
  return val;
}

void Memory_Sniffer::write(u16 addr, u8 val) {
  if (this->mem == nullptr) {
    printf(
      "[%s] Underlying Memory is nullptr!\n",
      this->label
    );
    return;
  }

  printf("[%s] W 0x%04X <- 0x%02X\n", this->label, addr, val);
  this->mem->write(addr, val);
}

/*----------------------------------------------------------------------------*/

OffsetMemory::OffsetMemory(Memory* mem, i32 diff) : mem {mem}, diff {diff} {};

u8 OffsetMemory::read(u16 addr)            { return this->mem->read(addr - diff); }
u8 OffsetMemory::peek(u16 addr) const      { return this->mem->peek(addr - diff); }
void OffsetMemory::write(u16 addr, u8 val) { this->mem->write(addr - diff, val);  }

/*----------------------------------------------------------------------------*/

DebugPixelbuffWindow::~DebugPixelbuffWindow() {
  delete this->frame;

  SDL_DestroyTexture(this->texture);
  SDL_DestroyRenderer(this->renderer);
  SDL_DestroyWindow(this->window);
}

DebugPixelbuffWindow::DebugPixelbuffWindow(const char* title, uint w, uint h, uint x, uint y) {
  SDL_Init(SDL_INIT_EVERYTHING);

  this->w = w;
  this->h = h;
  this->x = x;
  this->y = y;

  this->window = SDL_CreateWindow(
      title,
      x, y,
      w * 2, h * 2,
      SDL_WINDOW_RESIZABLE
  );

  this->renderer = SDL_CreateRenderer(
    this->window,
    -1,
    SDL_RENDERER_ACCELERATED
  );

  this->texture = SDL_CreateTexture(
    this->renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    w, h
  );

  this->frame = new u8 [w * h * 4];
}

void DebugPixelbuffWindow::set_pixel(uint x, uint y, u8 r, u8 g, u8 b, u8 a) {
  const uint offset = ((this->w * y) + x) * 4;
  this->frame[offset + 0] = b;
  this->frame[offset + 1] = g;
  this->frame[offset + 2] = r;
  this->frame[offset + 3] = a;
}

void DebugPixelbuffWindow::set_pixel(uint x, uint y, u32 color, u8 a) {
  const uint offset = ((this->w * y) + x) * 4;
  this->frame[offset + 0] = color & 0x0000FF >> 0;
  this->frame[offset + 1] = color & 0x00FF00 >> 4;
  this->frame[offset + 2] = color & 0xFF0000 >> 8;
  this->frame[offset + 3] = a;
}


void DebugPixelbuffWindow::render() {
  // Update the screen texture
  SDL_UpdateTexture(this->texture, nullptr, this->frame, 0x110 * 4);

  // Render everything
  SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);

  // Reveal out the composited image
  SDL_RenderPresent(this->renderer);
}

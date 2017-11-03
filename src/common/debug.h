//
// A real mish-mash of useful debug constructs
//

#pragma once

#include <cassert>
#include <cstdio>

#include "interfaces/memory.h"
#include "util.h"

// Void Memory Singleton
// Returns 0x00 on read
// No effect on write
class Void_Memory : public Memory {
private:
  Void_Memory() = default;

public:
  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  static Void_Memory* Get();
};

// Wrapper that transaparently intercepts all transactions that occur through a
// given Memory* and logs them.
//
// Kind of annoying since there is no GC to automatically clean these guys up,
// but ah well, it'll do for development :)
class Memory_Sniffer final : public Memory {
private:
  const char* label;
  Memory* mem;

public:
  Memory_Sniffer(const char* label, Memory* mem);

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // <Memory/>
};

// Wrapper that offsets all memory accesses through it by some difference
class OffsetMemory final : public Memory {
private:
  Memory* mem;
  i32 diff;

public:
  OffsetMemory(Memory* mem, i32 diff);

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // <Memory/>
};

#include <SDL.h>

// Simple SDL debug window that is pixel-addressable
struct DebugPixelbuffWindow {
  SDL_Window*   window;
  SDL_Renderer* renderer;
  SDL_Texture*  texture;

  u8* frame;

  uint w, h, x, y;

  ~DebugPixelbuffWindow();
  DebugPixelbuffWindow(const char* title, uint w, uint h, uint x, uint y);
  DebugPixelbuffWindow(const DebugPixelbuffWindow&) = delete;

  void set_pixel(uint x, uint y, u8 r, u8 g, u8 b, u8 a);
  void set_pixel(uint x, uint y, u32 rbg, u8 a);

  void render();
};

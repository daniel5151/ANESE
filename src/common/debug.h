//
// A real mish-mash of useful debug constructs
//

#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "nes/interfaces/memory.h"
#include "util.h"

// Extremely nasty debug variable singleton
struct DEBUG_VARS {
private:
  DEBUG_VARS() = default;
public:
  static DEBUG_VARS* Get() {
    static DEBUG_VARS* the_thing = nullptr;

    if (!the_thing) the_thing = new DEBUG_VARS ();
    return the_thing;
  }
public:

  bool print_nestest = false;
  bool step_cpu = false;

  bool fogleman_hack = false;

};

// Memory Singleton Template
// behavior defined by given function
template <u8 (*F)()>
class Func_Memory : public Memory {
private:
  Func_Memory() = default;

public:
  // <Memory>
  u8 read(u16 addr)            override { (void)addr; return F(); };
  u8 peek(u16 addr) const      override { (void)addr; return F(); };
  void write(u16 addr, u8 val) override { (void)addr; (void)val;  };
  // <Memory/>

  static Func_Memory* Get() {
    static Func_Memory* the_thing = nullptr;

    if (!the_thing) the_thing = new Func_Memory ();
    return the_thing;
  }
};

// Some concrete singletons
namespace Func_Memory_Functions {
inline u8 give_me_rand() { return u8(rand()); }
inline u8 give_me_zero() { return 0x00; }
inline u8 give_me_full() { return 0xFF; }
}

static auto init = [](){ srand(0xBADA55); };
typedef Func_Memory<Func_Memory_Functions::give_me_zero> Void_Memory;
typedef Func_Memory<Func_Memory_Functions::give_me_full> Full_Memory;
typedef Func_Memory<Func_Memory_Functions::give_me_rand> Rand_Memory;

// Map Memory
// Uses STL map to store memory
#include <map>
class Map_Memory : public Memory {
private:
  std::map<u16, u8> mem;

public:
  Map_Memory() = default;

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // <Memory/>
};

// Wrapper that transparently intercepts all transactions that occur through a
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

  uint window_w, window_h;
  uint texture_w, texture_h;
  uint x, y;

  ~DebugPixelbuffWindow();
  DebugPixelbuffWindow(
    const char* title,
    uint window_w, uint window_h,
    uint texture_w, uint texture_h,
    uint x, uint y
  );
  DebugPixelbuffWindow(const DebugPixelbuffWindow&) = delete;

  void set_pixel(uint x, uint y, u8 r, u8 g, u8 b, u8 a);
  void set_pixel(uint x, uint y, u32 rbg);

  void render();
};

//
// A collection of useful Debug tools
//

#pragma once

#include <cassert>
#include <cstdio>

#include "interfaces/memory.h"
#include "util.h"

// Void Memory Singleton
// Returns 0 on read
// No effect on write
class Void_Memory : public Memory {
private:
  Void_Memory() = default;

public:
  // <Memory>
  u8 read(u16 addr)       override { return 0; };
  u8 peek(u16 addr) const override { return 0; };
  void write(u16 addr, u8 val) override {};
  // <Memory/>

  static Void_Memory* Get() {
    static Void_Memory the_void;
    return &the_void;
  }
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
  Memory_Sniffer(const char* label, Memory* mem)
  : label(label),
    mem(mem)
  {}

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>
};

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
};

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
};

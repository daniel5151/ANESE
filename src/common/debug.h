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
  u8 read(u16 addr) override { return 0; };
  void write(u16 addr, u8 val) override {};

  static Void_Memory* Get() {
    static Void_Memory the_void;
    return &the_void;
  }
};

// Wrapper that transaparently intercepts all transactions that occur through a
// given Memory* and logs them

// NOT A PROPER LONG TERM SOLUTION!
// But it's good enough to use during development.
class Memory_Sniffer final : public Memory {
private:
  const char* label;
  Memory* mem;

public:
  Memory_Sniffer(const char* label, Memory* mem)
  : label(label),
    mem(mem)
  {}

  u8 read(u16 addr) override {
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

  void write(u16 addr, u8 val) override {
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
};

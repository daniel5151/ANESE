#pragma once

#include "common/util.h"

// 16 bit addressable memory interface
class Memory {
public:
  virtual u8 peek(u16 addr) const = 0; // read, but without the side-effects
  virtual u8 read(u16 addr) = 0;
  virtual void write(u16 addr, u8 val) = 0;

  // Derived memory access methods

  u16 peek16(u16 addr) const {
    return this->peek(addr + 0) |
          (this->peek(addr + 1) << 8);
  }
  u16 read16(u16 addr) {
  	return this->read(addr + 0) |
          (this->read(addr + 1) << 8);
  };

  u16 peek16_zpg(u16 addr) const {
    return this->peek(addr + 0) |
          (this->peek((addr & 0xFF00) | (addr + 1 & 0x00FF)) << 8);
  }
  u16 read16_zpg(u16 addr) {
    return this->read(addr + 0) |
          (this->read((addr & 0xFF00) | (addr + 1 & 0x00FF)) << 8);
  }

  void write16(u16 addr, u8 val) {
    this->write(addr + 0, val);
    this->write(addr + 1, val);
  }

};

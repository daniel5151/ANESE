#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "common/serializable.h"

// Thin bridge class that facilitates CPU -> PPU OAMDMA, without giving the PPU
// direct access to the CPU MMU
class DMA final : public Serializable {
private:
  Memory& cpu_mmu;

  u16 addr = 0x0000; // CPU addr to read from

  SERIALIZE_START(1, "DMA")
    SERIALIZE_POD(addr)
  SERIALIZE_END(1)

public:
  DMA(Memory& cpu_mmu) : cpu_mmu(cpu_mmu) {}

  // Set start-page for DMA
  void start(u8 page) {
  	this->addr = u16(page) << 8;
  }

  // Return a byte from CPU, and increment read-address for subsequent calls
  u8 transfer() {
  	return this->cpu_mmu[this->addr++];
  }
};

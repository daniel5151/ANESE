#pragma once

#include "cartridge/cartridge.h"
#include "common/components/ram/ram.h"
#include "common/util.h"
#include "cpu/cpu.h"
#include "cpu/cpu_mmu.h"

// Core NES class.
// - Owns all NES core resources (but NOT the cartridge)
// - Runs CPU, PPU, APU
class NES {
private:
  /*================================
  =            Hardware            =
  ================================*/

  /*----------  Borrowed Resources  -----------*/
  // Not owned by NES

  Cartridge* cart;

  /*----------  Volatile Resources  -----------*/
  // Fixed components, but have state

  // Processors
  CPU* cpu;
  // PPU* ppu;

  // RAM
  RAM* cpu_ram;   // 2k CPU general purpose RAM
  RAM* ppu_pram;  // 32 bytes PPU palette RAM
  RAM* ppu_ciram; // 2k PPU nametable VRAM

  // Joypads
  // JOY* joy;

  /*-----------  Static Resources  ------------*/
  // Fixed, non-stateful "wiring"

  CPU_MMU* cpu_mmu;
  // PPU_MMU* ppu_mmu;

  // DMA* dma;

  /*=====================================
  =            Emulator Vars            =
  =====================================*/

  bool is_running;
  u32 clock_cycles;

public:
  NES();
  ~NES();

  bool loadCartridge(Cartridge* cart);

  void power_cycle(); // Set all volatile components to default power_on state
  void reset();       // Set all volatile components to default reset state

  void step_frame(); // Step processors

  bool isRunning() const;
};

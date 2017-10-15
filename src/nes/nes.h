#pragma once

#include "cartridge/cartridge.h"
#include "common/util.h"
#include "cpu_mmu.h"
#include "ram/ram.h"
#include "cpu/cpu.h"

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
  RAM* cpu_ram;
  // RAM* ppu_ram;

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

public:
  NES();
  ~NES();

  bool loadCartridge(Cartridge* cart);

  void power_cycle(); // Set all volatile components to default power_on state
  void reset();       // Set all volatile components to default reset state

  void start(); // Begin execution
  void stop();  // Stop Execution

  bool isRunning() const;
};

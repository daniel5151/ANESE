#pragma once

#include "cartridge/cartridge.h"
#include "common/components/ram/ram.h"
#include "common/util.h"
#include "cpu/cpu.h"
#include "cpu/cpu_mmu.h"
#include "ppu/ppu.h"
#include "ppu/ppu_mmu.h"

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
  PPU* ppu;

  // RAM
  RAM* cpu_wram; // 2k CPU general purpose Work RAM
  RAM* ppu_vram; // 2k PPU nametable VRAM
  RAM* ppu_pram; // 32 bytes PPU palette RAM

  // Joypads
  // JOY* joy;

  /*-----------  Static Resources  ------------*/
  // Fixed, non-stateful "wiring"

  CPU_MMU* cpu_mmu;
  PPU_MMU* ppu_mmu;

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

  void cycle();      // Run a single clock cycle
  void step_frame(); // Run the NES until there is a new frame to display
                     // (calls cycle() internally)

  const u8* getFrame() const;

  bool isRunning() const;
};

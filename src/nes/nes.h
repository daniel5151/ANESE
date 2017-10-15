#pragma once

#include "cartridge/cartridge.h"
#include "common/util.h"
#include "cpu_mmu.h"
#include "ram/ram.h"

// Core NES class.
// - Owns all NES core resources (but NOT the cartridge)
// - Runs CPU, PPU, APU
class NES {
private:
  /*============================
  =            Data            =
  ============================*/

  /*----------  Borrowed Resources  -----------*/

  Cartridge* cart;

  /*------------  Owned Resources  ------------*/

  // CPU* cpu;
  RAM*     cpu_ram;
  CPU_MMU* cpu_mmu;

  // PPU* ppu;
  // RAM* ppu_ram;
  // PPU_MMU* ppu_mmu;

  // DMA* dma;

  // JOY* joy;

  /*-----------------  Flags  -----------------*/

  bool is_running;

  /*=====  End of Data  ======*/
public:
  NES();
  ~NES();

  bool loadCartridge(Cartridge* cart);

  void start();
  void stop();

  bool isRunning() const;
};

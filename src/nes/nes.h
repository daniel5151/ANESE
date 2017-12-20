#pragma once

#include "common/util.h"

#include "cartridge/cartridge.h"
#include "nes/generic/ram/ram.h"
#include "joy/joy.h"
#include "apu/apu.h"
#include "cpu/cpu.h"
#include "ppu/dma.h"
#include "ppu/ppu.h"
#include "wiring/cpu_mmu.h"
#include "wiring/ppu_mmu.h"
#include "wiring/interrupt_lines.h"

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
  APU* apu;
  PPU* ppu;

  // RAM
  RAM* cpu_wram; // 2k CPU general purpose Work RAM
  RAM* ppu_vram; // 2k PPU nametable VRAM
  RAM* ppu_pram; // 32 bytes PPU palette RAM
  RAM* ppu_oam;  // 256 bytes PPU Object Attribute Memory (OAM)
  RAM* ppu_oam2; // 32 bytes secondary OAM

  // Joypad controller
  JOY* joy;

  /*-----------  Static Resources  ------------*/
  // Fixed, non-stateful compnents

  InterruptLines interrupts;

  CPU_MMU* cpu_mmu;
  PPU_MMU* ppu_mmu;

  DMA* dma;

  /*=====================================
  =            Emulator Vars            =
  =====================================*/

  uint speed;

  bool is_running;

public:
  ~NES();
  NES();

  // Until I am at a point where I can serialize the state of the NES, i'm
  // disallowing copying NES instances
  NES(const NES&) = delete;

  /*-----------  Key Operation Functions  ------------*/

  bool loadCartridge(Cartridge* cart);
  void removeCartridge();

  void attach_joy(uint port, Memory* joy);
  void detach_joy(uint port);

  void power_cycle(); // Set all volatile components to default power_on state
  void reset();       // Set all volatile components to default reset state

  void cycle();      // Run a single clock cycle
  void step_frame(); // Run the NES until there is a new frame to display
                     // (calls cycle() internally)

  const u8* getFramebuff() const;
  void getAudiobuff(short*& samples, uint& len);

  bool isRunning() const;

  /*-----------  "Fun" Options  ------------*/
  // i.e: functions that do things the original hardware doesn't

  void set_speed(uint speed);
};

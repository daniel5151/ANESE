#pragma once

#include "common/serializable.h"
#include "common/util.h"

#include "cartridge/mapper.h"
#include "nes/generic/ram/ram.h"
#include "joy/joy.h"
#include "apu/apu.h"
#include "cpu/cpu.h"
#include "ppu/dma.h"
#include "ppu/ppu.h"
#include "wiring/cpu_mmu.h"
#include "wiring/ppu_mmu.h"
#include "wiring/interrupt_lines.h"

#include "params.h"

// Core NES class.
// - Owns all NES core resources (but NOT the cartridge or joypads)
// - Runs CPU, PPU, APU
class NES final : public Serializable {
private:
  /*================================
  =            Hardware            =
  ================================*/

  /*----------  External Hardware  ----------*/
  // I.e: Things not present on the NES mainboard

  Mapper* cart = nullptr; // Game Cartridge

  /*----------  Chips  ----------*/
  RAM cpu_wram; // 2k CPU general purpose Work RAM
  RAM ppu_vram; // 2k PPU nametable VRAM
  RAM ppu_pram; // 32 bytes PPU palette RAM

  CPU cpu;
  APU apu;
  PPU ppu;

  /*----------  Wiring  ----------*/

  // MMUs
  CPU_MMU cpu_mmu;
  PPU_MMU ppu_mmu;

  // Joypad router
  JOY joy;

  // DMA Facilitator
  DMA dma;

  // Interrupt wiring
  InterruptLines interrupts;

  /*=====================================
  =            Emulator Vars            =
  =====================================*/

  bool is_running = false;

  SERIALIZE_START(10, "NES")
    SERIALIZE_POD(is_running)
    SERIALIZE_SERIALIZABLE_PTR(cart)
    SERIALIZE_SERIALIZABLE(cpu)
    SERIALIZE_SERIALIZABLE(apu)
    SERIALIZE_SERIALIZABLE(ppu)
    SERIALIZE_SERIALIZABLE(cpu_wram)
    SERIALIZE_SERIALIZABLE(ppu_vram)
    SERIALIZE_SERIALIZABLE(ppu_pram)
    SERIALIZE_SERIALIZABLE(dma)
    SERIALIZE_SERIALIZABLE(interrupts)
  SERIALIZE_END(10)

  const NES_Params& params;
public:
  NES(const NES_Params& new_params);
  void updated_params();

  /*-----------  Key Operation Functions  ------------*/

  bool loadCartridge(Mapper* cart);
  void removeCartridge();

  void attach_joy(uint port, Memory* joy);
  void detach_joy(uint port);

  void power_cycle(); // Set all volatile components to default power_on state
  void reset();       // Set all volatile components to default reset state

  void cycle();      // Run a single clock cycle
  void step_frame(); // Run the NES until there is a new frame to display
                     // (calls cycle() internally)

  void getFramebuff(const u8*& framebuffer) const;
  void getAudiobuff(float*& samples, uint& len);

  bool isRunning() const;
};

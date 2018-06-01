#pragma once

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

#include "nes/interfaces/serializable.h"

// Core NES class.
// - Owns all NES core resources (but NOT the cartridge)
// - Runs CPU, PPU, APU
class NES final : public Serializable {
private:
  /*================================
  =            Hardware            =
  ================================*/

  /*----------  Borrowed Resources  -----------*/
  // Not owned by NES

  Mapper* cart; // Game Cartridge

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

  // DMA Facilitator
  DMA* dma;

  // Interrupt wiring
  InterruptLines interrupts;

  /*-----------  Static Resources  ------------*/
  // Fixed, non-stateful compnents

  CPU_MMU* cpu_mmu;
  PPU_MMU* ppu_mmu;

  /*=====================================
  =            Emulator Vars            =
  =====================================*/

  bool is_running;

  SERIALIZE_START(11, "NES")
    SERIALIZE_POD(is_running)
    SERIALIZE_SERIALIZABLE_PTR(cart)
    SERIALIZE_SERIALIZABLE_PTR(cpu)
    SERIALIZE_SERIALIZABLE_PTR(ppu)
    SERIALIZE_SERIALIZABLE_PTR(cpu_wram)
    SERIALIZE_SERIALIZABLE_PTR(ppu_vram)
    SERIALIZE_SERIALIZABLE_PTR(ppu_pram)
    SERIALIZE_SERIALIZABLE_PTR(ppu_oam)
    SERIALIZE_SERIALIZABLE_PTR(ppu_oam2)
    SERIALIZE_SERIALIZABLE_PTR(dma)
    SERIALIZE_SERIALIZABLE(interrupts)
  SERIALIZE_END(11)

public:
  ~NES();
  NES();

  // Until I am at a point where I can serialize the state of the NES, i'm
  // disallowing copying NES instances
  NES(const NES&) = delete;

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

  const u8* getFramebuff() const;
  void getAudiobuff(short*& samples, uint& len);

  bool isRunning() const;

  /*-----------  "Fun" Functions  ------------*/
  // i.e: toggles things the original hardware doesn't actually support / do

  void set_speed(uint speed);
};

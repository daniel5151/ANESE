#pragma once

#include "common/serializable.h"
#include "common/util.h"

#include "apu/apu.h"
#include "cartridge/mapper.h"
#include "cpu/cpu.h"
#include "generic/ram/ram.h"
#include "joy/joy.h"
#include "ppu/dma.h"
#include "ppu/ppu.h"
#include "wiring/cpu_mmu.h"
#include "wiring/interrupt_lines.h"
#include "wiring/ppu_mmu.h"

#include "params.h"

// Core NES class.
// - Owns all NES core resources (but NOT the cartridge or joypads)
// - Runs CPU, PPU, APU
// - serialize() returns a full save state (including cart)
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

public:
  virtual Serializable::Chunk* serialize() const override {
    Serializable::Chunk* c = this->Serializable::serialize();
    _callbacks.savestate_created.run();
    return c;
  }
  virtual const Serializable::Chunk* deserialize(const Serializable::Chunk* c) override {
    c = this->Serializable::deserialize(c);
    _callbacks.savestate_loaded.run();
    return c;
  }
private:
  const NES_Params& params;
public:
  NES(const NES_Params& new_params);
  void updated_params();

  /*-----------  Key Operation Functions  ------------*/

  bool loadCartridge(Mapper* cart);
  void removeCartridge();

  void attach_joy(uint port, Memory* joy);
  void detach_joy(uint port);

  void power_cycle();
  void reset();

  void cycle();      // Run a single clock cycle
  void step_frame(); // Cycle the NES until there is a new frame to display

  void getFramebuff(const u8** framebuffer) const;
  void getAudiobuff(float** samples, uint* len);

  bool isRunning() const { return this->is_running; }

  /*---------------  Debugging / Instrumentation  --------------*/

  APU& _apu() { return this->apu; }
  CPU& _cpu() { return this->cpu; }
  PPU& _ppu() { return this->ppu; }

  struct {
    CallbackManager<Mapper*> cart_changed;
    CallbackManager<> savestate_created;
    CallbackManager<> savestate_loaded;
  } _callbacks;
};

#pragma once

#include "common/util.h"
#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "nes/interfaces/memory.h"
#include "nes/interfaces/mirroring.h"
#include "rom_file.h"

#include "nes/wiring/interrupt_lines.h"

#include "common/callback_manager.h"
#include "common/serializable.h"

// Base Mapper Interface
// Implements common mapper utilities (eg: bank-chunking, IRQ handling)
// At minimum, Mappers must implement the following methods:
//   - peek/write
//   - update_banks
//   - mirroring
//   - reset
// The following virtual methods implement default behaviors:
//   - read ................. calls peek
//   - get/setBatterySave ... get returns nullptr, set does nothing
//   - cycle ................ does nothing
//   - power_cycle .......... clears CHR RAM, and calls reset + update_banks
class Mapper : public Memory, public Serializable {
private:
  /*---------------------------------  Data  ---------------------------------*/

  // Identifiers
  const char* name;
  const uint  number;

  // Wiring
  InterruptLines* interrupt_line = nullptr;

  // Banks
  struct {
    struct {
      uint  len;
      ROM** bank;
    } prg;

    struct {
      bool is_RAM = false;
      uint len;
      Memory** bank;
    } chr;
  } banks;

  /*----------------------------  Serialization  -----------------------------*/

protected:
  SERIALIZE_START(this->banks.chr.len, "Mapper")
    SERIALIZE_CUSTOM() {
      for (uint j = 0; j < this->banks.chr.len; j++) {
        RAM* chr_bank = this->banks.chr.is_RAM
          ? static_cast<RAM*>(this->banks.chr.bank[j])
          : nullptr;
        SERIALIZE_SERIALIZABLE_PTR(chr_bank)
      }
    }
  SERIALIZE_END(this->banks.chr.len)

  virtual void update_banks() = 0;

  virtual const Serializable::Chunk* deserialize(const Serializable::Chunk* c) override {
    c = this->Serializable::deserialize(c);
    this->update_banks();
    return c;
  }

  /*-------------------------------  Helpers  --------------------------------*/

private:
  void init_prg_banks(const ROM_File& rom_file, const u16 size);
  void init_chr_banks(const ROM_File& rom_file, const u16 size);

  /*-----------------  Common Mapper Functions / Services  -------------------*/

protected:
  // Services provided to all mappers
  void irq_trigger();
  void irq_service();
  uint get_prg_bank_len() const;
  uint get_chr_bank_len() const;
  ROM&    get_prg_bank(uint bank) const;
  Memory& get_chr_bank(uint bank) const;

  /*--------------------------  External Interface  --------------------------*/

public:
  virtual ~Mapper();
  Mapper(
    uint number, const char* name,
    const ROM_File& rom_file,
    u16 prg_bank_size, u16 chr_bank_size
  );

  // Should be called when when inserting / removing cartridges from NES
  void set_interrupt_line(InterruptLines* interrupt_line) {
    this->interrupt_line = interrupt_line;
  }

  // ---- Mapper Queries ---- //
  const char* mapper_name()   const { return this->name;   };
        uint  mapper_number() const { return this->number; };

  // ---- Battery Backed Saving ---- //
  virtual const Serializable::Chunk* getBatterySave() const { return nullptr; }
  virtual void setBatterySave(const Serializable::Chunk* c) { return (void)c; }

  // ---- Callbacks ---- //
  CallbackManager<Mapper*> irq_callbacks;

  /*------------------------  Core Mapper Interface  -------------------------*/

  // reads tend to be side-effect free, except with some fancier mappers
  virtual u8 read(u16 addr) override { return this->peek(addr); }
  virtual u8 peek(u16 addr) const override      = 0;
  virtual void write(u16 addr, u8 val) override = 0;

  virtual Mirroring::Type mirroring() const = 0; // Get mirroring mode

  virtual void cycle() {}

  virtual void power_cycle() {
    // NOTE: there are a couple of boards that have battery-backed CHR RAM.
    // They required a complete override of the power_cycle() method
    // iNES    168
    // NES 2.0 513
    if (this->banks.chr.is_RAM) {
      for (uint j = 0; j < this->banks.chr.len; j++) {
        static_cast<RAM*>(this->banks.chr.bank[j])->clear();
      }
    }
    this->reset();
    this->update_banks();
  };
  virtual void reset() = 0;

public:
  /*------------------------------  Utilities  -------------------------------*/

  // creates correct mapper given an ROM_File object
  static Mapper* Factory(const ROM_File* rom_file);
};

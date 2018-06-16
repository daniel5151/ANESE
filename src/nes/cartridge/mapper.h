#pragma once

#include "common/util.h"
#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "nes/interfaces/memory.h"
#include "nes/interfaces/mirroring.h"
#include "rom_file.h"

#include "nes/wiring/interrupt_lines.h"

#include "common/serializable.h"

/**
 * @brief Base Mapper Interface
 * @details Provides Mapper interface, and provides common mapper functionality
 */
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

  /*-------------------------------  Methods  --------------------------------*/

private:
  void set_prg_banks(const ROM_File& rom_file, const u16 size);
  void set_chr_banks(const ROM_File& rom_file, const u16 size);

protected:
  // Services provided to all mappers
  void irq_trigger() const;
  void irq_service() const;
  uint get_prg_bank_len() const;
  uint get_chr_bank_len() const;
  ROM&    get_prg_bank(uint bank) const;
  Memory& get_chr_bank(uint bank) const;

public:

  virtual ~Mapper();

  Mapper(
    uint number, const char* name,
    const ROM_File& rom_file,
    u16 prg_bank_size, u16 chr_bank_size
  );

  /*-------------------  Emulator Actions Mapper Interface  ------------------*/

  // Call when inserting / removing cartridge from NES
  void set_interrupt_line(InterruptLines* interrupt_line) {
    this->interrupt_line = interrupt_line;
  }

  // ---- Mapper Queries ---- //
  const char* mapper_name()   const { return this->name;   };
        uint  mapper_number() const { return this->number; };

  // ---- Battery Backed Saving ---- //
  virtual const Serializable::Chunk* getBatterySave() const { return nullptr; }
  virtual void setBatterySave(const Serializable::Chunk* c) { (void)c; }

  // ---- Serialization ---- //
  virtual const Serializable::Chunk* deserialize(const Serializable::Chunk* c) override {
    c = this->Serializable::deserialize(c);
    this->update_banks();
    return c;
  }

  /*------------------------  Core Mapper Interface  -------------------------*/

  // <Memory>
  // reading doesn't tend to have side-effects, except in some fancy mappers
  virtual u8 read(u16 addr) override { return this->peek(addr); }
  virtual u8 peek(u16 addr) const override      = 0;
  virtual void write(u16 addr, u8 val) override = 0;
  // <Memory/>

  virtual Mirroring::Type mirroring() const = 0; // Get mirroring mode

  // Mappers tend to be benign, but some do have some fancy behavior.
  virtual void cycle() {}

  virtual void power_cycle() {
    if (this->banks.chr.is_RAM) {
      for (uint j = 0; j < this->banks.chr.len; j++) {
        static_cast<RAM*>(this->banks.chr.bank[j])->clear();
      }
    }
    this->reset();
    this->update_banks();
  };
  virtual void reset() = 0;

protected:
  virtual void update_banks() = 0; // (called post-deserialization)

public:
  /*------------------------------  Utilities  -------------------------------*/

  // creates correct mapper given an ROM_File object
  static Mapper* Factory(const ROM_File* rom_file);
};

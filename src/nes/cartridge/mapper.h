#pragma once

#include "rom_file.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"
#include "nes/interfaces/mirroring.h"

#include "nes/wiring/interrupt_lines.h"

#include "common/serializable.h"

// Mapper Interface
class Mapper : public Memory, public Serializable {
private:
  const char* name;
  const uint  number;
  InterruptLines* interrupt_line = nullptr;

protected:
  void irq_trigger() {
    if (this->interrupt_line)
      this->interrupt_line->request(Interrupts::IRQ);
  }

  void irq_service() {
    if (this->interrupt_line)
      this->interrupt_line->service(Interrupts::IRQ);
  }

  // Called when deserializing
  virtual void update_banks() {}

public:
  virtual ~Mapper() = default;
  Mapper(uint number, const char* name) : name(name), number(number) {};

  // ---- Mapper Setup ---- //
  void set_interrupt_line(InterruptLines* interrupt_line) {
    this->interrupt_line = interrupt_line;
  }

  // <Memory>
  // reading doesn't tend to have side-effects, except in very advanced mappers
  virtual u8 read(u16 addr) override { return this->peek(addr); }
  virtual u8 peek(u16 addr) const override      = 0;
  virtual void write(u16 addr, u8 val) override = 0;
  // <Memory/>

  // ---- Mapper Queries ---- //
  const char* mapper_name()   const { return this->name;   };
        uint  mapper_number() const { return this->number; };

  // ---- Mapper Properties ---- //
  virtual Mirroring::Type mirroring() const = 0; // Get mirroring mode

  // ---- Mapper Interactions ---- //
  // Mappers tend to be benign, but some do have some fancy behavior.
  virtual void cycle() {}

  // ---- Battery Backed Saving ---- //
  virtual const Serializable::Chunk* getBatterySave() const { return nullptr; }
  virtual void setBatterySave(const Serializable::Chunk* c) { (void)c; }

  // ---- Serialization ---- //
  virtual const Serializable::Chunk* deserialize(const Serializable::Chunk* c) override {
    c = this->Serializable::deserialize(c);
    this->update_banks();
    return c;
  }

  // ---- Utilities ---- //
  // creates correct mapper given an ROM_File object
  static Mapper* Factory(const ROM_File* rom_file);
};

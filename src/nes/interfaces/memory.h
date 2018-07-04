#pragma once

#include "common/util.h"

// 16 bit addressable memory interface
// All memory interfaces must also implement peek, a const version on read.
// This is awesome, since it means one can write tools to inspect any memory
// location without modifying the emulator state!
class Memory {
public:
  virtual ~Memory() = default;

  virtual u8 peek(u16 addr) const = 0;
  virtual u8 read(u16 addr) = 0;
  virtual void write(u16 addr, u8 val) = 0;

  // ---- Overloaded Array Subscript Operator ---- //
  // If we want to define this overload at the interface level, things get hard.
  // See, the Array Subscript operator is supposed to return a &u8, so that
  // reads and write to it modify that value.
  // But that's not good at all.
  //
  // How would we detect when reads / writes happen (for side-effects)?
  // How do we handle memory objects that don't have a "physical" value
  //
  // Easy! Instead of returning the u8 reference, return something that "looks"
  // like a u8 reference!
  // These thin "ref" classes overload `operator u8()` and `operator =` with
  // calls to Memory::read and Memory::write instead!

  class       ref; // allows .read and .write
  class const_ref; // allows .peek

        ref operator[](u16 addr)       { return       ref(this, addr); }
  const_ref operator[](u16 addr) const { return const_ref(this, addr); }

  class ref {
  private:
    friend class Memory;

    Memory* self;
    u16 addr;

    // Should not be initialized by client code
    ref(Memory* self, u16 addr) : self(self), addr(addr) {};
    ref(const ref&) = default;

  public:
    // Read
    operator u8() const { return self->read(addr); }

    // Write
    ref& operator= (u8 val)         { self->write(addr, val); return *this; }
    ref& operator= (const ref& val) { self->write(addr, val); return *this; }
  };

  class const_ref {
  private:
    friend class Memory;

    const Memory* self;
    u16 addr;

    // Should not be initialized by client code
    const_ref(const Memory* self, u16 addr) : self(self), addr(addr) {};
    const_ref(const const_ref&) = default;

  public:
    // Peek
    operator u8() const { return self->peek(addr); }

    template <typename whateverT>
    const_ref& operator= (const whateverT&) = delete; // can't write to const Memory!
    const_ref& operator= (const const_ref&) = delete; // can't write to const Memory!
  };
};

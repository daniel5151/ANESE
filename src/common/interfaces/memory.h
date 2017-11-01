#pragma once

#include "common/util.h"

// 16 bit addressable memory interface
class Memory {
public:
  virtual ~Memory() = default;

  virtual u8 peek(u16 addr) const = 0; // read, but without the side-effects
  virtual u8 read(u16 addr) = 0;
  virtual void write(u16 addr, u8 val) = 0;

  // ---- Derived memory access methods ---- //

  u16 peek16(u16 addr) const {
    return this->peek(addr + 0) |
          (this->peek(addr + 1) << 8);
  }
  u16 read16(u16 addr) {
    return this->read(addr + 0) |
          (this->read(addr + 1) << 8);
  };

  u16 peek16_zpg(u16 addr) const {
    return this->peek(addr + 0) |
          (this->peek(((addr + 0) & 0xFF00) | ((addr + 1) & 0x00FF)) << 8);
  }
  u16 read16_zpg(u16 addr) {
    return this->read(addr + 0) |
          (this->read(((addr + 0) & 0xFF00) | ((addr + 1) & 0x00FF)) << 8);
  }

  void write16(u16 addr, u8 val) {
    this->write(addr + 0, val);
    this->write(addr + 1, val);
  }

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
  class const_ref; // only .peek

        ref operator[](u16 addr)       { return ref(this, addr);       }
  const_ref operator[](u16 addr) const { return const_ref(this, addr); }

  // This is for
  class ref {
  private:
    friend class Memory;

    Memory* self;
    u16 addr;

    // Should not be initialized by client code
    ref(Memory* self, u16 addr) : self(self), addr(addr) {};

  public:
    ~ref() = default;

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

  public:
    ~const_ref() = default;

    // Peek
    operator u8() const { return self->peek(addr); }

    // Disallow all write operations
    template <typename T>
    const_ref& operator= (const T&) = delete;
  };
};

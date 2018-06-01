#pragma once

#include "common/util.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <csignal>

// A DIY data-serialization system.
// Incredibly hacky, and a massive abuse of C++'s typesystem.
// But hey, it works!

// Caveats:
// --------
// 1) The chunks are _not_ tagged, so serialization order must be preserved!
// 2) Circular dependencties will not work (i.e: bad things will happen if you
//    call serialize() on A, which attemps to serialize() member B, who happens
//    to need to serialize A)
// 3) `Serializable` Inheritence is not handled properly, with only the data
//    of the leaf-class being serialized (not any of it's base classes)

// Usage:
// ------
// Say we have the following class:
// ```
//   struct my_thing {
//     // a POD type of fixed size
//     uint some_flags;
//
//     // a fixed-length array
//     u8 regs [10];
//
//     // a variable-length array with an associated length
//     u8*  ram;
//     uint ram_len;
//
//     // a nontrivial serializable type
//     Serializable_Derived  fancyboi;
//     Serializable_Derived* fancyboi_mightbenull;
//   }
// ```
//
// Serialization:
//
// 1) make the struct extend Serializable's public interface:
//      `struct my_thing : public Serializable`
//
// 2) define the serialization structure in the structure's definition
// ```
//   my_thing() {
//     // a POD type of fixed ssize
//     uint some_flags;
//     ...
//     ...
//     // a nontrivial serializable type
//     Serializable_Derived* fancyboi;
//
//     SERIALIZE_START(6) // <-- number of fields to serialize
//       SERIALIZE_POD(this->some_flags)
//       SERIALIZE_ARRAY_FIXED(this->regs 10)
//       SERIALIZE_ARRAY_VARIABLE(this->ram this->ram_len)
//       SERIALIZE_POD(this->ram_len)
//       SERIALIZE_SERIALIZABLE(this->fancyboi)
//       SERIALIZE_SERIALIZABLE_PTR(this->fancyboimightbenull)
//     SERIALIZE_END(6) // <-- number of fields to serialize (sanity check)
//   }
// ```
//
// 3) call my_thing.serialize() to get back a Chunk* list
// 4) call chunk->collate(data, len) with an empty data buffer, and it fill be
//    filled with data!
//
// Deserialization:
//
// 1) call Serializable::Chunk::parse(data, len) to get a Chunk* list from
//    existing data
// 2) Pass the Chunk* list to my_thing.deserialize(chunk)
//
// That's it!
// Have fun!

class Serializable {
public:
  // Chunks are the base units of serialization.
  // They are functionally a singly-linked list, where the data is just a bunch
  // of bytes of a certain length
  struct Chunk {
    uint len;
    u8* data;

    Chunk* next;

    ~Chunk();
    Chunk();
    Chunk(const void* data, uint len);

    const Chunk* debugprint() const {
      fprintf(stderr, "| %4X | ", this->len);
      if (this->len) fprintf(stderr, "0x%08X\n", *((uint*)this->data));
      else           fprintf(stderr, "(null)\n");

      if (this->next) this->next->debugprint();

      return this; // for chaining
    }

    // Collates the list of chunks into a flat array
    void collate(const u8*& data, uint& len);

    // Divide a flat array into a list of chunks
    static const Chunk* parse(const u8* data, uint len);
  };

  // Returns list of chunks for the class's data fields
  Chunk* serialize() const;
  // Updates class's data fields with chunk data, and returns new head-chunk
  const Chunk* deserialize(const Chunk* c);

/*------------------------------  Macro Support  -----------------------------*/

  // Macro Supports
  enum _field_type {
    INVALID = 0,
    POD,
    ARRAY_FIXED,
    ARRAY_VARIABLE,
    SERIALIZABLE,
    SERIALIZABLE_PTR
  };

  struct _field_data {
    const char* parent_label;
    const char* label;
    _field_type type;
    void* thing;
    // only one of the two is used
    uint len_fixed;
    uint* len_variable;
  };

protected:
  virtual void _delete_serializable_state() const {}
  virtual void _get_serializable_state(const _field_data*& data, uint& len) const {
    // To keep things cleaner in serialization / deserialization, if something
    // is marked serializable but doesn't provide a serializable state, then
    // simply return this empty
    static uint dump = 0;
    static const _field_data nothing [1] = {{
      "<nothing>", "<nothing>", _field_type::POD, &dump, sizeof dump, nullptr
    }};

    data = nothing;
    len = 1;
  }
};

// The macros themselves

#define SERIALIZE_START(n, label) \
  const char* _serializable_state_label = label; \
  const uint  _serializable_state_len = n; \
  const Serializable::_field_data* _serializable_state = nullptr; \
  void _delete_serializable_state() const override { \
    delete this->_serializable_state; \
    /* we are lying to the compiler about this being a const function... */ \
    *((Serializable::_field_data**)&this->_serializable_state) = nullptr; \
  } \
  void _get_serializable_state(const Serializable::_field_data*& data, uint& len) const override { \
    /* update pointers to serializables */ \
    delete this->_serializable_state; \
    /* we are lying to the compiler about this being a const function... */ \
    *((Serializable::_field_data**)&this->_serializable_state) \
      = new Serializable::_field_data [this->_serializable_state_len] {

#define SERIALIZE_END(n) \
    }; \
    data = this->_serializable_state; \
    len = this->_serializable_state_len; \
    /* sanity check */ \
    assert(len == n); \
  };


#define SERIALIZE_POD(thing) { \
    _serializable_state_label, __FILE__ ":" #thing, \
    Serializable::_field_type::POD, \
    (void*)&(thing), \
    sizeof(thing), 0 \
  },

#define SERIALIZE_ARRAY_FIXED(thing, len) { \
    _serializable_state_label, __FILE__ ":" #thing, \
    Serializable::_field_type::ARRAY_FIXED, \
    (void*)&(thing), \
    len, 0 \
  },

#define SERIALIZE_ARRAY_VARIABLE(thing, len) { \
    _serializable_state_label, __FILE__ ":" #thing, \
    Serializable::_field_type::ARRAY_VARIABLE, \
    (void*)&(thing), \
    0, (uint*)&len \
  },

#define SERIALIZE_SERIALIZABLE(thing) { \
    _serializable_state_label, __FILE__ ":" #thing, \
    Serializable::_field_type::SERIALIZABLE, \
    (void*)dynamic_cast<const Serializable*>(&(thing)), \
    0, 0 \
  },

#define SERIALIZE_SERIALIZABLE_PTR(thingptr) { \
    _serializable_state_label, __FILE__ ":" #thingptr, \
    Serializable::_field_type::SERIALIZABLE_PTR, \
    (void*)dynamic_cast<const Serializable*>(thingptr), \
    0, 0 \
  },

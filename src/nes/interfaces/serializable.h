#pragma once

#include "common/util.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <csignal>

// A DIY data-serialization system.
// **Cannot do circular references!**

// Caveats:
// --------
// 1) The chunks do _not_ have any sort of tags on them, so if the order of
//    serialization changes, old versions of the data will not work!
// 2) Circular dependencties will not work (i.e: bad things will happen if you
//    call serialize() on A, which attemps to serialize() member B, who happens
//    to need to serialize A)

// Usage:
// ------
// Say we have the following class:
// ```
//   struct my_thing {
//     // a POD type of fixed size
//     uint some_flag;
//
//     // a fixed-length array
//     u8 regs [10];
//
//     // a variable-length array with an associated length
//     u8*  ram;
//     uint ram_len;
//
//     // a nontrivial serializable type - WIP, NOT WORKING YET
//     Serializable_Derived* fancyboi;
//   }
// ```
//
// Serialization:
//
// 1) make the struct extend Serializable's public interface:
//      `struct my_thing : public Serializable`
//
// 2) define the serialization structure in the structure's constructor
// ```
//   my_thing() {
//     SERIALIZE_START(4)
//       SERIALIZE_POD(this->some_flag),
//       SERIALIZE_ARRAY_FIXED(this->regs, 10),
//       SERIALIZE_ARRAY_VARIABLE(this->ram, this->ram_len),
//       SERIALIZE_POD(this->ram_len),
//       SERIALIZE_SERIALIZABLE(this->fancyboi),
//     SERIALIZE_END(4)
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

    void debugprint() const {
      fprintf(stderr, "Chunk len %d, starts with 0x%08X\n",
        this->len,
        *((uint*)this->data));

      if (this->next) {
        this->next->debugprint();
      } else {
        fprintf(stderr, "\n");
      }
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
  enum _field_type { POD, ARRAY_FIXED, ARRAY_VARIABLE, SERIALIZABLE };
  struct _field_data {
    void* thing;
    uint  len;
    uint* varlen;
    _field_type type;
  };

protected:
  void _init_serializable_state(Serializable::_field_data* state, uint len) {
    this->_serializable_state = state;
    this->_serializable_state_len = len;
  }
private:
  Serializable::_field_data* _serializable_state = nullptr;
  uint                      _serializable_state_len = 0;
public:
  virtual ~Serializable() { delete this->_serializable_state; }
};

// The macros themselves

#define SERIALIZE_START(n) \
  _init_serializable_state(new Serializable::_field_data [n] {
#define SERIALIZE_END(n) }, n);

#define SERIALIZE_POD(thing) \
  { &(thing), sizeof(thing), 0,      Serializable::_field_type::POD            }
#define SERIALIZE_ARRAY_FIXED(thing, len) \
  {  (thing), (len),         0,      Serializable::_field_type::ARRAY_FIXED    }
#define SERIALIZE_ARRAY_VARIABLE(thing, len) \
  {  (thing), 0,             &(len), Serializable::_field_type::ARRAY_VARIABLE }
// WIP
#define SERIALIZE_SERIALIZABLE(thing) \
  { &(thing), 0,             0,      Serializable::_field_type::SERIALIZABLE   }

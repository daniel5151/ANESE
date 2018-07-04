#pragma once

#include "common/util.h"

#include <cassert>
#include <cstring>
#include <cstdio>

// A DIY data-serialization system.
// Hacky. Brittle. Beautiful.
// A true abuse of C/C++'s typesystem (or lack thereof)
//
// Caveats:
// --------
// 1) The chunks are _not_ tagged, so serialization order must be preserved!
// 2) SERIALIZE_SERIALIZABLE_PTR will never instantiate pointers!
//     this means that if you serialize a linked-list of Serializable*, you
//     _must_ deserialize it into a linked-list with the _same structure_,
//     or else deserialization will fail catastrophically
// 3) Circular dependencies break everything (i.e: bad things will happen when
//     serialize() gets called on A, thus calling serialize() on member B, who
//     holds a reference to A... You'll blow the stack, and that's bad)
// 4) `Serializable` inheritence is not handled _nicely_, with one having to
//    manually override de/serialize functions with calls to the parent funcs,
//    performing the chunk-manipulation manually.
//
// Dangerous Shenanigans that should be AVOIDED
// --------------------------------------------
// - Calling de/serialize in the constructor/destructor can be dangerous, as
//    some fields (notably SERIALIZE_ARRAY_VARIABLE and
//    SERIALIZE_SERIALIZABLE_PTR) may not have been instantiated yet.
//   Tread carefully.
//
// Basic Usage:
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
//   struct my_thing : public Serializable {
//     // a POD type of fixed size
//     uint some_flags;
//     ...
//     ...
//     // a nontrivial serializable type
//     Serializable_Derived* fancyboi;
//
//     SERIALIZE_START(6) // <-- number of fields to serialize
//       SERIALIZE_POD(this->some_flags)
//       SERIALIZE_POD(this->regs)
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
// Advanced:
// ---------
// - If a class need to preform some actions post / pre de/serialize, you can
//   override the serialization methods and add custom behavior.
//     - **Be Careful:** when overriding de/serialize functions, make sure to
//       call the parent class's de/serialize functions, or else the class will
//       not properly serialize!
// - If you inherit from an object that is already Serializable, make sure that
//   you add the SERIALIZE_PARENT(BaseClass) macro before SERIALIZE_START.
//   This tells Serializable to also serialize whatever state the parent had
//     - _Caveat:_ There is no support for multiple inheritance at the moment
// - Since everything is just regular 'ol C++ under the hood, you can implement
//   custom serialization routines (outside of the predefined macros). When
//   doing so, express intent by wrapping the custom implementation in a
//   SERIALIZE_CUSTOM() {} block
//
// That's it!
// Have fun!

class Serializable {
public:
  // Chunks are the base units of serialization.
  // Functionally, they are simple singly-linked lists, with each node holding
  // a chunk of data, it's length, and a reference to the next link in the chain
  struct Chunk {
    uint len;
    u8* data;

    Chunk* next;

    ~Chunk();
    Chunk();
    Chunk(const void* data, uint len);

    const Chunk* debugprint() const;

    // Collates the list of chunks into a flat array (ex: dump to file)
    static void collate(const u8*& data, uint& len, const Chunk* chunk);
    // Divide a flat array into a list of chunks (ex: load from file)
    static const Chunk* parse(const u8* data, uint len);
  };

  // Returns list of chunks for the class's data fields
  virtual Chunk* serialize() const;
  // Updates class's data fields with chunk data, and returns new head-chunk
  virtual const Chunk* deserialize(const Chunk* c);

/*------------------------------  Macro Support  -----------------------------*/
protected:
  enum _field_type {
    SERIAL_INVALID = 0,
    SERIAL_POD,
    SERIAL_ARRAY_VARIABLE,
    SERIAL_IZABLE,
    SERIAL_IZABLE_PTR
  };

  struct _field_data {
    const char* parent_label;
    const char* label;
    _field_type type;
    void* thing;
    // only one of the following are used
    uint len_fixed;
    uint* len_variable;
  };

  // This gets overridden through macros
  virtual void _get_serializable_state(const _field_data*& data, uint& len) const {
    // this is a bit of a hacky workaround to the case where a base-class is
    // marked serializable, but none of it's children, or itself, provide any
    // serializable state...
    // this is bad since it uses a static "dump" chunk of memory to read/write
    // dummy data, and that means the serialized state probably won't be
    // consistent :/
    fprintf(stderr, "[Serializable] WARNING: "
      "Calling base _get_serializable_state. "
      "This may result in different dumps for identical state!\n");

    static uint dump = 0;
    static const _field_data nothing [1] = {{
      "<nothing>", "<nothing>",
      _field_type::SERIAL_POD, &dump,
      sizeof dump, nullptr
    }};

    data = nothing;
    len = 1;
  }

  // This potentially gets _hidden_ (not overwritten) through macros
  void _get_parent_serializable_state(const _field_data*& data, uint& len) const {
    data = nullptr;
    len = 0;
  }
};

/*============================================
=            Serialization Macros            =
============================================*/

/*---------------------------  Boilerplate Macros  ---------------------------*/

#define SERIALIZE_PARENT(BaseClass)                                            \
  void _get_parent_serializable_state(                                         \
    const Serializable::_field_data*& data, uint& len                          \
  ) const { this->BaseClass::_get_serializable_state(data, len); }

#define SERIALIZE_START(n, label)                                              \
  void _get_serializable_state(                                                \
    const Serializable::_field_data*& data, uint& len                          \
  ) const override {                                                           \
    const char* chunk_label = (label);                                         \
    if ((n) <= 0) {                                                            \
      fprintf(stderr, "[Serializable][%s] Error: Cannot have %d fields\n",     \
        chunk_label, (n));                                                     \
      assert(false);                                                           \
    }                                                                          \
    /* If SERIALIZE_PARENT() was called, this should return be non-null */     \
    const Serializable::_field_data* parent_data = nullptr;                    \
    uint parent_len = 0;                                                       \
    this->_get_parent_serializable_state(parent_data, parent_len);             \
                                                                               \
    len = (n) + parent_len;                                                    \
    uint i = 0;                                                                \
    Serializable::_field_data* new_state                                       \
      = new Serializable::_field_data [len];                                   \
                                                                               \
    for (;i < parent_len;i++)                                                  \
      new_state[i] = parent_data[i];                                           \
    delete[] parent_data;                                                      \
    /* ... calls to SERIALIZE_XXX() ... */

#define SERIALIZE_END(n)                                                       \
    data = new_state;                                                          \
    /* sanity check */                                                         \
    if (len != i) {                                                            \
      fprintf(stderr, "[Serializable][%s] Mismatch between #fields declared"   \
                      " and #fields defined! %u != %u\n",                      \
                      chunk_label, len - parent_len, i - parent_len);          \
      assert(false);                                                           \
    }                                                                          \
  };

/*-----------------------  Serialize Datatypes Macros  -----------------------*/

#ifdef _WIN32
  static constexpr char slash = '\\';
#else
  static constexpr char slash = '/';
#endif

#define SERIALIZE_POD(thing)                                                   \
  new_state[i++] = {                                                           \
    chunk_label, strrchr(__FILE__ ": " #thing, slash) + 1,                     \
    Serializable::_field_type::SERIAL_POD,                                     \
    (void*)&(thing),                                                           \
    sizeof(thing), 0                                                           \
  };

#define SERIALIZE_ARRAY_VARIABLE(thing, len)                                   \
  new_state[i++] = {                                                           \
    chunk_label, strrchr(__FILE__ ": " #thing, slash) + 1,                     \
    Serializable::_field_type::SERIAL_ARRAY_VARIABLE,                          \
    (void*)&(thing),                                                           \
    0, (uint*)&len                                                             \
  };

#define SERIALIZE_SERIALIZABLE(item)                                           \
  new_state[i++] = {                                                           \
    chunk_label, strrchr(__FILE__ ": " #item, slash) + 1,                      \
    Serializable::_field_type::SERIAL_IZABLE,                                  \
    (void*)static_cast<const Serializable*>(&(item)),                          \
    0, 0                                                                       \
  };                                                                           \
  if (new_state[i-1].thing == nullptr)                                         \
    fprintf(stderr, "[Serializable][%s] Warning: could not cast `" #item       \
                    "` down to Serializable!\n", chunk_label);

#define SERIALIZE_SERIALIZABLE_PTR(thingptr)                                   \
  new_state[i++] = {                                                           \
    chunk_label, strrchr(__FILE__ ": " #thingptr, slash) + 1,                  \
    Serializable::_field_type::SERIAL_IZABLE_PTR,                              \
    (void*)static_cast<const Serializable*>(thingptr),                         \
    0, 0                                                                       \
  };

#define SERIALIZE_CUSTOM() // doesn't do anything except express intent

#pragma once

#include "common/util.h"

#include <cassert>
#include <cstring>
#include <cstdio>

// A DIY data-serialization system.
// Incredibly hacky, and a true abuse of C++'s typesystem (or lack thereof).
// But hey, it works!

// Caveats:
// --------
// 1) The chunks are _not_ tagged, so serialization order must be preserved!
// 2) SERIALIZE_SERIALIZABLE_PTR will never instantiate pointers!
//     this means that if you serialize a linked-list of Serializable*, you
//     _must_ deserialize it into a linked-list with the _same structure_,
//     or else deserialization will fail catastrophically
// 3) Circular dependencies break everything (i.e: bad things will happen when
//     serialize() gets called on A, thus calling serialize() on member B, who
//     holds a referance to A... You'll blow the stack, and that's bad)
// 4) `Serializable` Inheritence is not handled _nicely_, with one having to
//    manually override de/serialize functions with calls to the parent funcs,
//    performing the chunk-manipulation manually.
// 5) Fields have a maximum length of UINT32_MAX (uint is typedef for uint32_t)
//    (this probably won't be an issue though haha)

// Dangerous Shenanigans that should be AVOIDED
// --------------------------------------------
// - Defining something to be serialized with 0 fields is undefined behavior,
//    and will probably break things.
// - Calling de/serialize in the constructor/destructor can be dangeous, as some
//    fields (notably SERIALIZE_ARRAY_VARIABLE and SERIALIZE_SERIALIZABLE(_PTR))
//    may not be instatiated yet.
//   Tread carefully.

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
// If a class need to preform some actions post / pre de/serialize, you can
// override the serilization methods and add custom behavior, being careful to
// call the parent de/serialize function appropriately!
//
// That's it!
// Have fun!

class Serializable {
public:
  // Chunks are the base units of serialization.
  // They are functionally singly-linked lists, where each node stores a pointer
  // to an array of bytes of a certain length.
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
    INVALID = 0,
    POD,
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

  // This gets overridden through macros
  virtual void _get_serializable_state(const _field_data*& data, uint& len) const {
    // this is a bit of a hacky workaround to the case where a base-class is
    // marked serializable, but none of it's children, or itself, provide any
    // serializable state...
    // this is bad since it uses a static "dump" to read/write dummy data, and
    // it is hella-not guaranteed to be consistent
    fprintf(stderr, "[Serializable] WARNING: "
      "Calling base _get_serializable_state. "
      "This may result in different dumps for identical state!\n");

    static uint dump = 0;
    static const _field_data nothing [1] = {{
      "<nothing>", "<nothing>", _field_type::POD, &dump, sizeof dump, nullptr
    }};

    data = nothing;
    len = 1;
  }
};

// The macros themselves

#define SERIALIZE_START(n, label)                                              \
  void _get_serializable_state(                                                \
    const Serializable::_field_data*& data,                                    \
    uint& len                                                                  \
  ) const override {                                                           \
    if ((n) <= 0) {                                                            \
      fprintf(stderr, "[Serializable] Error: Cannot have %d fields\n", (n));   \
      assert(false);                                                           \
    }                                                                          \
    /* MSVC is a lil bitch, and has a compiler error when performing */        \
    /* aggregate instantiation with the new[] operator on user-defined types */\
    /* Instead, we have to do this less elegant method... */                   \
    Serializable::_field_data* new_state                                       \
      = new Serializable::_field_data [(n)];                                   \
    uint i = 0;                                                                \
    const char* chunk_label = (label);                                         \
    /* ... SERIALIZE_BLAHBLAHBLAH() ... */

#define SERIALIZE_END(n)                                                       \
    data = new_state;                                                          \
    len = (n);                                                                 \
    /* sanity check */                                                         \
    if (len != i) {                                                            \
      fprintf(stderr, "[Serializable] Error: Mismatch between #fields declared"\
                      " and #fields defined!\n");                              \
      assert(false);                                                           \
    }                                                                          \
  };

#ifdef _WIN32
  const char slash = '\\';
#else
  const char slash = '/';
#endif

#define SERIALIZE_POD(thing)                                         \
  new_state[i++] = {                                                 \
    chunk_label, strrchr(__FILE__ ": " #thing, slash) + 1,           \
    Serializable::_field_type::POD,                                  \
    (void*)&(thing),                                                 \
    sizeof(thing), 0                                                 \
  };

#define SERIALIZE_ARRAY_VARIABLE(thing, len)                         \
  new_state[i++] = {                                                 \
    chunk_label, strrchr(__FILE__ ": " #thing, slash) + 1,           \
    Serializable::_field_type::ARRAY_VARIABLE,                       \
    (void*)&(thing),                                                 \
    0, (uint*)&len                                                   \
  };

#define SERIALIZE_SERIALIZABLE(item)                                 \
  new_state[i++] = {                                                 \
    chunk_label, strrchr(__FILE__ ": " #item, slash) + 1,            \
    Serializable::_field_type::SERIALIZABLE,                         \
    (void*)static_cast<const Serializable*>(&(item)),                \
    0, 0                                                             \
  };                                                                 \
  if (new_state[i-1].thing == nullptr)                               \
    fprintf(stderr, "[Serializable] Warning: could not cast `" #item \
                    "` down to Serializable!\n");

#define SERIALIZE_SERIALIZABLE_PTR(thingptr)                         \
  new_state[i++] = {                                                 \
    chunk_label, strrchr(__FILE__ ": " #thingptr, slash) + 1,        \
    Serializable::_field_type::SERIALIZABLE_PTR,                     \
    (void*)static_cast<const Serializable*>(thingptr),               \
    0, 0                                                             \
  };

#define SERIALIZE_CUSTOM() // literally just for intent

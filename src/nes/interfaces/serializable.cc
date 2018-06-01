#include "serializable.h"

// #define fprintf(...) // disable spam

Serializable::Chunk::~Chunk() {
  delete this->data;
  delete this->next;
}

Serializable::Chunk::Chunk() {
  this->len = 0;
  this->data = nullptr;
  this->next = nullptr;
}

Serializable::Chunk::Chunk(const void* data, uint len) {
  this->len = len;
  if (this->len == 0) {
    // this is a nullchunk
    this->data = nullptr;
  } else {
    this->data = new u8 [len];
    memcpy(this->data, data, len);
  }
  this->next = nullptr;
}

void Serializable::Chunk::collate(const u8*& data, uint& len) {
  // calculate total length of chunk-list
  uint total_len = 0;
  {
    Chunk* c = this;
    while (c) {
      total_len += (sizeof Chunk::len) + c->len;
      c = c->next;
    }
  }

  // output buffer
  u8* output = new u8 [total_len];

  // fill the output buffer with all the chunks
  Chunk* c = this;
  u8* p = output;
  while(c) {
    memcpy(p, &c->len, sizeof Chunk::len);
    p += sizeof Chunk::len;
    memcpy(p, c->data, c->len);
    p += c->len;
    c = c->next;
  }

  data = output;
  len = total_len;
}

const Serializable::Chunk* Serializable::Chunk::parse(const u8* data, uint len) {
  Chunk *head, *tail, *next;
  head = tail = next = nullptr;

  const u8* p = data;
  while (p < data + len) {
    // Construct a chunk by reading length of next chunk from datastream
    const u8* chunk_data = p + (sizeof Chunk::len);
    uint      chunk_data_len = *((uint*)p);
    next = new Chunk(chunk_data, chunk_data_len);
    p += (sizeof Chunk::len) + chunk_data_len;

    // Append the new chunk to the list
    if (!head) {
      head = tail = next;
    } else {
      tail->next = next;
      tail = next;
    }
  }

  return head;
}

static char indent_buf [256] = {0};
static uint indent_i = 0;
void indent_add() { indent_buf[indent_i++] = ' ';  }
void indent_del() { indent_buf[--indent_i] = '\0'; }

#ifdef _WIN32
const char* pathstrip = "nes\\";
#elif
const char* pathstrip = "nes/";
#endif

Serializable::Chunk* Serializable::serialize() const {
  indent_add();
  const Serializable::_field_data* field_data = nullptr;
  uint field_data_len = 0;
  this->_get_serializable_state(field_data, field_data_len);

  Chunk *head, *tail, *next;
  head = tail = next = nullptr;

  for (uint i = 0; i < field_data_len; i++) {
    const _field_data& field = field_data[i];
    fprintf(stderr, "[Serialization][%d] %s%-60s: len %X | ",
      field.type,
      indent_buf,
      strstr(field.label, pathstrip),
      (field.type > 3)
        ? 0
        : (field.type == 3 ? *field.len_variable : field.len_fixed)
    );
    switch (field.type) {
    case _field_type::INVALID: assert(false); break;
    case _field_type::POD:
    case _field_type::ARRAY_FIXED:
      fprintf(stderr, "0x%08X\n", *((uint*)field.thing));
      next = new Chunk(field.thing, field.len_fixed);
      break;
    case _field_type::ARRAY_VARIABLE:
      fprintf(stderr, "0x%08X\n", **((uint**)field.thing));
      next = new Chunk(*((void**)field.thing), *field.len_variable);
      break;
    case _field_type::SERIALIZABLE:
      fprintf(stderr, "serializable: \n");
      next = ((Serializable*)field.thing)->serialize();
      assert(next != nullptr);
      break;
    case _field_type::SERIALIZABLE_PTR: {
      fprintf(stderr, "serializable_ptr: ");
      if (!field.thing) {
        fprintf(stderr, "null\n");
        next = new Chunk(); // nullchunk == tried to serialize null serializable
      } else {
        fprintf(stderr, "recursive\n");
  // asm("int $3");
        next = ((Serializable*)field.thing)->serialize();
        assert(next != nullptr);
      }
    } break;
    }

    if (!head) {
      head = tail = next;
    } else {
      tail->next = next;
      tail = next;
    }

    if (field.type == _field_type::SERIALIZABLE ||
        field.type == _field_type::SERIALIZABLE_PTR) {
      while (tail->next) tail = tail->next;
    }
  }
  indent_del();
  this->_delete_serializable_state();
  return head;
}

const Serializable::Chunk* Serializable::deserialize(const Chunk* c) {
  if (!c) return nullptr;
  indent_add();

  const Serializable::_field_data* field_data = nullptr;
  uint field_data_len = 0;
  this->_get_serializable_state(field_data, field_data_len);

  for (uint i = 0; i < field_data_len; i++) {
    const _field_data& field = field_data[i];
    fprintf(stderr, "[DeSerialization][%d] %s%-60s: len %X | ",
      field.type,
      indent_buf,
      strstr(field.label, pathstrip),
      (field.type > 3)
        ? 0
        : (field.type == 3 ? *field.len_variable : field.len_fixed)
    );

    if (c->len != field.len_fixed && (field.type == _field_type::ARRAY_VARIABLE && c->len != *field.len_variable)) {
      fprintf(stderr, "[Deserialization] Field length mismatch! "
        "This is probably caused by a change in serialization order.\n");
      assert(false);
    }

    switch (field.type) {
    case _field_type::INVALID: assert(false); break;
    case _field_type::POD:
    case _field_type::ARRAY_FIXED:
      fprintf(stderr, "0x%08X\n", *((uint*)c->data));
      memcpy(field.thing, c->data, c->len);
      c = c->next;
      break;
    case _field_type::ARRAY_VARIABLE:
      fprintf(stderr, "0x%08X\n", *((uint*)c->data));
      memcpy(*((void**)field.thing), c->data, c->len);
      c = c->next;
      break;
    case _field_type::SERIALIZABLE:
      fprintf(stderr, "serializable: \n");
      // recursively deserialize the data
      c = ((Serializable*)field.thing)->deserialize(c);
      break;
    case _field_type::SERIALIZABLE_PTR: {
      fprintf(stderr, "serializable_ptr: ");
      if (c->len == 0) {
        fprintf(stderr, "null\n");
        // nullchunk == this serializable was null, so the thing must be null
        c = c->next;
        fprintf(stderr, "null\n");
      } else {
        // recursively deserialize the data
        fprintf(stderr, "recursive\n");
        c = ((Serializable*)field.thing)->deserialize(c);
      }
    } break;
    }
  }

  indent_del();
  this->_delete_serializable_state();
  return c;
}

// "testing"
// struct test_struct_base : public Serializable {
//   uint balls;
// };

// struct test_struct : test_struct_base {
//   uint val;
//   test_struct* next;
//   uint val2;

//   u8* varlen_array;

//   SERIALIZE_START(4, "test")
//     SERIALIZE_POD(this->val)
//     SERIALIZE_SERIALIZABLE_PTR(this->next)
//     SERIALIZE_POD(this->val2)
//     SERIALIZE_ARRAY_VARIABLE(this->varlen_array, this->val)
//   SERIALIZE_END(4)

//   test_struct(uint val) {
//     this->val = val;
//     this->val2 = val << 16;
//     this->next = nullptr;
//     this->varlen_array = new u8 [val];
//     for (uint i = 0; i < val; i++) {
//       this->varlen_array[i] = (i + 1) * 2;
//       this->varlen_array[i] |= this->varlen_array[i] << 4;
//     }
//   }

//   void add(test_struct* next) { this->next = next; }
// };

// static auto test = [](){
//   test_struct* basicboi = new test_struct(1);
//   basicboi->add(new test_struct(2));
//   basicboi->next->add(new test_struct(3));

//   const u8* data;
//   uint len;
//   Serializable::Chunk* base_state = ((test_struct_base*)basicboi)->serialize();
//   base_state->debugprint();
//   base_state->collate(data, len);

//   basicboi->val = 100;
//   basicboi->next->val = 200;
//   basicboi->next->next->val = 300;

//   const Serializable::Chunk* too = Serializable::Chunk::parse(data, len);
//   too->debugprint();

//   ((test_struct_base*)basicboi)->deserialize(Serializable::Chunk::parse(data, len));

//   assert(basicboi->val == 1 && basicboi->next->val == 2 && basicboi->next->next->val == 3);

//   return 1;
// }();

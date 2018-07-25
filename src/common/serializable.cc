#include "serializable.h"

// #define fprintf(...) // disable spam

/*----------  Serializable Chunk Implementation  ----------*/

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

const Serializable::Chunk* Serializable::Chunk::debugprint() const {
  fprintf(stderr, "| %4X | ", this->len);
  if (this->len) fprintf(stderr, "0x%08X\n", *((uint*)this->data));
  else           fprintf(stderr, "(null)\n");

  if (this->next) this->next->debugprint();

  return this; // for chaining
}

void Serializable::Chunk::collate(const u8*& data, uint& len, const Chunk* co) {
  // calculate total length of chunk-list
  uint total_len = 0;
  {
    const Chunk* c = co;
    while (c) {
      total_len += (sizeof Chunk::len) + c->len;
      c = c->next;
    }
  }

  if (total_len == 0) {
    data = nullptr;
    len = 0;
    return;
  }

  // create a new output buffer
  u8* output = new u8 [total_len];

  // fill the output buffer with all the chunks
  const Chunk* c = co;
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

/*--------------------------  De/Serialize Methods  --------------------------*/

static char indent_buf [256] = {0};
static uint indent_i = 0;
void indent_add() { indent_buf[indent_i++] = ' ';  }
void indent_del() { indent_buf[--indent_i] = '\0'; }

Serializable::Chunk* Serializable::serialize() const {
  indent_add();
  const Serializable::_field_data* field_data = nullptr;
  uint field_data_len = 0;
  this->_get_serializable_state(field_data, field_data_len);

  Chunk *head, *tail, *next;
  head = tail = next = nullptr;

  for (uint i = 0; i < field_data_len; i++) {
    const _field_data& field = field_data[i];
    fprintf(stderr, "[Serialization][%d] %s%-50s: len %X | ",
      field.type,
      indent_buf,
      field.label,
      (field.type >= 3)
        ? 0
        : (field.type == 2 ? *field.len_variable : field.len_fixed)
    );
    switch (field.type) {
    case _field_type::SERIAL_INVALID: assert(false); break;
    case _field_type::SERIAL_POD:
      fprintf(stderr, "0x%08X\n", *((uint*)field.thing));
      next = new Chunk(field.thing, field.len_fixed);
      break;
    case _field_type::SERIAL_ARRAY_VARIABLE:
      fprintf(stderr, "0x%08X\n", **((uint**)field.thing));
      next = new Chunk(*((void**)field.thing), *field.len_variable);
      break;
    case _field_type::SERIAL_IZABLE:
      fprintf(stderr, "serializable: \n");
      next = ((Serializable*)field.thing)->serialize();
      assert(next != nullptr);
      break;
    case _field_type::SERIAL_IZABLE_PTR: {
      fprintf(stderr, "serializable_ptr: ");
      if (!field.thing) {
        fprintf(stderr, "null\n");
        next = new Chunk(); // nullchunk.
      } else {
        fprintf(stderr, "recursive\n");
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

    if (field.type == _field_type::SERIAL_IZABLE ||
        field.type == _field_type::SERIAL_IZABLE_PTR) {
      while (tail->next) tail = tail->next;
    }
  }
  indent_del();
  delete field_data;
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
    fprintf(stderr, "[DeSerialization][%d] %s%-50s: len %X | ",
      field.type,
      indent_buf,
      field.label,
      (field.type >= 3)
        ? 0
        : (field.type == 2 ? *field.len_variable : field.len_fixed)
    );

    switch (field.type) {
    case _field_type::SERIAL_INVALID: assert(false); break;
    case _field_type::SERIAL_POD:
      fprintf(stderr, "0x%08X\n", *((uint*)c->data));
      memcpy(field.thing, c->data, c->len);
      c = c->next;
      break;
    case _field_type::SERIAL_ARRAY_VARIABLE:
      fprintf(stderr, "0x%08X\n", *((uint*)c->data));
      memcpy(*((void**)field.thing), c->data, c->len);
      c = c->next;
      break;
    case _field_type::SERIAL_IZABLE:
      fprintf(stderr, "serializable: \n");
      // recursively deserialize the data
      c = ((Serializable*)field.thing)->deserialize(c);
      break;
    case _field_type::SERIAL_IZABLE_PTR: {
      fprintf(stderr, "serializable_ptr: ");
      if (c->len == 0 && field.thing == nullptr) {
        fprintf(stderr, "null\n");
        // nullchunk. Ignore this and carry on.
        c = c->next;
      } else {
        fprintf(stderr, "recursive\n");
        // recursively deserialize the data
        c = ((Serializable*)field.thing)->deserialize(c);
      }
    } break;
    }
  }

  indent_del();
  delete field_data;
  return c;
}

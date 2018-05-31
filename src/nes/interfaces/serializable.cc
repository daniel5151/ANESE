#include "serializable.h"

Serializable::Chunk::~Chunk() {
  delete this->data;
  delete this->next;
}

Serializable::Chunk::Chunk() {
  this->data = nullptr;
  this->len = 0;
  this->next = nullptr;
}

Serializable::Chunk::Chunk(const void* data, uint len) {
  this->data = new u8 [len];
  this->len = len;
  memcpy(this->data, data, len);
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

Serializable::Chunk* Serializable::serialize() const {
  if (this->_serializable_state == nullptr) {
    fprintf(stderr, "No serializable state defined. Dying...\n");
    assert(false);
  }

  Chunk *head, *tail, *next;
  head = tail = next = nullptr;

  for (uint i = 0; i < this->_serializable_state_len; i++) {
    const _field_data& field = this->_serializable_state[i];
    switch (field.type) {
    case _field_type::POD:
    case _field_type::ARRAY_FIXED:
      next = new Chunk(field.thing, field.len);
      break;
    case _field_type::ARRAY_VARIABLE:
      next = new Chunk(field.thing, *field.varlen);
      break;
    case _field_type::SERIALIZABLE:
      // TODO:
      break;
    }
    if (!head) {
      head = tail = next;
    } else {
      tail->next = next;
      tail = next;
    }
  }

  return head;
}

const Serializable::Chunk* Serializable::deserialize(const Chunk* c) {
  if (this->_serializable_state == nullptr) {
    fprintf(stderr, "No serializable state defined. Dying...\n");
    assert(false);
  }

  for (uint i = 0; i < this->_serializable_state_len; i++) {
    _field_data& field = this->_serializable_state[i];
    if (c->len != field.len && (field.type == _field_type::ARRAY_VARIABLE && c->len != *field.varlen)) {
      fprintf(stderr, "[Deserialization] Field length mismatch! "
        "This is probably caused by a change in serialization order.\n");
      assert(false);
    }

    switch (field.type) {
    case _field_type::POD:
    case _field_type::ARRAY_FIXED:
    case _field_type::ARRAY_VARIABLE:
      memcpy(field.thing, c->data, c->len);
      c = c->next;
      break;
    case _field_type::SERIALIZABLE:
      // TODO:
      break;
    }
  }
  return c;
}

// WIP
// struct test_struct : public Serializable {
//   uint val;
//   uint val2;
//   test_struct* next;

//   test_struct(uint val) {
//     this->val = val;
//     this->val2 = val + 1;
//     this->next = nullptr;

//     SERIALIZE_START(3)
//       SERIALIZE_SERIALIZABLE(this->next),
//       SERIALIZE_POD(this->val),
//       SERIALIZE_POD(this->val2),
//     SERIALIZE_END(3)
//   }

//   void add(test_struct* next) { this->next = next; }
// };

// static auto test = [](){
//   test_struct* basicboi = new test_struct(1);
//   basicboi->add(new test_struct(2));
//   basicboi->next->add(new test_struct(3));

//   const u8* data;
//   uint len;
//   Serializable::Chunk* base_state = basicboi->serialize();
//   base_state->debugprint();
//   base_state->collate(data, len);

//   basicboi->val = 100;
//   basicboi->next->val = 200;
//   basicboi->next->next->val = 300;

//   const Serializable::Chunk* too = Serializable::Chunk::parse(data, len);
//   too->debugprint();

//   basicboi->deserialize(Serializable::Chunk::parse(data, len));

//   assert(basicboi->val == 1 && basicboi->next->val == 2 && basicboi->next->next->val == 3);

//   return 1;
// }();


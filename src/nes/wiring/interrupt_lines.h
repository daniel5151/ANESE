#pragma once

#include "common/util.h"

#include "common/serializable.h"

namespace Interrupts {
  enum Type {
    NONE  = 0,
    RESET = 1,
    NMI   = 2,
    IRQ   = 3
  };
}

class InterruptLines : public Serializable {
public:
private:
  bool status [4]; // status[0] is unused

  SERIALIZE_START(1, "Interrupts")
    SERIALIZE_POD(status)
  SERIALIZE_END(1)

public:
  InterruptLines();

  InterruptLines(const InterruptLines&) = delete;
  InterruptLines& operator=(InterruptLines other) = delete;

  void clear();

  Interrupts::Type get() const; // return active interrupt with highest priority
  void request(Interrupts::Type type);
  void service(Interrupts::Type type);
};

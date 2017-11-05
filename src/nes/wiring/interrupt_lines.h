#pragma once

#include "common/util.h"

namespace Interrupts {
  enum Type {
    NONE  = 0,
    RESET = 1,
    NMI   = 2,
    IRQ   = 3
  };
}

class InterruptLines {
public:
private:
  bool status [4]; // status[0] is unused

public:
  ~InterruptLines() = default;
  InterruptLines();

  InterruptLines(const InterruptLines&) = delete;
  InterruptLines& operator=(InterruptLines other) = delete;

  void clear();

  Interrupts::Type get() const; // return active interrupt with highest priority
  void request(Interrupts::Type type);
  void service(Interrupts::Type type);
};

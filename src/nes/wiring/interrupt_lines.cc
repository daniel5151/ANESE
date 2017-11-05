#include "interrupt_lines.h"

using namespace Interrupts;

InterruptLines::InterruptLines() {
  this->clear();
}

void InterruptLines::clear() {
  this->status[RESET] = false;
  this->status[IRQ]   = false;
  this->status[NMI]   = false;
}

Interrupts::Type InterruptLines::get() const {
  if (this->status[RESET]) return RESET;
  if (this->status[NMI]  ) return NMI;
  if (this->status[IRQ]  ) return IRQ;
  return NONE;
}

void InterruptLines::request(Interrupts::Type type) {
  if (type == NONE) return;
  this->status[type] = true;
}


void InterruptLines::service(Interrupts::Type type) {
  if (type == NONE) return;
  this->status[type] = false;
}

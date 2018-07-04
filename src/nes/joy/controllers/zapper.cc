#include "zapper.h"

JOY_Zapper::JOY_Zapper(const char* label) : label(label) {
  (void)this->label; // silence unused warning
}

// <Memory>
u8 JOY_Zapper::read(u16 addr) { return this->peek(addr); }
u8 JOY_Zapper::peek(u16 addr) const {
  (void)addr;
  // 0 means light is detected
  return (!this->light << 3) | (this->trigger << 4);
}

void JOY_Zapper::write(u16 addr, u8 val) { (void)addr; (void)val; }
// </Memory>

void JOY_Zapper::set_trigger(bool active) { this->trigger = active; }
void JOY_Zapper::set_light(bool active) { this->light = active; }

bool JOY_Zapper::get_trigger() const { return this->trigger; }
bool JOY_Zapper::get_light() const { return this->light; }

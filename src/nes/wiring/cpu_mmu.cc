#include "cpu_mmu.h"

#include <cassert>
#include <cstdio>

CPU_MMU::CPU_MMU(
  Memory& ram,
  Memory& ppu,
  Memory& apu,
  Memory& joy
)
: ram(ram),
  ppu(ppu),
  apu(apu),
  joy(joy)
{
  this->cart = nullptr;
}

// 0x0000 ... 0x1FFF: 0x0000 - 0x07FF are RAM           (Mirrored 4x)
// 0x2000 ... 0x3FFF: 0x2000 - 0x2007 are PPU Regusters (Mirrored every 8 bytes)
// 0x4000 ... 0x4013: APU registers
// 0x4014           : PPU OAM DMA
// 0x4015           : APU register
// 0x4016           : Joy1 Data (Read) and Joystick Strobe (Write)
// 0x4017           : Joy2 Data (Read) and APU thing       (Write)
// 0x4018 ... 0x401F: APU and I/O functionality that is normally disabled
// 0x4018 ... 0xFFFF: Cartridge ROM (may not be plugged in)

#include "common/overload_macro.h"
#define ADDR(...) VFUNC(ADDR, __VA_ARGS__)
#define ADDR1(lo    ) if (in_range(addr, lo    ))
#define ADDR2(lo, hi) if (in_range(addr, lo, hi))

u8 CPU_MMU::read(u16 addr) {
  ADDR(0x0000, 0x1FFF) return this->ram.read(addr % 0x800);
  ADDR(0x2000, 0x3FFF) return this->ppu.read(addr % 8 + 0x2000);
  ADDR(0x4000, 0x4013) return this->apu.read(addr);
  ADDR(0x4014        ) return this->ppu.read(addr);
  ADDR(0x4015        ) return this->apu.read(addr);
  ADDR(0x4016        ) return this->joy.read(addr);
  ADDR(0x4017        ) return this->joy.read(addr); // not APU
  ADDR(0x4018, 0x401F) return 0x00; // ?
  ADDR(0x4020, 0xFFFF) return this->cart ? this->cart->read(addr) : 0x00;

  fprintf(stderr, "[CPU] unhandled address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

// unfortunately, I have to duplicate this map for peek
u8 CPU_MMU::peek(u16 addr) const {
  ADDR(0x0000, 0x1FFF) return this->ram.peek(addr % 0x800);
  ADDR(0x2000, 0x3FFF) return this->ppu.peek(addr % 8 + 0x2000);
  ADDR(0x4000, 0x4013) return this->apu.peek(addr);
  ADDR(0x4014        ) return this->ppu.peek(addr);
  ADDR(0x4015        ) return this->apu.peek(addr);
  ADDR(0x4016        ) return this->joy.peek(addr);
  ADDR(0x4017        ) return this->joy.peek(addr); // not APU
  ADDR(0x4018, 0x401F) return 0x00; // ?
  ADDR(0x4020, 0xFFFF) return this->cart ? this->cart->peek(addr) : 0x00;

  fprintf(stderr, "[CPU] unhandled address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

void CPU_MMU::write(u16 addr, u8 val) {
  // Some test roms provide test status info in addr 0x6000, and write c-style
  // null-terminated ascii strings starting at 0x6004
  // They signal this behavior by writing 0xDEB061 to 0x6001 - 0x6003
  static uint debug_log = 0;
  if (in_range(addr, 0x6001, 0x6003))
    debug_log |= val << ((2 - (addr - 0x6001)) * 8);

  if (debug_log == 0xDEB061) {
    if (addr == 0x6000)
      fprintf(stderr, "Status: %X\n", val);

    if (in_range(addr, 0x6004, 0x6100)) {
      fprintf(stderr, "%c", val);
    }
  }
  // END DEBUG

  ADDR(0x0000, 0x1FFF) return this->ram.write(addr % 0x800, val);
  ADDR(0x2000, 0x3FFF) return this->ppu.write(0x2000 + addr % 8, val);
  ADDR(0x4000, 0x4013) return this->apu.write(addr, val);
  ADDR(0x4014        ) return this->ppu.write(addr, val);
  ADDR(0x4015        ) return this->apu.write(addr, val);
  ADDR(0x4016        ) return this->joy.write(addr, val);
  ADDR(0x4017        ) return this->apu.write(addr, val); // not JOY
  ADDR(0x4018, 0x401F) return; // ?
  ADDR(0x4020, 0xFFFF) return this->cart ? this->cart->write(addr, val) : void();

  fprintf(stderr, "[CPU] unhandled address: 0x%04X\n", addr);
  assert(false);
}

void CPU_MMU::loadCartridge(Mapper* cart) { this->cart = cart;    }
void CPU_MMU::removeCartridge()           { this->cart = nullptr; }

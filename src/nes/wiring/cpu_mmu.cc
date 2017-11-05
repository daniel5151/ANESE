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
// 0x4018 ... 0xFFFF: Cartridge ROM (may not be plugged in)

u8 CPU_MMU::read(u16 addr) {
  if (in_range(addr, 0x0000, 0x1FFF)) return ram.read(addr % 0x800);
  if (in_range(addr, 0x2000, 0x3FFF)) return ppu.read(addr % 8 + 0x2000);
  if (in_range(addr, 0x4000, 0x4013)) return apu.read(addr);
  if (in_range(addr, 0x4014        )) return ppu.read(addr);
  if (in_range(addr, 0x4015        )) return apu.read(addr);
  if (in_range(addr, 0x4016        )) return joy.read(addr);
  if (in_range(addr, 0x4017        )) return joy.read(addr);
  if (in_range(addr, 0x4018, 0xFFFF)) return cart ? cart->read(addr) : 0x0;

  fprintf(stderr, "[CPU] unhandled address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

// unfortunately, I have to duplicate this map for peek
u8 CPU_MMU::peek(u16 addr) const {
  if (in_range(addr, 0x0000, 0x1FFF)) return ram.peek(addr % 0x800);
  if (in_range(addr, 0x2000, 0x3FFF)) return ppu.peek(addr % 8 + 0x2000);
  if (in_range(addr, 0x4000, 0x4013)) return apu.peek(addr);
  if (in_range(addr, 0x4014        )) return ppu.peek(addr);
  if (in_range(addr, 0x4015        )) return apu.peek(addr);
  if (in_range(addr, 0x4016        )) return joy.peek(addr);
  if (in_range(addr, 0x4017        )) return joy.peek(addr); // not APU
  if (in_range(addr, 0x4018, 0xFFFF)) return cart ? cart->peek(addr) : 0x0;

  fprintf(stderr, "[CPU] unhandled address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

void CPU_MMU::write(u16 addr, u8 val) {
  if (in_range(addr, 0x0000, 0x1FFF)) return ram.write(addr % 0x800, val);
  if (in_range(addr, 0x2000, 0x3FFF)) return ppu.write(0x2000 + addr % 8, val);
  if (in_range(addr, 0x4000, 0x4013)) return apu.write(addr, val);
  if (in_range(addr, 0x4014        )) return ppu.write(addr, val);
  if (in_range(addr, 0x4015        )) return apu.write(addr, val);
  if (in_range(addr, 0x4016        )) return joy.write(addr, val);
  if (in_range(addr, 0x4017        )) return apu.write(addr, val); // not JOY
  if (in_range(addr, 0x4018, 0xFFFF)) return cart ? cart->write(addr, val) : void();

  fprintf(stderr, "[CPU] unhandled address: 0x%04X\n", addr);
  assert(false);
}

void CPU_MMU::loadCartridge(Cartridge* cart) { this->cart = cart;    }
void CPU_MMU::removeCartridge()              { this->cart = nullptr; }

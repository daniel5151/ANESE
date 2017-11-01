#include "ppu_mmu.h"

#include <cassert>
#include <cstdio>

PPU_MMU::PPU_MMU(
  Memory& ciram,
  Memory& pram
)
: ciram(ciram),
  pram(pram)
{
  this->cart = nullptr;

  this->vram = &this->ciram;

  // these are arbitrarily chosen
  this->nt_0 = 0;
  this->nt_1 = 0;
  this->nt_2 = 0;
  this->nt_3 = 0;
}

// 0x0000 ... 0x1FFF: Pattern Tables
// 0x2000 ... 0x23FF: Nametable 0
// 0x2400 ... 0x27FF: Nametable 1
// 0x2800 ... 0x2BFF: Nametable 2
// 0x2C00 ... 0x2FFF: Nametable 3
// 0x3000 ... 0x3EFF: Mirrors of $2000-$2EFF
// 0x3F00 ... 0x3FFF: Palette RAM indexes (Mirrored every 32 bytes)

u8 PPU_MMU::read(u16 addr) {
  if (in_range(addr, 0x0000, 0x1FFF)) return cart ? cart->read(addr) : 0x0;
  if (in_range(addr, 0x2000, 0x23FF)) return vram->read(addr - this->nt_0);
  if (in_range(addr, 0x2400, 0x27FF)) return vram->read(addr - this->nt_1);
  if (in_range(addr, 0x2800, 0x2BFF)) return vram->read(addr - this->nt_2);
  if (in_range(addr, 0x2C00, 0x2FFF)) return vram->read(addr - this->nt_3);
  if (in_range(addr, 0x3000, 0x3EFF)) return this->read(addr - 0x1000);
  if (in_range(addr, 0x3F00, 0x3FFF)) return pram.read(addr % 32 + 0x3F00);
  if (in_range(addr, 0x4000, 0xFFFF)) return this->read(addr - 0x4000);

  fprintf(stderr, "[PPU] unhandled address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

// unfortunately, I have to duplicate this map for peek
u8 PPU_MMU::peek(u16 addr) const {
  if (in_range(addr, 0x0000, 0x1FFF)) return cart ? cart->peek(addr) : 0x0;
  if (in_range(addr, 0x2000, 0x23FF)) return vram->peek(addr - this->nt_0);
  if (in_range(addr, 0x2400, 0x27FF)) return vram->peek(addr - this->nt_1);
  if (in_range(addr, 0x2800, 0x2BFF)) return vram->peek(addr - this->nt_2);
  if (in_range(addr, 0x2C00, 0x2FFF)) return vram->peek(addr - this->nt_3);
  if (in_range(addr, 0x3000, 0x3EFF)) return this->peek(addr - 0x1000);
  if (in_range(addr, 0x3F00, 0x3FFF)) return pram.peek(addr % 32 + 0x3F00);
  if (in_range(addr, 0x4000, 0xFFFF)) return this->peek(addr - 0x4000);

  fprintf(stderr, "[PPU] unhandled address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

void PPU_MMU::write(u16 addr, u8 val) {
  if (in_range(addr, 0x0000, 0x1FFF)) return cart ? cart->write(addr, val) : void();
  if (in_range(addr, 0x2000, 0x23FF)) return vram->write(addr - this->nt_0, val);
  if (in_range(addr, 0x2400, 0x27FF)) return vram->write(addr - this->nt_1, val);
  if (in_range(addr, 0x2800, 0x2BFF)) return vram->write(addr - this->nt_2, val);
  if (in_range(addr, 0x2C00, 0x2FFF)) return vram->write(addr - this->nt_3, val);
  if (in_range(addr, 0x3000, 0x3EFF)) return this->write(addr - 0x1000, val);
  if (in_range(addr, 0x3F00, 0x3FFF)) return pram.write(addr % 32 + 0x3F00, val);
  if (in_range(addr, 0x4000, 0xFFFF)) return this->write(addr - 0x4000, val);

  fprintf(stderr, "[PPU] unhandled address: 0x%04X\n", addr);
  assert(false);
}

void PPU_MMU::loadCartridge(Cartridge* cart) {
  this->cart = cart;

  switch(cart->mirroring()) {
  case Cartridge::Mirroring::Vertical:
    this->vram = &this->ciram;
    this->nt_0 = 0x000; // 0x2000 -> 0x2000
    this->nt_1 = 0x000; // 0x2400 -> 0x2400
    this->nt_2 = 0x800; // 0x2800 -> 0x2000
    this->nt_3 = 0x800; // 0x2C00 -> 0x2400
    break;
  case Cartridge::Mirroring::Horizontal:
    this->vram = &this->ciram;
    this->nt_0 = 0x000; // 0x2000 -> 0x2000
    this->nt_1 = 0x400; // 0x2400 -> 0x2000
    this->nt_2 = 0x400; // 0x2800 -> 0x2400
    this->nt_3 = 0x800; // 0x2C00 -> 0x2400
    break;
  case Cartridge::Mirroring::FourScreen:
    this->vram = this->cart; // use ROM instead of internal VRAM
    this->nt_0 = 0x000; // 0x2000 -> 0x2000
    this->nt_1 = 0x000; // 0x2400 -> 0x2400
    this->nt_2 = 0x000; // 0x2800 -> 0x2800
    this->nt_3 = 0x000; // 0x2C00 -> 0x2C00
    break;
  }
}
void PPU_MMU::removeCartridge() {
  this->cart = nullptr;

  this->vram = &this->ciram;

  this->nt_0 = 0;
  this->nt_1 = 0;
  this->nt_2 = 0;
  this->nt_3 = 0;
}

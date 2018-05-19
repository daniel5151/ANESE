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

  this->mirroring = Mirroring::Type::INVALID;
  this->nt_0 = 0x2000;
  this->nt_1 = 0x2000;
  this->nt_2 = 0x2000;
  this->nt_3 = 0x2000;
}

// 0x0000 ... 0x1FFF: Pattern Tables
// 0x2000 ... 0x23FF: Nametable 0
// 0x2400 ... 0x27FF: Nametable 1
// 0x2800 ... 0x2BFF: Nametable 2
// 0x2C00 ... 0x2FFF: Nametable 3
// 0x3000 ... 0x3EFF: Mirrors of $2000-$2EFF
// 0x3F00 ... 0x3FFF: Palette RAM indexes (Mirrored every 32 bytes)

// Handles the weird Palette RAM mirroring behavior
// (notably, SMB and relies on this)
inline u16 pram_mirror(u16 addr) {
  addr %= 32;

  if (addr % 4 == 0) {
    if (addr >= 16) return addr -= 16;
  }
  return addr;
}

#define ADDR(lo, hi) if (in_range(addr, lo, hi))

u8 PPU_MMU::read(u16 addr) {
  ADDR(0x0000, 0x1FFF) {
    u8 val = this->cart ? this->cart->read(addr) : 0x00;
    this->set_mirroring();
    return val;
  }
  ADDR(0x2000, 0x23FF) return this->vram->read(addr - this->nt_0);
  ADDR(0x2400, 0x27FF) return this->vram->read(addr - this->nt_1);
  ADDR(0x2800, 0x2BFF) return this->vram->read(addr - this->nt_2);
  ADDR(0x2C00, 0x2FFF) return this->vram->read(addr - this->nt_3);
  ADDR(0x3000, 0x3EFF) return this->read(addr - 0x1000);
  ADDR(0x3F00, 0x3FFF) return this->pram.read(pram_mirror(addr));
  ADDR(0x4000, 0xFFFF) return this->read(addr - 0x4000);

  fprintf(stderr, "[PPU_MMU] unhandled read from address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

u8 PPU_MMU::peek(u16 addr) const {
  ADDR(0x0000, 0x1FFF) return this->cart ? this->cart->peek(addr) : 0x00;
  ADDR(0x2000, 0x23FF) return this->vram->peek(addr - this->nt_0);
  ADDR(0x2400, 0x27FF) return this->vram->peek(addr - this->nt_1);
  ADDR(0x2800, 0x2BFF) return this->vram->peek(addr - this->nt_2);
  ADDR(0x2C00, 0x2FFF) return this->vram->peek(addr - this->nt_3);
  ADDR(0x3000, 0x3EFF) return this->peek(addr - 0x1000);
  ADDR(0x3F00, 0x3FFF) return this->pram.peek(pram_mirror(addr));
  ADDR(0x4000, 0xFFFF) return this->peek(addr - 0x4000);

  fprintf(stderr, "[PPU_MMU] unhandled peek from address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

void PPU_MMU::write(u16 addr, u8 val) {
  ADDR(0x0000, 0x1FFF) {
    if (this->cart) {
      this->cart->write(addr, val);
      this->set_mirroring();
    }
    return;
  }
  ADDR(0x2000, 0x23FF) return this->vram->write(addr - this->nt_0, val);
  ADDR(0x2400, 0x27FF) return this->vram->write(addr - this->nt_1, val);
  ADDR(0x2800, 0x2BFF) return this->vram->write(addr - this->nt_2, val);
  ADDR(0x2C00, 0x2FFF) return this->vram->write(addr - this->nt_3, val);
  ADDR(0x3000, 0x3EFF) return this->write(addr - 0x1000, val);
  ADDR(0x3F00, 0x3FFF) return this->pram.write(pram_mirror(addr), val);
  ADDR(0x4000, 0xFFFF) return this->write(addr - 0x4000, val);

  fprintf(stderr, "[PPU_MMU] unhandled write to address: 0x%04X\n", addr);
  assert(false);
}

void PPU_MMU::set_mirroring() {
  // When using internal ciram, a additional 0x2000 offset is applied to the
  // nametable addresses, since the ciram RAM module has no concept of the
  // memory map, and assumes that things will be accessing it from it's internal
  // range (0x0000 -> 0x2000)

  if (this->cart == nullptr) {
    this->vram = &this->ciram;

    this->mirroring = Mirroring::Type::INVALID;
    this->nt_0 = 0x2000;
    this->nt_1 = 0x2000;
    this->nt_2 = 0x2000;
    this->nt_3 = 0x2000;

    return;
  }

  Mirroring::Type old_mirroring = this->mirroring;
  this->mirroring = this->cart->mirroring();

  if (old_mirroring == this->mirroring) return;

  fprintf(stderr,
    "[PPU_MMU] Mirroring: %s -> %s\n",
    Mirroring::toString(old_mirroring),
    Mirroring::toString(this->mirroring)
  );

  // This... this sucks.
  // And should be cleaned up to be more readable.
  // Someday...
  switch(this->mirroring) {
  case Mirroring::Type::Vertical:
    this->vram = &this->ciram;
    this->nt_0 = 0x2000; // 0x2000 -> 0x0000
    this->nt_1 = 0x2000; // 0x2400 -> 0x0400
    this->nt_2 = 0x2800; // 0x2800 -> 0x0000
    this->nt_3 = 0x2800; // 0x2C00 -> 0x0400
    break;
  case Mirroring::Type::Horizontal:
    this->vram = &this->ciram;
    this->nt_0 = 0x2000; // 0x2000 -> 0x0000
    this->nt_1 = 0x2400; // 0x2400 -> 0x0000
    this->nt_2 = 0x2400; // 0x2800 -> 0x0400
    this->nt_3 = 0x2800; // 0x2C00 -> 0x0400
    break;
  case Mirroring::Type::FourScreen:
    this->vram = this->cart; // use ROM instead of internal VRAM
    this->nt_0 = 0x0000; // 0x2000 -> 0x2000 (?)
    this->nt_1 = 0x0000; // 0x2400 -> 0x2400 (?)
    this->nt_2 = 0x0000; // 0x2800 -> 0x2800 (?)
    this->nt_3 = 0x0000; // 0x2C00 -> 0x2C00 (?)
    break;
  case Mirroring::Type::SingleScreenLo:
    this->vram = &this->ciram;
    this->nt_0 = 0x2000; // 0x2000 -> 0x0000
    this->nt_1 = 0x2400; // 0x2400 -> 0x0000
    this->nt_2 = 0x2800; // 0x2800 -> 0x0000
    this->nt_3 = 0x2C00; // 0x2C00 -> 0x0000
    break;
  case Mirroring::Type::SingleScreenHi:
    this->vram = &this->ciram;
    this->nt_0 = 0x1C00; // 0x2000 -> 0x0400
    this->nt_1 = 0x2000; // 0x2400 -> 0x0400
    this->nt_2 = 0x2400; // 0x2800 -> 0x0400
    this->nt_3 = 0x2800; // 0x2C00 -> 0x0400
    break;
  default:
    fprintf(stderr, "[PPU_MMU] Unhandled Mirror Mode!\n");
    assert(false);
    break;
  }
}

void PPU_MMU::loadCartridge(Cartridge* cart) {
  this->cart = cart;
  this->set_mirroring();
}

void PPU_MMU::removeCartridge() {
  this->cart = nullptr;
  this->set_mirroring();
}

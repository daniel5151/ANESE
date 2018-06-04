#include "ppu_mmu.h"

#include <cassert>
#include <cstdio>
#include <cstring>

PPU_MMU::PPU_MMU(
  Memory& ciram,
  Memory& pram
)
: ciram(ciram),
  pram(pram)
{ this->set_mirroring(); }

// 0x0000 ... 0x1FFF: Pattern Tables
// 0x2000 ... 0x23FF: Nametable 0
// 0x2400 ... 0x27FF: Nametable 1
// 0x2800 ... 0x2BFF: Nametable 2
// 0x2C00 ... 0x2FFF: Nametable 3
// 0x3000 ... 0x3EFF: Mirrors of $2000-$2EFF
// 0x3F00 ... 0x3FFF: Palette RAM indexes (Mirrored every 32 bytes)

// Handles the weird Palette RAM mirroring behavior
// (notably, SMB1 relies on this)
inline u16 pram_mirror(u16 addr) {
  addr %= 32;

  if (addr % 4 == 0) {
    if (addr >= 16) return addr -= 16;
  }

  return addr;
}

inline u16 PPU_MMU::nt_mirror(u16 addr) const {
  // When using FourScreen mirroring, the 0x2000 offset that is usually stripped
  // from the address to access ciram must be restored, since accessing the cart
  // from 0x0000 to 0x0FFF is CHR Memory, while the extra cart ram is located
  // from 0x2000 to 0x2FFF!
  const u16 fix_4s = 0x2000 * (this->mirroring == Mirroring::Type::FourScreen);

  return this->nt[(addr - 0x2000) / 0x400] * 0x400 + (addr % 0x400) + fix_4s;
}

#define ADDR(lo, hi) if (in_range(addr, lo, hi))

u8 PPU_MMU::read(u16 addr) {
  this->set_mirroring();

  ADDR(0x0000, 0x1FFF) return this->cart ? this->cart->read(addr) : 0x00;
  ADDR(0x2000, 0x2FFF) return this->vram->read(this->nt_mirror(addr));
  ADDR(0x3000, 0x3EFF) return this->read(addr - 0x1000);
  ADDR(0x3F00, 0x3FFF) return this->pram.read(pram_mirror(addr));
  ADDR(0x4000, 0xFFFF) return this->read(addr - 0x4000);

  fprintf(stderr, "[PPU_MMU] unhandled read from address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

u8 PPU_MMU::peek(u16 addr) const {
  ADDR(0x0000, 0x1FFF) return this->cart ? this->cart->peek(addr) : 0x00;
  ADDR(0x2000, 0x2FFF) return this->vram->peek(this->nt_mirror(addr));
  ADDR(0x3000, 0x3EFF) return this->peek(addr - 0x1000);
  ADDR(0x3F00, 0x3FFF) return this->pram.peek(pram_mirror(addr));
  ADDR(0x4000, 0xFFFF) return this->peek(addr - 0x4000);

  fprintf(stderr, "[PPU_MMU] unhandled peek from address: 0x%04X\n", addr);
  assert(false);
  return 0;
}

void PPU_MMU::write(u16 addr, u8 val) {
  this->set_mirroring();

  ADDR(0x0000, 0x1FFF) return this->cart ? this->cart->write(addr, val) : void();
  ADDR(0x2000, 0x2FFF) return this->vram->write(this->nt_mirror(addr), val);
  ADDR(0x3000, 0x3EFF) return this->write(addr - 0x1000, val);
  ADDR(0x3F00, 0x3FFF) return this->pram.write(pram_mirror(addr), val);
  ADDR(0x4000, 0xFFFF) return this->write(addr - 0x4000, val);

  fprintf(stderr, "[PPU_MMU] unhandled write to address: 0x%04X\n", addr);
  assert(false);
}

void PPU_MMU::set_mirroring() {
  static constexpr uint nt_mirroring [5][4] = {
    /* Vertical       */ { 0, 1, 0, 1 },
    /* Horizontal     */ { 0, 0, 1, 1 },
    /* FourScreen     */ { 0, 1, 2, 3 },
    /* SingleScreenLo */ { 0, 0, 0, 0 },
    /* SingleScreenHi */ { 1, 1, 1, 1 }
  };

  if (this->cart == nullptr) {
    this->mirroring = Mirroring::Type::INVALID;
    this->vram = &this->ciram;
    this->nt = nt_mirroring[Mirroring::Type::SingleScreenLo]; // y not
    return;
  }

  Mirroring::Type old_mirroring = this->mirroring;
  this->mirroring = this->cart->mirroring();
  assert(this->mirroring != Mirroring::Type::INVALID);
  if (old_mirroring == this->mirroring) return;

  // Change mirroring mode!

  fprintf(stderr,
    "[PPU_MMU] Mirroring: %s -> %s\n",
    Mirroring::toString(old_mirroring),
    Mirroring::toString(this->mirroring)
  );

  this->nt = nt_mirroring[this->mirroring];
  this->vram = (this->mirroring == Mirroring::Type::FourScreen)
    ? this->cart // Unlikely, but some games do this (Rad Racer II)
    : &this->ciram;
}

void PPU_MMU::loadCartridge(Mapper* cart) {
  this->cart = cart;
  this->set_mirroring();
}

void PPU_MMU::removeCartridge() {
  this->cart = nullptr;
  this->set_mirroring();
}

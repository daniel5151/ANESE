#include "cpu_mmu.h"

#include <assert.h>

CPU_MMU::CPU_MMU(
  Memory& ram,
  Memory& ppu,
  Memory& apu,
  Memory& dma,
  Memory& joy,

  Memory* rom
)
: ram(ram),
  ppu(ppu),
  apu(apu),
  dma(dma),
  joy(joy),

  rom(rom)
{}

// 0x0000 ... 0x1FFF: 0x0000 - 0x07FF are RAM           (Mirrored 4x)
// 0x2000 ... 0x3FFF: 0x2000 - 0x2007 are PPU Regusters (Mirrored every 8 bytes)
// 0x4000 ... 0x4013: APU registers
// 0x4014           : DMA
// 0x4015           : APU register
// 0x4016           : Joy1 Data (Read) and Joystick Strobe (Write)
// 0x4017           : Joy2 Data (Read) and APU thing (Write)
// 0x4018 ... 0xFFFF: Cartridge ROM (may not be plugged in)

uint8 CPU_MMU::read(uint16 addr) {
  switch (addr) {
  case 0x0000 ... 0x1FFF: return ram.read(addr % 0x1000);
  case 0x2000 ... 0x3FFF: return ppu.read(addr % 8 + 0x2000);
  case 0x4000 ... 0x4013: return apu.read(addr);
  case 0x4014           : return dma.read(addr);
  case 0x4015           : return apu.read(addr);
  case 0x4016           : return joy.read(addr);
  case 0x4017           : return joy.read(addr);
  case 0x4018 ... 0xFFFF: return rom ? rom->read(addr) : 0x0;
  }

  assert(false);
  return 0;
}

void CPU_MMU::write(uint16 addr, uint8 val) {
  switch (addr) {
  case 0x0000 ... 0x1FFF: return ram.write(addr % 0x1000, val);
  case 0x2000 ... 0x3FFF: return ppu.write(0x2000 + addr % 8, val);
  case 0x4000 ... 0x4013: return apu.write(addr, val);
  case 0x4014           : return dma.write(addr, val);
  case 0x4015           : return apu.write(addr, val);
  case 0x4016           : return joy.write(addr, val);
  case 0x4017           : return apu.write(addr, val);
  case 0x4018 ... 0xFFFF: return rom ? rom->write(addr, val) : void();
  }

  assert(false);
}

void CPU_MMU::addCartridge(Memory *cart) { this->rom = cart;    }
void CPU_MMU::removeCartridge()          { this->rom = nullptr; }

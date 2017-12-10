#include "nes.h"

#include "common/debug.h"

// The constructor creates the individual NES components, and "wires them up"
// to one antother
// IT DOES NOT INITIALIZE THEM! A CALL TO power_cycle IS NEEDED TO DO THAT!
NES::NES() {
  this->cart = nullptr;

  // Create RAM modules
  this->cpu_wram = new RAM (0x800, "WRAM");

  this->ppu_vram = new RAM (0x800, "CIRAM");
  this->ppu_pram = new RAM (32, "Palette");
  this->ppu_oam  = new RAM (256, "OAM");
  this->ppu_oam2 = new RAM (32, "Secondary OAM");

  // Create Joypad controller
  this->joy = new JOY ();

  // Create DMA component
  this->dma = new DMA (*this->cpu_wram);

  // Interrupt Lines are created automatically
  // That said, we may as well clear them
  this->interrupts.clear();

  // Create Joypads
  // JOY* joy;

  // Create PPU
  this->ppu_mmu = new PPU_MMU (
    /* vram */ *this->ppu_vram,
    /* pram */ *this->ppu_pram
  );
  this->ppu = new PPU (
    *this->ppu_mmu,
    *this->ppu_oam,
    *this->ppu_oam2,
    *this->dma,
    this->interrupts
  );

  // So, here's a fun fact: the APU is technically part of the CPU in the NES,
  // and as such, shares the CPU's MMU.
  //
  // But that's not good, since the CPU MMU needs a reference to the APU.
  //
  // Looks like we are in a bit of a sticky situation, eh?
  // Circular dependencies, with no clear way to resolve it.
  //
  // Well, instead of refactoring, fuck it, let's have some fun with pointers!
  //
  // After all, we are using C++ ;)

  // First, allocate _raw_ memory for the APU object, wihhout ever calling the
  // APU constructor!
  this->apu = reinterpret_cast<APU*>(new u8 [sizeof(APU)]);

  // Create CPU
  this->cpu_mmu = new CPU_MMU (
    /* ram */ *this->cpu_wram,
    /* ppu */ *this->ppu,
    /* apu */ *this->apu,
    /* joy */ *this->joy
  );
  this->cpu = new CPU (*this->cpu_mmu, this->interrupts);

  // Finally, construct the APU using placement new, allocating it at apu_mem!
  //
  // Since references are simply pointers under the hood, cpu_mmu's apu pointer
  // is still valid once a APU object is constructred :D
  //
  // Wewlad, that's what I call fun on the bun!
  this->apu = new(this->apu) APU (*this->cpu_mmu, this->interrupts);

  /*----------  Emulator Vars  ----------*/

  this->is_running = false;
}

NES::~NES() {
  // Don't delete Cartridge! It's not owned by NES!

  // since APU was allocated with placement new, default ctor must be called
  this->apu->~APU();
  delete this->apu;

  delete this->cpu_mmu;
  delete this->cpu;

  delete this->ppu_mmu;
  delete this->ppu;

  delete this->dma;

  delete this->joy; // wow, just like exams, amirite

  delete this->cpu_wram;
  delete this->ppu_pram;
  delete this->ppu_vram;
  delete this->ppu_oam;
  delete this->ppu_oam2;
}

bool NES::loadCartridge(Cartridge* cart) {
  if (cart == nullptr || cart->isValid() == false)
    return false;

  this->cart = cart;

  this->cpu_mmu->loadCartridge(this->cart);
  this->ppu_mmu->loadCartridge(this->cart);

  return true;
}

void NES::removeCartridge() {
  this->cart = nullptr;

  this->cpu_mmu->removeCartridge();
  this->ppu_mmu->removeCartridge();
}

void NES::attach_joy(uint port, Memory* joy) { this->joy->attach_joy(port, joy); }
void NES::detach_joy(uint port)              { this->joy->detach_joy(port);      }

// Power Cycling initializes all the components to their "power on" state
void NES::power_cycle() {
  this->is_running = true;

  this->apu->power_cycle();
  this->cpu->power_cycle();
  this->ppu->power_cycle();

  this->cpu_wram->clear();
  this->ppu_pram->clear();
  this->ppu_vram->clear();

  this->interrupts.clear();
  this->interrupts.request(Interrupts::RESET);
}

void NES::reset() {
  this->is_running = true;

  // cpu_wram, ppu_pram, and ppu_vram are not affected by resets
  // (i.e: they keep previous state)

  this->apu->reset();
  this->cpu->reset();
  this->ppu->reset();

  this->interrupts.clear();
  this->interrupts.request(Interrupts::RESET);
}

void NES::cycle() {
  if (this->is_running == false) return;

  // Execute a CPU instruction
  uint cpu_cycles = this->cpu->step();

  // Run PPU 3x per cpu_cycle, and APU 1x per cpu_cycle
  for (uint i = 0; i < cpu_cycles * 3; i++) this->ppu->cycle();
  for (uint i = 0; i < cpu_cycles    ; i++) this->apu->cycle();

  // Check if the CPU halted, and stop NES if it is
  if (this->cpu->getState() == CPU::State::Halted)
    this->is_running = false;
}

void NES::step_frame() {
  if (this->is_running == false) return;

  const uint curr_frame = this->ppu->getFrames();
  while (this->is_running && this->ppu->getFrames() == curr_frame) {
    this->cycle();
  }
}

const u8* NES::getFramebuff() const { return this->ppu->getFramebuff(); }

bool NES::isRunning() const { return this->is_running; }

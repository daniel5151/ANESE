#include "nes.h"

#include "common/debug.h"

// The constructor creates the individual NES components, and "wires them up"
// to one antother
// IT DOES NOT INITIALIZE THEM! A CALL TO power_cycle IS NEEDED TO DO THAT!
NES::NES() {
  this->cart = nullptr;

  // Due to the very interconnected nature of hardware, there are several
  // circular dependencies that occur when attempting to init NES hardware.
  //
  // Namely, several components all use the two MMUs, but those MMUs need
  // to know about those components!
  //
  // We can use some fun with C++ pointers and custom memory allocation to
  // work around this though!

  // First, we allocate the _space_ for the PPU and CPU MMUs, and initialize
  // the objects at a later point!
  this->cpu_mmu = reinterpret_cast<CPU_MMU*>(new u8 [sizeof(CPU_MMU)]);
  this->ppu_mmu = reinterpret_cast<PPU_MMU*>(new u8 [sizeof(PPU_MMU)]);
  // Bam! Circular dependencies resolved, and no need to use raw pointers
  // to refer to objects that will never be destroyed!

  // Create RAM modules
  this->cpu_wram = new RAM (0x800, "WRAM");
  this->ppu_vram = new RAM (0x800, "CIRAM");
  this->ppu_pram = new RAM (32, "Palette");
  this->ppu_oam  = new RAM (256, "OAM");
  this->ppu_oam2 = new RAM (32, "Secondary OAM");

  // Create Joypad controller
  this->joy = new JOY ();

  // Create DMA component
  this->dma = new DMA (*this->cpu_mmu);

  // Interrupt Lines are created automatically
  // That said, we may as well clear them
  this->interrupts.clear();

  // Create CPU
  this->cpu = new CPU (
    *this->cpu_mmu,
    this->interrupts
  );

  // Create PPU
  this->ppu = new PPU (
    *this->ppu_mmu,
    *this->ppu_oam,
    *this->ppu_oam2,
    *this->dma,
    this->interrupts
  );

  // Create APU
  this->apu = new APU (
    *this->cpu_mmu,
    this->interrupts
  );

  // Finally, initialize the MMUs using _placement_ new, building them in the
  // space previously allocated for them!

  this->ppu_mmu = new(this->ppu_mmu) PPU_MMU(
      /* vram */ *this->ppu_vram,
      /* pram */ *this->ppu_pram
  );

  this->cpu_mmu = new(this->cpu_mmu) CPU_MMU(
      /* ram */ *this->cpu_wram,
      /* ppu */ *this->ppu,
      /* apu */ *this->apu,
      /* joy */ *this->joy
  );

  /*----------  Emulator Vars  ----------*/

  this->speed = 1;

  this->is_running = false;
}

NES::~NES() {
  // Don't delete Cartridge! It's not owned by NES!

  delete this->apu;
  delete this->cpu;
  delete this->ppu;

  delete this->dma;

  delete this->joy; // wow, just like exams, amirite?

  // Since these two were allocated with _placement_ new,
  // they have to have dtors called manually
  this->cpu_mmu->~CPU_MMU();
  this->ppu_mmu->~PPU_MMU();
  delete this->cpu_mmu;
  delete this->ppu_mmu;

  delete this->cpu_wram;
  delete this->ppu_pram;
  delete this->ppu_vram;
  delete this->ppu_oam;
  delete this->ppu_oam2;
}

bool NES::loadCartridge(Cartridge* cart) {
  if (cart == nullptr)
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

  for (uint i = 0; i < this->speed; i++) {
    const uint curr_frame = this->ppu->getFrames();
    while (this->is_running && this->ppu->getFrames() == curr_frame) {
      this->cycle();
    }
  }
}

const u8* NES::getFramebuff() const {
  return this->ppu->getFramebuff();
}

void NES::getAudiobuff(short*& samples, uint& len) {
  this->apu->getAudiobuff(samples, len);
}

bool NES::isRunning() const { return this->is_running; }

void NES::set_speed(uint speed) {
  if (speed == 0) return;
  this->speed = speed;
  // Also tell Blaarg's APU of this development
  this->apu->set_speed(speed);
}

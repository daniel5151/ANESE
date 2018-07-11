#include "mapper.h"

/*--------------------------------  Helpers  ---------------------------------*/

void Mapper::init_prg_banks(const ROM_File& rom_file, const u16 size) {
  this->banks.prg.len = rom_file.rom.prg.len / size;
  this->banks.prg.bank = new ROM* [this->banks.prg.len];

  fprintf(stderr, "[Mapper] # %2uK PRG ROM Banks: %u\n",
    size / 1024, this->banks.prg.len);

  const u8* p = rom_file.rom.prg.data;
  for (uint i = 0; i < this->banks.prg.len; i++, p += size)
    this->banks.prg.bank[i] = new ROM (size, p, "Mapper PRG");
}

void Mapper::init_chr_banks(const ROM_File& rom_file, const u16 size) {
  if (!rom_file.rom.chr.len) {
    fprintf(stderr, "[Mapper] No CHR ROM detected. Using 8K CHR RAM\n");
    this->banks.chr.is_RAM = true;
    this->banks.chr.len = 0x2000 / size;
  } else {
    this->banks.chr.is_RAM = false;
    this->banks.chr.len = rom_file.rom.chr.len / size;
  }

  this->banks.chr.bank = new Memory* [this->banks.chr.len];

  fprintf(stderr, "[Mapper] # %2uK CHR Banks: %u\n",
    size / 1024, this->banks.chr.len);

  const u8* p = rom_file.rom.chr.data;
  for (uint i = 0; i < this->banks.chr.len; i++, p += size)
    this->banks.chr.bank[i] = this->banks.chr.is_RAM
      ? (Memory*) new RAM (size,    "Mapper CHR RAM")
      : (Memory*) new ROM (size, p, "Mapper CHR ROM");
}

/*------------------  Common Mapper Functions / Services  --------------------*/

void Mapper::irq_trigger() {
  if (this->interrupt_line)
    this->interrupt_line->request(Interrupts::IRQ);
  this->irq_callbacks.run(this);
}

void Mapper::irq_service() {
  if (this->interrupt_line)
    this->interrupt_line->service(Interrupts::IRQ);
}

ROM& Mapper::get_prg_bank(uint bank) const {
  return *this->banks.prg.bank[bank % this->banks.prg.len];
}

Memory& Mapper::get_chr_bank(uint bank) const {
  return *this->banks.chr.bank[bank % this->banks.chr.len];
}

uint Mapper::get_prg_bank_len() const {
  return this->banks.prg.len;
}

uint Mapper::get_chr_bank_len() const {
  return this->banks.chr.len;
}

/*-----------------------  Construction / Destruction  -----------------------*/

Mapper::~Mapper() {
  for (uint i = 0; i < this->banks.prg.len; i++)
    delete this->banks.prg.bank[i];
  delete[] this->banks.prg.bank;

  for (uint i = 0; i < this->banks.chr.len; i++)
    delete this->banks.chr.bank[i];
  delete[] this->banks.chr.bank;
}

Mapper::Mapper(
  uint number, const char* name,
  const ROM_File& rom_file,
  u16 prg_bank_size, u16 chr_bank_size
)
: name(name)
, number(number)
{
  this->init_prg_banks(rom_file, prg_bank_size);
  this->init_chr_banks(rom_file, chr_bank_size);
}

/*---------------------------------  Factory  --------------------------------*/

#include "mappers/mapper_000.h"
#include "mappers/mapper_001.h"
#include "mappers/mapper_002.h"
#include "mappers/mapper_003.h"
#include "mappers/mapper_004.h"
#include "mappers/mapper_007.h"
#include "mappers/mapper_009.h"

Mapper* Mapper::Factory(const ROM_File* rom_file) {
  if (rom_file == nullptr)
    return nullptr;

  switch (rom_file->meta.mapper) {
  case 0: return new Mapper_000(*rom_file);
  case 1: return new Mapper_001(*rom_file);
  case 2: return new Mapper_002(*rom_file);
  case 3: return new Mapper_003(*rom_file);
  case 4: return new Mapper_004(*rom_file);
  case 7: return new Mapper_007(*rom_file);
  case 9: return new Mapper_009(*rom_file);
  }

  return nullptr;
}

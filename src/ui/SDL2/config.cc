#include "config.h"

#include <iostream>

#include <cfgpath.h>
#include <clara.hpp>

Config::Config() { this->ini.SetUnicode(); }

void Config::load(int argc, char* argv[]) {
  // --------------------------- Argument Parsing --------------------------- //

  bool show_help = false;
  auto cli
    = clara::Help(show_help)
    | clara::Opt(this->cli.log_cpu)
        ["--log-cpu"]
        ("Output CPU execution over STDOUT")
    | clara::Opt(this->cli.no_sav)
        ["--no-sav"]
        ("Don't load/create sav/savestate files")
    | clara::Opt(this->cli.ppu_timing_hack)
        ["--alt-nmi-timing"]
        ("Enable NMI timing fix \n"
         "(fixes some games, eg: Bad Dudes, Solomon's Key)")
    | clara::Opt(this->cli.record_fm2_path, "path")
        ["--record-fm2"]
        ("Record a movie in the fm2 format")
    | clara::Opt(this->cli.replay_fm2_path, "path")
        ["--replay-fm2"]
        ("Replay a movie in the fm2 format")
    | clara::Opt(this->cli.config_file, "path")
        ["--config"]
        ("Use custom config file")
    | clara::Opt(this->cli.ppu_debug)
        ["--ppu-debug"]
        ("show ppu debug windows")
    | clara::Opt(this->cli.widenes)
        ["--widenes"]
        ("enable wideNES")
    | clara::Arg(this->cli.rom, "rom")
        ("an iNES rom");

  auto result = cli.parse(clara::Args(argc, argv));
  if(!result) {
    std::cerr << "Error: " << result.errorMessage() << "\n";
    std::cerr << cli;
    exit(1);
  }

  if (show_help) {
    std::cout << cli;
    exit(1);
  }

  // ------------------------- Config File Parsing ------------------------- -//

  // Get cross-platform config path (if no custom path specified)
  if (this->cli.config_file.empty()) {
    cfgpath::get_user_config_file(this->filename, 260, "anese");
  } else {
    strcpy(this->filename, this->cli.config_file.c_str());
  }

  // Try to load config, setting up a new one if none exists
  if (SI_Error err = this->ini.LoadFile(this->filename)) {
    (void)err; // TODO: handle me?
    fprintf(stderr, "[Config] Could not open config file %s!\n", this->filename);
    fprintf(stderr, "[Config]  Will generate a new one.\n");
    this->save();
  }

  // Load config vals
  this->window_scale = this->ini.GetLongValue("ui", "window_scale");
  strcpy(this->roms_dir, this->ini.GetValue("paths", "roms_dir"));
}

void Config::save() {
  this->ini.SetLongValue("ui",    "window_scale", this->window_scale);
  this->ini.SetValue    ("paths", "roms_dir",     this->roms_dir);

  if (SI_Error err = this->ini.SaveFile(this->filename)) {
    (void)err; // TODO: handle me?
    fprintf(stderr, "[Config] Could not save config file %s!\n", this->filename);
  }
}

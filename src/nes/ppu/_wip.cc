//
// This file contains chunks of code that should work, and should be accurate,
// but for whatever reason, don't get the job done properly.
// They live here for safekeeping, so that one day, once I got everything
// "working," I can try to re-integrate these functions for accuracy
//

#include "ppu.h"

#if 0

// Requires the following to be a member of PPU:

// struct {
//   u8 oam2addr; // pointer into oam2
// } spr_eval;

// Implemented verbaitem as documented on nesdev wiki
// https://wiki.nesdev.com/w/index.php/PPU_sprite_evaluation
void PPU::sprite_eval() {
  // Cycles 1-64: Secondary OAM is initialized to $FF
  // NOTE: attempting to read $2004 (OAMDATA) will return $FF
  if (in_range(this->scan.cycle, 1, 64)) {
    // reset relevant state
    if (this->scan.cycle == 1) {
      this->spr_eval.oam2addr = 0;
    }

    // Each write takes 2 cycles, so simulate this by only writing on every
    // other cycle
    if ((this->scan.cycle - 1) % 2 == 0)
      this->oam2[this->spr_eval.oam2addr++] = 0xFF;

    return;
  } else

  // Cycles 65-256: Sprite evaluation
  if (in_range(this->scan.cycle, 65, 256)) {
    const uint sprite_height = this->reg.ppuctrl.H ? 16 : 8;

    // This implementation isn't actually cycle-accurate, since it just does all
    // the calculations on cycle 65.
    // Ideally, c++ would have co-routines and yeild functionality, since that
    // would make this a whole hell of a lot easier, but ah well...
    //
    // Maybe i'll get around to implementing this properly later, but for now,
    // i'm just leaving it innaccurate

    if (this->scan.cycle != 65) return;

    // reset state
    this->reg.oamaddr = 0;
    this->spr_eval.oam2addr = 0;
    bool disable_oam2_write = false;

    u8   y_coord;
    bool y_in_range;
    // I use this->reg.oamaddr as the `n` var as described on nesdev

    // Okay, I know that gotos are generally a bad idea.
    // But in this case, holy cow, to they help a ton!
    // The alternative is adding a whole mess of helper functions that clutter
    // things up, and IMHO, make it complicated to see control flow.

  step1:
    // 1.
    // Starting at n = 0, read a sprite's Y-coordinate (OAM[n][0]),
    // copying it to the next open slot in secondary OAM
    y_coord = this->oam[this->reg.oamaddr];

    if (!disable_oam2_write)
      this->oam2[this->spr_eval.oam2addr] = y_coord;

    // 1a.
    // If Y-coordinate is in range, copy remaining bytes of sprite data
    // (OAM[n][1] thru OAM[n][3]) into secondary OAM.
    y_in_range = in_range(
      y_coord,
      this->scan.line + 1 - (sprite_height - 1),
      this->scan.line + 1
    );

    if (y_in_range && !disable_oam2_write) {
      this->spr_eval.oam2addr++;
      this->reg.oamaddr++;

      this->oam2[this->spr_eval.oam2addr++] = this->oam[this->reg.oamaddr++];
      this->oam2[this->spr_eval.oam2addr++] = this->oam[this->reg.oamaddr++];
      this->oam2[this->spr_eval.oam2addr++] = this->oam[this->reg.oamaddr++];
    } else {
      this->reg.oamaddr += 4;
    }

    // 2a. if n has overflowed back to 0 (i.e: all 64 sprites eval'd), go to 4
    if (this->reg.oamaddr == 0) goto step4;
    // 2b. if less than 8 sprites have been found, go to 1
    if (this->spr_eval.oam2addr < 32) goto step1;
    // 2c. If exactly 8 sprites have been found, disable writes to secondary OAM
    //     This causes sprites in back to drop out.
    if (this->spr_eval.oam2addr >= 32) disable_oam2_write = true;

  step3:
    // 3.
    // Starting at Starting at m = 0, evaluate OAM[n][m] as a Y-coordinate.
    y_coord = this->oam[this->reg.oamaddr];

    // 3a.
    // If the value is in range, set the sprite overflow flag in $2002 and
    // read the next 3 entries of OAM (incrementing 'm' after each byte and
    // incrementing 'n' when 'm' overflows)
    y_in_range = in_range(
      y_coord,
      this->scan.line + 1 - (sprite_height - 1),
      this->scan.line + 1
    );
    if (y_in_range) {
      this->reg.ppustatus.O = true;
      // not sure if these reads are used anywhere...
      this->oam.read(this->reg.oamaddr++);
      this->oam.read(this->reg.oamaddr++);
      this->oam.read(this->reg.oamaddr++);
    }
    // 3b.
    // If the value is not in range, increment n and m (without carry).
    // If n overflows to 0, go to 4; otherwise go to 3
    else {
      // NOTE: the m increment is a hardware bug
      this->reg.oamaddr += 4 + 1;

      if (this->reg.oamaddr == 0) goto step4;
      else                        goto step3;
    }

  step4:
    // 4.
    // Attempt (and fail) to copy OAM[n][0] into the next free slot in secondary
    // OAM, and increment n
    // (repeat until HBLANK is reached)

    // since i'm not cycle-accurate, just return here...
    return;
  } else

  // Cycles 257-320: Sprite fetches (8 sprites total, 8 cycles per sprite)
  if (in_range(this->scan.cycle, 257, 320)) {
    // reset state
    if (this->scan.cycle == 257) {
      this->spr_eval.oam2addr = 0;
    }

    const uint sprite = (this->scan.cycle - 257) / 8;
    const uint step = (this->scan.cycle - 257) % 8;

    // i can't tell if these reads are being used, but i'm going to follow what
    // the wiki says.
    if (in_range(step, 0, 3)) this->oam2.read(sprite * 4 + step);
    if (in_range(step, 4, 7)) this->oam2.read(sprite * 4 + 3);
  } else

  // Cycles 321-340+0: Background render pipeline initialization
  if (in_range(this->scan.cycle, 321, 340) || this->scan.cycle == 0) {
    // Read the first byte in secondary OAM

    // again, this read doesn't seem to be used anywhere...
    this->oam2.read(0x00);
  }
}

#endif

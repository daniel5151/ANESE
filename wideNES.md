# WideNES

Unlike many modern titles, NES games generally lacked "world maps" or "level
viewers." If you wanted to map-out the a level, you had to do it manually,
drawing a map by hand, or stitching together a bunch of screenshots.

That's a lot of work.

Wouldn't it be cool to automate that?

Enter **wideNES**, a novel method to map-out NES games automatically.

<p align="center">
  <img src="resources/web/wideNES_smb1.gif" alt="wideNES on SMB1">
</p>

Pretty cool huh? Here's another one:

<p align="center">
  <img src="resources/web/wideNES_metroid.gif" alt="wideNES on Metroid">
</p>

## Enabling wideNES

You can enable wideNES by passing the `--widenes` flag from the command-line.

## Controls

wideNES inherits all controls from ANESE, and adds a few additional ones:

#### Pan and Zoom

You can **pan** and **zoom** using the mouse + mousewheel.

#### Padding controls

wideNES has many built-in heuristics that are used to "guess" what parts of the
screen are not part of the level (i.e: status bars / leave artifacts), and while
these work pretty well, there are times some manual tweaking might be preferred.

 Side   | increase | decrease
--------|----------|-------
 Left   | s        | a
 Right  | d        | f
 Top    | e        | 3
 Bottom | d        | c

(hold shift for fine-grained control)

The keys make more sense when laid out visually:

<img height="256px" src="resources/web/wideNES_controls.png" alt="wideNES keyboard controls">

## How does this work?

The NES's PPU (graphics processor) supports _hardware scrolling_, i.e: there is
a specific register, 0x2005 - PPUSCROLL, that games can write to and have the
entire screen scroll by a certain amount.

WideNES watches the scroll register for changes, and when values are written to
it, the deltas between values at different frames (plus some heuristics) are
used to intelligently "sample" the framebuffer. Gradually, as the player moves
around the world, more and more of the screen is revealed (and saved).

That's it really!

## Caveats

#### Custom Scrolling implementations

There are some games that use unorthadox scrolling strategies. Take for example,
_The Legend of Zelda_. Although it does in fact use the scroll register to do
left-and-right screen transitions, it uses a custom technique to do up-and-down
screen transitions, never touching the scroll register.

I have implemented a heuristic that has _The Legend of Zelda_ kind-of working,
but it involved a non-trivial amount of work sniffing memory values and such.
Moreover, I have not tested enough games to confidently say the heuristic is
game-agnostic (although it is written quite generally)

#### Sprites as Background Elements

At the moment, wideNES only builds up the map using the background layer,
ignoring all sprites. That means any sprites that are _thematically_ part of the
background are ignored.

#### Non-euclidean levels

wideNES assumes that if you go in a circle, you end up where you started.
Most games follow this rule, but there are exceptions. For example, the Lost
Woods in _The Legend of Zelda_.

I haven't looked into this yet, so I cannot be sure if there is a work-around.

## Roadmap

- [x] avoid smearing in some games
- [x] mask-off edges (artifacting / static menus)
  - [x] manually
  - [x] automatically, _using heuristics_
    - [x] bgr mask (many games)
    - [x] MMC3 IRQ (SMB3, M.C Kids)
    - [x] mid-frame PPUADDR changes (TLOZ)
    - [ ] scrolling/mirroring mode mismatch
- [x] Handle "non-traditional" scrolling
  - Partially, seems to works with _TLOZ_
- [ ] remembering what's been seen
  - [x] Recognize scene-changes
    - using _very_ basic perceptual hashing
  - [ ] Scene change -> finds scene if exists
  - [ ] Save/Load tiles / scenes
- [ ] Animated background / tile support
  - [ ] Normalize wrt. current palette
  - [ ] Normalize wrt. tiles (might have to inspect RAM, eew)


# WideNES

Unlike many modern titles, NES games generally lacked "world maps" or "level
viewers." If you wanted to map-out the a level, you had to do it manually,
drawing a map by hand, or stitching together a bunch of screenshots.

That's a lot of work.

Wouldn't it be cool to automate that?

Enter **WideNES**, a novel method to map-out NES games automatically.

<p align="center">
  <img src="resources/web/wideNES_smb1.gif" alt="wideNES on SMB1">
</p>

Pretty cool huh? Here's another one:

<p align="center">
  <img src="resources/web/wideNES_metroid.gif" alt="wideNES on Metroid">
</p>

## Enabling wideNES

At the moment, wideNES is activated by default.

_this is not final_

## Controls

wideNES inherits all controls from ANESE, and adds a few more:

#### Pan and Zoom

You can **pan** and **zoom** using the mouse + mousewheel.

#### Padding controls

Many games have static sections of the screen / artifacts at the edge of the
screen. wideNES uses some heuristics (if the PPUMASK bgr-mask is active, mapper
IRQs, etc...) to intelligently "guess" padding values, but there are times when
a little bit of tweaking is needed...

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
around the world, more and more of the screne is revealed (and saved).

That's it really!

## Caveats

#### Custom Scrolling implementations

There are some games that use unorthadox scrolling strategies. Take for example,
_The Legend of Zelda_. Although it does in fact use the scroll register to do
left-and-right screen transitions, it uses a custom technique to do up-and-down
screen transitions, never touching the scroll register.

With it's limited knowledge of the NES, wideNES cannot account for such cases
at the moment. (although it is something I would like to look into)

#### Sprites as Background Elements

At the moment, wideNES only builds up the map using the background layer,
ignoring all sprites. That means any sprites that are _thematically_ part of the
background are ignored.

#### Non-euclidean levels

wideNES assumes that if you go in a circle, you end up where you started.
Most games follow this rule, but there are exceptions. For example, the Lost
Woods in _The Legend of Zelda_.

I am not sure if I can work around this, but I might try...

## Roadmap

- [x] avoid smearing in some games
- [x] mask-off edges (artifacting / satic menus)
  - [x] manually
  - [x] automatically - _using heuristics, not completely perfect_
- [ ] Save/Load tiles
  - Persistent levels!
- [ ] Recognize scene-changes
  - i.e: smb1 level transitions, game over screens, etc...
    - right now, they overwrite tile data
- [ ] Animated background / tile support
  - only sample in 8 pixel chunks?
- [ ] Handle non-hardware scrolling (?)

## Successes

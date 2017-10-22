;
; Simple sprite demo for NES (with paddle support)
; Copyright 2013 Damian Yerrick
;
; Copying and distribution of this file, with or without
; modification, are permitted in any medium without royalty provided
; the copyright notice and this notice are preserved in all source
; code copies.  This file is offered as-is, without any warranty.
;

.include "nes.h"
.include "ram.h"

.segment "ZEROPAGE"
nmis:          .res 1
oam_used:      .res 1  ; starts at 0
new_keys:      .res 1

OAM = $0200

; Game variables
player_xlo:       .res 1  ; horizontal position is xhi + xlo/256 px
player_xhi:       .res 1
player_dxlo:      .res 1  ; speed in pixels per 256 s
player_yhi:       .res 1
player_facing:    .res 1
player_frame:     .res 1
player_frame_sub: .res 1
control_type:     .res 1
paddle_min:       .res 1
paddle_max:       .res 1
indicator_x:      .res 1
target_x:         .res 1


.segment "INESHDR"
  .byt "NES",$1A  ; magic signature
  .byt 1          ; PRG ROM size in 16384 byte units
  .byt 1          ; CHR ROM size in 8192 byte units
  .byt $00        ; mirroring type and mapper number lower nibble
  .byt $00        ; mapper number upper nibble

.segment "VECTORS"
.addr nmi, reset, irq

.segment "CODE"
;;
; Good enough for a simple "has NMI occurred?" vblank-detect loop
.proc nmi
  inc nmis
  rti
.endproc

; A null IRQ handler that just does RTI
.proc irq
  rti
.endproc

; 
.proc reset
  ; The very first thing to do when powering on is to put all sources
  ; of interrupts into a known state.
  sei             ; Disable interrupts
  ldx #$00
  stx PPUCTRL     ; Disable NMI and set VRAM increment to 32
  stx PPUMASK     ; Disable rendering
  stx $4010       ; Disable DMC IRQ
  dex             ; Subtracting 1 from $00 gives $FF, which is a
  txs             ; quick way to set the stack pointer to $01FF
  bit PPUSTATUS   ; Acknowledge stray vblank NMI across reset
  bit SNDCHN      ; Acknowledge DMC IRQ
  lda #$40
  sta P2          ; Disable APU Frame IRQ
  lda #$0F
  sta SNDCHN      ; Disable DMC playback, initialize other channels

vwait1:
  bit PPUSTATUS   ; It takes one full frame for the PPU to become
  bpl vwait1      ; stable.  Wait for the first frame's vblank.

  ; We have about 29700 cycles to burn until the second frame's
  ; vblank.  Use this time to get most of the rest of the chipset
  ; into a known state.
  cld
  ldx #0
  jsr ppu_clear_oam  ; clear out OAM from X to end and set X to 0
  txa
clear_zp:
  sta $00,x
  inx
  bne clear_zp

vwait2:
  bit PPUSTATUS  ; After the second vblank, we know the PPU has
  bpl vwait2     ; fully stabilized.

start_over:
  lda #0
  sta PPUMASK
  sta PPUCTRL
  ; This means switch from spinning on PPUSTATUS to waiting for the
  ; NMI handler to run.
  jsr load_main_palette
  jsr draw_bg

  ; Set up game variables, as if it were the start of a new level.
  lda #0
  sta player_xlo
  sta player_dxlo
  sta player_facing
  sta player_frame
  sta control_type
  sta paddle_max
  lda #48
  sta player_xhi
  lda #192
  sta player_yhi
  sta paddle_min

mainloop:

  ; Game logic
  lda cur_keys_d0
  pha
  jsr read_all_pads
  jsr fixup_fc_control
  pla
  eor #$FF
  and cur_keys_d0
  sta new_keys
  jsr move_player

  ldx #0
  stx oam_used
  jsr draw_player_sprite
  jsr draw_indicator_sprite
  ldx oam_used
  jsr ppu_clear_oam

  ; Good; we have the full screen ready.  Wait for a vertical blank
  ; and set the scroll registers to display it.
  lda nmis
vw3:
  cmp nmis
  beq vw3
  
  ; Copy the display list from main RAM to the PPU
  lda #0
  sta OAMADDR
  lda #>OAM
  sta OAM_DMA
  
  ; Turn the screen on
  ldx #0
  ldy #0
  lda #VBLANK_NMI|BG_0000|OBJ_1000
  sec
  jsr ppu_screen_on

  ; See if we want to go to the detailed tests
  lda control_type
  lsr a
  beq mainloop
  lda new_keys
  and #KEY_SELECT
  beq mainloop

  jsr run_timing_test
  jsr run_derivative_test
  jmp start_over
.endproc

.proc load_main_palette
  ; seek to the start of palette memory ($3F00-$3F1F)
  ldx #$3F
  stx PPUADDR
  ldx #$00
  stx PPUADDR
copypalloop:
  lda initial_palette,x
  sta PPUDATA
  inx
  cpx #32
  bcc copypalloop
  rts
.endproc
.segment "RODATA"
initial_palette:
  .byt $22,$18,$28,$38,$0F,$06,$16,$26,$0F,$08,$19,$2A,$0F,$02,$12,$22
  .byt $22,$08,$15,$27,$0F,$06,$16,$26,$0F,$0A,$1A,$2A,$0F,$02,$12,$22

.segment "CODE"

.proc draw_bg
  ; Start by clearing the first nametable
  lda #VBLANK_NMI
  sta PPUCTRL
  asl a
  ldx #$20
  lda #$00
  ldy #$AA
  jsr ppu_clear_nt

  ; Draw a floor
  lda #$23
  sta PPUADDR
  lda #$00
  sta PPUADDR
  lda #$0B
  ldx #32
floorloop1:
  sta PPUDATA
  dex
  bne floorloop1
  
  ; Draw areas buried under the floor as solid color
  ; (I learned this style from "Pinobee" for GBA.  We drink Ritalin.)
  lda #$01
  ldx #5*32
floorloop2:
  sta PPUDATA
  dex
  bne floorloop2

  ; Draw blocks on the sides, in vertical columns
  lda #VBLANK_NMI|VRAM_DOWN
  sta PPUCTRL
  
  ; At position (2, 20) (VRAM $2282) and (28, 20) (VRAM $229C),
  ; draw two columns of two blocks each, each block being 4 tiles:
  ; 0C 0D
  ; 0E 0F
  ldx #2

colloop:
  lda #$22
  sta PPUADDR
  txa
  ora #$80
  sta PPUADDR

  ; Draw $0C $0E $0C $0E or $0D $0F $0D $0F depending on column
  and #$01
  ora #$0C
  ldy #4
tileloop:
  sta PPUDATA
  eor #$02
  dey
  bne tileloop

  ; Columns 2, 3, 28, and 29 only  
  inx
  cpx #4  ; Skip columns 4 through 27
  bne not4
  ldx #28
not4:
  cpx #30
  bcc colloop

  ; The attribute table elements corresponding to these stacks are
  ; (0, 5) (VRAM $23E8) and (7, 5) (VRAM $23EF).  Set them to 0.
  ldx #$23
  lda #$E8
  ldy #$00
  stx PPUADDR
  sta PPUADDR
  sty PPUDATA
  lda #$EF
  stx PPUADDR
  sta PPUADDR
  sty PPUDATA

  lda #VBLANK_NMI
  sta PPUCTRL
  lda #$20
  sta $03
  lda #$82
  sta $02
  lda #>bootmsg
  ldy #<bootmsg
  jsr puts_multiline
  rts
.endproc

.segment "RODATA"
bootmsg:
  .byte "NES PADDLE CONTROLLER TEST",LF
  .byte "COPR. 2013 DAMIAN YERRICK",LF
  .byte "WWW.PINEIGHT.COM",LF,LF
  .byte "PRESS THE A BUTTON OR THE",LF
  .byte "FIRE BUTTON, THEN MOVE WITH",LF
  .byte "THE CONTROL PAD OR KNOB.",LF,LF
  .byte "SELECT: GO TO DETAILED TEST",0

.segment "CODE"

;;
; Autodetects control type and translates to NES.
.proc fixup_fc_control
  ; First try to determine what controller is connected.
  ; A or Start pressed on controller 1:
  ;   Disable translation
  ; A or Start pressed on FC controller 3:
  ;   Treat FC controller 3 as controller 1
  ; A pressed on FC paddle:
  ;   Use FC paddle (port 2 d1)
  ; A pressed on NES paddle in port 2:
  ;   Use NES paddle (port 2 d4)
  ldx #0
  lda cur_keys_d0
  cmp #KEY_A
  beq set_control_type_x
  cmp #KEY_START
  beq set_control_type_x
  inx
  lda cur_keys_d1
  cmp #KEY_A
  beq set_control_type_x
  cmp #KEY_START
  beq set_control_type_x
  inx
  cmp #$FF
  beq set_control_type_x
  inx
  lda cur_keys_d3+1
  cmp #$FF
  bne no_set_control_type
set_control_type_x:
  stx control_type
no_set_control_type:
  lda control_type
  asl a
  tax
  lda control_type_handlers+1,x
  pha
  lda control_type_handlers+0,x
  pha
  rts

control_type_fc4p:
  lda cur_keys_d1
  ora cur_keys_d0
  sta cur_keys_d0
  ; fall through
control_type_null:
  ; Optional: construct new_keys
  rts
control_type_fcvaus:
  lda cur_keys_d1
  ldx cur_keys_d1+1
  jmp handle_vaus
control_type_nesvaus:
  lda cur_keys_d3+1
  ldx cur_keys_d4+1
handle_vaus:
  and #KEY_A
  ora cur_keys_d0
  sta cur_keys_d0
  txa
  eor #$FF  ; change to increasing right
  sta indicator_x
  
  ; Find target relative to center of observed range
  cmp paddle_min
  bcs :+
  sta paddle_min
:
  cmp paddle_max
  bcc :+
  sta paddle_max
  clc
:
  lda paddle_min
  adc paddle_max
  ror a
  eor #$7F
  adc indicator_x
  sta target_x
  
SLOP = 4
TARGET_WID = 7
PLAYER_WID = 16
  ; Now move toward the target
  sec
  sbc #<(SLOP + (TARGET_WID - PLAYER_WID) / 2)
  sec
  sbc player_xhi
  bcs not1
  lda #KEY_LEFT
  bne have_extra_keypress
not1:
  cmp #SLOP * 2 + 1
  bcc not2
  lda #KEY_RIGHT
have_extra_keypress:
  ora cur_keys_d0
  sta cur_keys_d0
not2:
  rts

.pushseg
.segment "RODATA"
control_type_handlers:
  .addr control_type_null-1, control_type_fc4p-1
  .addr control_type_fcvaus-1, control_type_nesvaus-1
.popseg
.endproc
  


; constants used by move_player
; PAL frames are about 20% longer than NTSC frames.  So if you make
; dual NTSC and PAL versions, or you auto-adapt to the TV system,
; you'll want PAL velocity values to be 1.2 times the corresponding
; NTSC values, and PAL accelerations should be 1.44 times NTSC.
WALK_SPD = 85   ; speed limit in 1/256 px/frame
WALK_ACCEL = 4  ; movement acceleration in 1/256 px/frame^2
WALK_BRAKE = 8  ; stopping acceleration in 1/256 px/frame^2

.proc move_player

  ; Acceleration to right: Do it only if the player is holding right
  ; on the Control Pad and has a nonnegative velocity.
  lda cur_keys_d0
  and #KEY_RIGHT
  beq notRight
  lda player_dxlo
  bmi notRight
  
  ; Right is pressed.  Add to velocity, but don't allow velocity
  ; to be greater than the maximum.
  clc
  adc #WALK_ACCEL
  cmp #WALK_SPD
  bcc :+
  lda #WALK_SPD
:
  sta player_dxlo
  lda player_facing  ; Set the facing direction to not flipped 
  and #<~$40
  sta player_facing
  jmp doneRight

  ; Right is not pressed.  Brake if headed right.
notRight:
  lda player_dxlo
  bmi doneRight
  cmp #WALK_BRAKE
  bcs notRightStop
  lda #WALK_BRAKE+1  ; add 1 to compensate for the carry being clear
notRightStop:
  sbc #WALK_BRAKE
  sta player_dxlo
doneRight:

  ; Acceleration to left: Do it only if the player is holding left
  ; on the Control Pad and has a nonpositive velocity.
  lda cur_keys_d0
  and #KEY_LEFT
  beq notLeft
  lda player_dxlo
  beq :+
  bpl notLeft
:

  ; Left is pressed.  Add to velocity.
  lda player_dxlo
  sec
  sbc #WALK_ACCEL
  cmp #256-WALK_SPD
  bcs :+
  lda #256-WALK_SPD
:
  sta player_dxlo
  lda player_facing  ; Set the facing direction to flipped
  ora #$40
  sta player_facing
  jmp doneLeft

  ; Left is not pressed.  Brake if headed left.
notLeft:
  lda player_dxlo
  bpl doneLeft
  cmp #256-WALK_BRAKE
  bcc notLeftStop
  lda #256-WALK_BRAKE
notLeftStop:
  adc #8-1
  sta player_dxlo
doneLeft:

  ; In a real game, you'd respond to A, B, Up, Down, etc. here.

  ; Move the player by adding the velocity to the 16-bit X position.
  lda player_dxlo
  bpl player_dxlo_pos
  ; if velocity is negative, subtract 1 from high byte to sign extend
  dec player_xhi
player_dxlo_pos:
  clc
  adc player_xlo
  sta player_xlo
  lda #0          ; add high byte
  adc player_xhi
  sta player_xhi

  ; Test for collision with side walls
  cmp #28
  bcs notHitLeft
  lda #28
  sta player_xhi
  lda #0
  sta player_dxlo
  beq doneWallCollision
notHitLeft:
  cmp #212
  bcc notHitRight
  lda #211
  sta player_xhi
  lda #0
  sta player_dxlo
notHitRight:
doneWallCollision:
  
  ; Animate the player
  ; If stopped, freeze the animation on frame 0 or 1
  lda player_dxlo
  bne notStop1
  lda #$80
  sta player_frame_sub
  lda player_frame
  cmp #2
  bcc have_player_frame
  lda #0
  beq have_player_frame
notStop1:

  ; Take absolute value of velocity (negate it if it's negative)
  bpl player_animate_noneg
  eor #$FF
  clc
  adc #1
player_animate_noneg:

  lsr a  ; Multiply abs(velocity) by 5/16
  lsr a
  sta 0
  lsr a
  lsr a
  adc 0

  ; And 16-bit add it to player_frame, mod $600  
  adc player_frame_sub
  sta player_frame_sub
  lda player_frame
  adc #0
  cmp #6
  bcc have_player_frame
  lda #0
have_player_frame:
  sta player_frame

  rts
.endproc


;;
; Draws the player's character to the display list as six sprites.
; In the template, we don't need to handle half-offscreen actors,
; but a scrolling game will need to "clip" sprites (skip drawing the
; parts that are offscreen).
.proc draw_player_sprite
draw_y = 0
cur_tile = 1
x_add = 2         ; +8 when not flipped; -8 when flipped
draw_x = 3
rows_left = 4
row_first_tile = 5
draw_x_left = 7

  lda #3
  sta rows_left
  
  ; In platform games, the Y position is often understood as the
  ; bottom of a character because that makes certain things related
  ; to platform collision easier to reason about.  Here, the
  ; character is 24 pixels tall, and player_yhi is the bottom.
  ; On the NES, sprites are drawn one scanline lower than the Y
  ; coordinate in the OAM entry (e.g. the top row of pixels of a
  ; sprite with Y=8 is on scanline 9).  But in a platformer, it's
  ; also common practice to overlap the bottom row of a sprite's
  ; pixels with the top pixel of the background platform that they
  ; walk on to suggest depth in the background.
  lda player_yhi
  sec
  sbc #24
  sta draw_y

  ; set up increment amounts based on flip value
  ; A: distance to move the pen (8 or -8)
  ; X: relative X position of first OAM entry
  lda player_xhi
  ldx #8
  bit player_facing
  bvc not_flipped
  clc
  adc #8
  ldx #(256-8)
not_flipped:
  sta draw_x_left
  stx x_add

  ; the six frames start at $10, $12, ..., $1A  
  lda player_frame
  asl a
  ora #$10
  sta row_first_tile

  ldx oam_used
rowloop:
  ldy #2              ; Y: remaining width on this row in 8px units
  lda row_first_tile
  sta cur_tile
  lda draw_x_left
  sta draw_x
tileloop:

  ; draw an 8x8 pixel chunk of the character using one entry in the
  ; display list
  lda draw_y
  sta OAM,x
  lda cur_tile
  inc cur_tile
  sta OAM+1,x
  lda player_facing
  sta OAM+2,x
  lda draw_x
  sta OAM+3,x
  clc
  adc x_add
  sta draw_x
  
  ; move to the next entry of the display list
  inx
  inx
  inx
  inx
  dey
  bne tileloop

  ; move to the next row, which is 8 scanlines down and on the next
  ; row of tiles in the pattern table
  lda draw_y
  clc
  adc #8
  sta draw_y
  lda row_first_tile
  clc
  adc #16
  sta row_first_tile
  dec rows_left
  bne rowloop

  stx oam_used
  rts
.endproc

.proc draw_indicator_sprite
  lda control_type
  cmp #2
  bcc skip
  ldy #15
  lda #15-1  ; carry is set here
  adc oam_used
  tax
copyloop:
  lda indicator_tmpl,y
  sta OAM,x
  dex
  dey
  bpl copyloop
  inx
  lda paddle_min
  sta OAM+3,x
  lda paddle_max
  sta OAM+7,x
  lda indicator_x
  sta OAM+11,x
  lda target_x
  sta OAM+15,x
  txa
  clc
  adc #16
  sta oam_used
skip:
  rts
.pushseg
.segment "RODATA"
indicator_tmpl:
  .byte 127, $04, $00, 0
  .byte 127, $04, $40, 0
  .byte 127, $05, $00, 0
  .byte 160, $05, $00, 0
.popseg
.endproc

;;
; @param A high byte of string
; @param Y low byte of string
; @param $02 nametable destination address
.proc puts_multiline
srcLo = $00
srcHi = $01
dstLo = $02
dstHi = $03

  sta srcHi
  ldx #0
  stx srcLo  ; keep the low byte in Y

newline:
  lda dstHi
  sta PPUADDR
  lda dstLo
  sta PPUADDR
  clc
  adc #32
  sta dstLo
  bcc :+
  inc dstHi
:
  ldx #0
charloop:
  lda (srcLo),y
  beq done
  iny
  bne :+
  inc srcHi
:
  cmp #LF
  beq newline
  ;and #$3F
  sta PPUDATA
  bpl charloop
done:
  rts
.endproc

.include "nes.h"
.include "ram.h"

.segment "ZEROPAGE"
indicator_xtm1: .res 1
indicator_xtm2: .res 1

displacement_bcd: .res 2

velocity_now: .res 1
velocity_peak: .res 1
velocity_holdtime: .res 1
velocity_bcd: .res 2
velocity_sign: .res 1
velocity_peak_bcd: .res 2

accel_now: .res 1
accel_peak: .res 1
accel_holdtime: .res 1
accel_bcd: .res 2
accel_sign: .res 1
accel_peak_bcd: .res 2

poll_period = accel_now

.segment "TIMEDCODE"

; Wait 1791*X cycles
.proc wait_x_ms
  ldy #105
yloop:  ; 17 cycles per iteration
  jsr skipbo
  dey
  bne yloop
  dex
  bne wait_x_ms
skipbo:
  rts
.endproc

.segment "CODE"
.proc detailed_read_pads
  lda cur_keys_d0
  pha
  jsr read_all_pads
  lda control_type
  lsr a
  bcs control_type_nesvaus
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
  
  ; detect new button presses
  pla
  eor #$FF
  and cur_keys_d0
  sta new_keys
  rts
.endproc

.proc run_timing_test
  lda #17
  sta poll_period
  lda #$FF
  sta cur_keys_d0

  ; set up bg
  lda #$80
  sta PPUCTRL
  asl a
  sta PPUMASK
  ldx #$20
  txa
  ldy #$AA
  jsr ppu_clear_nt
  lda #$20
  sta 3
  lda #$C2
  sta 2
  lda #>pollrate_msg
  ldy #<pollrate_msg
  jsr puts_multiline
  
  ; Statically allocate OAM; only sprite 0 is used
  ldx #4
  jsr ppu_clear_oam

loop:
  lda cur_keys_d0  ; save current keys so that only the last
  pha              ; new_keys processing actually takes effect
  jsr detailed_read_pads
  ldx poll_period
  dex
  beq :+
  jsr wait_x_ms
:  
  jsr detailed_read_pads
  ldx poll_period
  dex
  beq :+
  jsr wait_x_ms
:
  pla
  sta cur_keys_d0
  jsr detailed_read_pads
  
  ; Change poll period
  lda new_keys
  lsr a
  bcc notRight
  lda poll_period
  cmp #250
  bcs notLeft
  inc poll_period
  bcc notLeft
notRight:
  lsr a
  bcc notLeft
  lda poll_period
  cmp #2
  bcc notLeft
  dec poll_period
  bcs notLeft
notLeft:
  
  ; decimalize poll period
  ; decimalize reading
  
  ; show reading as sprite
  lda #87
  sta OAM+0
  lda #$05
  sta OAM+1
  lda #0
  sta OAM+2
  lda indicator_x
  sta OAM+3

  ; convert reading to bcd
  jsr bcd8bit
  sta displacement_bcd
  lda 0
  sta displacement_bcd+1

  lda poll_period
  jsr bcd8bit
  sta accel_bcd
  lda 0
  sta accel_bcd+1

  lda nmis
:
  cmp nmis
  beq :-

  ; Print current poll period
  lda #$21
  sta PPUADDR
  lda #$16
  sta PPUADDR
  lda accel_bcd+1
  ldy accel_bcd+0
  jsr write_bcd_ay
  bit PPUDATA
  lda #'M'
  sta PPUDATA
  lda #'S'
  sta PPUDATA

  ; Print current reading
  lda #$21
  sta PPUADDR
  lda #$56
  sta PPUADDR
  lda displacement_bcd+1
  ldy displacement_bcd+0
  jsr write_bcd_ay

  ldx #0
  lda #>OAM
  stx OAMADDR
  sta OAM_DMA
  ldy #0
  lda #VBLANK_NMI|BG_0000|OBJ_1000
  sec
  jsr ppu_screen_on
  lda new_keys
  and #KEY_SELECT
  bne :+
  jmp loop
:
  rts
.endproc

.proc run_derivative_test
  ldx #0
  stx velocity_peak
  stx velocity_holdtime
  stx accel_peak
  stx accel_holdtime
  dex
  stx cur_keys_d0
  lda indicator_x
  sta indicator_xtm1
  sta indicator_xtm2
  
  ; set up bg
  lda #$80
  sta PPUCTRL
  asl a
  sta PPUMASK
  ldx #$20
  txa
  ldy #$AA
  jsr ppu_clear_nt
  lda #$20
  sta 3
  lda #$C2
  sta 2
  lda #>derivative_msg
  ldy #<derivative_msg
  jsr puts_multiline

loop:
  lda indicator_xtm1
  sta indicator_xtm2
  lda indicator_x
  sta indicator_xtm1
  jsr detailed_read_pads

  lda indicator_x
  jsr bcd8bit
  sta displacement_bcd
  lda 0
  sta displacement_bcd+1

  ; Velocity = (x[t] - x[t - 2]) / 2
  lda indicator_x
  sec
  sbc indicator_xtm2
  ror a     ; Divide by 2 with sign extension (carry = inverted sign)
  cmp #$80  ; to compensate for taking velocity over 2 frames
  eor #$80
  ldx #<velocity_now
  jsr calc_derivative
  
  ; Acceleration = (x[t] + x[t - 2]) / 2 - x[t - 1]
  lda indicator_x
  clc
  adc indicator_xtm2
  ror a
  sec
  sbc indicator_xtm1
  ldx #<accel_now
  jsr calc_derivative
  
  ldx #0
  stx oam_used
  jsr draw_derivative_sprites
  ldx oam_used
  jsr ppu_clear_oam

  lda nmis
:
  cmp nmis
  beq :-
  
  ; TO DO:
  lda #$21
  sta PPUADDR
  lda #$16
  sta PPUADDR
  lda displacement_bcd+1
  ldy displacement_bcd
  jsr write_bcd_ay
  lda #$21
  ldy #$56
  ldx #<velocity_now
  jsr print_derivative
  lda #$21
  ldy #$96
  ldx #<accel_now
  jsr print_derivative

  ldx #0
  lda #>OAM
  stx OAMADDR
  sta OAM_DMA
  ldy #0
  lda #VBLANK_NMI|BG_0000|OBJ_1000
  sec
  jsr ppu_screen_on
  lda new_keys
  and #KEY_SELECT
  beq loop
  
  rts
.endproc

.proc draw_derivative_sprites
  ldy #27
  tya
  sec
  adc oam_used
  sta oam_used
  tax
tmplloop:
  dex
  lda spritedata,y
  sta OAM,x
  dey
  bpl tmplloop
  lda indicator_x
  sta OAM+3,x
  lda velocity_peak
  eor #$80
  sta OAM+7,x
  eor #$FF
  sta OAM+11,x
  lda velocity_now
  eor #$80
  sta OAM+15,x
  lda accel_peak
  eor #$80
  sta OAM+19,x
  eor #$FF
  sta OAM+23,x
  lda accel_now
  eor #$80
  sta OAM+27,x
  rts
.pushseg
.segment "RODATA"
spritedata:
  .byte  71, $05, $00, 0  ; pos ball
  .byte  87, $04, $40, 0  ; vel right bound
  .byte  87, $04, $00, 0  ; vel left bound
  .byte  87, $05, $00, 0  ; vel ball
  .byte 103, $04, $40, 0  ; accel right bound
  .byte 103, $04, $00, 0  ; accel left bound
  .byte 103, $05, $00, 0  ; accel ball
.popseg
.endproc

;;
; @param A:Y nametable address
; @param X zero page address of derivative block
.proc print_derivative
val_bcd = 3
val_bcdhigh = 4
val_sign = 5
peak_bcd = 6
peak_bcdhigh = 7

  sta PPUADDR
  sty PPUADDR
  lda val_sign,x
  sta PPUDATA
  ldy val_bcd,x
  lda val_bcdhigh,x
  jsr write_bcd_ay
  lda #'/'
  sta PPUDATA
  ldy peak_bcd,x
  lda peak_bcdhigh,x
  ; fall through
.endproc
.proc write_bcd_ay
  pha
  lsr a
  lsr a
  lsr a
  lsr a
  ora #'0'
  sta PPUDATA
  pla
  and #$0F
  ora #'0'
  sta PPUDATA
  tya
  ora #'0'
  sta PPUDATA
  rts
.endproc

.proc calc_derivative
val_now = 0
val_peak = 1
val_holdtime = 2
val_bcd = 3
val_bcdhigh = 4
val_sign = 5
peak_bcd = 6
peak_bcdhigh = 7

  sta val_now,x
  ldy #'+'
  bcs notneg
  ldy #'-'
  eor #$FF
  adc #1
notneg:
  sty val_sign,x
  
  ; increase peak marker if needed
  cmp val_peak,x
  bcc notpeak
  sta val_peak,x
  ldy #30
  sty val_holdtime,x
notpeak:

  ; convert current value to decimal
  jsr bcd8bit
  sta val_bcd,x
  lda 0
  sta val_bcdhigh,x

  ; fade peak marker
  lda val_peak,x
  beq notfade
  lda val_holdtime,x
  beq dofade
  dec val_holdtime,x
  bpl notfade
dofade:  
  dec val_peak,x
notfade:

  ; convert peak value to decimal
  lda val_peak,x
  jsr bcd8bit
  sta peak_bcd,x
  lda 0
  sta peak_bcdhigh,x
  rts
.endproc

.segment "RODATA"
pollrate_msg:
  .byte "POLL RATE TEST",LF,LF
  .byte "POLL PERIOD",LF
  .byte "(CHANGE WITH CONTROL PAD)",LF
  .byte "DISPLACEMENT",LF,LF
  .byte "SELECT: NEXT",0

derivative_msg:
  .byte "DERIVATIVES TEST",LF,LF
  .byte "DISPLACEMENT",LF,LF
  .byte "VELOCITY",LF,LF
  .byte "ACCELERATION",LF,LF
  .byte "SELECT: NEXT",0


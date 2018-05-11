; Verifies time VBL flag is set.
;
; Reads $2002 twice and prints VBL flags from
; them. Test is run one PPU clock later each time,
; around the time the flag is set.
;
; T+ 1 2
; 00 - V
; 01 - V
; 02 - V
; 03 - V   ; after some resets this is - -
; 04 - -   ; flag setting is suppressed
; 05 V -
; 06 V -
; 07 V -
; 08 V -

.include "shell.inc"
.include "sync_vbl.s"
	
main:   jsr console_hide
	print_str "T+ 1 2",newline
	loop_n_times test,9
	check_crc $1C3E0067
	jmp tests_passed        
	
test:   jsr print_a
	jsr disable_rendering
	jsr sync_vbl_delay
	delay 29763
	ldx $2002
	ldy $2002
	txa
	print_cc bmi,'V','-'
	jsr print_space
	tya
	print_cc bmi,'V','-'
	jsr print_newline
	rts

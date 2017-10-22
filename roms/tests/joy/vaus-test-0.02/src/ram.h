.global OAM

LF = 10

; main.s
.globalzp control_type, new_keys, indicator_x, nmis, oam_used
.global puts_multiline

; control_type values
CONTROL_STANDARD = 0
CONTROL_4P_FC = 1
CONTROL_ARKANOID_FC = 2   ; fire: cur_keys_d1+0; pot: cur_keys_d1+1
CONTROL_ARKANOID_NES = 3  ; fire: cur_keys_d3+1; pot: cur_keys_d4+1

; ppuclear.s
.global ppu_clear_nt, ppu_clear_oam, ppu_screen_on

; vauspads.s
.global read_all_pads
.globalzp cur_keys_d0, cur_keys_d1, cur_keys_d3, cur_keys_d4

; detailed.s
.global run_timing_test, run_derivative_test

; bcd.s
.global bcd8bit

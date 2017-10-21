# 6502 Notes

In no particular order, here are some things I learned about the 6502

- `rel` addressing mode instructions always follow the cycles rule:
  - `+1 if branch succeeds +2 if to a new page`
- From Wikipedia:
  - > When executing JSR (jump to subroutine) and RTS (return from subroutine) instructions, the return address pushed to the stack by JSR is that of the last byte of the JSR operand (that is, the most significant byte of the subroutine address), rather than the address of the following instruction. This is because the actual copy (from program counter to stack and then vice versa) takes place before the automatic increment of the program counter that occurs at the end of every instruction.[68] This characteristic would go unnoticed unless the code examined the return address in order to retrieve parameters in the code stream (a 6502 programming idiom documented in the ProDOS 8 Technical Reference Manual).
- The 6502 always reads 1 argument byte, regardless of the addressing mode.

// DisAssembler.h


   //
   // The next section implements a basic disassembler for the AVR Instruction Set which is available in debugWire mode
   // using thecommand  "Lxxxx", where xxxx is a 16 bit hexadecimal address.
   // Note: this dissaambler was written quickly and crudely so there may be errors, or omissions in its output
   //

#ifndef __DISASSEMBLER_H__
#define __DISASSEMBLER_H__

#include <Arduino.h>
#include "PrintFunctions.h"
#include "Helpers.h"
#include "MemoryHandler.h"

#define XX  1
#define YY  2
#define ZZ  3

class DisAssembler {
  public:
    uint8_t    srDst, srSrc;
    boolean showStatus, skipWord;
    boolean showSrc, showDst, srcPair, dstPair;
    uint8_t    dstReg, srcReg;
    
    // Constructor
    DisAssembler ();
    
    void dAsm (uint16_t addr, uint8_t count); // used in Debugger only
      /*
       * Specials case instructions: implement?
       *    ELPM  95D8
       *    LPM   95C8
       *    
       * Special case, implied operand display?
       *    SPM   Z+,r1:r0
       */
    // Disassemble 2 byte Instructions
    void dAsm2Byte (uint16_t addr, uint16_t opcode); // used in DisAssembler and Debugger only

  private:
    // old print functions
    
    void printStatus ();                              // used in DisAssembler only
    void printSpecialReg (uint8_t sr);                   // used in DisAssembler only
    void printRegsUsed ();                            // used in DisAssembler only
    void printInst (const __FlashStringHelper* str);  // used in DisAssembler only
    void printDstReg (uint8_t reg);                      // used in DisAssembler only
    void printSrcReg (uint8_t reg);                      // used in DisAssembler only
    void printDstPair (uint8_t reg);                     // used in DisAssembler only
    void printSrcPair (uint8_t reg);                     // used in DisAssembler only
    boolean dAsmNoArgs (uint16_t opcode);         // used in DisAssembler only
    boolean dAsmSetClr (uint16_t opcode);         // used in DisAssembler only
    boolean dAsmLogic (uint16_t opcode);          // used in DisAssembler only
    uint8_t dAsmXYZStore (uint16_t opcode);          // used in DisAssembler only
    boolean dAsmXYZLoad (uint16_t opcode);        // used in DisAssembler only
    boolean dAsmLddYZQ (uint16_t opcode);         // used in DisAssembler only
    boolean dAsmStdYZQ (uint16_t opcode);         // used in DisAssembler only
    boolean dAsmRelCallJmp (uint16_t opcode);     // used in DisAssembler only
    boolean dAsmBitOps2  (uint16_t opcode);       // used in DisAssembler only
    boolean dAsmByteImd (uint16_t opcode);        // used in DisAssembler only
    boolean dAsmArith (uint16_t opcode);          // used in DisAssembler only
    boolean dAsmWordImd (uint16_t opcode);        // used in DisAssembler only
    boolean dAsmBitOps (uint16_t opcode);         // used in DisAssembler only
    boolean dAsmBranch (uint16_t opcode);         // used in DisAssembler only
    boolean dAsmMul (uint16_t opcode);            // used in DisAssembler only
};

#endif

/*
  AVR opcodes (taken from a post to avrfreaks by Jeremy Brandon)
  
  Most are single 16 bit words; four marked * have a second word
  to define an address or address extension (kkkk kkkk kkkk kkkk)
  
    d  bits that specify an Rd (0..31 or 16..31 or 16..23 or 24..30)
    r  bits that specify an Rr ( - ditto - )
    k  bits that specify a constant or an address
    q  bits that specify an offset
    -  bit that specifies pre-decrement mode: 0=no, 1=yes
    +  bit that specifies post-decrement mode: 0=no, 1=yes
    x  bit of any value
    s  bits that specify a status bit (0..7)
    A  bits that specify i/o memory
    b  bits that define a bit (0..7)
  
    0000 0000 0000 0000  nop
    0000 0001 dddd rrrr  movw
    0000 0010 dddd rrrr  muls
    0000 0000 0ddd 0rrr  mulsu
    0000 0000 0ddd 1rrr  fmul
    0000 0000 1ddd 0rrr  fmuls
    0000 0000 1ddd 1rrr  fmulsu
    0000 01rd dddd rrrr  cpc
    0000 10rd dddd rrrr  sbc
    0000 11rd dddd rrrr  add
    0001 00rd dddd rrrr  cpse
    0001 01rd dddd rrrr  cp
    0001 10rd dddd rrrr  sub
    0001 11rd dddd rrrr  adc
    0001 00rd dddd rrrr  and
    0001 01rd dddd rrrr  eor
    0001 10rd dddd rrrr  or
    0001 11rd dddd rrrr  mov
    0011 kkkk dddd kkkk  cpi
    0100 kkkk dddd kkkk  sbci
    0101 kkkk dddd kkkk  subi
    0110 kkkk dddd kkkk  ori
    0111 kkkk dddd kkkk  andi
    1000 000d dddd 0000  ld Z
    1000 000d dddd 1000  ld Y
    10q0 qq0d dddd 0qqq  ldd Z
    10q0 qq0d dddd 1qqq  ldd Y
    10q0 qq1d dddd 0qqq  std Z
    10q0 qq1d dddd 1qqq  std Y
    1001 000d dddd 0000  lds *
    1001 000d dddd 00-+  ld –Z+
    1001 000d dddd 010+  lpm Z
    1001 000d dddd 011+  elpm Z
    1001 000d dddd 10-+  ld –Y+
    1001 000d dddd 11-+  ld X
    1001 000d dddd 1111  pop
    1001 001r rrrr 0000  sts *
    1001 001r rrrr 00-+  st –Z+
    1001 001r rrrr 01xx  ???
    1001 001r rrrr 10-+  st –Y+
    1001 001r rrrr 11-+  st X
    1001 001d dddd 1111  push
    1001 010d dddd 0000  com
    1001 010d dddd 0001  neg
    1001 010d dddd 0010  swap
    1001 010d dddd 0011  inc
    1001 010d dddd 0100  ???
    1001 010d dddd 0101  asr
    1001 010d dddd 0110  lsr
    1001 010d dddd 0111  ror
    1001 010d dddd 1010  dec
    1001 0100 0sss 1000  bset
    1001 0100 1sss 1000  bclr
    1001 0100 0000 1001  ijmp
    1001 0100 0001 1001  eijmp
    1001 0101 0000 1000  ret
    1001 0101 0000 1001  icall
    1001 0101 0001 1000  reti
    1001 0101 0001 1001  eicall
    1001 0101 1000 1000  sleep
    1001 0101 1001 1000  break
    1001 0101 1010 1000  wdr
    1001 0101 1100 1000  lpm
    1001 0101 1101 1000  elpm
    1001 0101 1110 1000  spm
    1001 0101 1111 1000  espm
    1001 010k kkkk 110k  jmp *
    1001 010k kkkk 111k  call *
    1001 0110 kkdd kkkk  adiw
    1001 0111 kkdd kkkk  sbiw
    1001 1000 aaaa abbb  cbi
    1001 1001 aaaa abbb  sbic
    1001 1010 aaaa abbb  sbi
    1001 1011 aaaa abbb  sbis
    1001 11rd dddd rrrr  mul
    1011 0aad dddd aaaa  in
    1011 1aar rrrr aaaa  out
    1100 kkkk kkkk kkkk  rjmp
    1101 kkkk kkkk kkkk  rcall
    1110 kkkk dddd kkkk  ldi
    1111 00kk kkkk ksss  brbs
    1111 01kk kkkk ksss  brbc
    1111 100d dddd 0bbb  bld
    1111 101d dddd 0bbb  bst
    1111 110r rrrr 0bbb  sbrc
    1111 111r rrrr 0bbb  sbrs
*/

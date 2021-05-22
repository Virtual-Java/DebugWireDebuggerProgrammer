// DisAssembler.cpp

   //
   // The next section implements a basic disassembler for the AVR Instruction Set which is available in debugWire mode
   // using thecommand  "Lxxxx", where xxxx is a 16 bit hexadecimal address.
   // Note: this dissaambler was written quickly and crudely so there may be errors, or omissions in its output
   //

#include "DisAssembler.h"

//uint8_t    srDst, srSrc;
//boolean showStatus, skipWord;
//boolean showSrc, showDst, srcPair, dstPair;
//uint8_t    dstReg, srcReg;

extern PrintFunctions  pf;
extern Helpers  hlprs;
MemoryHandler  memo;


// ========================================
// public methods:
// ----------------------------------------

DisAssembler::DisAssembler () {
  
}

// used in Debugger only
void DisAssembler::dAsm (uint16_t addr, uint8_t count) {
  for (uint8_t ii = 0; ii < count; ii++) {
    pf.cursor = 0;
    showStatus = skipWord = false;
    showDst = showSrc = false;
    srcPair = dstPair = false;
    uint8_t idx = ii * 2;
    uint16_t word2;
    // 16 Bit Opcode is LSB:MSB Order
    uint16_t opcode = hlprs.getFlashWord(addr + idx);
    pf.printHex16(addr + idx);
    hlprs.printCmd();
    pf.printHex16(opcode);
    pf.tabTo(14);
    if ((opcode & ~0x1F0) == 0x9000) {                        // lds (4 byte instruction)
      printInst(F("lds"));
      printDstReg((opcode & 0x1F0) >> 4);
      pf.print(F(",0x"));
      word2 = hlprs.getFlashWord(addr + idx + 2);
      pf.printHex16(word2);
      skipWord = true;
    } else if ((opcode & ~0x1F0) == 0x9200) {                 // sts (4 byte instruction)
      printInst(F("sts"));
      pf.print(F("0x"));
      word2 = hlprs.getFlashWord(addr + idx + 2);
      pf.printHex16(word2);
      printSrcReg((opcode & 0x1F0) >> 4);
      skipWord = true;
    } else if ((opcode & 0x0FE0E) == 0x940C) {                // jmp (4 byte instruction)
      printInst(F("jmp"));
      pf.print(F("0x"));
      word2 = hlprs.getFlashWord(addr + idx + 2);
      // 22 bit address
      long add22 = (opcode & 0x1F0) << 13;
      add22 += (opcode & 1) << 16;
      add22 += word2;
      pf.printHex24(add22 * 2);
      skipWord = true;
    } else if ((opcode & 0x0FE0E) == 0x940E) {                // call (4 byte instruction)
      printInst(F("call"));
      pf.print(F("0x"));
      word2 = hlprs.getFlashWord(addr + idx + 2);
      // 22 bit address
      long add22 = (opcode & 0x1F0) << 13;
      add22 += (opcode & 1) << 16;
      add22 += word2;
      pf.printHex24(add22 * 2);
      skipWord = true;
    } else {
     dAsm2Byte(addr + idx, opcode);
    }
   // If single stepping, show register and status information
    if (count == 1) {
      printRegsUsed();
      if (showStatus) {
        printStatus();
      }
    }
    if (skipWord) {
      // Print 2nd line to show extra word used by 2 word instructions
      pf.println();
      pf.printHex16(addr + idx + 2);
      pf.write(':');
      pf.tabTo(8);
      pf.printHex16(word2);
      ii++;
    }
    pf.println();
  }
}

  /*
   * Specials case instructions: implement?
   *    ELPM  95D8
   *    LPM   95C8
   *    
   * Special case, implied operand display?
   *    SPM   Z+,r1:r0
   */

// used in DisAssembler and Debugger only
// Disassemble 2 byte Instructions
void DisAssembler::dAsm2Byte (uint16_t addr, uint16_t opcode) {
  srDst = srSrc = 0;
  if (dAsmNoArgs(opcode)) {                                         // clc, clh, etc.
  } else if (dAsmLogic(opcode)) {                                   // 
    printDstReg((opcode & 0x1F0) >> 4);
  } else if ((srDst = dAsmXYZStore(opcode)) != 0) {                 //
    printSrcReg((opcode & 0x1F0) >> 4);
  } else if (dAsmBranch(opcode)) {                                  // Branch instruction
    int16_t delta = ((opcode & 0x200 ? (int) (opcode | 0xFC00) >> 3 : (opcode & 0x3F8) >> 3) + 1) * 2;
    pf.printHex16(addr + delta);
    showStatus = true;
  } else if (dAsmArith(opcode)) {
    printDstReg((opcode & 0x1F0) >> 4);
    printSrcReg(((opcode & 0x200) >> 5) + (opcode & 0x0F));
  } else if (dAsmBitOps(opcode)) {                                  // 
    printDstReg((opcode & 0x1F0) >> 4);
    pf.print(",");
    pf.printDec(opcode & 0x07);
  } else if (dAsmLddYZQ(opcode)) {                                  // ldd rn,Y+q, ldd rn,Z+q
    // Handled in function
  } else if (dAsmStdYZQ(opcode)) {                                  // std Y+q,rn, std Z+q,rn
    // Handled in function
  } else if (dAsmXYZLoad(opcode)) {                                 // 
    // Handled in function
  } else if (dAsmRelCallJmp(opcode)) {
    int16_t delta = ((opcode & 0x800 ? (int) (opcode | 0xF000) : (int) (opcode & 0xFFF)) + 1) * 2;
    pf.printHex16(addr + delta);
  } else if ((opcode & ~0x7FF) == 0xB000) {                          // in rn,0xnn
    printInst(F("in"));
    printDstReg((opcode & 0x1F0) >> 4);
    pf.print(F(",0x"));
    pf.printHex8(((opcode & 0x600) >> 5) + (opcode & 0x0F));
  } else if ((opcode & ~0x7FF) == 0xB800) {                           // out 0xnn,rn
    printInst(F("out"));
    pf.print(F("0x"));
    pf.printHex8(((opcode & 0x600) >> 5) + (opcode & 0x0F));
    printSrcReg((opcode & 0x1F0) >> 4);
  } else if (dAsmByteImd(opcode)) {                                   // cpi, sbci, subi, ori, andi or ldi
    printDstReg(((opcode & 0xF0) >> 4) + 16);
    pf.print(F(",0x"));
    pf.printHex8(((opcode & 0xF00) >> 4) + (opcode & 0x0F));
  } else if ((opcode & 0x7FF) == 0xA000) {                            // lds
    printInst(F("lds"));
    printDstReg(((opcode & 0xF0) >> 4) + 16);
    pf.print(F(",0x"));
    pf.printHex8(((opcode & 0x700) >> 4) + (opcode & 0x0F) + 0x40);
  } else if ((opcode & 0x7FF) == 0xA800) {                            // sts
    printInst(F("sts"));
    pf.print(F("0x"));
    pf.printHex8(((opcode & 0x700) >> 4) + (opcode & 0x0F) + 0x40);
    printSrcReg(((opcode & 0xF0) >> 4) + 16);
  } else if (dAsmSetClr(opcode)) {                                    // bclr or bset
    pf.print(" ");
    pf.printDec((opcode & 0x70) >> 4);
  } else if (dAsmBitOps2(opcode)) {                                   // cbi, sbi, sbic or sbis
    pf.print(F("0x"));
    pf.printHex8((opcode & 0xF8) >> 3);
    pf.print(",");
    pf.printDec( opcode & 0x07);
  } else if (dAsmWordImd(opcode)) {                                   // adiw or sbiw
    printDstPair(((opcode & 0x30) >> 4) * 2 + 24);
    pf.print(F(",0x"));
    pf.printHex8(((opcode & 0xC0) >> 4) + (opcode & 0x0F));
  } else if (dAsmMul(opcode)) {                                       // mulsu, fmul, fmuls or fmulsu
    printDstReg(((opcode & 7) >> 4) + 16);
    printSrcReg((opcode & 0x07) + 16);
  } else if ((opcode & ~0xFF) == 0x0100) {                            // movw r17:16,r1:r0
    printInst(F("movw"));
    printDstPair(((opcode & 0xF0) >> 4) * 2);
    pf.print(",");
    printSrcPair((opcode & 0x0F) * 2);
  } else if ((opcode & ~0xFF) == 0x0200) {                            // muls r21,r20
    printInst(F("muls"));
    printDstReg(((opcode & 0xF0) >> 4) + 16);
    printSrcReg((opcode & 0x0F) + 16);
  }
}


// ========================================
// private methods:
// ----------------------------------------

// old print functions

// used in DisAssembler only
void DisAssembler::printStatus () {
  pf.tabTo(36);
  pf.print(F("; "));
  uint8_t stat = memo.readSRamByte(0x3F + 0x20);
  pf.write((stat & 0x80) != 0 ? 'I' : '-');
  pf.write((stat & 0x40) != 0 ? 'T' : '-');
  pf.write((stat & 0x20) != 0 ? 'H' : '-');
  pf.write((stat & 0x10) != 0 ? 'S' : '-');
  pf.write((stat & 0x08) != 0 ? 'V' : '-');
  pf.write((stat & 0x04) != 0 ? 'N' : '-');
  pf.write((stat & 0x02) != 0 ? 'Z' : '-');
  pf.write((stat & 0x01) != 0 ? 'C' : '-');
}
// used in DisAssembler only
void DisAssembler::printSpecialReg (uint8_t sr) {
  switch (sr) {
    case XX:
      pf.printHex8(memo.readRegister(27));
      pf.printHex8(memo.readRegister(26));
      break;
    case YY:
      pf.printHex8(memo.readRegister(29));
      pf.printHex8(memo.readRegister(28));
      break;
    case ZZ:
      pf.printHex8(memo.readRegister(31));
      pf.printHex8(memo.readRegister(30));
      break;
  }
}
// used in DisAssembler only
void DisAssembler::printRegsUsed () {
  if (showSrc || showDst || srSrc != 0 || srDst != 0) {
    pf.tabTo(36);
    pf.print(F("; "));
    if (srDst != 0) {
      printSpecialReg(srDst);
    } else if (showDst) {
      if (dstPair) {
        pf.printHex8(memo.readRegister(dstReg + 1));
        pf.write(':');
        pf.printHex8(memo.readRegister(dstReg));
      } else {
        pf.printHex8(memo.readRegister(dstReg));
      }
    }
    if (showSrc || srSrc != 0) {
      if (showDst || srDst != 0) {
        pf.print(F(", "));
      }
      if (showSrc) {
        if (srcPair) {
          pf.printHex8(memo.readRegister(srcReg + 1));
          pf.write(':');
          pf.printHex8(memo.readRegister(srcReg));
        } else {
          pf.printHex8(memo.readRegister(srcReg));
        }
      } else {
        printSpecialReg(srSrc);
      }
    }
  }
}
// used in DisAssembler only
void DisAssembler::printInst (const __FlashStringHelper* str) {
  pf.print(str);
  pf.tabTo(20);
}
// used in DisAssembler only
void DisAssembler::printDstReg (uint8_t reg) {
  pf.print(F("r"));
  pf.printDec(dstReg = reg);
  showDst = true;
}
// used in DisAssembler only
void DisAssembler::printSrcReg (uint8_t reg) {
  pf.print(F(",r"));
  pf.printDec(srcReg = reg);
  showSrc = true;
}
// used in DisAssembler only
void DisAssembler::printDstPair (uint8_t reg) {
  pf.print(F("r"));
  pf.printDec(reg + 1);
  pf.print(F(":r"));
  pf.printDec(dstReg = reg);
  showDst = true;
  dstPair = true;
}
// used in DisAssembler only
void DisAssembler::printSrcPair (uint8_t reg) {
  pf.print(F("r"));
  pf.printDec(reg + 1);
  pf.print(F(":r"));
  pf.printDec(srcReg = reg);
  showSrc = true;
  srcPair = true;
}
// used in DisAssembler only
boolean DisAssembler::dAsmNoArgs (uint16_t opcode) {
  switch (opcode) {
    case 0x9598: pf.print(F("break"));   return true;
    case 0x9488: pf.print(F("clc"));     return true;
    case 0x94d8: pf.print(F("clh"));     return true;
    case 0x94f8: pf.print(F("cli"));     return true;
    case 0x94a8: pf.print(F("cln"));     return true;
    case 0x94c8: pf.print(F("cls"));     return true;
    case 0x94e8: pf.print(F("clt"));     return true;
    case 0x94b8: pf.print(F("clv"));     return true;
    case 0x9498: pf.print(F("clz"));     return true;
    case 0x9519: pf.print(F("eicall"));  return true;
    case 0x9419: pf.print(F("eijmp"));   return true;
    case 0x95d8: pf.print(F("elpm"));    return true;
    case 0x9509: pf.print(F("icall"));   return true;
    case 0x9409: pf.print(F("ijmp"));    return true;
    case 0x95c8: pf.print(F("lpm"));     return true;
    case 0x0000: pf.print(F("nop"));     return true;
    case 0x9508: pf.print(F("ret"));     return true;
    case 0x9518: pf.print(F("reti"));    return true;
    case 0x9408: pf.print(F("sec"));     return true;
    case 0x9458: pf.print(F("seh"));     return true;
    case 0x9478: pf.print(F("sei"));     return true;
    case 0x9428: pf.print(F("sen"));     return true;
    case 0x9448: pf.print(F("ses"));     return true;
    case 0x9468: pf.print(F("set"));     return true;
    case 0x9438: pf.print(F("sev"));     return true;
    case 0x9418: pf.print(F("sez"));     return true;
    case 0x9588: pf.print(F("sleep"));    return true;
    case 0x95e8: pf.print(F("spm"));     return true;
    case 0x95f8: pf.print(F("spm"));     return true;
    case 0x95a8: pf.print(F("wdr"));     return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmSetClr (uint16_t opcode) {
  switch (opcode & ~0x0070) {
    case 0x9488: printInst(F("bclr")); return true;
    case 0x9408: printInst(F("bset")); return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmLogic (uint16_t opcode) {
  switch (opcode & ~0x01F0) {
    case 0x900f: printInst(F("pop"));   return true;
    case 0x920f: printInst(F("push"));  return true;
    case 0x9400: printInst(F("com"));   return true;
    case 0x9401: printInst(F("neg"));   return true;
    case 0x9402: printInst(F("swap"));  return true;
    case 0x9403: printInst(F("inc"));   return true;
    case 0x9405: printInst(F("asr"));   return true;
    case 0x9406: printInst(F("lsr"));   return true;
    case 0x9407: printInst(F("ror"));   return true;
    case 0x940a: printInst(F("dec"));   return true;
  }
  return false;
}
// used in DisAssembler only
uint8_t DisAssembler::dAsmXYZStore (uint16_t opcode) {
  switch (opcode & ~0x01F0) {
    case 0x9204: printInst(F("xch")); pf.print(F("Z"));  return ZZ;
    case 0x9205: printInst(F("las")); pf.print(F("Z"));  return ZZ;
    case 0x9206: printInst(F("lac")); pf.print(F("Z"));  return ZZ;
    case 0x9207: printInst(F("lat")); pf.print(F("Z"));  return ZZ;
    case 0x920c: printInst(F("st"));  pf.print(F("X"));  return XX;
    case 0x920d: printInst(F("st"));  pf.print(F("X+")); return XX;
    case 0x920e: printInst(F("st"));  pf.print(F("-X")); return XX;
    case 0x8208: printInst(F("st"));  pf.print(F("Y"));  return YY;
    case 0x9209: printInst(F("st"));  pf.print(F("Y+")); return YY;
    case 0x920a: printInst(F("st"));  pf.print(F("-Y")); return YY;
    case 0x8200: printInst(F("st"));  pf.print(F("Z"));  return ZZ;
    case 0x9201: printInst(F("st"));  pf.print(F("Z+")); return ZZ;
    case 0x9202: printInst(F("st"));  pf.print(F("-Z")); return ZZ;
  }
  return 0;
}
// used in DisAssembler only
boolean DisAssembler::dAsmXYZLoad (uint16_t opcode) {
  char src[4];
  switch (opcode & ~0x1F0) {
    case 0x9004: printInst(F("lpm"));  strcpy_P(src, (char*) F("Z"));  srSrc = ZZ; break;
    case 0x9005: printInst(F("lpm"));  strcpy_P(src, (char*) F("Z+")); srSrc = ZZ; break;
    case 0x9006: printInst(F("elpm")); strcpy_P(src, (char*) F("Z"));  srSrc = ZZ; break;
    case 0x9007: printInst(F("elpm")); strcpy_P(src, (char*) F("Z+")); srSrc = ZZ; break;
    case 0x900c: printInst(F("ld"));   strcpy_P(src, (char*) F("X"));  srSrc = XX; break;
    case 0x900d: printInst(F("ld"));   strcpy_P(src, (char*) F("X+")); srSrc = XX; break;
    case 0x900e: printInst(F("ld"));   strcpy_P(src, (char*) F("-X")); srSrc = XX; break;
    case 0x8008: printInst(F("ld"));   strcpy_P(src, (char*) F("Y"));  srSrc = YY; break;
    case 0x9009: printInst(F("ld"));   strcpy_P(src, (char*) F("Y+")); srSrc = YY; break;
    case 0x900a: printInst(F("ld"));   strcpy_P(src, (char*) F("-Y")); srSrc = YY; break;
    case 0x8000: printInst(F("ld"));   strcpy_P(src, (char*) F("Z"));  srSrc = ZZ; break;
    case 0x9001: printInst(F("ld"));   strcpy_P(src, (char*) F("Z+")); srSrc = ZZ; break;
    case 0x9002: printInst(F("ld"));   strcpy_P(src, (char*) F("-Z")); srSrc = ZZ; break;;
    default: return false;
  }
  printDstReg((opcode & 0x1F0) >> 4);
  pf.print(",");
  pf.print(src);
  return true;
}
// used in DisAssembler only
boolean DisAssembler::dAsmLddYZQ (uint16_t opcode) {
  char src[4];
  switch (opcode & ~0x2DF7) {
    case 0x8008: printInst(F("ldd"));  strcpy_P(src, (char*) F("Y+"));  srSrc = YY; break;
    case 0x8000: printInst(F("ldd"));  strcpy_P(src, (char*) F("Z+"));  srSrc = ZZ; break;
    default: return false;
  }
  uint8_t qq = ((opcode & 0x2000) >> 8) + ((opcode & 0x0C00) >> 7) + (opcode & 0x07);
  printDstReg((opcode & 0x1F0) >> 4);
  pf.print(",");
  pf.print(src);
  pf.printDec(qq);
  return true;
}
// used in DisAssembler only
boolean DisAssembler::dAsmStdYZQ (uint16_t opcode) {
  switch (opcode & ~0x2DF7) {
    case 0x8208: printInst(F("std"));  pf.print(F("Y+"));  srSrc = YY; break;
    case 0x8200: printInst(F("std"));  pf.print(F("Z+"));  srSrc = ZZ; break;
    default: return false;
  }
  uint8_t qq = ((opcode & 0x2000) >> 8) + ((opcode & 0x0C00) >> 7) + (opcode & 0x07);
  pf.printDec(qq);
  pf.print(",");
  printDstReg((opcode & 0x1F0) >> 4);
  return true;
}
// used in DisAssembler only
boolean DisAssembler::dAsmRelCallJmp (uint16_t opcode) {
  switch (opcode & ~0xFFF) {
    case 0xd000: printInst(F("rcall")); return true;
    case 0xc000: printInst(F("rjmp"));  return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmBitOps2  (uint16_t opcode) {
  switch (opcode & ~0xFF) {
    case 0x9800: printInst(F("cbi"));   return true;
    case 0x9a00: printInst(F("sbi"));   return true;
    case 0x9900: printInst(F("sbic"));  return true;
    case 0x9b00: printInst(F("sbis"));  return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmByteImd (uint16_t opcode) {
  switch (opcode & ~0xFFF) {
    case 0x3000: printInst(F("cpi"));   return true;
    case 0x4000: printInst(F("sbci"));  return true;
    case 0x5000: printInst(F("subi"));  return true;
    case 0x6000: printInst(F("ori"));   return true;
    case 0x7000: printInst(F("andi"));  return true;
    case 0xE000: printInst(F("ldi"));   return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmArith (uint16_t opcode) {
  switch (opcode & ~0x3FF) {
    case 0x1c00: printInst(F("adc"));   return true;
    case 0x0c00: printInst(F("add"));   return true;
    case 0x2000: printInst(F("and"));   return true;
    case 0x1400: printInst(F("cp "));   return true;
    case 0x0400: printInst(F("cpc"));   return true;
    case 0x1000: printInst(F("cpse"));  return true;
    case 0x2400: printInst(F("eor"));   return true;
    case 0x2c00: printInst(F("mov"));   return true;
    case 0x9c00: printInst(F("mul"));   return true;
    case 0x2800: printInst(F("or"));    return true;
    case 0x0800: printInst(F("sbc"));   return true;
    case 0x1800: printInst(F("sub"));   return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmWordImd (uint16_t opcode) {
  switch (opcode & ~0xFF) {
    case 0x9600: printInst(F("adiw")); return true;
    case 0x9700: printInst(F("sbiw")); return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmBitOps (uint16_t opcode) {
  switch (opcode & ~0x1F7) {
    case 0xf800: printInst(F("bld"));   return true;
    case 0xfa00: printInst(F("bst"));   return true;
    case 0xfc00: printInst(F("sbrc"));  return true;
    case 0xfe00: printInst(F("sbrs"));  return true;
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmBranch (uint16_t opcode) {
  switch (opcode & ~0x3F8) {
    case 0xf000: printInst(F("brcs")); return true;   // 1111 00kk kkkk k000
    case 0xf001: printInst(F("breq")); return true;   // 1111 00kk kkkk k001
    case 0xf002: printInst(F("brmi")); return true;   // 1111 00kk kkkk k010
    case 0xf003: printInst(F("brvs")); return true;   // 1111 00kk kkkk k011
    case 0xf004: printInst(F("brlt")); return true;   // 1111 00kk kkkk k100
    case 0xf005: printInst(F("brhs")); return true;   // 1111 00kk kkkk k101
    case 0xf006: printInst(F("brts")); return true;   // 1111 00kk kkkk k110
    case 0xf007: printInst(F("brie")); return true;   // 1111 00kk kkkk k111
    case 0xf400: printInst(F("brcc")); return true;   // 1111 01kk kkkk k000
    case 0xf401: printInst(F("brne")); return true;   // 1111 01kk kkkk k001
    case 0xf402: printInst(F("brpl")); return true;   // 1111 01kk kkkk k010
    case 0xf403: printInst(F("brvc")); return true;   // 1111 01kk kkkk k011
    case 0xf404: printInst(F("brge")); return true;   // 1111 01kk kkkk k100
    case 0xf405: printInst(F("brhc")); return true;   // 1111 01kk kkkk k101
    case 0xf406: printInst(F("brtc")); return true;   // 1111 01kk kkkk k110
    case 0xf407: printInst(F("brid")); return true;   // 1111 01kk kkkk k111
  }
  return false;
}
// used in DisAssembler only
boolean DisAssembler::dAsmMul (uint16_t opcode) {
  switch (opcode & ~0x077) {
    case 0x0300: printInst(F("mulsu"));   return true;
    case 0x0308: printInst(F("fmul"));    return true;
    case 0x0380: printInst(F("fmuls"));   return true;
    case 0x0388: printInst(F("fmulsu"));  return true;
  }
  return false;
}

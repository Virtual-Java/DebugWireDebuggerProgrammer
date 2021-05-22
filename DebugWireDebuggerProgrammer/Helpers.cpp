// Helpers.cpp

#include "Helpers.h"

OnePinSerial  debugWire(RESET);
PrintFunctions  pf;
extern MemoryHandler  memo;


// ========================================
// public methods:
// ----------------------------------------

// used in MemoryHandler
// Constructor
Helpers::Helpers() {
  
}

// used in Helpers and Debugger only
uint16_t Helpers::convertHex (uint8_t idx) {
  uint16_t val = 0;
  while (HexDigit(buf[idx])) {
    val = (val << 4) + toHex(buf[idx++]);
  }
  return val;
}
// used in Helpers and Debugger only
uint8_t Helpers::after (char cc) {
  uint8_t idx = 0;
  while (buf[idx++] != cc && idx < 32)
    ;
  return idx;
}
// used in Helpers and Debugger only
// Read hex characters in buf[] starting at <off> and convert to bytes in buf[] starting at index 0
uint8_t Helpers::convertToHex (uint8_t off) {
  uint8_t idx = 0;
  char cc;
  while ((cc = buf[off++]) != 0) {
    uint8_t hdx = idx >> 1;
    buf[hdx] = (buf[hdx] << 4) + toHex(cc);
    idx++;
  }
  return (idx + 1) / 2;
}
// used in Helpers and Debugger only
uint8_t Helpers::readDecimal (uint8_t off) {
  uint8_t val = 0;
  char cc;
  while ((cc = buf[off++]) != 0 && cc >= '0' && cc <= '9') {
    val = (val * 10) + (byte) (cc - '0');
  }
  return val;
}
// used in Helpers and Debugger only
// Get '\n'-terminated string and return length
int Helpers::getString () {
  int16_t idx = 0;
  while (true) {
    if (Serial.available()) {
      char cc = toupper(Serial.read());
      if (cc == '\r') {
        // Ignore
      } else if (cc == '\n') {
        buf[idx] = 0;
        return idx;
      } else {
        buf[idx++] = cc;
      }
    }
  }
}
// used in Helpers and Debugger only
void Helpers::printBufToHex8 (int16_t count, boolean trailCrLF) {
  if (count > 16) {
    pf.println();
  }
  for (uint8_t ii = 0; ii < count; ii++) {
    pf.printHex8(buf[ii]);
    if ((ii & 0x0F) == 0x0F) {
      pf.println();
    } else {
      pf.print(" ");
    }
  }
  if (trailCrLF && (count & 0x0F) != 0) {
    pf.println();
  }
}
// used in Helpers and Debugger only
boolean Helpers::bufMatches (const __FlashStringHelper* ref) {
  uint8_t jj = 0;
  for (uint8_t ii = 0; ii < sizeof(buf); ii++) {
    uint8_t cc = pgm_read_byte((char *) ref + jj++);
    char bb = buf[ii];
    if (cc == 0) {
      return bb == 0;
    }
    if ((cc == 'x' || cc == 'X') && HexDigit(bb)) {         // 0-9 or A-F
      if (!HexDigit(buf[ii + 1])) {
        while (pgm_read_byte((char *) ref + jj) == 'X') {
          jj++;
        }
      }
    } else if ((cc == 'd' || cc == 'D') && isDecDigit(bb)) {  // 0-9
      if (!isDecDigit(buf[ii + 1])) {
        while (pgm_read_byte((char *) ref + jj) == 'D') {
          jj++;
        }
      }
    } else if (cc == 'o' && bb >= '0' && bb <= '7') {         // 0-7
      continue;
    } else if (cc == 'b' && (bb == '0' || bb == '1')) {       // 0 or 1
      continue;
    } else if (cc != bb) {
      return false;
    }
  }
  return false;
}
// used in Helpers and Debugger only
boolean Helpers::bufStartsWith (const __FlashStringHelper* ref) {
  for (uint8_t ii = 0; ii < sizeof(buf); ii++) {
    uint8_t cc = pgm_read_byte((char *) ref + ii);
    if (cc == 0) {
      return true;
    } else if (cc != buf[ii]) {
      return false;
    }
  }
  return false;
}
// used in Helpers, Debugger and in MemoryHandler too
void Helpers::sendCmd (byte* data, uint8_t count) {
  debugWire.sendCmd(data, count);
}

// used in Helpers, Debugger and in MemoryHandler too
uint8_t Helpers::getResponse (int16_t expected) {
  return getResponse(&buf[0], expected);
}
// used in Helpers and Debugger only
uint8_t Helpers::getResponse (uint8_t *data, int16_t expected) {
  uint8_t idx = 0;
  uint8_t timeout = 0;
  do {
    if (debugWire.available()) {
      data[idx++] = debugWire.read();
      timeout = 0;
      if (expected > 0 && idx == expected) {
        return expected;
      }
    } else {
      delayMicroseconds(timeOutDelay);
      timeout++;
    }
  } while (timeout < 50 && idx < sizeof(buf));
  if (reportTimeout) {
    pf.print(F("Timeout: received: "));
    pf.printDec(idx);
    pf.print(F(" expected: "));
    pf.printDec(expected);
    pf.println();
  }
  return idx;
}
// used in Helpers and Debugger only
void Helpers::setTimeoutDelay (uint16_t rate) {
  timeOutDelay = F_CPU / rate;
}
// used in Helpers only
uint16_t Helpers::getWordResponse () {
  uint8_t tmp[2];
  getResponse(&tmp[0], 2);
  return ((unsigned int) tmp[0] << 8) + tmp[1];
}
// used in Helpers and Debugger only
boolean Helpers::checkCmdOk () {
  uint8_t tmp[2];
  uint8_t rsp = getResponse(&tmp[0], 1);
  if (rsp == 1 && tmp[0] == 0x55) {
    pf.println("Ok");
    return true;
  } else {
    return false;
  }
}
// used in Helpers and Debugger only
boolean Helpers::checkCmdOk2 () {
  uint8_t tmp[2];
  if (getResponse(&tmp[0], 2) == 2 && tmp[0] == 0x00 && tmp[1] == 0x55) {
    pf.println("Ok");
    return true;
  } else {
    pf.println("Err");
    return false;
  }
}
// used in Helpers and Debugger only
void Helpers::printCommErr (uint8_t read, uint8_t expected) {
  pf.print(F("\ndebugWire Communication Error: read "));
  pf.printDec(read);
  pf.print(F(" expected "));
  pf.printDec(expected);
  pf.println();
}

// used in Debugger and DisAssembler
uint16_t Helpers::getFlashWord (uint16_t addr) {
  if (!flashLoaded || addr < flashStart || addr >= flashStart + sizeof(flashBuf)) {
    uint8_t len = memo.readFlashPage(&flashBuf[0], sizeof(flashBuf), flashStart = addr);
    if (len != sizeof(flashBuf)) {
      printCommErr(len, sizeof(flashBuf));
    }
    flashLoaded = true;
  }
  int16_t idx = addr - flashStart;
  return (flashBuf[idx + 1] << 8) + flashBuf[idx];
}
// used in Debugger only
void Helpers::printCmd (const __FlashStringHelper* cmd) {
  pf.print(cmd);
  pf.write(':');
  pf.tabTo(8);
}

// used in Debugger and DisAssembler
void Helpers::printCmd () {
  pf.write(':');
  pf.tabTo(8);
}
// used in Debugger only
void Helpers::echoCmd () {
  pf.print((char*) buf);
  pf.write(':');
  pf.tabTo(8);
}
// used in Debugger only
void Helpers::echoSetCmd (uint8_t len) {
  for (uint8_t ii = 0; ii < len; ii++) {
    pf.write(buf[ii]);
  }
  pf.print(":=");
  pf.tabTo(8);
}
// used in Debugger only
uint16_t Helpers::getPc () {
  sendCmd((const byte[]) {0xF0}, 1);
  uint16_t pc = getWordResponse();
  return (pc - 1) * 2;
}
// used in Debugger only
uint16_t Helpers::getBp () {
  sendCmd((const byte[]) {0xF1}, 1);
  return (getWordResponse() - 1) * 2;
}
// not used anywhere
uint16_t Helpers::getOpcode () {
  sendCmd((const byte[]) {0xF1}, 1);
  return getWordResponse();
}
// used in Debugger only
void Helpers::setPc (uint16_t pc) {
  pc = pc / 2;
  uint8_t cmd[] = {0xD0, pc >> 8, pc & 0xFF};
  sendCmd(cmd, sizeof(cmd));
}
// used in Debugger only
void Helpers::setBp (uint16_t bp) {
  bp = bp / 2;
  uint8_t cmd[] = {0xD1, bp >> 8, bp & 0xFF};
  sendCmd(cmd, sizeof(cmd));
}
// used in Debugger only
void Helpers::printDebugCommands () {
    pf.print(F(
      "Debugging Commands:\n"
      "  HELP          Print this menu\n"
      "  REGS          Print All Registers 0-31\n"
      "  Rdd           Print Value of Reg dd (dd is a decimal value from 0 - 31)\n"
      "  Rdd=xx        Set Reg dd to New Value xx (dd is a decimal value from 0 - 31)\n"
      "  IOxx          Print Value of I/O space location xx\n"
      "  IOxx=yy       Set I/O space location xx to new value yy\n"
      "  IOxx.d=b      Change bit d (0-7) in I/O location xx to value b (1 or 0)\n"
      "  SRAMxxxx      Read and Print 32 bytes from SRAM address xxxx\n"
      "  SBxxxx        Print Byte Value of SRAM location xxxx\n"
      "  SBxxxx=yy     Set SRAM location xxxx to new byte value yy\n"
      "  SWxxxx        Print Word Value of SRAM location xxxx\n"
      "  SWxxxx=yyyy   Set SRAM location xxxx to new word value yyyy\n"
      "  EBxxxx        Print Byte Value of EEPROM location xxxx\n"
      "  EBxxxx=yy     Set EEPROM location xxxx to new byte value yy\n"
      "  EWxxxx        Print Word Value of EEPROM location xxxx\n"
      "  EWxxxx=yyyy   Set EEPROM location xxxx to new word value yyyy\n"
      "  CMD=xxxx      Send sequence of bytes xxxx... and show response\n"
      "  FWxxxx        Print 32 Word Values (64 bytes) from Flash addr xxxx\n"
      "  FBxxxx        Print 64 Byte Values from Flash addr xxxx and decode ASCII\n"
      "  LISTxxxx      Disassemble 16 words (32 bytes) from Flash addr xxxx\n"
      "  RUN           Start Execution at Current Value of PC (use BREAK to stop)\n"
      "  RUNxxxx       Start Execution at xxxx (use BREAK to stop)\n"
      "  RUNxxxx yyyy  Start Execution at xxxx with a Breakpoint set at yyyy\n"
      "  RUN xxxx      Start Execution at Current Value of PC with breakpoint at xxxx\n"
      "  BREAK         Send Async BREAK to Target (stops execution)\n"
      "  STEP          Single Step One Instruction at Current PC\n"
      "  RESET         Reset Target\n"
      "  EXIT          Exit from debugWire mode back to In-System\n"
      "  PC            Read and Print Program Counter\n"
      "  PC=xxxx       Set Program Counter to xxxx\n"
      "  SIG           Read and Print Device Signature\n"
#if DEVELOPER
      "Developer Commands:\n"
      "  CMD=xxxx      Send Sequence of Bytes xxxx... and show response\n"
      "  BP            Read and Print Breakpoint Register\n"
      "  BP=xxxx       Set Breakpoint Register to xxxxe\n"
      "  EXEC=xxxx     Execute Current Instruction opcode xxxxe\n"
      "  RAMSET        Init first 32 bytes of SRAMe\n"
      "  Dxxxx         Disassemble single word instruction opcode xxxxe\n"
      "  Dxxxx yyyy    Disassemble two word instruction opcode xxxx + yyyye\n"
#endif
  ));
}
// used in Debugger only
void Helpers::setRepeatCmd (const __FlashStringHelper* cmd, uint16_t addr) {
  strcpy_P(rpt, (char*) cmd);
  uint8_t idx = (byte) strlen_P((char*) cmd);
  rpt[idx++] = toHexDigit((addr >> 12) & 0xF);
  rpt[idx++] = toHexDigit((addr >> 8) & 0xF);
  rpt[idx++] = toHexDigit((addr >> 4) & 0xF);
  rpt[idx++] = toHexDigit(addr & 0xF);
  rpt[idx] = 0;
}


// ========================================
// private methods:
// ----------------------------------------

// used in Helpers only
uint8_t Helpers::toHex (char cc) {
  if (cc >= '0' && cc <= '9') {
    return cc - '0';
  } else if (cc >= 'A' && cc <= 'F') {
    return cc - 'A' + 10;
  }
  return 0;
}
// used in Helpers only
char Helpers::toHexDigit (uint8_t nib) {
  if (nib >= 0 & nib < 10) {
    return '0' + nib;
  } else {
    return 'A' + (nib - 10);
  }
}
// used in Helpers only
boolean Helpers::isDecDigit (char cc) {
  return cc >= '0' && cc <= '9';
}
// used in Helpers only
boolean Helpers::HexDigit (char cc) {
  return isDecDigit(cc) || (cc >= 'A' && cc <= 'F');
}

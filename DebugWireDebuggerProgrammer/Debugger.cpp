// Debugger.cpp

#include "Debugger.h"

//OnePinSerial  debugWire(RESET);
// used in DWDP_OOP.ino only

//#define RESET  10

extern PrintFunctions  pf;
extern Helpers  hlprs;
extern MemoryHandler  memo;
InSystemProgrammer  isp;
DisAssembler  dasm;
extern OnePinSerial  debugWire;

// ========================================
// public methods:
// ----------------------------------------

// Constructor
Debugger::Debugger () {
  // OnePinSerial  debugWire(RESET);
  // PrintFunctions  pf;
  // Helpers  hlpr;
}

// used in DWDP_OOP.ino only
void Debugger::selectDebugger() {
  powerOff();
  debugWire.begin(DWIRE_RATE);    
  hlprs.setTimeoutDelay(DWIRE_RATE); 
  debugWire.enable(true);
  Serial.begin(DEBUG_BAUD);
  progMode = false;
  debugWireOn = false;
  hlprs.flashLoaded = false;
  hasDeviceInfo = false;
  breakWatch = false;
  printMenu();
}
//// used in DWDP_OOP.ino only
//void Debugger::selectProgrammer () {
//  isp.pmode = 0;
//  digitalWrite(VCC, HIGH);    // Enable VCC
//  pinMode(VCC, OUTPUT);
//  delay(100);
//  debugWire.enable(false);
//  isp.disableSpiPins();          // Disable SPI pins
//  Serial.begin(PROGRAM_BAUD);
//}
// used in DWDP.ino (loop) only
void Debugger::debugger () {
  if (debugWireOn) {
    // This section implement the debugWire commands after being enabled by the break ('b') command
    if (Serial.available()) {
      if (hlprs.getString() == 0 && hlprs.rpt[0] != 0) {              // Check for repeated command
        strcpy(hlprs.buf, hlprs.rpt);
      }
      hlprs.rpt[0] = 0;
      if (hlprs.bufMatches(F("BREAK"))) {                       // BREAK - Send Async BREAK Target
        hlprs.echoCmd();
        sendBreak();
        if (hlprs.checkCmdOk()) {
          dasm.dAsm(pcSave = hlprs.getPc(), 1);
          hlprs.setBp(0);
          strcpy(hlprs.rpt, "STEP");
        }
        breakWatch = false;
        return;
      } else if (breakWatch) {
        // Code is running, wait for BREAK or breakpoint
        return;
      }
      if (hlprs.bufMatches(F("HELP"))) {                        // HELP - Prints menu of debug commands
        hlprs.printDebugCommands();
      } else if (hlprs.bufMatches(F("PC"))) {                   // PC - Read and Print Internal Value of Program Counter
        hlprs.echoCmd();
        pf.printHex16(pcSave);
        pf.println();
        dasm.dAsm(pcSave, 1);
      } else if (hlprs.bufMatches(F("PC=xXXX"))) {              // PC=xXXX - Set Program Counter to xXXX
        hlprs.echoSetCmd(2);
        hlprs.setPc(pcSave = hlprs.convertHex(3));
        pf.printHex16(pcSave);
        pf.println();
        dasm.dAsm(pcSave, 1);
      } else if (hlprs.bufMatches(F("RESET"))) {                // RESET - Reset Target
        hlprs.echoCmd();
        hlprs.sendCmd((const uint8_t[]) {0x07}, 1);
        if (hlprs.checkCmdOk2()) {
          hlprs.setPc(pcSave = 0);
          // Preset registers 0-31
          for (uint8_t ii = 0; ii < 32; ii++) {
            memo.writeRegister(ii, ii);
          }
          dasm.dAsm(pcSave, 1);
        }
      } else if (hlprs.bufMatches(F("SIG"))) {                  // SIG - Read and Print Device Signature
        hlprs.echoCmd();
        hlprs.sendCmd((const uint8_t[]) {0xF3}, 1);
        hlprs.printBufToHex8(hlprs.getResponse(2), false);
        printPartFromId(hlprs.buf[0], hlprs.buf[1]);
      } else if (hlprs.bufMatches(F("EXIT"))) {                 // EXIT - Exit from debugWire mode back to In-System
        hlprs.echoCmd();
        hlprs.sendCmd((const uint8_t[]) {0x06}, 1);
        debugWireOn = false;
        isp.enableSpiPins();
        pf.println(F("debugWire temporarily Disabled"));
      } else if (hlprs.bufMatches(F("REGS"))) {                 // REGS - Print Registers labelled as 0-31
        if (memo.readAllRegisters()) {
          for (uint8_t ii = 0; ii < 32; ii++) {
            pf.print(ii < 10 ? F(" r") : F("r") );
            pf.printDec(ii);
            pf.write(':');
            pf.printHex8(hlprs.buf[ii]);
            if ((ii & 7) == 7) {
              pf.println();
            } else {
              pf.print(F(", "));
            }
          }
        } else {
          pf.println(F("debugWire Communication Error"));
        }
      } else if (hlprs.bufMatches(F("RdD"))) {                  // RdD - Print Reg dD, where dD is a 1, or 2 digit, decimal value
        hlprs.echoCmd();
        uint8_t reg = hlprs.readDecimal(1);
        if (reg < 32) {
          pf.printHex8(memo.readRegister(reg));
          if (reg < 31) {
            // Set repeat command to show next register
            uint8_t idx = 0;
            reg++;
            hlprs.rpt[idx++] = 'R';
            if (reg >= 10) {
              hlprs.rpt[idx++] = (reg / 10) + '0';
            }
            hlprs.rpt[idx++] = (reg % 10) + '0';
            hlprs.rpt[idx] = 0;
          }
        } else {
          pf.print(F("Invalid register"));
        }
        pf.println();
      } else if (hlprs.bufMatches(F("RdD=xX"))) {               // RdD=xX - Set Reg dD to new value, where dD is a 1, or 2 digit, decimal value
        hlprs.echoSetCmd(hlprs.after('=') - 1);
        uint8_t reg = hlprs.readDecimal(1);
        if (reg < 32) {
          uint8_t val = hlprs.convertHex(hlprs.after('='));
          memo.writeRegister(reg, val);
          pf.printHex8(val);
        } else {
          pf.print(F("Invalid register"));
        }
        pf.println();
      } else if (hlprs.bufMatches(F("IOxX"))) {                 // IOxX - Print I/O space location xX, where xx is address for "in", or "out"
        // Set repeat command for I/O input polling
        strcpy(hlprs.rpt, hlprs.buf);
        hlprs.echoCmd();
        uint16_t addr = hlprs.convertHex(2);
        if (addr < 0x40) {
          pf.printHex8(memo.readSRamByte(addr + 0x20));
        } else {
          pf.print(F("Invalid I/O Address, Range is 0x00 - 0x3F"));        
        }
        pf.println();
      } else if (hlprs.bufMatches(F("IOxX=xX"))) {              // IOxX=xX - Set I/O space location xx to new value
        hlprs.echoSetCmd(hlprs.after('=') - 1);
        uint16_t addr = hlprs.convertHex(2);
        uint8_t val = hlprs.convertHex(hlprs.after('='));
        if (addr < 0x40) {
          memo.writeSRamByte(addr + 0x20, val);
          pf.printHex8(val);
        } else {
          pf.print(F("Invalid I/O Address, Range is 0x00 - 0x3F"));        
        }
        pf.println();
      } else if (hlprs.bufMatches(F("IOxX.o=b"))) {             // IOxX.0=b change bit 'o' in I/O location xx to 'b'
        hlprs.echoSetCmd(hlprs.after('=') - 1);
        uint8_t addr = hlprs.convertHex(2);
        if (addr < 0x20) {
          uint8_t bit = hlprs.readDecimal(hlprs.after('.'));
          char ss = hlprs.buf[hlprs.after('=')];
          if (bit <= 7 && (ss == '1' || ss == '0')) {
            memo.setClrIOBit(addr, bit, ss == '1');
            pf.write(ss);
          } else {
            pf.print(F("Invalid bit # or value"));        
          }
        } else {
          pf.print(F("Invalid I/O Address, Range is 0x00 - 0x1F"));
        }
        pf.println();
      } else if (hlprs.bufMatches(F("SRAMxXXX"))) {             // SRAMxXXX - Read and Print 32 bytes from SRAM address xXXX
        uint16_t addr = hlprs.convertHex(4);
        if (addr >= 0x20) {
          uint8_t len = 32;
          if (memo.readSRamBytes(addr, len)) {
           for (uint8_t ii = 0; ii < len; ii++) {
              if ((ii & 0x0F) == 0) {
                pf.write('S');
                pf.printHex16(addr + ii);
                hlprs.printCmd();
              }
              pf.printHex8(hlprs.buf[ii]);
              if ((ii & 0x0F) == 0x0F) {
                pf.println();
              } else {
                pf.write(' ');
              }
            }
          } else {
            pf.println(F("Read Err"));
          }
        } else {
          pf.write('S');
          pf.printHex16(addr);
          hlprs.printCmd();
          pf.print(F("Invalid Address, Must be >= 0x20"));
          pf.println();
        }
        hlprs.setRepeatCmd(F("SRAM"), addr + 32);
      } else if (hlprs.bufMatches(F("SBxXXX"))) {               // SBxXXX - Print byte value of SRAM location xXXX
        uint16_t addr = hlprs.convertHex(2);
        pf.write('S');
        pf.printHex16(addr);
        hlprs.printCmd();
        if (addr >= 0x20) {
          pf.printHex8(memo.readSRamByte(addr));
        } else {
          pf.print(F("Invalid Address, Must be >= 0x20"));
        }
        pf.println();
        hlprs.setRepeatCmd(F("SB"), addr + 1);
      } else if (hlprs.bufMatches(F("SBxXXX=xX"))) {            // SBxXXX=xX - Set SRAM location xXXX to new byte value xX
        uint16_t addr = hlprs.convertHex(2);
        pf.write('S');
        pf.printHex16(addr);
        hlprs.printCmd(F("="));
        if (addr >= 0x20) {
          uint8_t val = hlprs.convertHex(hlprs.after('='));
          memo.writeSRamByte(addr, val);
          pf.printHex8(val);
        } else {
          pf.print(F("Invalid Address, Must be >= 0x20"));
        }
        pf.println();
      } else if (hlprs.bufMatches(F("SWxXXX"))) {               // SWxXXX - Print word value of SRAM location xXXX
        uint16_t addr = hlprs.convertHex(2);
        pf.write('S');
        pf.printHex16(addr);
        hlprs.printCmd();
        if (addr >= 0x20) {
         // 16 Bit word bytes are in LSB:MSB Order
          pf.printHex8(memo.readSRamByte(addr + 1));
          pf.printHex8(memo.readSRamByte(addr));
        } else {
          pf.print(F("Invalid Address, Must be >= 0x20"));
        }
        pf.println();
        hlprs.setRepeatCmd(F("SW"), addr + 2);
      } else if (hlprs.bufMatches(F("SWxXXX=xXXX"))) {          // SWxXXX=xXXX - Set SRAM location xXXX to new word value xX
        uint16_t addr = hlprs.convertHex(2);
        uint16_t data =  hlprs.convertHex(hlprs.after('='));
        pf.write('S');
        pf.printHex16(addr);
        hlprs.printCmd(F("="));
        if (addr >= 0x20) {
          memo.writeSRamByte(addr, data & 0xFF);
          memo.writeSRamByte(addr + 1, data >> 8);
          pf.printHex16(data);
        } else {
          pf.print(F("Invalid Address, Must be >= 0x20"));
        }
        pf.println();
      } else if (hlprs.bufMatches(F("EBxXXX"))) {               // EBxXXX - Print value of EEPROM location xXXX
        uint16_t addr = hlprs.convertHex(2);
        pf.write('E');
        pf.printHex16(addr);
        hlprs.printCmd();
        if (addr < eeSize) {
          pf.printHex8(memo.readEepromByte(addr));
        } else {
          pf.print(F("Invalid EEPROM Address"));        
        }
        pf.println();     
        hlprs.setRepeatCmd(F("EB"), addr + 1);
      } else if (hlprs.bufMatches(F("EBxXXX=xX"))) {            // EBxXXX=xX - Set EEPROM location xXXX to new value xX
        uint16_t addr = hlprs.convertHex(2);
        pf.write('E');
        pf.printHex16(addr);
        hlprs.printCmd(F("="));
        if (addr < eeSize) {
          uint8_t val = hlprs.convertHex(hlprs.after('='));
          memo.writeEepromByte(addr, val);
          pf.printHex8(val);
        } else {
          pf.print(F("Invalid EEPROM Address"));        
        }
        pf.println();
      } else if (hlprs.bufMatches(F("EWxXXX"))) {               // EWxXXX - Print word value of EEPROM location xXXX
        uint16_t addr = hlprs.convertHex(2);
        pf.write('E');
        pf.printHex16(addr);
        hlprs.printCmd();
        if (addr < eeSize) {
         // 16 Bit word bytes are in LSB:MSB Order
         uint16_t data = ((unsigned int) memo.readEepromByte(addr + 1) << 8) + memo.readEepromByte(addr);
         pf.printHex16(data);
        } else {
          pf.print(F("Invalid EEPROM Address"));        
        }
        pf.println();
        hlprs.setRepeatCmd(F("EW"), addr + 2);
      } else if (hlprs.bufMatches(F("EWxXXX=xXXX"))) {          // EWxXXX=xXXX - Set EEPROM location xXXX to new word value xX
        uint16_t addr = hlprs.convertHex(2);
        uint16_t data =  hlprs.convertHex(hlprs.after('='));
        pf.write('E');
        pf.printHex16(addr);
        hlprs.printCmd(F("="));
        if (addr < eeSize) {
          memo.writeEepromByte(addr, data & 0xFF);
          memo.writeEepromByte(addr + 1, data >> 8);
          pf.printHex16(data);
        } else {
          pf.print(F("Invalid EEPROM Address"));        
        }
        pf.println();
      } else if (hlprs.bufMatches(F("FBxXXX"))) {               // FBxXXX - Print 64 bytes from Flash addr xXXX
        uint16_t addr = hlprs.convertHex(2);
        uint16_t count = memo.readFlashPage(&hlprs.buf[0], sizeof(hlprs.flashBuf), addr); // TODO!!!
        if (count == sizeof(hlprs.flashBuf)) { // TODO!!!
          for (uint8_t ii = 0; ii < count; ii++) {
            if ((ii & 0x0F) == 0) {
              pf.write('F');
              pf.printHex16(addr + ii);
              hlprs.printCmd();
            }
            pf.printHex8(hlprs.buf[ii]);
            if ((ii & 0x0F) == 0x0F) {
              // Print ASCII equivalents for characters <= 0x7F and >= 0x20
              uint8_t idx = ii & 0xF0;
              pf.print(F("    "));
              for (uint8_t jj = 0; jj < 16; jj++) {
                char cc = hlprs.buf[idx + jj];
                pf.write(cc >= 0x20 && cc <= 0x7F ? cc : '.');
              }
              pf.println();
            } else {
              pf.write(' ');
            }
          }
          hlprs.setRepeatCmd(F("FB"), addr + 128);
        } else {
          hlprs.printCommErr(count, sizeof(hlprs.flashBuf)); // TODO!!!
        }
      } else if (hlprs.bufMatches(F("FWxXXX"))) {               // FWxXXX - Print 32 words (64 bytes) from Flash addr xXXX
        uint16_t addr = hlprs.convertHex(2);
        uint16_t count = memo.readFlashPage(&hlprs.buf[0], sizeof(hlprs.flashBuf), addr);  //TODO!!!
        if (count == sizeof(hlprs.flashBuf)) { //TODO!!!
          for (uint8_t ii = 0; ii < count; ii += 2) {
            if ((ii & 0x0E) == 0) {
              pf.write('F');
              pf.printHex16(addr + ii);
              hlprs.printCmd();
            }
            // 16 Bit Opcode is LSB:MSB Order
            pf.printHex8(hlprs.buf[ii + 1]);
            pf.printHex8(hlprs.buf[ii]);
            if ((ii & 0x0E) == 0x0E) {
              pf.println();
            } else {
              pf.write(' ');
            }
          }
          hlprs.setRepeatCmd(F("FW"), addr + 128);
        } else {
          hlprs.printCommErr(count, sizeof(hlprs.flashBuf)); // TODO!!! error sizeof hlprs.flashBuf
        }
      } else if (hlprs.bufMatches(F("LxXXX"))) {                // LxXXX - Disassemble 16 words (32 bytes) from Flash addr xXXX
        uint16_t addr = hlprs.convertHex(1);
        dasm.dAsm(addr, 16);
        hlprs.setRepeatCmd(F("L"), addr + 32);
      } else if (hlprs.bufMatches(F("STEP"))) {                 // STEP - Single Step One Instruction at Current PC
        hlprs.echoCmd();
        uint16_t opcode = hlprs.getFlashWord(pcSave);
        if (opcode == 0x9598) {
          // Skip over "break" instruction
          pf.println(F("Skipping break"));
          dasm.dAsm(pcSave += 2, 1);
        } else {
          if (true) {
            memo.readSRamByte((unsigned int) memo.eecr + 0x2C);     // Reading EECR register seems to fix unexpected EE_RDY interrupt issue
            uint16_t pc = pcSave / 2;
            uint8_t cmd[] = {0xD0, pc >> 8, pc, 0x31};
            hlprs.sendCmd(cmd, sizeof(cmd));
          } else {
            hlprs.setPc(pcSave);
            hlprs.sendCmd((const byte[]) {0x31}, 1);
            //hlprs.sendCmd((const byte[]) {0x60, 0x31}, 2);
          }
          if (hlprs.checkCmdOk2()) {
            dasm.dAsm(pcSave = hlprs.getPc(), 1);
          }
        }
        strcpy(hlprs.rpt, "STEP");
      } else if (hlprs.bufMatches(F("RUNxXXX xXXXX"))) {         // RUNxXXX xXXXX - Start execution at loc xXXX with breakpoint at 2nd xXXX
        hlprs.echoSetCmd(3);
        uint16_t pc = hlprs.convertHex(3);
        uint16_t bp = hlprs.convertHex(hlprs.after(' '));
        pf.print(F("PC:"));
        pf.printHex16(pc);
        pf.print(F(" BP:"));
        pf.printHex16(bp);
        // Set Run To Cursor (BP) Mode (timers enabled)
        pcSave = pc;
        bpSave = bp;
        pc /= 2;
        bp /= 2;
        // Needs 0x41/0x61 or break doesn't happen
        uint8_t cmd[] = {0x61, 0xD1, bp >> 8, bp, 0xD0, pc >> 8, pc, 0x30};
        hlprs.sendCmd(cmd, sizeof(cmd));
        pf.println();
        breakWatch = true;
      } else if (hlprs.bufMatches(F("RUN xXXX"))) {             // RUN xXXX - Start execution at current PC with breakpoint at xXXX
        hlprs.echoSetCmd(3);
        uint16_t bp = hlprs.convertHex(4);
        pf.print(F("PC:"));
        pf.printHex16(pcSave);
        pf.print(F(" BP:"));
        pf.printHex16(bp);
        hlprs.setBp(bpSave = bp);
        // Set Run To Cursor (BP) Mode (timers enabled)
        bpSave = bp;
        bp /= 2;
        uint16_t pc = pcSave / 2;
        // Needs 0x41/0x61 or break doesn't happen
        uint8_t cmd[] = {0x61, 0xD1, bp >> 8, bp, 0xD0, pc >> 8, pc, 0x30};
        hlprs.sendCmd(cmd, sizeof(cmd));
        pf.println();
        breakWatch = true;
      } else if (hlprs.bufMatches(F("RUNxXXX"))) {              // RUNxXXX - Start execution on Target at xXXX
        hlprs.echoSetCmd(3);
        uint16_t pc = hlprs.convertHex(3);
        pf.print(F("PC:"));
        pf.printHex16(pc);
        // Set Run without breakpoint (timers enabled)
        pcSave = pc;
        pc /= 2;
        uint8_t cmd[] = {0x60, 0xD0, pc >> 8, pc, 0x30};
        hlprs.sendCmd(cmd, sizeof(cmd));
        pf.println(F(" Running"));
        breakWatch = true;
      } else if (hlprs.bufMatches(F("RUN"))) {                  // RUN code from current PC
        hlprs.echoCmd();
        uint16_t opcode = hlprs.getFlashWord(pcSave);
        // Skip over "break" instruction, if needed
        pcSave = opcode == 0x9598 ? pcSave + 2 : pcSave;
        // Set Run without breakpoint (timers enabled)
        uint16_t pc = pcSave / 2;
        uint8_t cmd[] = {0x60, 0xD0, pc >> 8, pc, 0x30};
        hlprs.sendCmd(cmd, sizeof(cmd));
        pf.println(F("Running"));
        breakWatch = true;
#if 0
      // Still trying to make this work...
      } else if (hlprs.bufMatches(F("RETURN"))) {               // RETURN - RUN code from current PC and break on return
        hlprs.echoCmd();
        uint16_t opcode = hlprs.getFlashWord(pcSave);
        // Skip over "break" instruction, if needed
        pcSave = opcode == 0x9598 ? pcSave + 2 : pcSave;
        // Set Run without breakpoint (timers enabled)
        uint16_t pc = pcSave / 2;
        uint8_t cmd[] = {0x63, 0xD0, pc >> 8, pc, 0x30};
        hlprs.sendCmd(cmd, sizeof(cmd));
        pf.println(F("Running"));
        breakWatch = true;
#endif
#if DEVELOPER
      } else if (hlprs.bufMatches(F("REGS=xX"))) {              // REGS=xx set all registers to value of xx
        hlprs.echoSetCmd(4);
        uint8_t val = hlprs.convertHex(5);
        pf.printHex8(val);
        for (uint8_t ii = 0; ii < 32; ii++) {
          writeRegister(ii, val);
        }
        pf.println();
      } else if (hlprs.bufMatches(F("BP"))) {                   // BP - Read and Print Breakpoint Register
        hlprs.echoCmd();
        pf.printHex16(hlprs.getBp());
        pf.println();
      } else if (hlprs.bufMatches(F("BP=xXXX"))) {              // BP=xXXX - Set Breakpoint Register to xXXX
        uint16_t bp = hlprs.convertHex(3);
        hlprs.echoSetCmd(2);
        hlprs.setBp(bp);
        pf.printHex16(bp);
        pf.println();
      } else if (hlprs.bufStartsWith(F("CMD="))) {              // CMD=xx - Send sequence of bytes and show response
        hlprs.echoSetCmd(3);
        uint8_t cnt = hlprs.convertToHex(4);
        for (uint8_t ii = 0; ii < cnt; ii++) {
          pf.printHex8(hlprs.buf[ii]);
          pf.print(" ");
        }
        hlprs.sendCmd(&hlprs.buf[0], cnt);
        uint8_t rsp = hlprs.getResponse(0);
        hlprs.printCmd(F("RSP"));
        hlprs.printBufToHex8(rsp, true);
      } else if (hlprs.bufMatches(F("EXEC=xxxx"))) {            // EXEC=xxxx - Execute Instruction opcode xxxx
        hlprs.echoSetCmd(4);
        uint16_t opcode = hlprs.convertHex(5);
        uint8_t cmd[] = {0xD2, opcode >> 8, opcode & 0xFF};
        hlprs.sendCmd(cmd, sizeof(cmd));
        hlprs.sendCmd((const byte[]) {0x64, 0x23}, 2);
        pf.printHex16(opcode);
        pf.print("  ");
        cursor = 13;
        dasm.dAsm2Byte(0, opcode);
        pf.println();
      } else if (hlprs.bufMatches(F("RAMSET"))) {               // RAMSET - Init first 23 bytes of SRAM
        hlprs.echoCmd();
        // Preset first 32 bytes of SRAM
        for (uint8_t ii = 0; ii < 32; ii++) {
          writeSRamByte(ramBase + ii, ii);
          pf.write('.');
        }
        pf.println();
      } else if (hlprs.bufMatches(F("Dxxxx xxxx"))) {           // Dxxxx xxxx - Disassemble two word instruction opcode xxxx + xxxx
        // Used during development of the disasembler
        pf.println((char *) hlprs.buf);
        uint16_t tmp = hlprs.convertHex(1);
        hlprs.flashbuf[0] = tmp;
        hlprs.flashbuf[1] = tmp >> 8;
        tmp = hlprs.convertHex(6);
        hlprs.flashbuf[2] = tmp;
        hlprs.flashbuf[3] = tmp >> 8;
        hlprs.flashLoaded = true;
        hlprs.flashStart = 0;
        dasm.dAsm(0, 1);
        hlprs.flashLoaded = false;
      } else if (hlprs.bufMatches(F("Dxxxx"))) {                // Dxxxx - Disassemble single word instruction opcode xxxx
        // Used during development of the disasembler
        pf.println((char *) hlprs.buf);
        uint16_t tmp = hlprs.convertHex(1);
        hlprs.flashbuf[0] = tmp;
        hlprs.flashbuf[1] = tmp >> 8;
        hlprs.flashLoaded = true;
        hlprs.flashStart = 0;
        dasm.dAsm(0, 1);
        hlprs.flashLoaded = false;
#endif
#if 0
        /*
         * Unfortunately, enabling access to the Bootloader via debugWire commands doesn't seem to work
         */
      } else if (hlprs.bufMatches(F("BOOT"))) {                 // BOOT - Display status of IVSEL bit in MCUCR Register (0x35)
        hlprs.echoCmd();
        uint8_t r16 = memo.readRegister(16);            // Save r16
        uint8_t cmd[] = {0x64,
                      0xD2, inHigh(0x3, 16), inLow(0x35, 16), 0x23,       // Build "in r16, 0x35" instruction
                      0xD2, outHigh(memo.dwdr, 16), outLow(memo.dwdr, 16), 0x23};   // Build "out DWDR, reg" instruction
        hlprs.sendCmd(cmd,  sizeof(cmd));
        hlprs.getResponse(1);
        writeRegister(16, r16);                 // Restore r16
        pf.print(F("MCUCR = "));
        pf.printHex8(hlprs.buf[0]);
        pf.println();
      } else if (hlprs.bufMatches(F("BOOT ON"))) {              // BOOT ON - Enable Access to Bootloader
        hlprs.echoCmd();
        pf.println();
        uint8_t r16 = memo.readRegister(16);            // Save r16
        uint8_t cmd[] = {0x64,
                      0xD2, 0xE0, 0x01, 0x23,   // ldi r16, 0x01 - MCUCR = (1 << IVCE);
                      0xD2, 0xBF, 0x05, 0x23,   // out 0x35, r16
                      0xD2, 0xE0, 0x02, 0x23,   // ldi r16, 0x02 - MCUCR = (1 << IVSEL);
                      0xD2, 0xBF, 0x05, 0x23};  // out 0x35, r16
        hlprs.sendCmd(cmd,  sizeof(cmd));
        writeRegister(16, r16);                 // Restore r16
      } else if (hlprs.bufMatches(F("BOOT OFF"))) {             // BOOT OFF - Disable Access to Bootloader
        hlprs.echoCmd();
         pf.println();
       uint8_t r16 = memo.readRegister(16);            // Save r16
        uint8_t cmd[] = {0x64,
                      0xD2, 0xE0, 0x01, 0x23,   // ldi r16, 0x01 - MCUCR = (1 << IVCE);
                      0xD2, 0xBF, 0x05, 0x23,   // out 0x35, r16
                      0xD2, 0xE0, 0x00, 0x23,   // ldi r16, 0x00 - MCUCR = 0;
                      0xD2, 0xBF, 0x05, 0x23};  // out 0x35, r16
        hlprs.sendCmd(cmd,  sizeof(cmd));
        writeRegister(16, r16);                 // Restore r16
#endif
      } else {
        pf.print((char *) hlprs.buf);
        pf.println(F(" ?"));
      }
    } else if (breakWatch) {
       if (debugWire.available()) {
        uint8_t cc = debugWire.read();
        if (cc == 0x55) {
          pf.println(F("BREAKPOINT"));
          dasm.dAsm(pcSave = hlprs.getPc(), 1);
          strcpy(hlprs.rpt, "STEP");
          breakWatch = false;
        }
      }
    }
  } else {
    // This section implements the In-System Programming Commands in a way that's compatible with using it to
    // program AVR Chips from ATTinyIDE
    if (Serial.available()) {
      char cc = toupper(Serial.read());
      switch (cc) {
#if 0
        case 'E':                                           // Erase Flash and EEPROM Memory
          if (enterProgramMode()) {
            isp.ispSend(0xAC, 0x80, 0x00, 0x00);
            busyWait();
            pf.println(F("Erased"));
          }
          break;
#endif
          
#if DEVELOPER
        case 'C':                                           // Send arbitrary 4 byte command sequence
          if (enterProgramMode()) {
            hlprs.getString();
            if (hlprs.convertToHex(0) == 4) {
              pf.print(F("Cmd:   "));
              pf.printBufToHex8 (4, false);
              pf.print(F(" = "));
              isp.ispSend(hlprs.buf[0], hlprs.buf[1], hlprs.buf[2], hlprs.buf[3]);
              pf.println();
            } else {
              pf.println(F("Err: 4 bytes needed"));
            }
          } 
          break;
#endif
        case 'F':                                             // Identify Device and Print current fuse settings
          if (!hasDeviceInfo) {
            identifyDevice();
          }
          if (hasDeviceInfo && enterProgramMode()) {
            uint8_t lowFuse, highFuse;
            pf.print(F("Low: "));
            pf.printHex8(lowFuse = isp.ispSend(0x50, 0x00, 0x00, 0x00));
            pf.print(F(", High: "));
            pf.printHex8(highFuse = isp.ispSend(0x58, 0x08, 0x00, 0x00));
            pf.print(F(", Extd: "));
            pf.printHex8(isp.ispSend(0x50, 0x08, 0x00, 0x00));
            pf.print(F(" - CHDIV8 "));
            pf.print((ckdiv8 & lowFuse) != 0 ? F("Disabled") : F("Enabled") );
            pf.print(F(", DWEN "));
            pf.println((dwen & highFuse) != 0 ? F("Disabled") : F("Enabled") );
          }
          break;
          
        case '8':                                             // Enable CKDIV8 (divide clock by 8)
        case '1':                                             // Disable CKDIV8
          if (!hasDeviceInfo) {
            identifyDevice();
          }
          if (ckdiv8 != 0 && enterProgramMode()) {
             uint8_t lowFuse = isp.ispSend(0x50, 0x00, 0x00, 0x00);
             uint8_t newFuse;
             boolean enable = cc == '8';
            if (enable) {
              // Enable CKDIV8 fuse by setting bit LOW
              newFuse = lowFuse & ~ckdiv8;
            } else {
              // Disable CKDIV8 fuse by setting bit HIGHW
              newFuse = lowFuse + ckdiv8;
            }
            if (newFuse != lowFuse) {
              isp.ispSend(0xAC, 0xA0, 0x00, newFuse);
              pf.print(F("CKDIV8 Fuse "));
              pf.println(enable ? F("Enabled") : F("Disabled"));
            } else {
              pf.print(F("CKDIV8 Fuse Already "));
              pf.println(enable ? F("Enabled") : F("Disabled"));
            }
          } else {
            pf.println(F("Unable to change CKDIV8 fuse"));
          }
          break;
          
        case '+':                                             // Enable debugWire Fuse
        case '-':                                             // Disable debugWire Fuse
          if (!hasDeviceInfo) {
            identifyDevice();
          }
          if (dwen != 0 && enterProgramMode()) {
            uint8_t highFuse = isp.ispSend(0x58, 0x08, 0x00, 0x00);
            uint8_t newFuse;
            boolean enable = cc == '+';
            if (enable) {
              // Enable debugWire DWEN fuse by setting bit LOW
              newFuse = highFuse & ~dwen;
            } else {
              // Disable debugWire DWEN fuse by setting bit HIGH
              newFuse = highFuse + dwen;
            }
            if (newFuse != highFuse) {
              isp.ispSend(0xAC, 0xA8, 0x00, newFuse);
              pf.print(F("DWEN Fuse "));
              pf.println(enable ? F("Enabled") : F("Disabled"));
            } else {
              pf.print(F("DWEN Fuse Already "));
              pf.println(enable ? F("Enabled") : F("Disabled"));
            }
          } else {
            pf.println(F("Unable to change DWEN fuse"));
          }
          break;
          
#if DEVELOPER
        case 'P':                                             // Turn on power to chip (used to run code)
          powerOff();
          digitalWrite(VCC, HIGH);
          digitalWrite(RESET, LOW);
          pinMode(RESET, INPUT);
          pinMode(VCC, OUTPUT);
          pf.println(F("VCC On"));
          break;
          
        case 'O':                                             // Switch off power to chip
          powerOff();
          pf.println(F("VCC off"));
          break;
#endif
      
        case 'B':                                             // Cycle Vcc and Send BREAK to engage debugWire Mode
          if (doBreak()) {
            isp.disableSpiPins();
            hlprs.printCmd(F("SIG"));
            hlprs.sendCmd((const byte[]) {0xF3}, 1);
            hlprs.printBufToHex8(hlprs.getResponse(2), false);
            printPartFromId(hlprs.buf[0], hlprs.buf[1]);
            pf.print(F("Flash:  "));
            Serial.print(flashSize, DEC);
            pf.println(F(" bytes"));
            pf.print(F("SRAM:   "));
            Serial.print(ramSize, DEC);
            pf.println(F(" bytes"));
            pf.print(F("SRBase: 0x"));
            pf.printHex16(ramBase);
            pf.println();
            pf.print(F("EEPROM: "));
            Serial.print(eeSize, DEC);
            pf.println(F(" bytes"));
#if DEVELOPER
            pf.print(F("DWEN:   0x"));
            pf.printHex8(dwen);
            pf.println();
            pf.print(F("CKDIV8: 0x"));
            pf.printHex8(ckdiv8);
            pf.println();
#endif
            pcSave = 0;
          }
          break;

        default:
          printMenu();
          break;
      }
      // Gobble up any unused characters
      while (Serial.available()) {
        Serial.read();
      }
    }
  }
}
// used in Debugger only
void Debugger::printMenu () {
  pf.println(F("Commands:"));
  pf.println(F(" F - Identify Device & Print Fuses"));
  pf.println(F(" + - Enable debugWire DWEN Fuse"));
  pf.println(F(" - - Disable debugWire DWEN Fuse"));
  pf.println(F(" 8 - Enable CKDIV8 (divide clock by 8)"));
  pf.println(F(" 1 - Disable CKDIV8"));
  pf.println(F(" B - Engage Debugger"));
#if 0
  pf.println(F(" E - Erase Flash & EEPROM"));
#endif
#if DEVELOPER
  pf.println(F(" C - Send 4 Byte ISP Command"));
  pf.println(F(" P - Vcc On"));
  pf.println(F(" O - Vcc Off"));
#endif 
}

// ========================================
// Private methods:
// ----------------------------------------

// used in Debugger only
void Debugger::powerOn () {
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  isp.enableSpiPins();
  // Apply Vcc
  pinMode(VCC, OUTPUT);
  digitalWrite(VCC, HIGH);
  // Bounce RESET
  digitalWrite(RESET, HIGH);
  delay(50);
  digitalWrite(RESET, LOW);
  delay(50);
  digitalWrite(RESET, HIGH);
  delay(50);
  digitalWrite(RESET, LOW);
  delay(50);
}
// used in Debugger only
void Debugger::powerOff () {
  isp.disableSpiPins();
  digitalWrite(VCC, LOW);
  pinMode(VCC, INPUT);
  delay(50);
  progMode = false;
}
// used in Debugger only
boolean Debugger::enterProgramMode () {
  if (progMode) {
    return true;
  }
  uint8_t timeout = 0;
  uint8_t rsp;
  do {
    if (timeout > 0) {
      powerOff();
    }
    powerOn();
    isp.ispSend(0xAC, 0x53, 0x00, 0x00);
    rsp = isp.ispSend(0x30, 0x00, 0x00, 0x00);
  } while (rsp != 0x1E && ++timeout < 5);
  progMode = timeout < 5;
  if (!progMode) {
    pf.println(F("Timeout: Chip may have DWEN bit enabled"));
  }
  return progMode;
}
// used in Debugger only
// Read two byte chip signature into buf[]
void Debugger::getChipId () {
  if (enterProgramMode()) {
    for (uint8_t ii = 0; ii < 2; ii++) {
      hlprs.buf[ii] = isp.ispSend(0x30, 0x00, ii + 1, 0x00);
    }
  }
}
// used in Debugger only
uint16_t Debugger::identifyDevice () {
 if (enterProgramMode()) {
    pf.print(F("SIG:   "));
    getChipId();
    hlprs.printBufToHex8(2, false);
    printPartFromId(hlprs.buf[0], hlprs.buf[1]);
    return (hlprs.buf[0] << 8) + hlprs.buf[1];
  }
  return 0;
}
// used in Debugger only
void Debugger::busyWait () {
  while ((isp.ispSend(0xF0, 0x00, 0x00, 0x00) & 1) == 1) {
    delay(10);
  }
}


//    Sig      Device   CKDIV8  DWEN
//    0x920B - Tiny24     L:7   H:6
//    0x9108 - Tiny25     L:7   H:6
//    0x9205 - Mega48A    L:7   H:6
//    0x9207 - Tiny44     L:7   H:6
//    0x9206 - Tiny45     L:7   H:6
//    0x920A - Mega48PA   L:7   H:6
//    0x9215 - Tiny441    L:7   H:6
//    0x930A - Mega88A    L:7   H:6
//    0x930C - Tiny84     L:7   H:6
//    0x930B - Tiny85     L:7   H:6
//    0x930F - Mega88PA   L:7   H:6
//    0x9315 - Tiny841    L:7   H:6
//    0x9406 - Mega168A   L:7   H:6
//    0x940B - Mega168PA  L:7   H:6
//    0x950F - Mega328P   L:7   H:6   Arduino defaults: L: 0xFF. H: 0xDA, E: 0xFD
//    0x9514 - Mega328    L:7   H:6
//    
//    0x900A - Tiny13     L:4   H:3
//    0x910A - Tiny2313   L:7   H:7
//    0x9389 - Mega8u2    L:7   H:7
//    0x9489 - Mega16U2   L:7   H:7
//    0x958A - Mega32U2   L:7   H:7

// used in Debugger only
void Debugger::printPartFromId (uint8_t sig1, uint8_t sig2) {
  boolean isTiny = false;
  pf.print(F("= "));
  dwen = 0x40;                      // DWEN bit default location (override, as needed)
  ckdiv8 = 0x80;                    // CKDIV8 bit default location (override, as needed)
  hasDeviceInfo = true;
  if (sig1 == 0x90) {            
    if (sig2 == 0x07) {             // 0x9007 - Tiny13
      pf.println(F("Tiny13"));
      flashSize = 1024;
      ramBase = 0x60;
      ramSize = 64;
      eeSize = 64;
      memo.dwdr = 0x2E;
      dwen = 0x08;
      ckdiv8 = 0x10;
      isTiny = true;
    }
  } else if (sig1 == 0x91) {
    if (sig2 == 0x0A) {             // 0x920A - Tiny2313
      pf.println(F("Tiny2313"));  
      flashSize = 2048;
      ramBase = 0x60;
      ramSize = 128;
      eeSize = 128;
      memo.dwdr = 0x1F;
      dwen = 0x80;
      isTiny = true;
    } else if (sig2 == 0x0B) {      // 0x920B - Tiny24
      pf.println(F("Tiny24"));  
      flashSize = 2048;
      ramBase = 0x60;
      ramSize = 128;
      eeSize = 128;
      memo.dwdr = 0x27;
      isTiny = true;
   } else if (sig2 == 0x08) {      // 0x9108 - Tiny25
      pf.println(F("Tiny25"));  
      flashSize = 2048;
      ramBase = 0x60;
      ramSize = 128;
      eeSize = 128;
      memo.dwdr = 0x22;
      isTiny = true;
    }
  } else if (sig1 == 0x92) {
    if (sig2 == 0x05) {             // 0x9295 - Mega48A
      pf.println(F("Mega48A"));
      flashSize = 4096;
      ramBase = 0x100;
      ramSize = 512;
      eeSize = 256;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x07) {      // 0x9207 - Tiny44
      pf.println(F("Tiny44"));  
      flashSize = 4096;
      ramBase = 0x60;
      ramSize = 256;
      eeSize = 256;
      memo.dwdr = 0x27;
      isTiny = true;
    } else if (sig2 == 0x06) {      // 0x9206 - Tiny45
      pf.println(F("Tiny45"));  
      flashSize = 4096;
      ramBase = 0x60;
      ramSize = 256;
      eeSize = 256;
      memo.dwdr = 0x22;
      isTiny = true;
    } else if (sig2 == 0x0A) {      // 0x920A - Mega48PA
      pf.println(F("Mega48PA"));  
      flashSize = 4096;
      ramBase = 0x100;
      ramSize = 512;
      eeSize = 256;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x15) {      // 0x9215 - Tiny441
      pf.println(F("Tiny441"));  
      flashSize = 4096;
      ramBase = 0x100;
      ramSize = 256;
      eeSize = 256;
      memo.dwdr = 0x27;
      isTiny = true;
    }
  } else if (sig1 == 0x93) {
    if (sig2 == 0x0A) {             // 0x930A - Mega88A
      pf.println(F("Mega88A"));  
      flashSize = 8192;
      ramBase = 0x100;
      ramSize = 1024;
      eeSize = 512;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x0C) {      // 0x930C - Tiny84
      pf.println(F("Tiny84"));  
      flashSize = 8192;
      ramBase = 0x60;
      ramSize = 512;
      eeSize = 512;
      memo.dwdr = 0x27;
      isTiny = true;
    } else if (sig2 == 0x0B) {      // 0x930B - Tiny85
      pf.println(F("Tiny85"));  
      flashSize = 8192;
      ramBase = 0x60;
      ramSize = 512;
      eeSize = 512;
      memo.dwdr = 0x22;
      isTiny = true;
    } else if (sig2 == 0x0F) {      // 0x930F - Mega88PA
      pf.println(F("Mega88PA"));  
      flashSize = 8192;
      ramBase = 0x100;
      ramSize = 1024;
      eeSize = 512;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x15) {      // 0x9315 - Tiny841
      pf.println(F("Tiny841"));  
      flashSize = 8192;
      ramBase = 0x100;
      ramSize = 512;
      eeSize = 512;
      memo.dwdr = 0x27;
      isTiny = true;
    } else if (sig2 == 0x89) {      // 0x9389 - Mega8u2
      pf.println(F("Mega8u2"));  
      flashSize = 8192;
      ramBase = 0x100;
      ramSize = 512;
      eeSize = 512;
      memo.dwdr = 0x31;
    }
  } else if (sig1 == 0x94) {
    if (sig2 == 0x06) {             // 0x9406 - Mega168A
      pf.println(F("Mega168A"));  
      flashSize = 16384;
      ramBase = 0x100;
      ramSize = 1024;
      eeSize = 512;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x0B) {      // 0x9408 - Mega168PA
      pf.println(F("Mega168PA"));  
      flashSize = 16384;
      ramBase = 0x100;
      ramSize = 1024;
      eeSize = 512;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x89) {      // 0x9489 - Mega16U2
      pf.println(F("Mega16U2"));  
      flashSize = 16384;
      ramBase = 0x100;
      ramSize = 512;
      eeSize = 512;
      dwen = 0x80;
      memo.dwdr = 0x31;
    }
  } else if (sig1 == 0x95) {
    if (sig2 == 0x0F) {             // 0x950F - Mega328P
      pf.println(F("Mega328P"));  
      flashSize = 32768;
      ramBase = 0x100;
      ramSize = 2048;
      eeSize = 1024;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x14) {      // 0x9514 - Mega328
      pf.println(F("Mega328"));  
      flashSize = 32768;
      ramBase = 0x100;
      ramSize = 2048;
      eeSize = 1024;
      memo.dwdr = 0x31;
    } else if (sig2 == 0x8A) {      // 0x958A - Mega32U2
      pf.println(F("Mega32U2"));  
      flashSize = 32768;
      ramBase = 0x100;
      ramSize = 1024;
      eeSize = 1024;
      dwen = 0x80;
      memo.dwdr = 0x31;
    }
  } else {
    hasDeviceInfo = false;
  }
  // Set EEPROM Access Registers
  if (isTiny) {
    memo.eecr  = 0x1C;
    memo.eedr  = 0x1D;
    memo.eearl = 0x1E;
    memo.eearh = 0x1F;
  } else {
    memo.eecr  = 0x1F;
    memo.eedr  = 0x20;
    memo.eearl = 0x21;
    memo.eearh = 0x22;
  }
}

/*
uint8_t printPartFromId (uint8_t sig1, uint8_t sig2) {
  boolean isTiny = false;
  pf.print(F("= "));
  dwen = 0x40;                      // DWEN bit default location (override, as needed)
  ckdiv8 = 0x80;                    // CKDIV8 bit default location (override, as needed)
  //hasDeviceInfo = true;

} else {
    hasDeviceInfo = false;
  }
  // Set EEPROM Access Registers
  if (isTiny) {
    memo.eecr  = 0x1C;
    memo.eedr  = 0x1D;
    memo.eearl = 0x1E;
    memo.eearh = 0x1F;
  } else {
    memo.eecr  = 0x1F;
    memo.eedr  = 0x20;
    memo.eearl = 0x21;
    memo.eearh = 0x22;
  }
  for(uint8_t idx = 0; idx < partIDArray.length; idx++} {
    if(sig1 == partIDArray[idx].sig1) {
      if(sig2 == partIDArray[idx].sig2) {
        return idx;
      }
    }
  }
  return 255;
}

typedef struct partID {
  const String mcuLabel;
  const int16_t flashSize;
  const int16_t ramBase;
  const int16_t ramSize;
  const int16_t eeSize;
  const int16_t memo.dwdr;
} PARTID;

PARTID partID {
  Tiny13, ...
};
 // byte, byte, String,       uint,  uint, uint, uint, byte,  byte, byte, boolean  
 // sig1, sig2, mcuLabel,    flash,  ramB,  ram,   ee, dwdr,  dwen, ckd8, tiny
   {0x90, 0x07, "Tiny13",     1024,  0x60,   64,   64, 0x2E,  0x08, 0x10, true} // 0x9007 - Tiny13
   {0x91, 0x0A, "Tiny2313",   2048,  0x60,  128,  128, 0x1F,  0x80,   --, true} // 0x920A - Tiny2313
   {0x91, 0x0B, "Tiny24",     2048,  0x60,  128,  128, 0x27,    --,   --, true} // 0x920B - Tiny24
   {0x91, 0x08, "Tiny25",     2048,  0x60,  128,  128, 0x22,    --,   --, true} // 0x9108 - Tiny25
   {0x92, 0x05, "Mega48A",    4096, 0x100,  512,  256, 0x31,    --,   --,   --} // 0x9295 - Mega48A
   {0x92, 0x07, "Tiny44",     4096,  0x60,  256,  256, 0x27,    --,   --, true} // 0x9207 - Tiny44
   {0X92, 0x06, "Tiny45",     4096,  0x60,  256,  256, 0x22,    --,   --, true} // 0x9206 - Tiny45
   {0X92, 0x0A, "Mega48PA",   4096, 0x100,  512,  256, 0x31,    --,   --,   --} // 0x920A - Mega48PA
   {0x92, 0x15, "Tiny441",    4096, 0x100,  256,  256, 0x27,    --,   --, true} // 0x9215 - Tiny441
   {0x93, 0x0A, "Mega88A",    8192, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x930A - Mega88A
   {0x93, 0x0C, "Tiny84",     8192,  0x60,  512,  512, 0x27,    --,   --, true} // 0x930C - Tiny84
   {0x93, 0x0B, "Tiny85",     8192,  0x60,  512,  512, 0x22,    --,   --, true} // 0x930B - Tiny85
   {0x93, 0x0F, "Mega88PA",   8192, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x930F - Mega88PA
   {0x93, 0x15, "Tiny841",    8192, 0x100,  512,  512, 0x27,    --,   --, true} // 0x9315 - Tiny841
   {0x93, 0x89, "Mega8u2",    8192, 0x100,  512,  512, 0x31,    --,   --,   --} // 0x9389 - Mega8u2
   {0x94, 0x06, "Mega168A",  16384, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x9406 - Mega168A
   {0x94, 0x0B, "Mega168PA", 16384, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x9408 - Mega168PA
   {0x94, 0x89, "Mega16U2",  16384, 0x100,  512,  512, 0x31,  0x80,   --,   --} // 0x9489 - Mega16U2
   {0x95, 0x0F, "Mega328P",  32768, 0x100, 2048, 1024, 0x31,    --,   --,   --} // 0x950F - Mega328P
   {0x95, 0x14, "Mega328",   32768, 0x100, 2048, 1024, 0x31,    --,   --,   --} // 0x9514 - Mega328
   {0x95, 0x8A, "Mega32U2",  32768, 0x100, 1024, 1024, 0x31,  0x80,   --,   --} // 0x958A - Mega32U2
};
// approx. 500 Bytes
// if(true == isTiny) mcuLabels begins with "T", otherwhise with "M"
// almost always if(true == isTiny && !Tinyx41) {ramBase = 0x60; }else {ramBase = 0x100;}
// if(xxxxVVzz) flash is toInt(VV)*1024 Bytes
*/
/*
uint8_t printPartFromId (uint8_t sig1, uint8_t sig2) {
  boolean isTiny = false;
  pf.print(F("= "));
  dwen = 0x40;                      // DWEN bit default location (override, as needed)
  ckdiv8 = 0x80;                    // CKDIV8 bit default location (override, as needed)
  //hasDeviceInfo = true;

} else {
    hasDeviceInfo = false;
  }
  // Set EEPROM Access Registers
  if (isTiny) {
    memo.eecr  = 0x1C;
    memo.eedr  = 0x1D;
    memo.eearl = 0x1E;
    memo.eearh = 0x1F;
  } else {
    memo.eecr  = 0x1F;
    memo.eedr  = 0x20;
    memo.eearl = 0x21;
    memo.eearh = 0x22;
  }
// if(true == isTiny) mcuLabels begins with "T", otherwhise with "M"
  isTiny = false;
  if(myString.startsWith("T")) {
    isTiny = true;
    if(myString.
    ramBase = 0x60
  }
// almost always if(true == isTiny && !Tinyx41) {ramBase = 0x60; }else {ramBase = 0x100;}
  if(xxxxVVzz) flash is toInt(VV)*1024 Bytes
// or easier from byte/int to String

int16_t flashSize = (1 << (flash%2)) + 


const String PROGMEM MCUSeriesArray[seriesID] = {"Tiny", "Mega", "xMega", "PIC"};
String mcuLabel = (MCUSeriesArray[seriesID] + String(flashSize) + PartArray[idx].Label);


  for(uint8_t idx = 0; idx < partIDArray.length; idx++} {
    if(sig1 == partIDArray[idx].sig1) {
      if(sig2 == partIDArray[idx].sig2) {
        return idx;
      }
    }
  }
  return 255;
}

uint32_t sfloatToValStart(uint8_t mySFloat) {
  //                    sign bit
  sfloatToVal(mantissa, (mySFloat & 0x80) << 24);
}

uint32_t exponent(const uint8_t& base, uint8_t exponent) {
  return exponent(base, exponent, 1);
}

uint32_t exponent(const uint8_t& base, uint8_t exponent, uint32_t result) {
  if(exponent == 0) return result;
  result = base * result;
  return exponent(base, exponent-1, result);
}

// val = sign * base ^ exponent
uint32_t sfloatToVal(const uint8_t& base, const uint8_t exponent, const uint8_t& precisition, uint8_t& significant) {
  return sfloatToVal(base, precisition-1, significant, exponent(base, exponent);
}

// val = sign * base ^ exponent
uint32_t sfloatToVal(const uint8_t& base, uint8_t precisition, uint8_t& significant, uint32_t& result) {
  if(precisition == 0) return result;
  // result = result + (result >> (significant & precisition) * precisition;
  result = result | result(result >> (significant & precisition) * precisition; // ignore carrybit
  return sfloatToVal(significant, precisition-1, significant, result)
}



typedef struct partID {
  const String mcuLabel;
  const uint16_t flashSize;
  //const uint16_t ramBase;
  const uint16_t ramSize;
  const uint16_t eeSize;
  const uint8_t memo.dwdr;
  const uint8_t dwen;
} PARTID;

PARTID partID {
  Tiny13, ...
};
 // improved:   pointer,   512*2^x,  if(), *2^x, *2^x, if(),  if(), if(), extractFromLabel
 // byte, byte, char[],       byte,  byte, uint, uint, byte,  byte, byte, boolean  
 // sig1, sig2, mcuLabel,    flash,  ramB,  ram,   ee, dwdr,  dwen, ckd8, tiny
   {0x90, 0x07, "Tiny13",     1024,  0x60,   64,   64, 0x2E,  0x08, 0x10, true} // 0x9007 - Tiny13
   {0x91, 0x0A, "Tiny2313",   2048,  0x60,  128,  128, 0x1F,  0x80,   --, true} // 0x920A - Tiny2313
   {0x91, 0x0B, "Tiny24",     2048,  0x60,  128,  128, 0x27,    --,   --, true} // 0x920B - Tiny24
   {0x91, 0x08, "Tiny25",     2048,  0x60,  128,  128, 0x22,    --,   --, true} // 0x9108 - Tiny25
   {0x92, 0x05, "Mega48A",    4096, 0x100,  512,  256, 0x31,    --,   --,   --} // 0x9295 - Mega48A
   {0x92, 0x07, "Tiny44",     4096,  0x60,  256,  256, 0x27,    --,   --, true} // 0x9207 - Tiny44
   {0X92, 0x06, "Tiny45",     4096,  0x60,  256,  256, 0x22,    --,   --, true} // 0x9206 - Tiny45
   {0X92, 0x0A, "Mega48PA",   4096, 0x100,  512,  256, 0x31,    --,   --,   --} // 0x920A - Mega48PA
   {0x92, 0x15, "Tiny441",    4096, 0x100,  256,  256, 0x27,    --,   --, true} // 0x9215 - Tiny441
   {0x93, 0x0A, "Mega88A",    8192, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x930A - Mega88A
   {0x93, 0x0C, "Tiny84",     8192,  0x60,  512,  512, 0x27,    --,   --, true} // 0x930C - Tiny84
   {0x93, 0x0B, "Tiny85",     8192,  0x60,  512,  512, 0x22,    --,   --, true} // 0x930B - Tiny85
   {0x93, 0x0F, "Mega88PA",   8192, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x930F - Mega88PA
   {0x93, 0x15, "Tiny841",    8192, 0x100,  512,  512, 0x27,    --,   --, true} // 0x9315 - Tiny841
   {0x93, 0x89, "Mega8u2",    8192, 0x100,  512,  512, 0x31,    --,   --,   --} // 0x9389 - Mega8u2
   {0x94, 0x06, "Mega168A",  16384, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x9406 - Mega168A
   {0x94, 0x0B, "Mega168PA", 16384, 0x100, 1024,  512, 0x31,    --,   --,   --} // 0x9408 - Mega168PA
   {0x94, 0x89, "Mega16U2",  16384, 0x100,  512,  512, 0x31,  0x80,   --,   --} // 0x9489 - Mega16U2
   {0x95, 0x0F, "Mega328P",  32768, 0x100, 2048, 1024, 0x31,    --,   --,   --} // 0x950F - Mega328P
   {0x95, 0x14, "Mega328",   32768, 0x100, 2048, 1024, 0x31,    --,   --,   --} // 0x9514 - Mega328
   {0x95, 0x8A, "Mega32U2",  32768, 0x100, 1024, 1024, 0x31,  0x80,   --,   --} // 0x958A - Mega32U2
};
// approx. 500 Bytes
// if(true == isTiny) mcuLabels begins with "T", otherwhise with "M"
// almost always if(true == isTiny && !Tinyx41) {ramBase = 0x60; }else {ramBase = 0x100;}
// if(xxxxVVzz) flash is toInt(VV)*1024 Bytes
*/
// used in Debugger only
void Debugger::sendBreak () {
  debugWire.sendBreak(); // link to sendBreak routine of OnePinSerial object debugWire
}
// used in Debugger only
boolean Debugger::doBreak () { 
  pf.println(F("Cycling Vcc"));
  powerOff();
  digitalWrite(VCC, HIGH);
  digitalWrite(RESET, LOW);
  pinMode(RESET, INPUT);
  pinMode(VCC, OUTPUT);
  delay(100);
  // Mesaure debugWire Baud rate by sending two BREAK commands to measure both high-going and
  // low-going pulses in the 0x55 response byte
  debugWire.enable(false);
  unsigned long pulse = 0;
  uint8_t oldSREG = SREG;
  cli();                      // turn off interrupts for timing
  sendBreak();
  for (uint8_t ii = 0; ii < 4; ii++) {
    pulse += pulseIn(RESET, HIGH, 20000);
  }
  delay(10);
  sendBreak();
  for (uint8_t ii = 0; ii < 4; ii++) {
    pulse += pulseIn(RESET, LOW, 20000);
  }
  SREG = oldSREG;             // turn interrupts back on
  delay(10);
  unsigned long baud = 8000000L / pulse;
  pf.cursor = 0;
  hlprs.printCmd(F("Speed"));
  Serial.print(baud, DEC);
  Serial.println(F(" bps"));
  debugWire.enable(true);
  debugWire.begin(baud);                            // Set computed baud rate
  hlprs.setTimeoutDelay(DWIRE_RATE);                      // Set timeout based on baud rate
  pf.print(F("Sending BREAK: "));
  sendBreak();
  if (hlprs.checkCmdOk()) {
    debugWireOn = true;
    pf.println(F("debugWire Enabled"));
  } else {
    pf.println();
  }
  // Discard any suprious characters, such as '\n'
  while (Serial.available()) {
    Serial.read();
  }
  return debugWireOn;
}

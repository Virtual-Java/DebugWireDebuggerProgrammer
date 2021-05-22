// MemoryHandler.cpp

//  The functions used to read read and write registers, SRAM and flash memory use "in reg,addr" and "out addr,reg" instructions 
//  to trnasfers data over debugWire via the DWDR register.  However, because the location of the DWDR register can vary from device
//  to device, the necessary "in" and "out" instructions need to be build dynamically using the following 4 functions:
// 
//         -- --                            In:  1011 0aar rrrr aaaa
//      D2 B4 02 23 xx   - in r0,DWDR (xx)       1011 0100 0000 0010  a=100010(22), r=00000(00) // How it's used
//         -- --                            Out: 1011 1aar rrrr aaaa
//      D2 BC 02 23 <xx> - out DWDR,r0           1011 1100 0000 0010  a=100010(22), r=00000(00) // How it's used
//
//  Note: 0xD2 sets next two bytes as instruction which 0x23 then executes.  So, in first example, the sequence D2 B4 02 23 xx
//  copies the value xx into the r0 register via the DWDR register.  The second example does the reverse and returns the value
//  in r0 as <xx> by sending it to the DWDR register.

#include "MemoryHandler.h"
//#include "Helpers.h"

class Helpers; // forward declaration

Helpers  hlprs;

// ========================================
// public methods:
// ----------------------------------------

MemoryHandler::MemoryHandler () {
  
}

// used in MemoryHandler and Debugger
// Build high byte of opcode for "out addr, reg" instruction
uint8_t MemoryHandler::outHigh (uint8_t add, uint8_t reg) {
  // out addr,reg: 1011 1aar rrrr aaaa
  return 0xB8 + ((reg & 0x10) >> 4) + ((add & 0x30) >> 3);
}
// used in MemoryHandler and Debugger
// Build low byte of opcode for "out addr, reg" instruction
uint8_t MemoryHandler::outLow (uint8_t add, uint8_t reg) {
  // out addr,reg: 1011 1aar rrrr aaaa
  return (reg << 4) + (add & 0x0F);
}
// used in MemoryHandler and Debugger
// Build high byte of opcode for "in reg,addr" instruction
uint8_t MemoryHandler::inHigh  (uint8_t add, uint8_t reg) {
  // in reg,addr:  1011 0aar rrrr aaaa
  return 0xB0 + ((reg & 0x10) >> 4) + ((add & 0x30) >> 3);
}
// used in MemoryHandler and Debugger
// Build low byte of opcode for "in reg,addr" instruction
uint8_t MemoryHandler::inLow  (uint8_t add, uint8_t reg) {
  // in reg,addr:  1011 0aar rrrr aaaa
  return (reg << 4) + (add & 0x0F);
}
// used in MemoryHandler and Debugger
// Set register <reg> by building and executing an "out <reg>,DWDR" instruction via the CMD_SET_INSTR register
void MemoryHandler::writeRegister (uint8_t reg, uint8_t val) {
  uint8_t wrReg[] = {0x64,                                               // Set up for single step using loaded instruction
                  0xD2, inHigh(dwdr, reg), inLow(dwdr, reg), 0x23,    // Build "in reg,DWDR" instruction
                  val};                                               // Write value to register via DWDR
  hlprs.sendCmd(wrReg,  sizeof(wrReg));
}
// used in MemoryHandler, DisAssembler and Debugger
// Read register <reg> by building and executing an "out DWDR,<reg>" instruction via the CMD_SET_INSTR register
uint8_t MemoryHandler::readRegister (uint8_t reg) {
  uint8_t rdReg[] = {0x64,                                               // Set up for single step using loaded instruction
                  0xD2, outHigh(dwdr, reg), outLow(dwdr, reg),        // Build "out DWDR, reg" instruction
                  0x23};                                              // Execute loaded instruction
  hlprs.sendCmd(rdReg,  sizeof(rdReg));
  hlprs.getResponse(1);                                                     // Get value sent as response
  return hlprs.buf[0];;
}
// used in Debugger only
// Read registers 0-31 into buf[]
boolean MemoryHandler::readAllRegisters () {
  uint8_t rdRegs[] = {0x66,                                              // Set up for read/write using repeating simulated instructions
                  0xD0, 0x00, 0x00,                                   // Set Start Reg number (r0)
                  0xD1, 0x00, 0x20,                                   // Set End Reg number (r31) + 1
                  0xC2, 0x01,                                         // Set repeating copy to via DWDR to registers
                  0x20};                                              // Go
  hlprs.sendCmd(rdRegs,  sizeof(rdRegs));
  return hlprs.getResponse(32) == 32;
}
// used in Debugger only
// Set or clear bit in I/O address 0x00 - 0x1F by generating and executing an "sbi" or "cbi" instruction
void MemoryHandler::setClrIOBit (uint8_t addr, uint8_t bit, boolean set) {
  // Generate an "sbi/cbi addr,bit" instruction
  uint8_t cmd[] = {0x64, 0xD2, set ? 0x9A : 0x98, ((addr & 0x1F) << 3) + (bit & 0x7), 0x23};  // 1001 10x0 aaaa abbb
  hlprs.sendCmd(cmd,  sizeof(cmd));
}
// used in Debugger only
// Write one byte to SRAM address space using an SRAM-based value for <addr>, not an I/O address
void MemoryHandler::writeSRamByte (uint16_t addr, uint8_t val) {
  uint8_t r30 = readRegister(30);        // Save r30 (Z Reg low)
  uint8_t r31 = readRegister(31);        // Save r31 (Z Reg high)
  uint8_t wrSRam[] = {0x66,                                              // Set up for read/write using repeating simulated instructions
                   0xD0, 0x00, 0x1E,                                  // Set Start Reg number (r30)
                   0xD1, 0x00, 0x20,                                  // Set End Reg number (r31) + 1
                   0xC2, 0x05,                                        // Set repeating copy to registers via DWDR
                   0x20,                                              // Go
                   addr & 0xFF, addr >> 8,                            // r31:r30 (Z) = addr
                   0xD0, 0x00, 0x01,
                   0xD1, 0x00, 0x03,
                   0xC2, 0x04,                                        // Set simulated "in r?,DWDR; st Z+,r?" insrtuctions
                   0x20,                                              // Go
                   val};
  hlprs.sendCmd(wrSRam, sizeof(wrSRam));
  writeRegister(30, r30);             // Restore r30
  writeRegister(31, r31);             // Restore r31
}
// used in Debugger and DisAssembler
// Read one byte from SRAM address space using an SRAM-based value for <addr>, not an I/O address
uint8_t MemoryHandler::readSRamByte (uint16_t addr) {
  uint8_t r30 = readRegister(30);        // Save r30 (Z Reg low)
  uint8_t r31 = readRegister(31);        // Save r31 (Z Reg high)
  uint8_t rdSRam[] = {0x66,                                              // Set up for read/write using repeating simulated instructions
                   0xD0, 0x00, 0x1E,                                  // Set Start Reg number (r30)
                   0xD1, 0x00, 0x20,                                  // Set End Reg number (r31) + 1
                   0xC2, 0x05,                                        // Set repeating copy to registers via DWDR
                   0x20,                                              // Go
                   addr & 0xFF, addr >> 8,                            // r31:r30 (Z) = addr
                   0xD0, 0x00, 0x00,                                  // 
                   0xD1, 0x00, 0x02,                                  // 
                   0xC2, 0x00,                                        // Set simulated "ld r?,Z+; out DWDR,r?" insrtuctions
                   0x20};                                             // Go
  hlprs.sendCmd(rdSRam, sizeof(rdSRam));
  hlprs.getResponse(1);
  writeRegister(30, r30);             // Restore r30
  writeRegister(31, r31);             // Restore r31
  return hlprs.buf[0];
}
// used in Debugger only
// Read <len> bytes from SRAM address space into buf[] using an SRAM-based value for <addr>, not an I/O address
// Note: can't read addresses that correspond to  r28-31 (Y & Z Regs) because Z is used for transfer (not sure why Y is clobbered) 
boolean MemoryHandler::readSRamBytes (uint16_t addr, uint8_t len) {
  uint16_t len2 = len * 2;
  uint8_t r28 = readRegister(28);        // Save r28 (Y Reg low)
  uint8_t r29 = readRegister(29);        // Save r29 (Y Reg high)
  uint8_t r30 = readRegister(30);        // Save r30 (Z Reg low)
  uint8_t r31 = readRegister(31);        // Save r31 (Z Reg high)
  uint8_t rsp;
  hlprs.reportTimeout = false;
  for (uint8_t ii = 0; ii < 4; ii++) {
    uint8_t rdSRam[] = {0x66,                                            // Set up for read/write using repeating simulated instructions
                     0xD0, 0x00, 0x1E,                                // Set Start Reg number (r30)
                     0xD1, 0x00, 0x20,                                // Set End Reg number (r31) + 1
                     0xC2, 0x05,                                      // Set repeating copy to registers via DWDR
                     0x20,                                            // Go
                     addr & 0xFF, addr >> 8,                          // r31:r30 (Z) = addr
                     0xD0, 0x00, 0x00,                                // 
                     0xD1, len2 >> 8, len2,                           // Set repeat count = len * 2
                     0xC2, 0x00,                                      // Set simulated "ld r?,Z+; out DWDR,r?" instructions
                     0x20};                                           // Go
    hlprs.sendCmd(rdSRam, sizeof(rdSRam));
    rsp = hlprs.getResponse(len);
    if (rsp == len) {
      break;
    } else {
      // Wait and retry read
      delay(5);
    }
  }
  hlprs.reportTimeout = true;
  writeRegister(28, r28);             // Restore r28
  writeRegister(29, r29);             // Restore r29
  writeRegister(30, r30);             // Restore r30
  writeRegister(31, r31);             // Restore r31
  return rsp == len;
}


//   EEPROM Notes: This section contains code to read and write from EEPROM.  This is accomplished by setting parameters
//    into registers 28 - r31 and then using the 0xD2 command to send and execure a series of instruction opcodes on the
//    target device. 
// 
//   EEPROM Register Locations for ATTiny25/45/85, ATTiny24/44/84, ATTiny13, Tiny2313, Tiny441/841
//     EECR    0x1C EEPROM Control Register
//     EEDR    0x1D EEPROM Data Register
//     EEARL   0x1E EEPROM Address Register (low byte)
//     EEARH   0x1F EEPROM Address Register (high byte)
// 
//   EEPROM Register Locations for ATMega328, ATMega32U2/16U2/32U2, etc.
//     EECR    0x1F EEPROM Control Register
//     EEDR    0x20 EEPROM Data Register
//     EEARL   0x21 EEPROM Address Register (low byte)
//     EEARH   0x22 EEPROM Address Register (high byte)

// 
//   Read one byte from EEPROM
//   
// used in Debugger only
uint8_t MemoryHandler::readEepromByte (uint16_t addr) {
  uint8_t setRegs[] = {0x66,                                               // Set up for read/write using repeating simulated instructions
                    0xD0, 0x00, 0x1C,                                   // Set Start Reg number (r28(
                    0xD1, 0x00, 0x20,                                   // Set End Reg number (r31) + 1
                    0xC2, 0x05,                                         // Set repeating copy to registers via DWDR
                    0x20,                                               // Go
                    0x01, 0x01, addr & 0xFF, addr >> 8};                // Data written into registers r28-r31
  uint8_t doRead[]  = {0x64,                                               // Set up for single step using loaded instruction
                    0xD2, outHigh(eearh, 31), outLow(eearh, 31), 0x23,  // out EEARH,r31  EEARH = ah  EEPROM Address MSB
                    0xD2, outHigh(eearl, 30), outLow(eearl, 30), 0x23,  // out EEARL,r30  EEARL = al  EEPROMad Address LSB
                    0xD2, outHigh(eecr, 28), outLow(eecr, 28), 0x23,    // out EECR,r28   EERE = 01 (EEPROM Read Enable)
                    0xD2, inHigh(eedr, 29), inLow(eedr, 29), 0x23,      // in  r29,EEDR   Read data from EEDR
                    0xD2, outHigh(dwdr, 29), outLow(dwdr, 29), 0x23};   // out DWDR,r29   Send data back via DWDR reg
  uint8_t r28 = readRegister(28);        // Save r28
  uint8_t r29 = readRegister(29);        // Save r29
  uint8_t r30 = readRegister(30);        // Save r30
  uint8_t r31 = readRegister(31);        // Save r31
  hlprs.sendCmd(setRegs, sizeof(setRegs));
  hlprs.sendCmd(doRead, sizeof(doRead));
  hlprs.getResponse(1);                                                       // Read data from EEPROM location
  writeRegister(28, r28);             // Restore r28
  writeRegister(29, r29);             // Restore r29
  writeRegister(30, r30);             // Restore r30
  writeRegister(31, r31);             // Restore r31
  return hlprs.buf[0];
}

//   
//   Write one byte to EEPROM
//   
// used in Debugger only
void MemoryHandler::writeEepromByte (uint16_t addr, uint8_t val) {
  uint8_t r28 = readRegister(28);        // Save r28
  uint8_t r29 = readRegister(29);        // Save r29 
  uint8_t r30 = readRegister(30);        // Save r30
  uint8_t r31 = readRegister(31);        // Save r31
  uint8_t setRegs[] = {0x66,                                                 // Set up for read/write using repeating simulated instructions
                    0xD0, 0x00, 0x1C,                                     // Set Start Reg number (r30)
                    0xD1, 0x00, 0x20,                                     // Set End Reg number (r31) + 1
                    0xC2, 0x05,                                           // Set repeating copy to registers via DWDR
                    0x20,                                                 // Go
                    0x04, 0x02, addr & 0xFF, addr >> 8};                  // Data written into registers r28-r31
  uint8_t doWrite[] = {0x64,                                                 // Set up for single step using loaded instruction
                    0xD2, outHigh(eearh, 31), outLow(eearh, 31), 0x23,    // out EEARH,r31  EEARH = ah  EEPROM Address MSB
                    0xD2, outHigh(eearl, 30), outLow(eearl, 30), 0x23,    // out EEARL,r30  EEARL = al  EEPROM Address LSB
                    0xD2, inHigh(dwdr, 30), inLow(dwdr, 30), 0x23,        // in  r30,DWDR   Get data to write via DWDR
                    val,                                                  // Data written to EEPROM location
                    0xD2, outHigh(eedr, 30), outLow(eedr, 30), 0x23,      // out EEDR,r30   EEDR = data
                    0xD2, outHigh(eecr, 28), outLow(eecr, 28), 0x23,      // out EECR,r28   EECR = 04 (EEPROM Master Program Enable)
                    0xD2, outHigh(eecr, 29), outLow(eecr, 29), 0x23};     // out EECR,r29   EECR = 02 (EEPROM Program Enable)
  hlprs.sendCmd(setRegs, sizeof(setRegs));
  hlprs.sendCmd(doWrite, sizeof(doWrite));
  writeRegister(28, r28);             // Restore r28
  writeRegister(29, r29);             // Restore r29
  writeRegister(30, r30);             // Restore r30
  writeRegister(31, r31);             // Restore r31
}

//
//  Read 128 bytes from flash memory area at <addr> into data[] buffer
//
// used in Helper and Debugger
int16_t MemoryHandler::readFlashPage (char *data, uint16_t len, uint16_t addr) {
  uint8_t r28 = readRegister(28);        // Save r28 (Y Reg low)
  uint8_t r29 = readRegister(29);        // Save r29 (Y Reg high)
  uint8_t r30 = readRegister(30);        // Save r30 (Z Reg low)
  uint8_t r31 = readRegister(31);        // Save r31 (Z Reg high)
  // Read sizeof(flashBuf) bytes form flash page at <addr>
  uint8_t rsp;
  hlprs.reportTimeout = false;
  uint16_t lenx2 = len * 2;
  for (uint8_t ii = 0; ii < 4; ii++) {
    uint8_t rdFlash[] = {0x66,                                               // Set up for read/write using repeating simulated instructions
                      0xD0, 0x00, 0x1E,                                   // Set Start Reg number (r30)
                      0xD1, 0x00, 0x20,                                   // Set End Reg number (r31) + 1
                      0xC2, 0x05,                                         // Set repeating copy to registers via DWDR
                      0x20,                                               // Go
                      addr & 0xFF, addr >> 8,                             // r31:r30 (Z) = addr
                      0xD0, 0x00, 0x00,                                   // Set start = 0
                      0xD1, lenx2 >> 8,lenx2,                             // Set end = repeat count = sizeof(flashBuf) * 2
                      0xC2, 0x02,                                         // Set simulated "lpm r?,Z+; out DWDR,r?" instructions
                      0x20};                                              // Go
    hlprs.sendCmd(rdFlash, sizeof(rdFlash));
    rsp = hlprs.getResponse(data, len);                                         // Read len bytes
     if (rsp ==len) {
      break;
    } else {
      // Wait and retry read
      delay(5);
    }
  }
  hlprs.reportTimeout = true;
  hlprs.flashStart = addr;
  writeRegister(28, r28);             // Restore r28
  writeRegister(29, r29);             // Restore r29
  writeRegister(30, r30);             // Restore r30
  writeRegister(31, r31);             // Restore r31
  return rsp;
}

// ========================================
// no private methods!!!
// ----------------------------------------

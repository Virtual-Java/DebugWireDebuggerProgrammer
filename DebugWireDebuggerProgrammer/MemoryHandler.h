// MemoryHandler.h

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

#ifndef __MEMORYHANDLER_H__
#define __MEMORYHANDLER_H__

#include <Arduino.h>
#include "Helpers.h"

// Build high byte of opcode for "out addr, reg" instruction
class MemoryHandler {
  private:

  public:

    // used in Debugger and MemoryHandler
    uint8_t          eecr;                   // EEPROM Control Register
    // used in Debugger and MemoryHandler
    uint8_t          eedr;                   // EEPROM Data Register
    // used in Debugger and MemoryHandler
    uint8_t          eearl;                  // EEPROM Address Register (low byte)
    // used in Debugger and MemoryHandler
    uint8_t          eearh;                  // EEPROM Address Register (high byte)
    // used in Debugger and MemoryHandler
    uint8_t          dwdr;                   // I/O addr of DWDR Register


    MemoryHandler (); // Constructor
    
    uint8_t outHigh (uint8_t add, uint8_t reg);                    // used in MemoryHandler and Debugger
    
    // Build low byte of opcode for "out addr, reg" instruction
    uint8_t outLow (uint8_t add, uint8_t reg);                     // used in MemoryHandler and Debugger
    
    // Build high byte of opcode for "in reg,addr" instruction
    uint8_t inHigh  (uint8_t add, uint8_t reg);                    // used in MemoryHandler and Debugger
    
    // Build low byte of opcode for "in reg,addr" instruction
    uint8_t inLow  (uint8_t add, uint8_t reg);                     // used in MemoryHandler and Debugger
    
    // Set register <reg> by building and executing an "out <reg>,DWDR" instruction via the CMD_SET_INSTR register
    void writeRegister (uint8_t reg, uint8_t val);              // used in MemoryHandler and Debugger
    
    // Read register <reg> by building and executing an "out DWDR,<reg>" instruction via the CMD_SET_INSTR register
    uint8_t readRegister (uint8_t reg);                         // used in MemoryHandler, DisAssembler and Debugger
    
    // Read registers 0-31 into buf[]
    boolean  readAllRegisters ();                         // used in Debugger only
    
    // Set or clear bit in I/O address 0x00 - 0x1F by generating and executing an "sbi" or "cbi" instruction
    void setClrIOBit (uint8_t addr, uint8_t bit, boolean set);  // used in Debugger only
    
    // Write one byte to SRAM address space using an SRAM-based value for <addr>, not an I/O address
    void writeSRamByte (uint16_t addr, uint8_t val);     // used in Debugger only
    
    // Read one byte from SRAM address space using an SRAM-based value for <addr>, not an I/O address
    uint8_t readSRamByte (uint16_t addr);                // used in Debugger and DisAssembler
    
    // Read <len> bytes from SRAM address space into buf[] using an SRAM-based value for <addr>, not an I/O address
    // Note: can't read addresses that correspond to  r28-31 (Y & Z Regs) because Z is used for transfer (not sure why Y is clobbered) 
    boolean readSRamBytes (uint16_t addr, uint8_t len);  // used in Debugger only



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
    
    uint8_t readEepromByte (uint16_t addr); // used in Debugger only
    
    //   
    //   Write one byte to EEPROM
    //   
    
    void writeEepromByte (uint16_t addr, uint8_t val); // used in Debugger only
    
    //
    //  Read 128 bytes from flash memory area at <addr> into data[] buffer
    //
    int16_t readFlashPage (char *data, uint16_t len, uint16_t addr); // used in Helper and Debugger
};
#endif

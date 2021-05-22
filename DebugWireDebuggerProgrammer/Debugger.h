// Debugger.h

//  The debugWire Protocol
//
//  To use the debugWire protocol you must first enable debugWire Mode by using the In-System commands to 
//  clear the DWEN (DebugWIRE enabled) Fues.  On the ATTiny85, this means clearing bit 6 of the High Fuse
//  byte and cycling Vcc so the new fuese setting can take effect.  Note: once DWEN is enabled In-Syetem
//  Programming is no longer accessble via the conventional way (pulling RESET low) because debugWire is
//  now using the RESET pin.  Howwver, you can temporarily disable debugWire by sending a 0x06 byte on the
//  RESET line, then taking RESET LOW to enter in-System Programming Mode and clearing DWEN by setting bit
//  6 of the High Fuse byte.  This can be down with the following sequence of text commands:
//
//    1.  EXIT    Send disable debugWire command
//    2.  -       Disable DWEN bit (bit 6) in High Fuses byte on Tiny25/45/85
//    3.  F       Verify DWEN bit is set
//
//  IMPORTANT - You cannot program the chip with a programmer like the AVRISP mkII while the debugWire fuse
//  is enabled.
//  
//  DebugWire Command bytes
//    Basic commands
//      BREAK   Halts Target with a Break (Return 0xFF if stopped)
//      0x06    Disable debugWire (returns nothing)
//      0x07    Reset Target (returns 0x00 0x5 after 60 ms delay)
//      0x20    Starts Repeating Instructions (for example, 0xC2 0x01)
//      0x21    Steps Repeating Instruction once
//      0x23    Single Step Instruction Set by CMD_SET_INSTR
//      0x30    Start Normal Execution of Code (return nothing)
//      0x31    Single Step on target at PC (returns 0x00 0x55, where 0x00 comes right after 0x32; 0x55 after a delay)
//      0x32    GoStart Normal Execution using Instruction Set by CMD_SET_INSTR
//      0x33    Single Step Instruction (returns 0x00 0x55, where 0x00 comes right after 0x33; 0x55 after a delay)
//    Set Context
//      0x60    Set GO context  (No bp?)
//      0x61    Set run to cursor context (Run to hardware BP?)
//      0x63    Set step out context (Run to return instruction?)
//      0x64    Set up for single step using loaded instruction
//      0x66    Set up for read/write using repeating simulated instructions
//      0x79    Set step-in / autostep context or when resuming a sw bp (Execute a single instruction?)
//      0x7A    Set single step context
//    Read Control Register
//      0xF0      Returns PC as two bytes (MSB LSB)
//      0xF1      Returns HW Breakpoint Reg as two bytes (MSB LSB)
//      0xF2      Returns Current Instruction as two bytes (MSB LSB)
//      0xF3      Returns Device Signature as two bytes (MSB LSB)
//    Write Control Register
//      0xD0      Sets PC to following two bytes (MSB LSB)
//      0xD1      Sets HW Breakpoint Reg to following two bytes (MSB LSB)
//      0xD2      Set following two bytes as Instruction for CMD_SS_INSTR
//      0xD3      Not Used (can't write signature)
//    Results
//      0x00 0x55 Response to Asyn Serial BREAK and OK response for some commands
//
//    Two byte sequences for Hard-coded operations
//    Note: I'm still trying to figure out the details on how these sequences work...
//      0xC2 0x01   out DWDR,r0   out DWDR,r1,...   Start register read
//      0xC2 0x05   in r0,DWDR    in r0,DWDR,...    Start register write
//      0xC2 0x00   ld  r16,Z+    out DWDR,r16      Start data area read
//      0xC2 0x04   in r16,DWDR   st Z+,r16         Start data area write
//    Others...
//      0xC2 0x02   lpm r?,Z+     out DWDR,r?       ??
//      0xC2 0x03   lpm r?,Z+     out SWDR,r?       Flash Read??
//      0xC2 0x04   in r16,DWDR   st Z+,r16         Set data area write mode
//  
//    Send sequence of bytes using debugWire technique on RESET pin, for example:
//      CMD=66D00000D10020C20120    Read Registers 0 - 31
//      CMD=66D00000D10001C20521AA  Write 0xAA to Register 0
//
//    Dump 128 byte of Flash from Address 0x0000
//      CMD=66D0001ED10020C205200000D00000C202D1010020 (breaks out, as follows)
//        66
//        D0 00 1E    // PC = 001E
//        D1 00 20    // HWBP = 0020
//        C2 05       // Write Registers
//        20          // Go read/write
//        ll hh    
//        D0 00 00    // PC = 0000
//        C2 02       // Read Flash
//        D1 01 00    // HWBP = 0100
//        20          // Go read/write
//          
//    Example AVR Instructions for INST= and STEP commands
//        0000  NOP
//        9403  INC r0          1010 010d dddd 0011
//        9413  INC r1
//        9503  INC r16
//        940A  DEC r0          1010 010d dddd 1010
//        941A  DEC r1
//        E000  LDI, r16,0x00   1110 KKKK dddd KKKK (only regs 16-31)
//        E50A  LDI, r16,0x5A   1110 KKKK dddd KKKK
//        9598  BREAK
//
//    ATTiny85 Registers  I/O    SRAM
//        PORTB           0x18   0x38
//        DDRB            0017   0x37
//        PINB            0x16   0x36
//
//    Location of the DWDR Register (varies by processor)  ATTiny25/45/85 value verified, others from references
//        0x2E        ATTiny13
//        0x1F        ATTiny2313
//        0x22        ATTiny25/45/85
//        0x27        ATTiny24/44/84,441,841
//        0x31        ATmega48A,48PA,88A,88PA,8U2,168A,168PA,16U2,328P,328,32U2
//
//    Location of the DWEN fuse and fuse bit (varies by processor)
//        Fuse  Bit
//        High  0x04  ATTiny13/A
//        High  0x80  ATTiny2313/A
//        High  0x40  ATTiny25,45,85
//        High  0x40  ATTiny24/44/84,441,841
//        High  0x40  ATmega48A,48PA,88A,88PA,8U2,168A,168PA,16U2,328P,328,32U2
//
//    Location of fuse bits controlling clock speed
//        Fuse  Bits  (  R = divide by 8, 3:0 select clock, 0=ext, 1=
//        Low   ---R--10  ATTiny13
//        Low   R---3210  ATTiny2313
//        Low   R---3210  ATTiny25/45/85
//        Low   R---3210  ATTiny24/44/84
//        Low   R---3210  ATmega48A, etc. 
//
//    ATMega328P Fuse Settiings with Standard Arduino Bootloader - Low: FF, High: D8, Extd: FD
//    ATMega328P Fuse Settiings with Optiboot Bootloader -         Low: F7, High: DE, Extd: FD
//
//    Low byte:
//      7 = 1   Divide clock by 8
//      6 = 1   Clock output
//      5 = 1   Select start-up time bit 1
//      4 = 1   Select start-up time bit 0
//      3 = 1   Select Clock source bit 3 (F = Low Power Crystal Oscillator, 8-16 MHz range)
//      2 = 1   Select Clock source bit 2
//      1 = 1   Select Clock source bit 1
//      0 = 1   Select Clock source bit 0
//
//    High byte:
//      7 = 1   External Reset Disable
//      6 = 1   debugWIRE Enable
//      5 = 0   Enable Serial Program and Data Downloading
//      4 = 1   Watchdog Timer Always On
//      3 = 1   EEPROM memory is preserved through the Chip Erase
//      2 = 0   Select Boot Size bit 1 (see below)
//      1 = 0   Select Boot Size bit 0
//      0 = 0   Select Reset Vector (if enabled, sets IVSEL in MCUCR on RESET which moves int vectors to boot area)
//
//    MCUCR 0x35 (0x55) - IVSEL is bit 1
//
//    Bootloader Size:
//      0 = 0x3800 - 0x3FFF (2048 words)
//      1 = 0x3C00 - 0x3FFF (1024 words)
//      2 = 0x3E00 - 0x3FFF (512 words)
//      3 = 0x3F00 - 0x3FFF (256 words)
//
//    Extd byte:
//      7 = 1   Not used
//      6 = 1   Not used
//      5 = 1   Not used
//      4 = 1   Not used
//      3 = 1   Not used
//      2 = 1   Brown-out Detector trigger level bit 2
//      1 = 0   Brown-out Detector trigger level bit 1
//      0 = 1   Brown-out Detector trigger level bit 0

#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "OnePinSerial.h"
#include "PrintFunctions.h"
#include "Helpers.h"
#include "MemoryHandler.h"   // includes Helpers indirectly?!
#include "DisAssembler.h"
#include "InSystemProgrammer.h"


class Debugger {
  public:

    // Constructor
    Debugger ();
    
//    void selectProgrammer ();                           // used in DWDP_OOP.ino only
    void selectDebugger ();                             // used in DWDP_OOP.ino only
    void debugger ();                                   // used in DWDP.ino (loop) only

  private:
    // used in Debugger only
    boolean       progMode = false;
    // used in Debugger only
    boolean       debugWireOn = false;
    // used in Debugger only
    boolean       hasDeviceInfo = false;  // True if Device Info has been read (dwdr, dwen, ckdiv8)
    // used in Debugger only
    uint8_t          dwen;                   // DWEN bit mask in High Fuse byte
    // used in Debugger only
    uint8_t          ckdiv8;                 // CKDIV8 bit mask in Low Fuse BYte
    // used in Debugger only
    uint16_t  ramBase;                // Base address of SRAM
    // used in Debugger only
    uint16_t  ramSize;                // SRAM size in bytes
    // used in Debugger only
    uint16_t  eeSize;                 // EEPROM size in bytes
//    // used in Debugger and MemoryHandler
//    uint8_t          eecr;                   // EEPROM Control Register
//    // used in Debugger and MemoryHandler
//    uint8_t          eedr;                   // EEPROM Data Register
//    // used in Debugger and MemoryHandler
//    uint8_t          eearl;                  // EEPROM Address Register (low byte)
//    // used in Debugger and MemoryHandler
//    uint8_t          eearh;                  // EEPROM Address Register (high byte)
    // used in Debugger only
    uint16_t  flashSize;              // Flash Size in Bytes
    // used in Debugger only
    uint16_t  pcSave;                 // Used by single STEP and RUN
    // used in Debugger only
    uint16_t  bpSave;                 // Used by RUN
    // used in Debugger only
    boolean       breakWatch = false;     // Used by RUN commands


    void powerOn ();                                    // used in Debugger only
    void powerOff ();                                   // used in Debugger only
    boolean enterProgramMode ();                        // used in Debugger only
    // Read two byte chip signature into buf[]
    void getChipId ();                                  // used in Debugger only
    uint16_t identifyDevice ();                     // used in Debugger only
    void busyWait ();                                   // used in Debugger only
    void printPartFromId (uint8_t sig1, uint8_t sig2);        // used in Debugger only
    void sendBreak ();                                  // used in Debugger only
    boolean doBreak ();                                 // used in Debugger only
    void printMenu ();                                  // used in debugger only
};
#endif

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

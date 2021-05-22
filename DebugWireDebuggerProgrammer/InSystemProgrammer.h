// InSystemProgrammer.h

//
//  In-System Programmer (adapted from ArduinoISP sketch by Randall Bohn)
//
//  My changes to Mr Bohn's original code include removing and renaming functions that were redundant with
//  my code, converting other functions to cases in a master switch() statement, removing code that served
//  no purpose, and making some structural changes to reduce the size of the code.
//  
//  Portions of the code after this point were originally Copyright (c) 2008-2011 Randall Bohn
//    If you require a license, see:  http://www.opensource.org/licenses/bsd-license.php
//

//  This code supports experimentation with AVR In-System Programming Protocol and the debugWire protocol.
//  The In-System programming protocol uses the SPI interface and the RESET pin while debugWire requires
//  only the RESET pin to operate.  As currently configured, this code is designed to program and control
//  an ATTiny85, but can be reconfigured to control other AVR-Series 8 Bit Microcontrollers.  The code starts
//  up in In-System Programming mode, which has only a few single letter commands:
//
//    e            Erase Flash and EEPROM Memory
//    cxxxxxxxx   Send arbitrary 4 byte (xxxxxxxx) command sequence
//    i           Identify part type
//    f           Print current fuse settings for chip
//    +           Enable debugWire Fuse
//    -           Disable debugWire Fuse
//    p           Turn on power to chip (used to run code)
//    o           Switch off power to chip
//    b           Cycle Vcc and Send BREAK to engage debugWire Mode (debugWire Fuse must be enabled) 
//
//  Note: case does not matter, so typing f, or F is equivalent.
//
//  The In-System Programming Protocol uses a series of 4 byte commands which can be sent to the target using
//  the target's SPI interface while the RESET pin is active (LOW).  However, the first 4 byte command sent must
//  be the "Program Enable" command 0xAC 0x53 0xXX 0xYY, where the values of 0xXX and 0xYY do not matter.  This
//  command puts the chip into programming mode and echos back 0xZZ 0xAC 0x53 0xXX.  You can verify that the chip
//  is in programming mode by sending the command to read the chip's Vendor Code by sending 0x30 0x00 0x00 0x00.
//  This should echo back something like 0xYY 0x30 0xNN 0x1E, where 0x1E is the device code indicating an ATMEL
//  AVR-series microcontroller.  See the function enterProgramMode() for an example.  
//
//  Other 4 byte commands include:
//    Send                  Receive
//    0x30 0x00 0xNN 0x00   0xYY 0x30 0xNN 0xVV   Read mem address 0xNN and return value as 0xVV
//    0x30 0x00 0x00 0x00   0xYY 0x30 0xNN 0xVV   Read Vendor Code byte and return value as 0xVV 
//    0x30 0x00 0x01 0x00   0xYY 0x30 0xNN 0xVV   Read Part Family and Flash Size byte and return value as 0xVV
//    0x30 0x00 0x02 0x00   0xYY 0x30 0xNN 0xVV   Read Part Number byte and return value as 0xVV
//  
//  So, send a 4 byte sequence and get back a one byte response.  This code uses the function ispSend()
//  to make this simpler, such as;
//    ispSend()             Returns
//    0xF0 0x00 0x00 0x00   Polls the target's RDY/~BSY bit and returns 0xFF when busy, else 0xFE (doc error)
//    0x50 0x00 0x00 0x00   Read Low Fuses byte
//    0x58 0x08 0x00 0x00   Read High Fuses byte
//    0x50 0x08 0x00 0x00   Read Extended Fuses byte
//    0x58 0x00 0x00 0x00   Read Lock Bits
//    0xA0 0x00 <ad> 0x00   Reads byte from EEPROM at (<ad> & 0x3F) on selected page
//    0x28 <ms> <ls> 0x00   Reads High Byte from Program Memory <ms><ls>
//    0x20 <ms> <ls> 0x00   Reads Low Byte from Program Memory <ms><ls>
//  
//  You can use the cxxxxxxxx command to experiment with sending 4 byte commands while in the In-System Programming
//  mode.  For simplicity, the 'c' command only prints out the last byte of the 4 byte response.  So, for example,
//  to send the command to return the chip's device Id, type in the following and press Send:
//
//    c30000000
//
//  And the following will echo back:
//
//    Cmd: 30 00 00 00  = 1E
//
//  Similar commands support writing to Fuse bytes, etc:
//    ispSend()             Effect
//    0xAC 0xA0 0x00 <val>  Writes <val> into ATTiny85 Low Fuses byte
//    0xAC 0xA8 0x00 <val>  Writes <val> into ATTiny85 High Fuses byte
//    0xAC 0xA4 0x00 <val>  Writes <val> into ATTiny85 Extended Fuses byte
//    0xAC 0xE0 0x00 <val>  Writes ATTiny85 Lock Bits
//    0xC1 0x00 <pg> 0x00   Sets selected EEPROM Page to (<pg> & 0x03)
//    0xC0 0x00 <add> <val> Writes <val> to EEPROM address (<add> & 0x3F) on selected Page
//
//  IMPORTANT: Be very careful when changing the value of fuse bytes, as some combinations will put the chip in
//  a state where you can no longer program or communicate with it.  This can be especially tricky because the
//  enabled fuses are typically such that setting a bit to '1' disables the fuse and '0' enables it.
//
//  ATTiny series signature word and default fuses settings (see also printPartFromId())
//    ATTINY13   0x9007  // L: 0x6A, H: 0xFF              8 pin
//    ATTINY24   0x910B  // L: 0x62, H: 0xDF, E: 0xFF    14 pin
//    ATTINY44   0x9207  // L: 0x62, H: 0xDF, E: 0xFF    14 pin
//    ATTINY84   0x930C  // L: 0x62, H: 0xDF, E: 0xFF    14 pin
//    ATTINY25   0x9108  // L: 0x62, H: 0xDF, E: 0xFF     8 pin
//    ATTINY45   0x9206  // L: 0x62, H: 0xDF, E: 0xFF     8 pin
//    ATTINY85   0x930B  // L: 0x62, H: 0xDF, E: 0xFF     8 pin
//
//  Write to Flash Memory (Must first erase FLASH to all 0xFF values)
//
//  ispSend()             Effect
//  0xAC 0x80 0x00 0x00   Erase all Flash Program Memory and EEPROM Contents
//                        Note: wait nn ms then release RESET to end Erase Cycle
//  0x60 <ms> <ls> <val>  Writes <val> to Low Byte of Flash Address <ms><ls>  (must write LSB first)
//  0x68 <ms> <ls> <val>  Writes <val> to High Byte of Flash Address <ms><ls> (poll RDY
//

#ifndef __INSYSTEMPROGRAMMER_H__
#define __INSYSTEMPROGRAMMER_H__

#include <Arduino.h>
//#include "CommandDispatcher.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "OnePinSerial.h"

/*
// STK Definitions
#define STK_OK      0x10    // DLE
#define STK_FAILED  0x11    // DC1
#define STK_UNKNOWN 0x12    // DC2
#define STK_INSYNC  0x14    // DC4
#define STK_NOSYNC  0x15    // NAK
#define CRC_EOP     0x20    // Ok, it is a space...

// Code definitions
#define EECHUNK   (32)
*/
class InSystemProgrammer {
  public:
    // Constructor
    InSystemProgrammer ();
    
    void programmer (void);       // used in DWDP_OOP.ino only
    
    void selectProgrammer ();     // used in DWDP_OOP.ino only
  
//    uint8_t getch ();             // used in InSystemProgrammer only
//    
//    void fill (int16_t n);            // used in InSystemProgrammer only
//    
//    void empty_reply ();          // used in InSystemProgrammer only
//    
//    void breply (uint8_t b);      // used in InSystemProgrammer only
  
    void enableSpiPins ();                              // used in Debugger and InSystemProgrammer
    
    void disableSpiPins ();                             // used in Debugger and InSystemProgrammer
    
    uint8_t ispSend (uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4);  // used in Debugger and InSystemProgrammer
  
    uint8_t transfer (uint8_t val);                           // used in InSystemProgrammer only
  
//    void avrisp ();               // used in InSystemProgrammer only
    //// Command Dispatcher.h
    
    ////////////////////////////////////
    //      Command Dispatcher
    ////////////////////////////////////
    
    #include <Arduino.h>
    
    // Code definitions
    #define EECHUNK   (32)
    
    //class CommandDispatcher {
    //  private:
    //  // uint8_t ch;
    //  // uint8_t ch;
    //  public:
    //    CommandDispatcher ();
    //    void avrisp ();
    //};
    
  private:
    // STK Definitions
    const uint8_t STK_OK      = 0x10;    // DLE
    const uint8_t STK_FAILED  = 0x11;    // DC1
    const uint8_t STK_UNKNOWN = 0x12;    // DC2
    const uint8_t STK_INSYNC  = 0x14;    // DC4
    const uint8_t STK_NOSYNC  = 0x15;    // NAK
    const uint8_t CRC_EOP     = 0x20;    // Ok, it is a space...
  
    // Variables used by In-System Programmer code
    
    uint16_t      pagesize;
    uint16_t      eepromsize;
    bool          rst_active_high;
    int16_t       pmode = 0;
    uint16_t      here;           // Address for reading and writing, set by 'U' command
    uint16_t      hMask;          // Pagesize mask for 'here" address

    uint8_t getch ();             // used in InSystemProgrammer only
    void fill (int16_t n);            // used in InSystemProgrammer only
    void empty_reply ();          // used in InSystemProgrammer only
    void breply (uint8_t b);      // used in InSystemProgrammer only
    void avrisp ();               // used in InSystemProgrammer only
};

#endif

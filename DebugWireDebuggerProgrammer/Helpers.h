// Helpers.h

//  ATTiny85 Pinout
//
//                +-\/-+
//  RESET / PB5 1 |    | 8 Vcc
//   CLKI / PB3 2 |    | 7 PB2 / SCK
//   CLKO / PB4 3 |    | 6 PB1 / MISO
//          Gnd 4 |    | 5 PB0 / MOSI
//                +----+
//  
//  The following sources provided invaluable information:
//    http://www.ruemohr.org/docs/debugwire.html
//    https://github.com/dcwbrown/dwire-debug
//    Google "Understanding debugWIRE and the DWEN Fuse"
//    https://hackaday.io/project/20629-debugwire-debugger
//    https://github.com/jbtronics/WireDebugger/releases
//


#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <Arduino.h>
#include "PrintFunctions.h"
#include "MemoryHandler.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "OnePinSerial.h"


#define DEVELOPER 0

#define PMODE    8    // Input - HIGH = Program Mode, LOW = Debug Mode
#define VCC      9    // Target Pin 8 - Vcc
#define RESET   10    // Target Pin 1 - RESET, or SCI
#define MOSI    11    // Target Pin 5 - MOSI,  or SDI
#define MISO    12    // Target Pin 6 - MISO,  or SII
#define SCK     13    // Target Pin 7 - SCK,   or SDO

// Alternate Pin Definitions for High Voltage Programmer (for future expansion)
#define SCI     10    // Target Pin 2 - SCI
#define SII     11    // Target Pin 6 - SII
#define SDI     12    // Target Pin 5 - SDI
#define SDO     13    // Target Pin 7 - SDO

#define DEBUG_BAUD    115200
#define PROGRAM_BAUD  19200

#define DWIRE_RATE    (1000000 / 128) // Set default baud rate (1 MHz / 128) Note: the 'b' command can change this

//    boolean       debugWireOn = false;
//    uint8_t          buf[256];               // Command and data buffer (also used by programmer)
//    uint8_t          flashBuf[64];           // Flash read buffer for Diasm and FXxxxx and FBxxxx commands
//    uint16_t  flashStart;             // Base address of data in flashBuf[]
//    boolean       flashLoaded = false;    // True if flashBuf[] has valid data
//    boolean       hasDeviceInfo = false;  // True if Device Info has been read (dwdr, dwen, ckdiv8)
//    char          rpt[16];                // Repeat command buffer
//    uint8_t          dwdr;                   // I/O addr of DWDR Register
//    uint8_t          dwen;                   // DWEN bit mask in High Fuse byte
//    uint8_t          ckdiv8;                 // CKDIV8 bit mask in Low Fuse BYte
//    uint16_t  ramBase;                // Base address of SRAM
//    uint16_t  ramSize;                // SRAM size in bytes
//    uint16_t  eeSize;                 // EEPROM size in bytes
//    uint8_t          eecr;                   // EEPROM Control Register
//    uint8_t          eedr;                   // EEPROM Data Register
//    uint8_t          eearl;                  // EEPROM Address Register (low byte)
//    uint8_t          eearh;                  // EEPROM Address Register (high byte)
//    uint16_t  flashSize;              // Flash Size in Bytes
//    uint16_t  pcSave;                 // Used by single STEP and RUN
//    uint16_t  bpSave;                 // Used by RUN
//    char          cursor = 0;             // Output line cursor used by disassembler to tab
//    boolean       breakWatch = false;     // Used by RUN commands
//    uint16_t  timeOutDelay;           // Timeout delay (based on baud rate)
//    boolean       runMode;                // If HIGH Debug Mode, else Program
//    boolean       reportTimeout = true;   // If true, report read timeout errors

//class PrintFunctions; // forward declaration
class MemoryHandler; // forward declaration

class Helpers {
  public:

    // used in Debugger, Helpers, InSystemProgrammer and MemoryHandler
    uint8_t          buf[256];               // Command and data buffer (also used by programmer)
    // used in Debugger and Helpers
    uint8_t          flashBuf[64];           // Flash read buffer for Diasm and FXxxxx and FBxxxx commands
    // used in Debugger and Helpers
    uint16_t  flashStart;             // Base address of data in flashBuf[]
    // used in Debugger and Helpers
    boolean       flashLoaded = false;    // True if flashBuf[] has valid data
    // used in Debugger and Helpers
    char          rpt[16];                // Repeat command buffer
    // used in Helpers and MemoryHandler
    boolean       reportTimeout = true;   // If true, report read timeout errors


    Helpers::Helpers(); // Constructor

    uint16_t convertHex (uint8_t idx);                 // used in Helpers and Debugger only
    uint8_t after (char cc);                               // used in Helpers and Debugger only
    
    // Read hex characters in buf[] starting at <off> and convert to bytes in buf[] starting at index 0
    uint8_t convertToHex (uint8_t off);                       // used in Helpers and Debugger only
    uint8_t readDecimal (uint8_t off);                        // used in Helpers and Debugger only
    // Get '\n'-terminated string and return length
    int16_t getString ();                                   // used in Helpers and Debugger only
    void printBufToHex8 (int16_t count, boolean trailCrLF); // used in Helpers and Debugger only
    boolean bufMatches (const __FlashStringHelper* ref); // used in Helpers and Debugger only
    boolean bufStartsWith (const __FlashStringHelper* ref); // used in Helpers and Debugger only
    void sendCmd (byte* data, uint8_t count);            // used in Helpers and Debugger only
    uint8_t getResponse (int16_t expected);                  // used in Helpers, Debugger and in MemoryHandler too
    uint8_t getResponse (uint8_t *data, int16_t expected);      // used in Helpers, Debugger and in MemoryHandler too
    void setTimeoutDelay (uint16_t rate);         // used in Helpers and Debugger only
    uint16_t getWordResponse ();                  // used in Helpers only
    boolean checkCmdOk ();                            // used in Helpers and Debugger only
    boolean checkCmdOk2 ();                           // used in Helpers and Debugger only
    void printCommErr (uint8_t read, uint8_t expected);     // used in Debugger only
    uint16_t getFlashWord (uint16_t addr);    // used in Debugger and DisAssembler
    void printCmd (const __FlashStringHelper* cmd);   // used in Debugger only
    void printCmd ();                                 // used in Debugger and DisAssembler
    void echoCmd ();                                  // used in Debugger only
    void echoSetCmd (uint8_t len);                       // used in Debugger only
    uint16_t getPc ();                            // used in Debugger only
    uint16_t getBp ();                            // used in Debugger only
    uint16_t getOpcode ();                        // not used anywhere
    void setPc (uint16_t pc);                     // used in Debugger only
    void setBp (uint16_t bp);                     // used in Debugger only
    void printDebugCommands ();                       // used in Debugger only
    void setRepeatCmd (const __FlashStringHelper* cmd, uint16_t addr); // used in Debugger only
    
  private:

    // used in Helpers only
    uint16_t  timeOutDelay;           // Timeout delay (based on baud rate)

    uint8_t toHex (char cc);                               // used in Helpers only
    char toHexDigit (uint8_t nib);                         // used in Helpers only
    boolean isDecDigit (char cc);                       // used in Helpers only
    boolean HexDigit (char cc);                         // used in Helpers only
//    uint16_t getWordResponse ();                    // used in Helpers only
};

#endif

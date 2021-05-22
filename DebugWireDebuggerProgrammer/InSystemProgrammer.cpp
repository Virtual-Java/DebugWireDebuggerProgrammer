// InSystemProgrammer.cpp

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

#include "InSystemProgrammer.h"
#include "Helpers.h"

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

// Variables used by In-System Programmer code

uint16_t      pagesize;
uint16_t      eepromsize;
bool          rst_active_high;
int16_t           pmode = 0;
uint16_t  here;           // Address for reading and writing, set by 'U' command
uint16_t  hMask;          // Pagesize mask for 'here" address
*/
#define RESET  10

//CommandDispatcher  cmdd;
extern Helpers  hlprs;
extern OnePinSerial  debugWire;

// ========================================
// public methods:
// ----------------------------------------

// Constructor:
InSystemProgrammer::InSystemProgrammer () {
  
}

// used in InSystemProgrammer only
uint8_t InSystemProgrammer::getch () {
  while (!Serial.available())
    ;
  return Serial.read();
}
// used in InSystemProgrammer only
void InSystemProgrammer::fill (int16_t n) {
  for (int16_t x = 0; x < n; x++) {
    hlprs.buf[x] = getch();
  }
}
// used in InSystemProgrammer only
void InSystemProgrammer::empty_reply () {
  if (CRC_EOP == getch()) {
    Serial.write(STK_INSYNC);
    Serial.write(STK_OK);
  } else {
    Serial.write(STK_NOSYNC);
  }
}
// used in InSystemProgrammer only
void InSystemProgrammer::breply (uint8_t b) {
  if (CRC_EOP == getch()) {
    Serial.write(STK_INSYNC);
    Serial.write(b);
    Serial.write(STK_OK);
  } else {
    Serial.write(STK_NOSYNC);
  }
}
// used in Debugger and InSystemProgrammer
void InSystemProgrammer::enableSpiPins () {
  digitalWrite(SCK, LOW);
  digitalWrite(MOSI, LOW);
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
}
// used in Debugger and InSystemProgrammer
void InSystemProgrammer::disableSpiPins () {
  pinMode(SCK, INPUT);
  digitalWrite(SCK, LOW);
  pinMode(MOSI, INPUT);
  digitalWrite(MOSI, LOW);
  pinMode(MISO, INPUT);
  digitalWrite(MISO, LOW);
}
// used in Debugger and InSystemProgrammer
uint8_t InSystemProgrammer::ispSend (uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4) {
  transfer(c1);
  transfer(c2);
  transfer(c3);
  return transfer(c4);
}
// used in Debugger only
uint8_t InSystemProgrammer::transfer (uint8_t val) {
  for (uint8_t ii = 0; ii < 8; ++ii) {
    digitalWrite(MOSI, (val & 0x80) ? HIGH : LOW);
    digitalWrite(SCK, HIGH);
    delayMicroseconds(4);
    val = (val << 1) + digitalRead(MISO);
    digitalWrite(SCK, LOW); // slow pulse
    delayMicroseconds(4);
  }
  return val;
}


// (Subclass:) // used in InSystemProgrammer only
////////////////////////////////////
//      Command Dispatcher
////////////////////////////////////

//#include "CommandDispatcher.h"
//#include "InSystemProgrammer.h"

//InSystemProgrammer isp;

//CommandDispatcher::CommandDispatcher() {
//  
//}
// (Subclass) used in InSystemProgrammer only
void InSystemProgrammer::avrisp () {
  //uint8_t ch = getch();
  uint8_t ch = getch();
  switch (ch) {
    case 0x30:                                  // '0' - 0x30 Get Synchronization (Sign On)
      empty_reply();
      break;
      
    case 0x31:                                  // '1' - 0x31 Check if Starterkit Present
      if (getch() == CRC_EOP) {
        Serial.write(STK_INSYNC);
        Serial.print(F("AVR ISP"));
        Serial.write(STK_OK);
      } else {
        Serial.write(STK_NOSYNC);
      }
      break;

    case 0x41:                                  // 'A' - 0x41 Get Parameter Value
      switch (getch()) {
        case 0x80:
          breply(0x02); // HWVER
          break;
       case 0x81:
          breply(0x01); // SWMAJ
          break;
        case 0x82:
          breply(0x12); // SWMIN
          break;
        case 0x93:
          breply('S');  // Serial programmer
          break;
        default:
          breply(0);
      }
      break;
      
    case 0x42:                                  // 'B' - 0x42 Set Device Programming Parameters
      fill(20);
      // AVR devices have active low reset, AT89Sx are active high
      rst_active_high = (hlprs.buf[0] >= 0xE0);
      pagesize   = (hlprs.buf[12] << 8) + hlprs.buf[13];
      // Setup page mask for 'here' address variable
      if (pagesize == 32) {
        hMask = 0xFFFFFFF0;
      } else if (pagesize == 64) {
        hMask = 0xFFFFFFE0;
      } else if (pagesize == 128) {
        hMask = 0xFFFFFFC0;
      } else if (pagesize == 256) {
        hMask = 0xFFFFFF80;
      } else {
        hMask = 0xFFFFFFFF;
      }
      eepromsize = (hlprs.buf[14] << 8) + hlprs.buf[15];
      empty_reply();
      break;
      
    case 0x45:                                  // 'E' - 0x45 Set Extended Device Programming Parameters (ignored)
      fill(5);
      empty_reply();
      break;
      
    case 0x50:                                  // 'P' - 0x50 Enter Program Mode
      if (!pmode) {
        delay(100);
        // Reset target before driving SCK or MOSI
        digitalWrite(RESET, rst_active_high ? HIGH : LOW);  // Reset Enabled
        pinMode(RESET, OUTPUT);
        enableSpiPins();
        // See AVR datasheets, chapter "Serial_PRG Programming Algorithm":
        // Pulse RESET after SCK is low:
        digitalWrite(SCK, LOW);
        delay(20); // discharge SCK, value arbitrarily chosen
        digitalWrite(RESET, !rst_active_high ? HIGH : LOW); // Reset Disabled
        // Pulse must be minimum 2 target CPU clock cycles so 100 usec is ok for CPU
        // speeds above 20 KHz
        delayMicroseconds(100);
        digitalWrite(RESET, rst_active_high ? HIGH : LOW);  // Reset Enabled
        // Send the enable programming command:
        delay(50); // datasheet: must be > 20 msec
        ispSend(0xAC, 0x53, 0x00, 0x00);
        pmode = 1;
      }
      empty_reply();
      break;
      
    case 0x51:                                  // 'Q' - 0x51 Leave Program Mode
      // We're about to take the target out of reset so configure SPI pins as input
      pinMode(MOSI, INPUT);
      pinMode(SCK, INPUT);
      digitalWrite(RESET, !rst_active_high ? HIGH : LOW); // Reset Disabled
      pinMode(RESET, INPUT);
      pmode = 0;
      empty_reply();
      break;

    case  0x55:                                 // 'U' - 0x55 Load Address (word)
      here = getch();
      here += 256 * getch();
      empty_reply();
      break;

    case 0x56:                                  // 'V' - 0x56 Universal Command
      fill(4);
      breply(ispSend(hlprs.buf[0], hlprs.buf[1], hlprs.buf[2], hlprs.buf[3]));
      break;

    case 0x60:                                  // '`' - 0x60 Program Flash Memory
      getch(); // low addr
      getch(); // high addr
      empty_reply();
      break;
      
    case 0x61:                                  // 'a' - 0x61 Program Data Memory
      getch(); // data
      empty_reply();
      break;

    case 0x64: {                                // 'd' - 0x64 Program Page (Flash or EEPROM)
      char result = STK_FAILED;
      uint16_t length = 256 * getch();
      length += getch();
      char memtype = getch();
      // flash memory @here, (length) bytes
      if (memtype == 'F') {
        fill(length);
        if (CRC_EOP == getch()) {
          Serial.write(STK_INSYNC);
          int16_t ii = 0;
          uint16_t page = here & hMask;
          while (ii < length) {
            if (page != (here & hMask)) {
              ispSend(0x4C, (page >> 8) & 0xFF, page & 0xFF, 0);  // commit(page);
              page = here & hMask;
            }
            ispSend(0x40 + 8 * LOW, here >> 8 & 0xFF, here & 0xFF, hlprs.buf[ii++]);
            ispSend(0x40 + 8 * HIGH, here >> 8 & 0xFF, here & 0xFF, hlprs.buf[ii++]);
            here++;
          }
          ispSend(0x4C, (page >> 8) & 0xFF, page & 0xFF, 0);      // commit(page);
          Serial.write(STK_OK);
          break;
        } else {
          Serial.write(STK_NOSYNC);
        }
        break;
      } else if (memtype == 'E') {
        // here is a word address, get the byte address
        uint16_t start = here * 2;
        uint16_t remaining = length;
        if (length > eepromsize) {
          result = STK_FAILED;
        } else {
          while (remaining > EECHUNK) {
            // write (length) bytes, (start) is a byte address
            // this writes byte-by-byte, page writing may be faster (4 bytes at a time)
            fill(length);
            for (uint16_t ii = 0; ii < EECHUNK; ii++) {
              uint16_t addr = start + ii;
              ispSend(0xC0, (addr >> 8) & 0xFF, addr & 0xFF, hlprs.buf[ii]);
              delay(45);
            }
            start += EECHUNK;
            remaining -= EECHUNK;
          }
          // write (length) bytes, (start) is a byte address
          // this writes byte-by-byte, page writing may be faster (4 bytes at a time)
          fill(length);
          for (uint16_t ii = 0; ii < remaining; ii++) {
            uint16_t addr = start + ii;
            ispSend(0xC0, (addr >> 8) & 0xFF, addr & 0xFF, hlprs.buf[ii]);
            delay(45);
          }
          result = STK_OK;
        }
        if (CRC_EOP == getch()) {
          Serial.write(STK_INSYNC);
          Serial.write(result);
        } else {
          Serial.write(STK_NOSYNC);
        }
        break;
      }
      Serial.write(STK_FAILED);
    } break;

    case 0x74: {                                // 't' - 0x74 Read Page (Flash or EEPROM)
      char result = STK_FAILED;
      int16_t length = 256 * getch();
      length += getch();
      char memtype = getch();
      if (CRC_EOP != getch()) {
        Serial.write(STK_NOSYNC);
        break;
      }
      Serial.write(STK_INSYNC);
      if (memtype == 'F') {
        for (int16_t ii = 0; ii < length; ii += 2) {
          Serial.write(ispSend(0x20 + LOW * 8, (here >> 8) & 0xFF, here & 0xFF, 0));   // low
          Serial.write(ispSend(0x20 + HIGH * 8, (here >> 8) & 0xFF, here & 0xFF, 0));  // high
          here++;
        }
        result = STK_OK;
      } else if (memtype == 'E') {
        // here again we have a word address
        int16_t start = here * 2;
        for (int16_t ii = 0; ii < length; ii++) {
          int16_t addr = start + ii;
          Serial.write(ispSend(0xA0, (addr >> 8) & 0xFF, addr & 0xFF, 0xFF));
        }
        result = STK_OK;
      }
      Serial.write(result);
    } break;

    case 0x75:                                  // 'u' - 0x75 Read Signature Bytes
      if (CRC_EOP != getch()) {
        Serial.write(STK_NOSYNC);
        break;
      }
      Serial.write(STK_INSYNC);
      Serial.write(ispSend(0x30, 0x00, 0x00, 0x00)); // high
      Serial.write(ispSend(0x30, 0x00, 0x01, 0x00)); // middle
      Serial.write(ispSend(0x30, 0x00, 0x02, 0x00)); // low
      Serial.write(STK_OK);
      break;

    case 0x20:                                  // ' ' - 0x20 CRC_EOP
      // expecting a command, not CRC_EOP, this is how we can get back in sync
      Serial.write(STK_NOSYNC);
      break;

    default:                                    // anything else we will return STK_UNKNOWN
      if (CRC_EOP == getch()) {
        Serial.write(STK_UNKNOWN);
      } else {
        Serial.write(STK_NOSYNC);
      }
  }
}

// ========================================
// public methods:
// ----------------------------------------

// used in DWDP_OOP.ino only
void InSystemProgrammer::programmer (void) {
  if (Serial.available()) {
    avrisp();
  }
}

// used in DWDP_OOP.ino only
void InSystemProgrammer::selectProgrammer () {
  pmode = 0;
  digitalWrite(VCC, HIGH);    // Enable VCC
  pinMode(VCC, OUTPUT);
  delay(100);
  debugWire.enable(false);
  disableSpiPins();          // Disable SPI pins
  Serial.begin(PROGRAM_BAUD);
}

// PrintFunctions.cpp

#include "PrintFunctions.h"

// ========================================
// Public methods:
// ----------------------------------------

PrintFunctions::PrintFunctions () {
  
}

// Print functions that track cursor position (to support tabbing)
// used in Helpers DisAssembler and Debugger
void PrintFunctions::print (char *txt) {
  cursor += strlen(txt);
  Serial.print(txt);
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::print (const __FlashStringHelper* txt) {
  cursor += strlen_P((char *) txt);
  Serial.print(txt);
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::println (char *txt) {
  Serial.println(txt);
  cursor = 0;
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::println (const __FlashStringHelper* txt) {
  Serial.println(txt);
  cursor = 0;
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::println () {
  Serial.println();
  cursor = 0;
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::write (char cc) {
  Serial.write(cc);
  cursor++;
}
// used in Helpers and DisAssembler
void PrintFunctions::tabTo (uint8_t pos) {
  while (cursor < pos) {
    Serial.write(' ');
    cursor++;
  }
}
// not used anywhere
void PrintFunctions::printHex (uint8_t val) {
  Serial.print(val, HEX);
  cursor += val < 0x10 ? 1 : 2;
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::printHex8 (uint8_t val) {
  if (val < 0x10) {
    Serial.print("0");
  }
  Serial.print(val, HEX);
  cursor += 2;
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::printHex16 (uint16_t val) {
  printHex8(val >> 8);
  printHex8(val);
}
// used in Helpers DisAssembler and Debugger
void PrintFunctions::printHex24 (unsigned long val) {
  printHex8(val >> 16);
  printHex8(val >> 8);
  printHex8(val);
}

// used in Helpers and Debugger
void PrintFunctions::printDec (uint8_t val) {
  Serial.print(val, DEC);
  if (val < 10) {
    cursor++;
  } else if (val < 100) {
    cursor += 2;
  } else {
    cursor += 3;
  }
}

// ========================================
// no Private methods!!!
// ----------------------------------------

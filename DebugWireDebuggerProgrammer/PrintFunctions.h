// PrintFunctions.h

#ifndef __PRINTFUNCTIONS_H__
#define __PRINTFUNCTIONS_H__

#include <Arduino.h>

class PrintFunctions {
  private:

  public:

    // used in Debugger, DisAssembler and PrintFunctions
    char cursor = 0;             // Output line cursor used by disassembler to tab
  
    PrintFunctions (); // Constructor
    void print (char *txt);                         // used in Helpers DisAssembler and Debugger
    void print (const __FlashStringHelper* txt);    // used in Helpers DisAssembler and Debugger
    void println (char *txt);                       // used in Helpers DisAssembler and Debugger
    void println (const __FlashStringHelper* txt);  // used in Helpers DisAssembler and Debugger
    void println ();                                // used in Helpers DisAssembler and Debugger
    void write (char cc);                           // used in Helpers DisAssembler and Debugger
    void tabTo (uint8_t pos);                          // used in Helpers and DisAssembler
    void printHex (uint8_t val);                       // not used anywhere
    void printHex8 (uint8_t val);                      // used in Helpers DisAssembler and Debugger
    void printHex16 (uint16_t val);             // used in Helpers DisAssembler and Debugger
    void printHex24 (unsigned long val);            // used in Helpers DisAssembler and Debugger
    void printDec (uint8_t val);                         // used in Helpers and Debugger
};
#endif

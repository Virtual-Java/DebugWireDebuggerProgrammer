//
//  I'm publishing this source code under the MIT License (See: https://opensource.org/licenses/MIT)
//
//    Copyright 2018 Wayne Holder (https://sites.google.com/site/wayneholder/)
//
//    Portions Copyright Copyright (c) 2008-2011 Randall Bohn
//  
//    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//    documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//    the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
//    to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  
//    The above copyright notice and this permission notice shall be included in all copies or substantial portions of
//    the Software.
//  
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//    THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

//#include "InSystemProgrammer.h"
//#include "Helpers.h"
#include "Debugger.h"


// used in DWDP_OOP.ino only
boolean       runMode;                // If HIGH Debug Mode, else Program

extern InSystemProgrammer  isp;
Debugger  dbug;

void setup () {
  pinMode(PMODE, INPUT);               // Mode Input
  digitalWrite(PMODE, HIGH);
  delay(1);
  if (runMode = digitalRead(PMODE)) {
    isp.selectProgrammer();
  } else {
    dbug.selectDebugger();
  }
}

void loop () {
  boolean mode = digitalRead(PMODE);
  if (runMode != mode) {
    runMode = mode;
    if (runMode) {
      isp.selectProgrammer();
    } else {
      dbug.selectDebugger();
    }
  }
  if (mode) {
    isp.programmer();
  } else {
    dbug.debugger();
  }
}

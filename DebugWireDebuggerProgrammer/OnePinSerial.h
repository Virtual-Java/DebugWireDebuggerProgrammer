/*
 *	OnePinSerial.h (based on SoftwareSerial.h, formerly NewSoftSerial.h) - 
*/

#ifndef __OnePinSerial_h__
#define __OnePinSerial_h__

#include <inttypes.h>

/******************************************************************************
* Definitions
******************************************************************************/

#ifndef _SS_MAX_RX_BUFF
#define _SS_MAX_RX_BUFF 128 // RX buffer size
#endif

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class OnePinSerial {
  private:
    // per object data
    uint8_t _ioPin;
    uint8_t _receiveBitMask;
    volatile uint8_t *_receivePortRegister;
    uint8_t _transmitBitMask;
    volatile uint8_t *_transmitPortRegister;
    volatile uint8_t *_pcint_maskreg;
    uint8_t _pcint_maskvalue;
    uint8_t _pcint_clrMask;
    bool  _fastRate;
  
    // Expressed as 4-cycle delays (must never be 0!)
    uint16_t _rx_delay_centering;
    uint16_t _rx_delay_intrabit;
    uint16_t _rx_delay_stopbit;
    uint16_t _tx_delay;
  
    // static data
    static uint8_t _receive_buffer[_SS_MAX_RX_BUFF]; 
    static volatile uint8_t _receive_buffer_tail;
    static volatile uint8_t _receive_buffer_head;
    static OnePinSerial *active_object;
  
    // private methods
    inline void recv () __attribute__((__always_inline__)); // used in OnePinSerial only
    uint8_t rx_pin_read();                                  // used in OnePinSerial only
    inline void setRxIntMsk(bool enable) __attribute__((__always_inline__));  // used in OnePinSerial only
  
    // Return num - sub, or 1 if the result would be < 1
    static uint16_t subtract_cap (uint16_t num, uint16_t sub); // used in OnePinSerial only
  
    // private static method for timing
    static inline void tunedDelay (uint16_t delay);         // used in OnePinSerial only
  
  public:
    // public methods
    OnePinSerial (uint8_t ioPin);
    void begin (long speed);                                // used in Debugger only
    void enable (bool);                                     // used in Debugger and InSystemProgrammer
    void sendBreak ();                                      // used in Debugger only
  
    virtual void sendCmd (uint8_t *byte, uint8_t len);      // used in Helpers only
    virtual void write (uint8_t byte);                      // used in OnePinSerial only
    virtual void write (uint8_t *byte, uint8_t len);        // used in OnePinSerial only
    virtual int16_t read ();                                    // used in Helpers and Debugger
    virtual int16_t available ();                               // used in Helpers and Debugger
    operator bool () { return true; }
    
    // public only for easy access by interrupt handlers
    static inline void handle_interrupt () __attribute__((__always_inline__));
};

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif

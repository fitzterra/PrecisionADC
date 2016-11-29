/**
 * Header for the PrecisionADC set of functions
 **/
#ifndef __PRECISION_VCC__
#define __PRECISION_VCC__

#include <stdint.h>
#include <Arduino.h>
#include <EEPROM.h>

// The bandgap voltage should be 1.1V, but could be between 1.0V and 1.2V but
// is fixed for the specific MCU. Here we define the expected bandgap voltage
// in mV.
#define BGREFmV 1100

// Key codes and results definitions for readSerial()
#define KEY_UP     1
#define KEY_DOWN   2
#define KEY_SPACE  3
#define KEY_ESC    4
#define KEY_ONE    5
#define KEY_TWO    6

// State names for calibrateBG() method
#define FT_STATE_MENU 'A'
#define FT_STATE_TUNE 'B'

// Structure to define the "memory" layout for writing the device specific
// bandgap reference to EEPROM.
typedef struct {
    char label[5];  // 4 character (plus terminating 0) label identifier
    uint16_t bgRef; // The actual bandgap reference
} bgMem;

// This is the address in the EEPROM of where to write the bandgap ref. Since
// the assumption is that most utilities that uses EEPROM will write to the
// start, we will default to writing to very end of EEPROM.
// The last EEPROM address is defined by the symbol E2END (see 
//   http://www.nongnu.org/avr-libc/user-manual/group__avr__io.html )
const uint16_t EEPROMAddy = E2END - sizeof(bgMem);

class PrecisionADC {
        uint16_t bgRefmV;   // Bandgap reference voltage in mV. 1100mV default
        uint32_t bgADC();   // Reads the bandgap ADC value relative to VCC
        uint8_t readSerial(uint8_t tout=100); // Monitors and decodes serial in
    public:
        // Constructor to use default BGREFmV bandgap or from EEPROM if available
        PrecisionADC() : bgRefmV(BGREFmV) { fromEEPROM(); };
        // Constructor that accepts bandgap ref voltage in mV as arg
        PrecisionADC(uint16_t mV) : bgRefmV(mV) {};

        // Writes the current (calibrated) bandgap reference to EEPROM
        void toEEPROM();
        // Sets the bandgap ref from a previously saved EEPROM value. Returns
        // true if a valid saved bg ref was found, false otherwise.
        bool fromEEPROM();

        // Sets a previously determined, more accurate, bandgap voltage in mV
        void setBGRef(uint16_t mV) {bgRefmV = mV;};
        // Returns the current bandgap reference voltage used for determining VCC
        uint16_t getBGRef() {return bgRefmV;};

        uint16_t readVcc(); // Returns the real VCC based on the bandgap voltage

        void calibrateBG();  // Interactively fine tune the bandgap reference

        uint16_t analogVoltage(uint16_t pin); // Samples voltage on analog pin
};

#endif  // __Precision_VCC__

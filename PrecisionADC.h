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

// ## Support and config for readSerial() method ## //
// Defines for detecting and stripping CR and LF characters
#define CR 0x0D     // Carraige return
#define LF 0x0A     // Line feed

// This defines the maximum number of bytes that can be expected for multi-byte
// keys. It is used as the second dimension for the multi dimensional keyTab
// array defined below.
#define KBUFSZ 3
// Keybord scan table for keys to detect and decode. This is a multi
// dimensional array of key, or multi-byte key definitions.
// To make it easy to use and define the table, add macro names as comments
// after each key/multi-key definition. Then you can use your favourite editor
// to extract those macro names and convert them to #defines with the value of
// each macro being the index in the table for the key/multi-key.
const char keyTab[][KBUFSZ] = {
                                  {'1'},        // KEY_1
                                  {'2'},        // KEY_2
                                  {' '},        // KEY_SPC
                                  {27, 91, 65}, // KEY_AUP  - Up arrow
                                  {27, 91, 66}, // KEY_ADN  - Down arrow
                                  {27},         // KEY_ESC  - Escape
                                  {'q'},        // KEY_q
                                  {'j'},        // KEY_j
                                  {'k'},        // KEY_k
                               };
// Calculate the number of keys in the scan table
const uint8_t keyTabLen = sizeof(keyTab) / (KBUFSZ * sizeof(char));
// Macro definitions for the key def indexes in keyTab
#define NO_KEY   -1
#define KEY_1     0
#define KEY_2     1
#define KEY_SPC   2
#define KEY_AUP   3
#define KEY_ADN   4
#define KEY_ESC   5
#define KEY_q     6
#define KEY_j     7
#define KEY_k     8
// END ## Support and config for readSerial() method ## //


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
        int8_t readSerial(uint8_t tout=100); // Monitors and decodes serial in
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

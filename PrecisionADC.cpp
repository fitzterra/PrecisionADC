/**
 * Library to allow precision ADC measurements on Arduino platform.
 */

#include "PrecisionADC.h"


bgMem bgEEPROMID {"bgID"}; // Struct that will be used as BG ref EEPROM ID

/**
 * Function to read the 1.1v reference voltage from the ATmega and use that to
 * calculate the true Vcc which is used by the ADC when measuring analog input.
 * The result is in mV.
 *
 * In order to obtain a more accurate analog input value, use this Vcc value to
 * calculate the real analog input:
 *
 * Vinput = (readVcc * pinVal) / 1023  -> in mV
 *
 * Credit here: https://code.google.com/p/tinkerit/wiki/SecretVoltmeter
 * with changes to make it work on other ATmega MCUs by various people in the
 * comments.
 *
 * Copying from the link above:
 *
 * The ATmega MCUs has an internal switch that selects which pin the analogue
 * to digital converter reads. That switch has a few leftover connections, so
 * the chip designers wired them up to useful signals. One of those signals is
 * the 1.1V precision reference that may be used as Vref for ADC measurements.
 *
 * So if you measure how large the known 1.1V reference is in comparison to Vcc,
 * you can back-calculate what Vcc is with a little algebra. That is how this
 * works.
 *
 * From the equation in section 24.7 of the ATmega data sheet giving the
 * formula used to get the ADC input value:
 *
 * ADC = (Vin * 1024) / Vref
 *
 * This function internally connects Vref to Vcc and the input to the ADC comes
 * from the internal 1.1V reference. Thus, rearranging this equation and
 * expressing voltage in millivolts gives:
 *
 * Vcc = (1100 * 1024) / ADC
 *
 * or
 *
 * Vcc = 1126400L / ADC
 *
 * Although this helps give a very accurate VCC value, the bandgap reference
 * voltage on the MCU is not guaranteed to be 1.1V :-(
 * It will however be between 1.0V and 1.2V, but it will be fixed for each MCU.
 * This means that once the true bandgap voltage is known for a specific MCU,
 * we can always get a very accurate VCC using the technique above.
 */
uint32_t PrecisionADC::bgADC() {
    // NOTE: this is not my code and bits and pieces were cobbled together from
    // all over the interwebs to get something that is supposed to work with
    // most Arduino supported MCU. My great thanks to all these authors!

	// Read Bandgap voltage reference against AVcc
	// set the reference to Vcc and the measurement to the internal bandgap reference
	#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
	#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
	#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	#endif

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA,ADSC)); // measuring

	uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
	uint8_t high = ADCH; // unlocks both

	uint32_t result = (high<<8) | low;

    return result;
}

/**
 * Private method to read and decode keyboard input from the serial port.
 *
 * When using a real terminal (unlike the serial console used by the Arduino
 * IDE), it is much easier to allow single key presses to issue commands
 * (instead of having the press enter after every command character), and more
 * intuative to use the up and down arrows to make adjustments, and the Escape
 * key to exit an operation.
 *
 * Such a terminal will send multi character codes to represent certain control
 * keys like arrows, page up/down, etc. To make it worse, these multi character
 * codes can start with the decimal value 27 which is also the code for the
 * escape key. This makes decoding all these keys a bit more complex.
 *
 * This method tries to make that easier by allowing a key table (keyTab) to
 * define all keys, and possible multi character codes for special keys, to
 * monitor the serial input for and decode to single integer value which is the
 * index in the keyTab of the input identified.
 *
 * See the header for a description of defining keyTab and setting macros to
 * make it easier to identify the decoded input.
 *
 * @param tout: Amount of time (millis) to wait for input on the serial line.
 *              This time should be just long enough to get all bytes for a
 *              multi-byte key press. The default is 100ms which should be fine.
 * 
 * @return : NO_KEY (-1) if no key press was available, or else the index into
 *           keyTab for the decoded keypress.
 **/
int8_t PrecisionADC::readSerial(uint8_t tout) {
    // We bail if there is no serial input
    if(!Serial.available())
        return NO_KEY;

    // Buffer and index
    uint8_t buf[KBUFSZ] = {};
    uint8_t bufIdx = 0;
    // Preset the timeout millis
    uint32_t exitAt = millis() + tout;
    // Will be set to the index in keyTab for the key matched from serial input
    // or -1 (NO_KEY) if no matches.
    int8_t matchedKey = NO_KEY;
    uint8_t partial;  // WIll count partial matches from the keyTab
    uint8_t c = 0;

    while(millis() < exitAt) {
        // Any serial data?
        if(!Serial.available()) continue;
        // Read it into the next spot in the buffer.
        buf[bufIdx] = Serial.read();
        // Ignore any CR or LF chars in case the serial terminal sends them
        if (buf[bufIdx]==CR or buf[bufIdx]==LF) {
            buf[bufIdx]=0;
            continue;
        }
        // Reset the timer
        exitAt = millis() + tout;

        // Try to match the buffer in the key table
        matchedKey = NO_KEY;
        partial = 0;
        for (uint8_t k=0; k<keyTabLen; k++) {
            // If there is no match on the first char, don't even bother
            if(buf[0] != keyTab[k][0]) {
                continue;
            }
            // First chars match, test how much of the buffer matches this key
            // definition , keeping in mind that both the buffer and key defs
            // will have all unused/trailing array elements set to 0
            c = 0;
            while(c<KBUFSZ && keyTab[k][c]==buf[c]) {
                c++;
            }
            // A complete match will leave our char index at KBUFSZ
            if (c==KBUFSZ) {
                matchedKey = k;
            } else {
                // It's a partial match
                partial++;
            }
        }
        // If we have an exact match, and the serial buffer is empty, we
        // can return the match
        if(matchedKey!=NO_KEY && partial==0 && !Serial.available()) {
            return matchedKey;
        }
        // If partial is 0 at this stage, then we know we have also not matched
        // anything, but we have already read at least one char from serial, so
        // there is no way to make any more matches, so we can return NO_KEY.
        // Also, if we have read KBUFSZ chars already, then there is no match,
        // so we can return NO_KEY
        if (partial==0 || bufIdx==KBUFSZ) {
            return NO_KEY;
        }
        // Otherwise we try to read another key into the buffer
        bufIdx++;
    }
    // After a timeout we can simply return matchedKey
    return matchedKey;
}

/**
 * Writes the current (adjusted) bandgap reference voltage to EEPROM.
 *
 * This allows the exact (calibrated) BG reference for this specific MCU
 * to be saved in EEPROM in a specific format that will allow it to be
 * identified and read back in again later after power cycling.
 **/
void PrecisionADC::toEEPROM() {
    // Put the current bandgap ref into bgEEPROMID before writing to EEPROM
    bgEEPROMID.bgRef = bgRefmV;
    // Write it
    EEPROM.put(EEPROMAddy, bgEEPROMID);
};

/**
 * Reads data from the EEPROM at EEPROMAddy and tests if this is a valid BG
 * reference ID structure.
 *
 * If the structure is correct, updates bgRefmV with the saved reference
 * voltage.
 *
 * @return : True if a valid saved refrence was found, false otherwise.
 **/
bool PrecisionADC::fromEEPROM() {
    // Declare a BG ref ID to receive the data from the EEPROM.
    bgMem savedRef;

    // Get from EEPROM
    EEPROM.get(EEPROMAddy, savedRef);
    // Check if the labels match
    if(strcmp(bgEEPROMID.label, savedRef.label) == 0) {
        // Update the reference
        bgRefmV = savedRef.bgRef;
        // Indicate success
        return true;
    }

    // We did not get a valid saved ref ID
    return false;
}

/**
 * Uses the bgADC() method to measure the bandgap reference against Vcc and
 * then calculates the true VCC from there.
 *
 * @return: The calculated VCC.
 **/
uint16_t PrecisionADC::readVcc() {
	uint32_t bgVal = bgADC();

	uint32_t vcc = (bgRefmV * 1024L) / bgVal; // Calculate Vcc (in mV)
	return (uint16_t)vcc; // Vcc in millivolts
}

/**
 * This method can be used to interactively fine tune the exact bandgap voltage
 * for this MCU.
 *
 * To use this method, the user should monitor the actual voltage supplied to
 * the MCU using a multimeter, and also monitor the serial console. This method
 * will then continuesly send the measured VCC, using the current bandgap
 * voltage as reference, to the serial console. This will be shown in mV.
 *
 * The VCC voltage displayed should be compared to that read on the DMM, and by
 * using the up and down arrow keys, the user can adjust the voltage shown on
 * the serial console to match the DMM.
 *
 * Every up/down keypress will adjust the the current bandgap reference voltage
 * by 1mV until it is perfectly accurate as future reference for VCC
 * measurement.
 *
 * Pressing the spacebar will display the current bandgap reference voltage,
 * and will prompt if this should be written to or retrieved from EEPROM.
 * TODO: More about storing this in EEPROM.....
 *
 * NOTE: It is assumed that the main application has already set up the serial
 * connection before this method is called.
 **/
void PrecisionADC::calibrateBG() {
    const uint16_t updateFreq = 1000;   // Live Vcc update frequency - mSecs
    char state = FT_STATE_MENU;         // Start off showing the menu
    int8_t key;                        // Will represent key pressed
    uint32_t nextUpdate = millis() + updateFreq; // Next millis to update VCC
    const char menu[] = "\n\r== Bandgap calibration ==\n\r" \
                        "[Space] to enter calibration display.\n\r" \
                        "[1] to retrieved saved bandgap value from EEPROM.\n\r" \
                        "[2] to save current bandgap value to EEPROM.\n\r" \
                        "[Escape]/[q] to exit calibration.\n\r\n\r" \
                        "While in calibration display, press:\n\r"\
                        "[Space] to return to this menu.\n\r" \
                        "[Up/Down arrows]/[k or j] to adjust bandgap voltage while\n\r" \
                        "  measuring the supply voltage (Vcc) externally with\n\r" \
                        "  a multimeter.\n\r\n\r" \
                        "[Space], [1], [2] or [Escape]/[q]?\n\r\n\r";

    // Show the menu on entry
    Serial.print(menu);
    // Loop forever, or until user quits
    while (true) {
        // Check for serial key input using default timeout
        key = readSerial();
        // Did we get anything
        if (key == NO_KEY) {
            // No input. In menu state we do nothing, in tuning state we update
            // live Vcc display if needed
            if (state==FT_STATE_TUNE && millis()>=nextUpdate) {
                // Show values
                Serial.print(F("Vcc: "));
                Serial.print(readVcc());
                Serial.print(F("mv, BG ref: "));
                Serial.print(bgRefmV);
                Serial.print(F("mV\n\r"));
                // Reset update indicator
                nextUpdate = millis() + updateFreq;
            }
        } else if (key==KEY_SPC) {
            // Space toggles between states
            state = state==FT_STATE_MENU ? FT_STATE_TUNE : FT_STATE_MENU;
            // Show the menu if we moved to menu state
            if (state == FT_STATE_MENU)
                Serial.print(menu);
        } else if (key==KEY_ESC || key==KEY_q) {
            // Escpae key pressed
            // When in menu state, we exit fine tuning
            if(state==FT_STATE_MENU) return;
            // We're in tuning state, so we allow going to the menu in addition
            // to pressing space.
            state = FT_STATE_MENU;
            Serial.print(menu);
        } else if (state==FT_STATE_MENU) {
            // In menu state we still need to deal with the EEPROM save and get
            if (key!=KEY_1 && key!=KEY_2) {
                // None of the keys we want, so get out of here
                continue;
            }
            if(key==KEY_2) {
                // Save our current bandgap reference value to EEPROM
                toEEPROM();
                Serial.print(F("\n\rSaved to EEPROM.\n\r\n\r"));
            } else {
                // Get any saved reference from EEPROM
                if (fromEEPROM()) {
                    Serial.print(F("Retrieved saved value from EEPROM.\n\r\n\r"));
                } else {
                    // No previously saved value was found.
                    Serial.print(F("\n\rNo saved bandgap value found in EEPROM.\n\r"));
                    // We want to stay in menu state, so we continue the loop
                    continue;
                }
            }
            // We have saved or retrieved from EEPROM, jump directly to tune state
            state = FT_STATE_TUNE;
        } else {
            // State is tuning, so deal with adjustments
            if(key==KEY_AUP || key==KEY_k) {
                Serial.println(F("[up]"));
                bgRefmV++;
            } else if (key==KEY_ADN || key==KEY_j) {
                Serial.println(F("[down]"));
                bgRefmV--;
            }
        }
    }
}

/**
 * Performs an analogRead() on the given pin, and returns an accurate voltage
 * representation in milli volt.
 *
 * This method will expect the analog regerence to be the DEFAULT supply
 * voltage. It will always first make a measurement of the current VCC, then do
 * an analogRead() on the given pin, and use the measured VCC to convert the
 * analog value to a voltage in milli volts.
 *
 * Due to the multiple sampling, this method is a lot slower than a simple
 * analogRead().
 *
 * @param pin (uint16_t): The analog pin to sample for the voltage reading
 * @return (uint16_t): The representative voltage in millivolt
 *
 * @todo: We may need some buffering and summing/averaging to increase accuracy
 * since there are still some very small errors.
 **/
uint16_t PrecisionADC::analogVoltage(uint16_t pin) {
    uint16_t vcc = readVcc();         // Get current VCC
    uint16_t adcIn = analogRead(pin); // Get ADC value

    // Convert to voltage
    return map(adcIn, 0, 1023, 0, vcc);
}

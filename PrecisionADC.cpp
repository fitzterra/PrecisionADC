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
 * Private method to read and decode keyboard input from the serial port.
 *
 * Since some control keys like the arrow keys are multi character inputs, this
 * method will try to decode these inputs as the characters are received. It
 * will wait up to tout millis between receiving input, and will return if
 * nothing was been received or no full input could be decoded.
 *
 * This method will scan for the following keys and return the values indicated
 * (see the header for for the key defines):
 * up arrow   -> KEY_UP
 * down arrow -> KEY_DOWN
 * space bar  -> KEY_SPACE
 * escape     -> KEY_ESC
 * 1          -> KEY_ONE
 * 2          -> KEY_TWO
 *
 * @param tout: Amount of time (millis) to wait for input on the serial line.
 *              This time should be just long enough to get all bytes for a
 *              multi-byte key press. The default is 100ms which should be fine.
 **/
uint8_t PrecisionADC::readSerial(uint8_t tout) {
    // Preset the timeout millis
    uint32_t exitAt = millis() + tout;
    uint8_t in;
    // State machine current state representation. The states are:
    // 'A' : start
    // 'B' : escape code received (final)
    // 'C' : extended code received
    // 'D' : up arrow pressed (final)
    // 'E' : down arrow pressed (final)
    // 'F' : space key (final)
    // 'G' : numeric 1 key (final)
    // 'H' : numeric 2 key (final)
    char state = 'A';

    // Keep looping if not timed out yet
    while(millis() < exitAt) {
        // Any serial data?
        if(!Serial.available()) continue;
        // There is something available, so reset the timer and get the value
        exitAt = millis() + tout;
        in = Serial.read();
        switch (in) {
            case 27:    // Escape code - either from the key or extended start
                // We advance only if current state is 'A', else we reset
                state = state=='A' ? 'B' : 'A';
                break;
            case 91:    // Extended code indicator if we're in state B
                // We advance only if current state is 'B', else we reset
                state = state=='B' ? 'C' : 'A';
                break;
            case 65:    // Up arrow if we're in state C
            case 66:    // Down arrow if we're in state C
                if (state=='C') {
                    // We're in state 'D' or 'E' which are final states, so
                    // we can return here
                    return in==65 ? KEY_UP : KEY_DOWN;
                }
                // Reset state
                state = 'A';
                break;
            case 32:    // Space if we're in state 'A'
                if (state=='A') {
                    // We're in state 'F' which is a final state, so
                    // we can return here
                    return KEY_SPACE;
                }
                // Reset state
                state = 'A';
                break;
            case 49:    // Number '1' key if we're in state 'A'
            case 50:    // Number '2' key if we're in state 'A'
                if (state=='A') {
                    // We're in state 'G' or 'H' which are final states, so
                    // we can return here
                    return in==49 ? KEY_ONE : KEY_TWO;
                }
                // Reset state
                state = 'A';
                break;
            default:
                // Reset for anything else
                state = 'A';
        }
    }

    // We could possibly be in state B here (escape pressed)
    if (state == 'B') {
        return KEY_ESC;
    }
    
    // Indicate no valid input was received
    return 0;
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
void PrecisionADC::fineTuneBG() {
    const uint16_t updateFreq = 1000;   // Live Vcc update frequency - mSecs
    char state = FT_STATE_MENU;         // Start off showing the menu
    uint8_t key;                        // Will represent key pressed
    uint32_t nextUpdate = millis() + updateFreq; // Next millis to update VCC
    const char menu[] = "\n== Bandgap fine tuning ==\n" \
                        "[Space] to enter fine tuning display.\n" \
                        "[1] to retrieved saved bandgap value from EEPROM.\n" \
                        "[2] to save current bandgap value to EEPROM.\n" \
                        "[Escape] to exit fine tuning.\n\n" \
                        "While in fine tuning display, press:\n"\
                        "[Space] to return to this menu.\n" \
                        "[Up/Down arrows] to adjust bandgap voltage while\n" \
                        "  measuring the supply voltage (VCC) externally with\n" \
                        "  a multimeter.\n\n" \
                        "[Space], [1], [2] or [Escape]?\n\n";

    // Show the menu on entry
    Serial.print(menu);
    // Loop forever, or until user quits
    while (true) {
        // Check for serial key input using default timeout
        key = readSerial();
        // Did we get anything
        if (key == 0) {
            // No input. In menu state we do nothing, in tuning state we update
            // live Vcc display if needed
            if (state==FT_STATE_TUNE && millis()>=nextUpdate) {
                // Show values
                Serial.print(F("Vcc: "));
                Serial.print(readVcc());
                Serial.print(F("mv, BG ref: "));
                Serial.print(bgRefmV);
                Serial.print(F("mV\n"));
                // Reset update indicator
                nextUpdate = millis() + updateFreq;
            }
        } else if (key==KEY_SPACE) {
            // Space toggles between states
            state = state==FT_STATE_MENU ? FT_STATE_TUNE : FT_STATE_MENU;
            // Show the menu if we moved to menu state
            if (state == FT_STATE_MENU)
                Serial.print(menu);
        } else if (key==KEY_ESC) {
            // Escpae key pressed
            // When in menu state, we exit fine tuning
            if(state==FT_STATE_MENU) return;
            // We're in tuning state, so we allow going to the menu in addition
            // to pressing space.
            state = FT_STATE_MENU;
            Serial.print(menu);
        } else if (state==FT_STATE_MENU) {
            // In menu state we still need to deal with the EEPROM save and get
            if (key!=KEY_ONE && key!=KEY_TWO) {
                // None of the keys we want, so get out of here
                continue;
            }
            if(key==KEY_TWO) {
                // Save our current bandgap reference value to EEPROM
                saveToEEPROM();
                Serial.print(F("\nSaved to EEPROM.\n\n"));
            } else {
                // Get any saved reference from EEPROM
                if (getFromEEPROM()) {
                    Serial.print(F("Retrieved saved value from EEPROM.\n\n"));
                } else {
                    // No previously saved value was found.
                    Serial.print(F("\nNo saved bandgap value found in EEPROM.\n"));
                    // We want to stay in menu state, so we continue the loop
                    continue;
                }
            }
            // We have saved or retrieved from EEPROM, jump directly to tune state
            state = FT_STATE_TUNE;
        } else {
            // State is tuning, so deal with adjustments
            if(key==KEY_UP) {
                Serial.println(F("[up]"));
                bgRefmV++;
            } else if (key==KEY_DOWN) {
                Serial.println(F("[down]"));
                bgRefmV--;
            }
        }
    }
}

/**
 * Writes the current (adjusted) bandgap reference voltage to EEPROM.
 *
 * This allows the exact (after fine tuning) BG reference for this specific MCU
 * to be saved in EEPROM in a specific format that will allow it to be
 * identified and read back in again later after power cycling.
 * 
 **/
void PrecisionADC::saveToEEPROM() {
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
bool PrecisionADC::getFromEEPROM() {
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

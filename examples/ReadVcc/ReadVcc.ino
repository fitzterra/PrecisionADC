/**
 * Sketch to read the current voltage supplying the MCU.
 *
 * The default Arduino EEPROM library is required to use the PrecisionADC
 * library.
 * 
 * No external components are required.
 * A fairly stable supply voltage, within spec, is required though. 
 * A more accurate reading may be possible if the bandgap reference voltage has
 * been calibrated.
 **/

#include <PrecisionADC.h>

// Set up an instance of the PrecisionADC class, using the default constructor.
// The default constructor would look for a previously saved bandgap voltage in
// EEPROM, and automatically use that voltage as the calibrated bandgap
// voltage.
PrecisionADC pADC;

void setup() {
    Serial.begin(115200);
}

void loop() {
    Serial.print("Current Vcc: ");
    Serial.print(pADC.readVcc());
    Serial.println("mV\n");
    delay(2000);
}


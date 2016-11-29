/**
 * Sketch to read n analog input voltage on an analog pin using the
 * PrecisionADC library.
 *
 * The default Arduino EEPROM library is required to use the PrecisionADC
 * library.
 * 
 * Connect a potentiometer as follows:
 *
 *   .----\/\/\/\/\/\-----.
 *   |         A          | 
 *   |        /|\         | 
 *   |         |          | 
 *   o         o          o 
 *  Gnd        A0        Vcc
 * 
 * Also connect a mulitmeter between Gnd and potentiometer wiper connection
 * (which is also connected to A0).
 *
 * When running the sketch, compare the multimeter reading with the measured
 * analog voltage on the serial output. Adjust the pot for different voltages.
 *
 * A more accurate reading may be possible if the bandgap reference voltage has
 * been calibrated before (see the calibration example).
 **/

#include <PrecisionADC.h>

#define SENSEPIN A0

// Set up an instance of the PrecisionADC class, using the default constructor.
// The default constructor would look for a previously saved bandgap voltage in
// EEPROM, and automatically use that voltage as the calibrated bandgap
// voltage.
PrecisionADC pADC;

void setup() {
    Serial.begin(115200);
}

void loop() {
    Serial.print("A0: ");
    Serial.print(pADC.analogVoltage(SENSEPIN));
    Serial.println("mV");
    delay(2000);
}



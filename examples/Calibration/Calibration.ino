/**
 * Sketch to calibrate the bandgap reference voltage for precision ADC
 * readings.
 *
 * The default Arduino EEPROM library is required to use the PrecisionADC
 * library.
 * 
 * No external components are required.
 * A fairly stable supply voltage, within spec, is required though. The supply
 * voltage on some USB ports are too far below 5V which could make good
 * calibration of the bandgap voltage difficult and inacurate.
 *
 * Please note that when writing the calibrated bandgap voltage to EEPROM, that
 * it will by default write to very end of available EEPROM, affectively
 * overwriting anything already stored there.
 **/

#include <PrecisionADC.h>

// Set up an instance of the PrecisionADC class, using the default constructor.
// The default constructor would look for a previously saved bandgap voltage in
// EEPROM, and automatically use that voltage as the calibrated bandgap
// voltage.
PrecisionADC pADC;

void setup() {
    // The bandgap fine tuning method uses the serial in/out for calibration
    // interaction, so the serial port must be set up.
    Serial.begin(115200);
}

void loop() {
    Serial.println("\nStarting main loop...\n\n");
    // We just keep calling the calibration routine
    pADC.calibrateBG();
}

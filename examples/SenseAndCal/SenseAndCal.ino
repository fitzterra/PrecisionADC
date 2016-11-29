/**
 * Example of using the PrecisionADC library with an anolog sensor and an
 * option for calibrating the bandgap voltage.
 *
 * Connect any anolog type sensor to A0 and hook up a mulitple to measure the
 * supply voltage to the MCU.
 *
 * While running, the analog voltage from the sendsor will be displayed.
 * Press v to display the currently measured MCU supply voltage.
 * Press c to calibrate the bandgap reference voltage.
 */
#include <PrecisionADC.h>

#define SENSEPIN A0     // Pin sensor is connected to

PrecisionADC pADC;

void setup() {
  Serial.begin(115200);
}

/**
 * Monitor serial input for fine tuning bandgap, or showing current VCC.
 **/
void serialEvent() {
    char in;

    in = Serial.read();
    switch (in) {
        case 'c':
            pADC.calibrateBG();
            break;
        case 'v':
            Serial.print("Vcc: ");
            Serial.print(pADC.readVcc());
            Serial.println("mV");
            break;
    }
}

/**
 * Main loop
 **/
void loop() {
  int sensemV = pADC.analogVoltage(SENSEPIN);

  Serial.print("Analog Voltage: ");
  Serial.print(sensemV);
  Serial.println("mV");
  delay(1000);
}

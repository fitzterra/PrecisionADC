Precise ADC measuring regardless of supply voltage
===================================================

Abstract
--------
This library provides a class and additional support functionality to allow
accurate ADC readings regardless of the supply voltage, when using the default
supply voltage as ADC reference (i.e. `analogReference(DEFAULT)` ) for Arduino
type environments.

ADC and Supply Voltage (VCC)
----------------------------
Any Analog to Digital Conversion has to be done with _reference_ to another
voltage source.

For the normal `analogRead()` function, without explicitly changing the
conversion reference voltage using `analogReference()`, the supply voltage to
the MCU (**VCC**) is used as the **conversion reference**.

Since VCC to the MCU may vary under different conditions (USB power is not
always exactly 5V, battery voltages drop as they get depleted, changes in system
load, etc.), any ADC reading using the DEFAULT analog reference will be subject
to this variation in VCC.

This can be overcome by always connecting a stable, external, analog voltage
reference circuit to the AREF input, but this is often more trouble than needed.

Another, more convenient option is to use the internal stable bandgap reference
voltage to determine the true VCC as described below.

Using the internal bandgap reference to determine VCC
-----------------------------------------------------
The Atmel MCUs used in the Arduino range has an internal bandgap voltage
reference of 1.1V, which is stable regardless of supply voltage.

There is a way that this internal bandgap voltage can be used as analog input
and measured as if it is connected to an analog pin. This masurement can be done
with the supply voltage (VCC) as analog reference.

Since the bangap voltage is known, we can determine VCC by rearanging the
formula used to calculate the ADC value. From the Atmel datasheet, the ADC value
will be given by this formula:

    ADC = (Vin/Vref) * 1024

Where `Vin` is the analog input voltage (the bandgap reference in this case),
and Vref is VCC.

Solving for Vref, we get:

    Vref = (Vin/ADC) * 1024

In short this means that we can determine VCC by measuring the internal bandgap
voltage with VCC as reference, and then use the rearranged formula to figure
out the actual VCC.

Unfortunately, the bandgap voltage is not 100% accurate, and can be anywhere
between 1.0V and 1.2V for a given chip, so for more accurate results, we
need to know the exact bandgap voltage. See below.

Overcomming bandgap reference variations
---------------------------------------
The Atmel datasheets mentions that the bandgap reference voltage can be anywhere
between 1.0V and 1.2V, with a typical value of 1.1V. That is a 200mV variation,
which, when used to measure a temp sensor with 10mV/degree output, can result in
a 20degree inacuracy!

Although the bandgap reference may not be 100% precise, it is fixed and stable
for each individual MCU.

If we can figure out the exact bandgap reference for a specific MCU, we can use
that value in the formula mentioned above:

    Vref = (Vbandgap/ADC) * 1024

to get a very accurate VCC value when measuring the bandgap voltage with VCC as
reference.

If we then store this bandgap value in a specific area in EEPROM for this
specific chip, and then always look for this value in EEPROM when we start up,
we should be set.

### Measuring the true bandgap reference voltage
A simple way of meassuring the true bandgap voltage is:

1. use a multimeter to measure the actual VCC to the MCU
2. start with an assumption of the bandgap reference being 1100mv
3. measure the actual bandgap voltage with VCC as reference as described above
4. use the formula above to calculate VCC
5. compare the the calculated VCC with the VCC reading on the multimeter
6. adjust the bandgap reference value up or down based on the difference
   between calculated VCC and measured VCC
7. repeat from step 3 until calculated and measured VCC matches.

The `PrecisionADC::calibrateBG()` method can be used to do exactly this. When
called, it will present a menu of options on the serial console, and allow you to
enter a _Fine Tuning_ state where the calculated VCC and bandgap reference value
used for the calculation is output every second.

You then compare the calculated VCC to your multimeter measured VCC for the MCU,
and use the keyboard up and down arrows to adjust the bandgap reference until
the measured and calculated VCCs match.

Once the true bandgap reference has been found in this way, you have the option
to save this value to EEPROM. See below

### Saving the true bandgap reference on the MCU
Once you have determined the true bandgap voltage reference, you can save it to
the EEPROM on the MCU.

This will be saved by default to the end of the EEPROM and will use a short
signature to detect if the bandgap has previously been saved to EEPROM for this
MCU.

The `PrecisionADC` class default constructor will automatically look for this
signature and bandgap ref value on instantiation, and set the bandgap value if
found.

PrecisionADC usage
------------------
To use this library:

1. include it in your source
2. create an instance of the `PrecisionADC` class
3. ensure the instance has an accurate bandgap reference
4. use the `analogVoltage()` method on any anlog pin to read as millivolt

Example:
```c++
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
```

TODO: Explain how to set/save measured bandgap voltage, and other available
methods.

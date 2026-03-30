# M5Unit - ANADIG

## Overview

Library for ANADIG using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U013-V11
Unit ADC v1.1 is a unit integrated with a 16-bit Analog-to-Digital Converter using the ADS1110 chip, featuring an internal reference voltage (2.048V). Through the I2C interface, it achieves high-precision conversions at 15, 30, 60, or 240 times per second. The unit has a built-in Programmable Gain Amplifier (PGA), with a maximum gain of up to 8x, enabling direct measurement of weak signals. The ADS1110 also comes with an internal reference voltage (2.048V) to support high-resolution measurements.
It is used in industrial process control, factory automation, portable instruments, and other fields.

### SKU:U012
Unit DAC is a high-performance digital/analog signal converter, which has a built-in MCP4725. It features low power consumption, high precision, single-channel, 12-bit buffered voltage output digital-to-analog converter (DAC), and non-volatile memory (EEPROM).
Users can use I2C interface commands to write the DAC input and configuration into the non-volatile memory (EEPROM), allowing the DAC to retain the code during power-off and be used directly upon power-on. The I2C address is 0x60.

### SKU:U012-B
Unit DAC2 is an I2C digital-to-analog signal conversion unit. It uses the GP8413 solution, which can linearly convert 15-bit digital signals into two independent 0-5V/0-10V analog voltages with an output voltage error of 0.2% and a linearity of 0.01%. In terms of expandability, the circuit design allows for the selection of three hardware addresses A2/A1/A0, supporting the simultaneous operation of up to 8 devices with 16 channels output.
In terms of safety, the device has a short-circuit protection function, automatically entering protection mode and stopping output when the output pins are shorted to ground. It is suitable for general signal conversion, motor speed control, LED dimming, inverters, power supplies, and industrial analog signal isolation.

### SKU:U069
Hat ADC is a high-precision analog-to-digital conversion module designed specifically for the StickC series. It integrates the ADS1100 16-bit Δ-Σ ADC chip, featuring fully differential input, self-calibration, and programmable gain. The chip itself supports differential inputs from -5V ~ +5V. Through optimized peripheral circuit design, the module extends this capability to a DC voltage measurement range of 0 ~ 12V.
Using a standard I2C communication interface, the module enables accurate and stable voltage acquisition, making it suitable for industrial sensor signal processing, battery voltage monitoring, and various analog signal acquisition applications.

### SKU:U069-V11
Hat ADC v1.1 is a high-precision analog-to-digital conversion module specifically designed for the StickC series. It integrates the ADS1110 16-bit Δ-Σ ADC chip, featuring fully differential inputs, self-calibration, and adjustable gain. The chip itself supports a differential input range of -5V ~ +5V. Through optimized peripheral circuit design, the module extends this capability to a DC voltage measurement range of 0 ~ 12V.
It also includes a built-in 2.048V reference voltage, enabling stable and accurate signal acquisition without the need for an external reference source. The module communicates via a standard I2C interface and is suitable for applications such as industrial sensor signal processing, battery voltage monitoring, and various analog signal acquisition scenarios.

### SKU:U068-B
Hat DAC2 is an I2C digital-to-analog signal conversion unit designed for the StickC series controllers. Using the GP8413 solution, this chip can linearly convert a 15-bit digital quantity into two independent 0–5V / 0–10V analog voltages with an output voltage error of 0.2% and linearity of 0.01%. For scalability, the circuit design allows selection via three hardware address pins (A2/A1/A0).
In terms of safety, the device supports output short-circuit protection — when the output pin is shorted to ground, it automatically enters protection mode and stops output. It is suitable for general signal conversion, motor speed control, LED dimming, inverters, power supplies, and industrial analog signal isolation applications.

## Related Link
See also examples using conventional methods here.

- [Unit ADC v1.1 & Datasheet](https://docs.m5stack.com/en/unit/Unit-ADC_V1.1)
- [Unit DAC & Datasheet](https://docs.m5stack.com/en/unit/dac)
- [Unit DAC2 & Datasheet](https://docs.m5stack.com/en/unit/Unit-DAC2)
- [Hat ADC & Datasheet](https://docs.m5stack.com/en/hat/hat-adc)
- [Hat ADC v1.1 & Datasheet](https://docs.m5stack.com/en/hat/HAT-ADC-V11)
- [Hat DAC2 & Datasheet](https://docs.m5stack.com/en/hat/Hat-DAC2)

### Required Libraries:
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-ANADIG - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### For ArduinoIDE settings
You must choose a define symbol for the unit you will use.
(Uncomment the corresponding `#define` in the example, or specify it with compile options.)
- ADCInput
```cpp
// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_ADC11) && !defined(USING_HAT_ADC11) && !defined(USING_HAT_ADC)
// For UnitADC11
// #define USING_UNIT_ADC11
// For HatADC11
// #define USING_HAT_ADC11
// For HatADC
// #define USING_HAT_ADC
#endif
```

- DACOutput
```cpp
// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_DAC) && !defined(USING_UNIT_DAC2) && !defined(USING_HAT_DAC2)
// For UnitDAC
// #define USING_UNIT_DAC
// For UnitDAC2
// #define USING_UNIT_DAC2
// For HatDAC2
// #define USING_HAT_DAC2
#endif
```

### Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-ANADIG/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

#### Required
- [Doxygen](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)

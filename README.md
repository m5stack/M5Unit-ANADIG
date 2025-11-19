# M5Unit - ANADIG

## Overview

Library for ANADIG using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U013-V11
ADC V1.1 Unit is an A/D conversion module that utilizes the ADS1110 chip, a 16-bit self-calibrating analog-to-digital converter. It is designed with an I2C interface, offering convenient connectivity. The module offers conversion speeds of 8, 16, 32, and 128 samples per second (SPS), providing varying levels of accuracy at 16, 15, 14, and 12 bits of resolution respectively.

### SKU:U012
DAC, is a unit with the ability to convert digital signal to analog signal (voltage waveform signal) with support for audio waveform and more. It integrates a 12-bit high resolution DAC chip named MCP4725 which has an on-board non-volatile memory (EEPROM). The unit communicates with the M5Core using the I2C protocol.

### SKU:U012-B
DAC2 Unit is an I2C digital-to-analog signal conversion unit. It utilizes the GP8413 solution, which provides high precision and accuracy in its performance. This chip can linearly convert a 15-bit digital value into two independent analog voltages of 0-5V or 0-10V. The output voltage error is only 0.2%, and it achieves linearity of up to 0.01%.

### SKU:U069
ADC HAT is another type of C-HAT specifically design for M5StickC controller. Same as ADC unit, this is an ADC converter component for stickc. Packed with an ADC converter chip ADS1100, 

### SKU:U069-V11
ADC HAT V1.1 is another type of C-HAT specifically design for M5StickC controller. Same as ADC unit, this is an ADC converter component for stickc. Packed with an ADC converter chip ADS1110, 

### SKU:U068-B
DAC2 Hat is an I2C digital-to-analog signal conversion unit designed to be compatible with the StickC series controllers. It offers high precision and accuracy in its performance by using the GP8413 solution

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
(Rewrite source or specify with compile options)
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


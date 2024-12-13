# M5Unit - ANADIG

## Overview

Library for ANADIG using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U012
DAC, is a unit with the ability to convert digital signal to analog signal (voltage waveform signal) with support for audio waveform and more. It integrates a 12-bit high resolution DAC chip named MCP4725 which has an on-board non-volatile memory (EEPROM). The unit communicates with the M5Core using the I2C protocol.

### SKU:U012-B
DAC2 Unit is an I2C digital-to-analog signal conversion unit. It utilizes the GP8413 solution, which provides high precision and accuracy in its performance. This chip can linearly convert a 15-bit digital value into two independent analog voltages of 0-5V or 0-10V. The output voltage error is only 0.2%, and it achieves linearity of up to 0.01%.

## Related Link
See also examples using conventional methods here.

- [Unit DAC & Datasheet](https://docs.m5stack.com/en/unit/dac)
- [Unit DAC2 & Datasheet](https://docs.m5stack.com/en/unit/Unit-DAC2)


### Required Libraries:
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-ANADIG - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-ANADIG/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

#### Required
- [Doxyegn](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)


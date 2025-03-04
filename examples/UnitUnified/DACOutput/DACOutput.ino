/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitDAC2/HatDAC2
*/
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
// *************************************************************
#include "main/DACOutput.cpp"

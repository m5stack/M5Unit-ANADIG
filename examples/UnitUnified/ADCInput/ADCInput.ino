/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitADC11/HatADC11/HatADC
*/
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
// *************************************************************
#include "main/ADCInput.cpp"

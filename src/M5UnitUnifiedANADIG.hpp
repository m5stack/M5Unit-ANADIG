/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file M5UnitUnifiedANADIG.hpp
  @brief Main header of M5UnitANADIG using M5UnitUnified

  @mainpage M5Unit-ANADIG
  Library for UnitANADIG using M5UnitUnified.
*/
#ifndef M5_UNIT_UNIFIED_ANADIG_HPP
#define M5_UNIT_UNIFIED_ANADIG_HPP

#include "unit/unit_ADS1100.hpp"
#include "unit/unit_ADS1110.hpp"
#include "unit/unit_MCP4725.hpp"
#include "unit/unit_GP8413.hpp"

/*!
  @namespace m5
  @brief Top level namespace of M5Stack
 */
namespace m5 {

/*!
  @namespace unit
  @brief Unit-related namespace
 */
namespace unit {

//! @brief Alias for ADS1110 based unit
using UnitADC11 = m5::unit::UnitADS1110;
//! @brief Alias for MCP4725 based unit
using UnitDAC = m5::unit::UnitMCP4725;
//! @brief Alias for GP8413 based unit
using UnitDAC2 = m5::unit::UnitGP8413;

//! @brief Alias for ADS1100 based HAT
using HatADC = m5::unit::UnitADS1100;
//! @brief Alias for ADS1110 based HAT
using HatADC11 = m5::unit::UnitADS1110;
//! @brief Alias for GP8413 based HAT
using HatDAC2 = m5::unit::UnitGP8413;

}  // namespace unit
}  // namespace m5
#endif

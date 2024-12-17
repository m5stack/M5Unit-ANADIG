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

#include "unit/unit_ADS1110.hpp"
#include "unit/unit_MCP4725.hpp"
#include "unit/unit_GP8413.hpp"

/*!
  @namespace m5
  @brief Top level namespace of M5stack
 */
namespace m5 {

/*!
  @namespace unit
  @brief Unit-related namespace
 */
namespace unit {

using UnitADC  = m5::unit::UnitADS1110;
using UnitDAC  = m5::unit::UnitMCP4725;
using UnitDAC2 = m5::unit::UnitGP8413;

}  // namespace unit
}  // namespace m5
#endif

/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1100.hpp
  @brief ADS1100 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ANADIG_UNIT_ADS1100_HPP
#define M5_UNIT_ANADIG_UNIT_ADS1100_HPP
#include "unit_ADS11xx.hpp"

namespace m5 {
namespace unit {
/*!
  @namespace ads1100
  @brief For ADS1100
*/
namespace ads1100 {
/*!
  @enum Sampling
  @brief Data sampling rate for periodic
 */
enum class Sampling : uint8_t {
    Rate128,  //!< 128 SPS
    Rate32,   //!< 32 SPS
    Rate16,   //!< 16 SPS
    Rate8     //!< 8 SPS (as default)
};

using PGA  = m5::unit::ads11xx::PGA;
using Data = m5::unit::ads11xx::Data;

}  // namespace ads1100

/*!
  @class UnitADS1100
  @brief 16-bit, self-calibrating, delta-sigma A/D converter
*/
class UnitADS1100 : public UnitADS11XX {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitADS1100, 0x48);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Data sampling rate if start on begin
        //        ads1100::Sampling sampling_rate{ads1100::Sampling::Rate8};
        ads1100::Sampling sampling_rate{ads1100::Sampling::Rate32};
        //! PGA if start on begin
        ads1100::PGA pga{ads1100::PGA::Gain1};
        //! VDD (mV) Unit/HatADC is 3.3V
        float vdd{3300.f};
        //! Correction factor (Normalization factor of input due to voltage divider resistors, etc)
        float factor{0.25f};
    };

    explicit UnitADS1100(const float vdd = 3.3f, const float factor = 0.25f, const uint8_t addr = DEFAULT_ADDRESS)
        : UnitADS11XX(addr)
    {
        _vdd        = vdd;
        _cfg.factor = _factor = factor;
    }
    virtual ~UnitADS1100()
    {
    }

    virtual bool begin() override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@name Settings
    ///@{
    /*!
      @brief Read the Sampling rate
      @param[out] rate Sampling rate
      @return True if successful
     */
    bool readSamplingRate(ads1100::Sampling& rate);
    /*!
      @brief Write the Sampling rate
      @param rate Sampling rate
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeSamplingRate(const ads1100::Sampling rate);
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param rate Data sampling rate
      @param pga Programmable Gain Amplifier
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const ads1100::Sampling rate, const ads1100::PGA pga)
    {
        return start_periodic_measurement(rate, pga);
    }
    //! @brief Start periodic measurement using current settings
    inline bool startPeriodicMeasurement()
    {
        return UnitADS11XX::startPeriodicMeasurement();
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return UnitADS11XX::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @param[out] data Measuerd data
      @param rate Data sampling rate
      @param pga Programmable Gain Amplifier
      @return True if successful
      @note Blocked until the end of measurement
      @note Blocking time depends on rate value
      @warning During periodic detection runs, an error is returned
      @warning Each setting is overwritten
    */
    bool measureSingleshot(ads1100::Data& data, const ads1100::Sampling rate, const ads1100::PGA pga);
    //! @brief Measurement single shot using current settings
    inline bool measureSingleshot(ads1100::Data& data)
    {
        return UnitADS11XX::measure_singleshot(data);
    }
    ///@}

    /*!
      @brief General reset
      @details Reset using I2C general call
      @return True if successful
      @warning This is a reset by General command, the command is also sent to all devices with I2C connections
    */
    virtual bool generalReset() override;

protected:
    bool start_periodic_measurement(const ads1100::Sampling rate, const ads1100::PGA pga);
    virtual bool read_if_ready_in_periodic(uint8_t v[2]) override;
    virtual uint32_t get_interval(const uint8_t rate) override;

private:
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5
#endif

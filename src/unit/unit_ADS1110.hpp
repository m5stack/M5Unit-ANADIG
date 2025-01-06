/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1110.hpp
  @brief ADS1110 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ANADIG_UNIT_ADS1110_HPP
#define M5_UNIT_ANADIG_UNIT_ADS1110_HPP
#include <M5UnitComponent.hpp>
#include <m5_utility/stl/extension.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

/*!
  @namespace mcp4725
  @brief For ADS1110
*/
namespace ads1110 {
/*!
  @enum Sampling
  @brief Data sampling rate for periodic
 */
enum class Sampling : uint8_t {
    Rate240,  //!< 240 SPS
    Rate60,   //!< 60 SPS
    Rate30,   //!< 30 SPS
    Rate15,   //!< 15 SPS as default
};

/*!
  @enum PGA
  @brief Programmable Gain Amplifier
 */
enum class PGA : uint8_t {
    Gain1,  //!< 1 as default
    Gain2,  //!< 2
    Gain4,  //!< 4
    Gain8,  //!< 8
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 2> raw{};  //!< Raw
    PGA pga{};                     //!< PGA

    ///! @brief Gets the differential value
    inline int16_t differentialValue() const
    {
        return (int16_t)m5::types::big_uint16_t(raw[0], raw[1]).get();
    }
    //! @brief differential value to differential voltage(mV)
    inline float differentialVoltage() const
    {
        return (differentialValue() * gain_table[m5::stl::to_underlying(pga)]) / 32768.f * 1000.f;
    }
    static const float gain_table[4];
};

}  // namespace ads1110

/*!
  @class UnitADS1110
  @brief 16-bit, self-calibrating, delta-sigma A/D converter
*/
class UnitADS1110 : public Component, public PeriodicMeasurementAdapter<UnitADS1110, ads1110::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitADS1110, 0x48);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Data sampling rate if start on begin
        ads1110::Sampling sampling_rate{ads1110::Sampling::Rate15};
        //! PGA if start on begin
        ads1110::PGA pga{ads1110::PGA::Gain1};
    };

    explicit UnitADS1110(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<ads1110::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitADS1110()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

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

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest measured differential value
    inline int16_t differentialValue() const
    {
        return !empty() ? oldest().differentialValue() : 0;
    }
    //! @brief Oldest measured differential voltage(mV)
    inline float differentialVoltage() const
    {
        return !empty() ? oldest().differentialVoltage() : std::numeric_limits<float>::quiet_NaN();
    }
    ///@}

    ///@name Settings
    ///@{
    /*!
      @brief Read the Sampling rate
      @param[out] rate Sampling rate
      @return True if successful
     */
    bool readSamplingRate(ads1110::Sampling& rate);
    /*!
      @brief Write the Sampling rate
      @param rate Sampling rate
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeSamplingRate(const ads1110::Sampling rate);
    /*!
      @brief Read the PGA
      @param[out] pga PGA
      @return True if successful
     */
    bool readPGA(ads1110::PGA& pga);
    /*!
      @brief Write the PGA
      @param pga PGA
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writePGA(const ads1110::PGA pga);
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param rate Data sampling rate
      @param pga Programmable Gain Amplifier
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const ads1110::Sampling rate, const ads1110::PGA pga)
    {
        return start_periodic_measurement(rate, pga);
    }
    //! @brief Start periodic measurement using current settings
    inline bool startPeriodicMeasurement()
    {
        return start_periodic_measurement();
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return stop_periodic_measurement();
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
    bool measureSingleshot(ads1110::Data& data, const ads1110::Sampling rate, const ads1110::PGA pga);
    //! @brief Measurement single shot using current settings
    bool measureSingleshot(ads1110::Data& data);
    ///@}

    /*!
      @brief General reset
      @details Reset using I2C general call
      @return True if successful
      @warning This is a reset by General command, the command is also sent to all devices with I2C connections
    */
    bool generalReset();

protected:
    bool read_config(uint8_t& v);
    bool write_config(const uint8_t v);
    bool read_measurement(uint8_t* v);
    bool is_data_ready();

    bool start_periodic_measurement(const ads1110::Sampling rate, const ads1110::PGA pga);
    bool start_periodic_measurement();
    bool stop_periodic_measurement();

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitADS1110, ads1110::Data);

private:
    std::unique_ptr<m5::container::CircularBuffer<ads1110::Data>> _data{};
    ads1110::PGA _pga{ads1110::PGA::Gain1};
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5

#endif

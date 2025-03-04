/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS11XX.hpp
  @brief Base class of ADS1100,ADS1110
*/
#ifndef M5_UNIT_ANADIG_UNIT_ADS11XX_HPP
#define M5_UNIT_ANADIG_UNIT_ADS11XX_HPP
#include <M5UnitComponent.hpp>
#include <m5_utility/stl/extension.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

/*!
  @namespace ads11xx
  @brief For ADS1100,ADS1110
 */
namespace ads11xx {

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
    uint8_t rate{};                //!< SPS (Value and content depend on derived class)
    PGA pga{};                     //!< PGA
    float vdd{2048.f};             //!< VDD(mV)
    float factor{1.0f};            //!< Correction factor

    ///! @brief Gets the differential value
    inline int16_t differentialValue() const
    {
        return (int16_t)m5::types::big_uint16_t(raw[0], raw[1]).get();
    }
    //! @brief Gets the differential voltage(mV)
    inline float differentialVoltage() const
    {
        return differentialValue() / (-min_code_table[rate & 0x03] / vdd * (1U << m5::stl::to_underlying(pga))) /
               factor;
    }
    static const int32_t min_code_table[4];
};

}  // namespace ads11xx

/*!
  @class UnitADS11XX
  @brief Base class of ADS1100,ADS1110
*/
class UnitADS11XX : public Component, public PeriodicMeasurementAdapter<UnitADS11XX, ads11xx::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitADS11XX, 0x00);

public:
    explicit UnitADS11XX(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<ads11xx::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitADS11XX()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

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
      @brief Read the PGA
      @param[out] pga PGA
      @return True if successful
     */
    bool readPGA(ads11xx::PGA& pga);
    /*!
      @brief Write the PGA
      @param pga PGA
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writePGA(const ads11xx::PGA pga);
    ///@}

    /*!
      @brief General reset
      @details Reset using I2C general call
      @return True if successful
      @warning This is a reset by General command, the command is also sent to all devices with I2C connections
    */
    virtual bool generalReset();

protected:
    bool start_periodic_measurement(const uint8_t cfg_value);
    bool start_periodic_measurement();
    bool stop_periodic_measurement();

    bool measure_singleshot(ads11xx::Data& data, const uint8_t cfg_value);
    bool measure_singleshot(ads11xx::Data& data);

    bool read_config(uint8_t& v);
    bool write_config(const uint8_t v);
    bool read_measurement(uint8_t v[2]);
    bool is_data_ready();

    virtual bool read_if_ready_in_periodic(uint8_t v[2]);
    virtual uint32_t get_interval(const uint8_t /* rate */)
    {
        return 0;
    }

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitADS11XX, ads11xx::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<ads11xx::Data>> _data{};
    ads11xx::PGA _pga{};
    uint8_t _rate{};
    float _vdd{2.048f};
    float _factor{1.0f};

    struct Config {
        inline uint8_t rate() const
        {
            return (value >> 2) & 0x03;
        }
        inline ads11xx::PGA pga() const
        {
            return static_cast<ads11xx::PGA>(value & 0x03);
        }
        inline bool continuous() const
        {
            return value & (1U << 4);
        }
        inline bool single() const
        {
            return !continuous();
        }
        inline bool st() const
        {
            // ADS1100 ST/BSY   Continuous: Always true, Single: False if data raedy
            // ADS1110 ST/DRDY  Continuous/Single: False if data ready
            return (value & 0x80);
        }

        inline void rate(const uint8_t rate)
        {
            value = (value & ~(0x03 << 2)) | ((rate & 0x03) << 2);
        }
        inline void pga(const ads11xx::PGA pga)
        {
            value = (value & ~0x03) | m5::stl::to_underlying(pga);
        }
        inline void continuous(bool enable)
        {
            value = (value & ~(1U << 4)) | ((enable ? 0 : 1) << 4);
        }
        inline void single(bool enable)
        {
            continuous(false);
        }
        inline void st(const bool b)
        {
            value = (value & ~0x80) | (b ? 0x80 : 0x00);
        }
        uint8_t value{};
    };
};
}  // namespace unit
}  // namespace m5
#endif

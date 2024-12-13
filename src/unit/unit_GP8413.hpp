/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_GP8413.hpp
  @brief GP8413 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_GP8413_HPP
#define M5_UNIT_ENV_UNIT_GP8413_HPP
#include <M5UnitComponent.hpp>

namespace m5 {
namespace unit {

/*!
  @namespace gp8413
  @brief For GP8413
*/
namespace gp8413 {

enum class Output : uint8_t {
    Range5V,   //!< 0 ~ 5V
    Range10V,  //!< 0 ~ 10V
};

enum class Channel : uint8_t {
    Zero,  //!< channel 0
    One,   //!< channel 1
};

}  // namespace gp8413

/*!
  @class UnitGP8413
  @brief Digital-to-analog signal conversion unit
*/
class UnitGP8413 : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitGP8413, 0x59);

public:
    static constexpr uint16_t RESOLUTION{0x7FFF};  // 15bits

    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Output range for channel 0
        gp8413::Output range0{gp8413::Output::Range10V};
        //! Output range for channel 1
        gp8413::Output range1{gp8413::Output::Range10V};
    };

    explicit UnitGP8413(const uint8_t addr = DEFAULT_ADDRESS) : Component(addr)
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitGP8413()
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
    //! @brief Gets the inner range
    gp8413::Output range(const gp8413::Channel channel) const
    {
        return _range[m5::stl::to_underlying(channel)];
    }
    //! @brief Get the maximum voltage of the channel
    float maximumVoltage(const gp8413::Channel channel) const;
    /*!
      @brief Write the output range
      @param  range0 Output range for channel 0
      @param  range1 Output range for channel 1
      @return True if successful
     */
    bool writeOutputRange(const gp8413::Output range0, const gp8413::Output range1);
    ///@}

    ///@name Output the voltage
    ///@{
    /*!
      @brief Output the voltage
      @param channel Channel to output
      @param mv Output voltage(mV)
      @return True if successful
      @Note If exceeding the range, it will be kept within the range
     */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeVoltage(const gp8413::Channel channel, const T mv)
    {
        return (mv >= 0.0f) && writeVoltage(channel, voltage_to_raw(channel, (float)mv));
    }
    //! @brief Output the voltage(mV) to channel 0
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeChannel0Voltage(const T mv)
    {
        return (mv >= 0.0f) && writeVoltage(gp8413::Channel::Zero, voltage_to_raw(gp8413::Channel::Zero, (float)mv));
    }
    //! @brief Output the voltage(mV) to channel 1
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeChannel1Voltage(const T mv)
    {
        return (mv >= 0.0f) && writeVoltage(gp8413::Channel::One, voltage_to_raw(gp8413::Channel::One, (float)mv));
    }
    /*!
      @brief Output the voltage to both channel
      @param mv0 Output voltage(mV) for channel 0
      @param mv1 Output voltage(mV) for channel 1
      @return True if successful
    */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeBothVoltage(const T mv0, const T mv1)
    {
        return writeBothVoltage(voltage_to_raw(gp8413::Channel::Zero, (float)mv0),
                                voltage_to_raw(gp8413::Channel::One, (float)mv1));
    }
    /*!
      @brief Output the voltage to both channel
      @param mv Output voltage(mV)
      @return True if successful
      @note Write the same value to both channels
    */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeBothVoltage(const T mv)
    {
        return (mv >= 0.0f) && writeBothVoltage(voltage_to_raw(gp8413::Channel::Zero, (float)mv),
                                                voltage_to_raw(gp8413::Channel::One, (float)mv));
    }
    ///@}

    ///@name Output the raw value
    ///@{
    /*!
      @brief Output the raw value
      @param channel Channel to output
      @param raw Output raw value
      @return True if successful
     */
    bool writeVoltage(const gp8413::Channel channel, const uint16_t raw);

    //! @brief Output the raw value
    template <typename T, typename std::enable_if<!std::is_same<uint16_t, T>::value && std::is_unsigned<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    inline bool writeVoltage(const gp8413::Channel channel, const T raw)
    {
        return writeVoltage(channel, static_cast<uint16_t>(raw & RESOLUTION));
    }
    //! @brief Output the raw value to channel 0
    template <typename T, typename std::enable_if<!std::is_same<uint16_t, T>::value && std::is_unsigned<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    inline bool writeChannel0Voltage(const T raw)
    {
        return writeVoltage(gp8413::Channel::Zero, static_cast<uint16_t>(raw & RESOLUTION));
    }
    //! @brief Output the raw value to channel 1
    template <typename T, typename std::enable_if<!std::is_same<uint16_t, T>::value && std::is_unsigned<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    inline bool writeChannel1Voltage(const T raw)
    {
        return writeVoltage(gp8413::Channel::One, static_cast<uint16_t>(raw & RESOLUTION));
    }
    /*!
      @brief Output the raw value to both channel
      @param raw0 Output raw value for channel 0
      @param raw1 Output raw value for channel 1
      @return True if successful
    */
    bool writeBothVoltage(const uint16_t raw0, const uint16_t raw1);
    //! @brief Output the raw value to both channel
    template <typename T, typename std::enable_if<!std::is_same<uint16_t, T>::value && std::is_unsigned<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    inline bool writeBothVoltage(const T raw0, const T raw1)
    {
        return writeBothVoltage(static_cast<uint16_t>(raw0 & RESOLUTION), static_cast<uint16_t>(raw1 & RESOLUTION));
    }
    /*!
      @brief Output the raw value to both channel
      @param raw Output raw value
      @return True if successful
      @note Write the same value to both channels
    */
    template <typename T, typename std::enable_if<!std::is_same<uint16_t, T>::value && std::is_unsigned<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    inline bool writeBothVoltage(const T raw)
    {
        return writeBothVoltage(static_cast<uint16_t>(raw & RESOLUTION), static_cast<uint16_t>(raw & RESOLUTION));
    }
    ///@}

protected:
    uint16_t voltage_to_raw(const gp8413::Channel channel, const float mv);
    bool write_voltage(const uint8_t reg, const uint8_t* buf, const uint32_t len);

private:
    gp8413::Output _range[2]{};
    config_t _cfg{};
};

///@cond
namespace gp8413 {
namespace command {

constexpr uint8_t OUTPUT_RANGE{0x01};
constexpr uint8_t OUTPUT_CHANNEL_0{0x02};
constexpr uint8_t OUTPUT_CHANNEL_1{0x04};

}  // namespace command
}  // namespace gp8413
///@endcond

}  // namespace unit
}  // namespace m5

#endif

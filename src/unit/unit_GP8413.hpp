/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_GP8413.hpp
  @brief GP8413 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ANADIG_UNIT_GP8413_HPP
#define M5_UNIT_ANADIG_UNIT_GP8413_HPP
#include <M5UnitComponent.hpp>

namespace m5 {
namespace unit {

/*!
  @namespace gp8413
  @brief For GP8413
*/
namespace gp8413 {

/*!
  @enum Output
  @brief Output voltage range
 */
enum class Output : uint8_t {
    Range5V,   //!< 0 ~ 5V
    Range10V,  //!< 0 ~ 10V
};

/*!
  @enum Channel
  @brief Output channel
 */
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
    //! @brief Maximum raw output value (15 bits)
    static constexpr uint16_t RESOLUTION{0x7FFF};

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

    /*!
      @brief Constructor
      @param addr I2C address
     */
    explicit UnitGP8413(const uint8_t addr = DEFAULT_ADDRESS) : Component(addr)
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitGP8413()
    {
    }

    /*!
      @brief Initialize the unit and apply config_t (output ranges)
      @return True if successful
     */
    virtual bool begin() override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configuration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configuration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@name Settings
    ///@{
    /*!
      @brief Gets the output range set for the channel
      @param channel Target channel
      @return Output range
     */
    gp8413::Output range(const gp8413::Channel channel) const
    {
        return _range[m5::stl::to_underlying(channel)];
    }
    /*!
      @brief Gets the maximum voltage of the channel
      @param channel Target channel
      @return Maximum voltage (mV) of the current output range
     */
    float maximumVoltage(const gp8413::Channel channel) const;
    /*!
      @brief Write the output range
      @param  range0 Output range to channel 0
      @param  range1 Output range to channel 1
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
      @note Negative values return false. Values exceeding the output range are clamped
     */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeVoltage(const gp8413::Channel channel, const T mv)
    {
        return (mv >= 0.0f) && writeVoltage(channel, voltage_to_raw(channel, static_cast<float>(mv)));
    }
    /*!
      @brief Output the voltage to channel 0
      @param mv Output voltage(mV)
      @return True if successful
      @note Negative values return false. Values exceeding the output range are clamped
     */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeChannel0Voltage(const T mv)
    {
        return (mv >= 0.0f) &&
               writeVoltage(gp8413::Channel::Zero, voltage_to_raw(gp8413::Channel::Zero, static_cast<float>(mv)));
    }
    /*!
      @brief Output the voltage to channel 1
      @param mv Output voltage(mV)
      @return True if successful
      @note Negative values return false. Values exceeding the output range are clamped
     */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeChannel1Voltage(const T mv)
    {
        return (mv >= 0.0f) &&
               writeVoltage(gp8413::Channel::One, voltage_to_raw(gp8413::Channel::One, static_cast<float>(mv)));
    }
    /*!
      @brief Output the voltage to both channels
      @param mv0 Output voltage(mV) to channel 0
      @param mv1 Output voltage(mV) to channel 1
      @return True if successful
      @note Negative values return false. Values exceeding the output range are clamped
    */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeBothVoltage(const T mv0, const T mv1)
    {
        return (mv0 >= 0.0f) && (mv1 >= 0.0f) &&
               writeBothVoltage(voltage_to_raw(gp8413::Channel::Zero, static_cast<float>(mv0)),
                                voltage_to_raw(gp8413::Channel::One, static_cast<float>(mv1)));
    }
    /*!
      @brief Output the voltage to both channels
      @param mv Output voltage(mV) to both channels
      @return True if successful
      @note Negative values return false. Values exceeding the output range are clamped
    */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeBothVoltage(const T mv)
    {
        return (mv >= 0.0f) && writeBothVoltage(voltage_to_raw(gp8413::Channel::Zero, static_cast<float>(mv)),
                                                voltage_to_raw(gp8413::Channel::One, static_cast<float>(mv)));
    }
    ///@}

    ///@name Output the raw value
    ///@{
    /*!
      @brief Output the raw value
      @param channel Channel to output
      @param raw Output raw value (masked with RESOLUTION)
      @return True if successful
     */
    bool writeVoltage(const gp8413::Channel channel, const uint16_t raw);
    /*!
      @brief Output the raw value to channel 0
      @param raw Output raw value (masked with RESOLUTION)
      @return True if successful
     */
    inline bool writeChannel0Voltage(const uint16_t raw)
    {
        return writeVoltage(gp8413::Channel::Zero, static_cast<uint16_t>(raw & RESOLUTION));
    }
    /*!
      @brief Output the raw value to channel 1
      @param raw Output raw value (masked with RESOLUTION)
      @return True if successful
     */
    inline bool writeChannel1Voltage(const uint16_t raw)
    {
        return writeVoltage(gp8413::Channel::One, static_cast<uint16_t>(raw & RESOLUTION));
    }
    /*!
      @brief Output the raw value to both channels
      @param raw0 Output raw value to channel 0 (masked with RESOLUTION)
      @param raw1 Output raw value to channel 1 (masked with RESOLUTION)
      @return True if successful
    */
    bool writeBothVoltage(const uint16_t raw0, const uint16_t raw1);
    /*!
      @brief Output the raw value to both channels
      @param raw Output raw value to both channels (masked with RESOLUTION)
      @return True if successful
    */
    inline bool writeBothVoltage(const uint16_t raw)
    {
        return writeBothVoltage(static_cast<uint16_t>(raw & RESOLUTION), static_cast<uint16_t>(raw & RESOLUTION));
    }
    ///@}

    ///@note The GP8413 supports storing voltage data in the chip to ensure that the voltage output state remains
    /// in the corresponding state after power-down or start-up.
    ///@name Store output voltage
    ///@{
    /*!
      @brief Store the current output voltage to the chip
      @details The stored voltage is output when the chip is powered on (until the output is written again).
      The store sequence cannot be generated by the I2C controller, so the I2C bus is released and the sequence
      is bit-banged on the SDA/SCL pins, then the bus is restored.
      Supported buses: Arduino TwoWire, m5::I2C_Class (M5.Ex_I2C etc.), M5HAL software I2C,
      and via a hub (PaHub etc.) connected to one of these
      @return True if successful
      @warning Not supported on the ESP-IDF native I2C drivers and on the ESP32-C6 LP I2C (e.g. NessoN1 Hat
      port) (returns false)
      @warning Other devices on the same bus cannot be accessed during the store (about 10 ms). Blocked until complete
      @warning Note that the output range is not stored
      @note The chip has no read command, so the stored value can only be checked by power-cycling the unit
      and measuring the output
     */
    bool storeBothVoltage();
    ///@}

protected:
    uint16_t voltage_to_raw(const gp8413::Channel channel, const float mv) const;
    bool write_voltage(const uint8_t reg, const uint8_t* buf, const uint32_t len);

private:
    gp8413::Output _range[2]{};
    config_t _cfg{};
};

namespace gp8413 {
///@cond
namespace command {
constexpr uint8_t OUTPUT_RANGE_REG{0x01};     // W
constexpr uint8_t OUTPUT_CHANNEL0_REG{0x02};  // W
constexpr uint8_t OUTPUT_CHANNEL1_REG{0x04};  // W
}  // namespace command
///@endcond
}  // namespace gp8413

}  // namespace unit
}  // namespace m5
#endif

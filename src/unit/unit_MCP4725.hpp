/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_MCP4725.hpp
  @brief MCP4725 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ANADIG_UNIT_MCP4725_HPP
#define M5_UNIT_ANADIG_UNIT_MCP4725_HPP
#include <M5UnitComponent.hpp>

namespace m5 {
namespace unit {

/*!
  @namespace mcp4725
  @brief For MCP4725
*/
namespace mcp4725 {

enum class PowerDown : uint8_t {
    Normal,    //!< Normal mode
    OHM_1K,    //!< 1k ohm  resistor to ground
    OHM_100K,  //!< 100k ohm resistor to ground
    OHM_500K   //!< 500k ohm resistor to ground
};

}  // namespace mcp4725

/*!
  @class UnitMCP4725
  @brief Digital-to-analog signal conversion unit
*/
class UnitMCP4725 : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitMCP4725, 0x60);

public:
    static constexpr uint16_t RESOLUTION{0x0FFF};    // 12bits
    static constexpr float MAXIMUM_VOLTAGE{3300.f};  // mV

    //! @brief Raw value to voltage(mV)
    static inline float raw_to_voltage(const uint16_t raw, const float supply_voltage = 5000.f)
    {
        return static_cast<float>(raw) * supply_voltage / RESOLUTION;
    }
    //! @brief Voltage(mV) to raw value
    static inline uint16_t voltage_to_raw(const float mv, const float supply_voltage = 5000.f)
    {
        float val = std::fmin(std::fmax(mv, 0.0f), MAXIMUM_VOLTAGE);
        return static_cast<uint16_t>((val / supply_voltage) * RESOLUTION);
    }

    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Using EEPROM settings on begin?
        bool using_eeprom_settings{false};
        //! Voltage supplied mV (Used to calculate output values)
        float supply_voltage{5000.f};
    };

    explicit UnitMCP4725(const uint8_t addr = DEFAULT_ADDRESS) : Component(addr)
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitMCP4725()
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

    ///@name Properties
    ///@{
    //! @brief Gets the iner power down mode
    inline mcp4725::PowerDown powerDown() const
    {
        return _powerDown;
    }
    //! @brief Gets the last output raw value
    inline uint16_t lastValue() const
    {
        return _lastValue;
    }
    ///@}

    ///@name Settings
    ///@{
    /*!
      @brief Write the power down mode
      @param pd Mode
      @return True if successful
      @note Output voltage is not changed
     */
    bool writePowerDown(const mcp4725::PowerDown pd);
    ///@}

    ///@note FastMode is used, so EEPROM is not affected
    ///@name Output the voltage
    ///@{
    /*!
       @brief Output the voltage
       @param mv Output voltage(mV)
       @return True if successful
       @Note If exceeding the range, it will be kept within the range
      */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeVoltage(const T mv)
    {
        return (mv >= 0.0f) && writeVoltage(voltage_to_raw((float)mv, _cfg.supply_voltage));
    }
    /*!
       @brief Output the voltage
       @param raw Output raw value
        @return True if successful
       @Note If exceeding the range, it will be kept within the range
      */
    inline bool writeVoltage(const uint16_t raw)
    {
        return write_voltage(Command::FastMode, raw);
    }
    //! @brief Output the voltage
    template <typename T, typename std::enable_if<!std::is_same<uint16_t, T>::value && std::is_unsigned<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    inline bool writeVoltage(const T raw)
    {
        return writeVoltage(static_cast<uint16_t>(raw & RESOLUTION));
    }

    ///@}

    //! @note After a reset, the device uploads the contents of the EEPROM to the DAC register
    ///@name Write to DAC register and EEPROM
    ///@{
    /*!
      @brief Write to DAC register and EEPROM
      @param mv Output voltage(mV)
      @param blocking Wait until EEPROM write is complete if true
      @return True if successful
      @Note If exceeding the range, it will be kept within the range
     */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeVoltageAndEEPROM(const T mv, const bool blocking = true)
    {
        return (mv >= 0.0f) && writeVoltageAndEEPROM(voltage_to_raw((float)mv, _cfg.supply_voltage), blocking);
    }

    /*!
      @brief Write to DAC register and EEPROM
      @param raw Output raw value
      @param blocking Wait until EEPROM write is complete if true
      @return True if successful
     */
    bool writeVoltageAndEEPROM(const uint16_t raw, const bool blocking = true);

    //! @brief Write to DAC register and EEPROM
    template <typename T, typename std::enable_if<!std::is_same<uint16_t, T>::value && std::is_unsigned<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    bool writeVoltageAndEEPROM(const T raw, const bool blocking = true)
    {
        return writeVoltageAndEEPROM(static_cast<uint16_t>(raw & RESOLUTION), blocking);
    }
    ///@}

    /*!
      @brief General reset
      @details Reset using I2C general call
      @return True if successful
      @note Immediately after this reset event, the deviceuploads the contents of the EEPROM into the DACregister
      @warning This is a reset by General command, the command is also sent to all devices with I2C connections
    */
    bool generalReset();

    /*!
      @brief Read the DAC register
      @param[out] pd Power down mode
      @param[out] raw Output voltage raw value
      @return True if successful
     */
    bool readDACRegister(mcp4725::PowerDown& pd, uint16_t& raw);
    /*!
      @brief Read the EEPROM settings
      @param[out] pd Power down mode
      @param[out] raw Output voltage raw value
      @return True if successful
     */
    bool readEEPROM(mcp4725::PowerDown& pd, uint16_t& raw);

protected:
    enum class Command : uint8_t {
        FastMode,           // 2bytes [0:0:PD1:PD0:D11:D10:D9:D8] [D7... D0]
        WriteDAC,           // 3bytes [0:1:0:X:X:PD1:PD0:X]       [D11...D4] [D3:D2:D1:D0:X:X:X:X]
        WriteDACAndEEPROM,  // 3bytes [0:1:1:X:X:PD1:PD0:X]       [D11...D4] [D3:D2:D1:D0:X:X:X:X]
    };

    bool write_voltage(const Command cmd, const uint16_t raw);
    bool is_eeprom_ready();
    uint32_t make_buffer(uint8_t buf[3], const uint16_t raw, const Command cmd);
    bool read_status(uint8_t rbuf[5]);

private:
    mcp4725::PowerDown _powerDown{};
    uint16_t _lastValue{};
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5

#endif

# Map the Kconfig "Target unit" choice (common/Kconfig.variant.*) to the source-level macro that
# both the Arduino and ESP-IDF builds use. Include this from an example's main/CMakeLists.txt
# *after* idf_component_register() (it needs ${COMPONENT_LIB}). Only the examples that have a
# variant (ADCInput, DACOutput) include this file; each of them rsource's exactly one
# Kconfig.variant.*, so only one of the choices below is ever defined.
set(M5UNIT_VARIANT "")
# ADCInput (common/Kconfig.variant.adc)
if(CONFIG_EXAMPLE_USING_UNIT_ADC11)
    set(M5UNIT_VARIANT USING_UNIT_ADC11)
elseif(CONFIG_EXAMPLE_USING_HAT_ADC11)
    set(M5UNIT_VARIANT USING_HAT_ADC11)
elseif(CONFIG_EXAMPLE_USING_HAT_ADC)
    set(M5UNIT_VARIANT USING_HAT_ADC)
# DACOutput (common/Kconfig.variant.dac)
elseif(CONFIG_EXAMPLE_USING_UNIT_DAC)
    set(M5UNIT_VARIANT USING_UNIT_DAC)
elseif(CONFIG_EXAMPLE_USING_UNIT_DAC2)
    set(M5UNIT_VARIANT USING_UNIT_DAC2)
elseif(CONFIG_EXAMPLE_USING_HAT_DAC)
    set(M5UNIT_VARIANT USING_HAT_DAC)
elseif(CONFIG_EXAMPLE_USING_HAT_DAC2)
    set(M5UNIT_VARIANT USING_HAT_DAC2)
endif()
if(M5UNIT_VARIANT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ${M5UNIT_VARIANT})
endif()

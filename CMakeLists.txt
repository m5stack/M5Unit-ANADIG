# ESP-IDF component manifest for M5Unit-ANADIG.
# Upstream: https://github.com/m5stack/M5Unit-ANADIG
#
# The public headers pull in both M5UnitUnified and M5Utility,
# because the periodic measurement data is held in m5::container::CircularBuffer.
# M5HAL is private: unit_GP8413.cpp bit-bangs the store sequence with its GPIO layer.
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified M5Utility
    PRIV_REQUIRES M5HAL
)

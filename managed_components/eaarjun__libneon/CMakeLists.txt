# IDF v6.0+ split the monolithic "driver" into per-peripheral components
if(IDF_VERSION_MAJOR GREATER_EQUAL 6)
    set(NEO_DRIVER_DEPS
        esp_driver_rmt
        esp_driver_gptimer
        esp_hal_gpio
        freertos
        log
    )
else()
    set(NEO_DRIVER_DEPS
        driver
        esp_timer
        freertos
        log
    )
endif()

file(GLOB_RECURSE component_srcs "src/*.cpp")

idf_component_register(
    SRCS ${component_srcs}
    INCLUDE_DIRS "include"
    REQUIRES ${NEO_DRIVER_DEPS}
)

# C++20 required for std::bit_cast, concepts, and std::ranges
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)

# Suppress warnings that are common with ESP-IDF + modern C++
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=narrowing
    -Wno-error=missing-field-initializers
)

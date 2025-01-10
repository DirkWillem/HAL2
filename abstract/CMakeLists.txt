add_library(abstract_hal INTERFACE
        hal/callback.h
        hal/clocks.h
        hal/dma.h
        hal/i2c.h
        hal/i2s.h
        hal/peripheral.h
        hal/pin.h
        hal/spi.h
        hal/system.h
        hal/tim.h
        hal/uart.h)

target_include_directories(abstract_hal INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(abstract_hal INTERFACE
        constexpr_tools)

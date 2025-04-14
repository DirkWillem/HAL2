add_library(abstract_hal INTERFACE
        hal/clocks.h
        hal/dma.h
        hal/i2c.h
        hal/i2s.h
        hal/peripheral.h
        hal/pin.h
        hal/pin_interrupts.h
        hal/spi.h
        hal/system.h
        hal/tim.h
        hal/uart.h)

target_include_directories(abstract_hal INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(abstract_hal INTERFACE
        constexpr_tools halstd)


add_library(hal_abstract)


target_sources(hal_abstract
        PUBLIC
        FILE_SET CXX_MODULES FILES
        hal_mod/clocks.cppm
        hal_mod/dma.cppm
        hal_mod/i2c.cppm
        hal_mod/i2s.cppm
        hal_mod/peripheral.cppm
        hal_mod/pin.cppm
        hal_mod/pin_interrupts.cppm
        hal_mod/spi.cppm
        hal_mod/system.cppm
        hal_mod/tim.cppm
        hal_mod/uart.cppm

        hal_mod/hal_abstract.cppm)

target_link_libraries(hal_abstract PRIVATE project_settings hstd)
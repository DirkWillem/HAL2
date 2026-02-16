# HAL concepts library.
add_library(hal_abstract)
target_sources(hal_abstract
        PUBLIC
        FILE_SET CXX_MODULES FILES
        abstract/clocks.cppm
        abstract/dma.cppm
        abstract/i2c.cppm
        abstract/i2s.cppm
        abstract/peripheral.cppm
        abstract/pin.cppm
        abstract/pin_interrupts.cppm
        abstract/spi.cppm
        abstract/system.cppm
        abstract/tim.cppm
        abstract/uart.cppm

        abstract/hal_abstract.cppm)

target_link_libraries(hal_abstract PRIVATE project_settings hstd rtos_concepts)

# HAL USB abstract library
add_library(hal_usb_abstract STATIC)
target_sources(hal_usb_abstract
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        usb_abstract/usb_abstract.cppm)
target_link_libraries(hal_usb_abstract PUBLIC hstd)
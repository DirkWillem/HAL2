add_library(abstract_hal INTERFACE
        hal/callback.h
        hal/dma.h
        hal/peripheral.h
        hal/pin.h
        hal/uart.h)

target_include_directories(abstract_hal INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(abstract_hal INTERFACE
        constexpr_tools)

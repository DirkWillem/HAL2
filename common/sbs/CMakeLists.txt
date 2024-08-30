add_library(sbs INTERFACE
        sbs.h
        sbs_type_name.h)

target_link_libraries(sbs INTERFACE fp)
target_include_directories(sbs INTERFACE "${CMAKE_CURRENT_LIST_DIR}/..")

add_library(sbs_uart INTERFACE
        uart/detail/slot.h

        uart/sbs_uart.h)

target_link_libraries(sbs_uart INTERFACE sbs abstract_hal)
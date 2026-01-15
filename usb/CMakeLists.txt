

function(add_hal_usb_lib VARIANT TINYUSB_TARGET)
    add_library(hal_usb_${VARIANT} STATIC)

    target_sources(hal_usb_${VARIANT}
            PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
            FILES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/usb.cppm)
    target_link_libraries(hal_usb_${VARIANT} PUBLIC
            hstd
            ${TINYUSB_TARGET}
            rtos_concepts
            freertos_kernel_include
            freertos_kernel_port_headers)
endfunction()
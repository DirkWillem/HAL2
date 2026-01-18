add_library(hal_usb_abstract STATIC)
target_sources(hal_usb_abstract
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        usb_abstract.cppm)
target_link_libraries(hal_usb_abstract PUBLIC hstd)

function(add_hal_usb_lib VARIANT TINYUSB_TARGET)
    add_library(hal_usb_${VARIANT} STATIC)

    target_sources(hal_usb_${VARIANT}
            PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
            FILES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/usb.cppm

            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/concepts.cppm
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/helpers.cppm
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/functions/cdc.cppm)

    target_sources(hal_usb_${VARIANT}
            PUBLIC
            FILE_SET callback_hdr
            TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include"
            FILES

            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include/usb_callbacks.h")
    target_link_libraries(hal_usb_${VARIANT} PUBLIC
            hstd
            ${TINYUSB_TARGET}
            rtos_concepts
            freertos_kernel_include
            freertos_kernel_port_headers)
endfunction()
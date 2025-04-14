find_program(STM32_HAL_TOOLS_EXE "stm32_hal_tools" REQUIRED)

function(add_generated_file FAMILY VARIANT GEN_TYPE XML_PATH OUT_PATH)
    add_custom_command(
            OUTPUT "${OUT_PATH}"
            DEPENDS "${XML_PATH}"
            COMMAND "${STM32_HAL_TOOLS_EXE}" --gen "${GEN_TYPE}" --family "${FAMILY}" --mcu "${VARIANT}" "'${XML_PATH}'" "${OUT_PATH}"
    )
endfunction()

function(add_stm32g0_hal VARIANT XMLFILE)
    # Add generated code
    add_generated_file(
            stm32g0
            "${VARIANT}"
            "uart-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.h")

    add_generated_file(
            stm32g0
            "${VARIANT}"
            "spi-i2s-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.h")

    add_generated_file(
            stm32g0
            "${VARIANT}"
            "tim-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.h")

    # Add library
    add_library("${VARIANT}_hal"
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.h"
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.h"
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.h"

            stm32g0/tim/channel.h
            stm32g0/tim/common.cpp
            stm32g0/tim/common.h

            stm32g0/clocks.h
            stm32g0/dma.h
            stm32g0/dma.cpp
            stm32g0/i2s.cpp
            stm32g0/i2s.h
            stm32g0/interrupts.h
            stm32g0/peripheral_ids.h
            stm32g0/pin.cpp
            stm32g0/pin.h
            stm32g0/pin_interrupt.cpp
            stm32g0/pin_interrupt.h
            stm32g0/spi.cpp
            stm32g0/spi.h
            stm32g0/system.cpp
            stm32g0/system.h
            stm32g0/tim.h
            stm32g0/uart.cpp
            stm32g0/uart.h)

    target_link_libraries("${VARIANT}_hal" PUBLIC abstract_hal "${VARIANT}_vendor" project_settings)
    target_include_directories("${VARIANT}_hal" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}")

    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/stm32g0_mod/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.cppm")
        add_generated_file(
                stm32g0
                "${VARIANT}"
                "uart-pin-mapping-module"
                "${XMLFILE}"
                "${CMAKE_CURRENT_LIST_DIR}/stm32g0_mod/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.cppm")
    endif()

    add_library("${VARIANT}_hal_mod")
    target_sources("${VARIANT}_hal_mod"
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0_mod/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.cppm"

            stm32g0_mod/clocks.cppm
            stm32g0_mod/dma.cppm
            stm32g0_mod/peripherals.cppm
            stm32g0_mod/pin.cppm
            stm32g0_mod/system.cppm
            stm32g0_mod/uart.cppm

            stm32g0_mod/stm32g0.cppm

            PUBLIC
            FILE_SET pub_headers TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/stm32g0_mod"
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0_mod/pin_macros.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0_mod/interrupts.h"
            PRIVATE
            FILE_SET gen_headers TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}"
            FILES
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.h"
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.h"
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.h"
    )

    target_link_libraries("${VARIANT}_hal_mod"
            PUBLIC
            "${VARIANT}_vendor"

            PRIVATE
            hstd
            hal_abstract
            project_settings)

endfunction()

function(add_stm32g4_hal VARIANT XMLFILE)
    # Add generated code
    add_generated_file(
            stm32g4
            "${VARIANT}"
            "uart-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/uart_pin_mapping.h")
    add_generated_file(
            stm32g4
            "${VARIANT}"
            "spi-i2s-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/spi_i2s_pin_mapping.h")
    add_generated_file(
            stm32g4
            "${VARIANT}"
            "i2c-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/i2c_pin_mapping.h")

    # Add library
    add_library("${VARIANT}_hal"
            "stm32g4/generated/${VARIANT}/stm32g4/mappings/uart_pin_mapping.h"
            "stm32g4/generated/${VARIANT}/stm32g4/mappings/spi_i2s_pin_mapping.h"
            "stm32g4/generated/${VARIANT}/stm32g4/mappings/i2c_pin_mapping.h"

            stm32g4/internal/i2c_timing.h

            stm32g4/clocks.cpp
            stm32g4/clocks.h
            stm32g4/cordic.h
            stm32g4/dma.cpp
            stm32g4/dma.h
            stm32g4/i2c.cpp
            stm32g4/i2c.h
            stm32g4/interrupts.h
            stm32g4/peripheral_ids.h
            stm32g4/pin.cpp
            stm32g4/pin.h
            stm32g4/spi.cpp
            stm32g4/spi.h
            stm32g4/system.cpp
            stm32g4/system.h
            stm32g4/uart.cpp
            stm32g4/uart.h
    )

    target_link_libraries("${VARIANT}_hal" PUBLIC abstract_hal "${VARIANT}_vendor" project_settings)
    target_include_directories("${VARIANT}_hal" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}")
endfunction()


function(add_stm32h5_hal VARIANT XMLFILE)
    # Add generated code
    add_generated_file(
            stm32h5
            "${VARIANT}"
            "uart-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/uart_pin_mapping.h")
    add_generated_file(
            stm32h5
            "${VARIANT}"
            "spi-i2s-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/spi_i2s_pin_mapping.h")
    add_generated_file(
            stm32h5
            "${VARIANT}"
            "usb-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/usb_pin_mapping.h")


    # Add library
    add_library("${VARIANT}_hal"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/uart_pin_mapping.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/spi_i2s_pin_mapping.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/usb_pin_mapping.h"

            stm32h5/clocks.h
            stm32h5/dma.cpp
            stm32h5/dma.h
            stm32h5/interrupts.h
            stm32h5/peripheral_ids.h
            stm32h5/pin.cpp
            stm32h5/pin.h
            stm32h5/uart.cpp
            stm32h5/uart.h
    )

    target_link_libraries("${VARIANT}_hal" PUBLIC abstract_hal "${VARIANT}_vendor" project_settings)
    target_include_directories("${VARIANT}_hal" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}")
endfunction()

function(add_stm32h7_hal VARIANT XMLFILE)
    # Add generated code
    add_generated_file(
            stm32h7
            "${VARIANT}"
            "uart-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h7/generated/${VARIANT}/stm32h7/mappings/uart_pin_mapping.h")
    add_generated_file(
            stm32h7
            "${VARIANT}"
            "spi-i2s-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h7/generated/${VARIANT}/stm32h7/mappings/spi_i2s_pin_mapping.h")
    add_generated_file(
            stm32h7
            "${VARIANT}"
            "i2c-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h7/generated/${VARIANT}/stm32h7/mappings/i2c_pin_mapping.h")

    # Add library
    add_library("${VARIANT}_hal"
            "stm32h7/generated/${VARIANT}/stm32h7/mappings/uart_pin_mapping.h"
            "stm32h7/generated/${VARIANT}/stm32h7/mappings/spi_i2s_pin_mapping.h"
            "stm32h7/generated/${VARIANT}/stm32h7/mappings/i2c_pin_mapping.h"

            stm32h7/boot.cpp
            stm32h7/boot.h
            stm32h7/clocks.h
            stm32h7/core.h
            stm32h7/dma.cpp
            stm32h7/dma.h
            stm32h7/hardware_semaphore.cpp
            stm32h7/hardware_semaphore.h
            stm32h7/interrupts.h
            stm32h7/memory_config.cpp
            stm32h7/memory_config.h
            stm32h7/peripheral_ids.h
            stm32h7/pin.cpp
            stm32h7/pin.h
            stm32h7/power.h
            stm32h7/spi.cpp
            stm32h7/spi.h
            stm32h7/uart.cpp
            stm32h7/uart.h
    )

    target_link_libraries("${VARIANT}_hal" PUBLIC abstract_hal "${VARIANT}_vendor" project_settings)
    target_include_directories("${VARIANT}_hal" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h7/generated/${VARIANT}")
endfunction()


if ("${TARGET}" STREQUAL "arm-cortex-m0plus")
    add_stm32g0_hal(stm32g0b1 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32G0B1R(B-C-E)Tx.xml")
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m4")
    add_stm32g4_hal(stm32g474 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32G474R(B-C-E)Tx.xml")

    add_multicore_linkopts(stm32h755 cm4)
    add_stm32h7_hal(stm32h755 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32H755ZITx.xml")
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m7")
    add_multicore_linkopts(stm32h755 cm7)
    add_stm32h7_hal(stm32h755 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32H755ZITx.xml")
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m33")
    add_stm32h5_hal(stm32h533 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32H533RETx.xml")
endif ()
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

    # Add library
    add_library("${VARIANT}_hal"
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.h"
            "stm32g0/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.h"

            stm32g0/dma.h
            stm32g0/dma.cpp
            stm32g0/i2s.cpp
            stm32g0/i2s.h
            stm32g0/interrupts.h
            stm32g0/pin.cpp
            stm32g0/pin.h
            stm32g0/spi.cpp
            stm32g0/spi.h
            stm32g0/uart.cpp
            stm32g0/uart.h)

    target_link_libraries("${VARIANT}_hal" PUBLIC abstract_hal "${VARIANT}_vendor")
    target_include_directories("${VARIANT}_hal" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}")
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

    target_link_libraries("${VARIANT}_hal" PUBLIC abstract_hal "${VARIANT}_vendor")
    target_include_directories("${VARIANT}_hal" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}")
endfunction()


if ("${TARGET}" STREQUAL "arm-cortex-m0plus")
    add_stm32g0_hal(stm32g0b1 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32G0B1R(B-C-E)Tx.xml")
endif()

if ("${TARGET}" STREQUAL "arm-cortex-m4")
    add_stm32g4_hal(stm32g474 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32G474R(B-C-E)Tx.xml")
endif()
find_program(STM32_HAL_TOOLS_EXE "stm32_hal_tools" REQUIRED)

function(add_generated_file FAMILY VARIANT GEN_TYPE XML_PATH OUT_PATH)
    add_custom_command(
            OUTPUT "${OUT_PATH}"
            DEPENDS "${XML_PATH}"
            COMMAND "${STM32_HAL_TOOLS_EXE}" --gen "${GEN_TYPE}" --family "${FAMILY}" --mcu "${VARIANT}" "'${XML_PATH}'" "${OUT_PATH}"
    )
endfunction()

function(generate_pin_mapping FAMILY VARIANT GEN_TYPE XML_PATH OUT_PATH)
    if (NOT VENV_DIR)
        message(FATAL_ERROR "VENV_DIR is not set, cannot generate pin mappings")
    endif ()

    set(GEN_PIN_MAPPING "${VENV_DIR}/bin/hal2.generate_stm32_pin_mapping")

    add_custom_command(
            OUTPUT "${OUT_PATH}"
            DEPENDS "${XML_PATH}"
            COMMAND "${GEN_PIN_MAPPING}" --gen "${GEN_TYPE}" --mcu-family "${FAMILY}" --mcu "${VARIANT}" --xml-file "'${XML_PATH}'" --dst-file "${OUT_PATH}"
    )
endfunction()

function(add_stm32g0_hal VARIANT XMLFILE)
    # Add generated code
    add_generated_file(
            stm32g0
            "${VARIANT}"
            "uart-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0_old/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.h")

    add_generated_file(
            stm32g0
            "${VARIANT}"
            "spi-i2s-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0_old/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.h")

    add_generated_file(
            stm32g0
            "${VARIANT}"
            "tim-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0_old/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.h")

    # Add library
    add_library("${VARIANT}_hal_old"
            "stm32g0_old/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.h"
            "stm32g0_old/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.h"
            "stm32g0_old/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.h"

            stm32g0_old/tim/channel.h
            stm32g0_old/tim/common.cpp
            stm32g0_old/tim/common.h

            stm32g0_old/clocks.h
            stm32g0_old/dma.h
            stm32g0_old/dma.cpp
            stm32g0_old/i2s.cpp
            stm32g0_old/i2s.h
            stm32g0_old/interrupts.h
            stm32g0_old/peripheral_ids.h
            stm32g0_old/pin.cpp
            stm32g0_old/pin.h
            stm32g0_old/pin_interrupt.cpp
            stm32g0_old/pin_interrupt.h
            stm32g0_old/spi.cpp
            stm32g0_old/spi.h
            stm32g0_old/system.cpp
            stm32g0_old/system.h
            stm32g0_old/tim.h
            stm32g0_old/uart.cpp
            stm32g0_old/uart.h)

    target_link_libraries("${VARIANT}_hal_old" PUBLIC abstract_hal "${VARIANT}_vendor" project_settings)
    target_include_directories("${VARIANT}_hal_old" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0_old/generated/${VARIANT}")

    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.cppm")
        add_generated_file(
                stm32g0
                "${VARIANT}"
                "uart-pin-mapping-module"
                "${XMLFILE}"
                "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.cppm")
    endif ()

    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.cppm")
        add_generated_file(
                stm32g0
                "${VARIANT}"
                "spi-i2s-pin-mapping-module"
                "${XMLFILE}"
                "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.cppm")
    endif ()

    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.cppm")
        add_generated_file(
                stm32g0
                "${VARIANT}"
                "tim-pin-mapping-module"
                "${XMLFILE}"
                "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.cppm")
    endif ()

    add_library("${VARIANT}_hal")
    target_sources("${VARIANT}_hal"
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/uart_pin_mapping.cppm"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/tim_pin_mapping.cppm"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/generated/${VARIANT}/stm32g0/mappings/spi_i2s_pin_mapping.cppm"

            stm32g0/tim/channel.cppm
            stm32g0/tim/common.cppm
            stm32g0/tim/tim.cppm

            stm32g0/clocks.cppm
            stm32g0/dma.cppm
            stm32g0/peripherals.cppm
            stm32g0/pin_interrupt.cppm
            stm32g0/pin.cppm
            stm32g0/spi.cppm
            stm32g0/system.cppm
            stm32g0/uart.cppm

            stm32g0/stm32g0.cppm

            PUBLIC
            FILE_SET pub_headers TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/stm32g0"
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/pin_macros.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/interrupts.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g0/internal/peripheral_availability.h"
    )

    target_link_libraries("${VARIANT}_hal"
            PUBLIC
            "${VARIANT}_vendor"
            rtos_check
            rtos_concepts

            PRIVATE
            hstd
            hal_abstract
            project_settings)

endfunction()

function(add_stm32u0_hal VARIANT XMLFILE)
    generate_pin_mapping(
            stm32u0
            ${VARIANT}
            "uart"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/generated/${VARIANT}/stm32u0/mappings/uart_pin_mapping.cppm"
    )

    generate_pin_mapping(
            stm32u0
            ${VARIANT}
            "spi_i2s"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/generated/${VARIANT}/stm32u0/mappings/spi_i2s_pin_mapping.cppm"
    )

    generate_pin_mapping(
            stm32u0
            ${VARIANT}
            i2c
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/generated/${VARIANT}/stm32u0/mappings/i2c_pin_mapping.cppm"
    )

    add_library("${VARIANT}_hal")
    target_sources("${VARIANT}_hal"
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/generated/${VARIANT}/stm32u0/mappings/uart_pin_mapping.cppm"
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/generated/${VARIANT}/stm32u0/mappings/i2c_pin_mapping.cppm"
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/generated/${VARIANT}/stm32u0/mappings/spi_i2s_pin_mapping.cppm"

            stm32u0/stm32u0.cppm

            stm32u0/clocks.cppm
            stm32u0/peripherals.cppm
            stm32u0/pin.cppm

            PUBLIC
            FILE_SET pub_headers TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/stm32u0"
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/interrupts.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32u0/internal/peripheral_availability.h"
    )

    target_link_libraries("${VARIANT}_hal"
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
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4_old/generated/${VARIANT}/stm32g4/mappings/uart_pin_mapping.h")
    add_generated_file(
            stm32g4
            "${VARIANT}"
            "spi-i2s-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4_old/generated/${VARIANT}/stm32g4/mappings/spi_i2s_pin_mapping.h")
    add_generated_file(
            stm32g4
            "${VARIANT}"
            "i2c-pin-mapping"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4_old/generated/${VARIANT}/stm32g4/mappings/i2c_pin_mapping.h")

    # Add library
    add_library("${VARIANT}_hal_old"
            "stm32g4_old/generated/${VARIANT}/stm32g4/mappings/uart_pin_mapping.h"
            "stm32g4_old/generated/${VARIANT}/stm32g4/mappings/spi_i2s_pin_mapping.h"
            "stm32g4_old/generated/${VARIANT}/stm32g4/mappings/i2c_pin_mapping.h"

            stm32g4_old/internal/i2c_timing.h

            stm32g4_old/clocks.cpp
            stm32g4_old/clocks.h
            stm32g4_old/cordic.h
            stm32g4_old/dma.cpp
            stm32g4_old/dma.h
            stm32g4_old/i2c.cpp
            stm32g4_old/i2c.h
            stm32g4_old/interrupts.h
            stm32g4_old/peripheral_ids.h
            stm32g4_old/pin.cpp
            stm32g4_old/pin.h
            stm32g4_old/spi.cpp
            stm32g4_old/spi.h
            stm32g4_old/system.cpp
            stm32g4_old/system.h
            stm32g4_old/uart.cpp
            stm32g4_old/uart.h
    )

    target_link_libraries("${VARIANT}_hal_old" PUBLIC abstract_hal "${VARIANT}_vendor" project_settings)
    target_include_directories("${VARIANT}_hal_old" PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4_old/generated/${VARIANT}")

    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/uart_pin_mapping.cppm")
        add_generated_file(
                stm32g4
                "${VARIANT}"
                "uart-pin-mapping-module"
                "${XMLFILE}"
                "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/uart_pin_mapping.cppm")
    endif ()

    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/tim_pin_mapping.cppm")
        add_generated_file(
                stm32g4
                "${VARIANT}"
                "tim-pin-mapping-module"
                "${XMLFILE}"
                "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/tim_pin_mapping.cppm")
    endif ()

    add_library("${VARIANT}_hal")
    target_sources("${VARIANT}_hal"
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/uart_pin_mapping.cppm"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/generated/${VARIANT}/stm32g4/mappings/tim_pin_mapping.cppm"

            stm32g4/stm32g4.cppm

            stm32g4/tim/channel.cppm
            stm32g4/tim/common.cppm
            stm32g4/tim/features.cppm
            stm32g4/tim/tim.cppm

            stm32g4/clocks.cppm
            stm32g4/dma.cppm
            stm32g4/nvic.cppm
            stm32g4/peripherals.cppm
            stm32g4/pin.cppm
            stm32g4/system.cppm
            stm32g4/uart.cppm

            PUBLIC
            FILE_SET pub_headers TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/stm32g4"
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/pin_macros.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32g4/interrupts.h"
    )

    target_link_libraries("${VARIANT}_hal"
            PUBLIC
            "${VARIANT}_vendor"
            rtos_check
            rtos_concepts

            PRIVATE
            hstd
            hal_abstract
            project_settings)
endfunction()


function(add_stm32h5_hal VARIANT XMLFILE)
    generate_pin_mapping(
            stm32h5
            ${VARIANT}
            "uart"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/uart_pin_mapping.cppm"
    )

    generate_pin_mapping(
            stm32h5
            ${VARIANT}
            "spi_i2s"
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/spi_i2s_pin_mapping.cppm"
    )

    generate_pin_mapping(
            stm32h5
            ${VARIANT}
            i2c
            "${XMLFILE}"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/i2c_pin_mapping.cppm"
    )

    add_library("${VARIANT}_hal")
    target_sources("${VARIANT}_hal"
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            stm32h5/internal/i2c_timing.cppm
            stm32h5/internal/i2s_config.cppm
            stm32h5/internal/i2s_pinout.cppm
            stm32h5/internal/spi_config.cppm
            stm32h5/internal/spi_pinout.cppm

            stm32h5/clocks.cppm
            stm32h5/dma.cppm
            stm32h5/i2c.cppm
            stm32h5/i2s.cppm
            stm32h5/nvic.cppm
            stm32h5/peripherals.cppm
            stm32h5/pin.cppm
            stm32h5/pin_interrupt.cppm
            stm32h5/spi.cppm
            stm32h5/system.cppm
            stm32h5/tim.cppm
            stm32h5/uart.cppm
            stm32h5/usb.cppm

            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/uart_pin_mapping.cppm"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/spi_i2s_pin_mapping.cppm"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/generated/${VARIANT}/stm32h5/mappings/i2c_pin_mapping.cppm"

            stm32h5/stm32h5.cppm

            PUBLIC
            FILE_SET pub_headers TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/stm32h5"
            FILES
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/interrupts.h"
            "${CMAKE_CURRENT_LIST_DIR}/stm32h5/internal/peripheral_availability.h"
    )

    target_link_libraries("${VARIANT}_hal"
            PUBLIC
            "${VARIANT}_vendor"
            rtos_check
            rtos_concepts

            PRIVATE
            hstd
            hal_abstract
            project_settings)

    set_target_properties("${VARIANT}_hal" PROPERTIES FOLDER hal/impl)
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
    if ("${STM32G0_VARIANT}" STREQUAL "STM32G030")
        add_stm32g0_hal(stm32g030 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32G030K(6-8)Tx.xml")
    elseif ("${STM32G0_VARIANT}" STREQUAL "STM32G0B1")
        add_stm32g0_hal(stm32g0b1 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32G0B1R(B-C-E)Tx.xml")
    endif ()

    if ("${STM32U0_VARIANT}" STREQUAL "STM32U083")
        add_stm32u0_hal(stm32u083 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32U083RCTx.xml")
    endif ()
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
    if ("${STM32H5_VARIANT}" STREQUAL "STM32H533RE")
        add_stm32h5_hal(stm32h533 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32H533RETx.xml")
    elseif ("${STM32H5_VARIANT}" STREQUAL "STM32H563VI")
        add_stm32h5_hal(stm32h563 "${CMAKE_CURRENT_LIST_DIR}/../vendor/stm32_pin_data/mcu/STM32H563VITx.xml")
    endif ()
endif ()

if (NOT CMAKE_CROSSCOMPILING)
    add_library(hal_sil)
    target_sources(hal_sil
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            sil/sil.cppm

            sil/gpio.cppm
            sil/scheduler.cppm
            sil/spi.cppm
            sil/system.cppm
            sil/uart.cppm

            PRIVATE
            sil/scheduler_external_event_item.cpp
            sil/scheduler_scheduler.cpp
            sil/scheduler_thread_item.cpp)
    target_link_libraries(hal_sil PUBLIC hal_abstract hstd rtos_concepts)
endif ()
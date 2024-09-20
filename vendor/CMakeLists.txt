add_subdirectory(linker)

add_library(stm32_system STATIC
        stm32_system/syscalls.c
        stm32_system/sysmem.c)

FetchContent_Declare(cmsis_dsp URL https://github.com/ARM-software/CMSIS-DSP/archive/refs/tags/v1.16.2.zip)
set(CMSISCORE "${CMAKE_CURRENT_LIST_DIR}/cmsis_core")
FetchContent_MakeAvailable(cmsis_dsp)


function(add_stm32g0_vendor_hal VARIANT)
    string(TOUPPER "${VARIANT}" VARIANT_UPPER)

    add_linkopts(${VARIANT})

    # stm32g0xx_device library
    add_library("${VARIANT}_device" INTERFACE
            "stm32g0xx_device/Include/stm32g0xx.h"
            "stm32g0xx_device/Include/${VARIANT}xx.h"
            "stm32g0xx_device/Include/system_stm32g0xx.h")

    target_include_directories("${VARIANT}_device" SYSTEM INTERFACE
            stm32g0xx_device/Include)

    target_link_libraries("${VARIANT}_device" INTERFACE
            cmsis_core_includes
            project_settings)

    target_compile_definitions("${VARIANT}_device" INTERFACE
            "STM32G0" "${VARIANT_UPPER}" "${VARIANT_UPPER}xx")

    # stm32g0xx_hal library
    file(GLOB HAL_HEADERS "stm32g0xx_hal/Inc/stm32g0xx_*.h")
    file(GLOB HAL_SRCS "stm32g0xx_hal/Src/stm32g0xx_*.c")

    add_library("${VARIANT}_st_hal" STATIC
            ${HAL_HEADERS} ${HAL_SRCS})

    target_include_directories("${VARIANT}_st_hal" SYSTEM PUBLIC
            stm32g0xx_hal/Inc
            hal_conf)

    target_link_libraries("${VARIANT}_st_hal" PUBLIC
            "${VARIANT}_device")

    # stm32g0xx library
    add_library("${VARIANT}_vendor" STATIC
            "stm32g0xx_device/Source/Templates/gcc/startup_${VARIANT}xx.s"
            "stm32g0xx_device/Source/Templates/system_stm32g0xx.c")
    target_link_libraries("${VARIANT}_vendor" PUBLIC "${VARIANT}_st_hal" "${VARIANT}_linkopts")
endfunction()

function(add_stm32g4_vendor_hal VARIANT)
    string(TOUPPER "${VARIANT}" VARIANT_UPPER)

    add_linkopts(${VARIANT})

    # stm32g0xx_device library
    add_library("${VARIANT}_device" INTERFACE
            "stm32g4xx_device/Include/stm32g4xx.h"
            "stm32g4xx_device/Include/${VARIANT}xx.h"
            "stm32g4xx_device/Include/system_stm32g4xx.h")

    target_include_directories("${VARIANT}_device" SYSTEM INTERFACE
            stm32g4xx_device/Include)

    target_link_libraries("${VARIANT}_device" INTERFACE
            cmsis_core_includes
            project_settings)

    target_compile_definitions("${VARIANT}_device" INTERFACE
            "STM32G4" "${VARIANT_UPPER}" "${VARIANT_UPPER}xx")

    # stm32g0xx_hal library
    file(GLOB HAL_HEADERS "stm32g4xx_hal/Inc/stm32g4xx_*.h")
    file(GLOB HAL_SRCS "stm32g4xx_hal/Src/stm32g4xx_*.c")

    add_library("${VARIANT}_st_hal" STATIC
            ${HAL_HEADERS} ${HAL_SRCS})

    target_include_directories("${VARIANT}_st_hal" SYSTEM PUBLIC
            stm32g4xx_hal/Inc
            hal_conf)

    target_link_libraries("${VARIANT}_st_hal" PUBLIC
            "${VARIANT}_device")

    # stm32g0xx library
    add_library("${VARIANT}_vendor" STATIC
            "stm32g4xx_device/Source/Templates/gcc/startup_${VARIANT}xx.s"
            "stm32g4xx_device/Source/Templates/system_stm32g4xx.c")
    target_link_libraries("${VARIANT}_vendor" PUBLIC "${VARIANT}_st_hal" "${VARIANT}_linkopts")
endfunction()

if ("${TARGET}" STREQUAL "arm-cortex-m0plus")
    # CMSIS core includes
    if (NOT TARGET cmsis_core_includes)
        add_library(cmsis_core_includes INTERFACE)
        target_include_directories(cmsis_core_includes INTERFACE
                "cmsis_core/Include")
    endif ()

    add_stm32g0_vendor_hal(stm32g0b1)
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m4")
    # CMSIS core includes
    if (NOT TARGET cmsis_core_includes)
        add_library(cmsis_core_includes INTERFACE)
        target_include_directories(cmsis_core_includes INTERFACE
                "cmsis_core/Include")
    endif ()

    add_stm32g4_vendor_hal(stm32g474)
endif ()

add_subdirectory(linker)
add_subdirectory(cmsis_dsp)

add_library(stm32_system STATIC
        stm32_system/syscalls.c
        stm32_system/sysmem.c)


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
            project_settings_base)

    target_compile_definitions("${VARIANT}_device" INTERFACE
            "CORE_CM0P" "STM32G0" "${VARIANT_UPPER}" "${VARIANT_UPPER}xx")

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
    target_link_libraries("${VARIANT}_vendor" PUBLIC "${VARIANT}_st_hal")
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

function(add_stm32h5_vendor_hal VARIANT)
    string(TOUPPER "${VARIANT}" VARIANT_UPPER)

    add_linkopts(${VARIANT})

    # stm32h5xx_device library
    add_library("${VARIANT}_device" INTERFACE
            "stm32h5xx_device/Include/stm32h5xx.h"
            "stm32h5xx_device/Include/${VARIANT}xx.h"
            "stm32h5xx_device/Include/system_stm32h5xx.h")

    target_include_directories("${VARIANT}_device" SYSTEM INTERFACE
            stm32h5xx_device/Include)

    target_link_libraries("${VARIANT}_device" INTERFACE
            cmsis_core_includes
            project_settings)

    target_compile_definitions("${VARIANT}_device" INTERFACE
            "STM32H5" "${VARIANT_UPPER}" "${VARIANT_UPPER}xx")

    # stm32h5xx_hal library
    file(GLOB HAL_HEADERS "stm32h5xx_hal/Inc/stm32h5xx_hal*.h" "stm32h5xx_hal/Inc/stm32h5xx_ll_*.h")
    file(GLOB HAL_SRCS "stm32h5xx_hal/Src/stm32h5xx_hal*.c" "stm32h5xx_hal/Src/stm32h5xx_ll_*.c")

    add_library("${VARIANT}_st_hal" STATIC
            ${HAL_HEADERS} ${HAL_SRCS})

    target_include_directories("${VARIANT}_st_hal" SYSTEM PUBLIC
            stm32h5xx_hal/Inc
            hal_conf)

    target_link_libraries("${VARIANT}_st_hal" PUBLIC
            "${VARIANT}_device")

    # stm32h5xx library
    add_library("${VARIANT}_vendor" STATIC
            "stm32h5xx_device/Source/Templates/gcc/startup_${VARIANT}xx.s"
            "stm32h5xx_device/Source/Templates/system_stm32h5xx.c")
    target_link_libraries("${VARIANT}_vendor" PUBLIC "${VARIANT}_st_hal" "${VARIANT}_linkopts")
endfunction()


function(add_stm32h7_dualcore_vendor_hal VARIANT BOOT_TYPE)
    string(TOUPPER "${VARIANT}" VARIANT_UPPER)

    add_linkopts(${VARIANT})

    # stm32g0xx_device library
    add_library("${VARIANT}_device" INTERFACE
            "stm32h7xx_device/Include/stm32h7xx.h"
            "stm32h7xx_device/Include/${VARIANT}xx.h"
            "stm32h7xx_device/Include/system_stm32h7xx.h")

    target_include_directories("${VARIANT}_device" SYSTEM INTERFACE
            stm32h7xx_device/Include)

    target_link_libraries("${VARIANT}_device" INTERFACE
            cmsis_core_includes
            project_settings)

    target_compile_definitions("${VARIANT}_device" INTERFACE
            "STM32H7" "${VARIANT_UPPER}" "${VARIANT_UPPER}xx")

    if ("${TARGET}" STREQUAL "arm-cortex-m4")
        target_compile_definitions("${VARIANT}_device" INTERFACE "CORE_CM4")
    elseif ("${TARGET}" STREQUAL "arm-cortex-m7")
        target_compile_definitions("${VARIANT}_device" INTERFACE "CORE_CM7")
    endif ()

    # stm32g0xx_hal library
    file(GLOB HAL_HEADERS "stm32h7xx_hal/Inc/stm32h7xx_*.h")
    file(GLOB HAL_SRCS "stm32h7xx_hal/Src/stm32h7xx_*.c")

    add_library("${VARIANT}_st_hal" STATIC
            ${HAL_HEADERS} ${HAL_SRCS})

    target_include_directories("${VARIANT}_st_hal" SYSTEM PUBLIC
            stm32h7xx_hal/Inc
            hal_conf)

    target_link_libraries("${VARIANT}_st_hal" PUBLIC
            "${VARIANT}_device")

    # stm32g0xx library
    add_library("${VARIANT}_vendor" STATIC
            "stm32h7xx_device/Source/Templates/gcc/startup_${VARIANT}xx.s"
            "stm32h7xx_device/Source/Templates/system_stm32h7xx_${BOOT_TYPE}.c")
    target_link_libraries("${VARIANT}_vendor" PUBLIC "${VARIANT}_st_hal")

    if ("${TARGET}" STREQUAL "arm-cortex-m4")
        target_link_libraries("${VARIANT}_vendor" PUBLIC "${VARIANT}_cm4_linkopts")
    elseif ("${TARGET}" STREQUAL "arm-cortex-m7")
        target_link_libraries("${VARIANT}_vendor" PUBLIC "${VARIANT}_cm7_linkopts")
    endif ()
endfunction()

function(ADD_FREERTOS CORE PORT)
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE freertos_conf)
    target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0 ${CORE})

    set(FREERTOS_PORT ${PORT})

    add_subdirectory(freertos)
endfunction()

if ("${TARGET}" STREQUAL "arm-cortex-m0plus")
    # CMSIS core includes
    if (NOT TARGET cmsis_core_includes)
        add_library(cmsis_core_includes INTERFACE)
        target_include_directories(cmsis_core_includes INTERFACE
                "cmsis_core/Include")

        target_link_libraries(CMSISDSP PUBLIC cmsis_core_includes)
    endif ()

    add_stm32g0_vendor_hal(stm32g0b1)
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m4")
    # CMSIS core includes
    if (NOT TARGET cmsis_core_includes)
        add_library(cmsis_core_includes INTERFACE)
        target_include_directories(cmsis_core_includes INTERFACE
                "cmsis_core/Include")

        target_link_libraries(CMSISDSP PUBLIC cmsis_core_includes)
    endif ()

    # FreeRTOS
    add_freertos(CORE_CM4 "GCC_ARM_CM4F")
    target_link_libraries(freertos_kernel PUBLIC project_settings)
    target_link_libraries(freertos_kernel_include INTERFACE project_settings)
    target_link_libraries(freertos_kernel_port_headers INTERFACE project_settings)
    target_link_libraries(freertos_kernel_port PUBLIC project_settings)



    # HAL
    add_stm32g4_vendor_hal(stm32g474)
    add_stm32h7_dualcore_vendor_hal(stm32h755 dualcore_boot_cm4_cm7)
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m7")
    # CMSIS core includes
    if (NOT TARGET cmsis_core_includes)
        add_library(cmsis_core_includes INTERFACE)
        target_include_directories(cmsis_core_includes INTERFACE
                "cmsis_core/Include")

        target_link_libraries(CMSISDSP PUBLIC cmsis_core_includes)
    endif ()

    add_stm32h7_dualcore_vendor_hal(stm32h755 dualcore_boot_cm4_cm7)
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m33")
    # CMSIS core includes
    if (NOT TARGET cmsis_core_includes)
        add_library(cmsis_core_includes INTERFACE)
        target_include_directories(cmsis_core_includes INTERFACE
                "cmsis_core/Include")

        target_link_libraries(CMSISDSP PUBLIC cmsis_core_includes)
    endif ()

    add_stm32h5_vendor_hal(stm32h533)
endif ()

if ("${TARGET}" STREQUAL "arm-cortex-m0plus"
        OR "${TARGET}" STREQUAL "arm-cortex-m4"
        OR "${TARGET}" STREQUAL "arm-cortex-m7"
        OR "${TARGET}" STREQUAL "arm-cortex-m33")
    set(STM32_MW_USBD_CORE_SRCS
            stm32_mw_usb_device/Core/Inc/usbd_core.h
            stm32_mw_usb_device/Core/Inc/usbd_ctlreq.h
            stm32_mw_usb_device/Core/Inc/usbd_def.h
            stm32_mw_usb_device/Core/Inc/usbd_ioreq.h

            stm32_mw_usb_device/Core/Src/usbd_core.c
            stm32_mw_usb_device/Core/Src/usbd_ctlreq.h
            stm32_mw_usb_device/Core/Src/usbd_ioreq.c)


    # - USBD CDC class
    set(STM32_MW_USBD_CLASS_CDC_SRCS
            stm32_mw_usb_device/Class/CDC/Inc/usbd_cdc.h
            stm32_mw_usb_device/Class/CDC/Src/usbd_cdc.c)

    function(add_usbd_library NAME)
        set(MULTI_VALUE_ARGS CONF_FILES CLASSES LIBS)

        cmake_parse_arguments(ADD_USBD "" "" "${MULTI_VALUE_ARGS}" ${ARGN})

        set(TARGET "usbd_${NAME}")

        add_library(${TARGET} STATIC
                ${STM32_MW_USBD_CORE_SRCS}
                ${ADD_USBD_CONF_FILES})

        foreach (CONF_FILE ${ADD_USBD_CONF_FILES})
            get_filename_component(CONF_FILE_DIR "${CONF_FILE}" DIRECTORY)
            if (NOT CONF_FILE_DIRS)
                list(APPEND CONF_FILE_DIRS "${CONF_FILE_DIR}")
            else ()
                if (NOT ("${CONF_FILE_DIR}" IN_LIST CONF_FILE_DIRS))
                    list(APPEND CONF_FILE_DIRS "${CONF_FILE_DIR}")
                endif ()
            endif ()
        endforeach ()

        target_include_directories(${TARGET} PUBLIC
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/stm32_mw_usb_device/Core/Inc"
                ${CONF_FILE_DIRS})
        target_link_libraries(${TARGET} PUBLIC ${ADD_USBD_LIBS})

        if ("CDC" IN_LIST ADD_USBD_CLASSES)
            target_sources(${TARGET} PRIVATE
                    ${STM32_MW_USBD_CLASS_CDC_SRCS})
            target_include_directories(${TARGET} PUBLIC
                    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/stm32_mw_usb_device/Class/CDC/Inc")
        endif ()
    endfunction()
endif ()
add_library(constexpr_tools INTERFACE
        constexpr_tools/buffer_io.h
        constexpr_tools/chrono_ex.h
        constexpr_tools/crc.h
        constexpr_tools/helpers.h
        constexpr_tools/integer.h
        constexpr_tools/logic.h
        constexpr_tools/math.h
        constexpr_tools/memory_size.h
        constexpr_tools/spans.h
        constexpr_tools/static_mapping.h
        constexpr_tools/static_string.h
        constexpr_tools/std_ex.h
        constexpr_tools/strings.h
        constexpr_tools/type_helpers.h)
target_include_directories(constexpr_tools INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})


# Proxy library for CMSIS-DSP which allows disabling it under certain build conditions
add_library(cmsis_dsp INTERFACE)
target_link_libraries(cmsis_dsp INTERFACE CMSISDSP)
target_compile_definitions(cmsis_dsp INTERFACE HAS_CMSIS_DSP)


add_library(fp INTERFACE
        fp/concepts.h
        fp/fix.h)
target_include_directories(fp INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})

add_library(math INTERFACE
        math/functions/transcendental/trigonometric.h

        math/functions/common.h
        math/functions/interpolation.h
        math/functions/transcendental/power.h
        math/functions/transcendental.h
        math/functions/utilities.h

        math/geometry/transformation_matrix.h

        math/linalg/vector.h
        math/linalg/matrix.h

        math/concepts.h
        math/functions.h
        math/math_config.h)

target_link_libraries(math INTERFACE cmsis_dsp fp)
target_compile_definitions(math INTERFACE HAL2_CMSIS_DSP_ENABLED)

target_include_directories(math INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(sbs)
add_subdirectory(vrpc)

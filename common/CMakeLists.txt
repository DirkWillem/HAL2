add_library(constexpr_tools INTERFACE
        constexpr_tools/buffer_io.h
        constexpr_tools/crc.h
        constexpr_tools/helpers.h
        constexpr_tools/integer.h
        constexpr_tools/math.h
        constexpr_tools/memory_size.h
        constexpr_tools/static_mapping.h
        constexpr_tools/static_string.h
        constexpr_tools/strings.h)
target_include_directories(constexpr_tools INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})

add_library(halstd INTERFACE
        halstd/mp/helpers.h
        halstd/mp/type_helpers.h
        halstd/mp/types.h
        halstd/mp/values.h

        halstd/atomic.h
        halstd/atomic_helpers.h
        halstd/callback.h
        halstd/chrono_ex.h
        halstd/compile_time_assert.h
        halstd/logic.h
        halstd/ratio_ex.h
        halstd/spans.h)
target_include_directories(halstd INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})



add_library(hstd)
target_sources(hstd
    PUBLIC
    FILE_SET CXX_MODULES FILES
        halstd_module/mp/helpers.cppm
        halstd_module/mp/types.cppm
        halstd_module/mp/values.cppm

        halstd_module/atomic.cppm
        halstd_module/callback.cppm
        halstd_module/chrono.cppm
        halstd_module/logic.cppm
        halstd_module/ratio.cppm
        halstd_module/spans.cppm

        halstd_module/halstd.cppm)
target_link_libraries(hstd PRIVATE project_settings)

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

# Statechart library
add_library(statechart INTERFACE
        sc/statechart.h
        sc/statechart_callback.h)
target_include_directories(statechart INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(statechart INTERFACE halstd)

target_link_libraries(math INTERFACE cmsis_dsp fp)
target_compile_definitions(math INTERFACE HAL2_CMSIS_DSP_ENABLED)

target_include_directories(math INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(sbs)
add_subdirectory(vrpc)

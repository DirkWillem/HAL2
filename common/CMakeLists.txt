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


add_library(hstd)
target_sources(hstd
        PUBLIC
        FILE_SET CXX_MODULES FILES
        hstd/mp/helpers.cppm
        hstd/mp/stl_types.cppm
        hstd/mp/types.cppm
        hstd/mp/values.cppm

        hstd/array.cppm
        hstd/atomic.cppm
        hstd/callback.cppm
        hstd/crc.cppm
        hstd/endian.cppm
        hstd/chrono.cppm
        hstd/constants.cppm
        hstd/logic.cppm
        hstd/math.cppm
        hstd/memory.cppm
        hstd/ratio.cppm
        hstd/spans.cppm
        hstd/static_string_builder.cppm

        hstd/halstd.cppm)
target_link_libraries(hstd PUBLIC project_settings)

# Proxy library for CMSIS-DSP which allows disabling it under certain build conditions
add_library(cmsis_dsp INTERFACE)
target_link_libraries(cmsis_dsp INTERFACE CMSISDSP)
target_compile_definitions(cmsis_dsp INTERFACE HAS_CMSIS_DSP)

# Math library
add_library(math)
target_sources(math
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        math/math.cppm

        math/functions/interpolation.cppm
        math/functions/power.cppm
        math/functions/trigonometric.cppm
        math/geometry/coordinate.cppm
        math/linalg/vector.cppm
        math/concepts.cppm
        math/lut.cppm
        math/settings.cppm)
target_link_libraries(math PUBLIC hstd cmsis_dsp)

# Statechart library
add_library(statechart)
target_sources(statechart
        PUBLIC
        FILE_SET CXX_MODULES FILES
        sc/core.cppm
        sc/statechart_callback.cppm

        sc/statechart.cppm)
target_link_libraries(statechart PRIVATE hstd project_settings)

#add_subdirectory(sbs)
#add_subdirectory(vrpc)

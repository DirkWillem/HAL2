
# hstd library.
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
        hstd/buffers.cppm

        hstd/halstd.cppm)
target_link_libraries(hstd PUBLIC project_settings)

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

# hstd DSP library.
add_library(hstd_dsp)
target_sources(hstd_dsp
        PUBLIC
        FILE_SET CXX_MODULES FILES
        hstd_dsp/hstd_dsp.cppm)
target_link_libraries(hstd_dsp PUBLIC hstd cmsis_dsp math)

# Statechart library
add_library(statechart)
target_sources(statechart
        PUBLIC
        FILE_SET CXX_MODULES FILES
        sc/core.cppm
        sc/statechart_callback.cppm

        sc/statechart.cppm)
target_link_libraries(statechart PRIVATE hstd project_settings)

# RTOS concepts libraries.
add_subdirectory(rtos)

# HAL concepts libraries.
add_subdirectory(hal)
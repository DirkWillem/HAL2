add_library(constexpr_tools INTERFACE
        constexpr_tools/buffer_io.h
        constexpr_tools/chrono_ex.h
        constexpr_tools/crc.h
        constexpr_tools/helpers.h
        constexpr_tools/logic.h
        constexpr_tools/math.h
        constexpr_tools/spans.h
        constexpr_tools/static_mapping.h
        constexpr_tools/static_string.h
        constexpr_tools/std_ex.h
        constexpr_tools/strings.h
        constexpr_tools/type_helpers.h)
target_include_directories(constexpr_tools INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})


add_library(fp INTERFACE
        fp/fix.h)
target_include_directories(fp INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})


add_subdirectory(sbs)
add_subdirectory(vrpc)

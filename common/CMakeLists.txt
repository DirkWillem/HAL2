add_library(constexpr_tools INTERFACE
        constexpr_tools/logic.h)
target_include_directories(constexpr_tools INTERFACE
    ${CMAKE_CURRENT_LIST_DIR})

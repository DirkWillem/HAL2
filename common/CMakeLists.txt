add_library(constexpr_tools INTERFACE
        constexpr_tools/logic.h)
target_include_directories(constexpr_tools INTERFACE
    ${CMAKE_CURRENT_LIST_DIR})


add_library(fp INTERFACE
        fp/fix.h)
target_include_directories(fp INTERFACE
    ${CMAKE_CURRENT_LIST_DIR})

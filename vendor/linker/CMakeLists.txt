function(add_linkopts VARIANT)
    add_library("${VARIANT}_linkopts" INTERFACE)

    target_link_directories("${VARIANT}_linkopts" INTERFACE
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    target_link_options("${VARIANT}_linkopts" INTERFACE
            -T${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${VARIANT}.ld)
endfunction()
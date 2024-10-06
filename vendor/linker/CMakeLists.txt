function(add_linkopts VARIANT)
    add_library("${VARIANT}_linkopts" INTERFACE)

    target_link_directories("${VARIANT}_linkopts" INTERFACE
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    target_link_options("${VARIANT}_linkopts" INTERFACE
            -T${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${VARIANT}.ld -flto -Wl,-Map=output.map)
endfunction()

function(add_multicore_linkopts VARIANT CORE)
    add_library("${VARIANT}_${CORE}_linkopts" INTERFACE)

    target_link_directories("${VARIANT}_${CORE}_linkopts" INTERFACE
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    target_link_options("${VARIANT}_${CORE}_linkopts" INTERFACE
            -T${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${VARIANT}_${CORE}.ld -flto -Wl,-Map=output.map)
endfunction()
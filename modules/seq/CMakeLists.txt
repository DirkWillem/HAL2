add_library(seq_abstract)
target_sources(seq_abstract
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        core/seq_abstract.cppm)
target_link_libraries(seq_abstract
        PUBLIC
        hstd)


add_library(seq)
target_sources(seq
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        core/seq.cppm)
target_link_libraries(seq
        PUBLIC
        hstd

        seq_abstract)


add_library(seq_hal)
target_sources(seq_hal
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        hal/hal.cppm

        hal/common.cppm
        hal/uart.cppm)
target_link_libraries(seq_hal
        PUBLIC
        hstd
        hal_abstract
        seq_abstract)

if (CMAKE_CROSSCOMPILING)
    if ("${TARGET}" STREQUAL "arm-cortex-m0plus")
        add_library(seq_port_system_arm_cortex_m)
        target_sources(seq_port_system_arm_cortex_m
                PUBLIC
                FILE_SET CXX_MODULES
                FILES
                port/system/system_arm_cortex_m.cppm)
        target_compile_definitions(seq_port_system_arm_cortex_m PRIVATE CORE_CM0P)
        target_link_libraries(seq_port_system_arm_cortex_m PUBLIC hstd)
    endif()
endif()

function(ADD_SEQ_PORT_CRITICAL_SECTION_QUEUE)
    add_library(seq_port_critical_section_queue)
    target_sources(seq_port_critical_section_queue
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/port/queue/critical_section_queue.cppm)

    target_link_libraries(seq_port_critical_section_queue
            PRIVATE
            hstd
            cmsis_core_cpp)
endfunction()
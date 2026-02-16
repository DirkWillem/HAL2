add_library(rtos_check)
target_sources(rtos_check
        PUBLIC
        FILE_SET CXX_MODULES FILES
        check_rtos/check_rtos.cppm)
target_link_libraries(rtos_check PRIVATE project_settings)

add_library(rtos_concepts)
target_sources(rtos_concepts
        PUBLIC
        FILE_SET CXX_MODULES FILES
        abstract/rtos_concepts.cppm)
target_link_libraries(rtos_concepts PRIVATE project_settings)

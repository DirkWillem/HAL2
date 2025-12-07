add_library(rtos_check)
target_sources(rtos_check
        PUBLIC
        FILE_SET CXX_MODULES FILES
        check_rtos/check_rtos.cppm)
target_link_libraries(rtos_check PRIVATE project_settings)


if (CMAKE_CROSSCOMPILING)
    add_library(rtos_freertos)
    target_sources(rtos_freertos
            PUBLIC
            FILE_SET CXX_MODULES FILES
            freertos/freertos.cppm

            freertos/event_group.cppm
            freertos/mutex.cppm
            freertos/queue.cppm
            freertos/system.cppm
            freertos/task.cppm
            freertos/time.cppm)
    target_link_libraries(rtos_freertos
            PUBLIC
            freertos_kernel

            PRIVATE
            project_settings
            hstd
            hal_abstract)
endif ()


add_library(rtos_concepts)
target_sources(rtos_concepts
        PUBLIC
        FILE_SET CXX_MODULES FILES
        concepts/rtos_concepts.cppm)
target_link_libraries(rtos_concepts PRIVATE project_settings)


if (NOT CMAKE_CROSSCOMPILING)
    add_library(rtos_sil)
    target_sources(rtos_sil
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            rtos_sil/rtos_sil.cppm)
    target_link_libraries(rtos_sil PRIVATE hstd rtos_concepts hal_sil)
endif ()
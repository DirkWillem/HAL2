add_library(rtos_freertos)
target_sources(rtos_freertos
        PUBLIC
        FILE_SET CXX_MODULES FILES
        freertos.cppm

        event_group.cppm
        mutex.cppm
        queue.cppm
        system.cppm
        task.cppm
        time.cppm)
target_link_libraries(rtos_freertos
        PUBLIC
        freertos_kernel

        PRIVATE
        project_settings
        hstd
        hal_abstract)

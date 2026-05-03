# Target only
if (CMAKE_CROSSCOMPILING)
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

    set_target_properties(rtos_freertos PROPERTIES FOLDER hal/modules/freertos)
endif ()

# Desktop-only
if (NOT CMAKE_CROSSCOMPILING)
    # Test helpers
    add_library(freertos_test_helpers)
    target_sources(freertos_test_helpers
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            test_helpers/freertos_test_helpers.cppm)
    target_link_libraries(freertos_test_helpers
            PRIVATE
            # HAL libraries
            hstd
            # Google test
            GTest::gtest GTest::gmock)

    set_target_properties(freertos_test_helpers PROPERTIES FOLDER hal/modules/freertos)
endif ()
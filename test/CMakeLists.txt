if (NOT TARGET GTest::gtest)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()

# Test Proto files
generate_vrpc(
        test_protos common/vrpc/protos common/vrpc/generated
        GEN_TYPES uart)

# Test source files
set(HAL2_TEST_SRCS
        doubles/hal/fake_uart.h
        doubles/hal/mock_system.h
        doubles/hal/mock_system.cpp
        doubles/hal/mock_uart.h

        helpers/buffer_builder.cpp
        helpers/buffer_builder.h
        helpers/matchers.h
        helpers/proto_helpers.h

        common/fp/test_uq.cpp

        common/vrpc/uart_calculator_service.h
        common/vrpc/test_vrpc_uart.cpp
        common/vrpc/test_vrpc_uart_decoder.cpp
        common/vrpc/test_vrpc_uart_integration.cpp)


add_library(hal2_tests_lib STATIC ${HAL2_TEST_SRCS})
link_library_hdronly(hal2_tests_lib PUBLIC GTest::gtest)
link_library_hdronly(hal2_tests_lib PUBLIC GTest::gmock)
target_link_libraries(hal2_tests_lib PUBLIC abstract_hal fp)
target_include_directories(hal2_tests_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

add_executable(hal2_tests ${HAL2_TEST_SRCS})
target_link_libraries(hal2_tests PUBLIC
        abstract_hal
        fp
        vrpc_uart
        test_protos

        GTest::gmock
        GTest::gtest_main)
target_include_directories(hal2_tests PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

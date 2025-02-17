generate_vrpc(rad_svcs proto generated
        GEN_TYPES uart)

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

add_library(hal_test_helpers STATIC
        helpers/event_loop.h

        doubles/hal/fake_clock.h
        doubles/hal/fake_pin.h
        doubles/hal/mock_clock.cpp
        doubles/hal/mock_clock.h)
target_link_libraries(hal_test_helpers PUBLIC
        abstract_hal
        GTest::gmock)

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

        common/ct/test_crc.cpp
        common/ct/test_math.cpp
        common/ct/test_static_string.cpp

        common/fp/test_q.cpp
        common/fp/test_uq.cpp

        common/math/test_functions_power.cpp
        common/math/test_functions_trigonometric.cpp
        common/math/test_interpolation_bilinear.cpp

        common/vrpc/uart_calculator_service.h
        common/vrpc/test_vrpc_uart_server.cpp
        common/vrpc/test_vrpc_uart_decoder.cpp
        common/vrpc/test_vrpc_uart_integration.cpp)


add_library(hal2_tests_lib STATIC ${HAL2_TEST_SRCS})
link_library_hdronly(hal2_tests_lib PUBLIC GTest::gtest)
link_library_hdronly(hal2_tests_lib PUBLIC GTest::gmock)
target_link_libraries(hal2_tests_lib PUBLIC abstract_hal fp test_protos hal_test_helpers)
target_include_directories(hal2_tests_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

add_executable(hal2_tests ${HAL2_TEST_SRCS})
target_link_libraries(hal2_tests PUBLIC
        abstract_hal
        fp
        vrpc_uart
        test_protos
        math

        GTest::gmock
        GTest::gtest_main)
target_include_directories(hal2_tests PUBLIC "${CMAKE_CURRENT_LIST_DIR}")


add_executable(playground main.cpp)
target_link_libraries(playground PUBLIC math rad_svcs)
target_compile_options(playground PUBLIC -g3)

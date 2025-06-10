
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

add_library(test_helpers)
target_sources(test_helpers
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        helpers/helpers.cppm
        helpers/matchers.cppm

        helpers/buffer.cppm)

target_link_libraries(test_helpers PUBLIC
        hstd
        project_settings

        GTest::gtest
        GTest::gmock)


# Test source files
set(HAL2_TEST_SRCS
        modbus/test_encoding_rtu_decoder.cpp
        modbus/test_encoding_rtu_encoder.cpp)


add_library(hal2_tests_lib STATIC ${HAL2_TEST_SRCS})
link_library_hdronly(hal2_tests_lib PUBLIC GTest::gtest)
link_library_hdronly(hal2_tests_lib PUBLIC GTest::gmock)
target_link_libraries(hal2_tests_lib PUBLIC abstract_hal fp test_protos hal_test_helpers test_helpers)
target_include_directories(hal2_tests_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

add_executable(hal2_tests ${HAL2_TEST_SRCS})
target_link_libraries(hal2_tests PUBLIC
        modbus_encoding_rtu

        hstd

        GTest::gmock
        GTest::gtest_main

        test_helpers)
target_include_directories(hal2_tests PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_options(hal2_tests PUBLIC -g3)

if(APPLE)
    target_link_options(hal2_tests PRIVATE
            -L/opt/homebrew/opt/llvm/lib/c++ # TODO: non-hardcoded path
    )
endif()
# Ensure Google Test is downloaded
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

# Test helpers
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

# hstd tests
add_executable(test_hstd
        hstd/test_crc.cpp
        hstd/test_logic.cpp
        hstd/test_memory.cpp)
target_link_libraries(test_hstd
        PRIVATE
        hstd
        GTest::gtest GTest::gmock GTest::gtest_main)
add_test(test_hstd test_hstd)

# MODBUS tests
add_executable(test_modbus
        modbus/test_bit_storage.cpp
        modbus/test_encoding_rtu_decoder.cpp
        modbus/test_encoding_rtu_encoder.cpp
        modbus/test_server.cpp
        modbus/test_server_frames.cpp)
target_link_libraries(test_modbus
        PRIVATE
        modbus_encoding_rtu modbus_server
        test_helpers
        GTest::gtest GTest::gmock GTest::gtest_main)
add_test(test_modbus test_modbus)

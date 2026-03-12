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
add_library(hal2_test_helpers)
target_sources(hal2_test_helpers
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        helpers/helpers.cppm
        helpers/matchers.cppm

        helpers/mocks/mock_performance_timer.cppm
        helpers/mocks/mock_clock.cppm

        helpers/buffer.cppm)

target_link_libraries(hal2_test_helpers PUBLIC
        hstd
        project_settings
        hal_abstract

        GTest::gtest
        GTest::gmock)

set_target_properties(hal2_test_helpers PROPERTIES FOLDER hal/test)

# hstd tests
add_executable(hal2_test_hstd
        core/hstd/test_crc.cpp
        core/hstd/test_logic.cpp
        core/hstd/test_memory.cpp
        core/hstd/test_buffer.cpp)
target_link_libraries(hal2_test_hstd
        PRIVATE
        hstd
        GTest::gtest GTest::gmock GTest::gtest_main)
add_test(hal2_test_hstd hal2_test_hstd)

set_target_properties(hal2_test_hstd PROPERTIES FOLDER hal/test)

# Math tests
add_executable(hal2_test_math
        core/math/test_coordinate.cpp)
target_link_libraries(hal2_test_math
        PRIVATE
        math
        GTest::gtest GTest::gmock GTest::gtest_main)
add_test(hal2_test_math hal2_test_math)

set_target_properties(hal2_test_math PROPERTIES FOLDER hal/test)

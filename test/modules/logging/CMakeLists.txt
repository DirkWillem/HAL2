add_executable(hal2_test_logging
        test_encoding_binary.cpp)
target_link_libraries(hal2_test_logging
        PRIVATE
        # Module under test
        logging
        # Helpers
        hal2_test_helpers
        # Google Test
        GTest::gtest GTest::gmock GTest::gtest_main)
add_test(hal2_test_logging hal2_test_logging)
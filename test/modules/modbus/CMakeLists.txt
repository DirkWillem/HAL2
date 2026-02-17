# MODBUS tests
add_executable(hal2_test_modbus
        modbus/test_bit_storage.cpp
        test_encoding_rtu_decoder.cpp
        test_encoding_rtu_encoder.cpp
        test_server.cpp
        test_server_frames.cpp)
target_link_libraries(hal2_test_modbus
        PRIVATE
        modbus_encoding_rtu modbus_server
        hal2_test_helpers
        GTest::gtest GTest::gmock GTest::gtest_main)
add_test(hal2_test_modbus hal2_test_modbus)

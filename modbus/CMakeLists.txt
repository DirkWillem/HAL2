add_library(modbus_encoding)
target_sources(modbus_encoding PUBLIC
    FILE_SET CXX_MODULES
    FILES
    encoding/common/encoding.cppm)
target_link_libraries(modbus_encoding PUBLIC hstd project_settings)


add_library(modbus_encoding_rtu)
target_sources(modbus_encoding_rtu PUBLIC
        FILE_SET CXX_MODULES
        FILES
        encoding/modbus_rtu/modbus_rtu.cppm

        encoding/modbus_rtu/decoder.cppm
        encoding/modbus_rtu/encoder.cppm)
target_link_libraries(modbus_encoding_rtu PUBLIC modbus_encoding)
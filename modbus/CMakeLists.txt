add_library(modbus_core)
target_sources(modbus_core PUBLIC
        FILE_SET CXX_MODULES
        FILES
        core/core.cppm

        core/pdu.cppm)
target_link_libraries(modbus_core PUBLIC hstd project_settings)


add_library(modbus_encoding_rtu)
target_sources(modbus_encoding_rtu PUBLIC
        FILE_SET CXX_MODULES
        FILES
        encoding/rtu/rtu.cppm

        encoding/rtu/decoder.cppm
        encoding/rtu/encoder.cppm
        encoding/rtu/frames.cppm)
target_link_libraries(modbus_encoding_rtu PUBLIC modbus_core)


add_library(modbus_server)
target_sources(modbus_server PUBLIC
        FILE_SET CXX_MODULES
        FILES
        server/server.cppm

        server/coil.cppm)
target_link_libraries(modbus_server PUBLIC modbus_core)
# MODBUS General
# - Core
add_library(modbus_core)
target_sources(modbus_core PUBLIC
        FILE_SET CXX_MODULES
        FILES
        core/core.cppm

        core/pdu.cppm)
target_link_libraries(modbus_core PUBLIC hstd project_settings)

# - Encoding Core
add_library(modbus_encoding)
target_sources(modbus_encoding PUBLIC
        FILE_SET CXX_MODULES
        FILES encoding/core/encoding.cppm)
target_link_libraries(modbus_encoding PUBLIC modbus_core)

# - RTU Encoding
add_library(modbus_encoding_rtu)
target_sources(modbus_encoding_rtu PUBLIC
        FILE_SET CXX_MODULES
        FILES
        encoding/rtu/rtu.cppm

        encoding/rtu/decoder.cppm
        encoding/rtu/encoder.cppm
        encoding/rtu/frames.cppm)
target_link_libraries(modbus_encoding_rtu PUBLIC modbus_encoding)

# MODBUS Server
# - Specification
add_library(modbus_server_spec)
target_sources(modbus_server_spec
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        server/spec/server_spec.cppm

        server/spec/array.cppm
        server/spec/enumerate.cppm)
target_link_libraries(modbus_server_spec PUBLIC hstd project_settings)

if (NOT CMAKE_CROSSCOMPILING)
    add_library(modbus_server_spec_gen)
    target_sources(modbus_server_spec_gen
            PUBLIC
            FILE_SET CXX_MODULES
            FILES
            server/spec_gen/spec_gen.cppm

            server/spec_gen/registers.cppm)
    target_link_libraries(modbus_server_spec_gen PUBLIC modbus_server_spec nlohmann_json::nlohmann_json)

    add_executable(modbus_server_spec_gen_main
            server/spec_gen/spec_gen_main.cpp)
    target_link_libraries(modbus_server_spec_gen_main modbus_server_spec_gen)
endif ()

# - Core
add_library(modbus_server)
target_sources(modbus_server PUBLIC
        FILE_SET CXX_MODULES
        FILES
        server/core/server.cppm

        server/core/bit.cppm
        server/core/register.cppm
        server/core/server_storage.cppm)
target_link_libraries(modbus_server PUBLIC modbus_server_spec modbus_core)

# - FreeRTOS Implementation
add_library(modbus_server_freertos)
target_sources(modbus_server_freertos PUBLIC
        FILE_SET CXX_MODULES
        FILES
        server/freertos/freertos_server.cppm

        server/freertos/helpers.cppm)
target_link_libraries(modbus_server_freertos
        PUBLIC
        modbus_server
        modbus_encoding
        modbus_encoding_rtu
        rtos_freertos
        hal_abstract)
function(GENERATE_VRPC_MODULE TARGET MODULE_NAME SRCDIR DSTDIR)
    set(OPTION_ARGS NO_DEPEND_COMMON NO_DEPEND_UART_SERVER)
    set(SINGLE_VALUE_ARGS)
    set(MULTI_VALUE_ARGS GEN_TYPES)

    cmake_parse_arguments(GENERATE_VRPC "${OPTION_ARGS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    file(GLOB_RECURSE PROTOS "${SRCDIR}/*.proto")

    get_filename_component(SRCDIR_ABS "${SRCDIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    get_filename_component(DSTDIR_ABS "${DSTDIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

    set(GEN_CORE_CPPMS OFF)
    set(GEN_SERVER_CPPMS OFF)
    set(GEN_CLIENT_CPPMS OFF)
    set(GEN_UART_SERVER_CPPMS OFF)
    set(GEN_UART_CLIENT_CPPMS OFF)

    foreach (PROTO_ABS_PATH ${PROTOS})
        # Obtain file name components
        string(REPLACE "${SRCDIR_ABS}/" "" PROTO_REL_PATH "${PROTO_ABS_PATH}")
        get_filename_component(PROTO_REL_DIR "${PROTO_REL_PATH}" DIRECTORY)
        get_filename_component(PROTO_BASE_NAME "${PROTO_REL_PATH}" NAME_WLE)

        if ("${PROTO_REL_DIR}" STREQUAL "")
            set(PROTO_BASE_PATH "${PROTO_BASE_NAME}")
        else ()
            set(PROTO_BASE_PATH "${PROTO_REL_DIR}/${PROTO_BASE_NAME}")
        endif ()

        # Skip NanoPB source files
        if (NOT "${PROTO_BASE_NAME}" STREQUAL "nanopb")
            set(CORE_REL_PATH "${PROTO_BASE_PATH}.cppm")
            set(SERVER_REL_PATH "${PROTO_BASE_PATH}_server.cppm")
            set(CLIENT_REL_PATH "${PROTO_BASE_PATH}_client.cppm")
            set(PROTO_HDR_REL_PATH "${PROTO_BASE_PATH}.pb.h")
            set(PROTO_SRC_REL_PATH "${PROTO_BASE_PATH}.pb.c")

            set(CORE_ABS_PATH "${DSTDIR_ABS}/${CORE_REL_PATH}")
            set(SERVER_ABS_PATH "${DSTDIR_ABS}/${SERVER_REL_PATH}")
            set(CLIENT_ABS_PATH "${DSTDIR_ABS}/${CLIENT_REL_PATH}")
            list(APPEND CORE_CPPMS "${CORE_ABS_PATH}")
            list(APPEND SERVER_CPPMS "${SERVER_ABS_PATH}")
            list(APPEND CLIENT_CPPMS "${CLIENT_ABS_PATH}")

            if (NOT EXISTS "${CORE_ABS_PATH}")
                set(GEN_CORE_CPPMS ON)
            endif ()
            if (NOT EXISTS "${SERVER_ABS_PATH}")
                set(GEN_SERVER_CPPMS ON)
            endif()
            if (NOT EXISTS "${CLIENT_ABS_PATH}")
                set(GEN_CLIENT_CPPMS ON)
            endif()

            # Add source files
            set(PROTO_HDR_ABS_PATH "${DSTDIR_ABS}/${PROTO_HDR_REL_PATH}")
            set(PROTO_SRC_ABS_PATH "${DSTDIR_ABS}/${PROTO_SRC_REL_PATH}")
            list(APPEND GEN_CXX_SRCS "${PROTO_HDR_ABS_PATH}" "${PROTO_SRC_ABS_PATH}")

            set(GEN_CMD "${HAL_PYTHON}"
                    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../vrpc/vendor/nanopb/generator/nanopb_generator.py"
                    "${PROTO_REL_PATH}"
                    -D "${DSTDIR_ABS}"
                    --quiet --cpp-descriptors)
        endif ()

        if (NOT ${GENERATE_VRPC_NO_DEPEND_COMMON})
            list(APPEND GEN_CMD "--protoc-opt='--proto_path=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../vrpc/protos/common'")
        endif ()

        # Add UART source files if required
        if ("uart_server" IN_LIST GENERATE_VRPC_GEN_TYPES)
            list(APPEND UART_SERVER_CPPMS "${DSTDIR_ABS}/${PROTO_BASE_PATH}_uart_server.cppm")

            if (NOT EXISTS "${DSTDIR_ABS}/${PROTO_BASE_PATH}_uart_server.cppm")
                set(GEN_UART_SERVER_CPPMS ON)
            endif ()
        endif ()

        if ("uart_client" IN_LIST GENERATE_VRPC_GEN_TYPES)
            list(APPEND UART_CLIENT_CPPMS "${DSTDIR_ABS}/${PROTO_BASE_PATH}_uart_client.cppm")

            if (NOT EXISTS "${DSTDIR_ABS}/${PROTO_BASE_PATH}_uart_client.cppm")
                set(GEN_UART_CLIENT_CPPMS ON)
            endif ()
        endif ()

        if (NOT EXISTS "${PROTO_HDR_ABS_PATH}")
            add_custom_command(
                    DEPENDS "${PROTO_ABS_PATH}"
                    OUTPUT "${PROTO_HDR_ABS_PATH}" "${PROTO_SRC_ABS_PATH}"
                    WORKING_DIRECTORY "${SRCDIR_ABS}"
                    COMMAND ${GEN_CMD})
        endif ()
    endforeach ()


    if (GEN_CORE_CPPMS)
        add_custom_command(
                OUTPUT ${CORE_CPPMS}
                DEPENDS ${PROTOS}
                COMMAND
                vrpc_gen
                --gen core-module
                --module-name "${MODULE_NAME}"
                --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../vrpc/protos/common"
                "${SRCDIR_ABS}"
                "${DSTDIR_ABS}")
    endif()

    if (GEN_SERVER_CPPMS)
        add_custom_command(
                OUTPUT ${SERVER_CPPMS}
                DEPENDS ${PROTOS}
                COMMAND
                vrpc_gen
                --gen server-module
                --module-name "${MODULE_NAME}"
                --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../vrpc/protos/common"
                "${SRCDIR_ABS}"
                "${DSTDIR_ABS}")

    endif()

    if (GEN_CLIENT_CPPMS)
        add_custom_command(
                OUTPUT ${CLIENT_CPPMS}
                DEPENDS ${PROTOS}
                COMMAND
                vrpc_gen
                --gen client-module
                --module-name "${MODULE_NAME}"
                --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../vrpc/protos/common"
                "${SRCDIR_ABS}"
                "${DSTDIR_ABS}")
    endif ()


    if (GEN_UART_SERVER_CPPMS)
        add_custom_command(
                OUTPUT ${UART_SERVER_CPPMS}
                DEPENDS ${PROTOS}
                COMMAND
                vrpc_gen
                --gen uart-server-module
                --module-name "${MODULE_NAME}"
                --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../vrpc/protos/common"
                "${SRCDIR_ABS}"
                "${DSTDIR_ABS}")
    endif ()

    if (GEN_UART_CLIENT_CPPMS)
        add_custom_command(
                OUTPUT ${UART_CLIENT_CPPMS}
                DEPENDS ${PROTOS}
                COMMAND
                vrpc_gen
                --gen uart-client-module
                --module-name "${MODULE_NAME}"
                --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../vrpc/protos/common"
                "${SRCDIR_ABS}"
                "${DSTDIR_ABS}")
    endif ()

    # Core library
    add_library(${TARGET} STATIC)
    target_sources(${TARGET}
            PUBLIC
            ${GEN_CXX_SRCS}
            PUBLIC FILE_SET CXX_MODULES
            FILES
            ${CORE_CPPMS})

    target_link_libraries(${TARGET}
            PUBLIC nanopb vrpc_common project_settings)
    set_source_files_properties(${GEN_SRCS} PROPERTIES SKIP_LINTING ON)

    if (NOT ${GENERATE_VRPC_NO_DEPEND_COMMON})
        target_link_libraries(${TARGET} PUBLIC vrpc_protocol_common)
        target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generated/common")
    endif ()


    # Server library
    add_library(${TARGET}_server)
    target_sources(${TARGET}_server PUBLIC FILE_SET CXX_MODULES FILES ${SERVER_CPPMS})
    target_link_libraries(${TARGET}_server PUBLIC ${TARGET} vrpc_server)

    # Client library
    add_library(${TARGET}_client)
    target_sources(${TARGET}_client PUBLIC FILE_SET CXX_MODULES FILES ${CLIENT_CPPMS})
    target_link_libraries(${TARGET}_client PUBLIC ${TARGET})

    # Uart server library
    if ("uart_server" IN_LIST GENERATE_VRPC_GEN_TYPES)
        add_library(${TARGET}_uart_server)
        target_sources(${TARGET}_uart_server PUBLIC FILE_SET CXX_MODULES FILES ${UART_SERVER_CPPMS})
        target_link_libraries(${TARGET}_uart_server PUBLIC ${TARGET}_server vrpc_uart_common hal_abstract)

        if (NOT ${GENERATE_VRPC_NO_DEPEND_UART_SERVER})
            target_link_libraries(${TARGET}_uart_server PUBLIC vrpc_uart_server)
        endif ()
    endif ()

    # Uart client library
    if ("uart_client" IN_LIST GENERATE_VRPC_GEN_TYPES)
        add_library(${TARGET}_uart_client)
        target_sources(${TARGET}_uart_client PUBLIC FILE_SET CXX_MODULES FILES ${UART_CLIENT_CPPMS})
        target_link_libraries(${TARGET}_uart_client PUBLIC ${TARGET}_client vrpc_uart_client hal_abstract)
    endif ()
endfunction()

# NanoPB
set_source_files_properties(
        vendor/nanopb/pb.h
        vendor/nanopb/pb_common.c
        vendor/nanopb/pb_common.h
        vendor/nanopb/pb_decode.c
        vendor/nanopb/pb_decode.h
        vendor/nanopb/pb_encode.c
        vendor/nanopb/pb_encode.h
        PROPERTIES SKIP_LINTING ON)

add_library(nanopb STATIC
        vendor/nanopb/pb.h
        vendor/nanopb/pb_common.c
        vendor/nanopb/pb_common.h
        vendor/nanopb/pb_decode.c
        vendor/nanopb/pb_decode.h
        vendor/nanopb/pb_encode.c
        vendor/nanopb/pb_encode.h)
target_include_directories(nanopb PUBLIC vendor vendor/nanopb)

# NanoPB C++ Module
add_library(nanopb_mod)
target_sources(nanopb_mod
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        nanopb/nanopb.cppm)
target_link_libraries(nanopb_mod PUBLIC project_settings nanopb)


add_library(vrpc_common)

target_sources(vrpc_common
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        common/proto_helpers.cppm
        common/vrpc.cppm)

target_link_libraries(vrpc_common
        PRIVATE nanopb_mod
        PUBLIC hstd project_settings)


add_library(vrpc_server)
target_sources(vrpc_server
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        server/server.cppm)

target_link_libraries(vrpc_server PUBLIC vrpc_common)


add_library(vrpc_server_builtins)
target_sources(vrpc_server_builtins
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        server/builtins/server_index.cppm
        server/builtins/builtins.cppm)
target_link_libraries(vrpc_server_builtins PUBLIC vrpc_server vrpc_protocol_server)


add_library(vrpc_uart_common)
target_sources(vrpc_uart_common
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        uart/common/async_command_callback.cppm
        uart/common/decoder.cppm
        uart/common/encoder.cppm
        uart/common/frame.cppm

        uart/common/uart_common.cppm)
target_link_libraries(vrpc_uart_common
        PUBLIC vrpc_server)

generate_vrpc_module(
        vrpc_protocol_common vrpc.protocol.common protos/common generated/common
        NO_DEPEND_COMMON
)
generate_vrpc_module(
        vrpc_protocol vrpc.protocol protos/services generated/services
        GEN_TYPES uart_server uart_client
        NO_DEPEND_UART_SERVER)

add_library(vrpc_uart_server)
target_sources(vrpc_uart_server
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        uart/server/server.cppm)

target_link_libraries(vrpc_uart_server
        PUBLIC vrpc_protocol_uart_server vrpc_server_builtins vrpc_uart_common  hal_abstract)


add_library(vrpc_uart_client)
target_sources(vrpc_uart_client
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        uart/client/client_base.cppm
        uart/client/service_collection.cppm
        uart/client/transport.cppm

        uart/client/client.cppm)

target_link_libraries(vrpc_uart_client
        PUBLIC vrpc_protocol_uart_server vrpc_server_builtins vrpc_uart_common hal_abstract)
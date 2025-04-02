include(CMakeParseArguments)

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


add_library(nanopb_headers INTERFACE
        vendor/nanopb/pb.h
        vendor/nanopb/pb_common.h
        vendor/nanopb/pb_decode.h
        vendor/nanopb/pb_decode.h)
target_include_directories(nanopb_headers INTERFACE vendor vendor/nanopb)

function(GENERATE_VRPC TARGET SRCDIR DSTDIR)
    set(OPTION_ARGS NO_DEPEND_COMMON)
    set(SINGLE_VALUE_ARGS)
    set(MULTI_VALUE_ARGS GEN_TYPES)

    cmake_parse_arguments(GENERATE_VRPC "${OPTION_ARGS}" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    file(GLOB_RECURSE PROTOS "${SRCDIR}/*.proto")

    get_filename_component(SRCDIR_ABS "${SRCDIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    get_filename_component(DSTDIR_ABS "${DSTDIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

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
            set(PROTO_SVC_HDR_REL_PATH "${PROTO_BASE_PATH}.h")
            set(PROTO_HDR_REL_PATH "${PROTO_BASE_PATH}.pb.h")
            set(PROTO_SRC_REL_PATH "${PROTO_BASE_PATH}.pb.c")

            set(PROTO_SVC_HDR_ABS_PATH "${DSTDIR_ABS}/${PROTO_SVC_HDR_REL_PATH}")
            list(APPEND SVC_HDRS "${PROTO_SVC_HDR_ABS_PATH}")

            # Add source files
            set(PROTO_HDR_ABS_PATH "${DSTDIR_ABS}/${PROTO_HDR_REL_PATH}")
            set(PROTO_SRC_ABS_PATH "${DSTDIR_ABS}/${PROTO_SRC_REL_PATH}")
            list(APPEND GEN_SRCS "${PROTO_HDR_ABS_PATH}" "${PROTO_SRC_ABS_PATH}")

            set(GEN_CMD "${HAL_PYTHON}"
                    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/vendor/nanopb/generator/nanopb_generator.py"
                    "${PROTO_REL_PATH}"
                    -D "${DSTDIR_ABS}"
                    --quiet --cpp-descriptors)
        endif ()

        if (NOT ${GENERATE_VRPC_NO_DEPEND_COMMON})
            list(APPEND GEN_CMD "--protoc-opt='--proto_path=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/protos/common'")
        endif ()

        # Add UART headers if required
        if ("uart_server" IN_LIST GENERATE_VRPC_GEN_TYPES)
            list(APPEND UART_SERVER_HDRS "${DSTDIR_ABS}/${PROTO_BASE_PATH}_uart_server_service.h")
        endif()

        if ("uart_client" IN_LIST GENERATE_VRPC_GEN_TYPES)
            list(APPEND UART_CLIENT_HDRS "${DSTDIR_ABS}/${PROTO_BASE_PATH}_uart_service_client.h")
        endif()

        add_custom_command(
                DEPENDS "${PROTO_ABS_PATH}"
                OUTPUT "${PROTO_HDR_ABS_PATH}" "${PROTO_SRC_ABS_PATH}"
                WORKING_DIRECTORY "${SRCDIR_ABS}"
                COMMAND ${GEN_CMD})
    endforeach ()

    # Command for generating service headers
    #            --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/vendor/nanopb/generator/proto"
    add_custom_command(
            OUTPUT ${SVC_HDRS}
            DEPENDS ${PROTOS}
            COMMAND
            vrpc_gen
            --gen service
            --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/protos/common"
            "${SRCDIR_ABS}"
            "${DSTDIR_ABS}")

    # Command for generating UART headers
    if ("uart_server" IN_LIST GENERATE_VRPC_GEN_TYPES)
        add_custom_command(
                OUTPUT ${UART_SERVER_HDRS}
                DEPENDS ${PROTOS}
                COMMAND
                vrpc_gen
                --gen uart-server-service
                --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/protos/common"
                "${SRCDIR_ABS}"
                "${DSTDIR_ABS}")
    endif()

    if ("uart_client" IN_LIST GENERATE_VRPC_GEN_TYPES)
        add_custom_command(
                OUTPUT ${UART_CLIENT_HDRS}
                DEPENDS ${PROTOS}
                COMMAND
                vrpc_gen
                --gen uart-service-client
                --common-dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/protos/common"
                "${SRCDIR_ABS}"
                "${DSTDIR_ABS}")
    endif()

    # Regular library
    add_library(${TARGET} STATIC ${GEN_SRCS} ${SVC_HDRS} ${UART_SERVER_HDRS} ${UART_CLIENT_HDRS})
    target_include_directories(${TARGET} PUBLIC "${DSTDIR_ABS}")
    target_link_libraries(${TARGET}
            PUBLIC nanopb)

    set_source_files_properties(${GEN_SRCS} PROPERTIES SKIP_LINTING ON)

    # "Bare" library
    add_library(${TARGET}_bare STATIC ${GEN_SRCS})
    target_include_directories(${TARGET}_bare PUBLIC "${DSTDIR_ABS}")
    target_link_libraries(${TARGET}_bare
            PUBLIC nanopb_headers)

    if (NOT ${GENERATE_VRPC_NO_DEPEND_COMMON})
        target_link_libraries(${TARGET} PUBLIC vrpc_common_protos)
        target_link_libraries(${TARGET}_bare PUBLIC vrpc_common_protos_bare)

        if ("uart_server" IN_LIST GENERATE_VRPC_GEN_TYPES)
            target_link_libraries(${TARGET} PUBLIC vrpc_uart_server)
        endif()

        if ("uart_client" IN_LIST GENERATE_VRPC_GEN_TYPES)
            target_link_libraries(${TARGET} PUBLIC vrpc_uart_client)
        endif()
    endif ()
endfunction()

generate_vrpc(vrpc_common_protos protos/common generated/common NO_DEPEND_COMMON)
generate_vrpc(vrpc_system_protos protos/services generated/services GEN_TYPES uart uart_server uart_client)

add_library(vrpc STATIC
        parameters.h
        proto_helpers.h
        proto_helpers.cpp
        server.h
        vrpc.h)
target_include_directories(vrpc PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
target_link_libraries(vrpc PUBLIC nanopb halstd statechart)


add_library(vrpc_builtins INTERFACE
        builtins/server_index.h)
target_link_libraries(vrpc_builtins INTERFACE vrpc)


# vRPC UART
add_library(vrpc_uart_common STATIC
        uart/vrpc_uart_decoder.h
        uart/vrpc_uart_encode.cpp
        uart/vrpc_uart_encode.h
        uart/vrpc_uart.h
        uart/vrpc_uart_frames.h)
target_link_libraries(vrpc_uart_common PUBLIC
        vrpc abstract_hal
        constexpr_tools
        vrpc_system_protos vrpc_builtins)

add_library(vrpc_uart_server INTERFACE
        uart/server/server.h)
target_link_libraries(vrpc_uart_server INTERFACE
        vrpc_uart_common)

add_library(vrpc_uart_client INTERFACE
        uart/client/service_client_base.h
        uart/client/service_collection.h
        uart/client/transport.h)
target_link_libraries(vrpc_uart_client INTERFACE
        vrpc_uart_common)
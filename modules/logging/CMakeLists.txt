# Logging abstract library
add_library(logging_abstract)
target_sources(logging_abstract
        PUBLIC
        FILE_SET CXX_MODULES FILES
        abstract/logging_abstract.cppm

        abstract/enumeration.cppm
)

target_link_libraries(logging_abstract
        PUBLIC
        hstd
        hal_usb_abstract)

# Logging library
add_library(logging)
target_sources(logging
        PUBLIC
        FILE_SET CXX_MODULES FILES
        logging.cppm

        encoding/binary.cppm
)

target_link_libraries(logging
        PUBLIC
        hstd
        rtos_concepts
        logging_abstract)

# Logging sinks
add_library(logging_sink_usb)
target_sources(logging_sink_usb
        PUBLIC
        FILE_SET CXX_MODULES FILES
        sink/usb_cdc.cppm)
target_link_libraries(logging_sink_usb
        PUBLIC
        hstd
        rtos_concepts
        hal_usb_abstract
        logging_abstract)

add_library(logging_sink_rtos_uart)
target_sources(logging_sink_rtos_uart
        PUBLIC
        FILE_SET CXX_MODULES FILES
        sink/rtos_uart.cppm)
target_link_libraries(logging_sink_rtos_uart
        PUBLIC
        hstd
        rtos_concepts
        hal_abstract
        logging_abstract)


set_target_properties(logging_abstract logging logging_sink_usb logging_sink_rtos_uart PROPERTIES FOLDER hal/modules/logging)



# Desktop-only
if (NOT CMAKE_CROSSCOMPILING)
    # Logging spec generator
    add_library(logging_spec_gen)
    target_sources(logging_spec_gen
            PUBLIC
            FILE_SET CXX_MODULES FILES
            spec_gen/spec_gen.cppm
    )

    target_link_libraries(logging_spec_gen
            PUBLIC
            hstd
            logging_abstract
            # External libraries
            nlohmann_json::nlohmann_json
            argparse
    )

    # Test helpers
    add_library(logging_test_helpers)
    target_sources(logging_test_helpers
            PUBLIC
            FILE_SET CXX_MODULES
            FILES 
            test_helpers/logging_test_helpers.cppm)
    target_link_libraries(logging_test_helpers PUBLIC logging_abstract)

    set_target_properties(logging_spec_gen logging_test_helpers PROPERTIES FOLDER hal/modules/logging)
endif ()
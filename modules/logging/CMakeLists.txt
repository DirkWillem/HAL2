# Logging abstract library
add_library(logging_abstract)
target_sources(logging_abstract
        PUBLIC
        FILE_SET CXX_MODULES FILES
        abstract/logging_abstract.cppm
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

        sink/usb_cdc.cppm
)

target_link_libraries(logging
        PUBLIC
        hstd
        rtos_concepts
        hal_usb_abstract
        logging_abstract)

# Logging spec generator
if (NOT CMAKE_CROSSCOMPILING)
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

    add_library(logging_test_helpers)
    target_sources(logging_test_helpers
            PUBLIC
            FILE_SET CXX_MODULES
            FILES 
            test_helpers/logging_test_helpers.cppm)
    target_link_libraries(logging_test_helpers PUBLIC logging_abstract)
endif ()
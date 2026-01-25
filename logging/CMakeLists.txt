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
endif ()
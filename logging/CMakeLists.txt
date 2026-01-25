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
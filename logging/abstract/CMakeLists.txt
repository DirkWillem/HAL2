# Logging library
add_library(logging_abstract)
target_sources(logging_abstract
        PUBLIC
        FILE_SET CXX_MODULES FILES
        logging_abstract.cppm
)

target_link_libraries(logging_abstract
        PUBLIC
        hstd
        hal_usb_abstract)
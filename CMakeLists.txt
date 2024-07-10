include(cmake/setup_python.cmake)
include(cmake/utilities.cmake)

add_subdirectory(common)
add_subdirectory(vendor)
add_subdirectory(abstract)
add_subdirectory(impl)

if ("${TARGET}" STREQUAL "")
    set(TARGET "desktop")
endif()

if (${TARGET} STREQUAL "desktop")
    add_subdirectory(test)
endif()

include(cmake/add_application.cmake)

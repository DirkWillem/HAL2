include(cmake/setup_python.cmake)
include(cmake/utilities.cmake)

if (NOT CMAKE_CROSSCOMPILING)
    include(FetchContent)

    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()

add_subdirectory(common)
add_subdirectory(vendor)
add_subdirectory(rtos)
add_subdirectory(abstract)
add_subdirectory(modbus)
add_subdirectory(impl)

if ("${TARGET}" STREQUAL "")
    set(TARGET "desktop")
endif()

if (${TARGET} STREQUAL "desktop")
    add_subdirectory(test)
endif()

include(cmake/add_application.cmake)

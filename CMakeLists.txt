include(cmake/utilities.cmake)

if (NOT CMAKE_CROSSCOMPILING)
    # nlohmann::json library
    include(FetchContent)

    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(json)

    # argparse library
    include(FetchContent)
    FetchContent_Declare(
            argparse
            GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    )
    FetchContent_MakeAvailable(argparse)
endif ()

add_subdirectory(vendor)
add_subdirectory(core)
add_subdirectory(impl)

if (NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(test)
endif()

include(cmake/add_application.cmake)


function(ADD_LOGGING)
    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/logging)

    # If not cross-compiling, add unit tests.
    if (NOT CROSSCOMPILING)
        add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test/modules/logging)
    endif()
endfunction()

function(ADD_MODBUS)
    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/modbus)

    # If not cross-compiling, add unit tests.
    if (NOT CROSSCOMPILING)
        add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test/modules/logging)
    endif()
endfunction()

function(ADD_FREERTOS)
    if (CMAKE_CROSSCOMPILING)
        add_freertos_kernel()
        add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/rtos_freertos)
    endif()
endfunction()

function(ADD_USB)
    if (CMAKE_CROSSCOMPILING)
        add_tinyusb_core()
        add_add_tinyusb_function()
        add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/usb)
    endif()

endfunction()
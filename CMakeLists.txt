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

include(cmake/add_application.cmake)


function(ADD_LOGGING)
    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/logging)
endfunction()

function(ADD_MODBUS)
    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/modbus)
endfunction()

function(ADD_FREERTOS)
    add_freertos_kernel()
    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/rtos_freertos)
endfunction()

function(ADD_USB)
    add_tinyusb_core()
    add_add_tinyusb_function()
    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/modules/usb)
endfunction()
include(CMakeParseArguments)

function(add_application NAME)
    set(SINGLE_VALUE_ARGS MCU TARGET)
    set(MULTI_VALUE_ARGS SRCS LIBS)

    cmake_parse_arguments(ADD_APPLICATION "" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    add_executable("${NAME}_${ADD_APPLICATION_TARGET}" ${ADD_APPLICATION_SRCS})
    target_link_libraries("${NAME}_${ADD_APPLICATION_TARGET}" PRIVATE
            ${ADD_APPLICATION_MCU}_linkopts

            ${ADD_APPLICATION_LIBS}
            -Wl,--whole-archive
            stm32_system
            ${ADD_APPLICATION_MCU}_hal
            -Wl,--no-whole-archive)
    target_include_directories("${NAME}_${ADD_APPLICATION_TARGET}" PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/target/${ADD_APPLICATION_TARGET}")

    add_custom_command(
            TARGET "${NAME}_${ADD_APPLICATION_TARGET}"
            POST_BUILD
            COMMAND ${CMAKE_SIZE} --format=berkeley "${NAME}_${ADD_APPLICATION_TARGET}${CMAKE_EXECUTABLE_SUFFIX_CXX}")

endfunction()

function(add_application_modules NAME)
    set(SINGLE_VALUE_ARGS MCU TARGET)
    set(MULTI_VALUE_ARGS SRCS LIBS FULL_LIBS)

    cmake_parse_arguments(ADD_APPLICATION "" "${SINGLE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    add_executable("${NAME}_${ADD_APPLICATION_TARGET}" ${ADD_APPLICATION_SRCS})
    target_link_libraries("${NAME}_${ADD_APPLICATION_TARGET}" PRIVATE
            ${ADD_APPLICATION_MCU}_linkopts
            project_settings

            ${ADD_APPLICATION_LIBS}
            -Wl,--whole-archive
            stm32_system
            ${ADD_APPLICATION_MCU}_hal_mod
            ${ADD_APPLICATION_FULL_LIBS}
            -Wl,--no-whole-archive)
    target_include_directories("${NAME}_${ADD_APPLICATION_TARGET}" PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}"
            "${CMAKE_CURRENT_LIST_DIR}/target/${ADD_APPLICATION_TARGET}")

    add_custom_command(
            TARGET "${NAME}_${ADD_APPLICATION_TARGET}"
            POST_BUILD
            COMMAND ${CMAKE_SIZE} --format=berkeley "${NAME}_${ADD_APPLICATION_TARGET}${CMAKE_EXECUTABLE_SUFFIX_CXX}")

endfunction()

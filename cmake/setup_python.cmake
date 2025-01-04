include(FindPython)

# Find python interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)


set(VENV_DIR "${CMAKE_CURRENT_LIST_DIR}/../.venv")

if (NOT EXISTS "${VENV_DIR}")
    message("Creating Python venv at ${VENV_DIR}")

    execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV_DIR}")
    set(HAL_PYTHON "${VENV_DIR}/bin/python3")
    set(HAL_PIP "${VENV_DIR}/bin/pip3")
    set(HAL_PYTHON "${VENV_DIR}/bin/python3" PARENT_SCOPE)
    set(HAL_PIP "${VENV_DIR}/bin/pip3" PARENT_SCOPE)

    message("Installing Python dependencies")
    execute_process(COMMAND "${HAL_PIP}" install --upgrade pip)
    execute_process(COMMAND "${HAL_PIP}" install -r "${CMAKE_CURRENT_LIST_DIR}/requirements.txt")
else()
    set(HAL_PYTHON "${VENV_DIR}/bin/python3")
    set(HAL_PIP "${VENV_DIR}/bin/pip3")
    set(HAL_PYTHON "${VENV_DIR}/bin/python3" PARENT_SCOPE)
    set(HAL_PIP "${VENV_DIR}/bin/pip3" PARENT_SCOPE)
endif()
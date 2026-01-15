set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)
# clang does not properly link binaries without custom linker flags and scripts

# when cross-compiling so tell cmake to only test if clang can compile object files
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PREFIX "")

# Find llvm from homebrew if running under macOS
find_program(HOMEBREW brew)
if (HOMEBREW)
    execute_process(COMMAND "${HOMEBREW}" --prefix llvm
            RESULT_VARIABLE FIND_LLVM_STATUS
            OUTPUT_VARIABLE LLVM_PREFIX
            ERROR_VARIABLE FIND_LLVM_ERR
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (FIND_LLVM_STATUS EQUAL 0)
        set(TOOLCHAIN_PREFIX "${LLVM_PREFIX}/bin/")
        message("Using homebrew LLVM at ${LLVM_PREFIX}/bin")
    else()
        message("Did not find LLVM")
    endif()
endif()


set(TOOLCHAIN_SUFFIX "")
set(CLANG_TARGET_TRIPLE "armv8m-none-eabi")
set(MACHINE_FLAGS -mcpu=cortex-m33 -mfloat-abi=hard)

if (NOT "${CLANG_EXPLICIT_VERSION}" STREQUAL "")
set(CLANG_VERSION_SUFFIX "-${CLANG_EXPLICIT_VERSION}")
else()
set(CLANG_VERSION_SUFFIX "")
endif()


set(CMAKE_AR           ${TOOLCHAIN_PREFIX}llvm-ar${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX})
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}clang${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} )
set(CMAKE_ASM_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}clang${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} ${MACHINE_FLAGS})
set(CMAKE_C_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}clang++${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} ${MACHINE_FLAGS})
set(CMAKE_CXX_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})
set(CMAKE_LINKER       ${TOOLCHAIN_PREFIX}ld.lld${TOOLCHAIN_SUFFIX})
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}llvm-objcopy${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}llvm-ranlib${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}llvm-size${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_STRIP        ${TOOLCHAIN_PREFIX}llvm-strip${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_GCOV         ${TOOLCHAIN_PREFIX}llvm-cov${CLANG_VERSION_SUFFIX}${TOOLCHAIN_SUFFIX} gcov CACHE INTERNAL "")

set(GCC_PREFIX arm-none-eabi-)

execute_process(COMMAND ${GCC_PREFIX}gcc ${MACHINE_FLAGS} --print-sysroot OUTPUT_VARIABLE CMAKE_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GCC_PREFIX}gcc ${MACHINE_FLAGS} --print-multi-directory OUTPUT_VARIABLE ARM_MULTI_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GCC_PREFIX}gcc ${MACHINE_FLAGS} -print-libgcc-file-name OUTPUT_VARIABLE ARM_LIBGCC_FULLPATH OUTPUT_STRIP_TRAILING_WHITESPACE)

get_filename_component(ARM_LIBGCC_DIR ${ARM_LIBGCC_FULLPATH} DIRECTORY)
if ("${CMAKE_SYSROOT}" STREQUAL "")
set(CMAKE_SYSROOT "/usr/lib/arm-none-eabi")
set(ARM_LIBGCC_DIR "/usr/lib/gcc/arm-none-eabi/10.3.1")
endif()

function(set_flags_from_gcc compiler lang OUTVAR)
set(GCC_INVOCATION ${GCC_PREFIX}${compiler} ${MACHINE_FLAGS} --specs=nosys.specs --specs=nano.specs)

execute_process(
COMMAND ${CMAKE_COMMAND} -E echo
COMMAND ${GCC_INVOCATION} -E -Wp,-v -x ${lang} -
ERROR_VARIABLE ARM_INCLUDE_DIRS
OUTPUT_QUIET)

string(REGEX MATCH "#include <\\.\\.\\.> search starts here:.*End of search list\\." ARM_INCLUDE_DIRS "${ARM_INCLUDE_DIRS}")
string(REGEX REPLACE "#include <\\.\\.\\.> search starts here:[ \n]*(.*)[ \n]*End of search list\\." "\\1" ARM_INCLUDE_DIRS "${ARM_INCLUDE_DIRS}")
string(REGEX REPLACE " *\n *" "\n" ARM_INCLUDE_DIRS "${ARM_INCLUDE_DIRS}")
string(REPLACE "\n" ";" ARM_INCLUDE_DIRS ${ARM_INCLUDE_DIRS})

set(${OUTVAR} ${ARM_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()


set_flags_from_gcc(gcc c CMAKE_C_STANDARD_INCLUDE_DIRECTORIES)
set_flags_from_gcc(g++ c++ CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES)

# ${CMAKE_SYSROOT}/newlib/${ARM_MULTI_DIR}/crt0.o
set(CMAKE_C_STANDARD_LIBRARIES "-L${ARM_LIBGCC_DIR} -L${CMAKE_SYSROOT}/lib/${ARM_MULTI_DIR} ${ARM_LIBGCC_DIR}/crti.o ${ARM_LIBGCC_DIR}/crtbegin.o -lm -Wl,--start-group -lgcc -lc_nano -lnosys -Wl,--end-group ${ARM_LIBGCC_DIR}/crtend.o ${ARM_LIBGCC_DIR}/crtn.o")
set(CMAKE_CXX_STANDARD_LIBRARIES "-D_LIBCPP_FREESTANDING  -L${ARM_LIBGCC_DIR} -L${CMAKE_SYSROOT}/lib/${ARM_MULTI_DIR} ${ARM_LIBGCC_DIR}/crti.o ${ARM_LIBGCC_DIR}/crtbegin.o -lstdc++_nano -lm -Wl,--start-group -lgcc -lc_nano -lnosys -Wl,--end-group ${ARM_LIBGCC_DIR}/crtend.o ${ARM_LIBGCC_DIR}/crtn.o")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-D_LIBCPP_FREESTANDING -nodefaultlibs -Wl,-nostdlib -Wl,--gc-sections")
#set(CMAKE_CXX_FLAGS_INIT -stdlib=libstdc++)

set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_EXECUTABLE_SUFFIX_C   .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)
set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)

# Clang-tidy setup
# set(CMAKE_CXX_CLANG_TIDY "${TOOLCHAIN_PREFIX}clang-tidy")

set(TARGET "arm-cortex-m33")


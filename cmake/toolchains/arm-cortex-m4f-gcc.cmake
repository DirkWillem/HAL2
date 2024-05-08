set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# arm-none-eabi-gcc does not properly link binaries without custom linker flags and scripts
# so tell cmake to only test if arm-none-eabi-gcc can compile object files
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MACHINE_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")

set(CMAKE_AR           arm-none-eabi-ar${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER   arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX} ${MACHINE_FLAGS})
set(CMAKE_CXX_COMPILER arm-none-eabi-g++${CMAKE_EXECUTABLE_SUFFIX} ${MACHINE_FLAGS})
set(CMAKE_LINKER       arm-none-eabi-ld${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_OBJCOPY      arm-none-eabi-objcopy${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_RANLIB       arm-none-eabi-ranlib${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_SIZE         arm-none-eabi-size${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_STRIP        arm-none-eabi-strip${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_GCOV         arm-none-eabi-gcov${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs -Wl,--gc-sections")

set(CMAKE_EXECUTABLE_SUFFIX_C   .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)
set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)

set(TARGET "arm-cortex-m4")

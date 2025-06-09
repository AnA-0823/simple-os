cmake_minimum_required(VERSION 3.15)
project(simple-os C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-none-elf-gcc)
set(CMAKE_ASM_COMPILER aarch64-none-elf-gcc)
set(CMAKE_OBJCOPY aarch64-none-elf-objcopy)

set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O0 -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/linker.ld -nostdlib")

add_executable(kernel.elf
        src/boot.S
        src/main.c
)

add_custom_command(TARGET kernel.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary kernel.elf kernel.bin
        COMMENT "Generating kernel.bin"
)

add_custom_target(qemu
        COMMAND ${CMAKE_COMMAND} --build . --target kernel.elf
        COMMAND qemu-system-aarch64
        -machine virt
        -cpu cortex-a53
        -nographic
        -serial mon:stdio
        -kernel kernel.bin
        DEPENDS kernel.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running QEMU with kernel.bin"
)

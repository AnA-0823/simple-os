cmake_minimum_required(VERSION 3.15)
project(simple-os C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
#set(CMAKE_SOURCE_DIR src)

# 工具链前缀
set(TOOLPREFIX "aarch64-none-elf-")

# 设置工具链
set(CMAKE_C_COMPILER ${TOOLPREFIX}gcc)
set(CMAKE_ASM_COMPILER ${TOOLPREFIX}gcc)
set(CMAKE_LINKER ${TOOLPREFIX}ld)
set(CMAKE_OBJCOPY ${TOOLPREFIX}objcopy)
set(CMAKE_OBJDUMP ${TOOLPREFIX}objdump)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Os -g -fno-omit-frame-pointer -mcpu=cortex-a72+nofp")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -fno-common -nostdlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector")

# 禁用PIE
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-pie -no-pie")

# 设置汇编选项
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Og -ggdb -mcpu=cortex-a72 -MD -I.")

# 设置链接选项
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/src/kernel/kernel.ld -nostdlib -z max-page-size=4096")

# 添加源文件
set(SOURCES
        src/kernel/boot.S
        src/kernel/main.c
        src/kernel/uart.c
        src/kernel/proc.c
        src/kernel/switch.S
)

# 创建可执行文件
add_executable(kernel.elf ${SOURCES})

# 生成二进制文件
add_custom_command(TARGET kernel.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary kernel.elf kernel.bin
        COMMAND ${CMAKE_OBJDUMP} -S kernel.elf > kernel.asm
        COMMAND ${CMAKE_OBJDUMP} -t kernel.elf > kernel.tmp
        COMMAND sed "1,/SYMBOL TABLE/d" kernel.tmp | awk '{print \$NF}' > kernel.sym
        COMMAND ${CMAKE_COMMAND} -E remove kernel.tmp
        COMMENT "Generating kernel.bin, kernel.asm, and kernel.sym"
)

# 设置QEMU运行目标
set(CPUS 1 CACHE STRING "Number of CPUs to use in QEMU")
add_custom_target(qemu
        COMMAND ${CMAKE_COMMAND} --build . --target kernel.elf
        COMMAND qemu-system-aarch64
        -cpu cortex-a72
        -machine virt,gic-version=3
        -kernel kernel.bin
        -m 128M
        -smp ${CPUS}
        -nographic
        -serial mon:stdio
        -monitor none
        -d guest_errors
        -D qemu.log
        DEPENDS kernel.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running QEMU with kernel.bin"
)

# 添加清理目标
add_custom_target(clean-all
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
        COMMENT "Cleaning all build files"
)

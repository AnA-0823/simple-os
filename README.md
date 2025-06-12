## simple-os

### 简介

一个简单的操作系统，基于ARMv8-A指令集架构，使用AArch64

本项目的目的是为了验证自己独立开发简单操作系统并在未来拓展为复杂操作系统的可行性

现计划采用宏内核的内核架构，未来计划更改为微内核的内核架构设计

### 指令集架构

ARMv8-A,AArch64

### 开发工具

- CMake v3.28.3
- aarch64-linux-gnu-gcc v13.3.0

### 核心功能

- 内存管理
- 进程调度
- 设备驱动（串口）
- 文件系统

### 运行

```shell
mkdir "build"
cd build
cmake ..
make qemu
```

### 清理编译
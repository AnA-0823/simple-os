# simple-os

一个简单的操作系统，基于ARMv8-A指令集架构，使用AArch64

本项目的目的是为了验证自己独立开发简单操作系统并在未来拓展为复杂操作系统的可行性

现计划采用宏内核的内核架构，未来计划更改为微内核的内核架构设计

## 指令集架构

ARMv8-A,AArch64

## 开发工具

- CMake v3.28.3
- Arm GNU Toolchain 14.2.Rel1 (Build arm-14.52)
- qemu-system-aarch64 v8.2.2

## 核心功能

- 内存管理
- 进程调度
- 设备驱动（串口）
- 文件系统

## 构建和运行

### 基本构建

```bash
mkdir build
cd build
cmake ..
```

### 运行 QEMU（自动创建磁盘镜像）

```bash
# 在 build 目录下
make qemu
```

此命令会：

1. 编译内核
2. 自动创建 10MB 的 disk.img 文件
3. 启动 QEMU 并配置 virtio-blk 设备

### 手动创建磁盘镜像

```bash
# 在 build 目录下
make create-disk
```

### 清理所有文件

```bash
# 在 build 目录下
make clean-all
```

## 故障排除

### 磁盘设备未找到

确保使用 `make qemu` 而不是直接运行 QEMU，这样会自动创建磁盘镜像。

### 编译错误

检查工具链是否正确安装：

```bash
aarch64-none-elf-gcc --version
```

### QEMU 启动失败

确保 QEMU 版本支持 ARM64 virt 机器：

```bash
qemu-system-aarch64 --version
```
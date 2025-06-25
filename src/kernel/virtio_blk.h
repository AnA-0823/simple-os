#ifndef _VIRTIO_BLK_H
#define _VIRTIO_BLK_H

#include "types.h"

// Virtio MMIO 寄存器定义
#define VIRTIO_MMIO_MAGIC_VALUE        0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION            0x004 // 版本；1为legacy
#define VIRTIO_MMIO_DEVICE_ID          0x008 // 设备类型；1为网卡，2为磁盘
#define VIRTIO_MMIO_VENDOR_ID          0x00c // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES    0x010
#define VIRTIO_MMIO_DRIVER_FEATURES    0x020
#define VIRTIO_MMIO_GUEST_PAGE_SIZE    0x028 // 用于PFN的页大小，只写
#define VIRTIO_MMIO_QUEUE_SEL          0x030 // 选择队列，只写
#define VIRTIO_MMIO_QUEUE_NUM_MAX      0x034 // 当前队列的最大大小，只读
#define VIRTIO_MMIO_QUEUE_NUM          0x038 // 当前队列的大小，只写
#define VIRTIO_MMIO_QUEUE_ALIGN        0x03c // 已用环对齐，只写
#define VIRTIO_MMIO_QUEUE_PFN          0x040 // 队列的物理页号，读/写
#define VIRTIO_MMIO_QUEUE_READY        0x044 // 就绪位
#define VIRTIO_MMIO_QUEUE_NOTIFY       0x050 // 只写
#define VIRTIO_MMIO_INTERRUPT_STATUS   0x060 // 只读
#define VIRTIO_MMIO_INTERRUPT_ACK      0x064 // 只写
#define VIRTIO_MMIO_STATUS             0x070 // 读/写

// 状态寄存器位定义
#define VIRTIO_CONFIG_S_ACKNOWLEDGE    1
#define VIRTIO_CONFIG_S_DRIVER         2
#define VIRTIO_CONFIG_S_DRIVER_OK      4
#define VIRTIO_CONFIG_S_FEATURES_OK    8

// 设备特性位定义
#define VIRTIO_BLK_F_RO                5    // 磁盘为只读
#define VIRTIO_BLK_F_SCSI              7    // 支持SCSI命令直通
#define VIRTIO_BLK_F_CONFIG_WCE        11   // 配置中可用写回模式
#define VIRTIO_BLK_F_MQ                12   // 支持多个虚拟队列
#define VIRTIO_F_ANY_LAYOUT            27
#define VIRTIO_RING_F_INDIRECT_DESC    28
#define VIRTIO_RING_F_EVENT_IDX        29

// 描述符数量，必须是2的幂
#define VIRTIO_NUM_DESC                8

// 单个描述符结构
struct virtq_desc {
    uint64 addr;
    uint32 len;
    uint16 flags;
    uint16 next;
};

#define VRING_DESC_F_NEXT  1 // 链接到另一个描述符
#define VRING_DESC_F_WRITE 2 // 设备写入（vs 读取）

// 可用环结构
struct virtq_avail {
    uint16 flags; // 总是零
    uint16 idx;   // 驱动程序将写入 ring[idx]
    uint16 ring[VIRTIO_NUM_DESC]; // 链头描述符编号
    uint16 unused;
};

// 已用环中的一个条目
struct virtq_used_elem {
    uint32 id;   // 已完成描述符链开始的索引
    uint32 len;
};

// 已用环结构
struct virtq_used {
    uint16 flags; // 总是零
    uint16 idx;   // 设备添加 ring[] 条目时递增
    struct virtq_used_elem ring[VIRTIO_NUM_DESC];
};

// 块设备特定的定义
#define VIRTIO_BLK_T_IN  0 // 读取磁盘
#define VIRTIO_BLK_T_OUT 1 // 写入磁盘

// 磁盘请求格式
struct virtio_blk_req {
    uint32 type; // VIRTIO_BLK_T_IN 或 VIRTIO_BLK_T_OUT
    uint32 reserved;
    uint64 sector;
};

// 函数声明
void virtio_blk_init(void);
int virtio_blk_rw(char *buf, uint32 sector, int write);

#endif

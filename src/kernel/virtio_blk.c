#include "virtio_blk.h"
#include "uart.h"
#include "memlayout.h"
#include "mm.h"

// 获取virtio MMIO寄存器地址
#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))

// 页大小定义
#define PGSIZE 4096
#define PGSHIFT 12

// 磁盘设备结构
static struct disk {
    // 用于virtio驱动和设备通信的内存页面
    // 必须由两个连续的页面对齐的物理内存页面组成
    char pages[2 * PGSIZE];

    // 描述符数组
    struct virtq_desc *desc;

    // 可用环
    struct virtq_avail *avail;

    // 已用环
    struct virtq_used *used;

    // 我们自己的簿记
    char free[VIRTIO_NUM_DESC];  // 描述符是否空闲？
    uint16 used_idx; // 我们已经查看了used[2..NUM]这么远

    // 跟踪正在进行的操作信息
    // 用于完成中断到达时使用
    // 按链的第一个描述符索引
    struct {
        char *buf;
        char status;
        int write;
        uint32 sector;
    } info[VIRTIO_NUM_DESC];

    // 磁盘命令头
    // 与描述符一一对应，方便使用
    struct virtio_blk_req ops[VIRTIO_NUM_DESC];

} __attribute__ ((aligned (PGSIZE))) disk;

// 初始化virtio块设备
void virtio_blk_init(void) {
    uint32 status = 0;

    // 检查设备标识
    uint32 magic = *R(VIRTIO_MMIO_MAGIC_VALUE);
    uint32 version = *R(VIRTIO_MMIO_VERSION);
    uint32 device_id = *R(VIRTIO_MMIO_DEVICE_ID);
    uint32 vendor_id = *R(VIRTIO_MMIO_VENDOR_ID);

    if(magic != 0x74726976 ||
       version != 1 ||
       device_id != 2 ||
       vendor_id != 0x554d4551) {
        uart_puts("ERROR: could not find virtio disk\n");
        return;
    }

    // 设置状态：确认设备
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = status;

    // 设置状态：驱动程序已找到
    status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS) = status;

    // 协商特性
    uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // 告诉设备特性协商完成
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // 告诉设备我们完全准备好了
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    *R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;

    // 初始化队列0
    *R(VIRTIO_MMIO_QUEUE_SEL) = 0;
    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if(max == 0) {
        uart_puts("ERROR: virtio disk has no queue 0\n");
        return;
    }
    if(max < VIRTIO_NUM_DESC) {
        uart_puts("ERROR: virtio disk max queue too short\n");
        return;
    }
    *R(VIRTIO_MMIO_QUEUE_NUM) = VIRTIO_NUM_DESC;
    memset(disk.pages, 0, sizeof(disk.pages));
    *R(VIRTIO_MMIO_QUEUE_PFN) = (uint64)disk.pages >> PGSHIFT;

    // 设置描述符、可用环和已用环的指针
    disk.desc = (struct virtq_desc *) disk.pages;
    disk.avail = (struct virtq_avail *)(disk.pages + VIRTIO_NUM_DESC * sizeof(struct virtq_desc));
    disk.used = (struct virtq_used *) (disk.pages + PGSIZE);

    // 所有描述符初始化为未使用
    for(int i = 0; i < VIRTIO_NUM_DESC; i++) {
        disk.free[i] = 1;
    }

    // 设置队列就绪
    *R(VIRTIO_MMIO_QUEUE_READY) = 1;

    uart_puts("Virtio block device initialized\n");
}

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

// 查找空闲描述符，标记为非空闲，返回其索引
static int alloc_desc(void) {
    for(int i = 0; i < VIRTIO_NUM_DESC; i++) {
        if(disk.free[i]) {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

// 标记描述符为空闲
static void free_desc(int i) {
    if(i >= VIRTIO_NUM_DESC) {
        uart_puts("ERROR: free_desc: invalid index\n");
        return;
    }
    if(disk.free[i]) {
        uart_puts("ERROR: free_desc: already free\n");
        return;
    }
    disk.desc[i].addr = 0;
    disk.desc[i].len = 0;
    disk.desc[i].flags = 0;
    disk.desc[i].next = 0;
    disk.free[i] = 1;
}

// 释放描述符链
static void free_chain(int i) {
    while(1) {
        int flag = disk.desc[i].flags;
        int nxt = disk.desc[i].next;
        free_desc(i);
        if(flag & VRING_DESC_F_NEXT)
            i = nxt;
        else
            break;
    }
}

// 分配三个描述符（不需要连续）
// 磁盘传输总是使用三个描述符
static int alloc3_desc(int *idx) {
    for(int i = 0; i < 3; i++) {
        idx[i] = alloc_desc();
        if(idx[i] < 0) {
            for(int j = 0; j < i; j++)
                free_desc(idx[j]);
            return -1;
        }
    }
    return 0;
}

static void wait_for_done(void) {
    while(disk.used->idx == disk.used_idx) {
        // 检查并确认设备中断
        uint32 status = *R(VIRTIO_MMIO_INTERRUPT_STATUS);
        if (status & 1) { // Bit 0 表示“已用缓冲区通知”
            *R(VIRTIO_MMIO_INTERRUPT_ACK) = status; // 确认中断
            asm volatile("dsb sy" ::: "memory");
        }
    }
}

// 块设备读写操作
int virtio_blk_rw(char *buf, uint32 sector, int write) {
    int idx[3];
    char status = 0xFF; // 初始化 status 为一个非零值，以便观察变化

    // 分配三个描述符
    if(alloc3_desc(idx) < 0) {
        uart_puts("ERROR: failed to allocate descriptors\n");
        return -1;
    }

    // 设置请求头
    struct virtio_blk_req *req = &disk.ops[idx[0]];
    req->type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    req->reserved = 0;
    req->sector = sector;

    // 设置第一个描述符（请求头）
    disk.desc[idx[0]].addr = (uint64)req;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    // 设置第二个描述符（数据缓冲区）
    disk.desc[idx[1]].addr = (uint64)buf;
    disk.desc[idx[1]].len = 512; // 扇区大小
    disk.desc[idx[1]].flags = VRING_DESC_F_NEXT | (write ? 0 : VRING_DESC_F_WRITE);
    disk.desc[idx[1]].next = idx[2];

    // 设置第三个描述符（状态字节）
    disk.desc[idx[2]].addr = (uint64)&status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE;
    disk.desc[idx[2]].next = 0;

    // 保存操作信息
    disk.info[idx[0]].buf = buf;
    disk.info[idx[0]].write = write;
    disk.info[idx[0]].sector = sector;

    // 将描述符添加到可用环
    int avail_idx = disk.avail->idx % VIRTIO_NUM_DESC;
    disk.avail->ring[avail_idx] = idx[0];
    disk.avail->idx++;

    // 通知设备
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

    // 等待完成
    wait_for_done();

    // 处理完成的请求
    while(disk.used->idx != disk.used_idx) {
        int id = disk.used->ring[disk.used_idx % VIRTIO_NUM_DESC].id;
        free_chain(id);
        disk.used_idx++;
    }

    // 检查状态
    if(status != 0) {
        uart_puts("ERROR: disk operation failed with status ");
        uart_put_hex(status);
        uart_puts("!\n");
        return -1;
    }
    return 0;
}

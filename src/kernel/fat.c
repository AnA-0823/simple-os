#include "fat.h"
#include "virtio_blk.h"
#include "uart.h"
#include "mm.h"

static struct fat_bpb bpb;
static uint32 fat_start_sector;
static uint32 root_dir_sector;
static uint32 data_start_sector;
static uint32 root_dir_sectors;
static uint32 sectors_per_fat;
static uint32 bytes_per_sector;
static uint8 sectors_per_cluster;
static uint8 num_fats;

static int read_sector(uint32 sector, void *buf) {
    return virtio_blk_rw((char*)buf, sector, 0);
}
static int write_sector(uint32 sector, const void *buf) {
    return virtio_blk_rw((char*)buf, sector, 1);
}

int fat_init() {
    uint8 buf[512];
    if (read_sector(0, buf) != 0) return -1;
    memcpy(&bpb, buf, sizeof(struct fat_bpb));
    bytes_per_sector = bpb.bytes_per_sector;
    sectors_per_cluster = bpb.sectors_per_cluster;
    sectors_per_fat = bpb.sectors_per_fat;
    num_fats = bpb.num_fats;
    fat_start_sector = bpb.reserved_sectors;
    root_dir_sectors = ((bpb.root_entries * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
    root_dir_sector = fat_start_sector + num_fats * sectors_per_fat;
    data_start_sector = root_dir_sector + root_dir_sectors;
    return 0;
}

static int fat_read_dir_sector(uint32 sector, struct fat_dir_entry *entries, int max_entries) {
    uint8 buf[512];
    if (read_sector(sector, buf) != 0) return -1;
    memcpy(entries, buf, max_entries * sizeof(struct fat_dir_entry));
    return 0;
}

int fat_list_dir(const char *path, struct fat_dir_entry *entries, int max_entries) {
    int count = 0;
    for (uint32 s = 0; s < root_dir_sectors; s++) {
        uint8 buf[512];
        if (read_sector(root_dir_sector + s, buf) != 0) break;
        struct fat_dir_entry *p = (struct fat_dir_entry*)buf;
        int ents_per_sec = bytes_per_sector / sizeof(struct fat_dir_entry);
        for (int i = 0; i < ents_per_sec; i++) {
            if (count < max_entries) {
                entries[count++] = p[i];
            }
        }
    }
    return count;
}

static uint32 get_fat_entry(uint32 cluster) {
    uint8 buf[512];
    uint32 fat_offset = cluster * 2;
    uint32 fat_sector = fat_start_sector + (fat_offset / bytes_per_sector);
    uint32 ent_offset = fat_offset % bytes_per_sector;
    if (read_sector(fat_sector, buf) != 0) return 0xFFFF;
    return *(uint16*)(buf + ent_offset);
}

static int find_file_in_root(const char *name, struct fat_dir_entry *entry) {
    struct fat_dir_entry entries[16];
    if (fat_read_dir_sector(root_dir_sector, entries, 16) != 0) return -1;
    for (int i = 0; i < 16; i++) {
        if (memcmp(entries[i].name, name, 11) == 0) {
            memcpy(entry, &entries[i], sizeof(struct fat_dir_entry));
            return 0;
        }
    }
    return -1;
}

// 查找根目录空闲目录项
static int find_free_dir_entry(struct fat_dir_entry *entry, int *idx) {
    struct fat_dir_entry entries[16];
    if (fat_read_dir_sector(root_dir_sector, entries, 16) != 0) return -1;
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) { // 空或已删除
            if (entry) *entry = entries[i];
            if (idx) *idx = i;
            return 0;
        }
    }
    return -1;
}

// 查找FAT表空闲簇
static uint32 find_free_cluster() {
    uint8 buf[512];
    for (uint32 cl = 2; cl < (sectors_per_fat * bytes_per_sector * 8) / 16; cl++) {
        uint32 fat_offset = cl * 2;
        uint32 fat_sector = fat_start_sector + (fat_offset / bytes_per_sector);
        uint32 ent_offset = fat_offset % bytes_per_sector;
        if (read_sector(fat_sector, buf) != 0) return 0;
        if (*(uint16*)(buf + ent_offset) == 0x0000) {
            return cl;
        }
    }
    return 0;
}

// 设置FAT表项
static int set_fat_entry(uint32 cluster, uint16 val) {
    uint8 buf[512];
    uint32 fat_offset = cluster * 2;
    uint32 fat_sector = fat_start_sector + (fat_offset / bytes_per_sector);
    uint32 ent_offset = fat_offset % bytes_per_sector;
    if (read_sector(fat_sector, buf) != 0) return -1;
    *(uint16*)(buf + ent_offset) = val;
    if (write_sector(fat_sector, buf) != 0) return -1;
    return 0;
}

// 创建根目录新文件
static int create_file_in_root(const char *name, struct fat_dir_entry *entry) {
    int idx;
    if (find_free_dir_entry(entry, &idx) != 0) return -1;
    uint32 cl = find_free_cluster();
    if (cl == 0) return -1;
    // 分配FAT表项
    if (set_fat_entry(cl, 0xFFF8) != 0) return -1; // 标记为文件结尾
    // 构造目录项
    struct fat_dir_entry new_entry;
    for (int i = 0; i < 11; i++) new_entry.name[i] = name[i];
    new_entry.attr = 0x20; // 普通文件
    new_entry.reserved = 0;
    new_entry.ctime_ms = 0;
    new_entry.ctime = 0;
    new_entry.cdate = 0;
    new_entry.adate = 0;
    new_entry.first_cluster_high = (cl >> 16) & 0xFFFF;
    new_entry.mtime = 0;
    new_entry.mdate = 0;
    new_entry.first_cluster_low = cl & 0xFFFF;
    new_entry.size = 0;
    // 写回目录项
    uint8 buf[512];
    if (read_sector(root_dir_sector, buf) != 0) return -1;
    ((struct fat_dir_entry*)buf)[idx] = new_entry;
    if (write_sector(root_dir_sector, buf) != 0) return -1;
    if (entry) *entry = new_entry;
    // 清空新簇
    uint8 zero[512] = {0};
    for (uint8 i = 0; i < sectors_per_cluster; i++) {
        uint32 sector = data_start_sector + (cl - 2) * sectors_per_cluster + i;
        write_sector(sector, zero);
    }
    return 0;
}

int fat_read_file(const char *name, void *buf, uint32 size, uint32 offset) {
    struct fat_dir_entry entry;
    if (find_file_in_root(name, &entry) != 0) return -1;
    uint32 cluster = (entry.first_cluster_high << 16) | entry.first_cluster_low;
    uint32 file_offset = 0;
    uint32 remain = size;
    uint8 sector_buf[512];
    while (cluster >= 2 && cluster < 0xFFF8 && remain > 0) {
        for (uint8 i = 0; i < sectors_per_cluster && remain > 0; i++) {
            uint32 sector = data_start_sector + (cluster - 2) * sectors_per_cluster + i;
            if (read_sector(sector, sector_buf) != 0) return -1;
            uint32 to_copy = (remain > 512) ? 512 : remain;
            memcpy((uint8*)buf + file_offset, sector_buf, to_copy);
            file_offset += to_copy;
            remain -= to_copy;
        }
        cluster = get_fat_entry(cluster);
    }
    return file_offset;
}

int fat_write_file(const char *name, const void *buf, uint32 size, uint32 offset) {
    struct fat_dir_entry entry;
    if (find_file_in_root(name, &entry) != 0) {
        // 文件不存在，自动新建
        if (create_file_in_root(name, &entry) != 0) return -1;
    }
    uint32 cluster = (entry.first_cluster_high << 16) | entry.first_cluster_low;
    uint32 file_offset = 0;
    uint32 remain = size;
    uint8 sector_buf[512];
    while (cluster >= 2 && cluster < 0xFFF8 && remain > 0) {
        for (uint8 i = 0; i < sectors_per_cluster && remain > 0; i++) {
            uint32 sector = data_start_sector + (cluster - 2) * sectors_per_cluster + i;
            uint32 to_copy = (remain > 512) ? 512 : remain;
            memcpy(sector_buf, (const uint8*)buf + file_offset, to_copy);
            if (write_sector(sector, sector_buf) != 0) return -1;
            file_offset += to_copy;
            remain -= to_copy;
        }
        cluster = get_fat_entry(cluster);
    }
    // 更新文件大小
    uint8 dir_buf[512];
    if (read_sector(root_dir_sector, dir_buf) != 0) return -1;
    struct fat_dir_entry *entries = (struct fat_dir_entry*)dir_buf;
    for (int i = 0; i < 16; i++) {
        if (memcmp(entries[i].name, name, 11) == 0) {
            entries[i].size = size;
            break;
        }
    }
    if (write_sector(root_dir_sector, dir_buf) != 0) return -1;
    return file_offset;
} 
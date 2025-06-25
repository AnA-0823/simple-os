#ifndef _FAT_H_
#define _FAT_H_
#include "types.h"

struct fat_bpb {
    uint8  jmp[3];
    uint8  oem[8];
    uint16 bytes_per_sector;
    uint8  sectors_per_cluster;
    uint16 reserved_sectors;
    uint8  num_fats;
    uint16 root_entries;
    uint16 total_sectors_short;
    uint8  media_descriptor;
    uint16 sectors_per_fat;
    uint16 sectors_per_track;
    uint16 num_heads;
    uint32 hidden_sectors;
    uint32 total_sectors_long;
    // ... 省略其他字段
} __attribute__((packed));

struct fat_dir_entry {
    char name[11];
    uint8 attr;
    uint8 reserved;
    uint8 ctime_ms;
    uint16 ctime;
    uint16 cdate;
    uint16 adate;
    uint16 first_cluster_high;
    uint16 mtime;
    uint16 mdate;
    uint16 first_cluster_low;
    uint32 size;
} __attribute__((packed));

int fat_init();
int fat_read_file(const char *name, void *buf, uint32 size, uint32 offset);
int fat_write_file(const char *name, const void *buf, uint32 size, uint32 offset);
int fat_list_dir(const char *path, struct fat_dir_entry *entries, int max_entries);

#endif // _FAT_H_ 
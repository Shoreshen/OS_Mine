#pragma once
#include "lib.h"

#pragma region functions
FS_entry * in_parent(char* name, FS_entry *parent);
FS_entry * path_walk(char* name, unsigned long flags);
FS_entry * Search_Dir(char* name, int name_len, FS_entry *parent);
FS_entry * init_entry(FAT32_Directory * dir_entry, FS_entry *parent, char *name, int p_offset);
void DISK1_FAT32_FS_init();
unsigned int Read_FAT(FAT32_sb_info * fsbi, unsigned int cluster);
unsigned long WRITE_FAT(FAT32_sb_info * fsbi, unsigned int cluster,unsigned int value);
super_block * fat32_read_superblock(Disk_Partition_Table_Entry *DPTE);
unsigned long register_filesystem(file_system_type *fs);
super_block* mount_fs(char *name, Disk_Partition_Table_Entry *DPTE);
long FAT32_open(FS_entry *entry);
long FAT32_close(FS_entry *entry);
long FAT32_read(FS_entry *entry, void *buf, unsigned long count);
long FAT32_write(FS_entry *entry, void *buf, unsigned long count);
long FAT32_lseek(FS_entry *entry, long offset, unsigned long whence);
long FAT32_lsDir(FS_entry *parent);
unsigned long FAT32_find_available_cluster(FAT32_sb_info * fsbi);
void FAT32_write_dir(FS_entry *entry, FAT32_sb_info *fsbi);
#pragma endregion
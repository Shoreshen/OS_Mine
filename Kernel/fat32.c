#include "fat32.h"
#include "keyboard.h"

#pragma region VFS
file_ops FAT32_File_ops = {
	.open  = FAT32_open,
	.close = FAT32_close,
	.read  = FAT32_read,
	.write = FAT32_write,
	.lseek = FAT32_lseek,
	.ioctl = NULL,
};
dir_ops FAT32_dir_ops = {
    .lsdir = FAT32_lsDir,
};
file_ops keyboard_ops = {
	.open  = keyboard_open,
	.close = keyboard_close,
	.ioctl = keyboard_ioctl,
	.read  = keyboard_read,
	.write = keyboard_write,
    .lseek = NULL,
};
super_block * root_sb = NULL;
file_system_type filesystem = {"filesystem",0,NULL,NULL};
file_system_type FAT32_fs_type=
{
	.name = "FAT32",
	.fs_flags = 0,
	.read_superblock = fat32_read_superblock,
	.next = NULL,
};
super_block* mount_fs(char *name, Disk_Partition_Table_Entry *DPTE)
{
    file_system_type *p = NULL;
	for(p = &filesystem;p;p = p->next){
        if(!strcmp(name,p->name)){
            return p->read_superblock(DPTE);
        }
    }
    return NULL;
}
unsigned long register_filesystem(file_system_type *fs)
{
	file_system_type *p = NULL;

	for(p = &filesystem;p->next;p = p->next){
        if(!strcmp(fs->name,p->name)){
            return 0;
        }
    }
    p->next = fs;
    fs->next = NULL;
	return 1;
}
#pragma endregion

#pragma region FAT32_Ops
super_block * fat32_read_superblock(Disk_Partition_Table_Entry *DPTE)
{
    super_block *sb = kmalloc(sizeof(super_block),0);
    FAT32_sb_info *sbinfo = kmalloc(sizeof(FAT32_sb_info),0);
    memset(sb,0,sizeof(super_block));
    memset(sbinfo,0,sizeof(FAT32_sb_info));
    sb->private_sb_info = sbinfo;

    AHCI_REQ(DPTE->start_LBA, 1, &sbinfo->fat32_bootsector, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
    if(MACRO_PRINT){
        color_printk(BLUE,BLACK,"FAT32 Boot Sector\n\tBPB_FSInfo:%04X\n\tBPB_BkBootSec:%04X\n\tBPB_TotSec32:%08X\n",
            sbinfo->fat32_bootsector.BPB_FSInfo,
            sbinfo->fat32_bootsector.BPB_BkBootSec,
            sbinfo->fat32_bootsector.BPB_TotSec32);
    }
    AHCI_REQ(DPTE->start_LBA + sbinfo->fat32_bootsector.BPB_FSInfo, 1, 
        &sbinfo->fat32_fsinfo, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
    if(MACRO_PRINT){
        color_printk(BLUE,BLACK,"FAT32 FSInfo\n\tFSI_LeadSig:%08X\n\tFSI_StrucSig:%08X\n\tFSI_Free_Count:%d\n",
            sbinfo->fat32_fsinfo.FSI_LeadSig,
            sbinfo->fat32_fsinfo.FSI_StrucSig,
            sbinfo->fat32_fsinfo.FSI_Free_Count);
    }
    
    sbinfo->FirstDataSector = DPTE[0].start_LBA + sbinfo->fat32_bootsector.BPB_RsvdSecCnt + sbinfo->fat32_bootsector.BPB_FATSz32 * sbinfo->fat32_bootsector.BPB_NumFATs;
    //                      = 2048 + 0 + 2048 * 2 = 6144
	sbinfo->FirstFAT1Sector = DPTE[0].start_LBA + sbinfo->fat32_bootsector.BPB_RsvdSecCnt;
	sbinfo->FirstFAT2Sector = sbinfo->FirstFAT1Sector + sbinfo->fat32_bootsector.BPB_FATSz32;
	sbinfo->BytesPerClus    = sbinfo->fat32_bootsector.BPB_SecPerClus * sbinfo->fat32_bootsector.BPB_BytesPerSec;
    sbinfo->sector_per_FAT  = sbinfo->fat32_bootsector.BPB_FATSz32;

    //Root dir
    sb->root.name = kmalloc(2,0);
    sb->root.name[0] = '/';
    sb->root.name[1] = 0;
    sb->root.name_length = 1;
    sb->root.parent_entry = &sb->root;
    sb->root.sp = sb;
    sb->root.first_cluster = sbinfo->fat32_bootsector.BPB_RootClus;
    sb->root.attr |= ATTR_DIRECTORY;
    sb->root.type.dir.ops = &FAT32_dir_ops;

    List_Init(&sb->root.type.dir.subdir_list);
    List_Init(&sb->root.FS_node);

    sb->sb_op.Search_FS = Search_Dir;
    return sb;
}

long FAT32_open(FS_entry *entry)
{
	return 1;
}
long FAT32_close(FS_entry *entry)
{
	entry->type.file.pos = 0;
    return 1;
}
long FAT32_read(FS_entry *entry, void *buf, unsigned long count)
{
    unsigned long cluster = entry->first_cluster, index, offset, sector, length, last_cluster = 0;
    int i;
    FAT32_sb_info *fsbi = (FAT32_sb_info *)entry->sp->private_sb_info;
    char * buffer = (char *)kmalloc(fsbi->BytesPerClus, 0);

    index = entry->type.file.pos / fsbi->BytesPerClus;
    offset = entry->type.file.pos % fsbi->BytesPerClus;

    if(!cluster){
        return EFAULT;
    }
    for(i=0; i<index; i++){
        if(cluster < 0x0ffffff7){
            cluster = Read_FAT(fsbi, cluster);
        }
    }

    if(entry->type.file.pos + count > entry->type.file.size){
        count = entry->type.file.size - entry->type.file.pos;
    }

    index = count;

    while(index > 0 && last_cluster < 0x0ffffff7){
        sector = fsbi->FirstDataSector + (cluster - 2) * fsbi->fat32_bootsector.BPB_SecPerClus;
        AHCI_REQ(sector, fsbi->fat32_bootsector.BPB_SecPerClus, buffer, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);

        if(index > (fsbi->BytesPerClus - offset)){
            offset = 0;
            length = fsbi->BytesPerClus - offset;
        }else{
            length = index;
        }
        
        memcpy(buffer + offset, buf, length);
        
        index -= length;
        buf = (void *)((unsigned long)buf + length);
        last_cluster = cluster;
        entry->type.file.pos += length;

        cluster = Read_FAT(fsbi, last_cluster);
    }

    kfree(buffer);
    if(!index){
        return count;
    }
    return EIO;
}
long FAT32_write(FS_entry *entry, void *buf, unsigned long count)
{
    unsigned long cluster = entry->first_cluster, index, offset, sector, length, next_cluster;
    FAT32_sb_info *fsbi = (FAT32_sb_info *)entry->sp->private_sb_info;
    int i;
    char * buffer = (char *)kmalloc(fsbi->BytesPerClus, 0);

    index = entry->type.file.pos / fsbi->BytesPerClus;
    offset = entry->type.file.pos % fsbi->BytesPerClus;

    if(!cluster){
        FAT32_write_dir(entry, fsbi);
        WRITE_FAT(fsbi,entry->first_cluster,0x0ffffff8);
    }else{
        for(i=0;i<index;i++){
            if(cluster < 0x0ffffff7){
                cluster = Read_FAT(fsbi, cluster);
            }
        }
    }
    color_printk(GREEN,BLACK,"index: %d; offet: %d\n",index, offset);
    if(!cluster){
        return EFAULT;
    }

    index = count;

    while(index){
        memset(buffer,0,fsbi->BytesPerClus);
        
        sector = fsbi->FirstDataSector + (cluster - 2) * fsbi->fat32_bootsector.BPB_SecPerClus;
        AHCI_REQ(sector, fsbi->fat32_bootsector.BPB_SecPerClus, buffer, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
        
        if(fsbi->BytesPerClus - offset < index){
            length = fsbi->BytesPerClus - offset;
            offset = 0;
        }else{
            length = index;
        }
        
        memcpy(buf, buffer + offset, length);
        AHCI_REQ(sector, fsbi->fat32_bootsector.BPB_SecPerClus, buffer, ATA_CMD_WRITE_DMA_EX, AHCI_queue.global_schedual);
        
        buf = (void *)((unsigned long)buf + length);
        index -= length;
        entry->type.file.pos += length;

        if(index){
            next_cluster = Read_FAT(fsbi, cluster);
        }else{
            break;
        }

        if(next_cluster >= 0x0ffffff8){
            next_cluster = FAT32_find_available_cluster(fsbi);
            if(!next_cluster){
				kfree(buffer);
				return -ENOSPC;
            }
            WRITE_FAT(fsbi,cluster,next_cluster);
            WRITE_FAT(fsbi,next_cluster,0x0ffffff8);
        }
        cluster = next_cluster;
    }

    if(entry->type.file.pos > entry->type.file.size){
        entry->type.file.size = entry->type.file.pos;
    }
    
    kfree(buffer);
    
    if(!index){
        return count;
    }
    
    return EIO;
}
long FAT32_lseek(FS_entry *entry, long offset, unsigned long whence)
{
    unsigned long pos = 0;

    if(whence == SEEK_SET){
        pos = offset;
    }else if(whence == SEEK_CUR){
        pos = entry->type.file.pos + offset;
    }else if(whence == SEEK_END){
        pos = entry->type.file.size - offset;
    }else{
        return EINVAL;
    }

    if(pos < 0 || pos > entry->type.file.size){
        return EOVERFLOW;
    }

    entry->type.file.pos = pos;

    return pos;
}
long FAT32_lsDir(FS_entry *parent)
{
    unsigned int cluster = parent->first_cluster;
    unsigned int sector;
    FAT32_sb_info *fsbi = (FAT32_sb_info *)parent->sp->private_sb_info;
    FAT32_Directory * dir_clus = kmalloc(fsbi->BytesPerClus, 0);
    FS_entry *entry;
    int i, j, k, len = fsbi->BytesPerClus / sizeof(FAT32_Directory);
    char tmpname[20];
    
    if(!(parent->attr & ATTR_DIRECTORY)){
        color_printk(BLACK,GREEN,"List target is not a directory!\n");
    }
    while(cluster < 0x0ffffff7){
        sector = fsbi->FirstDataSector + (cluster - 2) * fsbi->fat32_bootsector.BPB_SecPerClus;
        AHCI_REQ(sector, fsbi->fat32_bootsector.BPB_SecPerClus, dir_clus, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
        for(i = 0; i < len; i++){
            if( dir_clus[i].DIR_Attr == ATTR_LONG_NAME ||
                dir_clus[i].DIR_Name[0] == 0xe5 || 
                dir_clus[i].DIR_Name[0] == 0x00 || 
                dir_clus[i].DIR_Name[0] == 0x05){
                continue;
            }
            j = 0;
            //Name
            for(k=0; k<8; k++){
                if(dir_clus[i].DIR_Name[k] == ' '){
                    break;
                }else {
                    tmpname[j] = dir_clus[i].DIR_Name[k];
                    j++;
                }
            }
            //Extension
            if(dir_clus[i].DIR_Name[8] != ' '){
                tmpname[j] = '.';
                j++;
                for(k=8;k<11;k++){
                    if(dir_clus[i].DIR_Name[k] == ' '){
                        break;
                    }else{
                        tmpname[j] = dir_clus[i].DIR_Name[k];
                        j++;
                    }
                }
            }
            tmpname[j] = 0;
            entry = in_parent(&tmpname[0], parent);
            if(!entry){
                entry = init_entry(&dir_clus[i], parent, &tmpname[0], i);
            }
            if(entry->attr & ATTR_DIRECTORY){
                color_printk(INDIGO, BLACK,"%s\n",entry->name);
            }else{
                if(!strcmp(&tmpname[0], "KEYBOARD.DEV")){
                    color_printk(GREEN, BLACK,"%s\n",entry->name);
                }else{
                    color_printk(WHITE, BLACK,"%s\n",entry->name);
                }
            }
        }
        cluster = Read_FAT(fsbi, cluster);
    }

    return 1;
}

void FAT32_write_dir(FS_entry *entry, FAT32_sb_info *fsbi)
{
    unsigned long sector;
    FAT32_Directory *buf = kmalloc(fsbi->BytesPerClus,0);
    
    entry->first_cluster = FAT32_find_available_cluster(fsbi);
    memset(buf,0,fsbi->BytesPerClus);

    sector = fsbi->FirstDataSector + (entry->parent_entry->first_cluster - 2) * fsbi->fat32_bootsector.BPB_SecPerClus;
    AHCI_REQ(sector, fsbi->fat32_bootsector.BPB_SecPerClus, buf, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
    buf[entry->parent_offset].DIR_FileSize = entry->type.file.size;
    buf[entry->parent_offset].DIR_FstClusLO = entry->first_cluster & 0xffff;
    buf[entry->parent_offset].DIR_FstClusHI = (buf[entry->parent_offset].DIR_FstClusHI & 0xf000) | (entry->first_cluster >> 16);

    AHCI_REQ(sector, fsbi->fat32_bootsector.BPB_SecPerClus, buf, ATA_CMD_WRITE_DMA_EX, AHCI_queue.global_schedual);
    kfree(buf);
}
#pragma endregion

#pragma region FAT32
Disk_Partition_Table    DPT;
gpt_lba1 GPT_LBA1;
gpt_part_entry *GPT_PRT_ENRTY;
void DISK1_FAT32_FS_init()
{
    unsigned char buf[512];
    int i;
    // FS_entry * entry;
 
    register_filesystem(&FAT32_fs_type);

    memset(&DPT, 0, 512);
    AHCI_REQ(0, 1, &DPT, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);//schedule back to kernel, since kernel schedule to user by interrupt, IF will be set after iret
    if(MACRO_PRINT){
        color_printk(BLUE,BLACK,"DPTE[0] start_LBA:%d\ttype:%02X\n",DPT.DPTE[0].start_LBA,DPT.DPTE[0].type);
    }

    if(DPT.DPTE[0].type == 0xEE){
        if(MACRO_PRINT){
            color_printk(ORANGE,BLACK,"GPT partition, loading details...\n");
        }
        AHCI_REQ(1, 1, &GPT_LBA1, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
        if(MACRO_PRINT){
            color_printk(BLUE,BLACK,"Sig:%s, ParEntryLBA: %d, ParNum: %d, ParSize: %d\n",
                GPT_LBA1.GPT_HEAD.Signature,
                GPT_LBA1.GPT_HEAD.ParStarLBA,
                GPT_LBA1.GPT_HEAD.ParNum,
                GPT_LBA1.GPT_HEAD.ParSize
            );
        }
        GPT_PRT_ENRTY = kmalloc(512, 0);
        AHCI_REQ(GPT_LBA1.GPT_HEAD.ParStarLBA, 1, GPT_PRT_ENRTY, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
        for(i = 0; i < 512/GPT_LBA1.GPT_HEAD.ParSize; i++){
            if(
                *(unsigned long*)&GPT_PRT_ENRTY[i].PartGUID[0] != 0x11D2F81FC12A7328 &&
                *(unsigned long*)&GPT_PRT_ENRTY[i].PartGUID[0] != 0                  &&
                *(unsigned long*)&GPT_PRT_ENRTY[i].PartGUID[8] != 0x3BC93EC9A0004BBA &&
                *(unsigned long*)&GPT_PRT_ENRTY[i].PartGUID[8] != 0
            ){
                if(MACRO_PRINT){
                    color_printk(BLUE,BLACK,"GUID:%p,%p, Name:%s, StartLBA:%d, EndLBA:%d\n",
                        *(unsigned long*)&GPT_PRT_ENRTY[i].PartGUID[0],*(unsigned long*)&GPT_PRT_ENRTY[i].PartGUID[8],
                        GPT_PRT_ENRTY[i].PartitionName,
                        GPT_PRT_ENRTY[i].StartSec,
                        GPT_PRT_ENRTY[i].EndSec
                    );
                }
                break;
            }
        }
        DPT.DPTE[1].start_LBA = GPT_PRT_ENRTY[i].StartSec;
        DPT.DPTE[1].end_sector = GPT_PRT_ENRTY[i].StartSec;
        root_sb = mount_fs("FAT32", &DPT.DPTE[1]);
    } else{
        if(MACRO_PRINT){
            color_printk(ORANGE,BLACK,"MBR partition, loading details...\n");
        }
        root_sb = mount_fs("FAT32", &DPT.DPTE[0]);
    }

    color_printk(BLUE,BLACK,"root_sb:%p; root_sb->private_sb_info:%p\n",(unsigned long)root_sb,(unsigned long)root_sb->private_sb_info);
    color_printk(BLUE,BLACK,"FirstDataSector:%d\tBytesPerClus:%d\tFirstFAT1Sector:%d\tFirstFAT2Sector:%d\n",
        ((FAT32_sb_info *)root_sb->private_sb_info)->FirstDataSector,
        ((FAT32_sb_info *)root_sb->private_sb_info)->BytesPerClus,
        ((FAT32_sb_info *)root_sb->private_sb_info)->FirstFAT1Sector,
        ((FAT32_sb_info *)root_sb->private_sb_info)->FirstFAT2Sector
    );  
    // entry = path_walk("/abc/makefile", 0);
    // color_printk(BLACK,YELLOW,"First_Cls: %d, Name: %s, parent->name: %s\n", entry->first_cluster,entry->name, entry->parent->name);
}

FS_entry * init_entry(FAT32_Directory * dir_entry, FS_entry *parent, char *name, int p_offset)
{
    FS_entry *entry = NULL;
    int namelen = strlen(name);

    entry = kmalloc(sizeof(FS_entry), 0);
    memset(entry, 0, sizeof(FS_entry));
    entry->name = kmalloc(namelen + 1, 0);
    entry->name_length = namelen;
    memcpy(name, entry->name, namelen + 1);
    entry->first_cluster = (dir_entry->DIR_FstClusHI << 16 | dir_entry->DIR_FstClusLO) & 0x0fffffff;
    entry->attr = dir_entry->DIR_Attr;
    entry->sp = parent->sp;
    entry->parent_entry = parent;
    entry->parent_offset = p_offset;
    List_Init(&entry->FS_node);
    List_Insert_After(&parent->type.dir.subdir_list,&entry->FS_node);
    if(entry->attr & ATTR_DIRECTORY){
        List_Init(&entry->type.dir.subdir_list);
        entry->type.dir.ops = &FAT32_dir_ops;
    }else{
        entry->type.file.size = dir_entry->DIR_FileSize;
        entry->type.file.pos = 0;
        entry->type.file.Data = NULL;
        if(!strcmp(entry->name, "KEYBOARD.DEV")){
            entry->type.file.ops = &keyboard_ops;
        }else{
            entry->type.file.ops = &FAT32_File_ops;
        }
    }
}

FS_entry * in_parent(char* name, FS_entry *parent)
{
    FS_entry *path = NULL;
    if(!List_is_empty(&parent->type.dir.subdir_list)){
        path = container_of(parent->type.dir.subdir_list.next, FS_entry, FS_node);
        while(&path->FS_node != &parent->type.dir.subdir_list){
            if (!stricmp(path->name, name)){
                return path;
            }
            path = container_of(path->FS_node.next, FS_entry, FS_node);
        }
    }
    return NULL;
}

FS_entry * path_walk(char* name, unsigned long flags)
{
    char* tmp_name = kmalloc(MAX_FILE_DIR_LEN,0);
    unsigned long tmp_len;
	FS_entry *parent = NULL, *path = NULL;

    if(!strcmp(name, "/")){
        return &root_sb->root;
    }

    while(*name){
        if(!parent){
            parent = &root_sb->root;
        }else{
            parent = path;
        }
        path = NULL; 
        while(*name == '/'){
            name++;
        }
        tmp_len = 0;
        while(*((char *)(name + tmp_len)) != '/' && *((char *)(name + tmp_len))){
            tmp_len++;
        }
        if(tmp_len > MAX_FILE_DIR_LEN){
            color_printk(RED,BLACK,"File/Diretory name exceed maximun lenghth!\n");
            return NULL;
        }
        memcpy(name, tmp_name, tmp_len);
        tmp_name[tmp_len] = 0;
        path = in_parent(&tmp_name[0],parent);
        if(!path){
            path = parent->sp->sb_op.Search_FS(tmp_name, tmp_len, parent);
        }
        // color_printk(BLACK,GREEN,"Check file name: %s, name length: %d\n",tmp_name, tmp_len);
        if(!path){
            color_printk(BLACK,GREEN,"Can not find file name: %s\n",tmp_name);
            return NULL;
        }
        // color_printk(BLACK,GREEN,"Found file name: %s\n",tmp_name);
        name = name + tmp_len;
    }
    if(flags & 1){
        return parent;
    }
    return path;
}
//================================================================================
//1. Only search short name
//2. "name" variable cannot caintain symbol other than "a~z,A~Z,0~9"
//================================================================================
FS_entry * Search_Dir(char* name, int name_len, FS_entry *parent)
{
    unsigned int cluster = parent->first_cluster;
    unsigned int sector;
    FAT32_sb_info *fsbi = (FAT32_sb_info *)parent->sp->private_sb_info;
    FAT32_Directory * dir_clus = kmalloc(fsbi->BytesPerClus, 0);
    FS_entry *entry;
    int i,j,k,len = fsbi->BytesPerClus / sizeof(FAT32_Directory);
next_cluster:
    sector = fsbi->FirstDataSector + (cluster - 2) * fsbi->fat32_bootsector.BPB_SecPerClus;
    AHCI_REQ(sector, fsbi->fat32_bootsector.BPB_SecPerClus, dir_clus, ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
    for(i = 0;i < len; i++){
        //DIR_Name[0] = 0x00 || 0xe5 || 0x05: Not valit entry
        if( dir_clus[i].DIR_Attr == ATTR_LONG_NAME ||
            dir_clus[i].DIR_Name[0] == 0xe5 || 
            dir_clus[i].DIR_Name[0] == 0x00 || 
            dir_clus[i].DIR_Name[0] == 0x05){
            continue;
        }
        j = 0;
        //Long: need to fill, not supporting utf-8 currently
        //Short: Base name
        // color_printk(BLACK,GREEN,"DIR_Name: %s",&dir_clus[i].DIR_Name[0]);
        for(k=0; k<8; k++){
            if(dir_clus[i].DIR_Name[k] == ' ' || name[j] == '.'){
                continue;
            }else if(name[j]>='a' && name[j]<='z'){
                name[j] -= 32;
                if((dir_clus[i].DIR_Name[k] != name[j]) || j >= name_len){
                    goto not_match;
                }
                j++;
            }else if((name[j]>='A' && name[j]<='Z') || (name[j]>='0' && name[j]<='9' || name[j]==' ')){
                if((dir_clus[i].DIR_Name[k] != name[j]) || j >= name_len){
                    goto not_match;
                }
                j++;
            }
        }
        //Short: Extention
        if(!(dir_clus[i].DIR_Attr & ATTR_DIRECTORY)){
            j++;
            for(k=8;k<11;k++){
                if(dir_clus[i].DIR_Name[k] == ' '){
                    continue;
                }else if(name[j]>='a' && name[j]<='z'){
                    name[j] -= 32;
                    if((dir_clus[i].DIR_Name[k] != name[j]) || j >= name_len){
                        goto not_match;
                    }
                    j++;
                }else if((name[j]>='A' && name[j]<='Z') || (name[j]>='0' && name[j]<='9') || name[j]==' '){
                    if((dir_clus[i].DIR_Name[k] != name[j]) || j >= name_len){
                        goto not_match;
                    }
                    j++;
                }
            }
        }
        entry = init_entry(&dir_clus[i], parent, name, i);
        kfree(dir_clus);
        return entry;
        not_match:;
    }
	cluster = Read_FAT(fsbi, cluster);
	if(cluster < 0x0ffffff7){
        goto next_cluster;
    }
    return NULL;
}

unsigned int Read_FAT(FAT32_sb_info * fsbi, unsigned int cluster)
{
	unsigned int buf[128];
	memset(buf,0,512);
    AHCI_REQ(fsbi->FirstFAT1Sector + (cluster>>7), 1, &buf[0], ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
    return buf[cluster & 0x7f] & 0x0fffffff; //First 4 bit not used, so maximum 0x0fffffff
}
unsigned long WRITE_FAT(FAT32_sb_info * fsbi, unsigned int cluster,unsigned int value)
{
	unsigned int buf[128];
	memset(buf,0,512);
    AHCI_REQ(fsbi->FirstFAT1Sector + (cluster>>7), 1, &buf[0], ATA_CMD_READ_DMA_EX, AHCI_queue.global_schedual);
    buf[cluster & 0x7f] = (buf[cluster & 0x7f] & 0xf0000000) | (value & 0x0fffffff);
    AHCI_REQ(fsbi->FirstFAT1Sector + (cluster>>7), 1, &buf[0], ATA_CMD_WRITE_DMA_EX, AHCI_queue.global_schedual);
    AHCI_REQ(fsbi->FirstFAT2Sector + (cluster>>7), 1, &buf[0], ATA_CMD_WRITE_DMA_EX, AHCI_queue.global_schedual);
	return 1;	
}
unsigned long FAT32_find_available_cluster(FAT32_sb_info * fsbi)
{
	int i,j;
	unsigned long sector_per_fat = fsbi->sector_per_FAT;
	unsigned int buf[128];

	for(i = 0;i < sector_per_fat;i++) {
		memset(buf,0,512);
        AHCI_REQ(fsbi->FirstFAT1Sector + i, 1, &buf[0], ATA_CMD_WRITE_DMA_EX, AHCI_queue.global_schedual);
		for(j = 0;j < 128;j++) {
			if((buf[j] & 0x0fffffff) == 0){
                return (i << 7) + j;
            }
		}
	}
	return 0;
}
#pragma endregion
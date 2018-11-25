#ifndef _FS_H_
#define _FS_H_

#define _POSIX_C_SOURCE 199309
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <fuse.h>
#include <sys/stat.h>
#include <stdint.h>
#include <pthread.h>

#define LOG_FILE ((FILE*)fuse_get_context()->private_data)

#define direct          10
#define single_indirect 1
#define double_indirect 1

#define size_table      (direct + single_indirect + double_indirect)
#define size_hash       10
#define size_disk       (sb.file_size)
#define size_blk        4096
#define size_max_name   118//22
#define size_max_file   (direct*size_blk + single_indirect*num_addr_per_blk*size_blk + double_indirect*num_addr_per_blk*num_addr_per_blk*size_blk)

//#define num_blk         (size_disk/size_blk)
#define num_ilist       64000//200
#define num_iblk        (sizeof(disk_inode) * num_ilist / size_blk + 1)
//#define num_datablk     (num_blk-150-num_ilist)
#define num_inode_per_blk (size_blk/sizeof(disk_inode))

#define num_addr_per_blk  (size_blk/sizeof(int))
#define num_max_ent      10000//40
#define max_open_files   num_ilist

typedef struct used_blk used_blk;
struct used_blk{
        uint32_t blk_id;
        used_blk* blk_fp;
        used_blk* blk_bp;
};
        

typedef struct{
	uint8_t owner;
	uint8_t group;
	uint8_t type;
	mode_t perms;
	uint32_t num_links;
	uint32_t table[size_table];   //12
	uint32_t file_size;
        struct timespec t1;
        struct timespec t2;
} disk_inode;

typedef struct incore_inode incore_inode;

struct incore_inode{
	disk_inode inode;
	uint8_t status;
	uint8_t num_device;
	uint32_t inumber;        
	uint32_t ref_count;
} ;

typedef struct {
	uint32_t file_size;
	uint32_t num_freeblks;
	uint32_t num_freeinodes;
	uint32_t head_freeblk;
        uint8_t init;      // 0: need to init 1: no need
        uint32_t used_inode[num_ilist];
} super_blk;

typedef struct {
	uint32_t inumber;
	uint32_t offset;
	char filename[size_max_name];
} dir_entry;


typedef struct {
	uint32_t num_ent;
	dir_entry ent[num_max_ent];
} dir_inode;


void read_inode_from_disk(incore_inode* inode);
void write_inode_to_disk(incore_inode* inode);
void read_dir_from_incore(dir_inode* dir, incore_inode* inode);
void write_dir_to_incore(dir_inode dir, incore_inode* inode);

incore_inode* iget(int inumber);
void free_diskblk(incore_inode* inode);
void iput(incore_inode* inode);
incore_inode* ialloc();

void bmap(incore_inode* inode, int offset, int* _numblk, int* _byteoffset, int* _outbyte, int* _headblk);
void get_parent_child(char* path, char* child, char* parent);
incore_inode* namei(char* path);

int unlink_file(incore_inode* inode, char* name);
int add_file_to_dir(int inumber, char* name, incore_inode* parent_inode);
int dir_empty(incore_inode* inode);


extern char*  memory;
extern int fd;
extern super_blk  sb;


#endif

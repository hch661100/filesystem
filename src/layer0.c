#include"fs2.h"

char*  memory;
int fd;


void init_disk(char* path, int flag)
{
    if ((fd = open(path, O_RDWR)) == -1) {
        printf("Failed to load the disk.\n");
        return;
    }

    FILE* f = fopen(path, "w+");
    fseek(f, 0L, SEEK_END);
    long fsize = ftell(f);
    printf("%ld\n", fsize);
    fclose(f);

    //uint32_t file_size = lseek(fd, 0, SEEK_END);
    //lseek(fd, 0, SEEK_SET);
    unsigned long file_size = fsize;

    printf("disk size is %lu\n", file_size);
    int _num_blk = file_size / size_blk;
    if(flag == 1){
       char *buf = calloc(size_blk*50000,sizeof(char));   
       for(int i=0;i<_num_blk;i+=50000){
           printf("%d \n", i);
           write_memory(buf, i, 50000);
//           if(i > 500000) break;
       }
       free(buf);
    }
    printf("Load the disk successfully!\n");    
	//memory = calloc(size_disk, sizeof(char));
}

void init_memory()
{  
   memory = calloc(size_disk, sizeof(char));
}

void free_memory()
{
     close(fd);
     //free(memory);
}

int read_memory(void* buf, int addr, int num)
{
    int tmp = fd;
    lseek(fd, 0, SEEK_SET);
    if (pread(fd, buf, num*size_blk, addr*size_blk) < 0) {
	fd = tmp;
        return -1;
    }
    
    fd = tmp;
//    memcpy(buf, memory + addr*size_blk, num*size_blk);
	return 0;
}

int write_memory(void* buf, int addr, int num)
{
    int tmp = fd;
    lseek(fd, 0, SEEK_SET);
    if (pwrite(fd, buf, num*size_blk, addr*size_blk) < 0) {
	fd = tmp;
        return -1;
    }
    
    fd = tmp;        
    //memcpy(memory + addr*size_blk, buf, num*size_blk);
    return 0;
}

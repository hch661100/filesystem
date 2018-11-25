#include"fs2.h"
#include <fuse.h>
#include <math.h>

super_blk sb;

static incore_inode* i_free;
static int used_inode[num_ilist];
static used_blk* used_block;
static int num_used_block;

void print_blk()
{
     used_blk* p = used_block;
     printf("blk: ");
     while(p != NULL){
        printf("%d ", p->blk_id);
        p = p->blk_fp;
     }
     printf("\n");
}

void print_inodes()
{
     fprintf(LOG_FILE, "ok1\n");  
     for(int i=0;i<num_ilist;i++){
            sb.used_inode[i]=used_inode[i];
            if(sb.used_inode[i]>0){
               incore_inode* inode = i_free + i;
               write_inode_to_disk(inode);
               fprintf(LOG_FILE, "write: %d: %d\n", inode->inumber, inode->inode.file_size);
            }
        }
        fprintf(LOG_FILE, "ok2\n");

        for(int i=0;i<num_ilist;i++){
            sb.used_inode[i]=used_inode[i];
            if(sb.used_inode[i]>0){
               incore_inode* inode = i_free + i;
               read_inode_from_disk(inode);
               fprintf(LOG_FILE, "read: %d: %d\n", inode->inumber, inode->inode.file_size);
            }
        }

}

void read_inode_from_disk(incore_inode* inode)
{
	disk_inode disk_p;
	int inumber = inode->inumber;
	char* buf = calloc(size_blk,1);
	if (!buf)
		return -1;
	read_memory(buf, 150 + inumber / num_inode_per_blk, 1);
	memcpy(&disk_p, buf + (inumber % num_inode_per_blk) * sizeof(disk_inode), sizeof(disk_inode));
	inode->inode = disk_p;
	free(buf);
}


void write_inode_to_disk(incore_inode* inode)
{
	disk_inode disk_p = inode->inode;
	int inumber = inode->inumber;
	char *buf = calloc(size_blk,1);
	int offset = 150 + inumber / num_inode_per_blk;
	if (!buf)
		return -1;
	read_memory(buf, offset, 1);
	memcpy(buf + (inumber % num_inode_per_blk) * sizeof(disk_inode), &disk_p, sizeof(disk_inode));
	write_memory(buf, offset, 1);
	free(buf);
}

void read_dir_from_incore(dir_inode* dir, incore_inode* inode){
/*        
	int addr_blk = inode->inode.table[0];
	int num_ent;
	dir_entry ent[num_max_ent];
	char *buf = malloc(size_blk);
	int offset = 0;
	if (!buf)
		return -1;
	read_memory(buf, addr_blk, 1);
	memcpy(&(num_ent), buf, sizeof(num_ent));
	offset = offset + sizeof(num_ent);
	memcpy(ent, buf + offset, sizeof(dir_entry)*num_max_ent);
	dir->num_ent = num_ent;
	for (int i = 0; i < num_ent; i++){
		dir->ent[i] = ent[i];
	}
	free(buf);
*/
        int num_ent = inode->inode.file_size / sizeof(dir_entry);
        dir->num_ent  = num_ent;
        dir_entry tmp_ent;
        int i;
        int blockid, inblock_offset, _numblk, _outbyte, offset = 0;
        char *buf = calloc(size_blk, sizeof(char));       
        for(i=0;i<num_ent;i++){            
            bmap(inode, offset, &_numblk, &inblock_offset, &_outbyte, &blockid);
            offset = offset + sizeof(dir_entry);
            read_memory(buf, blockid, 1);
            memcpy(&tmp_ent, buf + inblock_offset, sizeof(dir_entry));
            dir->ent[i] = tmp_ent;
        }
        free(buf);
            
}

void write_dir_to_incore(dir_inode dir, incore_inode* inode){
/*
	int addr_blk = inode->inode.table[0];
	char *buf = malloc(size_blk);
	int offset = 0;
	if (!buf)
		return -1;
	read_memory(buf, addr_blk, 1);
	memcpy(buf, &(dir.num_ent), sizeof(dir.num_ent));
	offset = offset + sizeof(dir.num_ent);
	memcpy(buf+offset, dir.ent, sizeof(dir_entry)*num_max_ent);
	write_memory(buf, addr_blk, 1);
	free(buf);
	inode->inode.file_size = dir.num_ent*sizeof(dir_entry);
*/
//        int num_ent = inode->inode.file_size / sizeof(dir_entry);

        int num_ent = dir.num_ent;
        inode->inode.file_size = num_ent * sizeof(dir_entry);
        dir_entry tmp_ent;
        int i;
        int blockid, inblock_offset, _numblk, _outbyte, offset = 0;
        char *buf = calloc(size_blk, sizeof(char));       
        for(i=0;i<num_ent;i++){            
            tmp_ent = dir.ent[i];
            bmap(inode, offset, &_numblk, &inblock_offset, &_outbyte, &blockid);
            offset = offset + sizeof(dir_entry);
            read_memory(buf, blockid, 1);
            memcpy(buf + inblock_offset, &tmp_ent, sizeof(dir_entry));
            write_memory(buf, blockid, 1);            
        }
        free(buf);
        
}



void init_superblk()
{
	
        char *buf = calloc(size_blk*149, sizeof(char));
        read_memory(buf, 1, 149);
	memcpy(&sb, buf, sizeof(sb));
        free(buf);

        sb.file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        //sb.file_size = size_disk;
	//sb.num_freeblks = _num_freeblks;
	sb.num_freeinodes = num_ilist;//_num_freeinodes;
	sb.head_freeblk = 150 + num_iblk;
	//sb.head_freeinode = 0;
	//sb.modified = 0;


}

void write_superblk()
{
 /*       
        for(int i=0;i<num_ilist;i++){
            sb.used_inode[i]=used_inode[i];
            if(sb.used_inode[i]>0){
               incore_inode* inode = i_free + i;
               write_inode_to_disk(inode);
            }
        }
*/        
        //sb.init = 1;
	char *buf = calloc(size_blk*149, sizeof(char));
	read_memory(buf, 1, 149);
	memcpy(buf, &sb, sizeof(sb));
	write_memory(buf, 1, 149);
        free(buf);
}

void print_superblk()
{
	//printf("sb.file_size = %lu\n", (unsigned long)(sb.file_size));
	//printf("sb.num_freeblks = %d\n", sb.num_freeblks);
	printf("sb.num_freeinodes = %d\n", sb.num_freeinodes);
	printf("sb.head_freeblk = %d\n", sb.head_freeblk);
        printf("size(block) = %d\n", size_blk);
	//printf("sb.head_freeinode = %d\n", sb.head_freeinode);
	printf("sizeof(sb) = %d\n", sizeof(sb));
	printf("sizeof(disk_inode) = %d\n", sizeof(disk_inode));
	printf("sizeof(incore_inode) = %d\n", sizeof(incore_inode));
        printf("sizeof(dir_entry) = %d\n", sizeof(dir_entry));
//	printf("sb.freeblks: ");


}

void insert_blk(int blk_id)
{
        used_blk* p = used_block;
        used_blk* blk = calloc(1,sizeof(used_blk));
        blk->blk_id = blk_id;
        printf("ready to insert blk %d\n", blk_id);
        if(p->blk_fp == NULL){
           blk->blk_fp = NULL;
           blk->blk_bp = p;
           p->blk_fp = blk;
           num_used_block++;
           printf("insert block: %d\n", blk->blk_id);
           return;
        }
        while(p->blk_fp!=NULL){
           p=p->blk_fp;
           if(p->blk_id > blk_id){
              blk->blk_bp = p->blk_bp;
              blk->blk_fp = p;
              p->blk_bp->blk_fp = blk;
              p->blk_bp = blk;
              num_used_block++;
              printf("insert block: %d\n", blk->blk_id);
              return;
           }
           if(p->blk_fp == NULL){
              blk->blk_fp = NULL;
              blk->blk_bp = p;
              p->blk_fp = blk;
              num_used_block++;
              printf("insert block: %d\n", blk->blk_id);
              return;
           }           
           if((p->blk_id < blk_id)&&(p->blk_fp->blk_id > blk_id)){
              blk->blk_bp = p;
              blk->blk_fp = p->blk_fp;
              p->blk_fp = blk;
              if(p->blk_fp != NULL){
                 p->blk_fp->blk_bp = blk;
              }
              printf("insert block: %d\n", blk->blk_id);
           }
        }
}

void insert_blklst(int blk_id){
	int* addr = calloc(num_addr_per_blk, sizeof(int));
	read_memory(addr, blk_id, 1);
	for (int j = 0; j < num_addr_per_blk; j++){
                if(addr[j] != 0){
  		   insert_blk(addr[j]);
                }
	}
	free(addr);

        insert_blk(blk_id);
}

void debug_build_single()
{
    printf("debug_build_single\n");
}

void debug_build_double()
{
    printf("debug_build_double\n");
}

void build_used_blk(incore_inode* inode)
{
     
        int used_table, blk_id;
	/*for (int i = 0; i < size_table; i++){
		if (inode->inode.table[i] == 0){
			used_table = i;
			break;
		}
	}*/
        used_table = ceil((double)(inode->inode.file_size)/(double)size_blk);
        if((used_table > direct)&&(used_table < direct + single_indirect*num_addr_per_blk)) used_table = 11;
        if(used_table > direct + single_indirect*num_addr_per_blk) used_table = 12;


        for (int i = 0; i < used_table; i++){
		if (i < direct){
			blk_id = inode->inode.table[i];
                        insert_blk(blk_id);
		}
		if ((i < direct + single_indirect) && (i >= direct)){
			int tmp_headblk = inode->inode.table[i];
                        debug_build_single();
			insert_blklst(tmp_headblk);
		}
		if ((i < direct + single_indirect + double_indirect) && (i >= direct + single_indirect)){
                        debug_build_double();
			int tmp_headblk1 = inode->inode.table[i];
			int tmp_headblk2;
			int* buf1 = calloc(num_addr_per_blk, sizeof(int));			
			read_memory(buf1, tmp_headblk1, 1);
			for (int j = 0; j < num_addr_per_blk; j++){
				tmp_headblk2 = buf1[j];
                                if(tmp_headblk2 != 0){
      				   insert_blklst(tmp_headblk2);	
                                }
			}
			insert_blklst(tmp_headblk1);
			free(buf1);

		}
	}            
}


void init_ifree()
{
        used_block = calloc(1,sizeof(used_blk));
        used_block->blk_fp = NULL;
        used_block->blk_bp = NULL;
        used_block->blk_id = -1;
        num_used_block = 0;

        
	i_free = calloc(max_open_files, sizeof(incore_inode));
	if (!i_free) {
		//if (errno <= 0)
		return -1;
	}

	int i;
        incore_inode* inode;


        for(i=0;i<num_ilist;i++){
           inode = i_free+i;
           inode->inumber = i;
           //used_inode[i]=0;
        }

}

void destory_ifree(){
        free(used_block);
	free(i_free);
}

void reset_incoreinode(incore_inode* inode){
     inode->status = 0;
     inode->num_device = 0;
     inode->inumber = 0;
     inode->ref_count = 0;

}

void reset_diskinode(incore_inode* inode)
{
	
	inode->inode.group = 0;
	inode->inode.owner = 0;
	inode->inode.perms = 0;
	inode->inode.type=0;
	free_diskblk(inode);
}

incore_inode* iget(int inumber)
{
	while (1)
	{

                if(inumber >= num_ilist){
                   printf("Cannot support so many inodes.\n");
                   return NULL;
                }

                incore_inode* inode;
                if(used_inode[inumber] != 0){
                  inode = i_free + inumber;
                  inode->ref_count++;
                  used_inode[inumber] = inode->ref_count;
                  return inode;
                }

		if (sb.num_freeinodes == 0){
			printf("There is no inodes on free list\n");
			return NULL;
		}

                inode = i_free + inumber;
                reset_incoreinode(inode);
                inode->ref_count = 1;
                inode->inumber = inumber;
                used_inode[inumber] = inode->ref_count;
                read_inode_from_disk(inode);
                inode->inode.group = 0;
	        inode->inode.owner = 0;
	        inode->inode.perms = 0;
	        inode->inode.type=0;
                inode->inode.file_size=0;

		
		printf("iget: %d\n", inumber);
		return inode;
	}
}

void ifree(int inumber)
{
	sb.num_freeinodes++;
//	sb.freeinodes[inumber] = inumber;
        used_inode[inumber] = 0;
	write_superblk();
}

void bfree(int blk_id){

       used_blk* p = used_block;
       while(p->blk_fp != NULL){
           p = p->blk_fp;
           if(p->blk_id == blk_id){
              p->blk_bp->blk_fp = p->blk_fp;
              if(p->blk_fp!=NULL){
                 p->blk_fp->blk_bp = p->blk_bp;
              }
              num_used_block--;

	int* buf = calloc(num_addr_per_blk, sizeof(int));
	write_memory(buf, blk_id, 1);              
	free(buf);

              printf("free block %d\n", blk_id);
              free(p);
           }
       }          
}

void free_addr(int blk_id){
	int* addr = calloc(num_addr_per_blk, sizeof(int));
	read_memory(addr, blk_id, 1);
	for (int j = 0; j < num_addr_per_blk; j++){
		bfree(addr[j]);
	}
	free(addr);

	int* buf = calloc(num_addr_per_blk, sizeof(int));
	write_memory(buf, blk_id, 1);              
	free(buf);
}

void debug_double()
{
   printf("let's debug!\n");
}

void free_diskblk(incore_inode* inode)
{
	int used_table = ceil(inode->inode.file_size / size_blk);
        if((used_table > direct)&&(used_table < direct + single_indirect*num_addr_per_blk)) used_table = 11;
        if(used_table > direct + single_indirect*num_addr_per_blk) used_table = 12;


	for (int i = 0; i < used_table; i++){
		if (i < direct){
			bfree(inode->inode.table[i]);
			inode->inode.table[i] = 0;
		}
		if ((i < direct + single_indirect) && (i >= direct)){
			int tmp_headblk = inode->inode.table[i];
			free_addr(tmp_headblk);
			bfree(inode->inode.table[i]);
			inode->inode.table[i] = 0;
		}
		if ((i < direct + single_indirect + double_indirect) && (i >= direct + single_indirect)){
                        debug_double();
			int tmp_headblk1 = inode->inode.table[i];
			int tmp_headblk2;
			int* buf1 = calloc(num_addr_per_blk, sizeof(int));
			//int* buf2 = calloc(num_addr_per_blk, sizeof(int));
			read_memory(buf1, tmp_headblk1, 1);
			for (int j = 0; j < num_addr_per_blk; j++){
				tmp_headblk2 = buf1[j];
				free_addr(tmp_headblk2);	
				bfree(tmp_headblk2);
			}
			free_addr(tmp_headblk1);
			//write_memory(buf2, tmp_headblk1, 1);
			bfree(inode->inode.table[i]);
			inode->inode.table[i] = 0;
			free(buf1);
			//free(buf2);
		}
	}
	inode->inode.file_size = 0;

}



void iput(incore_inode* inode)
{
	(inode->ref_count)--;
        write_inode_to_disk(inode);
        used_inode[inode->inumber] = inode->ref_count;
        printf("inode: %d ref_count: %d num_links: %d\n", inode->inumber, inode->ref_count, inode->inode.num_links);
	if (inode->ref_count == 1){
		if (inode->inode.num_links == 0){
                        //used_inode[inode->inumber] = 0;
			ifree(inode->inumber);	
			reset_diskinode(inode);
			write_inode_to_disk(inode);
		
		}

		//insert_inode_to_freelst(inode); 
		write_inode_to_disk(inode);
	}
	printf("iput: %d\n", inode->inumber);
	inode = NULL;
	
}


   
incore_inode* ialloc()
{
	int inumber;
	incore_inode* inode;
	while (1)
	{
		if (sb.num_freeinodes == 0){
		    printf("There is no free inodes in superblk.\n");
		    return NULL;
		}
                
                
                for(int i=0; i < num_ilist; i++){
                   if(used_inode[i]==0){
                      inumber = i;                    
                      break;
                   }
                }
                /*  replace it with no freeinodes give up 
		for (int i = 0; i < _num_freeinodes; i++)
		{
		     if (sb.freeinodes[i] != -1){
				 inumber = i;
				 sb.freeinodes[inumber] = -1;
		         break;
		     }
		}*/
		inode = iget(inumber);
      
		/////////////////////////not free how to describe?
                inode->ref_count++;
		inode->inode.num_links = 1;

                //printf("ok1\n");
		write_inode_to_disk(inode);
		sb.num_freeinodes--;
                //printf("ok2\n");
		write_superblk();
                //printf("ok3\n");
		break;
	} 
        printf("alloc\n");
	return inode;
}

int balloc()
{
	//int blk_id = -1;

        used_blk* p = used_block;
        if(p->blk_fp == NULL){
           used_blk* blk = calloc(1,sizeof(used_blk));
           blk->blk_fp = NULL;
           blk->blk_bp = p;
           blk->blk_id = sb.head_freeblk;
           p->blk_fp = blk;
           num_used_block++;
           printf("alloc block: %d\n", blk->blk_id);
           return blk->blk_id;
        }

        while(p->blk_fp != NULL){
           p = p->blk_fp;
           if(p->blk_fp == NULL){
              used_blk* blk = calloc(1,sizeof(used_blk));
              blk->blk_fp = NULL;
              blk->blk_bp = p;
              blk->blk_id = p->blk_id + 1;
              p->blk_fp = blk;
              num_used_block++;
              printf("alloc block: %d\n", blk->blk_id);
              return blk->blk_id;
           }

           used_blk* pp = p->blk_fp;
           if(p->blk_id != (pp->blk_id - 1)){
              used_blk* blk = calloc(1,sizeof(used_blk));
              blk->blk_fp = pp;
              blk->blk_bp = p;
              blk->blk_id = p->blk_id + 1;
              p->blk_fp = blk;
              pp->blk_bp = blk;
              num_used_block++;
              printf("alloc block: %d\n", blk->blk_id);
              return blk->blk_id; 
           }
        }
}

void init_rootdir(int flag)
{
     if(flag==1) sb.init = 0;
     if(flag==0) sb.init = 1;
     if(sb.init == 0){
        printf("init fs\n");
	dir_inode dir;
	int offset = 0;
	dir.num_ent = 0;
	incore_inode * inode = (incore_inode*)ialloc();
	inode->inode.perms = 0755;
	inode->inode.type = 2;             // directory
	dir_entry ent1 = { 0, offset, "." };
	dir.ent[0] = ent1;
	dir.num_ent++;
	//inode->inode.num_links++;
	offset = offset + sizeof(dir_entry);
	dir_entry ent2 = { 0, offset, ".." };
	dir.ent[1] = ent2;
	dir.num_ent++;
	inode->inode.num_links++;
	offset = offset + sizeof(dir_entry);

	//inode->inode.table[0] = balloc();
	//inode->inode.file_size = dir.num_ent*sizeof(dir_entry);
	write_dir_to_incore(dir, inode);		
	iput(inode);
     }else{
        printf("load fs\n");
        for(int i=0;i<num_ilist;i++){         
            used_inode[i]=sb.used_inode[i];
            if(used_inode[i]>0){               
               incore_inode* inode = i_free + i;
               inode->ref_count = 1;
               read_inode_from_disk(inode);
               printf("load inode %d  file_size: %d\n", inode->inumber, inode->inode.file_size);
               build_used_blk(inode);
            }
        }
        print_blk();
     }
        

}






int determine_level_indirect(int numblk)
{
	int level;
	if (numblk < direct){
		level = 0;
	}
	else if (numblk < direct + single_indirect*num_addr_per_blk){
		level = 1;
	}
	else if (numblk < direct + single_indirect*num_addr_per_blk + double_indirect*num_addr_per_blk*num_addr_per_blk){
		level = 2;
	}
	return level;
}

void bmap(incore_inode* inode, int offset, int* _numblk, int* _byteoffset, int* _outbyte, int* _headblk)
{
	int numblk = offset / size_blk;
	int byteoffset = offset % size_blk;
	int outbyte;
	int headblk;

	if (offset > inode->inode.file_size){
		printf("The offset is larger than size of file\n");
	}

	if ((numblk + 1) * size_blk > inode->inode.file_size){
		outbyte = inode->inode.file_size - byteoffset;
	}
	else{
		outbyte = (numblk + 1) * size_blk - byteoffset;
	}
	*_numblk = numblk;
	*_byteoffset = byteoffset;
	*_outbyte = outbyte;

	int level = determine_level_indirect(numblk);

	if (level == 0){
		if (inode->inode.table[numblk] == 0){
			inode->inode.table[numblk] = balloc();
		}
		headblk = inode->inode.table[numblk];
	}
	else if (level == 1){
		int indir_addr = direct; 
		if (inode->inode.table[indir_addr] == 0){
			inode->inode.table[indir_addr] = balloc();
		}
		int tmp_headblk = inode->inode.table[indir_addr];
		int indir_offset = (numblk - direct) % num_addr_per_blk;
		int* buf = calloc(num_addr_per_blk, sizeof(int));
		read_memory(buf, tmp_headblk, 1);
		if (buf[indir_offset] == 0){
			buf[indir_offset] = balloc();
		}
		write_memory(buf, tmp_headblk, 1);
		headblk = buf[indir_offset];
		free(buf);
	}
	else if (level == 2){
		int indir_addr1 = direct + single_indirect; 
		if (inode->inode.table[indir_addr1] == 0){
			inode->inode.table[indir_addr1] = balloc();
		}
		int tmp_headblk1 = inode->inode.table[indir_addr1];
		int indir_offset1 = (numblk - direct - num_addr_per_blk) / num_addr_per_blk;

		int* buf = calloc(num_addr_per_blk, sizeof(int));
		read_memory(buf, tmp_headblk1, 1);
		if (buf[indir_offset1] == 0){
			buf[indir_offset1] = balloc();
		}		
		write_memory(buf, tmp_headblk1, 1);
		int headblk2 = buf[indir_offset1];
		free(buf);

		buf = calloc(num_addr_per_blk, sizeof(int));
		read_memory(buf, headblk2, 1);
		int indir_offset2 = (numblk - direct - num_addr_per_blk - num_addr_per_blk*headblk2) % num_addr_per_blk;
		if (buf[indir_offset2] == 0){
			buf[indir_offset2] = balloc();
		}
                write_memory(buf, headblk2, 1);////////

		headblk = buf[indir_offset2];

		free(buf);
	}
	*_headblk = headblk;

}

void get_parent_child(char* path, char* child, char* parent){
	char * name = malloc(strlen(path) + 1);
	strcpy(name, path);
	char *next_path = strtok(name, "/");

	while (next_path){
		strcpy(child, next_path);
		next_path = strtok(NULL, "/");

	}
	
	int a = strlen(path);
	int b = strlen(child);
	strcpy(name, path);
	int len_parent = strlen(path) - strlen(child)-1;
	strncpy(parent, name, len_parent);
	parent[len_parent] = '\0';
	free(name);
}

incore_inode* namei(char* path)
{
	incore_inode* working_inode;
	dir_inode working_dir;
	char * name = malloc(strlen(path) + 1);
        printf("namei: %s\n", path);
	strcpy(name, path);
/*	if (strncmp(name, "/", 1) == 0){
		working_inode = iget(0);
	}
	else{
		printf("This is %s", name);
	}*/
	working_inode = iget(0);
	
	char *next_path = strtok(name, "/");
	while (next_path){
		if (1){
			if ((working_inode->inode).type != 2){
				iput(working_inode);
				printf("This is not directory.");
			}
			read_dir_from_incore(&working_dir, working_inode);

			int i;
			for (i = 0; i < working_dir.num_ent; i++){
				if (strcmp(next_path, working_dir.ent[i].filename)==0){
					int inumber = working_dir.ent[i].inumber;
					iput(working_inode);
					working_inode = iget(inumber);
					break;
				}
                               
			}
                        if(i==working_dir.num_ent){
                           iput(working_inode);                        //chong huang
                           printf("There is no file %s\n", next_path);
                           return NULL;
                        }
		}
		next_path = strtok(NULL, "/");
	}
	return working_inode;
}

int unlink_file(incore_inode* inode, char* name)
{
	dir_inode dir;
	dir_entry tmp_ent[num_max_ent];
	int flag = 0;
	read_dir_from_incore(&dir, inode);
	for (int i = 0, j=0; i < dir.num_ent; i++){		
		if (strcmp(dir.ent[i].filename, name) == 0){
			flag = 1;
			continue;
		}
		tmp_ent[j] = dir.ent[i];
		j++;
	}
	if (flag == 1){
		dir.num_ent--;
	}
	int offset=0;
	for (int i = 0; i < dir.num_ent; i++){
		tmp_ent[i].offset = offset;
		dir.ent[i] = tmp_ent[i];
		offset = offset + sizeof(dir_entry);
	}
	write_dir_to_incore(dir, inode);
	return flag;
}


int add_file_to_dir(int inumber, char* name, incore_inode* parent_inode)
{
	//incore_inode* parent_inode = namei(parent);
	dir_inode dir;
	
	int flag = 1;
	read_dir_from_incore(&dir, parent_inode);
	for (int i = 0; i < dir.num_ent; i++){
		if (strcmp(dir.ent[i].filename, name) == 0){
			flag = 0;
			printf("%s existed.", name);
			return flag;
		}
	}


	int offset = dir.num_ent*sizeof(dir_entry);
	dir_entry tmp;
	tmp.inumber = inumber;
	tmp.offset = offset;
	strcpy(tmp.filename, name);

	dir.ent[dir.num_ent] = tmp;
	dir.num_ent++;

	write_dir_to_incore(dir, parent_inode);
	//parent_inode->inode.file_size += sizeof(dir_entry);
	return flag;
}

int dir_empty(incore_inode* inode)
{
	dir_inode dir;
	read_dir_from_incore(&dir, inode);
	if (dir.num_ent == 2){
		return 1;
	}
	else{
		return 0;
	}

}

void change_name(incore_inode* parent_inode, char* childFrom, char* childTo)
{
        dir_inode dir;
	
	int flag = 0;
	read_dir_from_incore(&dir, parent_inode);
	for (int i = 0; i < dir.num_ent; i++){
		if (strcmp(dir.ent[i].filename, childFrom) == 0){
			flag = 1;
			printf("%s would be renamed by %s.\n", childFrom, childTo);
                        strcpy(dir.ent[i].filename, childTo);
			break;
		}
	}
        write_dir_to_incore(dir, parent_inode);
       // return 1;
}

void move_file(incore_inode* parent_inode, incore_inode*  incoreFromC, char* childTo)
{
        dir_inode dir;
	dir_entry ent;

        if(incoreFromC->inode.type == 2){
          read_dir_from_incore(&dir, incoreFromC);
          for (int i = 0; i < dir.num_ent; i++){
   	     if (strcmp(dir.ent[i].filename, "..") == 0){
                dir.ent[i].inumber = parent_inode->inumber;
                break;
	     }
          }               
          write_dir_to_incore(dir, incoreFromC);
        }
        if(incoreFromC->inode.type == 1){
           incoreFromC->ref_count++;
        }

	read_dir_from_incore(&dir, parent_inode);
        ent.inumber = incoreFromC->inumber;
        //incore_inode* inode = iget(ent.inumber);
        
        //iput(inode);
        ent.offset = dir.num_ent * sizeof(dir_entry);
        strcpy(ent.filename, childTo);
        dir.ent[dir.num_ent] = ent;
        dir.num_ent++;
        write_dir_to_incore(dir, parent_inode);                      
}

int same_name(incore_inode * parent_inode, char* childFrom)
{
      dir_inode dir;  
      read_dir_from_incore(&dir, parent_inode);
      int flag = 0;
      for (int i = 0; i < dir.num_ent; i++){
	  if (strcmp(dir.ent[i].filename, childFrom) == 0){
		printf("%s is namesakes.\n", childFrom);
                incore_inode* inode = iget(dir.ent[i].inumber);
                if(inode->inode.type == 1) flag = 1;
                if(inode->inode.type == 2) flag = 2;
                iput(inode);
                break;
	  }
      }      
      return flag;
}

int cur_num_ent(char* path)
{
    incore_inode* parent_inode = namei(path);
    dir_inode dir;  
    read_dir_from_incore(&dir, parent_inode);
    iput(parent_inode);
    return dir.num_ent;
}

int truncate_inode(incore_inode* inode, long file_size)
{
    long old_file_size = inode->inode.file_size;
    int blockid, inblock_offset, _numblk, _outbyte, offset, num_trun=0;
    int* buf = calloc(num_addr_per_blk, sizeof(int));
    int* buf2 = calloc(num_addr_per_blk, sizeof(int));

    for(offset=old_file_size; offset>=file_size; offset-=size_blk){
       bmap(inode, offset, &_numblk, &inblock_offset, &_outbyte, &blockid);
       bfree(blockid);
       if(_numblk < direct){
          inode->inode.table[_numblk] = 0;
       }
       if((_numblk >= direct) && (_numblk < direct + single_indirect*num_addr_per_blk)){
          read_memory(buf, inode->inode.table[direct], 1);
          buf[_numblk-direct]=0;
          write_memory(buf, inode->inode.table[direct], 1);
          if(_numblk == direct) {
             bfree(inode->inode.table[direct]);
             inode->inode.table[direct] = 0;
          }
       }
       if(_numblk >= direct + single_indirect*num_addr_per_blk){
          read_memory(buf, inode->inode.table[direct + single_indirect], 1);
          int addr1 = (_numblk - direct - single_indirect*num_addr_per_blk) / num_addr_per_blk;          
          int addr2 = (_numblk - direct - single_indirect*num_addr_per_blk) % num_addr_per_blk;
          
          read_memory(buf2, addr1, 1);
          buf2[addr2] = 0;
          write_memory(buf2, addr1, 1);
 
          if(addr2 == 0){
             bfree(buf[addr1]);
             buf[addr1] = 0;
          }

          write_memory(buf, inode->inode.table[direct + single_indirect], 1);
          if(_numblk == direct + single_indirect*num_addr_per_blk){
             bfree(inode->inode.table[direct + single_indirect]);
             inode->inode.table[direct + single_indirect] = 0;
          }
       }
       num_trun++;
    }

    num_trun++;
    free(buf);
    free(buf2);

    return num_trun;
}

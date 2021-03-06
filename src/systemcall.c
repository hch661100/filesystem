#include "fs2.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>




static void* tagliatelle_init(struct fuse_conn_info *conn)	
{

        fprintf(LOG_FILE,"tagliatelle_init\n");

	return (FILE*)fuse_get_context()->private_data;
}

	static int tagliatelle_getattr(const char *path, struct stat *stbuf)	//get attribute
{
        fprintf(LOG_FILE,"tagliatelle_getattr\n");
        print_blk();
        print_inodes();
        write_superblk();
	memset(stbuf, 0, sizeof(struct stat));
	incore_inode* incore=namei(path);
	if (!incore)
	{
		return -ENOENT;
	}
	if (incore->inode.type == 1)	//regular file
	{
		stbuf->st_ino = incore->inumber;
		stbuf->st_mode = __S_IFREG | incore->inode.perms;		//S_IFREG  0100000
		stbuf->st_size = incore->inode.file_size;
		stbuf->st_nlink = incore->inode.num_links;
		stbuf->st_atime = stbuf->st_mtime = time(NULL);		
		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();
		iput(incore);
		return 0;
	}
	if (incore->inode.type == 2)	//directory
	{
		stbuf->st_ino = incore->inumber;
		stbuf->st_mode = __S_IFDIR | incore->inode.perms;		//S_IFDIR  0040000
		stbuf->st_nlink = incore->inode.num_links;
		stbuf->st_atime = stbuf->st_mtime = time(NULL);		
		stbuf->st_uid = getuid();			
		stbuf->st_gid = getgid();
		iput(incore);
		return 0;
	}
	else
	{
		iput(incore);
		return -ENOENT;
	}
}

static int tagliatelle_open(const char *path, struct fuse_file_info *fi)	
{
        fprintf(LOG_FILE,"tagliatelle_open\n");
	incore_inode* incore = namei(path);			
	if (!incore)
	{
		return -ENOENT;
	}
	if (fi->flags & O_RDWR)		//read&write
	{
		if ((incore->inode.perms&__S_IREAD) && (incore->inode.perms&__S_IWRITE)){
			fi->fh=(uint64_t)incore;
			return 0;
		}else{
			return -EACCES;
                }
	}
	if (fi->flags ^ O_RDONLY)	//read only
	{
		if (incore->inode.perms&__S_IREAD){
			fi->fh=(uint64_t)incore;
			return 0;
		}else{
			return -EACCES;
                }
	}
	if (fi->flags & O_WRONLY)	//write only
	{
		if (incore->inode.perms&__S_IWRITE){
			fi->fh=(uint64_t)incore;			
			return 0;
		}else{
			return -EACCES;
                }
	}
      
	
}

static int tagliatelle_release(const char *path, struct fuse_file_info *fi)
{
        fprintf(LOG_FILE,"tagliatelle_release\n");
	incore_inode* reals_incore = (incore_inode*)(fi->fh);
	if (!reals_incore)
	{
		return 0;
	}
	
	iput(reals_incore);
	return 0;
}

static int tagliatelle_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
        fprintf(LOG_FILE,"tagliatelle_read\n");
	incore_inode* incore = (incore_inode*)(fi->fh);
	int counter=0;
	if (!incore)
	{
		return -ENOENT;
	}
	int blockid, inblock_offset, _numblk, _outbyte;
	while (counter < size){
		if (offset <= incore->inode.file_size)	
		{
			bmap(incore, offset, &_numblk, &inblock_offset, &_outbyte, &blockid);

		}
		else
		{
			fprintf(stderr, "cannot read, there is no more data in the file");
			break;
		}
		uint8_t temp[size_blk];		

		
		read_memory(temp, blockid, 1);

		int i;
	
		for (i = inblock_offset; i < size_blk; i++)	//read temp to buf. from inblock_offset to the end
		{
			if (counter>size)
				break;
			buf[counter] = temp[i];
			counter++;

		}
		offset = offset + i - inblock_offset;
	}
	return counter;
}


static int tagliatelle_write(const char *path, const char* buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
        fprintf(LOG_FILE,"tagliatelle_write\n");
	incore_inode* incore = (incore_inode*)(fi->fh);
	int counter = 0;
	if (!incore)
	{
		return -ENOENT;
	}
	int blockid, inblock_offset, _numblk, _outbyte;

	while (counter < size)
	{
		if (incore->inode.file_size + size < size_max_file){		
			bmap(incore, offset, &_numblk, &inblock_offset, &_outbyte, &blockid);
		}
		else
		{
			fprintf(stderr, "cannot write, the file is too big");
			return -EFBIG;
		}
		int i;
		//int readindex=0;
		uint8_t temp[size_blk];
		//if (inblock_offset > 0)              
		//{
			read_memory(temp, blockid, 1);
		//}
		for (i = inblock_offset; i < size_blk; i++)
		{
			if (counter >= size)
			{
				break;
			}
			temp[i] = buf[counter];
			counter++;

		}
		write_memory(temp, blockid, 1);		
		offset = offset + i - inblock_offset;

		incore->inode.file_size = incore->inode.file_size + i - inblock_offset;	//update file size
	}
	return counter;
}

static int tagliatelle_create(const char *path, mode_t mode, struct fuse_file_info *fi)		
{
        fprintf(LOG_FILE,"tagliatelle_create\n");
	incore_inode* creat_incore;
	creat_incore = namei(path);
	char file_name[size_max_name];
	char parent_path[size_max_name*10];
	get_parent_child(path, file_name, parent_path);

      /*  if(cur_num_ent(parent_path) == num_max_ent){
           return -1; //////
        }*/

	if (!creat_incore)
	{
		creat_incore = ialloc();			//allocate a new inode
		creat_incore->inode.type = 1;		//regular file
		creat_incore->inode.perms = mode;
		incore_inode* parent_incore = namei(parent_path);
		if (add_file_to_dir(creat_incore->inumber, file_name, parent_incore))
		{
                        fi->fh=(uint64_t)creat_incore;
		        iput(parent_incore);
   		        //iput(creat_incore);             //////////// you should not iput.
			return 0;
		}
		else
		{
                        iput(parent_incore);
   		        iput(creat_incore);
			fprintf(stderr, "cannot create, there is a file with the same name!");	
			return -EEXIST;
		}

	}
	else
	{
   	        iput(creat_incore);
		fprintf(stderr, "cannot create, there is a file with the same name!\n");
		return -EEXIST;
	}
}


static int tagliatelle_truncate(const char*path, off_t size)		//truncate
{
        
	fprintf(LOG_FILE,"tagliatelle_truncate\n");
	incore_inode* trunc_incore = namei(path);
	
	if(trunc_incore->inode.type==2)		//directory
	{
		return EISDIR;
	}
	
	if(size<(trunc_incore->inode.file_size))		//size<file size
	{
		truncate_inode(trunc_incore, size);			//truncate the file
		trunc_incore->inode.file_size=size;
		iput(trunc_incore);
		return 0;
	}
	if(size>(trunc_incore->inode.file_size))		//size > file size
	{
		if(size>size_max_file)						//cannot bigger than the max size
		{
			iput(trunc_incore);
			return -EFBIG;
		}
		int flag=size-(trunc_incore->inode.file_size);
		int counter=0;
		int blockid, inblock_offset, _numblk, _outbyte;
		
		while (counter<flag)			//enlarge the file 
		{
			int current_size=trunc_incore->inode.file_size;
			bmap(trunc_incore, current_size, &_numblk, &inblock_offset, &_outbyte, &blockid);
			uint8_t temp[size_blk];
			read_memory(temp, blockid, 1);
                        int i;
			for (i = inblock_offset; i < size_blk; i++)
			{
				if (counter >= flag)
				{
					break;
				}
				temp[i] = '\0';
				counter++;
			}
			write_memory(temp, blockid, 1);	
			trunc_incore->inode.file_size=trunc_incore->inode.file_size+i-inblock_offset;
		}
		iput(trunc_incore);
		return 0;
	}
	if(size==(trunc_incore->inode.file_size))			//size = file size
	{
		iput(trunc_incore);
		return 0;
	}
	
}

static int tagliatelle_access(const char *path, int mask)
{
        fprintf(LOG_FILE,"tagliatelle_access\n");
	incore_inode* incore = namei(path);
	
	if (!incore)
		return -ENOENT; //-ENOENT;

	if (mask&R_OK)
	{
		if (incore->inode.perms&__S_IREAD){
                        iput(incore);
			return 0;
		}else{
                        iput(incore);
			return -EACCES;		
                }
	}

	if (mask&W_OK)
	{
		if (incore->inode.perms&__S_IWRITE){
                        iput(incore);
			return 0;
		}else{
                        iput(incore);
			return -EACCES;		
                }
	}

	if (mask&X_OK)
	{
		if (incore->inode.perms&__S_IEXEC){
                        iput(incore);
			return 0;
		}else{
                        iput(incore);
			return -EACCES;		//-1
                }
	}
}

static int tagliatelle_utimens(const char *path, const struct timespec t[2])
{
    fprintf(LOG_FILE,"tagliatelle_utimens!\n");

    incore_inode* inode = namei(path);
    inode->inode.t1 = t[0];
    inode->inode.t2 = t[1];
    iput(inode);

    return 0;
}

static int tagliatelle_chown(const char* path, uint8_t uid, uint8_t gid)	//Change the owner and group of a file
{
        fprintf(LOG_FILE,"tagliatelle_chown\n");
	incore_inode* incore = namei(path);
	if (!incore)
		return -ENOENT;

	incore->inode.owner = uid;
	incore->inode.group = gid;

	iput(incore);
	return 0;
}

static int tagliatelle_chmod(const char* path, mode_t mode)	//change mode
{
        fprintf(LOG_FILE,"tagliatelle_chmod\n");
	incore_inode* incore = namei(path);
	if (!incore)
		return -ENOENT;

	incore->inode.perms = mode;

	iput(incore);
	return 0;
}

static int tagliatelle_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
        fprintf(LOG_FILE,"tagliatelle_readdir\n");
	incore_inode* dir_incore = namei(path);		//get a directory's incore_inode
	if (!dir_incore)
	{
		return -ENOENT;
	}
	if (dir_incore->inode.type == 2)		//directory
	{
		int name_num;
		
		dir_inode dir;		
		read_dir_from_incore(&dir, dir_incore);

		for (name_num = 0; name_num < dir.num_ent; name_num++)
		{
			filler(buf, dir.ent[name_num].filename, NULL, 0);
		}
		iput(dir_incore);
		return 0;
	}
	else
	{
		iput(dir_incore);		//not a directory, return erro
		return -ENOTDIR;
	}
}

static int tagliatelle_unlink(const char *path)
{
        fprintf(LOG_FILE,"tagliatelle_unlink\n");
	//char* child_name;
	//char* parent;
	char child_name[size_max_name];
	char parent_path[size_max_name * 10];
	incore_inode* dir_incore;
	incore_inode* file_incore;

	get_parent_child(path, child_name, parent_path);
	if (strcmp(child_name, ".") == 0 || strcmp(child_name, "..") == 0) //path is a directory
	{
		return -1;
	}
	
	dir_incore = namei(parent_path);
	file_incore = namei(path);
	
	if (!dir_incore)
	{
		return -ENOENT;
	}
	if (dir_incore->inode.type != 2)	//parent is not a directory
	{
                iput(dir_incore);
                iput(file_incore);
		return -ENOTDIR;
	}
	if (!file_incore)
	{
                iput(dir_incore);
		return -1;
	}
	if (file_incore->inode.type != 1)	//target is not a file
	{
                iput(dir_incore);
                iput(file_incore);
		return -ENOENT;
	}
	unlink_file(dir_incore, child_name);	//delete the file in the directory	
	file_incore->inode.num_links--;				//link--
	iput(file_incore);
	iput(dir_incore);
	return 0;
}

static int tagliatelle_mkdir(const char *path,mode_t mode)		//make directory
{
        fprintf(LOG_FILE,"tagliatelle_mkdir\n");
	incore_inode* dir_incore = namei(path);
	char dir_name[size_max_name];
	char parent_path[size_max_name * 10];
	get_parent_child(path, dir_name, parent_path);
    /*    if(cur_num_ent(parent_path) == num_max_ent){
           return -1; //////
        }*/
	if (!dir_incore)
	{
		dir_incore = ialloc();
		dir_incore->inode.type = 2;		//directory
		dir_incore->inode.perms = mode;
		dir_incore->inode.table[0] = balloc();
		incore_inode* parent_incore = namei(parent_path);
		add_file_to_dir(dir_incore->inumber, ".", dir_incore);
		add_file_to_dir(parent_incore->inumber, "..", dir_incore);
		add_file_to_dir(dir_incore->inumber, dir_name, parent_incore);
		iput(parent_incore);
		iput(dir_incore);
		return 0;
	}
	else
	{
		fprintf(stderr, "cannot create, there is a directory with the same name!\n");
		return -1;
	}
}

static int tagliatelle_rmdir(const char *path)		//remove directory
{
        fprintf(LOG_FILE,"tagliatelle_rmdir\n");
	char dir_name[size_max_name];
	char parent_path[size_max_name * 10];
	incore_inode* dir_incore;
	incore_inode* parent_incore;
	//if (!get_parent(path, dir_name, parent_path))	
        get_parent_child(path, dir_name, parent_path);	//get the parent path and child name
	//{
	//	return -1;
	//}
	if (strcmp(dir_name, ".") == 0)
		return -1;  

	if (strcmp(dir_name, "..") == 0)
		return -1;  
	dir_incore = namei(path);
	parent_incore = namei(parent_path);

	if (!parent_incore)
	{
		return -ENOENT;
	}
	if (parent_incore->inode.type!=2)
	{
                iput(dir_incore);
                iput(parent_incore);
		return -ENOTDIR;
	}
	if (!dir_incore)
	{
                iput(parent_incore);
		return -ENOENT;
	}
	if (dir_incore->inode.type != 2)	//not a directory
	{
                iput(dir_incore);
                iput(parent_incore);
		return -ENOTDIR;
	}


	if (dir_empty(dir_incore))		//if empty, return 1,else return 0
	{
		unlink_file(parent_incore, dir_name);
		dir_incore->inode.num_links=0;				//link--
		iput(parent_incore);
		iput(dir_incore);
		return 0;
	}
	else
	{
		fprintf(stderr, "This directory is not empty!\n");		//if the directory is not empty, return ENOTEMPTY 
		iput(parent_incore);
		iput(dir_incore);
		return -ENOTEMPTY;
	}
}
static void tagliatelle_destroy(void *userdata)
{
        fprintf(LOG_FILE,"tagliatelle_destroy\n");
        write_superblk();
        print_inodes();
	free_memory();
}

static int tagliatelle_rename(const char *pathFrom, const char *pathTo)		//rename or move 
{
	incore_inode* incoreFromC = namei(pathFrom);
	if (!incoreFromC){
		return -ENOENT;
	}
	incore_inode* incoreToC = namei(pathTo);
	char childFrom[size_max_name];
	char parentFrom[size_max_name * 10];
	char childTo[size_max_name];
	char parentTo[size_max_name * 10];
	get_parent_child(pathFrom, childFrom, parentFrom);
	get_parent_child(pathTo, childTo, parentTo);

	if (incoreFromC->inode.type==1)		//is a file 
	{
		if (!incoreToC)		//target not exist
		{
			incore_inode* incoreToP = namei(parentTo);
			if (!incoreToP)
			{
				return -ENOENT;
			}
			incore_inode* incoreFromP = namei(parentFrom);

			if (incoreFromP == incoreToP)		//in the same directory
			{
				change_name(incoreFromP, childFrom, childTo);		//////
				iput(incoreFromC);
				iput(incoreFromP);
				iput(incoreToP);
				//iput(incoreToC);
				return 0;
			}
			else				//not in the same directory
			{
				
				move_file(incoreToP, incoreFromC, childTo);			/////
				tagliatelle_unlink(pathFrom);
				iput(incoreFromC);
				iput(incoreFromP);
				iput(incoreToP);
				//iput(incoreToC);
				return 0;
			}
		}
		else		//target exists
		{
			incore_inode* incoreToP = namei(parentTo);
			incore_inode* incoreFromP = namei(parentFrom);
			if (incoreToC->inode.type == 1)		//is a file 
			{
				if (incoreFromP == incoreToP)
				{
					tagliatelle_unlink(pathTo);
					change_name(incoreFromP, childFrom, childTo);
					iput(incoreFromC);
					iput(incoreFromP);
					iput(incoreToP);
					iput(incoreToC);
					return 0;
				}
				else
				{
					tagliatelle_unlink(pathTo);
					move_file(incoreToP, incoreFromC, childTo);
					tagliatelle_unlink(pathFrom);
					iput(incoreFromC);
					iput(incoreFromP);
					iput(incoreToP);
					iput(incoreToC);
					return 0;
				}
			}
			else                //type==2 is a directory
			{
	//same file ->1  ;  same directory->2   ;   not same->0
				if (same_name(incoreToC, childFrom) == 1)	//same file
				{
					char file_name[size_max_name * 10]; 
					strcpy(file_name,pathTo);
					strcat(file_name,'/');
					strcat(file_name,childFrom);
					tagliatelle_unlink(file_name);
					move_file(incoreToC, incoreFromC, childFrom);
					tagliatelle_unlink(pathFrom);
					iput(incoreFromC);
					iput(incoreFromP);
					iput(incoreToP);
					iput(incoreToC);
					return 0;
				} 
				if (same_name(incoreToC, childFrom) == 2)	//same directory
				{
					iput(incoreFromC);
					iput(incoreFromP);
					iput(incoreToP);
					iput(incoreToC);
				        return EISDIR;
				}
				if (same_name(incoreToC, childFrom) == 0)	//not same
				{
					move_file(incoreToC, incoreFromC, childFrom);
					tagliatelle_unlink(pathFrom);
					iput(incoreFromC);
					iput(incoreFromP);
					iput(incoreToP);
					iput(incoreToC);
					return 0;
				}
			}
		}
	}

	if (incoreFromC->inode.type == 2)	//is a directory
	{
                incore_inode* incoreToP = namei(parentTo);
                incore_inode* incoreFromP = namei(parentFrom);
		if (!incoreToC)
		{
			
			if (!incoreToP)
			{
				iput(incoreFromC);
				iput(incoreFromP);
				return -ENOENT;
			}
			//incore_inode* incoreFromP = namei(parentFrom);
			if (incoreFromP == incoreToP)
			{
				change_name(incoreFromP, childFrom, childTo);		//////
				iput(incoreFromC);
				iput(incoreFromP);
				iput(incoreToP);
				//iput(incoreToC);
				return 0;
			}
			else
			{                                
				move_file(incoreToP, incoreFromC, childTo);                            
				unlink_file(incoreFromP, childFrom);//tagliatelle_unlink(pathFrom);
				iput(incoreFromC);
				iput(incoreFromP);
				iput(incoreToP);
				//iput(incoreToC);
				return 0;
			}
		}
		else
		{
                      //  incore_inode* incoreFromP = namei(parentFrom);
	/*		if (incoreToP->inode.type == 1)
			{
				iput(incoreFromC);
				iput(incoreFromP);
				iput(incoreToP);
				iput(incoreToC);
				return ENOTDIR;			//oldpath is a directory, and newpath exists but is not a directory.
			}*/
			if (incoreToP->inode.type == 2)
			{
				if (same_name(incoreToP, childFrom)==2)				//same file ->1  ;  same directory->2   ;   not same->0
				{
					char directory[size_max_name * 10];    
					//strcpy(directory,pathTo);
					//strcat(directory,'/');				
					//strcat(directory,childFrom);
					incore_inode* dir_incore = namei(pathTo);
					if (dir_empty(dir_incore))	////if empty, return 1,else return 0
					{
                                                iput(dir_incore);
						tagliatelle_rmdir(pathTo);
						move_file(incoreToP, incoreFromC, childFrom);
						unlink_file(incoreFromP, childFrom);//tagliatelle_unlink(pathFrom);
						iput(incoreFromC);
						iput(incoreFromP);
						iput(incoreToP);
						iput(incoreToC);
						return 0;
					}
                                        else{
                                                iput(dir_incore);
						iput(incoreFromC);
						iput(incoreFromP);
						iput(incoreToP);
						iput(incoreToC);
                                                return 0; /////
                                        }
				}
				if (same_name(incoreToP, childFrom) == 1)
				{
					iput(incoreFromC);
					iput(incoreFromP);
					iput(incoreToP);
					iput(incoreToC);
					return ENOTDIR;
				}
				if (same_name(incoreToP, childFrom) == 0)
				{
					move_file(incoreToP, incoreFromC, childFrom);
					unlink_file(incoreFromP, childFrom);//tagliatelle_unlink(pathFrom);
					iput(incoreFromC);
					iput(incoreFromP);
					iput(incoreToP);
					iput(incoreToC);
					return 0;
				}
			}
		}
	}
}

static struct fuse_operations tagliatelle_oper = {
	.init = tagliatelle_init,
	.getattr = tagliatelle_getattr,
	.open = tagliatelle_open,
	.release = tagliatelle_release,
	.read = tagliatelle_read,
	.write = tagliatelle_write,
	.create = tagliatelle_create,
	.truncate=tagliatelle_truncate,
	.access=tagliatelle_access,
        .utimens=tagliatelle_utimens,
	.chown=tagliatelle_chown,
	.chmod=tagliatelle_chmod,
	.readdir = tagliatelle_readdir,
	.unlink=tagliatelle_unlink,
	.mkdir=tagliatelle_mkdir,
	.rmdir=tagliatelle_rmdir,
	.destroy = tagliatelle_destroy,
        .rename=tagliatelle_rename,
};

void mkfs1(char* path_disk, int flag)
{
        
	init_disk(path_disk, flag);
	init_superblk();
	print_superblk();
	init_ifree();    
	init_rootdir(flag);
}

void mkfs2()
{

	init_memory();
	init_superblk();
	print_superblk();
	init_ifree();
	init_rootdir();
}

int main(int argc, char* argv[])
{
	if (argc < 2){
	   printf("Usage: ./fs path_mount path_disk disk_formatting\n");
	   return -1;
	}
	FILE* log = fopen("fs.log", "w");
        setvbuf(log, NULL, _IOLBF, 0);

	
//        mkfs1("/dev/vdb", 1);
        mkfs1(argv[2], atoi(argv[3]));
        argv[3]=NULL;
        argc--;
        argv[2]=NULL;
        argc--;
	int ret = fuse_main(argc, argv, &tagliatelle_oper, log);
        return ret;
}

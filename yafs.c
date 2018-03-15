
#define FUSE_USE_VERSION 31

#include "fuse.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#define N_DIR 10
#define N_FILE 100
#define FILE_SIZE 4096
#define NO_OF_DIRS 100
#define NO_OF_FILES	100
#define BLOCK_SIZE 4096
#define INODE_SIZE 104
#define N_BLOCKS (4*1024)
#define DEFAULT 0
#define REG 1
#define DIR 2
#define LNK 3
/*
 * Command line files
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct files {
	char filename[100];
	char contents[4096];
};
static int showhelp;
static struct files files[N_FILE];
static struct dirs {
	char dirname[100];
};
static struct dirs dirs[N_DIR];
static int n_files=0;
static int n_dirs=0;
FILE* fpfiles;
FILE* fpdirs;
//-------------------------------------------------------------------------------------------------------
static void *hello_init(struct fuse_conn_info *conn,struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;
	return NULL;
}
static int hello_getattr(const char *path, struct stat *stbuf,struct fuse_file_info *fi)
{
	(void) fi;
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if ((path[strlen(path)-1]== '/') ) {
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		return res;
	} 
	else{
		int i=0;
		for(;i<n_files;i++){
			 if (strcmp(path+1, files[i].filename) == 0) {
				stbuf->st_mode = S_IFREG | 0777;
				stbuf->st_nlink = 1;
				stbuf->st_size = strlen(files[i].contents);
				return res;			
			}
		}
		i=0;
		for(;i<n_dirs;i++){
			if (strcmp((path+1), dirs[i].dirname) == 0) {
				stbuf->st_mode = S_IFDIR | 0777;
				stbuf->st_nlink = 2;
				return res;			
			}
		}
	}
	res = -ENOENT;
	return res;
}
static int checkpath(char* name,char* path,int dirflag){
	if(strcmp(path,"/")!=0)	path+=1;
	int nlen = strlen(name);
	int i=nlen-1;
	for(;i>0;i--){
		if(name[i]=='/'){
			i--;
			break;
		}
	}
	printf("i=%d,%s\n",i,name);	
	int plen = strlen(path);
	int j=plen-1;
	printf("j=%d, %s\n",j,path);
	if(i==j)	
		return 1;
	return 0;
}
static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi,enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;
	int dirflag=0;
	if ((path[strlen(path)-1]!= '/') )
		dirflag=1;
	
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	int i=0;
	for(;i<n_files;i++){
		if(checkpath(files[i].filename,path,dirflag))	
			filler(buf, (files[i].filename), NULL, 0, 0);
	}
	i=0;
	for(;i<n_dirs;i++){
		if(checkpath(dirs[i].dirname,path,dirflag))	
			filler(buf, (dirs[i].dirname), NULL, 0, 0);
	}
	return 0;
}
static int hello_open(const char *path, struct fuse_file_info *fi)
{
	int i=0;
	for(;i<n_files-1;i++){
		if(strcmp(path+1, files[i].filename) == 0){
			break;
		}
		else if(i==n_files-1){
			printf("Creating new file\n");
			n_files+=1;
			memcpy(files[n_files-1].filename,path+1,strlen(path+1));
			memset(files[n_files-1].contents,'\0',4096);
			break;
		}
		if(files[i].filename[0]==0){
			printf("Hi\n");
			n_files+=1;
			memcpy(files[i].filename,path+1,strlen(path+1));
			memset(files[i].contents,'\0',4096);
			break;
		}
	}
	size_t len = strlen(files[i].contents);
	return 0;
}
static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	int i=0;
	for(;i<n_files;i++){
		if (strcmp(path+1, files[i].filename) == 0){
			break;
		}
		else if(i==n_files-1){
			return -ENOENT;
		}
	}
	len = strlen(files[i].contents);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, files[i].contents + offset, size);
	} else
		size = 0;
	return size;
}
static int hello_write(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	int i=0;
	for(;i<n_files;i++){
		if (strcmp(path+1, files[i].filename) == 0){
			break;
		}
		else if(i==n_files-1){
			return -ENOENT;
		}
	}
	memset(files[i].contents,'\0',4096);
	memcpy(files[i].contents + offset,buf, size);
	return size;
}
static int hello_truncate(const char *path,off_t offset,struct fuse_file_info *fi){
	return 0;
}
static int hello_mkdir(const char *path, mode_t mode){
	n_dirs+=1;
	memcpy(dirs[n_dirs-1].dirname,path+1,strlen(path+1));
	return 0;
}
static int hello_rmdir(const char *path, mode_t mode){
	int i=0;
	for(; i< n_dirs; i++){	// Loop through the table to find that directory.
		if(strcmp(dirs[i].dirname,path+1)==0) 
			break; 		// Once you reach the node, break out of the loop and  "delete" that entry
	}
	int j=i;
	for(;j<n_dirs;j++){
		dirs[i]=dirs[i+1];
	}
	n_dirs--;
	return 0;
}
static int hello_mknod (const char * path, mode_t mode, dev_t dev){
	int nlen = strlen(path);
	int i=nlen-1;
	for(;i>0;i--){
		if(path[i]=='/'){
			i--;
			break;
		}
	}
	if(path[i+1]=='.')	return 0;			// Ignore object files
	n_files+=1;
	memcpy(files[n_files-1].filename,path+1,strlen(path+1));
	memset(files[n_files-1].contents,'\0',4096);
	return 0;
}
static int hello_unlink(const char * path){
	int i=0,delflag=0;
	for(;i<n_files;i++){
		if (strcmp(path+1, files[i].filename) == 0){
			delflag=1;
		}
		else if(i==n_files-1 && delflag==0){
			return -ENOENT;
		}
		if(delflag && i<n_files-1){
			files[i]=files[i+1];
		}
	}
	return 0;
}
static int hello_rename(const char* path, const char* new,unsigned int flags){
	int i=0;
	for(;i<n_files;i++){
		if (strcmp(path+1, files[i].filename) == 0){
			break;
		}
		else if(i==n_files-1){
			return -ENOENT;
		}
	}
	memcpy(files[i].filename,new+1,strlen(new+1));
	return 0;	
}
static int hello_destroy(void* private_data){
	fseek(fpfiles,0,SEEK_SET);
	int ret=fwrite(&n_files,sizeof(int),1,fpfiles);
	ret=fwrite(&files,N_FILE*(4196),sizeof(char),fpfiles);
	fseek(fpdirs,0,SEEK_SET);
	ret=fwrite(&n_dirs,sizeof(int),1,fpdirs);
	ret=fwrite(&dirs,N_DIR*100,sizeof(char),fpdirs);
	fclose(fpfiles);
	fclose(fpdirs);
	return 0;
}
static struct fuse_operations hello_oper = {
	.init       = hello_init,
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
	.write 		= hello_write,
	.truncate 	= hello_truncate,
	.mkdir		= hello_mkdir,
	.rmdir		= hello_rmdir,
	.unlink		= hello_unlink,
	.rename		= hello_rename,
	.mknod 		= hello_mknod,
	.destroy	= hello_destroy,
};

static void show_help(const char *progname)
{
	printf("usage: %s [files] <mountpoint>\n\n", progname);
	printf("File-system specific files:\n"
	       "    --name=<s>          Name of the \"hello\" file\n"
	       "                        (default: \"hello\")\n"
	       "    --contents=<s>      Contents \"hello\" file\n"
	       "                        (default \"Hello, World!\\n\")\n"
	       "\n");
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	fpfiles = fopen("files.dat","r+");
	fread(&n_files,sizeof(int),1,fpfiles);
	if(n_files){
		fread(&files,N_FILE*(4196),1,fpfiles);
	}
	else{
		char firstfile[] = "hello.txt";
		memcpy(files[0].filename,firstfile,strlen(firstfile));
		memset(files[0].contents,'\0',4096);
		n_files=1;
	}
	fpdirs = fopen("dirs.dat","r+");
	fread(&n_dirs,sizeof(int),1,fpdirs);
	if(n_dirs){
		fread(&dirs,N_DIR*100,1,fpdirs);
	}
	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	/* Parse files */
	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the files again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (showhelp) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0] = (char*) "";
	}
	return fuse_main(args.argc, args.argv, &hello_oper, NULL);
}

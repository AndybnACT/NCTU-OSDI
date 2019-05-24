/* This file use for NCTU OSDI course */
/* It's contants file operator's wapper API */
#include <fs.h>
#include <fat/ff.h>
#include <inc/string.h>
#include <inc/stdio.h>
#include <inc/assert.h>

/* Static file objects */
FIL file_objs[FS_FD_MAX];
DIR dir_objs[FS_FD_MAX];

/* Static file system object */
FATFS fat;

/* It file object table */
struct fs_fd fd_table[FS_FD_MAX];

/* File system operator, define in fs_ops.c */
extern struct fs_ops elmfat_ops; //We use only one file system...
extern struct file_ops elmfat_file_ops;

/* File system object, it record the operator and file system object(FATFS) */
struct fs_dev fat_fs = {
    .dev_id = 1, //In this lab we only use second IDE disk
    .path = {0}, // Not yet mount to any path
    .ops = &elmfat_ops,
    .fops = &elmfat_file_ops,
    .data = &fat
};
    
/*TODO: Lab7, VFS level file API.
 *  This is a virtualize layer. Please use the function pointer
 *  under struct fs_ops to call next level functions.
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   sys_open   │  file I/O system call interface
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║  file_open   ║  VFS level file API
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │   fat_open   │  fat level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    f_open    │  FAT File System Module
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    diskio    │  low level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │     disk     │  simple ATA disk dirver
 *        └──────────────┘
 */
int fs_init()
{
    int res, i;
    
    /* Initial fd_tables */
    for (i = 0; i < FS_FD_MAX; i++)
    {
        fd_table[i].flags = 0;
        fd_table[i].size = 0;
        fd_table[i].pos = 0;
        fd_table[i].type = 0;
        fd_table[i].ref_count = 0;
        fd_table[i].fdata = &file_objs[i];
        fd_table[i].ddata = &dir_objs[i];
        fd_table[i].fs = &fat_fs;
    }
    
    /* Mount fat file system at "/" */
    /* Check need mkfs or not */
    if ((res = fs_mount("elmfat", "/", NULL)) != 0)
    {
        res = fat_fs.ops->mkfs("elmfat");
        printk("mkfs: %d\n", res);
        res = fs_mount("elmfat", "/", NULL);
        printk("mount: %d\n", res);
        return res;
    }
    return -STATUS_EIO;
       
}

/** Mount a file system by path 
*  Note: You need compare the device_name with fat_fs.ops->dev_name and find the file system operator
*        then call ops->mount().
*
*  @param data: If you have mutilple file system it can be use for pass the file system object pointer save in fat_fs->data. 
*/
int fs_mount(const char* device_name, const char* path, const void* data)
{
    if (strcmp(device_name, fat_fs.ops->dev_name) == 0) {
        memcpy(fat_fs.path, path, 32);
        return fat_fs.ops->mount(&fat_fs, data);
    }
    panic("Unknown fs type!");
    return -STATUS_EIO;
} 

/* Note: Before call ops->open() you may copy the path and flags parameters into fd object structure */
int file_open(struct fs_fd* fd, const char *path, int flags)
{

    int pathlen = strlen(path);
    if (pathlen > 64) {
        panic("path length too long!");
    }
    memcpy(fd->path, path, pathlen);
    fd->flags = flags;
    return fd->fs->ops->open(fd);
    
}

int file_read(struct fs_fd* fd, void *buf, size_t len)
{

    int readlen;
    readlen = fd->fs->ops->read(fd, buf, len);
    if (readlen < 0) {
        panic("file_read");
    }
    fd->pos += readlen;
    return readlen;
}

int file_write(struct fs_fd* fd, const void *buf, size_t len)
{
    int writelen;
    writelen = fd->fs->ops->write(fd, buf, len);
    if (writelen < 0) {
        panic("file_write");
    }
    fd->pos += writelen;
    return writelen;

}

int file_close(struct fs_fd* fd)
{
    int stat;
    stat = fd->fs->ops->close(fd);
    memset(fd->path, 0, 64);
    fd->pos = 0;
    fd->flags = 0;
    fd->type = 0;
    memset(fd->fdata, 0, sizeof(FIL));
    fd_put(fd);
    return stat;

}
int file_lseek(struct fs_fd* fd, off_t offset)
{
    int new_off = fd->fs->ops->lseek(fd, offset);
    fd->pos = new_off;
    return new_off;

}
int file_unlink(const char *path)
{
    // temporary solution (since we have 1 fs only)
    return elmfat_ops.unlink(NULL, path);
}


int file_stat(const char *path, struct stat *buf){
    return elmfat_file_ops.stat(path, buf);
}

int file_opendir(struct fs_fd* fd, const char *path){
    int pathlen = strlen(path);
    if (pathlen > 64) {
        panic("path length too long");
    }
    memcpy(fd->path, path, pathlen);
    return fd->fs->fops->opendir(fd);
}

int file_closedir(struct fs_fd* fd){
    int stat;
    stat = fd->fs->fops->closedir(fd);
    memset(fd->path, 0, 64);
    fd->pos = 0;
    fd->flags = 0;
    fd->type = 0;
    memset(fd->fdata, 0, sizeof(DIR));
    fd_put(fd);
    return stat;
}

int file_readdir(struct fs_fd* fd, struct stat *buf){
    return fd->fs->fops->readdir(fd, buf);
}

int file_mkdir(const char* path){
    return elmfat_file_ops.mkdir(NULL, path);
}
/**
 * @ingroup Fd
 * This function will allocate a file descriptor.
 *
 * @return -1 on failed or the allocated file descriptor.
 */
int fd_new(void)
{
	struct fs_fd* d;
	int idx;

	/* find an empty fd entry */
	for (idx = 0; idx < FS_FD_MAX && fd_table[idx].ref_count > 0; idx++);


	/* can't find an empty fd entry */
	if (idx == FS_FD_MAX)
	{
		idx = -1;
		goto __result;
	}

	d = &(fd_table[idx]);
	d->ref_count = 1;

__result:
	return idx;
}

/**
 * @ingroup Fd
 *
 * This function will return a file descriptor structure according to file
 * descriptor.
 *
 * @return NULL on on this file descriptor or the file descriptor structure
 * pointer.
 */
struct fs_fd* fd_get(int fd)
{
	struct fs_fd* d;

	if ( fd < 0 || fd > FS_FD_MAX ) return NULL;

	d = &fd_table[fd];

	/* increase the reference count */
	d->ref_count ++;

	return d;
}

/**
 * @ingroup Fd
 *
 * This function will put the file descriptor.
 */
void fd_put(struct fs_fd* fd)
{

	fd->ref_count --;

	/* clear this fd entry */
	if ( fd->ref_count == 0 )
	{
		//memset(fd, 0, sizeof(struct fs_fd));
		//memset(fd->data, 0, sizeof(FIL));
	}
};

off_t fd_get_pos(struct fs_fd* fd){
    return fd->pos;
}
off_t fd_get_size(struct fs_fd* fd){
    return fd->size;
}


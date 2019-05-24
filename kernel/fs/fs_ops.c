/* This file use for NCTU OSDI course */
/* It's contants fat file system operators */

#include <inc/stdio.h>
#include <fs.h>
#include <fat/ff.h>
#include <diskio.h>

extern struct fs_dev fat_fs;

/*TODO: Lab7, fat level file operator.
 *       Implement below functions to support basic file system operators by using the elmfat's API(f_xxx).
 *       Reference: http://elm-chan.org/fsw/ff/00index_e.html (or under doc directory (doc/00index_e.html))
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
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   fat_open   ║  fat level file operator
 *        ╚══════════════╝
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

/* Note: 1. Get FATFS object from fs->data
*        2. Check fs->path parameter then call f_mount.
*/
int fat_mount(struct fs_dev *fs, const void* data)
{

    FATFS *fat = (FATFS*) fs->data;
    return f_mount(fat, fs->path, 1);
}

/* Note: Just call f_mkfs at root path '/' */
int fat_mkfs(const char* device_name)
{

    return f_mkfs("/", 0, 64);
}

#define RET_ON_ERR(errno, func){              \
    if (errno != FR_OK) {                     \
        return fat_err_handler(errno, #func); \
    }                                         \
}

int fat_err_handler(FRESULT err, char *func){
    switch (err) {
        case FR_OK:
            return STATUS_OK;
        case FR_NO_FILE:
            return -STATUS_ENOENT;
        case FR_EXIST:
            return -STATUS_EEXIST;
        case FR_INVALID_PARAMETER:
        case FR_INVALID_NAME:
            return -STATUS_EINVAL;
        default:
            printk("fat unexcepted err=%d @ %s\n", err, func);
            return -STATUS_EIO;
    }
}


/* Note: Convert the POSIX's open flag to elmfat's flag.
*        Example: if file->flags == O_RDONLY then open_mode = FA_READ
*                 if file->flags & O_APPEND then f_seek the file to end after f_open
*/
int fat_open(struct fs_fd* file)
{
    FIL *fp = (FIL*) file->fdata;
    BYTE mode;
    FRESULT res;
    if (file->flags == O_RDONLY) {
        mode = FA_READ;
    }else if (file->flags & O_WRONLY) {
        mode = FA_WRITE;
    }else if (file->flags & O_RDWR) {
        mode = FA_READ | FA_WRITE;
    }
    if (file->flags & O_CREAT) {
        if (file->flags & O_TRUNC) {
            mode |= FA_CREATE_ALWAYS;
        }else{
            mode |= FA_CREATE_NEW;
        }
    }else{
        mode |= FA_OPEN_EXISTING;
    }
    // printk("mode = %x\n", mode);
    res = f_open(fp, file->path, mode);
    RET_ON_ERR(res,fat_open);
    
    file->size = f_size(fp);
    if (file->flags & O_APPEND) {
        file->pos = file->size;
        f_lseek(fp, file->size);
    }else{
        file->pos = 0;
    }
    return 0;
}

int fat_close(struct fs_fd* file)
{
    FIL *fp = (FIL*) file->fdata;
    FRESULT res;
    res = f_close(fp);
    file->size = 0;
    RET_ON_ERR(res, fat_close);
    return 0;

}
int fat_read(struct fs_fd* file, void* buf, size_t count)
{
    FIL *fp = (FIL*) file->fdata;
    FRESULT res;
    UINT readcnt;
    res = f_read(fp, buf, count, &readcnt);
    RET_ON_ERR(res, fat_read);
    return readcnt;

}
int fat_write(struct fs_fd* file, const void* buf, size_t count)
{
    FIL *fp = (FIL*) file->fdata;
    FRESULT res;
    UINT wrotecnt;
    res = f_write(fp, buf, count, &wrotecnt);
    RET_ON_ERR(res, fat_write);
    return wrotecnt;
}
int fat_lseek(struct fs_fd* file, off_t offset)
{
    FIL *fp = (FIL*) file->fdata;
    FRESULT res;
    res = f_lseek(fp, offset);
    RET_ON_ERR(res, fat_lseek);
    return f_tell(fp);
}
int fat_unlink(struct fs_fd* file, const char *pathname)
{
    FRESULT res;
    res = f_unlink(pathname);
    RET_ON_ERR(res, fat_unlink);
    return STATUS_OK;
}


// hard-coded length, does not support LFN
#define FINFO_TO_STAT(finfo, stat){                                 \
    memcpy((stat)->name, (finfo)->fname, 13);                       \
    (stat)->size = (finfo)->fsize;                                  \
    (stat)->date = (finfo)->fdate;                                  \
    (stat)->attr = ((finfo)->fattrib & AM_DIR)? S_IFDIR:S_IFREG;    \
}

int fat_stat(struct fs_fd* file, const char* path, struct stat *buf){
    FILINFO finfo;
    FRESULT res;
    res = f_stat(path, &finfo);
    RET_ON_ERR(res, fat_stat);
    
    FINFO_TO_STAT(&finfo, buf);
    
    return STATUS_OK;
}

int fat_opendir(struct fs_fd* file){
    DIR *dp = (DIR*) file->ddata;
    FRESULT res = f_opendir(dp, file->path);
    RET_ON_ERR(res, fat_opendir);
    return STATUS_OK;
}

int fat_closedir(struct fs_fd* file){
    DIR *dp = (DIR*) file->ddata;
    FRESULT res = f_closedir(dp);
    RET_ON_ERR(res, fat_closedir);
}

int fat_readdir(struct fs_fd* file, struct stat *buf){
    DIR *dp = (DIR*) file->ddata;
    FILINFO finfo = {0, };

    FRESULT res = f_readdir(dp, &finfo);
    RET_ON_ERR(res, fat_readdir);
    
    FINFO_TO_STAT(&finfo, buf);
        
    return STATUS_OK;
}

int fat_mkdir(struct fs_fd* file, const char* pathname){
    FRESULT res = f_mkdir(pathname);
    RET_ON_ERR(res, fat_mkdir);
    return 0;
}

struct fs_ops elmfat_ops = {
    .dev_name = "elmfat",
    .mount = fat_mount,
    .mkfs = fat_mkfs,
    .open = fat_open,
    .close = fat_close,
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .unlink = fat_unlink
};

struct file_ops elmfat_file_ops = {
    .opendir = fat_opendir,
    .closedir = fat_closedir,
    .readdir = fat_readdir,
    .mkdir = fat_mkdir,
    .stat = fat_stat
};



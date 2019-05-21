/* This file use for NCTU OSDI course */


// It's handel the file system APIs 
#include <inc/stdio.h>
#include <inc/syscall.h>
#include <inc/assert.h>
#include <fs.h>

/*TODO: Lab7, file I/O system call interface.*/
/*Note: Here you need handle the file system call from user.
 *       1. When user open a new file, you can use the fd_new() to alloc a file object(struct fs_fd)
 *       2. When user R/W or seek the file, use the fd_get() to get file object.
 *       3. After get file object call file_* functions into VFS level
 *       4. Update the file objet's position or size when user R/W or seek the file.(You can find the useful marco in ff.h)
 *       5. Remember to use fd_put() to put file object back after user R/W, seek or close the file.
 *       6. Handle the error code, for example, if user call open() but no fd slot can be use, sys_open should return -STATUS_ENOSPC.
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   sys_open   ║  file I/O system call interface
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
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

// Below is POSIX like I/O system call 
int sys_open(const char *file, int flags, int mode)
{
    //We dont care the mode.
/* TODO */
    int fd = fd_new();
    int err;
    
    if (fd == -1)
        return  fd; // Spec requires us to return -1 though  -STATUS_ENOSPC;
        
    struct fs_fd* d = fd_get(fd);
    err = file_open(d, file, flags);
    if (err) {
        // printk("error opening file, %d\n", err);
        fd_put(d);
        fd = err;
    }
    fd_put(d);
    return fd;
}

int sys_close(int fd)
{
/* TODO */
    struct fs_fd* d = fd_get(fd);
    int ret;
    if (!d) {
        return -STATUS_EINVAL;
    }
    ret = file_close(d);
    fd_put(d);
    return ret;
}
int sys_read(int fd, void *buf, size_t len)
{
/* TODO */
    struct fs_fd* d = fd_get(fd);
    int ret;
    if (!d) {
        return -STATUS_EBADF;
    }
    if (!buf || len < 0) {
        return -STATUS_EINVAL;
    }
    ret = file_read(d, buf, len);
    fd_put(d);
    return ret;
}
int sys_write(int fd, const void *buf, size_t len)
{
/* TODO */
    struct fs_fd *d = fd_get(fd);
    int ret;
    if (!d) {
        return -STATUS_EBADF;
    }
    if (!buf || len < 0) {
        return -STATUS_EINVAL;
    }
    ret = file_write(d, buf, len);
    fd_put(d);
    return ret;
    
}

/* Note: Check the whence parameter and calcuate the new offset value before do file_seek() */
off_t sys_lseek(int fd, off_t offset, int whence)
{
/* TODO */
    struct fs_fd *d = fd_get(fd);
    if (!d) {
        return -STATUS_EBADF;
    }
    off_t abs_offset, ret;
    size_t size = fd_get_size(d);
    switch (whence) {
        case SEEK_SET:
            abs_offset = offset;
            break;
        case SEEK_CUR:
            abs_offset = offset + fd_get_pos(d);
            break;
        case SEEK_END:
            abs_offset = offset + size;
            break;
        default:
            return -STATUS_EINVAL;
    }
    // if (abs_offset > size) {
    //     panic("sys_lseek");
    //     return -STATUS_EIO;
    // }
    ret = file_lseek(d, abs_offset);
    fd_put(d);
    return ret;
}

int sys_unlink(const char *pathname)
{
/* TODO */ 
    return file_unlink(pathname);
}

int sys_opendir(const char *path)
{
    int fd = fd_new();
    int err;
    if (fd == -1)
        return fd;
    struct fs_fd* d = fd_get(fd);
    err = file_opendir(d, path);
    if (err) {
        fd_put(d);
        fd = err;
    }
    fd_put(d);
    return fd;
}

int sys_readdir(int fd, struct stat *buf)
{
    struct fs_fd *d = fd_get(fd);
    int ret;
    if (!d) {
        return -STATUS_EBADF;
    }
    if (!buf) {
        return -STATUS_EINVAL;
    }
    ret = file_readdir(d, buf);
    fd_put(d);
    return ret;
}

int sys_closedir(int fd)
{
    struct fs_fd *d = fd_get(fd);
    int ret;
    if (!d) {
        return -STATUS_EINVAL;
    }
    ret = file_closedir(d);
    fd_put(d);
    return ret;
}

int sys_mkdir(const char* path)
{
    return file_mkdir(path);
}

              


/* This file use for NCTU OSDI course */
#include <fs.h>
#include <fat/diskio.h>
#include <fat/ff.h>
#include <kernel/drv/disk.h>
#include <inc/assert.h>
#include <kernel/timer.h>

#define SECTOR_SIZE 512

/*TODO: Lab7, low level file operator.
 *  You have to provide some device control interface for 
 *  FAT File System Module to communicate with the disk.
 *
 *  Use the function under kernel/drv/disk.c to finish
 *  this part. You can also add some functions there if you
 *  need.
 *
 *  FAT File System Module reference document is under the
 *  doc directory (doc/00index_e.html)
 *
 *  Note:
 *  Since we only use primary slave disk as our file system,
 *  please ignore the pdrv parameter in below function and
 *  just manipulate the hdb disk.
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
 *        ┌──────────────┐
 *        │   fat_open   │  fat level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    f_open    │  FAT File System Module
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║    diskio    ║  low level file operator
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │     disk     │  simple ATA disk dirver
 *        └──────────────┘
 */

#define DISK_ID 1

/**
  * @brief  Initial IDE disk
  * @param  pdrv: Physical drive number
  * @retval disk error status
  *         - 0: Initial success
  *         - STA_NOINIT: Intial failed
  *         - STA_PROTECT: Medium is write protected
  */
DSTATUS disk_initialize (BYTE pdrv)
{
  /* TODO */
  /* Note: You can create a function under disk.c  
   *       to help you get the disk status.
   */
    int err;
    unsigned char buf[SECTOR_SIZE];
    disk_init();
    if (DISK_ID > 3 || ide_devices[DISK_ID].Reserved == 0)
        return STA_NOINIT;
    else if (ide_devices[DISK_ID].Type == IDE_ATAPI) 
        return STA_PROTECT;
    else
        return 0;
}

/**
  * @brief  Get disk current status
  * @param  pdrv: Physical drive number
  * @retval disk status
  *         - 0: Normal status
  *         - STA_NOINIT: Device is not initialized and not ready to work
  *         - STA_PROTECT: Medium is write protected
  */
DSTATUS disk_status (BYTE pdrv)
{
/* TODO */
/* Note: You can create a function under disk.c  
 *       to help you get the disk status.
 */
    int status = ide_read(DISK_ID, ATA_REG_STATUS) ;
    if (DISK_ID > 3 || ide_devices[DISK_ID].Reserved == 0 || 
        (status != 0 && !(status&ATA_SR_DRDY))){
        printk("<%x>\n", ide_read(DISK_ID, ATA_REG_STATUS));
        return STA_NOINIT;
    }else if (ide_devices[DISK_ID].Type == IDE_ATAPI) 
        return STA_PROTECT;
    else
        return 0;
}

/**
  * @brief  Read serval sector form a IDE disk
  * @param  pdrv: Physical drive number
  * @param  buff: destination memory start address
  * @param  sector: start sector number
  * @param  count: number of sector
  * @retval Results of Disk Functions (See diskio.h)
  *         - RES_OK: success
  *         - < 0: failed
  */
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
    int err = 0;
    int i = count;
    BYTE *ptr = buff;
    UINT cur_sector = sector;
    err = disk_status(DISK_ID);
    switch (err) {
        case STA_NOINIT:
            return RES_NOTRDY;
        case STA_PROTECT:
            return RES_WRPRT;
    }
    while ((i - 255) > 0) {
        printk("reading...\n");
        err = ide_read_sectors(DISK_ID, 255, cur_sector, ptr);
        if (err < 0) {
            panic("disk_read");
            return RES_ERROR;
        }
        cur_sector += 255;
        ptr        += 255*SECTOR_SIZE;
        i          -= 255;
    }
    if (i != 0) 
        err = ide_read_sectors(DISK_ID, i, cur_sector, ptr);
    if (err < 0) {
        panic("disk_read");
        return RES_ERROR;
    }
    assert(err == 0);
    return RES_OK;
}

/**
  * @brief  Write serval sector to a IDE disk
  * @param  pdrv: Physical drive number
  * @param  buff: memory start address
  * @param  sector: destination start sector number
  * @param  count: number of sector
  * @retval Results of Disk Functions (See diskio.h)
  *         - RES_OK: success
  *         - < 0: failed
  */
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
    int err = 0;
    int i = count;
    BYTE *ptr = buff;
    UINT cur_sector = sector;
    /* TODO */    
    err = disk_status(DISK_ID);
    switch (err) {
        case STA_NOINIT:
            // panic("disk_write not ready");
            return RES_NOTRDY;
        case STA_PROTECT:
            // panic("disk_write w protect");
            return RES_WRPRT;
    }
    while ((i - 255) > 0) {
        printk("writing...\n");
        err = ide_write_sectors(DISK_ID, 255, cur_sector, ptr);
        if (err < 0) {
            panic("disk_write");
            return RES_ERROR;
        }
        cur_sector += 255;
        ptr        += 255*SECTOR_SIZE;
        i          -= 255;
    }

    if (i != 0) 
        err = ide_write_sectors(DISK_ID, i, cur_sector, ptr);
    if (err < 0) {
        panic("disk_write");
        return RES_ERROR;
    }
    assert(err == 0);
    return RES_OK;
}

/**
  * @brief  Get disk information form disk
  * @param  pdrv: Physical drive number
  * @param  cmd: disk control command (See diskio.h)
  *         - GET_SECTOR_COUNT
  *         - GET_BLOCK_SIZE (Same as sector size)
  * @param  buff: return memory space
  * @retval Results of Disk Functions (See diskio.h)
  *         - RES_OK: success
  *         - < 0: failed
  */
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
    uint32_t *retVal = (uint32_t *)buff;
    DWORD *retsize;
    unsigned int channel = ide_devices[DISK_ID].Channel;
    /* TODO */    
    
    switch (cmd) {
        case GET_SECTOR_COUNT:
            retsize = buff;
            *retsize = ide_devices[DISK_ID].Size;
            return RES_OK;
        case GET_BLOCK_SIZE:
            retsize = buff;
            *retsize = SECTOR_SIZE;
            return RES_OK;
        case CTRL_SYNC:
            ide_write(channel, ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
            ide_polling(channel, 0); // Polling.
            return RES_OK;
        default:
            panic("ioctl not implemented");
    }
}

/**
  * @brief  Get OS timestamp
  * @retval tick of CPU
  */
DWORD get_fattime (void)
{
    /* TODO */
    return sys_get_ticks();
}
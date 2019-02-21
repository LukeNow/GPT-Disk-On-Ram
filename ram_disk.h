#ifndef RAMDISK_H
#define RAMDISK_H

#include <linux/types.h>

extern int ramdisk_init(void);

extern void ramdisk_cleanup(void);

extern void ramdisk_write(sector_t off, u8 *buffer, unsigned int blocks);

extern void ramdidk_read(sector_t off, u8 *buffer, unsigned int blocks);

#endif

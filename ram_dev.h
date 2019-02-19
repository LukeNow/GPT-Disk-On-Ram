#ifndef RAMDEV_H
#define RAMDEV_H

#include <linux/types.h>

extern int ramdev_init(void);

extern void ramdev_cleanup(void);

extern void ramdev_write(sector_t off, u8 *buffer, unsigned int blocks);

extern void ramdev_read(sector_t off, u8 *buffer, unsigned int blocks);

#endif

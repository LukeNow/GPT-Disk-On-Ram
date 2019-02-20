#include <linux/vmalloc.h> 
#include <linux/types.h>
#include <linux/module.h>
#include "partition.h"
#include "ram_dev.h"

#define DISK_SIZE 0x100000
#define BLOCK_SIZE 512

static u8 *disk;

int ramdev_init(void)
{
	disk = vmalloc(DISK_SIZE);
	if (disk == NULL)
		return -ENOMEM;

	write_headers(disk);
		
	return DISK_SIZE;
}

void ramdev_cleanup(void)
{
	vfree(disk);
}

void ramdev_write(sector_t off, u8 *buffer, unsigned int blocks)
{
	memcpy(disk + (off * BLOCK_SIZE), buffer,
	       blocks * BLOCK_SIZE);
}

void ramdev_read(sector_t off, u8 *buffer, unsigned int blocks)
{
	memcpy(buffer, disk + (off * BLOCK_SIZE), 
	       blocks * BLOCK_SIZE);
}


MODULE_LICENSE("GPL");

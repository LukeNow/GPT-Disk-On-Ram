#include <linux/vmalloc.h> 
#include <linux/types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "partition.h"
#include "ram_disk.h"

#define BLOCK_SIZE 512
#define DISK_BLOCKS (DISK_SIZE / BLOCK_SIZE)

static u8 *disk;

int ramdisk_init(void)
{
	disk = vzalloc(DISK_SIZE);
	if (disk == NULL)
		return -ENOMEM;
	
	write_headers_to_disk(disk);
		
	return DISK_BLOCKS;
}

void ramdisk_cleanup(void)
{
	vfree(disk);
}

void ramdisk_write(sector_t off, u8 *buffer, unsigned int blocks)
{
	memcpy(disk + (off * BLOCK_SIZE), buffer, blocks);
}

void ramdisk_read(sector_t off, u8 *buffer, unsigned int blocks)
{
	memcpy(buffer, disk + (off * BLOCK_SIZE), blocks);
}


MODULE_LICENSE("GPL");

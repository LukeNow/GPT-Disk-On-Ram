#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/errno.h>

#include "partition.h"
#include "ram_disk.h"

#define FIRST_MINOR 0
#define RD_MINOR_CNT 16 //hard cap

#define MODULE_NAME "rd_dev"

static unsigned int rd_major = 0;
static int err = 0;

static struct rd_device
{
	unsigned int size; //IN SECTORS/BLOCKS
	spinlock_t lock;
	struct request_queue *rd_queue;
	struct gendisk *rd_gendisk;
} rd_dev;

int __init rd_init(void)
{
	int ret;

	ret = ramdisk_init();
	if (ret < 0) {
		err = -ENOMEM;
		goto RAMDISK_ALLOC_FAIL;
	}
	rd_dev.size = ret;
	
	rd_major = register_blkdev(rd_major, MODULE_NAME);
	if (rd_major < 0) {
		printk(KERN_ERR "rd_dev: Unable to allocate major num\n");
		err = -EBUSY;
		goto MAJOR_REGISTER_FAIL;
	}
	printk(KERN_INFO "rd_dev: Registered with major num %d\n", rd_major);
	
	return 0;

MAJOR_REGISTER_FAIL:
	unregister_blkdev(rd_major, MODULE_NAME);
RAMDISK_ALLOC_FAIL:
	ramdisk_cleanup();
	
	return err;
}

void __exit rd_exit(void)
{
	unregister_blkdev(rd_major, MODULE_NAME);
	ramdisk_cleanup();
	printk(KERN_INFO "rd_dev: Unregistered device\n");
}

module_init(rd_init);
module_exit(rd_exit);
MODULE_LICENSE("GPL");

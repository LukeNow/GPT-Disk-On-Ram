#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/init.h>

#include "partition.h"
#include "ram_disk.h"

#define BLOCK_SIZE 512

int __init dev_init(void)
{
	//char buf[BLOCK_SIZE];
	//char *s = "Hello world!\n";
	
	
	printk(KERN_INFO "TEST_DEV: Initializing TEST!\n");
	ramdisk_init();
	
	printk(KERN_INFO "TEST_DEV: Writing TEST!\n");

	return 0;
}

void __exit dev_cleanup(void)
{
	ramdisk_cleanup();
	printk(KERN_INFO "TEST_DEV: Exiting TEST!\n");
}

module_init(dev_init);
module_exit(dev_cleanup);
MODULE_LICENSE("GPL");

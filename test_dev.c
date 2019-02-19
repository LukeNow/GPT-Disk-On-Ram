#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/init.h>

#include "parition.h"
#include "ram_dev.h"

#define BLOCK_SIZE 512

static int __init dev_init(void)
{
	char buf[BLOCK_SIZE];
	char *s = "Hello world!\n";
	
	
	printk(KERN_INFO "Initializing TEST!\n");
	ramdev_init();
	

	printk(KERN_INFO "Writing TEST!\n");
	

	return 0;
}

static void __exit dev_cleanup(void)
{
	ramdev_cleanup();
	printk(KERN_INFO "Exiting TEST!\n");
}

module_init(dev_init);
module_exit(dev_cleanup);
MODULE_LICENSE("GPL");

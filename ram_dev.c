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

#define MODULE_NAME "dor"

static unsigned int rd_major = 0;
static int err = 0;

static struct rd_device
{
	unsigned int size; //IN SECTORS/BLOCKS
	spinlock_t lock;
	struct request_queue *rd_queue;
	struct gendisk *rd_gendisk;
} rd_dev;


static int rd_open(struct block_device *bdev, fmod_t mode)
{
	unsigned int i_minor = iminor(bdev->bd_inode);
	
	if (iminor > RD_MINOR_CNT) {
		printk(KERN_ERR "ram_dev: Open failed\n");
		return -ENODEV;
	}

	printk(KERN_INFO "ram_dev: Device opened with Inode %d\n",
		i_minor);
	return 0;
}

static int rd_close(struct gendisk *disk, fmode_t mode)
{
	printk(KERN_INFO "ram_dev: Device closed\n");
	return 0;
}


static int rd_service(struct request *rq)
{
	
	struct bio_vec bvec;
	struct req_iterator iter;

	rq_for_each_segment(bvec, rq, iter) {
		sector_t sector = iter.iter.bi_sector;
		char *buffer = kmap_atomic(bvec.bv_page);
		unsigned long offset = bvec.bv_offset;
		
		size_t len = bvec.bv_len;
		sector_t sector_num = (len / SECTOR_SIZE);
		int dir = bio_data_dir(iter.bio);
		
		if (dir == WRITE) 
			ramdisk_write(sector + offset, buffer, sector_num);
		else
			ramdisk_read(sector + offset, buffer, sector_num);

		kunmap_atomic(buffer);
	}

	return 0;
}

static void rd_request(struct request_queue *q)
{
	int ret;
	struct request *rq;

	while (1) {
		/* fetch request from request queue */
		rq = blk_fetch_request(q);
		if (!rq)
			break;
		
		/* If the request is not from fs we skip */
		if (blk_rq_is_passthrough(rq)) {
		    printk (KERN_NOTICE "ram_dev: Skipping non-fs request\n");
		    __blk_end_request_all(rq, -EIO);
		   continue;
		}
		
		/* Lets service the request */
		ret = rd_service(rq);
		
		/* Return the return value associated with this request */
		__blk_end_request_all(rq, ret);
	}
}





static struct block_device_operations blk_fops = {
	.owner = THIS_MODULE,
	.open = rd_open,
	.close = rd_close
};













int __init rd_init(void)
{
	int ret;
	
	/* Lets initialize and vmalloc our disk */
	ret = ramdisk_init();
	if (ret < 0) {
		err = -ENOMEM;
		goto RAMDISK_ALLOC_FAIL;
	}
	rd_dev.size = ret;
	
	/* Lets register our block driver */
	rd_major = register_blkdev(rd_major, MODULE_NAME);
	if (rd_major < 0) {
		printk(KERN_ERR "dor: Unable to allocate major num\n");
		err = -EBUSY;
		goto MAJOR_REGISTER_FAIL;
	}
	printk(KERN_INFO "dor: Registered with major num %d\n", rd_major);
	
	/* Init spin lock and init request queue. Then set private data */
	spin_lock_init(&rd_dev.lock);
	rd_dev.rd_queue = blk_init_queue(rd_request, &rd_dev.lock);
	if (!rd_dev.rd_queue) {
		printk(KERN_ERR "dor: queue init failed\n");
		err = -ENOMEM;
		goto QUEUE_INIT_FAIL;
	}
	rd_dev.rd_queue->queuedata = &rd_dev;
	
	/* Lets create the gen disk struct */
	rd_dev.rd_gendisk = alloc_disk(RD_MINOR_CNT);
	if (!rd_dev.rd_gendisk) {
		printk(KERN_ERR "dor: Disk Alloc failed\n");
		err = -ENOMEM;
		goto DISK_ALLOC_FAIL;
	}
	
	rd_dev.rd_gendisk->fops = &blk_fops;
	rd_dev.rd_gendisk->private_data = &rd_dev;


	


	return 0;

DISK_ALLOC_FAIL:
	blk_cleanup_queue(rd_dev.rd_queue);
QUEUE_INIT_FAIL:
	unregister_blkdev(rd_major, MODULE_NAME);
MAJOR_REGISTER_FAIL:
	ramdisk_cleanup();
RAMDISK_ALLOC_FAIL:
	return err;
}

void __exit rd_exit(void)
{
	unregister_blkdev(rd_major, MODULE_NAME);
	ramdisk_cleanup();
	printk(KERN_INFO "dor: Unregistered device\n");
}

module_init(rd_init);
module_exit(rd_exit);
MODULE_LICENSE("GPL");

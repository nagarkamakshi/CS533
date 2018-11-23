#include <linux/init.h>         
#include <linux/module.h>       

#include <linux/errno.h>        //Error code definitions
#include <linux/genhd.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/kernel.h>	/* printk() */

#define MY_BLOCK_MAJOR           240	//240-254 is for local/ experimental use
#define MY_BLKDEV_NAME          "mybdev"
#define NR_SECTORS                   1024
#define MY_BLOCK_MINORS       1
#define KERNEL_SECTOR_SIZE           512

static struct my_block_dev {
    //...
    // TODO data array or a data structure where the data is stored ?
    // Maybe need to study CH.8 of ldd3 book
	unsigned long size;             /*Device size*/
	u8 *data;                       /*Data array */
    spinlock_t lock;                /* For mutual exclusion */
    struct request_queue *queue;    /* The device request queue */
    struct gendisk *gd;             /* The gendisk structure */
    //...
} dev;

static void my_block_request(struct request_queue *q);

static int my_block_open(struct block_device *bdev, fmode_t mode)
{
    //...
	printk(KERN_INFO "CS533 Device opened.\n");
    return 0;
}

static int my_block_release(struct gendisk *gd, fmode_t mode)
{
    //...
	printk(KERN_INFO "CS533 Device released.\n");
    return 0;
}

//TODO myblock_ioctl?

struct block_device_operations my_block_ops = {
    .owner = THIS_MODULE,
    .open = my_block_open,
    .release = my_block_release
	//.rw_page = my_block_rw_page
	//.ioctl = my_block_ioctl,
	//.direct_access = my_block_direct_access
};


static int create_block_device(struct my_block_dev *dev)
{
    // TODO ? Allocate memory and initialize simulated RAM device
    //(pages/ sectors/ partition of RAM disk memory or block/ sector /cylinders/ heads/ tracks/ etc. of HDD)
	
    //Need to decide simulated device type (Random access/ direct access of request OR sequential access of request)
    //Check whether it will have repercussions with the perf monitor and file r/w load distribution
    //(Maybe sequential access will make file split/ partitioning load distribution pointless as device I/O or the r/w request is processed 1-by-1)
	
	//From sample Allocate vmem for data array
    memset (dev, 0, sizeof (struct sbull_dev));
    dev->size = NR_SECTORS * KERNEL_SECTOR_SIZE;
    dev->data = vmalloc(dev->size);
    if (dev->data == NULL) {
        printk (KERN_NOTICE "CS533: vmalloc failure.\n");
        return;
    }
	
    //Register block device, optional since we statically define major number 240, tradition
    if(register_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME) <= 0)
    {
        printk(KERN_WARNING "CS533: register_blkdev failure\n");
        vfree(dev->data);
        return -EBUSY;
    }
	
    /* Initialize the gendisk structure */
    dev->gd = alloc_disk(MY_BLOCK_MINORS);
    if (!dev->gd) {
        printk (KERN_NOTICE "CS533: alloc_disk failure\n");
        unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME)
        vfree(dev->data);
        return -ENOMEM;
    }

    dev->gd->major = MY_BLOCK_MAJOR;
    dev->gd->first_minor = 0;
    dev->gd->fops = &my_block_ops;
    dev->gd->queue = dev->queue;
    dev->gd->private_data = dev;
    snprintf (dev->gd->disk_name, 32, "myblock");
    set_capacity(dev->gd, NR_SECTORS);

    add_disk(dev->gd);
	
	/* Initialize the I/O queue */
    spin_lock_init(&dev->lock);
    dev->queue = blk_init_queue(my_block_request, &dev->lock);	//If we will use Random Access/ no request queue
	                                                            //use blk_alloc_queue(GFP_KERNEL) instead
																//and implement a make request function
    if (dev->queue == NULL)
        goto out_err;
    blk_queue_logical_block_size(dev->queue, KERNEL_SECTOR_SIZE);
    dev->queue->queuedata = dev;
    //...

	return 0;
	
	out_err:
    blk_cleanup_queue(dev->queue);
    del_gendisk(dev->gd);	
    put_disk(dev->gd);		
    unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
    vfree(dev->data);vfree(dev->data);
    return -ENOMEM;
}

//The request function depends on
static void my_block_request(struct request_queue *q)
{
    struct request *rq;
    struct my_block_dev *dev = q->queuedata;

    while (1) {
        rq = blk_fetch_request(q);
        if (rq == NULL)
            break;

        if (blk_rq_is_passthrough(rq)) {
            printk (KERN_NOTICE "Skip non-fs request\n");
            __blk_end_request_all(rq, -EIO);
           continue;
        }

        /* do work */
        //... Maybe call the r/w function? or handled by bio? or simple/ full transfer request?

        __blk_end_request_all(rq, 0);
    }
}

static int my_block_init(void)
{
    int status;
    //...
    status = create_block_device(&dev);
    if (status < 0)
	{
       printk(KERN_ERR "unable to register mybdev block device\n");
       return -EBUSY;
	}
	return 0;
    //...
}

static void delete_block_device(struct my_block_dev *dev)
{
 	if(dev->queue) {
        blk_cleanup_queue(dev->queue);
	}
	if (dev->gd) {
        del_gendisk(dev->gd);
        put_disk(dev->gd);
    }
	if(dev->data) {
        vfree(dev->data);
	}
    //...
}

static void my_block_exit(void)
{
    delete_block_device(&dev);
    unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
    //...
}
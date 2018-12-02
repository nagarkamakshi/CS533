//The block device driver
//TO USE:
//First use Makefile to 'make' and compile the .ko module file
//Then do 'insmod my_driver.ko' to insert the module, may need super user root permission
//Device driver module should be inserted and device created
//Do Stuff...
//Do 'rmmod my_driver.ko' to remove the module and device
#include <linux/init.h>     
#include <linux/module.h>       
#include <linux/errno.h>
#include <linux/genhd.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/kernel.h>
#include <linux/bio.h>

#define MY_BLOCK_MAJOR           	240			//240-254 is for local/ experimental use
#define MY_BLKDEV_NAME          	"CS533_myblock"	//Name of device driver
#define NR_SECTORS                   	1024	//Number of sectors
#define MY_BLOCK_MINORS       			1		//Number of Minors/ segments of device, set to 1 for simplest device with 1 segment
#define KERNEL_SECTOR_SIZE           	512		//Size of a sector, which is how much sectors can operate on

MODULE_LICENSE("GPL");						//License so insmod doesn't give warnings

//Internal representation of the block device driver
static struct my_block_dev {
	unsigned long size;             //Size of device, should be NR_SECTORS * KERNEL_SECTOR_SIZE
	u8 *data;                       //The data array representing the RAM disk device, simplest is just virtual memory allocated
    spinlock_t lock;                //Lock for mutual exclusion of request queue
    struct request_queue *queue;    //Device request queue
    struct gendisk *gd;             //Gendisk structure
} dev;

//Declaration of request function as it is referenced in file ops structure
static void my_block_request(struct request_queue *q);

//open function, called by kernel whenever device is opened, e.g. before performing file ops. 
//Doesn't do much since our device driver is simple
static int my_block_open(struct block_device *bdev, fmode_t mode)
{
	//DEBUG printk(KERN_INFO "CS533 Device opened.\n");
    return 0;
}


//open function, called by kernel whenever device is closed/ released, e.g. after performing file ops. 
//Again, doesn't do much since our device driver is simple
static void my_block_release(struct gendisk *gd, fmode_t mode)
{
	//DEBUG printk(KERN_INFO "CS533 Device released.\n");
    return;
}

//File operations struct, listing the file operations supported by block device
struct block_device_operations my_block_ops = {
    .owner = THIS_MODULE,
    .open = my_block_open,
    .release = my_block_release
};

//Function to create "block device" and initialize device driver
static int create_block_device(struct my_block_dev *dev)
{
	//Allocate virtual memory as our "RAM disk device"
    memset (dev, 0, sizeof (struct my_block_dev));	//memset field for our internal representation
    dev->size = NR_SECTORS * KERNEL_SECTOR_SIZE;
    dev->data = vmalloc(dev->size);
    if (dev->data == NULL) {
        //DEBUG printk (KERN_NOTICE "CS533: vmalloc failure.\n");
        return -1;
    }
	
    //Register block device, optional since we statically define major number as 240, but tradition
    if(register_blkdev(MY_BLOCK_MAJOR, "CS533_myblock") < 0)
    {
        //DEBUG printk(KERN_WARNING "CS533: register_blkdev failure\n");
        vfree(dev->data);
        return -EBUSY;
    }
	
	//Initialize Input/ Output request queue
    spin_lock_init(&dev->lock);
    dev->queue = blk_init_queue(my_block_request, &dev->lock);	//We use simple device, the 'init' version of the function will also
																//allocate a bios (block io structure) for the queue.
																//we decided not to use Random Access/ no request queue and multiple bios
	                                                            //use blk_alloc_queue(GFP_KERNEL) instead
																//and implement a make request function
    if (dev->queue == NULL) {
        //DEBUG printk(KERN_WARNING "CS533: blk_init_queue failure\n");
        unregister_blkdev(MY_BLOCK_MAJOR, "CS533_myblock");
        vfree(dev->data);
	}
    
	//Set the logical block size the device can address, the logical block size for the queue
    blk_queue_logical_block_size(dev->queue, KERNEL_SECTOR_SIZE);
	
	//Save our internal representation of the device in the queue's private queuedata
    dev->queue->queuedata = dev;
	
    //Initialize the gendisk structure
    dev->gd = alloc_disk(MY_BLOCK_MINORS);
    if (!dev->gd) {
        //DEBUG printk (KERN_NOTICE "CS533: alloc_disk failure\n");
        goto out_err;
    }

    dev->gd->major = MY_BLOCK_MAJOR;				//Set major number, should be the macro defined 240
    dev->gd->first_minor = 0;						//First minor device number corresponding to disk, 
													//used for how driver partition the device, not to important for our simple device
	snprintf (dev->gd->disk_name, 32, "CS533_myblock");	//Write name of disk, used in creating a sysfs directory for the device
    dev->gd->fops = &my_block_ops;					//The device operations supported by the device, set it to our struct above
    dev->gd->queue = dev->queue;					//The request queue that will handle device operations for the disk
    dev->gd->private_data = dev;					//Used for storage of driver private data, store the internal representation here
    set_capacity(dev->gd, NR_SECTORS);				//Size of disk, in Numbers of Sectors

    add_disk(dev->gd);	//Add gendisk to list of active disks

	return 0;
	
	out_err:
	blk_cleanup_queue(dev->queue);
    unregister_blkdev(MY_BLOCK_MAJOR, "CS533_myblock");
    vfree(dev->data);
    return -ENOMEM;
}

//Function for processing read/ write requests
static int my_block_process_request_rw(struct my_block_dev *dev, sector_t sector,
               unsigned long length, char *buffer, int rwflag)
{
               unsigned long offset = sector * KERNEL_SECTOR_SIZE;	// Offset for the location of address to begin read/ write
			   
			   //Prevent read/write beyond block device memory
			   if((offset + length) > dev->size)
				   return 1;
			   
			   //(WRITE)Write from buffer to device memory starting at data+offset, for 'length' amount
			   if(rwflag == 1)
				   memcpy(dev->data + offset, buffer, length);
			   else
				   //(READ)Write from device memory to buffer starting at data+offset, for 'length' amount
				   memcpy(buffer, dev->data + offset, length);
			   
			   return 0;
}

//The request function which fetches request and filters read/write requests
static void my_block_request(struct request_queue *q)
{
    struct request *rq;
    struct my_block_dev *dev = q->queuedata;

    while (1) {
		//Get request from queue
        rq = blk_fetch_request(q);
        if (rq == NULL)
            break;

		//Check whether it is non file system (read/write) request, if so skip them
        if (blk_rq_is_passthrough(rq)) {
            //DEBUG printk (KERN_NOTICE "CS533: Skip non-fs request\n");
            __blk_end_request_all(rq, -EIO);
           continue;
        }
		
		//Print request info
		//DEBUG printk(KERN_INFO "CS533: request received: pos=%llu bytes=%u cur_bytes=%u dir=%c\n",
			(unsigned long long) blk_rq_pos(rq), blk_rq_bytes(rq), 
			blk_rq_cur_bytes(rq), rq_data_dir(rq) ? 'W' : 'R');

		
		//Pass the read/write request to our processing function
		//blkdev.h macros:
		//blk_rq_pos(rq), gets the current sector the request is for
		//blk_rq_bytes(rq), Bytes left to complete in the entire request
		//bio_data(rq->bio), Get kernel virtual address for the data buffer
		//rq_data_dir(rq), Get the Read/ Write direction of the request
		if(my_block_process_request_rw(dev, blk_rq_pos(rq), blk_rq_bytes(rq), 
		   bio_data(rq->bio), (rq_data_dir(rq))) == 1)
		   //DEBUG printk(KERN_NOTICE "CS533: Read/ Write request failed\n");

        __blk_end_request_all(rq, 0);
    }
}

//Init function for the block device driver, __init macro lets kernel do its stuff
static int __init my_block_init(void)
{
    int status;
    //...
    status = create_block_device(&dev);
    if (status < 0)
	{
       //DEBUG printk(KERN_ERR "CS533: unable to register CS533_myblock block device\n");
       return -EBUSY;
	}
	return 0;
    //...
}

//Helper function for cleaning up the block device when exiting
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
}


//Exit function for the block device driver, __exit macro lets kernel do its stuff
static void __exit my_block_exit(void)
{
    delete_block_device(&dev);
    unregister_blkdev(MY_BLOCK_MAJOR, "CS533_myblock");
}

//the module init and exit
module_init(my_block_init);
module_exit(my_block_exit);
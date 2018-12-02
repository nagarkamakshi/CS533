#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h> /*for the char driver support */

MODULE_LICENSE("GPL");

volatile static int is_open=0;
static char message[1024];
static int devnum =0;
static int bytes_read =0;
int num_bytes = 0;

int char_open(struct inode *pinode, struct file *pfile)
{
	if(is_open == 1 ){
	printk(KERN_INFO "Error - char device is open");
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);
	return -EBUSY;
	}
	is_open = 1;
	try_module_get(THIS_MODULE);
}

ssize_t char_read(struct file *pfile,char __user *buffer, size_t length, loff_t *offset)
{
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__); 
	if (offset == NULL) return -EINVAL;
	if(offset >= num_bytes) return 0;
	
	while((bytes_read <nbytes) && (*offset< num_bytes)){
		put_user(message[*offset], &outb[bytes_read]);
		*offset = *offset +1;
		bytes_read ++;
	}
	return bytes_read;
}
ssize_t char_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{

	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);
	return length;
}

int char_close(struct inode *pinode, struct file *pfile)
{
	
	if(is_open == 0 ){
	printk(KERN_INFO "Error - char device was not open");
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);
	return -EBUSY;
	}
	is_open = 0;
	module_put(THIS_MODULE);
}


struct file_operations char_file_operations = {
.owner = THIS_MODULE,
.open = char_open,
.read = char_read,
.write = char_write,
.release = char_close,
};

static int __init char_module_init(void){
	
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__); 
	/*Register with the kernel that we are registring a character device*/
	devnum = register_chrdev(240 /*Major number*/,"Character_Device " /*Name of the driver*/,&char_file_operations /*File operation */);
	printk(KERN_INFO "The character device major number is : %d\n", devnum);
	return 0;
}

void __exit char_module_exit(void){
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);

	/*unregister char device*/
	unregister_chrdev(240,"Character Device");
	return;

}

module_init(char_module_init);
module_exit(char_module_exit);
	

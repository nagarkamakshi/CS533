#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h> /*for the char driver support */

MODULE_LICENSE("GPL");

int char_open(struct inode *pinode, struct file *pfile)
{
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);
	return 0;
}

ssize_t char_read(struct file *pfile,char __user *buffer, size_t length, loff_t *offset)
{
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__); 
	return 0;
}
ssize_t char_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{

	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);
	return length;
}

int char_close(struct inode *pinode, struct file *pfile)
{
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);
	return 0;
}


struct file_operations char_file_operations = {
.owner = THIS_MODULE,
.open = char_open,
.read = char_read,
.write = char_write,
.release = char_close,
};

int char_module_init(void){
	
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__); 
	/*Register with the kernel that we are registring a character device*/
	register_chrdev(240 /*Major number*/,"Character_Device " /*Name of the driver*/,&char_file_operations /*File operation */);
	return 0;
}

void char_module_exit(void){
	printk(KERN_ALERT "Inside the %s function \n", __FUNCTION__);

	/*unregister char device*/
	unregister_chrdev(240,"Character Device");
	return;

}

module_init(char_module_init);
module_exit(char_module_exit);
	

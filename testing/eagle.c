#include<linux/init.h>
#include<linux/module.h>
#include<linux/mm.h>
#include<linux/types.h>		//for dev_t type
#include<linux/kdev_t.h> //for major minor macros
#include<linux/fs.h>	//for obtaining device numbers i.e registering chrdev
#include<linux/cdev.h> //for structure of char driver
#include<asm/uaccess.h> //for copying data to & from user to kernel space
#include<asm/atomic.h>
#include<linux/sched.h> //for checking *capability* using capabale() 

#include<linux/wait.h> //for blocking i/o -> wait_queue

#include<linux/slab.h> //for kmalloc and kfree

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RAJ");


static struct cdev cq_cdev;
static struct class *cq_class;
dev_t dev=0; //device 
static atomic_t open_count = ATOMIC_INIT(0);
static int cq_open(struct inode* inode,struct file* filp);
static int cq_release(struct inode* inode,struct file* filp);
static ssize_t cq_read(struct file* filp,char __user *buf,size_t count,loff_t *f_pos);
static ssize_t cq_write(struct file* filp,const char __user *buf,size_t count,loff_t *offp);
//static long cq_ioctl(struct file* filp,unsigned int cmd,unsigned long arg);


struct file_operations fops=
{
	.owner = THIS_MODULE,
	.read = cq_read,
	.write = cq_write,
	//.unlocked_ioctl = cq_ioctl,
	.open = cq_open,
	.release = cq_release,
};
static int __init initialise(void) {
    int err = 0;
    
    // Allocate device number
    err = alloc_chrdev_region(&dev, 1, 1, "eagledev");
    if (err < 0) {
        printk(KERN_WARNING "Failure in char device allocation\n");
        return err;
    }
    printk(KERN_INFO "Module loaded with MAJOR = %d && MINOR = %d\n", MAJOR(dev), MINOR(dev));
    
    // Initialize cdev
    cdev_init(&cq_cdev, &fops);
    cq_cdev.owner = THIS_MODULE;
    
    // Add cdev to the system
    err = cdev_add(&cq_cdev, dev, 1);
    if (err) {
        printk(KERN_NOTICE "Error %d adding cdev to the system\n", err);
        goto failed;
    }else{
     return 0;
    }  

failed:
    unregister_chrdev_region(dev, 1);
    return err;
}
static void __exit release(void){
	device_destroy(cq_class,dev);
	unregister_chrdev_region(dev,1);
	printk(KERN_INFO "module unloaded\n");

}
static int cq_open(struct inode* inode,struct file* filp){
	
	atomic_inc(&open_count);
    printk(KERN_INFO "Device opened. Open count: %d\n", atomic_read(&open_count));
     
	return 0;
}
static int cq_release(struct inode* inode,struct file* filp){
	 atomic_dec(&open_count);
    printk(KERN_INFO "Device closed. Open count: %d\n", atomic_read(&open_count));
    
	return 0;
}
static ssize_t cq_read(struct file* filp, char __user *buf, size_t count, loff_t *f_pos) {
    char *kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf) {
        printk(KERN_ERR "Failed to allocate memory in cq_read\n");
        return -ENOMEM;
    }

    strncpy(kbuf, "Sample data from device", count); // Example data
    if (copy_to_user(buf, kbuf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }

    printk(KERN_INFO "cq_read: %zu bytes read\n", count);
    kfree(kbuf);
    return count;  // Return the number of bytes read
}

static ssize_t cq_write(struct file* filp, const char __user *buf, size_t count, loff_t *offp) {
    char *kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf) {
        printk(KERN_ERR "Failed to allocate memory in cq_write\n");
        return -ENOMEM;
    }

    if (copy_from_user(kbuf, buf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }

    printk(KERN_INFO "cq_write: Received %zu bytes: %s\n", count, kbuf);
    kfree(kbuf);
    return count;  // Return the number of bytes written
}


module_init(initialise);
module_exit(release);

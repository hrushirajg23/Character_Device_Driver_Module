#include<linux/init.h>
#include<linux/module.h>
#include<linux/mm.h>
#include<linux/types.h>		//for dev_t type
#include<linux/kdev_t.h> //for major minor macros
#include<linux/fs.h>	//for obtaining device numbers i.e registering chrdev
#include<linux/cdev.h> //for structure of char driver
#include<asm/uaccess.h> //for copying data to & from user to kernel space

#include<linux/sched.h> //for checking *capability* using capabale() 

#include<linux/wait.h> //for blocking i/o -> wait_queue

#include<linux/slab.h> //for kmalloc and kfree

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RAJ");

/* you can use poll /epoll /select for blocking data writing process until queue is full */


/*
	using access_ok() for verifying user addrs before r/w_ing them 
	
	__put_user() & __get_user() macros shoud be used instead of copy_to_user()& 
	copy_from_user() as it is fast. But they only write single values at a time.
	
	For data not fitting specific size the latter must be used
*/


/*
	first declare a static wait_queue head by DECLARE macro
	
	then for putting process to sleep incase of no data by using macro 
	
	wait_event(queue,condition)
	
	queue is the queue_head we declared by DECLARE macro.

	
*/

#define CQUEUE_IOC_MAGIC 'a'
#define SET_SIZE_OF_QUEUE _IOW('a', 'a', int * ) //writing data to driver
#define PUSH_DATA _IOW('a', 'b', struct data * ) //writing data
#define POP_DATA _IOR('a', 'c', struct data * )
// #define VERIFY_READ 1
// #define VERIFY_WRITE 0


static struct cdev cq_cdev;
static struct class *cq_class;
dev_t dev=0; //device 
static int nr_opens=0;
DECLARE_WAIT_QUEUE_HEAD(wq);


struct data{
	int length;
	char* data;
};	

struct cqueue{
	struct data* dobj;
	struct cqueue* next;
};

struct cqueue_mgr{
	struct cqueue* head;
	struct cqueue* tail;
	int size;
	int curr_size;
};

static int cq_open(struct inode* inode,struct file* filp);
static int cq_release(struct inode* inode,struct file* filp);
static ssize_t cq_read(struct file* filp,char __user *buf,size_t count,loff_t *f_pos);
static ssize_t cq_write(struct file* filp,const char __user *buf,size_t count,loff_t *offp);
static long cq_ioctl(struct file* filp,unsigned int cmd,unsigned long arg);

void insert(struct cqueue_mgr* mgr,struct data* d);
void display(struct cqueue_mgr* mgr);
struct data* remove(struct cqueue_mgr* mgr);
void destroy_queue(struct cqueue_mgr* mgr);

struct file_operations fops=
{
	.owner = THIS_MODULE,
	.read = cq_read,
	.write = cq_write,
	.unlocked_ioctl = cq_ioctl,
	.open = cq_open,
	.release = cq_release,
};




static int __init initialise(void){
	if((alloc_chrdev_region(&dev,0,1,"mydev"))<0){
		printk(KERN_WARNING "failure in char device allocation");
		return -1;
	}
	printk(KERN_INFO "module loaded with MAJOR = %d && MINOR = %d\n",MAJOR(dev),MINOR(dev));
	
	/*
		statically initialising cdev  
	*/
	cdev_init(&cq_cdev,&fops);
	/*
		telling kernel about it
		*/
	if((cdev_add(&cq_cdev,dev,1))<0){
		printk(KERN_INFO "cannot add cdev to the system");
		goto failed;
	}
	if(IS_ERR(cq_class=class_create("cq_class2"))){
		printk(KERN_INFO "cannot create class");
		goto failed;
	}

	if(IS_ERR(device_create(cq_class,NULL,dev,NULL,"cq_device2"))){
		printk(KERN_INFO "Cannot create device\n");
		goto failed_device;
	}
	failed:
		unregister_chrdev_region(dev,1);
		cdev_del(&cq_cdev);
	failed_device:
		class_destroy(cq_class);
	return 0;
}
static void __exit release(void){
	device_destroy(cq_class,dev);
	unregister_chrdev_region(dev,1);
	printk(KERN_INFO "module unloaded\n");

}
static int cq_open(struct inode* inode,struct file* filp){
	printk(KERN_INFO "cq_open");
	if(nr_opens==0){
		struct cqueue_mgr* mgr=(struct cqueue_mgr*)kmalloc(sizeof(struct cqueue_mgr),GFP_KERNEL);
	if(mgr==NULL){
		printk(KERN_INFO "Couldn't allocate memory for cqueue_mgr\n");
		return -1;
	}
	mgr->head=NULL;
	mgr->tail=NULL;
	mgr->size=0;
	mgr->curr_size=0;
	filp->private_data=mgr;
	
	}
	nr_opens++;
	
	return 0;
}

void destroy_queue(struct cqueue_mgr* mgr){
	if(mgr->curr_size==0 && mgr->head==NULL && mgr->tail==NULL)return;
	
	else if(mgr->head==mgr->tail){
		kfree(mgr->head->dobj);
		kfree(mgr->head);
		mgr->head=NULL;
		mgr->tail=NULL;
	}
	else{
		struct cqueue* ptr=mgr->head;
		while(ptr!=mgr->tail){
			struct cqueue* hold=ptr->next;
			kfree(ptr->dobj->data);
			kfree(ptr->dobj);
			kfree(ptr);
			ptr=hold;
		}
		kfree(ptr->dobj->data);
		kfree(ptr->dobj);
		kfree(ptr);
		ptr=NULL;
		kfree(mgr);
		mgr=NULL;
		printk(KERN_INFO "Closed successfully with deallocation of resources\n");
	}
	

}

static int cq_release(struct inode* inode,struct file* filp){
	nr_opens--;
	if(nr_opens==0){
		destroy_queue((struct cqueue_mgr*)filp->private_data);
		kfree(filp->private_data);

	}
	printk(KERN_INFO "cq_release");
	return 0;
}
static ssize_t cq_read(struct file* filp,char __user *buf,size_t count,loff_t *f_pos){
	printk(KERN_INFO "cq_read");
	return 0;
}

static ssize_t cq_write(struct file* filp,const char __user *buf,size_t count,loff_t *offp){
	printk(KERN_INFO "cq_write");
	return 0;
}
static long cq_ioctl(struct file* filp,unsigned int cmd,unsigned long arg){
	struct data* d=(struct data*)kmalloc(sizeof(struct data),GFP_KERNEL);
	if(d==NULL){
		printk(KERN_INFO "kmalloc failed badly\n");
		return -EFAULT;
	}
	int size;
	switch(cmd){
		case SET_SIZE_OF_QUEUE:
			if(!access_ok((void __user*)arg,_IOC_SIZE(cmd))){
				kfree(d);
				return -EFAULT;
			}
			if((__get_user(size,(int __user*)arg)==0) ){
				((struct cqueue_mgr*)filp->private_data)->size=size;
			}
			break;
		case PUSH_DATA:
			if(!access_ok((void __user*)arg,_IOC_SIZE(cmd))){
				kfree(d);
				return -EFAULT;
			}
			if((__get_user(size,(int __user *)arg))!=0){
				kfree(d);
				return -EFAULT;
			}
			d->length=size;
			//char* ptr=d->data;
			if((d->data=(char*)kmalloc(d->length+1,GFP_KERNEL))==NULL){
				printk(KERN_INFO "kmalloc error for d.data\n");
				kfree(d);
				return -EFAULT;
			}
			
			if((copy_from_user(d->data,((struct data*)arg)->data,d->length))==-EFAULT){
				printk(KERN_INFO "data copying error\n");
				kfree(d->data);
				kfree(d);
				return -EFAULT;
			}
			d->data[d->length]='\n';
			insert(((struct cqueue_mgr*)filp->private_data),d);
			break;
		case POP_DATA:
			kfree(d);
			if(!access_ok((void __user*)arg,_IOC_SIZE(cmd))){
				return -EFAULT;
			}
			d=remove((struct cqueue_mgr*)filp->private_data);
			if((__put_user(*(int*)d,(int*)arg))==-EFAULT){
				kfree(d->data);
				kfree(d);
				return -EFAULT;
			}
			if((copy_to_user((*(struct data*)arg).data,d->data,d->length-1))==-EFAULT){
				printk(KERN_INFO "POP data copying error\n");
				kfree(d->data);
				kfree(d);
				return -EFAULT;
			}
			kfree(d->data);
			kfree(d);
			break;
		default:
			return -ENOTTY;
	}
	return 1;
}





void insert(struct cqueue_mgr* mgr,struct data* d){
	if(mgr->curr_size >=mgr->size){
		printk(KERN_INFO "Queue is full\n" );
		return;
	}
	struct cqueue* node=(struct cqueue*)kmalloc(sizeof(struct cqueue),GFP_KERNEL);
	
	if(node==NULL){
			printk(KERN_INFO "memory allocation for cqueue node failed");
			return;
	}
	wake_up_interruptible(&wq);
		// if((node->dobj.data=(char*)kmalloc(d->length+1,GFP_KERNEL))==NULL){
		// 	printk(KERN_INFO "memory allocation for cqueue node.obj failed");
		// 	return;
		// }
	
	node->dobj=(d);
		// node->dobj.data[length]='\n';
		// node->dobj.length=d->length+1;
	if(mgr->head==NULL && mgr->tail==NULL){
		mgr->head=node;
		mgr->tail=node;	
	}
	else{
		mgr->tail->next=node;
		mgr->tail=node;
	}
	mgr->curr_size++;
	if(mgr->tail!=NULL){
		mgr->tail->next=mgr->head;
	}
}

void display(struct cqueue_mgr* mgr){
	struct cqueue* node=mgr->head;
	printk(KERN_INFO "printing queue\n");
	do{
		printk(KERN_INFO "| %s |-> ",node->dobj->data);
		node=node->next;
	}while(node!=mgr->head);
	printk("NULL\n");
}

struct data* remove(struct cqueue_mgr* mgr){
	struct cqueue* node;
	if(mgr->curr_size==0){
		printk(KERN_INFO "Empty queue\n");
	}
	wait_event_interruptible(wq,mgr->curr_size!=0);
	if(mgr->head==NULL && mgr->tail==NULL && mgr->curr_size==0){
		printk(KERN_INFO "queue is empty wait_event activated\n");
		return NULL;
	}
	else if(mgr->head==mgr->tail){
		node=mgr->head;
		mgr->head=NULL;
		mgr->tail=NULL;
	}
	else{
		node=mgr->head;
		mgr->head=mgr->head->next;
		mgr->tail->next=mgr->head;
	}
	mgr->curr_size--;
	struct data* d=node->dobj;
	kfree(node);
	return d;
}

module_init(initialise);
module_exit(release);

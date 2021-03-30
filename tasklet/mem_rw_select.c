#include <linux/init.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/version.h>
//注册设备号所需要的头文件
#include <linux/fs.h>
//注册字符设备所需要的头文件
#include <linux/cdev.h>
//包含container_of宏
#include <linux/kernel.h>
//包含kmalloc和kfree
#include <linux/slab.h>
//包含cpoy_to_user和copy_from_user
#include <asm/uaccess.h>
//包含semaphore
//#include <asm/semaphore.h>
#include <linux/semaphore.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
//内核定时器
#include <linux/timer.h>
//tasklet
#include <linux/interrupt.h>

#define DEVICE_NAME     "aTest"
#define BUFFER_SIZE     256
#define MAJOR_MODE      0   //动态申请

//#define ONLY_ONE_USER
static int version = 0;
static int timeout_value = 10;

#ifdef ONLY_ONE_USER
static atomic_t test_s_available = ATOMIC_INIT(1);
#endif

//data pool, copy from stm32
#define USE_MALLOC	1
#pragma pack(1)
typedef struct DataPool{
	uint32_t u32Start;    //数据池的等待读取的开始地址a
	uint32_t u32End;      //等待读取的结束地址
	uint32_t u32MaxSize;  //最大空间
	bool  FullFlag;       //数据池存满标志
	uint8_t  *pool;       //数据池的数据缓冲指针
} DataPool;
#pragma pack()

struct test_device {
    struct cdev cdev; //contain_of在调试的时候发现该元素为指针没能正确的找到结构体位置
    struct semaphore write_sem;

	spinlock_t read_timer_lock;
	int read_timeout_done;
    struct timer_list read_timer;
    
	char *tmp_buffer;
    ssize_t tmp_buffer_size;
	DataPool *pDataPool;

    wait_queue_head_t read_queue;
    dev_t dev_no;
    int major;
};

struct test_device *device = NULL;

DataPool *DataPoolInit(uint32_t size) 
{
#if USE_MALLOC
	DataPool *ldp = (DataPool *)kmalloc(sizeof(DataPool), GFP_KERNEL);
	if ( ldp == NULL ) {
		printk("DataPoolInit kmalloc failed ldp\r\n");
		return NULL;
	}
#else   
	DataPool *ldp = (DataPool *)gDataPoolBuffer;
#endif
	memset(ldp, 0, sizeof(DataPool));

#if USE_MALLOC
	ldp->pool = (uint8_t *)kmalloc(sizeof(uint8_t) * size, GFP_KERNEL);
	if ( ldp->pool == NULL ) {
		printk("DataPoolInit kmalloc failed ldp->pool\r\n");
		kfree(ldp);
		return NULL;
	}
#else
	//printf("sizeof(datapool)=%d\r\n", sizeof(DataPool));
	ldp->pool = (uint8_t *)(gDataPoolBuffer + sizeof(DataPool));
	size -= sizeof(DataPool);
#endif

	memset(ldp->pool, 0, size * sizeof(uint8_t));

	ldp->u32End = 0;
	ldp->u32Start = 0;
	ldp->u32MaxSize = size;
	ldp->FullFlag = false;

	return ldp;
}

void DataPollReInit(DataPool *ldp)
{
	memset(ldp->pool, 0, ldp->u32MaxSize * sizeof(uint8_t));
	ldp->u32End = 0;
	ldp->u32Start = 0;
	ldp->FullFlag = false;
#if USE_MALLOC
	kfree(ldp->pool);
	kfree(ldp);
	ldp->pool = NULL;
	ldp = NULL;
#endif
}

/*
   *   将数据写入到数据池中
   */
bool DataPoolWrite(DataPool *ldp, uint8_t *buf, uint32_t len)
{
	uint32_t i;
	//如果end在start之前
	if ( ldp->u32End < ldp->u32Start ) {
		//如果剩余的空间不能容纳想要写入的数据长度
		if (ldp->u32End + len > ldp->u32Start ) 
			return false;
	} else if ( ldp->u32End > ldp->u32Start ) {
		//如果end在start之后
		if ( ldp->u32End + len > ldp->u32MaxSize ) {
			//如果end加上待写入的数据长度后超出最大范围
			if ( len - (ldp->u32MaxSize - ldp->u32End) > ldp->u32Start ) {
				//如果数据长度超过了start的位置，则失败
				return false;
			}
		}
	} else {
		if (ldp->FullFlag)
			return false;
	}

	for (i = 0; i < len; i++ ) {
		ldp->pool[ldp->u32End++] = buf[i];
		if ( ldp->u32End == ldp->u32MaxSize ) {
			//到达缓冲池的末尾
			ldp->u32End = 0;
		}
		if ( ldp->u32End == ldp->u32Start ) {
			//数据已满，不能存入新的数据,返回已经存入的数据长度
			ldp->FullFlag = true;
			break;
		}
	}

	return true;
}

/*
   *   从数据池中取出多个数据
   *   return: 返回取出的数据长度,0表示出错
   */
uint32_t DataPoolRead(DataPool *ldp, uint8_t *buf, uint32_t len)
{
	uint32_t u32End = ldp->u32End;
	uint32_t i;

	if ((ldp->u32Start == u32End) && (ldp->FullFlag == false)) {
		return 0;
	} else if (ldp->u32Start < u32End) {
		//正常的写入数据，此刻start在end之前
		if (u32End - ldp->u32Start < len) { 
			//数据不满足长度
			return 0;
		}
	} else if (ldp->u32Start > u32End) {
		if ( ldp->u32MaxSize - ldp->u32Start + u32End < len ) {
			//数据长度不够
			return 0;
		}
	}

	for(i = 0; i < len; i++) {
		buf[i] = ldp->pool[ldp->u32Start++];

		if(ldp->u32Start >= ldp->u32MaxSize)
			ldp->u32Start = 0;

		if(ldp->u32Start == ldp->u32End) {
			i++;
			break;
		}
	}

	if(ldp->FullFlag == true)
		ldp->FullFlag = false;

	return i;
}

uint32_t GetDataPoolDataLen(DataPool *ldp)
{
	if (ldp->u32Start >= ldp->u32MaxSize || ldp->u32End >= ldp->u32MaxSize) {
		ldp->u32Start = 0;
		ldp->u32End = 0;
		ldp->FullFlag = false;
		//printf("===>GetDataPoolDataLen find error\r\n");
		return 0;
	}

	if (ldp->u32End > ldp->u32Start)
		return ldp->u32End - ldp->u32Start;
	else if (ldp->u32End < ldp->u32Start)
		return ldp->u32MaxSize - ldp->u32Start + ldp->u32End;
	else if (ldp->FullFlag) {
		return ldp->u32MaxSize;
	} else {
		return 0;
	}
}


int test_open(struct inode *inode, struct file *filp)
{
    struct test_device *dev; /* device information */ 
    dev = container_of(inode->i_cdev, struct test_device, cdev);
    filp->private_data = dev; /* for other methods */

#ifdef ONLY_ONE_USER
    if (!atomic_dec_and_test(&test_s_available)) {
        printk("device busy\r\n");
        atomic_inc(&test_s_available);
        return -EBUSY;
    }
#endif

    printk("open test module\r\n");
    //printk("device:cdev=%08x\r\n", &dev->cdev);
    //printk("device:sem =%08x\r\n", &dev->sem);
    //printk("device:buffer=%08x\r\n", dev->buffer);
    //printk("device:buffer_size=%dx\r\n", dev->buffer_size);
    //printk("device:dev_no=%08x\r\n", dev->dev_no);
    //printk("device:major=%d\r\n", dev->major);
    //printk("device:name=%s\r\n", dev->device_name);
    return 0;
}

int test_close(struct inode *inode, struct file *filp)
{
    struct test_device *dev; /* device information */ 
    dev = container_of(inode->i_cdev, struct test_device, cdev);
#ifdef ONLY_ONE_USER
    atomic_inc(&test_s_available);
#endif
    printk("close test module\r\n");
    return 0;
}

#if 0
loff_t test_llseek(struct file *filp, loff_t offset, int flag)
{
    //struct test_device *dev = filp->private_data;
    loff_t newpos;
    ssize_t ret = -ENOMEM;

    switch(flag) {
        case SEEK_END:
            //if (sizeof(kernel_buff) + offset < 0) {
            /*if (dev->buffer_size + offset < 0) {
                ret = -EFAULT;
                goto out;
            } else {
                //newpos = sizeof(kernel_buff) + offset;            
                newpos = dev->buffer_size + offset;            
            }*/
            break;
        case SEEK_SET:
            //if (offset > sizeof(kernel_buff)) {
            /*if (offset > dev->buffer_size) {
                ret = -EFAULT;
                goto out;
            } else {
                newpos = offset;
            }*/
            break;
        case SEEK_CUR:
            /*if (filp->f_pos + offset > dev->buffer_size) {
                ret = -EFAULT;
                goto out;
            } else {
                newpos = filp->f_pos + offset;
            }*/
            break;
        default:
            goto out;
            break;
    }

    //printk("set pos = %d\r\n", (int)newpos);

    filp->f_pos = newpos;
    ret = newpos;

out:
    //printk("offset to big1111, pos=%d, offset=%d\r\n", (int)(filp->f_pos), (int)offset);
    return ret;
}
#endif

static unsigned int test_poll_wait(struct file *filp, poll_table *wait)
{
    struct test_device *dev = filp->private_data;
    unsigned int mask = 0;

	if (dev == NULL)
		return -EBADFD;

	//添加读等待队列
    poll_wait(filp, &dev->read_queue, wait);

	//如果满足条件，则设置可读标志,select会通过该标志位来确认文件描述符是否能够读取
	if (GetDataPoolDataLen(dev->pDataPool) != 0 || dev->read_timeout_done == 1) {
        mask |= POLLIN | POLLRDNORM;
	}

    return mask;
}

ssize_t test_read(struct file *filp, char __user *buff, size_t count, loff_t *offset)
{
    struct test_device *dev = filp->private_data;
	ssize_t len;
	ssize_t rlen; 
	ssize_t pos = 0;
	int ret;

	len = GetDataPoolDataLen(dev->pDataPool);
	printk("len=%d\r\n", len);
	if (len == 0) {
        if (filp->f_flags & O_NONBLOCK) 
            return -EAGAIN;
        //如果设备已经没有可读的消息，则进入等待，第二个参数必须为真才能从等待中唤醒
		do {
			ret = wait_event_interruptible_timeout(dev->read_queue, ((GetDataPoolDataLen(dev->pDataPool) > 0) || (dev->read_timeout_done == 1)), 1*HZ);
			if (ret == -ERESTARTSYS)
				return -ERESTARTSYS;
		} while (ret == 0);
		len = GetDataPoolDataLen(dev->pDataPool);
		if (len == 0) {
			//如果数据依然是0，说明是超时导致的唤醒，需要清楚重新设备定时器和标志位，否则会无限进入可读
			dev->read_timeout_done = 0;
			mod_timer(&dev->read_timer, jiffies + timeout_value * HZ);
			return 0;
		}
    }

	printk("len=%d start=%d, end=%d\r\n", len, dev->pDataPool->u32Start, dev->pDataPool->u32End);
	while (1) 
	{
		memset(dev->tmp_buffer, 0, dev->tmp_buffer_size);
		if (count < dev->tmp_buffer_size) {
			rlen = count;	
		} else {
			if (len > dev->tmp_buffer_size) {
				rlen = dev->tmp_buffer_size;
			} else {
				rlen = len;
			}
		}
	
		DataPoolRead(dev->pDataPool, dev->tmp_buffer, rlen);

		printk("datapool read:pos=%ld, rlen=%ld, len=%ld\r\n", pos, rlen, len);
		if (copy_to_user(buff + pos, dev->tmp_buffer, rlen)) {
			printk("copy_to_user error\r\n");
			return -EFAULT;
		}

		len -= rlen;
		pos += rlen;
		//pos等于已经读取的总数
		if (pos >= count)
			break;
		if (len == 0)
			break;
	}

	dev->read_timeout_done = 0;
	mod_timer(&dev->read_timer, jiffies + timeout_value * HZ);
	printk("read end!\r\n");
    return pos;
}

ssize_t test_write(struct file *filp, const char __user *buff, size_t count, loff_t *offset)
{
    struct test_device *dev = filp->private_data;
    ssize_t ret = -ENOMEM;
	size_t len;
	size_t rlen;
	size_t pos = 0;

    //返回0则表示申请成功
    if (down_interruptible(&dev->write_sem)) {
        ret = -EBUSY;
        goto out_sem;
    }
	len = GetDataPoolDataLen(dev->pDataPool);
	//计算能够用来写入的空间大小
	len = dev->pDataPool->u32MaxSize - len;
	if (count > len) {
		ret = -EFAULT;
		printk("free size = %ld, you write size too big\r\n", len);
		goto out;
	}

	printk("write:free size:%ld\r\n", len);
	while (1) {
		memset(dev->tmp_buffer, 0, dev->tmp_buffer_size);
		if (count >= dev->tmp_buffer_size) {
			rlen = dev->tmp_buffer_size;		
		} else {
			rlen = count;	
		}
		if(copy_from_user(dev->tmp_buffer, buff + pos, rlen)) {
			ret = -EFAULT;
			goto out;
		}	
		printk("write to datapool:pos=%ld, rlen=%ld, count=%ld\r\n", pos, rlen, count);
		if (DataPoolWrite(dev->pDataPool, dev->tmp_buffer, rlen) == false) {
			ret = -EFAULT;
			goto out;
		} else {
			ret = 0;
		}

		count -= rlen;
		if (count == 0) 
			break;
		pos += rlen;
	}
	printk("start=%d end=%d\r\n", dev->pDataPool->u32Start, dev->pDataPool->u32End);
	wake_up_interruptible(&dev->read_queue);
out:
    up(&dev->write_sem);
out_sem:
    return ret;
}

#define TEST_IOC_MAGIC 'k'
#define TEST_IOC_MAXNR 14
#define TEST_GET_VERSION 1
#define TEST_SET_VERSION 2

static long test_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

    //printk("ioctl type = %c\r\n",  _IOC_TYPE(cmd));
    if ( _IOC_TYPE(cmd) != TEST_IOC_MAGIC) {
        err = -ENOTTY;
        printk("ioctl type error\t\n");
        goto out;
    }

    printk("ioctl nr = %d\r\n",  _IOC_NR(cmd));
    if ( _IOC_NR(cmd) > TEST_IOC_MAXNR) {
        err = -ENOTTY;
        printk("ioctl nr error\t\n");
        goto out;
    }

    if ( _IOC_DIR(cmd) & _IOC_READ) {
        printk("check read\r\n");
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    } else if ( _IOC_DIR(cmd) & _IOC_WRITE) {
		printk("check write\r\n");
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if (err) {
        printk("ioctl asscess error\t\n");
        return -EFAULT;
    }

    switch(cmd) 
    {
        case _IOW(TEST_IOC_MAGIC, TEST_SET_VERSION, int):
            err = __get_user(version, (int __user *)arg);
            printk("set version:%d\r\n", version);
            break;
        case _IOW(TEST_IOC_MAGIC, TEST_GET_VERSION, int):
            err = __put_user(version, (int __user *)arg);
            printk("get version:%d\r\n", version);
            break;
        default:
            err = -EINVAL;
            break;
    }

out:
    return err;
}

struct file_operations test_fops = {
    .owner = THIS_MODULE,
    .open = test_open,
    .release = test_close,
    .write = test_write,
    .read = test_read,
	//.llseek = test_llseek,
    .unlocked_ioctl = test_ioctl,
    .poll = test_poll_wait,
};

struct timer_data {
    struct test_device *dev;
    int paramter;
};

void timer_handler(unsigned long arg)
{
    //unsigned long j = jiffies;
    struct timer_data *dat = (struct timer_data *)arg;
    struct test_device *dev = dat->dev;
	unsigned long flags;

	spin_lock_irqsave(&dev->read_timer_lock, flags);
	dev->read_timeout_done = 1;
	wake_up_interruptible(&dev->read_queue);
	spin_unlock_irqrestore(&dev->read_timer_lock, flags);

    /*printk("timer_handler\r\n");
    dev->read_timer.expires += 1000;
    add_timer(&dev->read_timer);
    tasklet_schedule(&dev->tasklet); 
	*/
}

/*void tasklet_handler(unsigned long arg)
{
    //struct timer_data *dat  = (struct timer_data *)arg;
    //struct test_device *dev = dat->dev;

    printk("tasklet_handler\r\n");
}*/

struct class *myclass = NULL;

static int __init hello_init(void)
{
    int err;
    struct timer_data *timerdata;
    struct timer_data *tasklet_data;
	
    printk("module init\r\n");

    device = kmalloc(sizeof(struct test_device), GFP_KERNEL);
    if (device == NULL) {
        err = -ENOMEM;
        goto out1;
    }
    memset(device, 0, sizeof(struct test_device));

    device->tmp_buffer_size = 32;
    device->tmp_buffer = kmalloc(device->tmp_buffer_size * sizeof(uint8_t), GFP_KERNEL);
    if (device->tmp_buffer == NULL) {
        err = -ENOMEM;
        goto out2;
    }

	device->pDataPool = DataPoolInit(BUFFER_SIZE);
	if (device->pDataPool == NULL) {
		err = -ENOMEM;
		goto out2;
	}

    device->major = MAJOR_MODE;
	spin_lock_init(&device->read_timer_lock);
    sema_init(&device->write_sem, 1);
    init_waitqueue_head(&device->read_queue);
    init_timer(&device->read_timer);
    //注册设备编号
    if (device->major == 0) {
        //动态申请
        err = alloc_chrdev_region(&device->dev_no, 0, 1, DEVICE_NAME);
        device->major = MAJOR(device->dev_no);
        //request_chrdev_region(devno, 1);
    } else {
        //静态申请
        //int major = MAJOR(siMajorDevno);
        //int minor = MINOR(siMajorDevno);
        //device->dev_no = MKDEV(device->major, device->minor);
        device->dev_no = MKDEV(device->major, 0);
        err = register_chrdev_region(device->dev_no, 1, DEVICE_NAME);
    }
    
    if (err < 0) {
        printk("can't get major %d\r\n", device->major);
        goto out3;
    }

    myclass = class_create(THIS_MODULE, "aTest" );

    //分配cdev空间,因为我在结构体中使用的静态cdev所以不需要动态申请内存
#if 0
    device->cdev = cdev_alloc();
    if (device->cdev == NULL) {
        printk("cdev_alloc failed\r\n");
        err = -ENOMEM;
        goto out_cdev_alloc;
    }
    printk("申请cdev空间\r\n");
#endif
	cdev_init(&device->cdev, &test_fops);
    err = cdev_add(&device->cdev, device->dev_no, 1);
    if (err) {
        printk("cdev_add error\r\n");
        //return err;
        goto out_cdev_add;
    }

    device_create(myclass, NULL, device->dev_no, NULL, "aTest");

    timerdata = kmalloc(sizeof(struct timer_data), GFP_KERNEL);
    if (timerdata == NULL) {
        err = -ENOMEM;
        goto out_cdev_add;
    }

    device->read_timer.expires = jiffies + 1000;
    device->read_timer.function = timer_handler;
    
    timerdata->dev = device;
    timerdata->paramter = 1;
    device->read_timer.data = (unsigned long)timerdata;

    //add_timer(&device->read_timer);
	device->read_timeout_done = 0;
	mod_timer(&device->read_timer, jiffies + timeout_value * HZ);

    tasklet_data = kmalloc(sizeof(struct timer_data), GFP_KERNEL);
    if (tasklet_data == NULL) {
        err = -ENOMEM;
        goto err_timer;
    }

    tasklet_data->dev = device;
    tasklet_data->paramter = 2;
    //tasklet_init(&device->tasklet, tasklet_handler, (unsigned long)tasklet_data);

    printk("success\r\n");
    return err;

err_timer:
    kfree(timerdata);
out_cdev_add:
    unregister_chrdev_region(device->dev_no,  1);
out3:
out2:
    kfree(device->tmp_buffer);
out1:
    kfree(device);

	return err;
}

static void __exit hello_exit(void)
{
    printk("module exit\r\n");
    //反注册设备编号
	device_destroy(myclass, MKDEV(device->major, 0));
    del_timer_sync(&device->read_timer);
    cdev_del(&device->cdev);
	class_destroy(myclass);
	unregister_chrdev_region(device->dev_no,  1);
    kfree(device->tmp_buffer);
    kfree(device);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");

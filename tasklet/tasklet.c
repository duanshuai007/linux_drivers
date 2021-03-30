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

//static char *whom = "world";
//static int howmany = 1;
//module_param(howmany, int, S_IRUGO);
//module_param(whom, charp, S_IRUGO);
//module_param_array(name, type, num, perm)

static int version = 0;

#ifdef ONLY_ONE_USER
static atomic_t test_s_available = ATOMIC_INIT(1);
#endif

struct test_device {
    struct cdev cdev; //contain_of在调试的时候发现该元素为指针没能正确的找到结构体位置
    struct semaphore sem;

    struct timer_list timer;
    //struct tasklet_struct tasklet;

    char *buffer;
    ssize_t buffer_size;

    ssize_t read_pos; //buffer中已经写入的数据位置
    ssize_t write_pos;

    wait_queue_head_t read_queue;

    dev_t dev_no;
    int major;
};

struct test_device *device = NULL;

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

loff_t test_llseek(struct file *filp, loff_t offset, int flag)
{
    struct test_device *dev = filp->private_data;

    loff_t newpos;
    
    ssize_t ret = -ENOMEM;

    switch(flag) {
        case SEEK_END:
            //if (sizeof(kernel_buff) + offset < 0) {
            if (dev->buffer_size + offset < 0) {
                ret = -EFAULT;
                goto out;
            } else {
                //newpos = sizeof(kernel_buff) + offset;            
                newpos = dev->buffer_size + offset;            
            }
            break;
        case SEEK_SET:
            //if (offset > sizeof(kernel_buff)) {
            if (offset > dev->buffer_size) {
                ret = -EFAULT;
                goto out;
            } else {
                newpos = offset;
            }
            break;
        case SEEK_CUR:
            //if (filp->f_pos + offset > sizeof(kernel_buff)) {
            if (filp->f_pos + offset > dev->buffer_size) {
                ret = -EFAULT;
                goto out;
            } else {
                newpos = filp->f_pos + offset;
            }
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

static unsigned int test_poll_wait(struct file *filp, poll_table *wait)
{
    struct test_device *dev = filp->private_data;
    unsigned int mask = 0;

    poll_wait(filp, &dev->read_queue, wait);

    if (dev->read_pos != dev->write_pos) {
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

ssize_t test_read(struct file *filp, char __user *buff, size_t count, loff_t *offset)
{
    struct test_device *dev = filp->private_data;
    ssize_t ret = -ENOMEM;

    if (*offset > dev->write_pos) {
		ret = -EINVAL;
        goto out;
    }
    
    if (count + *offset > dev->write_pos) {
        count = dev->write_pos - *offset;
    }

    if (dev->write_pos == dev->read_pos) {
        if (filp->f_flags & O_NONBLOCK) 
            return -EAGAIN;

        //如果设备已经没有可读的消息，则进入等待，第二个参数必须为真才能从等待中唤醒
        if (wait_event_interruptible(dev->read_queue, dev->write_pos != dev->read_pos))
            return -ERESTARTSYS;
		if (count + *offset > dev->write_pos) {
			count = dev->write_pos - *offset;
		}
    }

    printk("%s[%ld]:rpos:%ld, wpos:%ld, *offset=%lld, count=%ld\r\n", __func__, __LINE__, dev->read_pos, dev->write_pos, *offset, count);
    if (copy_to_user(buff, dev->buffer + *offset, count)) {
        ret = -EFAULT;
        printk("copy_to_user error\r\n");
        goto out;
    }

    *offset += count;
    ret = count;
    dev->read_pos = *offset;

out:
    return ret;
}

ssize_t test_write(struct file *filp, const char __user *buff, size_t count, loff_t *offset)
{
    struct test_device *dev = filp->private_data;
    ssize_t ret = -ENOMEM;
    
    //返回0则表示申请成功
    if (down_interruptible(&dev->sem)) {
        //ret = -ERESTART;
        ret = -EBUSY;
        goto out_sem;
    }

    if ( *offset >= dev->buffer_size)
        goto out;

    if ( *offset + count > dev->buffer_size) {
        //count = dev->buffer_size - *offset;
		goto out;
    }

    //printk("%s[%ld]:rpos:%ld, wpos:%ld, *offset=%lld, count=%ld\r\n", __func__, __LINE__, dev->read_pos, dev->write_pos, *offset, count);
    if(copy_from_user(dev->buffer + *offset, buff, count)) {
        ret = -EFAULT;
        goto out;
    }

    *offset += count;
    ret = count;
    if (*offset > dev->write_pos)
        dev->write_pos += count;
    //printk("rpos:%ld, wpos:%ld, *offset=%lld\r\n", dev->read_pos, dev->write_pos, *offset);
    wake_up_interruptible(&dev->read_queue);
out:
    up(&dev->sem);
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
        //printk("ioctl type error\t\n");
        goto out;
    }

    printk("ioctl nr = %d\r\n",  _IOC_NR(cmd));
    if ( _IOC_NR(cmd) > TEST_IOC_MAXNR) {
        err = -ENOTTY;
        //printk("ioctl nr error\t\n");
        goto out;
    }

    if ( _IOC_DIR(cmd) & _IOC_READ) {
        //printk("check read\r\n");
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    } else if ( _IOC_DIR(cmd) & _IOC_WRITE) {
       // printk("check write\r\n");
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if (err) {
        //printk("ioctl asscess error\t\n");
        return -EFAULT;
    }

    switch(cmd) 
    {
        case _IOW(TEST_IOC_MAGIC, TEST_SET_VERSION, int):
            err = __get_user(version, (int __user *)arg);
            //printk("set version:%d\r\n", version);
            break;
        case _IOW(TEST_IOC_MAGIC, TEST_GET_VERSION, int):
            err = __put_user(version, (int __user *)arg);
            //printk("get version:%d\r\n", version);
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
    .llseek = test_llseek,
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

    printk("timer_handler\r\n");
    dev->timer.expires += 1000;
    add_timer(&dev->timer);
    //tasklet_schedule(&dev->tasklet); 
}
#if 0
void tasklet_handler(unsigned long arg)
{
    //struct timer_data *dat  = (struct timer_data *)arg;
    //struct test_device *dev = dat->dev;

    printk("tasklet_handler\r\n");
}
#endif

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

    device->buffer_size = BUFFER_SIZE;
    device->buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (device->buffer == NULL) {
        err = -ENOMEM;
        goto out2;
    }
    memset(device->buffer, 0, device->buffer_size);

    device->major = MAJOR_MODE;
    sema_init(&device->sem, 1);
    init_waitqueue_head(&device->read_queue);
    init_timer(&device->timer);

    //注册设备编号
    if (device->major == 0) {
        //动态申请
        //printk("动态申请设备号\r\n");
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

    device->timer.expires = jiffies + 1000;
    device->timer.function = timer_handler;
    
    timerdata->dev = device;
    timerdata->paramter = 1;
    device->timer.data = (unsigned long)timerdata;

    add_timer(&device->timer);

#if 0
    tasklet_data = kmalloc(sizeof(struct timer_data), GFP_KERNEL);
    if (tasklet_data == NULL) {
        err = -ENOMEM;
        goto err_timer;
    }

    tasklet_data->dev = device;
    tasklet_data->paramter = 2;
    tasklet_init(&device->tasklet, tasklet_handler, (unsigned long)tasklet_data);
#endif
    //printk("success\r\n");
    return err;

err_timer:
    kfree(timerdata);
out_cdev_add:
    unregister_chrdev_region(device->dev_no,  1);
out3:
out2:
    kfree(device->buffer);
out1:
    kfree(device);

	return err;
}

static void __exit hello_exit(void)
{
    //printk(KERN_INFO "Goodbye, cruel world\n");
    printk("module exit\r\n");
    //反注册设备编号
    //devno = MKDEV(device_major, device_minor);
    //del_timer(&device->timer);
	device_destroy(myclass, MKDEV(device->major, 0));
    del_timer_sync(&device->timer);
    cdev_del(&device->cdev);
	class_destroy(myclass);
	unregister_chrdev_region(device->dev_no,  1);
    //kfree(&device->cdev);

    kfree(device->buffer);
    kfree(device);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");

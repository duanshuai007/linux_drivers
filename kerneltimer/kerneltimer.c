#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/sched.h>


#define DEBUG 0

#if DEBUG
#define PRINTK(format, ...) PRINTK(format, ##__VA_ARGS__)
#else
#define PRINTK(format, ...)
#endif
int kerneltimer_major = -1;

struct second_dev
{
    struct cdev * cdev;
    struct class * class;
    wait_queue_head_t queue;
    struct semaphore sem;
    atomic_t count;
    int update_flag;
    struct timer_list s_timer;
};

static struct second_dev kerneltimer;

static void kerneltimer_handle(unsigned long arg)
{
    //修改定时器的expires,在新的被传入的expires到来后才会执行定时器函数
    mod_timer(&kerneltimer.s_timer, jiffies + HZ);
    atomic_inc(&kerneltimer.count);
    kerneltimer.update_flag = 1;
    wake_up_interruptible(&kerneltimer.queue);
    PRINTK(KERN_NOTICE "current jiffies is %ld,HZ:%d,arg:%ld,expires:%ld\r\n",
            jiffies, HZ, arg, kerneltimer.s_timer.expires);
}

static int kerneltimer_open(struct inode * inode, struct file * filp)
{
    if(down_trylock(&kerneltimer.sem))
    {
        PRINTK("kernel: ktimer already open\r\n");
        return -EBUSY;
    }
    PRINTK("kernel:kerneltimer open\r\n");
    //初始化一个定时器，function是定时器期满后执行到函数
    //expires是定时器到期时间
    init_timer(&kerneltimer.s_timer);
    kerneltimer.s_timer.function = &kerneltimer_handle;
    kerneltimer.s_timer.expires = jiffies + HZ;
    //将定时器加入到内核动态定时器链表中。
    add_timer(&kerneltimer.s_timer);
    //count初始化为0
    atomic_set(&kerneltimer.count, 0);
    return 0;
}

static int kerneltimer_release(struct inode * inode, struct file * filp)
{
    up(&kerneltimer.sem);
    PRINTK("kernel:kerneltimer release\r\n");
    //删除定时器，如果编译内核不支持SMP，这del_timer于del_timer_sync等价
    del_timer_sync(&kerneltimer.s_timer);
    return 0;
}

static ssize_t kerneltimer_read(
        struct file * filp,
        char __user * buffer,
        size_t count,
        loff_t *ppos
        )
{
    int counter;
    int ret;
    if(kerneltimer.update_flag == 0)
    {
        if(filp->f_flags & O_NONBLOCK)
        {
            PRINTK("kernel:no block read\r\n");
            return -EAGAIN;
        }else{
            PRINTK("kernel:block read\r\n");
            wait_event_interruptible(kerneltimer.queue,
                    kerneltimer.update_flag);
        }
    }
    kerneltimer.update_flag = 0;
    counter = atomic_read(&kerneltimer.count);
    if(put_user(counter, (int *)buffer))
        return -EFAULT;
    else
        return sizeof(unsigned int);
}

static struct file_operations kerneltimer_ops = {
    .owner      = THIS_MODULE,
    .open       = kerneltimer_open,
    .release    = kerneltimer_release,
    .read       = kerneltimer_read,
};

static int __init KT_driver_init(void)
{
    int ret;
    dev_t devno;

    PRINTK("kernel:matck ok!\r\n");
    if(kerneltimer_major > 0)
    {
        PRINTK("kernel:static register\r\n");
        devno = MKDEV(kerneltimer_major, 0);
        ret = register_chrdev_region(devno, 1, "kerneltimer");
    }else{
        PRINTK("kernel:alloc register\r\n");
        ret = alloc_chrdev_region(&devno, 0, 1, "kerneltimer");
        kerneltimer_major = MAJOR(devno);
    }
    if(ret < 0)
    {
        PRINTK("chrdev region failed \r\b");
        return ret;
    }
    PRINTK("kernel:major:%d\r\n", kerneltimer_major);

    kerneltimer.class = class_create(THIS_MODULE, "kerneltimer");
    if(kerneltimer.class == NULL)
    {
        PRINTK("kernel:class_create failed\r\n");
        return -ENOENT;
    }
    kerneltimer.cdev = cdev_alloc();
    if(kerneltimer.cdev == NULL)
    {
        PRINTK("kernel:cdev_alloc failed\r\n");
        return -ENOENT;
    }
    cdev_init(kerneltimer.cdev, &kerneltimer_ops);
    ret = cdev_add(kerneltimer.cdev, devno, 1);
    if(ret)
    {
        PRINTK("kernel:timer cdev_add failed\r\n");
        return ret;
    }
    device_create(kerneltimer.class, NULL, devno, NULL, "kerneltimer");

    init_waitqueue_head(&kerneltimer.queue);
    sema_init(&kerneltimer.sem, 1);

    return 0;
}

static void __exit KT_driver_exit(void)
{
    PRINTK("kernel:KT driver exit\r\n");
    device_destroy(kerneltimer.class, MKDEV(kerneltimer_major, 0));
    cdev_del(kerneltimer.cdev);
    class_destroy(kerneltimer.class);
    unregister_chrdev_region(MKDEV(kerneltimer_major, 0), 1);
    return 0;
}

module_init(KT_driver_init);
module_exit(KT_driver_exit);

MODULE_AUTHOR("not me");
MODULE_LICENSE("GPL");


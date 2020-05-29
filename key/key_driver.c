#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mach/irqs.h>
#include <linux/irq.h>
#include <mach/regs-gpio.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <asm/irq.h>

#define DEBUG 0

#if DEBUG
#define PRINTK(format, ...) printk(format, ##__VAGS__)
#else
#define PRINTK(format, ...)
#endif

#define KEYSTATUS_DOWNX 1
#define KEYSTATUS_DOWN  2
#define KEYSTATUS_UP    3

#define NUM_RESOURCE 6

#define S3C2440_GPG_BASE 0X56000060
#define KEY1    0
#define KEY2    3
#define KEY3    5
#define KEY4    6
#define KEY5    7
#define KEY6    11

const unsigned int key_offset[6] = {KEY1, KEY2, KEY3, KEY4, KEY5, KEY6};
/*主设备号*/
int dev_major = -1;
/*中断结构体定义*/
struct irqs{
    int pirqs[NUM_RESOURCE];
    char * names[NUM_RESOURCE];
}irqs;
/*完成具体设备到结构体设计*/
struct s3c2440_button{
    struct cdev * cdev;
    struct class * myclass;
    struct semaphore sem;
    spinlock_t lock;
    wait_queue_head_t read_wait_queue;
    unsigned int keyStatus[NUM_RESOURCE];
    struct timer_list timer[NUM_RESOURCE];
    int bepressed;
};

int timer_open_flags[NUM_RESOURCE];

static struct s3c2440_button s3c2440_button;
static void __iomem * s3c_gpiog_base;
static struct resource * s3c_gpiog_mem;

#define ISKEY_UP(x) (readw(s3c_gpiog_base+0x04)&(1<<key_offset[x]))

static void key_timer_handler(unsigned long data)
{
    int key = data;
    PRINTK("kernel:time over\r\n");
    if(!ISKEY_UP(key))
    {
        PRINTK("kernel:timer key down, key:%d\r\n", key);
        if(s3c2440_button.keyStatus[key] == KEYSTATUS_DOWNX)
        {
            s3c2440_button.keyStatus[key] = KEYSTATUS_DOWN;
            mod_timer(&s3c2440_button.timer[key], jiffies + 4);
        }else{
            mod_timer(&s3c2440_button.timer[key], jiffies + 4);
        }
    }else{
        PRINTK("kernel:timer key up, key:%d", key);
        if(s3c2440_button.keyStatus[key] == KEYSTATUS_DOWN)
        {   //检测到有效跳变
            PRINTK("\t is youxiao\r\n");
            s3c2440_button.bepressed = key+1;
            wake_up_interruptible(&s3c2440_button.read_wait_queue);
        }else
        {   //非有效跳变，需要滤除
            PRINTK("\t is error\r\n");
            s3c2440_button.bepressed = 0;
        }
        s3c2440_button.keyStatus[key] = KEYSTATUS_UP;
        del_timer(&s3c2440_button.timer[key]);
        timer_open_flags[key] = 0;
        enable_irq(irqs.pirqs[key]);
    }
}

static irqreturn_t s3c2440_button_interrupt_handler(
        int irq,
        void * dev_id
        )
{
    int key = dev_id;
    PRINTK("kernel:enter interrupt,key:%d\r\n", key);
    //在中断中用disable_irq会导致阻塞
    if(timer_open_flags[key] == 0)
    {
        disable_irq_nosync(irqs.pirqs[key]);
        s3c2440_button.keyStatus[key] = KEYSTATUS_DOWNX;
        s3c2440_button.timer[key].expires = jiffies + 4;
        timer_open_flags[key] = 1;
        add_timer(&s3c2440_button.timer[key]);
    }
    PRINTK("kernel:exit interrupt, key:%d\r\n", key);
    return IRQ_RETVAL(IRQ_HANDLED);
}

static int s3c2440_button_open(struct inode * inode, struct file * filp)
{
    int i = 0, ret = 0;
    PRINTK("kernel:button open \r\n");
    filp->private_data = &s3c2440_button;
    s3c_gpiog_mem = request_mem_region(S3C2440_GPG_BASE, 0X10,
            "s3c_gpiog");
    if(s3c_gpiog_mem == NULL)
    {
        PRINTK("kernel:gpiog mem busy\r\n");
        return -ENOENT;
    }
    s3c_gpiog_base = ioremap(S3C2440_GPG_BASE, 0x10);
    if(s3c_gpiog_base == NULL)
    {
        PRINTK("kernel:failed to reserver memory region\r\n");
        return -ENOENT;
    }
    for(i=0;i<NUM_RESOURCE;i++)
    {   //1.要申请的硬件中断号，2.向系统注册到中断处理函数
        //3.中断处理的属性，4.中断的名称，可以在cat /proc/interrupts中看到
        //5.在中断共享时会用,作为参数传入中断程序中
        timer_open_flags[i] = 0;
        setup_timer(&s3c2440_button.timer[i], key_timer_handler, i);
        ret = request_irq(irqs.pirqs[i],
                s3c2440_button_interrupt_handler,
                IRQ_TYPE_EDGE_FALLING, irqs.names[i],
                i);
        if(ret)
        {
            if(ret == -EINVAL)
            {
                //中断号无效或处理函数指针为NULL
            }
            if(ret == -EBUSY)
            {
                //中断已经被占用且不能共享
            }
            break;
        }
    }
    if(ret)
    {
        i--;
        for(;i>=0;--i)
        {
            disable_irq(irqs.pirqs[i]);
            free_irq(irqs.pirqs[i], i);
        }
        return -EBUSY;
    }
    PRINTK("kernel:button open success\r\n");
    return 0;
}

static int s3c2440_button_close(struct inode * inode, struct file * filp)
{
    int i = 0;
    PRINTK("disable irq\r\n");
    for(i=0;i < NUM_RESOURCE;i++)
    {
        disable_irq_nosync(irqs.pirqs[i]);
        free_irq(irqs.pirqs[i], i);
    }
    iounmap(s3c_gpiog_base);
    release_mem_region(S3C2440_GPG_BASE, 0X10);
    return 0;
}

static unsigned long s3c2440_button_read(
        struct file * filp,
        char __user * buff,
        size_t count,
        loff_t offp
        )
{
    struct s3c2440_button * dev = filp->private_data;

    unsigned long err;

    if(!dev->bepressed)
    {
        if(filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        else
        {
            PRINTK("wair for key...\r\n");
            wait_event_interruptible(dev->read_wait_queue, dev->bepressed);
        }
    }
    PRINTK("key read...\r\n");
    err = copy_to_user(buff,
            &s3c2440_button.bepressed,
            count);

    spin_lock(&(dev->lock));
    dev->bepressed = 0;
    spin_unlock(&(dev->lock));

    return err?-EFAULT:count;
}

static unsigned int s3c2440_button_poll(
        struct file * filp,
        struct poll_table_struct * wait
        )
{
    struct s3c2440_button * dev = filp->private_data;
    unsigned int mask = 0;
    PRINTK("poll wait for key...\r\n");
    poll_wait(filp, &(dev->read_wait_queue), wait);
    if(dev->bepressed)
    {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

static const struct file_operations s3c2440_fops = {
    .owner      = THIS_MODULE,
    .open       = s3c2440_button_open,
    .release    = s3c2440_button_close,
    .read       = s3c2440_button_read,
    .poll       = s3c2440_button_poll,
};

static int s3c2440_button_probe(struct platform_device * dev)
{
    PRINTK("kernel:key match ok\r\n");
    struct resource * irq_resource;
    struct platform_device * pdev = dev;
    int i = 0;
    int ret = 0;
    /*1、设备号申请*/
    dev_t devno;
    if(dev_major > 0)
    {   //静态申请
        devno = MKDEV(dev_major, 0);
        ret = register_chrdev_region(devno, 1, "s3c2440_button");
    }
    else
    {   //动态申请
        ret = alloc_chrdev_region(&devno, 0, 1, "s3c2440_button");
        dev_major = MAJOR(devno);
    }
    if(ret < 0)
    {
        PRINTK("kernel:chrdev region failed\r\n");
        return ret;
    }
    //完成设备类的创建，主要实现设备文件到自动创建
    s3c2440_button.myclass = class_create(THIS_MODULE,
            "s3c2440_button_class");
    /*2、完成字符设备到加载*/
    s3c2440_button.cdev = cdev_alloc();
    cdev_init(s3c2440_button.cdev, &s3c2440_fops);
    ret = cdev_add(s3c2440_button.cdev, devno, 1);
    if(ret)
    {
        PRINTK("add device error\n");
        return ret;
    }
    /*初始化自旋锁*/
    spin_lock_init(&(s3c2440_button.lock));
    sema_init(&s3c2440_button.sem, 1);
    /*初始化等待队列*/
    init_waitqueue_head(&(s3c2440_button.read_wait_queue));
    /*设备到创建，实现设备文件的自动创建*/
    device_create(s3c2440_button.myclass, NULL,
            devno, NULL, "s3c2440_button");

    /*3、获得资源*/
    for(;i < NUM_RESOURCE; i++)
    {
        irq_resource = platform_get_resource(pdev, IORESOURCE_IRQ, i);
        if(irq_resource == NULL)
        {
            PRINTK("kernel:get resource failed\r\n");
            return -ENOENT;
        }
        irqs.pirqs[i] = irq_resource->start;
        irqs.names[i] = irq_resource->name;
    }
    return 0;
}

static int s3c2440_button_remove(struct platform_device * dev)
{
    PRINTK("kernel:removed driver from platform bus\r\n");
    device_destroy(s3c2440_button.myclass, MKDEV(dev_major, 0));
    cdev_del(s3c2440_button.cdev);
    class_destroy(s3c2440_button.myclass);
    unregister_chrdev_region(MKDEV(dev_major, 0), 1);
    return 0;
}

static const struct platform_driver s3c2440_button_driver = {
    .probe      = s3c2440_button_probe,
    .remove     = s3c2440_button_remove,
    .driver     =
    {
        .owner  = THIS_MODULE,
        .name   = "s3c2440_button",
    },
};

static int __init platform_driver_init(void)
{
    int ret;
    PRINTK("kernel:driver init\r\n");
    ret = platform_driver_register(&s3c2440_button_driver);
    if(ret)
    {
        PRINTK("button register failed\r\n");
        return ret;
    }
    return 0;
}

static void __exit platform_driver_exit(void)
{
    PRINTK("kernel:driver exit\r\n");
    platform_driver_unregister(&s3c2440_button_driver);
}

module_init(platform_driver_init);
module_exit(platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NOT ME");


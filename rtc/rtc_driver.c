#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <mach/irqs.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>

#include <linux/rtc.h>
#include <mach/hardware.h>
#include <plat/regs-rtc.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/delay.h>

#define DEBUG 1

#if DEBUG
#define PRINTK(format, ...) printk(format, ##__VA_ARGS__)
#else
#define PRINTK(format, ...)
#endif

struct rtc{
    int rtc;
    int rtc_irq;
    int tick_irq;
    char * rtc_name;
    char * irq_name;
    char * tick_irq_name;
}rtc;

struct s3c2440_rtc{
    struct cdev * mycdev;
    struct semaphore mysem;
    int openflag;
    int timeflag;
};

static struct s3c2440_rtc s3c2440_rtc;
static struct clk * rtc_clk;
static void __iomem * s3c_rtc_base;
static struct resource * s3c_rtc_mem;

#define SRCPND_ADDR 0X4A000000

static irqreturn_t s3c2440_rtc_interrupt_handler(
        int irq,
        void * devid
        )
{
    struct s3c2440_rtc * dev = devid;
    PRINTK("kernel:enter rtc irq...\r\n");
    if(irq == rtc.rtc_irq)
    {
        PRINTK("kernel:rtc interrupt\r\n");
    }
    return IRQ_HANDLED;
}

static irqreturn_t s3c2440_rtc_tick_handler(
        int irq,
        void * devid
        )
{
    struct s3c2440_rtc * dev = devid;
    PRINTK("kernel:enter tick irq...\r\n");
    if(irq == rtc.tick_irq)
    {
        PRINTK("kernel:rtc interrupt\r\n");
    }
    return IRQ_HANDLED;
}

static int s3c2440_rtc_open(
        struct inode * inode,
        struct file * filp
        )
{
    int ret;
    void __iomem * base = s3c_rtc_base;

    filp->private_data = &s3c2440_rtc;
    if(down_trylock(&s3c2440_rtc.mysem))
    {
        PRINTK("kernel:rtc already open\r\n");
        return -EBUSY;
    }
    PRINTK("kernel:rtc open\r\n");

    rtc_clk = clk_get(NULL, "rtc");
    if(!rtc_clk)
    {
       PRINTK("kernel:get rtc clk failed\r\n");
       return -ENOENT;
    }
    clk_enable(rtc_clk);
    //寄存器配置

    writew(S3C2410_RTCCON_RTCEN, base + S3C2410_RTCCON);
    writeb(0x01, base + S3C2410_RTCSEC);
    writeb(0x02, base + S3C2410_RTCMIN);
    writeb(0x03, base + S3C2410_RTCHOUR);
    writeb(0x04, base + S3C2410_RTCDATE);
    writeb(0x05, base + S3C2410_RTCMON);
    writeb(0x06, base + S3C2410_RTCYEAR);
    clk_disable(rtc_clk);

    ret = request_irq(rtc.rtc_irq,
                &s3c2440_rtc_interrupt_handler,
                IRQF_DISABLED,
                rtc.irq_name,
                (void *)&s3c2440_rtc);
    if(ret)
    {
        return ret;
    }
    ret = request_irq( rtc.tick_irq,
            &s3c2440_rtc_tick_handler,
            IRQF_DISABLED,
            rtc.tick_irq_name,
            (void *)&s3c2440_rtc);
    if(ret)
    {
        return ret;
    }
    PRINTK("kernel:rtc open success\r\n");
    return 0;
}

static int s3c2440_rtc_release(
        struct inode * inode,
        struct file * filp
        )
{
    struct s3c2440_rtc * dev = filp->private_data;
    PRINTK("kernel:rtc release\r\n");

    iounmap(s3c_rtc_base);
    release_mem_region(rtc.rtc, 0xff);
    free_irq(rtc.rtc_irq, (void *)&s3c2440_rtc);
    free_irq(rtc.tick_irq, (void *)&s3c2440_rtc);
    up(&dev->mysem);
    PRINTK("kernel:rtc release success\r\n");
    return 0;
}

static ssize_t s3c2440_rtc_write(
        struct file * filp,
        const char __user * buffer,
        size_t count,
        loff_t *pos
        )
{
    struct s3c2440_rtc * dev = filp->private_data;

    return 0;
}

static ssize_t s3c2440_rtc_read(
        struct file * filp,
        char __user * buffer,
        size_t count,
        loff_t *pos
        )
{
    void __iomem * base = s3c_rtc_base;
    struct s3c2440_rtc * dev = filp->private_data;
    int year,mon,day,hour,min,sec;
    char str[20];
    int len, err;
    //PRINTK("kernel:rtc read\r\n");
    if(dev->timeflag == 0)
    {
        if(filp->f_flags & O_NONBLOCK)
        {
            PRINTK("kernel: no block\r\n");
            return -EAGAIN;
        }
        else{
            PRINTK("kernel:wait for queue...\r\n");
        }
    }

    clk_enable(rtc_clk);
    min = readb(base + S3C2410_RTCMIN);
    hour = readb(base + S3C2410_RTCHOUR);
    day = readb(base + S3C2410_RTCDATE);
    mon = readb(base + S3C2410_RTCMON);
    year = readb(base + S3C2410_RTCYEAR);
    sec = readb(base + S3C2410_RTCSEC);
    clk_disable(rtc_clk);

    len = sprintf(str, "%d-%d-%d %d:%d:%d",
            year, mon, day, hour, min, sec);
    PRINTK(str);
    PRINTK("\r\n");
    err = copy_to_user(buffer, str, len);
    //PRINTK("kernel:%d-%d %d:%d:%d\r\n", month,day,hour,minute,second);
    return err?-EINVAL:len;
}

static unsigned int s3c2440_rtc_poll(
        struct file * filp,
        struct poll_table_struct * wait
        )
{
    unsigned int mask;
    struct s3c2440_rtc * dev = filp->private_data;

    //poll_wait(filp, &dev->myqueue, wait);
    if(dev->timeflag)
    {
        mask |= POLLIN | POLLRDNORM;
    }
    return 0;
}

static struct file_operations s3c2440_rtc_ops = {
    .owner      = THIS_MODULE,
    .open       = s3c2440_rtc_open,
    .release    = s3c2440_rtc_release,
    .read       = s3c2440_rtc_read,
    .write      = s3c2440_rtc_write,
};

static struct miscdevice rtc_miscdev = {
    .minor      = RTC_MINOR,
    .name       = "s3c2440_rtc",
    .fops       = &s3c2440_rtc_ops,
};

static int s3c2440_rtc_probe(struct platform_device * dev)
{
    int ret;
    dev_t devno;
    struct resource * rtc_resource;

    PRINTK("kernel:rtc match ok\r\n");

    ret = misc_register(&rtc_miscdev);
    if(ret)
    {
        PRINTK("kernel:can't register rtc miscdevice\r\n");
        return ret;
    }
    devno = MKDEV(MISC_MAJOR, rtc_miscdev.minor);
    s3c2440_rtc.mycdev = cdev_alloc();
    cdev_init(s3c2440_rtc.mycdev, &s3c2440_rtc_ops);
    ret = cdev_add(s3c2440_rtc.mycdev, devno, 1);
    if(ret)
    {
        PRINTK("kernel:rtc cdev add failed\r\n");
        return ret;
    }
    rtc_clk = clk_get(&dev->dev, "rtc");
    clk_enable(rtc_clk);
    sema_init(&s3c2440_rtc.mysem, 1);
    rtc_resource = platform_get_resource(dev, IORESOURCE_MEM, 0);
    if(rtc_resource == NULL)
    {
        PRINTK("kernel:get resource failed\r\n");
         return -ENOENT;
    }
    rtc.rtc = rtc_resource->start;
    rtc.rtc_name = rtc_resource->name;

    s3c_rtc_mem = request_mem_region(rtc.rtc, 0xff, "rtc_mem");
    if(s3c_rtc_mem == NULL)
    {
        PRINTK("kernel: get rtc mem failed\r\n");
        return -ENOENT;
    }
    s3c_rtc_base = ioremap(rtc.rtc, 0xff);

    rtc_resource = platform_get_resource(dev, IORESOURCE_IRQ, 0);
    if(rtc_resource == NULL)
    {
        PRINTK("kernel:get resource irq failed\r\n");
         return -ENOENT;
    }
    rtc.rtc_irq = rtc_resource->start;
    rtc.irq_name = rtc_resource->name;
    rtc_resource = platform_get_resource(dev, IORESOURCE_IRQ, 1);
    if(rtc_resource == NULL)
    {
        PRINTK("kernel:get tick irq failed\r\n");
        return -ENOENT;
    }
    rtc.tick_irq = rtc_resource->start;
    rtc.tick_irq_name = rtc_resource->name;
    PRINTK("RTC:addr:0x%08x,name:%s\r\nirq:%d,irqname:%s\r\ntick:%d,tickname:%s\r\n",
            rtc.rtc, rtc.rtc_name,
            rtc.rtc_irq, rtc.irq_name,
            rtc.tick_irq, rtc.tick_irq_name);
    return 0;
}

static int s3c2440_rtc_remove(struct platform_device * pdev)
{
    cdev_del(s3c2440_rtc.mycdev);
    misc_deregister(&rtc_miscdev);
    return 0;
}

static struct platform_driver s3c2440_rtc_driver = {
    .probe      = s3c2440_rtc_probe,
    .remove     = s3c2440_rtc_remove,
    .driver     = {
        .owner = THIS_MODULE,
        .name = "s3c2440_rtc",
    },
};

static int __init platform_driver_init(void)
{
    int ret;
    PRINTK("kernel:rtc driver init\r\n");
    ret = platform_driver_register(&s3c2440_rtc_driver);
    if(ret)
    {
        platform_driver_unregister(&s3c2440_rtc_driver);
        return ret;
    }
    PRINTK("kernel:rtc driver init success\r\n");
    return 0;
}

static void __exit platform_driver_exit(void)
{
    PRINTK("kernel:rtc driver exit\r\n");
    platform_driver_unregister(&s3c2440_rtc_driver);
}

module_init(platform_driver_init);
module_exit(platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("not me");

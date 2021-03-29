#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <linux/gpio.h>
#include <linux/clk.h>

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/irqreturn.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#define LED_ON      0
#define LED_OFF     1
#define LED_FREQ    2

#define NUM_RESOURCE 4

#define CLEAR_CON_BIT(x)    (~(3<<((x)*2)))
#define SET_CON_BIT(x)      (1<<((x)*2))
#define SET_UP_BIT(x)       (1<<(x))

#define S3C2440_GPB_BASEADDR    0X56000010

#define S3C2440_TCFG0_ADDR      0X51000000
#define S3C2440_TCFG1_ADDR      0X51000004
#define S3C2440_TCON_ADDR       0X51000008
#define S3C2440_TCNTB0_ADDR     0X5100000C
#define S3C2440_TCMPB0_ADDR     0X51000010
#define S3C2440_TCNTO0_ADDR     0X51000014
#define S3C2440_TCNTB1_ADDR     0X51000018
#define S3C2440_TCMPB1_ADDR     0X5100001C
#define S3C2440_TCNTO1_ADDR     0X51000020
#define S3C2440_TCNTB2_ADDR     0X51000024
#define S3C2440_TCMPB2_ADDR     0X51000028
#define S3C2440_TCNTO2_ADDR     0X5100002C
#define S3C2440_TCNTB3_ADDR     0X51000030
#define S3C2440_TCMPB3_ADDR     0X51000034
#define S3C2440_TCNTB3_ADDR     0X51000038
//定时器4是linux系统时钟定时器，不能对其操作

volatile unsigned long virt;
volatile unsigned long *mGPBCON, *mGPBDAT, *mGPBUP;
volatile unsigned long time_virt;
volatile unsigned long *mTCFG0, *mTCFG1, *mTCON;
volatile unsigned long *mTCNTB0, *mTCMPB0, *mTCNTO0;

#define W_DAT_BIT(bit,val)  do{if(val)\
        {\
            *mGPBDAT |= (1<<bit);\
        }\
        else\
        {\
            *mGPBDAT &= (~(1<<bit));\
        }}while(0)

int dev_major = -1;

struct leds
{
    int pleds[NUM_RESOURCE];
    char * names[NUM_RESOURCE];
    int timer;
    char * timer_name;
}leds;

struct s3c2440_led
{
    struct cdev * mycdev;
    struct class * myclass;
    spinlock_t lock;
    wait_queue_head_t myqueue;
    int timer_openflag;
    int interrupt_flag;
    int openflag;
};

static struct s3c2440_led s3c2440_led;

static irqreturn_t s3c2440_timer_handler(
        int irq,
        void * devid
        )
{
    struct s3c2440_led * dev = devid;
    unsigned int tmp;
    disable_irq_nosync(leds.timer);
    if(leds.timer == irq)
    {
        spin_lock(&(dev->lock));
        dev->interrupt_flag = 1;
        spin_unlock(&(dev->lock));
//        printk("kernel:\t\ttimer interrupt\r\n");

        tmp = *mGPBDAT;
        tmp ^= (1<<5);
        *mGPBDAT = tmp;
        wake_up_interruptible(&(dev->myqueue));
    }
    enable_irq(leds.timer);
    return IRQ_HANDLED;
}

static int s3c2440_led_open(struct inode * inode, struct file * filp)
{
    printk("led open\r\n");
    int i;
    int tmp;
    struct clk * clk_p;
    unsigned long pclk;
    unsigned long tcnt;
    if (s3c2440_led.openflag == 1)
        return -EINVAL;
    /*这句话用于间接对s3c2440_led控制，但是下面用到是直接控制，所以没啥用*/
    filp->private_data = &s3c2440_led;
    /*修改打开标志位*/
    spin_lock(&(s3c2440_led.lock));
    s3c2440_led.openflag = 1;
    s3c2440_led.timer_openflag = 0;
    spin_unlock(&(s3c2440_led.lock));

    time_virt = (unsigned long)(ioremap(S3C2440_TCFG0_ADDR, 0X42));
    mTCFG0 = (unsigned long *)(time_virt + 0x00);
    mTCFG1 = (unsigned long *)(time_virt + 0x04);
    mTCON = (unsigned long *)(time_virt + 0x08);
    mTCNTB0 = (unsigned long *)(time_virt + 0x0c);
    mTCMPB0 = (unsigned long *)(time_virt + 0x10);
    mTCNTO0 = (unsigned long *)(time_virt + 0x14);

    virt = (unsigned long)(ioremap(S3C2440_GPB_BASEADDR, 0x10));
    mGPBCON = (unsigned long *)(virt + 0x00);
    mGPBDAT = (unsigned long *)(virt + 0x04);
    mGPBUP  = (unsigned long *)(virt + 0x08);

    for(i=0;i<NUM_RESOURCE;i++)
    {
        *mGPBCON &= CLEAR_CON_BIT(leds.pleds[i]);
        *mGPBCON |= SET_CON_BIT(leds.pleds[i]);
        *mGPBUP  |= SET_UP_BIT(leds.pleds[i]);
    }
    //设置定时器0，1的预分频值为49
    tmp = *mTCFG0;
    tmp &= ~(0x000000ff);
    tmp |= 49;
    *mTCFG0 = tmp;
    //设置定时0，1分别为1/16，1/16分频。
    tmp = *mTCFG1;
    tmp &= ~(0x0000000f);
    tmp |= (3<<0);
    *mTCFG1 = tmp;
    //设置
    clk_p = clk_get(NULL, "pclk");
    pclk = clk_get_rate(clk_p);
    tcnt = (pclk/50/16)/1000;   //设置1000
    printk("kernel:pclk:%d,tcnt:%d\r\n", pclk, tcnt);
    /*硬件配置初始化*/
   // for(i=0;i<NUM_RESOURCE;i++)
   // {
   //     s3c2410_gpio_cfgpin(leds.pleds[i], S3C2410_GPIO_OUTPUT );
   // }
    if(s3c2440_led.openflag == 1)
    {
        int ret = request_irq(  leds.timer,
                            s3c2440_timer_handler,
                            IRQF_SHARED,
                            leds.timer_name,
                            (void *)&s3c2440_led);
        if(ret == -EINVAL)
        {

        }else if(ret == -EBUSY)
        {

        }
        if(ret)
        {
            disable_irq(leds.timer);
            free_irq(leds.timer, (void *)&s3c2440_led);
            return ret;
        }
    }
    return 0;
}

static int s3c2440_led_release(struct inode * inode, struct file * filp)
{
    struct s3c2440_led * dev = filp->private_data;
    int tmp;
    printk("led release\r\n");

    if (!dev->openflag)
        return -EINVAL;

    disable_irq(leds.timer);
    free_irq(leds.timer, (void *)dev);

    //关闭定时器
    if(dev->timer_openflag)
    {
        tmp = *mTCON;
        tmp &= ~(0x0000001f);
        *mTCON = tmp;
    }

    spin_lock(&(dev->lock));
    dev->openflag = 0;
    dev->timer_openflag = 0;
    spin_unlock(&(dev->lock));

    iounmap((void *)virt);

    return 0;
}

#define LED_IN_FREQ 100000

static void s3c2440_led_freq(struct s3c2440_led * dev, unsigned long arg)
{
    //struct s3c2440_led * dev = filp->private_data;
    printk("led freq\r\n");
    int tmp;

    if(!dev->timer_openflag && (arg > 0))
    {   //第一次打开定时器
        //tmp = *mTCON;
        //tmp &= ~(0x0000001f);
        //*mTCON = tmp;

        *mTCNTB0 = arg;
        *mTCMPB0 = arg/2;

        tmp = *mTCON;
        tmp &= ~(0x0000001f);
        tmp |= (1<<0)|(1<<1)|(0<<2)|(1<<3)|(0<<4);
        //tmp |= (1<<8)|(1<<9)|(0<<10)|(1<<11);
        *mTCON = tmp;
        *mTCON &= ~(1<<1);

        spin_lock(&(dev->lock));
        dev->timer_openflag = 1;
        spin_unlock(&(dev->lock));

        printk("kernel:timer open\r\n");
        //wait_event_interruptible(s3c2440_led.myqueue,
          //      s3c2440_led.timer_openflag);
    }
    if(dev->timer_openflag && (arg > 0))
    {   //在定时器打开过程中修改定时周期
        *mTCNTB0 = arg;
        *mTCMPB0 = arg/2;
    }
    if(dev->timer_openflag && (arg == 0))
    {   //关闭定时器
        spin_lock(&(dev->lock));
        dev->timer_openflag = 0;
        spin_unlock(&(dev->lock));

        tmp = *mTCON;
        tmp &= ~(0x0000001f);
        *mTCON = tmp;
        printk("kernel:timer close\r\n");
    }
}

static long s3c2440_led_ioctl(struct file * filp, unsigned int cmd,
        unsigned long arg)
{
    struct s3c2440_led * dev = filp->private_data;

    int i;
    unsigned int mycmd;
    mycmd = _IOC_NR(cmd);
    if(mycmd < 0 || mycmd > 5)
        return -EINVAL;
    if((arg != 0 && arg != 1) && ((mycmd >= 0) &&(mycmd <= 4)))
        return -EINVAL;
    switch(mycmd)
    {
        case 0:case 1:case 2:case 3:
            W_DAT_BIT(leds.pleds[mycmd], arg);
            break;
        case 4:
            for(i=0;i<NUM_RESOURCE;i++)
            {
                W_DAT_BIT(leds.pleds[i], arg);
            }break;
        case 5:
            {
                s3c2440_led_freq(dev, arg);
            }break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t s3c2440_led_read(
        struct file * filp,
        char * buffer,
        size_t count,
        loff_t *pos
        )
{
    struct s3c2440_led * dev = filp->private_data;
    char str[20];
    size_t len;
    int err;

//    printk("kernel:led read\r\n");
    spin_lock(&(dev->lock));
    dev->interrupt_flag = 0;
    spin_unlock(&(dev->lock));

    len = sprintf(str, "%d", (*mTCNTO0));
    err = copy_to_user(buffer, str, len);

    wait_event_interruptible(dev->myqueue,
            dev->interrupt_flag);
    return err?-EINVAL:len;
}

static struct file_operations s3c2440_led_ops = {
    .owner          = THIS_MODULE,
    .open           = s3c2440_led_open,
    .release        = s3c2440_led_release,
    .read           = s3c2440_led_read,
    .unlocked_ioctl = s3c2440_led_ioctl,
};

static int s3c2440_led_probe(struct platform_device * dev)
{
    int i, ret;
    struct resource * led_resource;
    struct platform_device * pdev = dev;

    printk("match ok!");
    //设备号申请
    dev_t devno;

    if(dev_major > 0)
    {
        devno = MKDEV(dev_major, 0);
        ret = register_chrdev_region(devno, 1, "s3c2440_led");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, "s3c2440_led");
        dev_major = MAJOR(devno);
    }
    if(ret < 0)
    {
        return ret;
    }
    //完成设备类创建，实现设备文件自动创建
    s3c2440_led.myclass = class_create(THIS_MODULE,
            "s3c2440_led");
    printk("create class:%d\r\n",s3c2440_led.myclass);
    //2、完成字符设备到加载
    s3c2440_led.mycdev = cdev_alloc();
    cdev_init(s3c2440_led.mycdev, &s3c2440_led_ops);
    //s3c2440_led.mycdev->owner = THIS_MODULE;
    //s3c2440_led.mycdev->ops = &s3c2440_led_ops;
    ret = cdev_add(s3c2440_led.mycdev, devno, 1);
    if(ret)
    {
        printk("add device error\r\n");
        return ret;
    }
    printk("s3c2440_led_cdev:%d\r\n",s3c2440_led.mycdev);
    //初始化消息队列
    init_waitqueue_head(&(s3c2440_led.myqueue));
    //初始化自旋锁
    spin_lock_init(&(s3c2440_led.lock));
    //修改打开标志
    spin_lock(&(s3c2440_led.lock));
    s3c2440_led.openflag = 1;
    spin_unlock(&(s3c2440_led.lock));
    //初始化等待队列
    //设备创建，是想问件自动创建
    device_create(s3c2440_led.myclass, NULL, devno, NULL, "s3c2440_led");

    for(i=0;i<NUM_RESOURCE;i++)
    {
        led_resource = platform_get_resource(pdev, IORESOURCE_MEM, i);
        if(led_resource == NULL)
        {
            return -ENOENT;
        }
        leds.pleds[i] = led_resource->start;
        leds.names[i] = led_resource->name;
    }

    led_resource = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if(led_resource == NULL)
    {
        printk("get led timer failed\r\n");
        return -ENOENT;
    }

    leds.timer = led_resource->start;
    leds.timer_name = led_resource->name;
    printk("lernel:led timer:%d,name:%s\r\n",
            leds.timer, leds.timer_name);

    return 0;
}

static int s3c2440_led_remove(struct platform_device * pdev)
{
    printk("s3c2440_led_remove-----------------start\r\n");
    printk("s3c2440_led_remove->class:%d\r\n",s3c2440_led.myclass);
    printk("s3c2440_led_remove->cdev:%d\r\n", s3c2440_led.mycdev);

    device_destroy(s3c2440_led.myclass, MKDEV(dev_major, 0));
    cdev_del(s3c2440_led.mycdev);
    class_destroy(s3c2440_led.myclass);

    unregister_chrdev_region(MKDEV(dev_major, 0), 1);
    printk("s3c2440_led_remove---------------- end\r\n");
    return 0;
}

static struct platform_driver s3c2440_led_driver = {
    .probe = s3c2440_led_probe,
    .remove = s3c2440_led_remove,
    .driver =
    {
        .owner = THIS_MODULE,
        .name = "s3c2440_led",
    },
};

static int __init platform_driver_init(void)
{
    int ret;

    ret = platform_driver_register(&s3c2440_led_driver);
    if(ret)
    {
        platform_driver_unregister(&s3c2440_led_driver);
        return ret;
    }
    return 0;
}

static void __exit platform_driver_exit(void)
{
    printk("in platform_driver_exit\r\n");

    platform_driver_unregister(&s3c2440_led_driver);
}

module_init(platform_driver_init);
module_exit(platform_driver_exit);

MODULE_LICENSE("GPL");


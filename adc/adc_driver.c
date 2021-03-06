#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/ioctl.h>
#include <linux/clk.h>
#include <linux/uaccess.h>

#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <mach/irqs.h>

#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

int dev_major = -1;

struct adcs{
    int padcs;
    int size;
    int adc_irqs;
    char * names;
    char * irq_names;
}adcs;

struct s3c2440_adc
{
    struct cdev * mycdev;
    struct class * myclass;
    wait_queue_head_t myqueue;
    int overflag;   //转换完成标志
    spinlock_t lock;
    int openflag;   //设备打开标志
};

static struct s3c2440_adc s3c2440_adc;

volatile unsigned long virt;
volatile unsigned long *mADCCON, *mADCTSC, *mADCDLY;
volatile unsigned long *mADCDAT0, *mADCDAT1, *mADCUPDN;

static struct clk * adc_clk;

static irqreturn_t s3c2440_adc_interrupt_handler(
        int irq,
        void * devid
        )
{
    struct s3c2440_adc * dev = devid;
    disable_irq_nosync(adcs.adc_irqs);
    if(irq == adcs.adc_irqs)
    {
        spin_lock(&(dev->lock));
        s3c2440_adc.overflag = 1;
        spin_unlock(&(dev->lock));
//        printk("kernel:adc interrupt\r\n");
        wake_up_interruptible(&(dev->myqueue));
    }
    enable_irq(adcs.adc_irqs);
    return IRQ_HANDLED;
}


static int s3c2440_adc_open(struct inode * inode, struct file * filp)
{
    printk("kernel:adc open\r\n");
    //用于实现间接控制
    filp->private_data = &s3c2440_adc;

    spin_lock(&(s3c2440_adc.lock));
    {
		if (s3c2440_adc.openflag == 1) {
			printk("already open !\r\n");
			spin_unlock(&(s3c2440_adc.lock));
			return 0;
		}

        s3c2440_adc.openflag = 1;
        //使能ADC时钟
        adc_clk = clk_get(NULL, "adc");
        if(!adc_clk) {
            printk(KERN_ERR "failed to find adc clock source\n");
			spin_unlock(&(s3c2440_adc.lock));
            return -ENOENT;
        }
        clk_enable(adc_clk);
    }
    spin_unlock(&(s3c2440_adc.lock));

    //对ADC进行配置
    //将寄存器物理地址映射到虚拟地址
    virt = (unsigned long)ioremap(adcs.padcs, adcs.size);
    mADCCON = (unsigned long *)(virt + 0x00);
    mADCTSC = (unsigned long *)(virt + 0x04);
    mADCDLY = (unsigned long *)(virt + 0x08);
    mADCDAT0 = (unsigned long *)(virt + 0x0c);
    mADCDAT1 = (unsigned long *)(virt + 0x10);
    mADCUPDN = (unsigned long *)(virt + 0x14);

    *mADCCON &= 0x0000;
    *mADCCON |= (1<<14)|(49<<6)|(0<<3)|(0<<2)|(0<<1);
    printk("kernel:ADCCON:0x%08x\r\n", *mADCCON);

    if(s3c2440_adc.openflag == 1)
    {
        int ret = request_irq(adcs.adc_irqs,
                s3c2440_adc_interrupt_handler,
                IRQF_SHARED,
                adcs.irq_names,
                (void *)&s3c2440_adc);
        if(ret == -EINVAL)
        {
            //中断号无效或处理函数指针为NULL
            printk("kernel:EINVAL...\r\n");
        }else if(ret == -EBUSY)
        {
            printk("kernel:EBUSY...\r\n");
        }
        if(ret)
        {
            disable_irq(adcs.adc_irqs);
            free_irq(adcs.adc_irqs, (void *)&s3c2440_adc);
            return -EBUSY;
        }
        printk("kernel:open adc success\r\n");
    }

    return 0;
}

static int s3c2440_adc_release(struct inode * inode, struct file * filp)
{
    printk("kernel:adc release\r\n");
    spin_lock(&(s3c2440_adc.lock));
	if (s3c2440_adc.openflag == 0) {
		printk("already close\r\n");
		spin_unlock(&(s3c2440_adc.lock));
		return 0;
	}

	s3c2440_adc.openflag = 0;
    spin_unlock(&(s3c2440_adc.lock));
    disable_irq(adcs.adc_irqs);
    free_irq(adcs.adc_irqs, (void *)&s3c2440_adc);
    return 0;
}

static long s3c2440_adc_ioctl(struct file * filp,
        unsigned int cmd,
        unsigned long arg)
{
    unsigned int myCmd;
    unsigned long adccon;
    myCmd = _IOC_NR(cmd);
    adccon = *mADCCON;
    printk("kernel:*ADCCON:0x%08x\r\n", *mADCCON);
    switch(myCmd)
    {
        case 0:
            *mADCCON |= (1<<0);
            printk("kernel:adc start\r\n");
            break;
        case 1:
            if(adccon & (1<15))
            {
                printk("kernel:adc:%d\r\n", (*mADCDAT0 & 0x3ff));
            }
            else
            {
                printk("kernel:adc not ready\r\n");
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t s3c2440_adc_read(
        struct file * filp,
        char * buffer,
        size_t count,
        loff_t *pos)
{
    // struct s3c2440_adc * dev = filp->private_data;

    char str[20];
    int err, value;
    size_t len;

    //启动一次ADC转换
    *mADCCON |= (1<<0);
    mdelay(5000);
    len = sprintf(str, "%d", (*mADCDAT0 & 0x3ff));
    //printk("kernel:str:%s, len:%d, count:%d\r\n", str, len, count);
    if(count >= len)
    {
        //将内核中到数据str，放入用户缓冲buffer中
        err = copy_to_user(buffer, str, len);
        //再次启动发送
        *mADCCON |= 1;
    }

    spin_lock(&(s3c2440_adc.lock));
    s3c2440_adc.overflag = 0;
    spin_unlock(&(s3c2440_adc.lock));

    if(!s3c2440_adc.overflag)
    {   //如果完成了一次转换并且标志置1
        if(filp->f_flags & O_NONBLOCK)
        {   //如果是非阻塞
            return -EAGAIN;
        }else{
            //printk("kernel:wait for adc...%d\r\n",s3c2440_adc.overflag);
            wait_event_interruptible(s3c2440_adc.myqueue,
                    s3c2440_adc.overflag);
            //wait_event_interruptible(dev-myqueue, dev->overflag);
        }
    }

    return err?-EINVAL:len;
}

static unsigned int s3c2440_adc_poll(
        struct file * filp,
        struct poll_table_struct * wait
        )
{
//    struct s3c2440_adc * dev = filp->private_data;
    unsigned int mask;
    *mADCCON |= (1<<0);
    //printk("kernel:adc poll wait...\r\n");
    poll_wait(filp, &(s3c2440_adc.myqueue), wait);
    if(s3c2440_adc.overflag)
    {
        mask |= POLLIN;
    }

    return mask;
}

static struct file_operations s3c2440_adc_ops = {
    .owner          = THIS_MODULE,
    .open           = s3c2440_adc_open,
    .release        = s3c2440_adc_release,
    .read           = s3c2440_adc_read,
    .poll           = s3c2440_adc_poll,
    .unlocked_ioctl = s3c2440_adc_ioctl,
};

static int s3c2440_adc_probe(struct platform_device * dev)
{
    int i, ret;
    struct resource * adc_resource;
    struct platform_device * pdev = dev;

    printk("kernel:adc match ok\r\n");
    //1、设备号到申请
    dev_t devno;

    if(dev_major > 0) {
        devno = MKDEV(dev_major, 0);
        ret = register_chrdev_region(devno, 1, "s3c2440_adc");
    } else {
        ret = alloc_chrdev_region(&devno, 0, 1, "s3c2440_adc");
        dev_major = MAJOR(devno);
    }
    if(ret < 0)
        return ret;

    //完成设备类创建
    s3c2440_adc.myclass = class_create( THIS_MODULE, "s3c2440_adc" );
	if (s3c2440_adc.myclass == NULL) {
		ret = -ENOMEM;
		goto err_class;
	}
    //2、完成字符设备的加载
    s3c2440_adc.mycdev = cdev_alloc();
	if (s3c2440_adc.mycdev == NULL) {
		ret = -ENOMEM;
		goto err_cdev_alloc;
	}
    cdev_init(s3c2440_adc.mycdev, &s3c2440_adc_ops);
    ret = cdev_add(s3c2440_adc.mycdev, devno, 1);
    if(ret) {
        printk("kernel:cdev_add error\r\n");
		ret = -EINVAL;
		goto err_cdev;
    }
    printk("kernel:s3c2440_adc cdev:%d\r\n", s3c2440_adc.mycdev);

    //初始化等待队列
    init_waitqueue_head(&(s3c2440_adc.myqueue));
    //初始化设备自旋锁
    spin_lock_init(&(s3c2440_adc.lock));
    //设备创建
    device_create(s3c2440_adc.myclass, NULL, devno, NULL, "s3c2440_adc");
    //获取device资源
    adc_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(adc_resource == NULL) {
        printk("kernel:platform get resource MEM failed\r\n");
        ret = -ENOENT;
		goto err_resource;
    }
    adcs.padcs = adc_resource->start;
    adcs.size = adc_resource->end - adc_resource->start;
    adcs.names = adc_resource->name;
    printk("kernel:adc name:%s adcs.padcs:%d adc_resource->start:%d\r\n",
            adcs.names, adcs.padcs, adc_resource->start);
    printk("kernel:size:%d\r\n", adcs.size);
    //获取adc中断资源
    adc_resource = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if(adc_resource == NULL)
    {
        printk("kernel:platform get resource IRQ failed\r\n");
        ret = -ENOENT;
		goto err_resource;
    }
    adcs.adc_irqs = adc_resource->start;
    adcs.irq_names = adc_resource->name;
    printk("kernel:adc_irqs:%d, irq_name:%s\r\n",
            adcs.adc_irqs, adcs.irq_names);
    return 0;

err_resource:
    device_destroy(s3c2440_adc.myclass, MKDEV(dev_major, 0));
err_cdev:
	cdev_del(s3c2440_adc.mycdev);
err_cdev_alloc:
	class_destroy(s3c2440_adc.myclass);
err_class:
    unregister_chrdev_region(MKDEV(dev_major, 0), 1);
	return ret;
}

static int s3c2440_adc_remove(struct platform_device * pdev)
{
    device_destroy(s3c2440_adc.myclass, MKDEV(dev_major, 0));
    cdev_del(s3c2440_adc.mycdev);
    class_destroy(s3c2440_adc.myclass);
    unregister_chrdev_region(MKDEV(dev_major, 0), 1);
    return 0;
}

static struct platform_driver s3c2440_adc_driver = {
    .probe      = s3c2440_adc_probe,
    .remove     = s3c2440_adc_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "s3c2440_adc",
    },
};

static int __init platform_driver_init(void)
{
    int ret;
    ret = platform_driver_register(&s3c2440_adc_driver);
    if(ret) {
        platform_driver_unregister(&s3c2440_adc_driver);
        return ret;
    }
    return 0;
}

static void __exit platform_driver_exit(void)
{
    platform_driver_unregister(&s3c2440_adc_driver);
}

module_init(platform_driver_init);
module_exit(platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("not me");


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
//#include <mach/hardware.h>
//#include <mach/regs-gpio.h>
#include <linux/gpio.h>
#include <linux/clk.h>
//kmalloc
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/irqreturn.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/timer.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#define NUM_RESOURCE	4
#define DRIVER_NAME		"platform_test_driver"

#define DEBUG(fmt, ...)		do {\
								printk("%s[%d]:", __func__, __LINE__);\
								printk(fmt"\r\n", ##__VA_ARGS__);\
							}while(0)

int dev_major = -1;

typedef struct led_t {
	int pin;
	char name[8];
	struct timer_list softtimer;
	int freq;
	int status;	// on/off
	bool enable;
} led_t;

typedef struct device_t
{
    struct cdev cdev;
    struct class *class;
    spinlock_t lock;
    wait_queue_head_t queue;
    int interrupt_flag;
    int openflag;
	dev_t devno;
	//timer
    int timer;
    char timer_name[8];
	//led
	struct semaphore led_sem;
	led_t led[NUM_RESOURCE];
} device_t;

static device_t *device = NULL;

#if 0
//硬件中断实现
static irqreturn_t s3c2440_timer_handler(
        int irq,
        void * devid
        )
{
    device_t *dev = devid;
    //unsigned int tmp;
    disable_irq_nosync(dev->timer);
    if(dev->timer == irq)
    {
        spin_lock(&dev->lock);
        dev->interrupt_flag = 1;
        spin_unlock(&dev->lock);
//        printk("kernel:\t\ttimer interrupt\r\n");

       /* tmp = *mGPBDAT;
        tmp ^= (1<<5);
        *mGPBDAT = tmp;*/
        wake_up_interruptible(&dev->queue);
    }
    enable_irq(dev->timer);
    return IRQ_HANDLED;
}
#endif

static int s3c2440_led_open(struct inode * inode, struct file * filp)
{
	struct device_t *dev = NULL;
    DEBUG("led open");

	dev = container_of(inode->i_cdev, struct device_t, cdev);
	filp->private_data = dev;

    if (dev->openflag == 1)
        return -EBUSY;
    /*这句话用于间接对s3c2440_led控制，但是下面用到是直接控制，所以没啥用*/
    /*修改打开标志位*/
    spin_lock(&dev->lock);
    dev->openflag = 1;
    spin_unlock(&dev->lock);

#if 0
    if(dev->openflag == 1)
    {
        int ret = request_irq(  dev->timer,
                            s3c2440_timer_handler,
                            IRQF_SHARED,
                            dev->timer_name,
                            (void *)device);
        if(ret == -EINVAL)
        {

        }else if(ret == -EBUSY)
        {

        }
        if(ret)
        {
            disable_irq(dev->timer);
            free_irq(dev->timer, (void *)device);
            return ret;
        }
    }
#endif
    return 0;
}

static int s3c2440_led_release(struct inode * inode, struct file * filp)
{
    device_t *dev = filp->private_data;
    int i;
	//int tmp;
    printk("led release\r\n");

    if (dev->openflag == 0)
        return -EINVAL;
#if 0
    disable_irq(dev->timer);
    free_irq(dev->timer, (void *)dev);
#endif
    //关闭定时器
#if 0
    if(dev->timer_openflag)
    {
        tmp = *mTCON;
        tmp &= ~(0x0000001f);
        *mTCON = tmp;
    }
#endif

    spin_lock(&(dev->lock));
    dev->openflag = 0;
    //dev->timer_openflag = 0;
    spin_unlock(&(dev->lock));

	for (i = 0; i < NUM_RESOURCE; i++) {
		if (device->led[i].enable == true) {
			del_timer_sync(&device->led[i].softtimer);
			device->led[i].enable = false;
		}
	}

    return 0;
}

static long s3c2440_led_freq(struct device_t *dev, unsigned long arg)
{
	int pin, freq;
	pin = arg >> 16;
	freq = arg & 0x0000ffff;
	DEBUG("arg=%08x, pin=%d, freq=%d", arg, pin, freq);

	if (pin > 3) {
		DEBUG("pin error, pin[%d] not exists", pin);
		return -EINVAL;
	}

	dev->led[pin].freq = freq;
	dev->led[pin].softtimer.expires = jiffies + freq * HZ;
	dev->led[pin].enable = true;
	mod_timer(&dev->led[pin].softtimer, dev->led[pin].softtimer.expires);

	return 0;
}

static long s3c2440_led_ioctl(struct file * filp, unsigned int cmd,
        unsigned long arg)
{
	int ret;
    unsigned int mycmd;
    device_t *dev = filp->private_data;

	if (down_interruptible(&dev->led_sem)) {
		DEBUG("can not get sem");
		ret = -EBUSY;
		goto err;
	}

	mycmd = _IOC_NR(cmd);
    if(mycmd < 0 || mycmd > 5) {
        ret = -EINVAL;
		goto err;
	}

	switch(mycmd)
    {
        case 0:
		case 1:
		case 2:
		case 3:
            //W_DAT_BIT(dev->pleds[mycmd], arg);
            break;
        case 4:
            /*for(i=0;i<NUM_RESOURCE;i++)
            {
                W_DAT_BIT(dev->pleds[i], arg);
            }*/
			break;
        case 5:
			ret = s3c2440_led_freq(dev, arg);
            break;
        default:
            ret = -EINVAL;
			break;
    }
	
	up(&dev->led_sem);
err:
	return ret;
}

static ssize_t s3c2440_led_read( struct file * filp, char * user_buffer,
								 size_t count, loff_t *pos )
{
	device_t *dev = filp->private_data;
    char str[32];
    size_t len = 0;
    int err, i;

    /*spin_lock(&(dev->lock));
    dev->interrupt_flag = 0;
    spin_unlock(&(dev->lock));
	*/

	memset(str, 0, sizeof(str));

	for (i = 0; i < NUM_RESOURCE; i++) {
		if (device->led[i].enable == true) {
			len += sprintf(str + len, "%s%d", device->led[i].name, device->led[i].status);
		}
	}
    err = copy_to_user(user_buffer, str, len);

    //wait_event_interruptible(dev->queue,
    //        dev->interrupt_flag);

    return err ? -EINVAL : len;
}

static struct file_operations s3c2440_led_ops = {
    .owner          = THIS_MODULE,
    .open           = s3c2440_led_open,
    .release        = s3c2440_led_release,
    .read           = s3c2440_led_read,
    .unlocked_ioctl = s3c2440_led_ioctl,
};

void softtimer_handler(unsigned long arg)
{
	led_t *led = (led_t *)arg;
	DEBUG("tiemr=%s pin=%d expires=%ld", led->name, led->pin, led->softtimer.expires);

	led->softtimer.expires += led->freq * HZ;
	//add_timer(&led->softtimer);
	led->status = !led->status;
	mod_timer(&led->softtimer, led->softtimer.expires);
}

static int platform_test_probe(struct platform_device * dev)
{
    int i, ret;
    struct resource *dev_resource;
    struct platform_device * pdev = dev;

    printk("match ok!");

	device = kmalloc(sizeof(device_t), GFP_KERNEL);
	if (device == NULL) {
		ret = -ENOMEM;
		printk("%s[%d]:kmalloc failed\r\n", __func__, __LINE__);
		goto err_register;
	}
	memset(device, 0, sizeof(device_t));

    //设备号申请
	if(dev_major > 0) {
        device->devno = MKDEV(dev_major, 0);
        ret = register_chrdev_region(device->devno, 1, DRIVER_NAME);
    } else {
        ret = alloc_chrdev_region(&device->devno, 0, 1, DRIVER_NAME);
        dev_major = MAJOR(device->devno);
    }
    if(ret < 0)
        return ret;

    //完成设备类创建，实现设备文件自动创建
    device->class = class_create(THIS_MODULE, DRIVER_NAME);
    printk("create class:%08x\r\n", device->class);
    //2、完成字符设备到加载
    //device->cdev = cdev_alloc();
    cdev_init(&device->cdev, &s3c2440_led_ops);
    ret = cdev_add(&device->cdev, device->devno, 1);
    if(ret) {
        printk("%s[%d]:add device error\r\n", __func__, __LINE__);
		ret = -EINVAL;
		goto err_cdev_add;
    }
    //初始化消息队列
    init_waitqueue_head(&device->queue);
    //初始化自旋锁
    spin_lock_init(&device->lock);
    //修改打开标志
    spin_lock(&device->lock);
    device->openflag = 0;
    spin_unlock(&device->lock);
    //初始化等待队列
    //设备创建，是想问件自动创建
    device_create(device->class, NULL, device->devno, NULL, DRIVER_NAME);

	sema_init(&device->led_sem, 1);

    for( i = 0; i < NUM_RESOURCE; i++) {
        dev_resource = platform_get_resource(pdev, IORESOURCE_MEM, i);
        if(dev_resource == NULL) {
            ret = -ENOENT;
			goto err_get_resource;
        }
        //device->pleds[i] = dev_resource->start;
		//memcpy(device->names[i], dev_resource->name, strlen(dev_resource->name));
		device->led[i].pin = dev_resource->start;
		memcpy(device->led[i].name, dev_resource->name, strlen(dev_resource->name));
		//初始化软件定时器
		init_timer(&device->led[i].softtimer);
		device->led[i].softtimer.function = softtimer_handler;
		device->led[i].softtimer.data = (unsigned long)&device->led[i];
		//device->led[i].softtimer.expires = jiffies;

		DEBUG("find resource:%s %d", device->led[i].name, device->led[i].pin);
    }

    dev_resource = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if(dev_resource == NULL) {
        printk("get led timer failed\r\n");
        ret =  -ENOENT;
		goto err_get_resource;
    }

    device->timer = dev_resource->start;
	memcpy(device->timer_name, dev_resource->name, strlen(dev_resource->name));
	DEBUG("find resource:%s %d", device->timer_name, device->timer);
    //device->timer_name = dev_resource->name;

    return ret;

err_get_resource:
    device_destroy(device->class, device->devno);
	for (i = 0; i < NUM_RESOURCE; i++) {
		if (device->led[i].enable == true) {
			del_timer_sync(&device->led[i].softtimer);
		}
	}
err_cdev_add:
	cdev_del(&device->cdev);
	kfree(device);
err_register:
	unregister_chrdev_region(device->devno, 1);
	return ret;
}

static int platform_test_remove(struct platform_device * pdev)
{
	int i;
    printk("platform_driver_remove-----------------start\r\n");
    printk("platform_driver_remove->class:%08x\r\n", device->class);
    printk("platform_driver_remove->cdev:%08x\r\n", &device->cdev);

    device_destroy(device->class, device->devno);
	for (i = 0; i < NUM_RESOURCE; i++) {
		if (device->led[i].enable == true) {
			del_timer_sync(&device->led[i].softtimer);
		}
	}
    cdev_del(&device->cdev);
    class_destroy(device->class);

    unregister_chrdev_region(device->devno, 1);

	kfree(device);
    printk("platform_driver_remove---------------- end\r\n");
    return 0;
}

static struct platform_driver platform_test_driver = {
    .probe = platform_test_probe,
    .remove = platform_test_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "platform_test",
    },
};

static int __init driver_test_init(void)
{
    int ret;

    ret = platform_driver_register(&platform_test_driver);
    if (ret) {
        platform_driver_unregister(&platform_test_driver);
        return ret;
    }
    return 0;
}

static void __exit driver_test_exit(void)
{
    printk("in platform_driver_exit\r\n");
    platform_driver_unregister(&platform_test_driver);
}

module_init(driver_test_init);
module_exit(driver_test_exit);

MODULE_LICENSE("GPL");

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>

#define DEVICE_NAME "DS_GPIO"
#define GPIO_G_BASEADDR 0x56000060

volatile unsigned long virt_gpg;
volatile unsigned long *mGPGCON, *mGPGDAT, *mGPGUP;

static int gpio_open(struct inode *inode, struct file *filp)
{
    virt_gpg = (unsigned long)ioremap(GPIO_G_BASEADDR, 0x10);
    mGPGCON = (unsigned long *)(virt_gpg + 0x00);
    mGPGDAT = (unsigned long *)(virt_gpg + 0x04);
    mGPGUP = (unsigned long *)(virt_gpg + 0x08);

    *mGPGCON = ((1<<(2*7)) | (1<<(2*6)) | (1<<(2*5)) | (1<<(2*3)));
    *mGPGUP = 0;

    return 0;
}

static int gpio_release(struct inode *inode, struct file *filp)
{
    iounmap((void *)virt_gpg);
    return 0;
}

static long gpio_ioctl(struct file *filp,
        unsigned int cmd,
        unsigned long arg)
{
    unsigned int theCMD;

    theCMD = _IOC_NR(cmd);
    printk("cmd:%d, arg:%d\r\n", theCMD, arg);
    if((theCMD < 0) || (theCMD > 15))
    {
        return -EINVAL;
    }
    switch(arg)
    {
        case 0:
            {
                *mGPGDAT &= ~(1<<theCMD);
            }break;
        case 1:
            {
                *mGPGDAT |= (1<<theCMD);
            }break;
        default:
            return -EINVAL;
    }

    return 0;
}

static ssize_t gpio_read(
        struct file *filp,
        char *buffer,
        size_t count,
        loff_t *pos
        )
{

    return 0;
}

static ssize_t gpio_write(
        struct file *filp,
        char *buffer,
        size_t count,
        loff_t *pos
        )
{

    return 0;
}

static const struct file_operations gpio_fops = {
    .owner  = THIS_MODULE,
    .open = gpio_open,
    .release = gpio_release,
    .read = gpio_read,
    .write = gpio_write,
    .unlocked_ioctl = gpio_ioctl,
};

static struct miscdevice gpio_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &gpio_fops,
};

static int __init gpio_init(void)
{
    int err;

    misc_register(&gpio_device);
}

static void __exit gpio_exit(void)
{
    misc_deregister(&gpio_device);
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("me");

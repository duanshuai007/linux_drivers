#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <linux/gpio.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "s3c2440_leds_by_BigFish"
#define LED_MAJOR   210
#define LED_ON      1
#define LED_OFF     0

static int open_flag = 0;

static unsigned long led_table[] =
{
    S3C2410_GPB(5),
    S3C2410_GPB(6),
    S3C2410_GPB(7),
    S3C2410_GPB(8),
};

static unsigned int led_cfg_table[] =
{
    S3C2410_GPIO_OUTPUT,
    S3C2410_GPIO_OUTPUT,
    S3C2410_GPIO_OUTPUT,
    S3C2410_GPIO_OUTPUT,
};

static int led_open(struct inode * inode, struct file * file)
{
    printk("led open\r\n");
    open_flag = 1;
    return 0;
}

static int led_release(struct inode * inode, struct file * file)
{
    printk("led close\r\n");
    open_flag = 0;
    return 0;
}

static long led_unlocked_ioctl(
        struct file * file,
        unsigned int io_cmd,
        unsigned long arg
        )
{
    int i;
    unsigned int cmd;
    if(open_flag == 0)
    {
        printk("led not open\r\n");
        return -EINVAL;
    }
    cmd = _IOC_NR(io_cmd);
    printk("module arg:%d,cmd:%d\r\n",arg, cmd);
    if(cmd < 0 || cmd > 4)
    {
        printk("led's cmd error.\r\n");
        return -EINVAL;
    }
    if(cmd == 4)
    {
        for(i=0;i<4;i++)
        {
            s3c2410_gpio_setpin(led_table[i], arg);
        }
        return 0;
    }
    if(arg != LED_ON && arg != LED_OFF)
    {
        printk("led arg error\r\n");
        return -EINVAL;
    }
    s3c2410_gpio_setpin(led_table[cmd], arg);
    return 0;
}

static struct file_operations led_ops = {
    .owner          = THIS_MODULE,
    .open           = led_open,
    .release        = led_release,
    .unlocked_ioctl = led_unlocked_ioctl,
};

static int  __init led_init(void)
{
    int ret, i;
    //设备初始化
    for(i=0;i<4;i++)
    {
        s3c2410_gpio_cfgpin(led_table[i], led_cfg_table[i]);
        s3c2410_gpio_setpin(led_table[i], LED_OFF);
    }
    //设备注册
    ret = register_chrdev(LED_MAJOR, DEVICE_NAME, &led_ops);
    if(ret < 0)
    {
        printk(DEVICE_NAME "register failed!\r\n");
    }
    return ret;
}

static void __exit led_exit(void)
{   //设备反注册
    unregister_chrdev(LED_MAJOR, DEVICE_NAME);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("duanshuai");
MODULE_DESCRIPTION("2440 led driver");


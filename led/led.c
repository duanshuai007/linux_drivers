#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <linux/gpio.h>

#define DEVICE_NAME "my2440_leds"
#define LED_MAJOR   231
#define LED_ON      1
#define LED_OFF     0

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

static int leds_open(struct inode *inode, struct file * file)
{
    printk("led open\r\n");
    return 0;
}

static int leds_ioctl(
        struct inode * inode,
        struct file * file,
        unsigned int cmd,
        unsigned long arg
        )
{
    if(arg < 0 || arg > 4)
    {
        printk("led's arg error\r\n");
        return -EINVAL;
    }
    if(arg == 4)
    {
        s3c2410_gpio_setpin(led_table[0], cmd);
        s3c2410_gpio_setpin(led_table[1], cmd);
        s3c2410_gpio_setpin(led_table[2], cmd);
        s3c2410_gpio_setpin(led_table[3], cmd);
        return 0;
    }
    switch(cmd)
    {
        case LED_ON:
            s3c2410_gpio_setpin(led_table[arg], LED_ON);
            break;
        case LED_OFF:
            s3c2410_gpio_setpin(led_table[arg], LED_OFF);
            break;
        default:
            printk("led cmd error\r\n");
            return -EINVAL;
    }
    return 0;
}

static struct file_operations leds_fops =
{
    .owner  = THIS_MODULE,
    .open   = leds_open,
    .ioctl  = leds_ioctl,
};

static int __init led_init(void)
{
    int ret, i;
    for(i = 0; i < 4; i++)
    {
        s3c2410_gpio_cfgpin(led_table[i], led_cfg_table[i]);
        s3c2410_gpio_setpin(led_table[i], LED_OFF);
    }
    ret = register_chrdev(LED_MAJOR, DEVICE_NAME, &leds_fops);
    if(ret < 0)
    {
        printk(DEVICE_NAME "register failed!\r\n");
    }
    return ret;
}

static void __exit led_exit(void)
{
    unregister_chrdev(LED_MAJOR, DEVICE_NAME);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("duan shuai");
MODULE_DESCRIPTION("2440 led driver");

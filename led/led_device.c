#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <mach/regs-gpio.h>

#include <asm/io.h>
#include <asm/irq.h>

static struct resource s3c2440_gpio_resource[] = {
    [0] = {
        .name   = "led1",
        .start  = 5,
        .end    = 5,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .name   = "led2",
        .start  = 6,
        .end    = 6,
        .flags  = IORESOURCE_MEM,
    },
    [2] = {
        .name   = "led3",
        .start  = 7,
        .end    = 7,
        .flags  = IORESOURCE_MEM,
    },
    [3] = {
        .name   = "led4",
        .start  = 8,
        .end    = 8,
        .flags  = IORESOURCE_MEM,
    },
    [4] = {
        .name   = "timer0",
        .start  = IRQ_TIMER0,
        .end    = IRQ_TIMER0,
        .flags  = IORESOURCE_IRQ,
    },
};

static int s3c2440_led_release(struct device * dev)
{
    printk("device release\r\n");
    return 0;
}

static struct platform_device s3c2440_led_device = {
    .name = "s3c2440_led",
    .id = -1,
    .dev.release = s3c2440_led_release,
    .num_resources = ARRAY_SIZE(s3c2440_gpio_resource),
    .resource = s3c2440_gpio_resource,
};

static int __init s3c2440_led_init(void)
{
    int ret;
    printk("led init");
    ret = platform_device_register(&s3c2440_led_device);
    if(ret)
    {
        platform_device_unregister(&s3c2440_led_device);
        return ret;
    }
    return 0;
}

static void __exit s3c2440_led_exit(void)
{
    printk("led exit");
    platform_device_unregister(&s3c2440_led_device);
}

module_init(s3c2440_led_init);
module_exit(s3c2440_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("not me");

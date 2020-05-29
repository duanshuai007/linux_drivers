#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/platform_device.h>

//硬件相关到头文件
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <linux/gpio.h>

//硬件密切相关的中断号
#include <mach/irqs.h>

static struct resource button_resource[] =
{
    /*EINT8*/
    [0] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IRQ_EINT8,
        .end    = IRQ_EINT8,
        .name   = "s3c2440_EINT8",
    },
    /*EINT11*/
    [1] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IRQ_EINT11,
        .end    = IRQ_EINT11,
        .name   = "s3c2440_EINT11",
    },
    /*EINT13*/
    [2] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IRQ_EINT13,
        .end    = IRQ_EINT13,
        .name   = "s3c2440_EINT13",
    },
    /*EINT14*/
    [3] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IRQ_EINT14,
        .end    = IRQ_EINT14,
        .name   = "s3c2440_EINT14",
    },
    /*EINT15*/
    [4] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IRQ_EINT15,
        .end    = IRQ_EINT15,
        .name   = "s3c2440_EINT15",
    },
    /*EINT19*/
    [5] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IRQ_EINT19,
        .end    = IRQ_EINT19,
        .name   = "s3c2440_EINT19",
    },
};

static int s3c2440_button_release(void)
{
    printk("s3c2440_button release\r\n");
    return 0;
}

static struct platform_device button_device = {
    .name = "s3c2440_button",
    .id = -1,
    .dev.release = s3c2440_button_release,
    .num_resources = ARRAY_SIZE(button_resource),
    .resource = button_resource,
};

static int __init s3c2440_button_init(void)
{
    int ret;
    ret = platform_device_register(&button_device);
}

static void __exit s3c2440_button_exit(void)
{
    platform_device_unregister(&button_device);
}

module_init(s3c2440_button_init);
module_exit(s3c2440_button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("not me");


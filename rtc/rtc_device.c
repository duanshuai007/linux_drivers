#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>

#include <mach/irqs.h>
#include <mach/hardware.h>

#include <asm/io.h>

#define RTC_BASE_ADDR   0X57000000
#define RTC_ADDR_END    0X570000ff

static struct resource s3c2440_rtc_resource[] = {
    [0] = {
        .name   = "rtc",
        .start  = RTC_BASE_ADDR,
        .end    = RTC_ADDR_END,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .name   = "rtc-irq",
        .start  = IRQ_RTC,
        .end    = IRQ_RTC,
        .flags  = IORESOURCE_IRQ,
    },
    [2] = {
        .name   = "tick_irq",
        .start  = IRQ_TICK,
        .end    = IRQ_TICK,
        .flags  = IORESOURCE_IRQ,
    },
};

static int s3c2440_rtc_release(struct devicee *dev)
{
    printk("s3c2440 rtc release\r\n");
    return 0;
}

static struct platform_device s3c2440_rtc_device ={
    .name       = "s3c2440_rtc",
    .id         = -1,
    .dev.release = s3c2440_rtc_release,
    .num_resources = ARRAY_SIZE(s3c2440_rtc_resource),
    .resource = s3c2440_rtc_resource,
};

static int __init s3c2440_rtc_init(void)
{
    int ret;
    printk("rtc init\r\n");
    ret = platform_device_register(&s3c2440_rtc_device);
    if(ret)
    {
        printk("device register failed\r\n");
        platform_device_unregister(&s3c2440_rtc_device);
        return ret;
    }
    printk("device register success\r\n");
    return 0;
}

static void __exit s3c2440_rtc_exit(void)
{
    printk("rtc exit\r\n");
    platform_device_unregister(&s3c2440_rtc_device);
}

module_init(s3c2440_rtc_init);
module_exit(s3c2440_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("not me");


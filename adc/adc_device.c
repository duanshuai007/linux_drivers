#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <mach/irqs.h>
#include <mach/hardware.h>

#include <asm/io.h>

#define S3C2440_ADCCON_ADDR     0X58000000
#define S3C2440_ADCTSC_ADDR     0X58000004
#define S3C2440_ADCDLY_ADDR     0X58000008
#define S3C2440_ADCDAT0_ADDR    0X5800000C
#define S3C2440_ADCDAT1_ADDR    0X58000010
#define S3C2440_ADCUPDN_ADDR    0X58000014

#define ADC_START_ADDR          S3C2440_ADCCON_ADDR
#define ADC_END_ADDR            ADC_START_ADDR+0x18

static struct resource s3c2440_adc_resource[] = {
    [0] = {
        .name   = "irq",
        .start  = IRQ_ADC,
        .end    = IRQ_ADC,
        .flags  = IORESOURCE_IRQ,
    },
    [1] = {
        .name   = "adc",
        .start  = ADC_START_ADDR,
        .end    = ADC_END_ADDR,
        .flags  = IORESOURCE_MEM,
    },
};

static int s3c2440_adc_release(struct device * dev)
{
    printk("adc device release\r\n");
    return 0;
}

static struct platform_device s3c2440_adc_device = {
    .name = "s3c2440_adc",
    .id = -1,
    .dev.release = s3c2440_adc_release,
    .num_resources = ARRAY_SIZE(s3c2440_adc_resource),
    .resource = s3c2440_adc_resource,
};

static int __init s3c2440_adc_init(void)
{
    int ret;
    printk("adc init");
//   printk("num_resources:%d\r\n", s3c2440_adc_device.num_resources);
    ret = platform_device_register(&s3c2440_adc_device);
    if(ret)
    {
        printk("device register failed\r\n");
        platform_device_unregister(&s3c2440_adc_device);
        return ret;
    }
    printk("device register success\r\n");
    return 0;
}

static void __exit s3c2440_led_exit(void)
{
    printk("adc exit");
    platform_device_unregister(&s3c2440_adc_device);
}

module_init(s3c2440_adc_init);
module_exit(s3c2440_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("not me");

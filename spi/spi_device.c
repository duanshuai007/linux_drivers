#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
//#include <mach/hardware.h>
//#include <mach/regs-gpio.h>
#include <linux/spi/spi.h>
#include <asm/io.h>
#include <asm/irq.h>


#define SPI_NO		0

#if 1
static struct resource s3c2440_spi_resource[] = {
    [0] = {
        .name = "spi1",
        .start = 0x59000020,
        .end = 0x5900003f,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .name = "spi1-irq",
        .start = 10,
        //.start = IRQ_SPI1,
        .end = 10,
        //.end = IRQ_SPI1,
        .flags = IORESOURCE_IRQ,
    },
};
#endif

static struct spi_board_info spidev_test_info[] __initconst = {
	{
		.modalias = "spilcd",
		.max_speed_hz = 1 * 1000 * 1000,
		.bus_num = SPI_NO,
		.chip_select = 1,
		.mode = SPI_MODE_0,
	},
};
#if 1
static void s3c2440_spi_release(struct device *dev)
{
    printk(KERN_INFO "spi device release\r\n");
    return;
}
#endif

static struct platform_device s3c2440_spi_device = {
    .name = "spilcd",
    .id = -1,
    .dev.release = s3c2440_spi_release,
    .num_resources = ARRAY_SIZE(s3c2440_spi_resource),
    .resource = s3c2440_spi_resource,
};

struct spi_device *spidev;
static int __init s3c2440_spi_init(void)
{
    int ret;
	printk(KERN_INFO "spi device init\n");

	struct spi_master *master;
//	spi_register_board_info(spidev_test_info, ARRAY_SIZE(spidev_test_info));
	master = spi_busnum_to_master(SPI_NO);
	if (!master) {
		printk(KERN_ALERT "spi_busnum_to_master failed\n");
		return -EFAULT;
	}

	spidev = spi_new_device(master, spidev_test_info);
	if (!spidev) {
		printk(KERN_ALERT "spi new device failed\n");
		return -EBUSY;
	}

	spidev->bits_per_word = 8;

	ret = spi_setup(spidev);
	if (ret) {
		printk(KERN_ALERT "spi setup failed\n");
		spi_unregister_device(spidev);
		return -ENODEV;
	}

    ret = platform_device_register(&s3c2440_spi_device);
    if(ret)
    {
        platform_device_unregister(&s3c2440_spi_device);
        return ret;
    }

   	return 0;
}

static void __exit s3c2440_spi_exit(void)
{
    printk(KERN_INFO "spi device exit\n");
    
	if (spidev) {
		spi_unregister_device(spidev);
	}
	
	platform_device_unregister(&s3c2440_spi_device);
}

module_init(s3c2440_spi_init);
module_exit(s3c2440_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("not me");

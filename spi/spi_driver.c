/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/cdev.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <asm/uaccess.h>

typedef struct spi_lcd_t {
	struct class *class;
	struct cdev *cdev;
	struct device *device;
	struct spi_device *spi;
	dev_t devno;
	int major;
} spi_lcd_t;

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

/* Write-only message with current device setup */
static ssize_t spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	return 0;
}

static int spidev_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	.write =	spidev_write,
	.read =		spidev_read,
	//.unlocked_ioctl = spidev_ioctl,
	//.compat_ioctl = spidev_compat_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
	//.llseek =	no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

/*-------------------------------------------------------------------------*/

static int spi_lcd_probe(struct spi_device *spi)
{
	int ret;
	struct spi_lcd_t *spilcd;

	printk(KERN_INFO "spi lcd probe!\n");

	/* Allocate driver data */
	spilcd = kmalloc(sizeof(struct spi_lcd_t), GFP_KERNEL);
	if (!spilcd)
		return -ENOMEM;

	/* Initialize the driver data */
	spilcd->spi = spi;
	ret = spi_setup(spi);
	if (ret) {
		printk(KERN_ALERT "setup spi error!\n");
		return ret;
	}

	ret = alloc_chrdev_region(&spilcd->devno, 0, 1, "spilcd");
	if (ret < 0) {
		printk(KERN_ALERT "alloc_chrdev_region failed\n");
		return ret;
	}
	spilcd->major = MAJOR(spilcd->devno);

	spilcd->cdev = cdev_alloc();
	cdev_init(spilcd->cdev, &spidev_fops);
	ret = cdev_add(spilcd->cdev, spilcd->devno, 1);


	spilcd->class = class_create(THIS_MODULE, "spilcd_class");
	if (IS_ERR(spilcd->class)) {
		printk(KERN_ALERT "class create failed\n");
		return -EFAULT;
	}

	spilcd->device = device_create(spilcd->class, NULL, spilcd->devno, NULL, "spilcd");

	spi_set_drvdata(spi, spilcd);

	return 0;
}

static int spi_lcd_remove(struct spi_device *spi)
{
	struct spi_lcd_t *spilcd = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spi_set_drvdata(spi, NULL);

	/* prevent new opens */
	device_destroy(spilcd->class, spilcd->devno);
	cdev_del(spilcd->cdev);
	class_destroy(spilcd->class);

	unregister_chrdev_region(spilcd->devno, 1);

	return 0;
}

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spilcd",
		.owner =	THIS_MODULE,
	},
	.probe =	spi_lcd_probe,
	.remove =	spi_lcd_remove,
};

static int __init spilcd_init(void)
{
	int status;
	struct spi_master *master;
	struct device dev;
	struct spi_lcd_t *sl;
	printk(KERN_INFO "spi lcd init\n");


	master = spi_alloc_master(&dev, sizeof(struct spi_lcd_t));
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
	master->bits_per_word_mask = 8;
	master->bus_num = 0;
	master->num_chipselect = 1;

	printk(KERN_INFO "spi_alloc_master done\n");
	sl = spi_master_get_devdata(master);
	spi_register_master(master);

	printk(KERN_INFO "spi_register_master done\n");

	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		printk(KERN_ALERT "spi register driver failed\n");
		return -EFAULT;
	}
	printk(KERN_INFO "spidev init ok\n");

	return status;
}

static void __exit spilcd_exit(void)
{
	printk(KERN_INFO "spidev exit");
	spi_unregister_driver(&spidev_spi_driver);
}

module_init(spilcd_init);
module_exit(spilcd_exit);

MODULE_LICENSE("GPL");

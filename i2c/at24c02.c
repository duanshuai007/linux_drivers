#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#define I2C_READ    0X100001
#define I2C_WRITE   0X100002

struct eeprom_data{
    unsigned char addr;
    unsigned char data;
};

static struct i2c_device_id at24c02_id[] = {
    {"at24c02", 0x50},
};

static struct i2c_client *g_client;

static unsigned char at24c02_i2c_read(unsigned char addr)
{
    int ret;
    ret = i2c_smbus_read_byte_data(g_client, addr);
    msleep(1);
    return ret;
}

static void at24c02_i2c_write(unsigned char addr, unsigned char data)
{
    i2c_smbus_write_byte_data(g_client, addr, data);
    msleep(1);
}

static long at24c02_ioctl(struct file *file,
        unsigned int cmd, unsigned long arg)
{
    struct eeprom_data eeprom;
    copy_from_user(&eeprom, (struct eeprom_data *)arg, sizeof(eeprom));

    switch(cmd)
    {
        case I2C_READ:
            //printk("kernel:i2c read\r\n");
            eeprom.data = at24c02_i2c_read(eeprom.addr);
            copy_to_user((struct eeprom_data *)arg, &eeprom, sizeof(eeprom));
            break;
        case I2C_WRITE:
            //printk("kernel:i2c write\r\n");
            at24c02_i2c_write(eeprom.addr, eeprom.data);
            break;
        default:
            return -1;
    }
    return 0;
}

static struct file_operations at24c02_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = at24c02_ioctl,
};

static struct miscdevice at24c02_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "at24c02",
    .fops = &at24c02_fops,
};

static int at24c02_probe(struct i2c_client *client, struct i2c_device_id *id)
{
    misc_register(&at24c02_dev);
    g_client = client;
}

static int at24c02_remove(struct i2c_client *client)
{
    misc_deregister(&at24c02_dev);
    return 0;
}

static struct i2c_driver at24c02_drv = {
    .driver = {
        .name = "at24c02",
    },
    .probe = at24c02_probe,
    .remove = at24c02_remove,
    .id_table = at24c02_id,
};

static int at24c02_init(void)
{
    //向I2C-CORE注册设备驱动
    i2c_add_driver(&at24c02_drv);
}

static int at24c02_exit(void)
{
    i2c_del_driver(&at24c02_drv);
}

module_init(at24c02_init);
module_exit(at24c02_exit);

MODULE_LICENSE("GPL");

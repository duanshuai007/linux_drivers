#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/cdev.h>

#define ADDRESS_MAX     1024-1
#define COUNT_MAX       1024

#define NUM_CLIENT      4

#define EEPROM_TYPE_16BIT_ADDR  2
#define EEPROM_TYPE_8BIT_ADDR   1

static struct i2c_device_id at24c08_id[] = {
    {"at24c08", 0x50},
};

static struct i2c_board_info at24c08_info = {
    I2C_BOARD_INFO("at24c08", 0x50),
};

struct at24c08_dev{
    unsigned int addr;
    char name[20];
    int current_pointer;
    struct semaphore sem;
    int smbus;
    int type;
    struct i2c_client *client[];    //可伸缩的结构体中到数组需要定义在最后
};

static struct at24c08_dev *at24c08;

static ssize_t at24c08_i2c_read(
        struct file *filp,
        char __user *buf,
        size_t count,
        loff_t *pos)
{
    if(count > COUNT_MAX)
    {
        printk("kernel:count must be < %d\r\n", COUNT_MAX);
        return -EINVAL;
    }
    int i = 0;
    int transferred = 0;
    int ret;
    char my_buff[1024] = {0};
    int client_no, addr;

    struct at24c08_dev *dev = (struct at24c08_dev *)filp->private_data;

    printk("read ceurrent pointer:%d\r\n", dev->current_pointer);

    while(transferred < count)
    {
        addr = (dev->current_pointer + i)%COUNT_MAX;
        if(addr < 256) client_no = 0;
        else if(addr >= 256 && addr < 512) client_no = 1;
        else if(addr >= 512 && addr < 768) client_no = 2;
        else if(addr >= 768 && addr < 1024) client_no = 3;

        ret = i2c_smbus_read_byte_data(dev->client[client_no], addr);
        my_buff[i++] = (unsigned char)ret;
        transferred++;
        msleep(1);
    }
    copy_to_user(buf, (void *)my_buff, transferred);
    dev->current_pointer += transferred;
    dev->current_pointer %= COUNT_MAX;

    return transferred;
}

static ssize_t at24c08_i2c_write(
        struct file *filp,
        char __user *buf,
        size_t count,
        loff_t *pos)
{
    if(count > COUNT_MAX)
    {
        printk("kernel:count must be < %d\r\n", COUNT_MAX);
        return -EINVAL;
    }
    int i = 0;
    int transferred = 0;
    int ret = 0;
    char my_buff[1024] = {0};
    int addr, client_no;

    struct at24c08_dev *dev = (struct at24c08_dev *)filp->private_data;
    //dev->current_pointer = *pos;
    printk("write current pointer:%d\r\n", dev->current_pointer);

    copy_from_user(my_buff, buf, count);
    while(transferred < count)
    {
        addr = (dev->current_pointer + i)%COUNT_MAX;
        if(addr < 256) client_no = 0;
        else if(addr >= 256 && addr < 512) client_no = 1;
        else if(addr >= 512 && addr < 768) client_no = 2;
        else if(addr >= 768 && addr < 1024) client_no = 3;

        ret = i2c_smbus_write_byte_data(dev->client[client_no],
                addr,
                my_buff[i]);
        i++;
        transferred++;
        msleep(1);
    }
    dev->current_pointer += transferred;
    dev->current_pointer %= COUNT_MAX;

    return transferred;
}

static int at24c08_i2c_open( struct inode *inode, struct file *filp )
{
    printk("kernel:i2c open\r\n");
    at24c08->current_pointer = 0;
    filp->private_data = at24c08;

    return 0;
}

static int at24c08_i2c_close(struct inode *inode, struct file *filp)
{
    printk("kernel:i2c release\r\n");
    filp->private_data = NULL;

    return 0;
}

//static struct i2c_client *g_client;
static loff_t at24c08_i2c_llseek(struct file *filp, loff_t poss, int flags)
{
    struct at24c08_dev *dev = (struct at24c08_dev *)filp->private_data;
    int ret = 0;

    switch(flags)
    {
        case SEEK_CUR:
            {
                if((poss + dev->current_pointer > ADDRESS_MAX) ||
                    (poss + dev->current_pointer < 0))
                {
                    ret = -EINVAL;
                    goto err;
                }
                dev->current_pointer += poss;
            }break;
        case SEEK_SET:
            {
                if((poss < 0) || (poss > ADDRESS_MAX))
                {
                    ret = -EINVAL;
                    goto err;
                }
                dev->current_pointer = poss;
            }break;
        case SEEK_END:
            {
                if(((ADDRESS_MAX + poss) < 0) || (poss > 0))
                {
                    ret = -EINVAL;
                    goto err;
                }
                dev->current_pointer = (ADDRESS_MAX + poss);
            }break;
        default:
            return -EINVAL;
    }
    printk("set current pointer:%d\r\n", dev->current_pointer);
    return dev->current_pointer;
err:
    return ret;
}

static long at24c08_ioctl(struct file *file,
        unsigned int cmd, unsigned long arg)
{


    return  0;
}

static struct file_operations at24c08_fops = {
    .owner      = THIS_MODULE,
    .open       = at24c08_i2c_open,
    .release    = at24c08_i2c_close,
    .write      = at24c08_i2c_write,
    .read       = at24c08_i2c_read,
    .llseek     = at24c08_i2c_llseek,
    .unlocked_ioctl = at24c08_ioctl,
};

static struct miscdevice at24c08_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "at24c08",
    .fops = &at24c08_fops,
};

static int at24c08_probe(struct i2c_client *client, struct i2c_device_id *id)
{
    int err;

    printk("kernel: at24c08 probe\r\n");

    at24c08->type = EEPROM_TYPE_8BIT_ADDR;
    //增加虚拟设备作为地址片选空间，AT24C08共有1024*8bit空间
    //通过P1，P0可以寻址4个地址空间，每个空间为256*8bit
    //00为基础的默认地址，其余三个分别是1，2，3
    for(int i=1; i < NUM_CLIENT; i++)
    {
        at24c08->client[i] = i2c_new_dummy(client->adapter,
                client->addr + i);
        if(at24c08->client[i] == NULL)
        {
            err = -EADDRINUSE;
            goto err;
        }
    }

    if(misc_register(&at24c08_dev) < 0)
    {
        err = -ENOENT;
        goto err;
    }

    return 0;

err:
    kfree(at24c08);
    return err;
}

static int at24c08_remove(struct i2c_client *client)
{
    int i;
    printk("kernel: at24c08 remove\r\n");
    misc_deregister(&at24c08_dev);
    kfree(at24c08);

    return 0;
}

static struct i2c_driver at24c08_drv = {
    .driver = {
        .name = "at24c08",
    },
    .probe = at24c08_probe,
    .remove = at24c08_remove,
    .id_table = at24c08_id,
};

static int at24c08_init(void)
{
    struct i2c_adapter *i2c_adap;
    //向I2C-CORE注册设备驱动
    printk("kernel: at24c08 init\r\n");

    at24c08 = kmalloc(sizeof(struct at24c08_dev)
            + NUM_CLIENT * sizeof(struct i2c_client *), GFP_KERNEL);
    if(at24c08 == NULL)
    {
        printk("kernel:system no mem\r\n");
        return -ENOMEM;
    }
    memset(at24c08, 0, sizeof(struct at24c08_dev));

    i2c_adap = i2c_get_adapter(0);  //从编号为0的I2C总线上获取I2C总线ADAP
    at24c08->client[0] = i2c_new_device(i2c_adap, &at24c08_info);
    i2c_put_adapter(i2c_adap);

    i2c_add_driver(&at24c08_drv);
    return 0;
}

static int at24c08_exit(void)
{
    printk("kernel: at24c08 exit\r\n");

    for(int i = 0; i < NUM_CLIENT; i++)
    {
        i2c_unregister_device(at24c08->client[i]);
    }

    i2c_del_driver(&at24c08_drv);
    return 0;
}

module_init(at24c08_init);
module_exit(at24c08_exit);

MODULE_LICENSE("GPL");

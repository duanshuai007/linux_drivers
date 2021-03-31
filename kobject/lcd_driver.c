#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
//#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>

/*
static char *name = "bigfish";
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "name,type: char *, permission:S_IRUGO");
static int watchdog = 1000;
module_param(watchdog, int, 0644);
MODULE_PARM_DESC(watchdog, "set watchdog timeout on million seconds");
*/

#define DEFAULT_MAJOR	0
//对应/dev/目录下的设备名称
#define DEVICE_NAME		"kobject_test_driver"
//对应/sys/目录下的驱动目录名称
#define OBJECT_PATH		"kobject_test"
#define OBJECT_CHILD_PATH	"child"

typedef struct lcd_t {
	int dev_major;
	struct class *class;
	struct device *device;
	struct kobject *kobj;
	struct semaphore sem;
	spinlock_t lock;
	int open_flag;
	char name[32];
	char buffer[1024];
} lcd_t;

static lcd_t *lcd = NULL;
static int g_value = 0;
//kobject OPS
//该函数被__ATTR_RO(name)自动扩展为${name}_show
static ssize_t led_show(struct kobject *kobjs, struct kobj_attribute *attr, char *buf) {
	printk(KERN_INFO "%s[%d]:val=%d\r\n", __func__, __LINE__, g_value);
	return sprintf(buf, "status = %d\r\n", g_value);
}

//这两个函数被_ATTR
static ssize_t led_status_show(struct kobject *kobjs, struct kobj_attribute *attr, char *buf) {
	printk(KERN_INFO "%s[%d]:val=%d\r\n", __func__, __LINE__, g_value);
	return sprintf(buf, "status = %d\r\n", g_value);
}

static ssize_t led_status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) 
{
	printk(KERN_INFO "%s[%d]:led status store\r\n", __func__, __LINE__);
	if (0 == memcmp(buf, "on", 2)) {
		g_value = 1;
	} else if (0 == memcmp(buf, "off", 3)) {
		g_value = 0;
	} else {
		printk(KERN_INFO "Not support cmd\r\n");
	}

	return count;
}

//对应/sys/OBJECT_PATH目录下的led
//在/sys/OBJECT_PATH/OBJECT_CHILD_PATH目录下生成一个只读属性的名为led的文件
static struct kobj_attribute status_attr = __ATTR_RO(led);
//对应/sys/OBJECT_PATH/OBJECT_CHILD_PATH目录下生成一个可以读写的名字为lcd_status的文件
static struct kobj_attribute led_attr = __ATTR( led_status, 0660, led_status_show, led_status_store);

static struct attribute *led_attrs[] = {
	&status_attr.attr,
	&led_attr.attr,
	NULL,
};

static struct attribute_group attr_g = {
	.name = OBJECT_CHILD_PATH,
	.attrs = led_attrs,
};
#if 0
static void pin_timer_handler(unsigned long data)
{
	printk(KERN_INFO "enter timer! data=%d", (int)data);
	
}
#endif

static int lcd_open(struct inode *inode, struct file *file)
{
	//struct lcd_t *lcd = NULL;
	//lcd = container_of(inode->i_cdev, struct lcd_t, dev_major);
		
	spin_lock(&lcd->lock);
	if (lcd->open_flag == 1) {
		printk("device busy\r\n");
		spin_unlock(&lcd->lock);
		return -EBUSY;
	}
	lcd->open_flag = 1;
	spin_unlock(&lcd->lock);

	file->private_data = (void *)lcd;

	printk("%s[%d]:name=%s\r\n", __func__, __LINE__, lcd->name);

	return 0;
}

static ssize_t lcd_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	int ret;
	lcd_t *p = (lcd_t *)file->private_data;

	printk("%s[%d]:name=%s\r\n", __func__, __LINE__, lcd->name);
	ret = copy_to_user(buf, p->name, strlen(p->name));
	if (ret == 0)
		return 0;
	else {
		printk(KERN_ALERT "error occur when reading!\r\n");
		return -EFAULT;
	}
	return -EINVAL;
}

static ssize_t lcd_write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
	int ret = 0;
	int len;
	
	lcd_t *p = (lcd_t *)file->private_data;

	if (down_interruptible(&p->sem)) {
		printk("%s[%d]:can not get sem\r\n", __func__, __LINE__);
		return -EBUSY;
	}

	if (count > sizeof(p->name)) {
		printk(KERN_ALERT "count too big, should less than\r\n");
		ret = -EFAULT;
	}
	
	len = count;

	memset(p->name, 0, sizeof(p->name));
	ret = copy_from_user(p->name, buf, count);
	if (ret == 0) {
		printk("%s[%d]:name=%s\r\n", __func__, __LINE__, lcd->name);
	} else {
		printk(KERN_ALERT "error occur when writing!\r\n");
		ret = -EFAULT;
	}

	up(&p->sem);

	return ret;
}

static int lcd_release(struct inode *node, struct file *file)
{
	lcd_t *lcd = (lcd_t *)file->private_data;
	printk(KERN_INFO "led release\r\n");
	
	if (lcd == NULL) {
		printk("lcd = NULL\r\n");
		return 0;
	}

	spin_lock(&lcd->lock);
	if (lcd->open_flag == 0) {
		printk("already close!\r\n");
		spin_unlock(&lcd->lock);
		return 0;
	}
	lcd->open_flag = 0;
	spin_unlock(&lcd->lock);

	file->private_data = NULL;

	return 0;
}

static struct file_operations led_ops = {
	.owner = THIS_MODULE,
	.open = lcd_open,
	.release = lcd_release,
	.read = lcd_read,
	.write = lcd_write,
};

static int __init lcd_driver_init(void)
{
	int ret = 0;

	printk(KERN_DEBUG "lcd driver!!!\n");

	lcd = kmalloc(sizeof(lcd_t), GFP_KERNEL);
	if (!lcd) {
		printk(KERN_ALERT "kmalloc failed\n");
		return -ENOMEM;
	}

	strcpy(lcd->name, "obj_test");
	lcd->dev_major = DEFAULT_MAJOR;

	sema_init(&lcd->sem, 1);
	spin_lock_init(&lcd->lock);
	//创建kobj结构体
	//在/sys/目录下生成OBJECT_PATH文件夹
	lcd->kobj = kobject_create_and_add(OBJECT_PATH, kernel_kobj->parent);
	if (!lcd->kobj) {
		printk(KERN_ALERT "kobject_create_and_add failed\n");
		ret = -ENOMEM;
		goto err_kobject_create;
	}
	
	if (sysfs_create_group(lcd->kobj, &attr_g)) {
		printk(KERN_ALERT "sysfs_create_group failed\n");
		ret = -EINVAL;
		goto err_sysfs_create;
	}

	//register_chrdev 内部封装了 __register_chrdev_region(), cdev_alloc(),kobject_set_name(),cdev_add()
	lcd->dev_major = register_chrdev(lcd->dev_major, DEVICE_NAME, &led_ops);
	if (lcd->dev_major < 0) {
		printk(KERN_ALERT "register chrdev failed\n");
		ret = -EINVAL;
		goto err_register;
	}
	
	printk(KERN_INFO "major = %d\n", lcd->dev_major);
	//CREATE DEVICE CLASS
	lcd->class = class_create(THIS_MODULE, DEVICE_NAME);	//对应/sys/class/spilcd
	if (IS_ERR(lcd->class)) {
		printk(KERN_ALERT "class create failed\n");
		ret = -EINVAL;
		goto err_class;
	}

	printk(KERN_INFO "devno=%d\n", MKDEV(lcd->dev_major, 0));
	lcd->device = device_create(lcd->class, NULL, MKDEV(lcd->dev_major, 0), NULL, DEVICE_NAME); //对应/dev/spilcd
	if (IS_ERR(lcd->device)) {
		printk(KERN_ALERT "device create failed\n");
		ret = -EINVAL;
		goto err_device_create;
	}

	//setup_timer(&lcd->pin_timer, &pin_timer_handler, 0);
	return 0;

err_device_create:
	class_destroy(lcd->class);
err_class:
	unregister_chrdev(MKDEV(lcd->dev_major, 0), DEVICE_NAME);
err_register:
err_sysfs_create:
	kobject_put(lcd->kobj);
err_kobject_create:
	kfree(lcd);

	return ret;
}

static void __exit lcd_driver_exit(void)
{
	printk(KERN_DEBUG "lcd driver exit!!!\n");

	kobject_put(lcd->kobj);

	device_destroy(lcd->class, MKDEV(lcd->dev_major, 0));
	class_destroy(lcd->class);
	unregister_chrdev(MKDEV(lcd->dev_major, 0), DEVICE_NAME);
}

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("duanshuai");
MODULE_DESCRIPTION("linux kernel driver - kobject example");
MODULE_VERSION("0.1");

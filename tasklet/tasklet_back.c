#include <linux/init.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/version.h>
//注册设备号所需要的头文件
#include <linux/fs.h>
//注册字符设备所需要的头文件
#include <linux/cdev.h>
//包含container_of宏
#include <linux/kernel.h>
//包含kmalloc和kfree
#include <linux/slab.h>
//包含cpoy_to_user和copy_from_user
#include <asm/uaccess.h>
//包含semaphore
#include <asm/semaphore.h>

//当前进程
//struct task_struct *current;
/*
    current->pid    进程id
    current->comm   进程命令名
 */

#define DEVICE_NAME     "aTest"
#define BUFFER_SIZE     256
#define MAJOR_MODE      0   //动态申请
//static dev_t dev_no = 0;
//static int major = 0;
//static struct cdev *pCdev = NULL;
//static char *kernel_buff = NULL;

//static char *whom = "world";
//static int howmany = 1;
//module_param(howmany, int, S_IRUGO);
//module_param(whom, charp, S_IRUGO);
//module_param_array(name, type, num, perm)

/*
    int (*open)(struct inode *inode, struct file *filp);
    ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
    ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);

 */
struct test_device {
    struct cdev cdev;
    struct semaphore sem;

    char *buffer;
    ssize_t buffer_size;
    
    dev_t dev_no;
    int major;
    int minor;
    
    char *device_name;
};

int test_open(struct inode *inode, struct file *filp)
{
    if (kernel_buff == NULL) {
        kernel_buff = kmalloc(BUFFER_SIZE, GFP_KERNEL);
        if (kernel_buff == NULL) {
            printk("kmalloc failed\r\n");
            return -ENOMEM;
        }
    }

    memset(kernel_buff, 0, BUFFER_SIZE);
    filp->f_pos = 0;
    printk("open test module\r\n");
    return 0;
}

int test_close(struct inode *inode, struct file *filp)
{
    if (kernel_buff) {
        kfree(kernel_buff);
        kernel_buff = NULL;
    }
    printk("close test module\r\n");
    return 0;
}

loff_t test_llseek(struct file *filp, loff_t offset, int flag)
{
    loff_t newpos;
    
    ssize_t ret = -ENOMEM;

    switch(flag) {
        case SEEK_END:
            //if (sizeof(kernel_buff) + offset < 0) {
            if (BUFFER_SIZE + offset < 0) {
                ret = -EFAULT;
                goto out;
            } else {
                //newpos = sizeof(kernel_buff) + offset;            
                newpos = BUFFER_SIZE + offset;            
            }
            break;
        case SEEK_SET:
            //if (offset > sizeof(kernel_buff)) {
            if (offset > BUFFER_SIZE) {
                ret = -EFAULT;
                goto out;
            } else {
                newpos = offset;
            }
            break;
        case SEEK_CUR:
            //if (filp->f_pos + offset > sizeof(kernel_buff)) {
            if (filp->f_pos + offset > BUFFER_SIZE) {
                ret = -EFAULT;
                goto out;
            } else {
                newpos = filp->f_pos + offset;
            }
            break;
        default:
            goto out;
            break;
    }

    //printk("set pos = %d\r\n", (int)newpos);

    filp->f_pos = newpos;
    ret = newpos;

out:
    //printk("offset to big1111, pos=%d, offset=%d\r\n", (int)(filp->f_pos), (int)offset);
    return ret;
}

ssize_t test_read(struct file *filp, char __user *buff, size_t count, loff_t *offset)
{
    ssize_t ret = -ENOMEM;
    //struct cdev *dev = filp->private_data;
    //if (*offset > sizeof(kernel_buff)) {
    if (*offset > BUFFER_SIZE) {
        goto out;
    }
    
    //if (count + *offset > sizeof(kernel_buff)) {
    if (count + *offset > BUFFER_SIZE) {
        //count = sizeof(kernel_buff) - *offset;
        count = BUFFER_SIZE - *offset;
    }

    //printk("read count=%d, offset=%d\r\n", (int)count, (int)(*offset));

    if (copy_to_user(buff, kernel_buff + *offset, count)) {
        ret = -EFAULT;
        goto out;
    }

    *offset += count;
    ret = count;

out:
    return ret;
}

ssize_t test_write(struct file *filp, const char __user *buff, size_t count, loff_t *offset)
{
    ssize_t ret = -ENOMEM;

    //if ( *offset >= sizeof(kernel_buff))
    if ( *offset >= BUFFER_SIZE)
        goto out;

    //if ( *offset + count > sizeof(kernel_buff)) {
    if ( *offset + count > BUFFER_SIZE) {
        //count = sizeof(kernel_buff) - *offset;
        count = BUFFER_SIZE - *offset;
    }

    //printk("write count:%d, offset=%d\r\n", (int)count, (int)(*offset));

    if(copy_from_user(kernel_buff + *offset, buff, count)) {
        ret = -EFAULT;
        goto out;
    }

    *offset += count;
    ret = count;

out:
    return ret;
}

struct file_operations test_fops = {
    .owner = THIS_MODULE,
    .open = test_open,
    .release = test_close,
    .write = test_write,
    .read = test_read,
    .llseek = test_llseek,
};

static int __init hello_init(void)
{
    //dev_t devno;
    int err;
    //int device_minor = 0;
	//printk(KERN_INFO "Hello, world\n");
	printk("module init\r\n");

    struct test_device *device = kmalloc(sizeof(struct test_device), GPL_KERNEL);
    if (device == NULL) {
        err = -ENOMEM;
        goto out1;
    }

    device->buffer_size = BUFFER_SIZE;
    device->buffer = kmalloc(BUFFER_SIZE, GPL_KERNEL);
    if (device->buffer == NULL) {
        err = -ENOMEM;
        goto out2;
    }

    device->device_name = kmalloc(strlen(DEVICE_NAME), GPL_KERNEL);
    if (device->device_name == NULL) {
        err = -ENOMEM;
        goto out3;
    }

    device->major = MAJOR_MODE;
    strcpy(device->device_name, DEVICE_NAME, strlen(device->device_name));

    //注册设备编号
    if (major == 0) {
        //动态申请
        //printk("动态申请设备号\r\n");
        err = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME);
        major = MAJOR(dev_no);
        //request_chrdev_region(devno, 1);
    } else {
        //静态申请
        //int major = MAJOR(siMajorDevno);
        //int minor = MINOR(siMajorDevno);
        dev_no = MKDEV(major, device_minor);
        err = register_chrdev_region(dev_no, 1, DEVICE_NAME);
    }
    
    if (err < 0) {
        printk("can't get major %d\r\n", major);
        return err;
    }

    //分配cdev空间
    pCdev = cdev_alloc();
    if (pCdev == NULL) {
        printk("cdev_alloc failed\r\n");
        return -1;
    }
    //printk("申请cdev空间\r\n");

    cdev_init(pCdev, &test_fops);
    //pCdev->owner = THIS_MODULE;
    //pCdev->ops = &test_fops;
    err = cdev_add(pCdev, dev_no, 1);
    if (err) {
        printk("cdev_add error\r\n");
        return err;
    }
    printk("success\r\n");
    return err;

out3:
    kfree(device->device_name);
out2:
    kfree(device->buffer);
out1:
    kfree(device);

	return err;
}

static void __exit hello_exit(void)
{
    //printk(KERN_INFO "Goodbye, cruel world\n");
    printk("module exit\r\n");
    //反注册设备编号
    //devno = MKDEV(device_major, device_minor);
    cdev_del(pCdev);
    unregister_chrdev_region(dev_no,  1);
    kfree(pCdev);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");


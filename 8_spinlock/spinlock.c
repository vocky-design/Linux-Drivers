/*
 * @Author: your name
 * @Date: 2021-10-06 17:06:28
 * @LastEditTime: 2021-10-11 18:35:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /5_gpioled/gpioled.c
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/spinlock.h>

#define GPIOLED_CNT     1
#define GPIOLED_NAME    "gpioled"

#define LEDON   1
#define LEDOFF  0

/* gpioled设备结构体 */
struct gpioled_dev{
    int major;
    int minor;
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;      /* 设备节点 */
    int gpios;                /* led所使用的GPIO编号 */
    int dev_status;              /* 设备状态：0设备未使用，>0设备已经使用 */
    spinlock_t lock;            /* 自旋锁 */
    
};
struct gpioled_dev gpioled;

/**
 * @description: 打开设备
 * @param {*}
 * @return {0成功，其他失败}
 */
static int led_open(struct inode *inode, struct file *filp)
{
    unsigned long flags;

    filp->private_data = &gpioled;  /* 设置私有数据 */

    spin_lock_irqsave(&gpioled.lock, flags);
    if(gpioled.dev_status) {/* 设备已经使用 */
        spin_unlock_irqrestore(&gpioled.lock, flags);
        return -EBUSY;
    }
    ++(gpioled.dev_status);
    spin_unlock_irqrestore(&gpioled.lock, flags);
    
    return 0;

}

/**
 * @description:从设备读取数据 
 * @param {*}
 * @return {*}
 */
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

/**
 * @description: 向设备写数据
 * @param {*}
 * @return {*}
 */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    struct gpioled_dev *dev = filp->private_data;
    unsigned char databuf[1];
    unsigned char ledstat;
    int ret;

    ret = copy_from_user(databuf, buf, cnt);
    if(ret < 0) {
        printk("kernel write failed!\n");
        return -EFAULT;
    }

    ledstat = databuf[0];

    if(ledstat == LEDON) {
        printk("LED open!\n");
        gpio_set_value(dev->gpios, 0);
    } else if(ledstat == LEDOFF) {
        printk("LED close!\n");
        gpio_set_value(dev->gpios, 1);
    }
    return 0;
    
}

/**
 * @description: 关闭/释放设备
 * @param {*}
 * @return {*}
 */
static int led_release(struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev = filp->private_data;
    unsigned long flags;
    spin_lock_irqsave(&dev->lock, flags);/* 上锁 */
    --(dev->dev_status);
    spin_unlock_irqrestore(&dev->lock, flags);/* 解锁 */
    return 0;
}

/* 设备操作函数 */
static struct file_operations gpioled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

/**
 * @description:驱动入口函数 
 * @param {*}
 * @return {*}
 */
static int __init gpioled_init(void)
{
    int ret;

    /* 初始化自旋锁 */
    spin_lock_init(&gpioled.lock);

    /* 设置LED所使用的GPIO */
    /* 1、获取设备节点： */
    gpioled.nd = of_find_node_by_path("/gpioled");
    if(gpioled.nd == NULL) {
        printk("gpioled node can't find!\n");
        return -EINVAL;
    } else {
        printk("gpioled node has been found!\n");
    }

    /* 2、获取设备树的gpip属性，得到LED所使用的LED编号 */
    gpioled.gpios = of_get_named_gpio(gpioled.nd, "gpios", 0);
    if(gpioled.gpios < 0) {
        printk("can't get gpios property!\n");
        return -EINVAL;
    } else {
        printk("gpios property = %d\n", gpioled.gpios);
    }

    /* 3、设置GPIO1_IO03为输出，并且输出高电平，默认关闭LED灯 */
    ret = gpio_direction_output(gpioled.gpios, 1);
    if(ret < 0) {
        printk("can't set gpio!\n");
    }

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    if(gpioled.major) {         /* 定义了主设备号 */
        gpioled.devid = MKDEV(gpioled.major, gpioled.minor);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    } else {
        alloc_chrdev_region(&gpioled.devid, gpioled.minor, GPIOLED_CNT, GPIOLED_NAME);
    }
    printk("gpioled major=%d, minor=%d\n", gpioled.major, gpioled.minor);

    /* 2、初始化cdev */
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);

    /* 3、添加一个cdev */
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

    /* 4、创建类 */
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if(IS_ERR(gpioled.class)) {
        return PTR_ERR(gpioled.class);
    }

    /* 5、创建设备 */
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if(IS_ERR(gpioled.device)) {
        return PTR_ERR(gpioled.device);
    }

    return 0;
}

/**
 * @description:驱动出口函数 
 * @param {*}
 * @return {*}
 */
static void __exit gpioled_exit(void) 
{
    /* 注销字符设备驱动 */
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);
}

module_init(gpioled_init);
module_exit(gpioled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("vocky");
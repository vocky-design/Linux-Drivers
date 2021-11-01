/*
 * @Author: your name
 * @Date: 2021-10-14 11:36:08
 * @LastEditTime: 2021-10-15 18:17:05
 * @LastEditors: Please set LastEditors
 * @Description: Linux内核定时器实验
 * @FilePath: /12_timer/timer.c
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TIMER_MAJOR     200
#define TIMER_CNT       1
#define TIMER_NAME      "timer"
#define CLOSE_CMD       (_IO(0XEF, 0X1))
#define OPEN_CMD        (_IO(0XEF, 0X2))
#define SETPERIOD_CMD   (_IO(0XEF, 0X3))

/* timer设备结构体 */
struct timer_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int gpio;
    unsigned int timerperiod;             /* 单位：ms */     
    struct timer_list timer;    /* 定义一个定时器 */
    spinlock_t lock;            /* 定义一个自旋锁 */
};
struct timer_dev timerdev;

static int led_init(void)
{
    int ret;
    /* 配置GPIO */
    timerdev.nd = of_find_node_by_path("/gpioled");
    if(timerdev.nd == NULL) {
        printk("Can't find node named 'gpioled'\n");
        return -EINVAL;
    }

    timerdev.gpio = of_get_named_gpio(timerdev.nd, "gpios", 0);
    if(timerdev.gpio < 0) {
        printk("Can't find property named 'gpios'\n");
        return -EINVAL;
    }

    gpio_request(timerdev.gpio, "led");
    ret = gpio_direction_output(timerdev.gpio, 1);
    if(ret < 0) {
        printk("Can't set gpio\n");
        return -EPERM;
    }

    return 0;
}

/**
 * @description: 打开设备，需要配置LED灯和定时器。
 * @param {*}
 * @return {*}
 */
static int timer_open(struct inode *inode, struct file *filp)
{
    int ret;
    filp->private_data = &timerdev;      /* 设置私有数据 */

    /* 设置周期，使定时器开始工作 */
    timerdev.timerperiod = 1000;          /* 默认周期为1s */

    ret = led_init();
    if(ret) return ret;


    return 0;

}

/**
 * @description: ioctl函数
 * @param {*}
 * @return {*}
 */
 static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
 {
     struct timer_dev *dev = (struct timer_dev *)filp->private_data;
     unsigned long flag;
     int timerperiod;

     switch(cmd) {
         case CLOSE_CMD:
            del_timer_sync(&dev->timer);
            break;
         case OPEN_CMD:
            spin_lock_irqsave(&dev->lock, flag);
            timerperiod = dev->timerperiod;
            spin_unlock_irqrestore(&dev->lock, flag);
            mod_timer(&dev->timer, jiffies+msecs_to_jiffies(timerperiod));
            break;
         case SETPERIOD_CMD:
            spin_lock_irqsave(&dev->lock, flag);
            dev->timerperiod = arg;
            spin_unlock_irqrestore(&dev->lock, flag);
            mod_timer(&dev->timer, jiffies+msecs_to_jiffies(arg));
            break;
         default:
            break;
     }

     return 0;
 }

static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .unlocked_ioctl = timer_unlocked_ioctl,
};

/**
 * @description: 定时器回调函数
 * @param {*}
 * @return {*}
 */
void timer_function(unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)arg;
    static int sta = 1;
    unsigned long flags;
    int timerperiod;

    sta = !sta;                         /* 每次都取反，实现LED灯反转 */
    gpio_set_value(dev->gpio, sta);

    /* 重启定时器 */
    spin_lock_irqsave(&dev->lock, flags);
    timerperiod = dev->timerperiod;
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->timer, jiffies+msecs_to_jiffies(timerperiod));
}

/**
 * @description: 驱动入口函数
 * @param {*}
 * @return {*}
 */
static int __init timer_init(void)
{
    /* 初始化自旋锁 */
    spin_lock_init(&timerdev.lock);

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    #ifdef TIMER_MAJOR
        timerdev.devid = MKDEV(TIMER_MAJOR, 0);
        register_chrdev_region(timerdev.devid, TIMER_CNT, TIMER_NAME);
    #else
        alloc_chrdev_region(&timerdev.devid, 0, TIMER_CNT, TIMER_NAME);
    #endif

    /* 2、初始化cdev */
    timerdev.cdev.owner = THIS_MODULE;
    cdev_init(&timerdev.cdev, &timer_fops);

    /* 3、添加一个cdev */
    cdev_add(&timerdev.cdev, timerdev.devid, TIMER_CNT);

    /* 4、创建类 */
    timerdev.class = class_create(THIS_MODULE, TIMER_NAME);
    if(IS_ERR(timerdev.class)) {
        return PTR_ERR(timerdev.class);
    }

    /* 5、创建设备 */
    timerdev.device = device_create(timerdev.class, NULL, timerdev.devid, NULL, TIMER_NAME);
    if(IS_ERR(timerdev.device)) {
        return PTR_ERR(timerdev.device);
    }

    /* 6、初始化timer，设置定时器处理函数，还未设置周期，所以不会激活定时器 */
    init_timer(&timerdev.timer);
    timerdev.timer.function = timer_function;
    timerdev.timer.data = (unsigned long)&timerdev;

    return 0;
}

/**
 * @description: 驱动出口函数
 * @param {*}
 * @return {*}
 */
static void __exit timer_exit(void)
{
    gpio_set_value(timerdev.gpio, 1);           /* 卸载驱动时关闭LED */
    del_timer_sync(&timerdev.timer);

    /* 卸载字符设备驱动 */
    cdev_del(&timerdev.cdev);
    unregister_chrdev_region(timerdev.devid, TIMER_CNT);
    
    device_destroy(timerdev.class, timerdev.devid);
    class_destroy(timerdev.class);
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("vocky");
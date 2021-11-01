/*
 * @Author: your name
 * @Date: 2021-10-15 21:17:20
 * @LastEditTime: 2021-10-20 20:29:46
 * @LastEditors: Please set LastEditors
 * @Description: 按键中断控制灯的亮灭(定时器消抖)
 * @FilePath: /13_irq/13_irq.c
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#define KEYMAJOR            200
#define KEYCNT              1
#define KEYNAME             "key"
#define LEDNAME             "led"
#define INTNAME             "key_int"
#define DELAYTIME           10              /* 单位：ms */ 

struct key_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;      
    struct timer_list timer;
    int key_gpio;
    int led_gpio;
    unsigned int irqnum;
    irqreturn_t (*irq_handler) (int, void *);           /* 中断处理函数 */
    atomic_t keyvalue;   
};
struct key_dev keydev;

static int key_open(struct inode *inode, struct file *filp) 
{
    filp->private_data = &keydev;   /* 设置私有数据 */
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = key_open,
};

/**
 * @description: 中断处理函数,在其中开启定时器，在timer_function中完成按键消抖任务。
 * @param {int} irq
 * @param {void} *dev
 * @return {*}
 */
static irqreturn_t key_handler(int irq, void *dev_id)
{
    struct key_dev *dev = (struct key_dev *)dev_id;

    /* 设置时间，启动定时器 */
    mod_timer(&dev->timer, jiffies+msecs_to_jiffies(DELAYTIME));

    return IRQ_RETVAL(IRQ_HANDLED);
}

/**
 * @description:定时器处理函数 
 * @param {*}
 * @return {*}
 */
void timer_function(unsigned long arg)
{
    struct key_dev *dev = (struct key_dev *)arg;
    unsigned char value;
    static int sta = 0;

    value = gpio_get_value(dev->key_gpio);
    if(value == 0) {                    /* 按下按键 */
        atomic_set(&dev->keyvalue, 0);
    } else {                            /* 按键松开 */
        atomic_set(&dev->keyvalue, 1);
        /* LED灯反转 */
        printk("sta1=%d\n", sta);
        sta = !!!sta;
        printk("sta2=%d\n", sta);
        gpio_set_value(dev->led_gpio, sta);
    }
}


static int Init_GpioInterrput(void)
{
    int ret;

    /* 获取设备节点 */
    keydev.nd = of_find_node_by_path("/key");
    if(keydev.nd == NULL) {
        printk("Can't find node named 'key'\n");
        return -EINVAL;
    } else {
        printk("node named 'key' has been found\n");
    }

    /* 获取GPIO号 */
    keydev.led_gpio = of_get_named_gpio(keydev.nd, "gpios", 0);
    if(keydev.led_gpio < 0) {
        printk("Can't get gpionum of led\n");
        return -EINVAL;
    } else {
        printk("gpionum of led is %d\n", keydev.led_gpio);
    }

    keydev.key_gpio = of_get_named_gpio(keydev.nd, "gpios", 1);
    if(keydev.key_gpio < 0) {
        printk("Can't get gpionum of key\n");
        return -EINVAL;
    } else {
        printk("gpionum of key is %d\n", keydev.key_gpio);
    }   

    /* 获取中断号 */
    keydev.irqnum = irq_of_parse_and_map(keydev.nd, 0);

    /* 初始化LED的GPIO */
    gpio_request(keydev.led_gpio, LEDNAME);
    ret = gpio_direction_output(keydev.led_gpio, 0);        /* 默认开启LED灯 */
    if(ret < 0) {
        printk("Can't set led output\n");
    }

    /* 初始化KEY的GPIO */
    gpio_request(keydev.key_gpio, KEYNAME);
    ret = gpio_direction_input(keydev.key_gpio);
    if(ret < 0) {
        printk("Can't set key input\n");
    }   

    /* 设置KEY的中断 */
    keydev.irq_handler = key_handler;
    ret = request_irq(keydev.irqnum, keydev.irq_handler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, INTNAME, &keydev);
    if(ret < 0) {
        printk("irq %d request failed\n", keydev.irqnum);
        return -EFAULT;
    }

    return 0;
     
}

static int __init key0_init(void)
{
    int ret;

    /* 创建字符设备 */
    /* 1、申请设备号 */
    #ifdef KEYMAJOR
        keydev.devid = MKDEV(KEYMAJOR, 0);
        register_chrdev_region(keydev.devid, KEYCNT, KEYNAME);
    #else
        alloc_chrdev_region(&keydev.devid, 0, KEYCNT, KEYNAME);
    #endif

    /* 2、注册字符设备 */
    cdev_init(&keydev.cdev, &fops);
    cdev_add(&keydev.cdev, keydev.devid, KEYCNT);

    /* 3、创建类 */
    keydev.class = class_create(THIS_MODULE, KEYNAME);
    if(IS_ERR(keydev.class)) {
        return PTR_ERR(keydev.class);
    }

    /* 4、创建设备 */
    keydev.device = device_create(keydev.class, NULL, keydev.devid, NULL, KEYNAME);
    if(IS_ERR(keydev.device)) {
        return PTR_ERR(keydev.device);
    }   

    /* 初始化按键值 */
    atomic_set(&keydev.keyvalue, 0XFF);             /* 无效按键值 */

    /* 初始化定时器 */
    init_timer(&keydev.timer);
    keydev.timer.function = timer_function;
    keydev.timer.data = (volatile unsigned long)&keydev;            /* volatile很有必要！ */

    /* 初始化GPIO和中断 */
    ret = Init_GpioInterrput();

    return ret;
}

static void __exit key0_exit(void)
{
    /* 释放定时器 */
    del_timer_sync(&keydev.timer);

    /* 释放中断 */
    free_irq(keydev.irqnum, &keydev);

    /* 释放IO */
    gpio_free(keydev.led_gpio);
    gpio_free(keydev.key_gpio);

    cdev_del(&keydev.cdev);
    unregister_chrdev_region(keydev.devid, KEYCNT);
    device_destroy(keydev.class, keydev.devid);
    class_destroy(keydev.class);
}

module_init(key0_init);
module_exit(key0_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("vokcy");

/*
 * @Author: your name
 * @Date: 2021-10-20 20:19:40
 * @LastEditTime: 2021-10-20 21:14:41
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /Linux_Drivers/20_input/20_input.c
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
#include <linux/input.h>

#define KEYCNT      1
#define KEYNAME     "key"
#define INTNAME     "key_int"
#define INPUTNAME   "key_input"
/* kyeinput设备结构体 */
struct keyinput_dev {
    dev_t devid;
    struct input_dev *inputdev;
    struct device_node *node;
    struct timer_list timer;
    void (*timer_function)(unsigned long );         //定时器处理函数
    unsigned int irqnum;
    irqreturn_t (*irq_handler) (int, void *);       //中断处理函数
    int gpio;

};
struct keyinput_dev keydev;

/**
 * @description:定时器处理函数 
 * @param {*}
 * @return {*}
 */
void timer_function(unsigned long arg)
{

}

/**
 * @description: 中断处理函数
 * @param {*}
 * @return {*}
 */
irqreturn_t irq_handler(int irqnum, void *dev_id)
{

}





/* 设备文件操作函数结构体 */
struct file_operations fops = {

}


/**
 * @description: 驱动入口函数
 * @param {*}
 * @return {*}
 */
int all_init(void)
{
    /* keydev函数指针初始化 */
    keydev.timer_function = timer_function;
    keydev.irq_handler = irq_handler;

    /* 定时器初始化 */
    init_timer(&keydev.timer);
    keydev.timer.function = timer_function;
    keydev.timer.data = (volatile unsigned long)&keydev;            /* volatile很有必要！ */

    /* 获取设备节点 */
    keydev.node = of_find_node_by_path(KEYNAME);
    if(keydev.node == NULL) {
        printk("key node not find!\n");
        return -EINVAL
    }

    /* 提取GPIO号和irq号 */
    keydev.gpio = of_get_named_gpio(keydev.node, "gpios", 1);
    if(keydev.gpio < 0) {
        printk("Can't get gpionum of key\n");
        return -EINVAL;        
    } else {
        printk("gpionum of key is %d\n", keydev.gpio);
    }
    keydev.irqnum = irq_of_parse_and_map(keydev.node, 0);

    /* 初始化GPIO */
    gpio_request(keydev.gpio, KEYNAME);
    ret = gpio_direction_input(keydev.gpio);
    if(ret < 0) {
        printk("Can't set key input\n");
    }

    /* 初始化GPIO上的中断 */
    ret = request_irq(keydev.irqnum, keydev.irq_handler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, INTNAME, &keydev);
    if(ret < 0) {
        printk("irq %d request failed\n", keydev.irqnum);
        return -EFAULT;
    }

    return 0;

}
static int __init keyinput_init(void)
{
    int ret;
    
    ret = all_init(void);
    if(ret < 0) {
        printk("error in _func_");
    }

    /* 申请inputdev */
    keydev.inputdev = input_allocate_device();
    keydev.inputdev->name = INPUTNAME;

    /* 设置INPUT的事件 */
    set_bit(EV_KEY, keydev.inputdev->evbit);    //按键事件
    set_bit(EV_REP, keydev.inputdev->evbit);    //重复事件

} 
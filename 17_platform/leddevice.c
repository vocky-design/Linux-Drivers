/*
 * @Author: your name
 * @Date: 2021-10-17 10:42:34
 * @LastEditTime: 2021-10-17 11:37:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /17_platform/leddevice.c
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
#include <linux/platform_device.h>
#include <linux/of_gpio.h>

/* 寄存器地址定义 */
#define CCM_CCGR1_BASE              (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE      (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE      (0X020E02F4)
#define GPIO1_DR_BASE               (0X0209C000)
#define GPIO1_GDIR_BASE             (0X0209C004)
#define REGISTER_LENGTH             4

/* 设备资源信息，也就是LED0所使用的所有寄存器 */
static struct resource led_resources[] = {
    [0] = {
        .start = CCM_CCGR1_BASE,
        .end = (CCM_CCGR1_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = SW_MUX_GPIO1_IO03_BASE,
        .end = (SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [2] = {
        .start = SW_PAD_GPIO1_IO03_BASE,
        .end = (SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [3] = {
        .start = GPIO1_DR_BASE,
        .end = (GPIO1_DR_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [4] = {
        .start = GPIO1_GDIR_BASE,
        .end = (GPIO1_GDIR_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
};

/**
 * @description:对struct device的realease函数
 * @param {*}
 * @return {*}
 */

void leddevice_realease(struct device *dev){
    printk("platform device named 'leddevice' has been realease\n");
}

/* platform设备结构体 */
static struct platform_device leddevice = {
    .name = "gpioled",
    .id = -1,           /* 表示此设备无ID */
    .dev = {
        .release = leddevice_realease,
    },
    .num_resources = ARRAY_SIZE(led_resources),
    .resource = led_resources,
};

/**
 * @description:设备模块加载 
 * @param {*}
 * @return {*}
 */
static int __init leddevice_init(void)
{
    int ret;
    ret = platform_device_register(&leddevice);
    if(ret < 0) {
        printk("platform device register failed\n");
        return -EFAULT;
    }

    return 0;
}

/**
 * @description:设备模块卸载 
 * @param {*}
 * @return {*}
 */
static void __exit leddevice_exit(void)
{
    platform_device_unregister(&leddevice);
}

module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("vocky");

/*
 * @Author: your name
 * @Date: 2021-10-17 11:53:49
 * @LastEditTime: 2021-10-17 12:44:23
 * @LastEditors: Please set LastEditors
 * @Description: 设备树下的 platform 驱动
 * @FilePath: /18_dtsplatform/ leddriver.c
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/timer.h>
#include <linux/semaphore.h>

#define LEDMAJOR    200
#define LEDCNT      1
#define LEDNAME     "imx6ul-led"    //TODO

/* led的设备结构体 */
struct led_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int led_gpio;
};
struct led_dev leddev;

/* 文件操作结构体 */
struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = ,
    .close = ,
}


/**
 * @description: led的probe函数
 * @param {struct platform_device *}
 * @return {int}
 */
static int led_probe(struct platform_device *)
{
    int ret;

    /* 1、申请设备号 */
    #ifdef LEDMAJOR
        leddev.devid = MKDEV(LEDMAJOR, 0);
        ret = register_chrdev_region(leddev.devid, LEDCNT, LEDNAME);
        if(ret < 0) {
            printk("device registe failed\n");
        }
    #else
        ret = alloc_chrdev_region(&leddev.devid, 0, LEDCNT, LEDNAME);
        if(ret < 0) {
            printk("device registe failed\n");
        }
    #endif

    /* 2、注册字符设备 */
    cdev_init(&leddev.cdev, &led_fops);
    cdev_add(&leddev.cdev, leddev.devid, LEDCNT);

    /* 3、创建类 */
    leddev.class = class_create(THIS_MODULE, LEDNAME);
    if (IS_ERR(leddev.class)) {
        return PTR_ERR(leddev.class);
    }

    /* 4、创建设备 */
    leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, LEDNAME);
    if (IS_ERR(leddev.device)) {
        return PTR_ERR(leddev.device);
    }

    /* 初始化LED的IO */
    leddev.node = of_find_node_by_path("/gpioled");
    if (leddev.node == NULL){
        printk("gpioled node not find\n");
        return -EINVAL;
    }

    leddev.led_gpio = of_get_named_gpio(leddev.node, "gpios", 0);
    if (leddev.led_gpio < 0) {
        printk("can't get gpios\n");
        return -EINVAL;
    }

    gpio_req(leddev.led_gpio, LEDNAME);
    gpio_direction_output(leddev.led_gpio, 1); /* 默认高电平，不亮 */

    return 0;
}

/**
 * @description: 
 * @param {struct platform_device *}
 * @return {int}
 */
static int led_remove(struct platform_device *)
{
    /* 卸载时，先关闭LED */
    gpio_set_value(leddev.led_gpio, 1);

    cdev_del(&leddev.cdev); /* 删除 cdev */
    unregister_chrdev_region(leddev.devid, LEDCNT);
    device_destroy(leddev.class, leddev.devid);
    class_destroy(leddev.class);

    return 0;
}

/* 匹配列表 */
static struct of_device_id led_of_match[] = {
    {.compatible = "atkalpha-gpioled"},
    {                                },
};

struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};
static struct platform_driver leddriver = {
    .driver = {
        .name = "imx6ul-led",           /* 驱动名字，用于和设备匹配 */
        .of_match_table = led_of_match, /* 设备树匹配表 */
    },
}
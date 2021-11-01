/*
 * @Author: your name
 * @Date: 2021-10-06 02:55:01
 * @LastEditTime: 2021-10-06 17:09:04
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /4_dtsled/dtsled.c
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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <linux/errno.h>

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

/* dtsled设备结构体 */
struct {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd; /* 设备节点 */

}dtsled_dev;

static int __init dtsled_init(void)
{
    int ret;
    struct property *proper;
    const char *str;
    u32 regdata[10];

    /* 获取设备树的属性数据 */
    /* 1、获取设备节点：alphaled */
    dtsled_dev.nd = of_find_node_by_path("/alphaled");
    if(dtsled_dev.nd == NULL) {
        printk("alphaled node can not found!\n");
        return -EINVAL;
    } else {
        printk("alphaled node has been found!\n");
    }

    /* 2、获取compatible属性内容 */
    proper = of_find_property(dtsled_dev.nd, "compatible", NULL);
    if(proper == NULL) {
        printk("compatible property find failed!\n");
    } else {
        printk("compatible = %s\n", (char *)proper->value);
    }

    /* 3、获取status属性内容 */
    ret = of_property_read_string(dtsled_dev.nd, "status", &str);
    if(ret < 0) {
        printk("status property read failed!\n");
    } else {
        printk("status = %s\n", str);
    }

    /* 获取reg属性内容 */
    of_property_read_u32_array(dtsled_dev.nd, "reg", regdata, 10);
    if(ret < 0) {
        printk("reg property read failed");
    } else {
        u8 i = 0;
        printk("reg data:\n");
        for(i = 0; i<10; ++i) {
            printk("%#X", regdata[i]);
        }
        printk("\n");
    }

    /* 初始化LED */
    /* 寄存器地址映射 */
#ifdef ADDR_MAP1
    IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
    SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
    GPIO1_DR = ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#elif ADDR_MAP2
    MX6U_CCM_CCGR1 = of_iomap(dtsled_dev.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
    GPIO1_DR = of_iomap(dtsled.nd, 3);
    GPIO1_GDIR = of_iomap(dtsled.nd, 4);
#endif

    /* 使能GPIO时钟 */

}

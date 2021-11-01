/*
 * @Author: your name
 * @Date: 2021-10-21 14:39:29
 * @LastEditTime: 2021-10-27 11:13:16
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /Linux_Drivers/21_iic/ap3216c.c
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
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "ap3216creg.h"


#define AP3216C_CNT     1
#define AP3216C_NAME    "ap3216c"


struct ap3216c_dev {
    dev_t               devid;
    struct cdev         cdev;
    struct class        *class;
    struct device       *device;
    struct device_node  *node;
    
    struct i2c_client   *client;
};
struct ap3216c_dev ap3216cdev;


/**
 * @description: 
 * @param {ap3216c_dev} *dev
 * @param {u8} reg
 * @param {void} *val
 * @param {int} len
 * @return {*}
 */
static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg, void *val, int len )
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = dev->client;
    /* msg[0]为发送要读取的首地址 */
    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &reg;
    msg[0].len = 1;
    /* msg[1]读取数据 */
    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter, msg, 2) ;
    if(ret != 2) {
        printk("i2c read failed=%d reg=%06x len=%d\n", ret, reg, len);
        return -EREMOTEIO;
    }

    return 0;
}

/**
 * @description: 
 * @param {ap3216c_dev} *dev
 * @param {u8} reg
 * @param {u8} *buf
 * @param {int} len
 * @return {*}
 */
static int ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, u8 *buf, int len)
{
    struct i2c_msg msg;
    struct i2c_client *client = dev->client;
    u8 *b = kzalloc(sizeof(u8)*(len+1) ,GFP_KERNEL);
    if(b == NULL) {
        printk("kzalloc error in %s", __func__);
        goto err_kzalloc;
    }

    b[0] = reg;                     /* 寄存器首地址 */
    memcpy(&b[1], buf, len);        /* 想要写入的数据 */

    msg.addr = client->addr;        /* ap3216c地址 */
    msg.flags = 0;                  /* 写数据 */
    msg.buf = b;
    msg.len = len+1;

    return i2c_transfer(client->adapter, &msg, 1) ;

    err_kzalloc:
        kfree(b);
        return -1; 
}

/**
 * @description: 读取ap3216c指定寄存器值
 * @param {ap3216c_dev} *dev
 * @param {u8} reg
 * @return {*}
 */
static u8 ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
    int ret;
    u8 data;
    ret = ap3216c_read_regs(dev, reg, &data, 1);
    if(ret != 1) {
        printk("read error in %s", __func__);
    }
    return data;
}

/**
 * @description: 设置ap3216c指定寄存器值
 * @param {ap3216c_dev} *dev
 * @param {u8} reg
 * @param {u8} data
 * @return {*}
 */
static int ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
    int ret;
    ret = ap3216c_write_regs(dev, reg, &data, 1);
    if(ret != 1) {
        printk("write error in %s", __func__);
    }
    return ret;
}

/**
 * @description: open函数
 * @param {inode} *inode
 * @param {file} *filp
 * @return {*}
 */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &ap3216cdev;

    /* 初始化ap3216c */
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x04); //0000 0100
    mdelay(50);     /* AP3216C 复位最少 10ms */
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x03); //0000 0011

    return 0;
}

/**
 * @description: read函数
 * @param {file} *filp
 * @param {char __user} *buf
 * @param {size_t} cnt
 * @param {loff_t} *loff
 * @return {*}
 */
static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *loff)
{
    int i;
    u8 data[6];
    u16 result[3];  //ir, als, ps
    struct ap3216c_dev *dev = (struct ap3216c_dev *)filp->private_data;
    for(i=0; i<6; ++i)
    {
        data[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW+i);
    }

    /* 对数据进行校验和整合 */
    if(data[0] & 0X80) /* IR_OF 位为 1,则数据无效 */
        result[0] = 0;
    else
        result[1] = ((unsigned short)data[1] << 2) | (data[0] & 0X03);
    
    result[2] = ((unsigned short)data[3] << 8) | data[2];
    
    if(data[4] & 0x40) /* IR_OF 位为 1,则数据无效 */
        result[2] = 0;
    else
        result[2] = ((unsigned short)(data[5] & 0X3F) << 4) | (data[4] & 0X0F);

    /* 返回给用户空间 */
    return copy_to_user(buf, result, sizeof(result));
}

/**
 * @description: release函数
 * @param {inode} *inode
 * @param {file} *filp
 * @return {*}
 */
static int ap3216c_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* 设备文件操作函数 */
struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read,
    .release = ap3216c_release,
};

/**
 * @description: 当驱动与设备匹配以后，probe函数就会执行
 * @param {i2c_client*} 
 * @param {i2c_device_id*} 
 * @return {int}
 */
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    /* 创建字符设备 */
    /* 1、申请设备号 */
    #ifdef AP3216C_MAJOR
        ap3216cdev.devid = MKDEV(AP3216C_MAJOR, 0);
        register_chrdev_region(ap3216cdev.devid, AP3216C_CNT, AP3216C_NAME);
    #else
        alloc_chrdev_region(&ap3216cdev.devid, 0, AP3216C_CNT, AP3216C_NAME);
    #endif

    /* 2、注册字符设备 */
    cdev_init(&ap3216cdev.cdev, &fops);
    cdev_add(&ap3216cdev.cdev, ap3216cdev.devid, AP3216C_CNT);   

    /* 3、创建类 */
    ap3216cdev.class = class_create(THIS_MODULE, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.class)) {
        return PTR_ERR(ap3216cdev.class);
    } 

    /* 4、创建设备 */
    ap3216cdev.device = device_create(ap3216cdev.class, NULL, ap3216cdev.devid, NULL, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.device)) {
        return PTR_ERR(ap3216cdev.device);
    }

    ap3216cdev.client = client;

    return 0;
}

/**
 * @description: 与probe函数对应
 * @param {i2c_client} *client
 * @return {int}
 */
static int ap3216cdev_remove(struct i2c_client *client)
{
    device_destroy(ap3216cdev.class, ap3216cdev.devid);
    class_destroy(ap3216cdev.class);
    cdev_del(&ap3216cdev.cdev);
    unregister_chrdev_region(ap3216cdev.devid, AP3216C_CNT);

    return 0;
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id ap3216c_id[] = {
    {"alientek,ap3216c",  (unsigned long)&ap3216cdev},
    {                                },
};

/* 设备树匹配列表 */
static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "alientek,ap3216c", .data = &ap3216cdev },
    {                                                       },
};

/* 定义IIC驱动结构体，并初始化 */
static struct i2c_driver ap3216c_driver = {
    .probe =    ap3216c_probe,
    .remove =   ap3216cdev_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ap3216c",
        .of_match_table = ap3216c_of_match,
    },
    .id_table = ap3216c_id,
};

module_i2c_driver(ap3216c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vocky");

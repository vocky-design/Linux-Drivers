/*
 * @Author: vocky
 * @Date: 2021-08-30 00:27:20
 * @LastEditTime: 2021-10-02 23:47:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /1_chrdevbase/chrdevbase.c
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>



#define CHARDEVBASE_MAJOR 200           /* 主设备名 */
#define CHARDEVBASE_NAME "chrdevbase"   /* 设备名 */

static char readbuf[100];               /* 读缓冲区 */
static char writebuf[100];              /* 写缓冲区 */
static char kerneldata[] = {"kernel data!"};


/**
 * @description: 打开设备
 * @param {inode：传递给驱动的inode
 *         filp:    file结构体有个叫private_data的成员变量，一般在open的时候将private_data指向设备结构体 }
 * @return {0成功，其他失败。}
 */
static int chrdevbase_open(struct inode *inode, struct file *filp)
{
    printk("chrdevbase open!\r\n");
    return 0;
};
/**
 * @description: 从设备读取数据
 * @param {flip}要打开的设备文件（文件描述符）
 * @param {buf}返回给用户空间的数据缓冲区
 * @param {cnt}要读取的数据长度
 * @param {offt}相对于文件首地址的偏移
 * @return {ssize_t}读取的字节数，如果为负值，表示读取失败
 */
static ssize_t chrdevbase_read(struct file *flip, char __user *buf, size_t cnt, loff_t *off_t)
{
    int retvalue = 0;
    /* 向用户空间发送数据 */
    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    retvalue = copy_to_user(buf, readbuf, cnt);
    if(retvalue == 0) {
        printk("kernel senddata ok!\r\n");
    } else {
        printk("kernel senddata failed!\r\n");
    }

    return 0;

}
/**
 * @description: 向设备写数据
 * @param {flip}要打开的设备文件（文件描述符）
 * @param {buf}要写给设备写入的数据
 * @param {cnt}要写入的数据长度
 * @param {offt}相对于文件首地址的偏移
 * @return {*}
 */
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue = 0;
    /* 接收用户空间传递给内核的数据并且打印出来 */
    retvalue = copy_from_user(writebuf, buf, cnt);
    if(retvalue == 0) {
        printk("kernel recevdata:%s\r\n", writebuf);
    } else {
        printk("kernel recevdata failed!\r\n");
    }

    return 0;
}
/**
 * @description: 关闭/释放设备
 * @param {flip}要关闭的设备文件（文件描述符）
 * @return {int}0成功，其他失败
 */
static int chrdevbase_release(struct inode *inode, struct file *flip)
{
    printk("chrdevbase released!\r\n");
    return 0;
}
/* 设备操作函数结构体 */
static struct file_operations chrdevbase_fops = {
	.owner = THIS_MODULE,	
	.open = chrdevbase_open,
	.read = chrdevbase_read,
	.write = chrdevbase_write,
	.release = chrdevbase_release,
};

static void __exit chrdevbase_exit(void)
{
    /* 注册字符设备驱动 */
    unregister_chrdev(CHARDEVBASE_MAJOR, CHARDEVBASE_NAME);
    printk("chrdevbase exit\r\n");
}

static int __init  chrdevbase_init(void)
{
    int retvalue;
    /*注册字符设备驱动  */
    retvalue = register_chrdev(CHARDEVBASE_MAJOR, CHARDEVBASE_NAME, &chrdevbase_fops);
    if(retvalue < 0) {
        printk("chrdevbase driver register failed\r\n");
    }
    printk("chrdevbase init \r\n");
    return 0;
}

module_init(chrdevbase_init);
module_exit(chrdevbase_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("dzl");

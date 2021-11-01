/*
 * @Author: your name
 * @Date: 2021-10-15 17:08:00
 * @LastEditTime: 2021-10-16 17:12:15
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /12_timer/timer_APP.c
 */
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/ioctl.h>
/* 命令值 */
#define CLOSE_CMD (_IO(0XEF, 0x1)) /* 关闭定时器 */
#define OPEN_CMD (_IO(0XEF, 0x2)) /* 打开定时器 */
#define SETPERIOD_CMD (_IO(0XEF, 0x3)) /* 设置定时器周期命令 */

int main(int argc, char *argv[])
{
    int fd, ret;
    char *filename;
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];
    if(argc != 2) {
        printf("Error Usage!\n");
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("Can't open file %s\n", filename);
        return -1;
    }

    while(1) {
        printf("Input CMD:");
        ret = scanf("%d", &cmd);

        if(cmd == 1) {
            cmd = CLOSE_CMD;
        } else if(cmd == 2) {
            cmd = OPEN_CMD;
        } else if(cmd == 3) {
            cmd = SETPERIOD_CMD;
            printf("Input Timer Period:");
            ret = scanf("%d", &arg);
        }

        ioctl(fd, cmd, arg);            /* 控制定时器的打开和关闭 */
    }

    close(fd);
}
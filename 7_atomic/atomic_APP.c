/*
 * @Author: your name
 * @Date: 2021-10-06 18:25:31
 * @LastEditTime: 2021-10-11 16:43:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /5_gpioled/gpioled_APP.c
 */
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define LEDOFF  0
#define LEDON   1

int main(int argc, char *argv[])
{
    int fd, ret;
    char *filename;
    unsigned char databuf[1];
    unsigned char cnt = 0;

    if(argc != 3) {
        printf("Error Usage!\n");
        return -1;
    }   

    filename = argv[1];
    /* 打开LED文件 */
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("file %s open failed!\n", filename);
        return -1;
    }

    databuf[0] = atoi(argv[2]);
    /* 向设备文件写入数据 */
    ret = write(fd, databuf, sizeof(databuf));
    if(ret < 0) {
        printf("LED Control Failed!\n");
        close(fd);
        return -1;
    }

    /* 模拟专用25sLED */
    while (1)
    {
        sleep(5);
        ++cnt;
        printf("APP running times: %d seconds\n", 5*cnt);
        if(cnt >= 5) break;
    }
    

    /* 关闭LED文件 */
    ret = close(fd);
    if(ret < 0) {
        printf("file %s close failed!\n", filename);
        return -1;
    }

    return 0;
}
/*
 * @Author: your name
 * @Date: 2021-10-21 21:01:44
 * @LastEditTime: 2021-10-21 21:12:32
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /Linux_Drivers/21_iic/ap3216cApp.c
 */
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int ret;

    int fd;
    char *filename;
    unsigned short databuf[3];

    if(argc != 2) {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("can't open file %s\r\n", filename);
        return -1;
    }

    while(1) {
        ret = read(fd, databuf, sizeof(databuf));
        if(ret == sizeof(databuf)) {
            printf("ir = %d, als = %d, ps = %d\n", databuf[0], databuf[1], databuf[2]);
        } else {
            printf("read error in %s", __func__);
            break;
        }
        usleep(200000);
    }

    close(fd);
    return 0;
}
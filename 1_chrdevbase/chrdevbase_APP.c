/*
 * @Author: your name
 * @Date: 2021-10-04 12:12:31
 * @LastEditTime: 2021-10-04 12:28:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /1_chrdevbase/chrdevbase_APP.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static char usrdata[] = {"usr data !"};

int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    char readbuf[100], writebuf[100];

    if(argc != 3) {
        printf("Error Usage!\n");
        return -1;
    }

    filename = argv[1];

    /* 打开驱动文件 */
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("Can't open file %s\n", filename);
        return -1;
    }

    /* 读或取 */
    if(atoi(argv[2]) == 1) {
        retvalue = read(fd, readbuf, 50);
        if(retvalue < 0) {
            printf("read file %s failed\n", filename);
        } else {
            printf("read data: %s\n", readbuf);
        }
    }
    if(atoi(argv[2]) == 2) {
        memcpy(writebuf, usrdata, sizeof(usrdata));
        retvalue = write(fd, writebuf, 50);
        if(retvalue < 0) {
            printf("write file %s failed!\n", filename);
        }
    }

    /* 关闭设备 */
    retvalue = close(fd);
    if(retvalue < 0) {
        printf("Can't close file %s\n", filename);
        return -1;
    }

    return 0;

}
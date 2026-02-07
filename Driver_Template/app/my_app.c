#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// [修改指南]: 指向正确的设备节点路径
#define DEVICE_PATH "/dev/my_template_device"

int main(int argc, char *argv[])
{
    int fd;

    printf("Starting application...\n");

    // 打开设备
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device");
        return -1;
    }

    printf("Device opened successfully (fd=%d)\n", fd);

    // [修改指南]: 在这里添加读写逻辑

    // 关闭设备
    close(fd);
    return 0;
}

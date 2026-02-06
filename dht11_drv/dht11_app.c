#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;
    unsigned char data[2]; // data[0]=湿度, data[1]=温度

    fd = open("/dev/dht11", O_RDONLY);
    if (fd < 0)
    {
        perror("Open device failed");
        return -1;
    }

    while (1)
    {
        int ret = read(fd, data, 2);
        if (ret == 2)
        {
            printf("Humidity: %d %%, Temperature: %d C\n", data[0], data[1]);
        }
        else
        {
            printf("Read failed (checksum or timeout)\n");
        }
        sleep(2); // DHT11 采样间隔建议大于1秒
    }

    close(fd);
    return 0;
}
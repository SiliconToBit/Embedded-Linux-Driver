#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fd;
    unsigned char val;

    if (argc != 2)
    {
        printf("Usage: %s <0|1>\n", argv[0]);
        printf("  0: Turn off LED\n");
        printf("  1: Turn on LED\n");
        return -1;
    }

    val = atoi(argv[1]);

    fd = open("/dev/led", O_WRONLY);
    if (fd < 0)
    {
        perror("Open failed");
        return -1;
    }

    if (write(fd, &val, 1) != 1)
    {
        perror("Write failed");
        close(fd);
        return -1;
    }

    printf("LED %s\n", val ? "ON" : "OFF");

    close(fd);
    return 0;
}

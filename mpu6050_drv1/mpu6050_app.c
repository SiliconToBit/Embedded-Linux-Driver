#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

struct mpu6050_sensor_data
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temp;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
};
int main(void)
{
    int fd;
    struct mpu6050_sensor_data data;
    int ret;

    fd = open("/dev/mpu6050", O_RDONLY);
    if (fd < 0)
    {
        perror("无法打开MPU6050设备");
        return -1;
    }

    printf("MPU6050传感器测试\n");

    while (1)
    {
        ret = read(fd, &data, sizeof(data));
        if (ret < 0)
        {
            perror("读取数据失败");
            close(fd);
            return -1;
        }

        printf("加速度: X=%d mg, Y=%d mg, Z=%d mg | \r\n", data.accel_x, data.accel_y, data.accel_z);
        printf("温度: %f °C | \r\n", (data.temp / 340.0) + 36.53);
        printf("陀螺仪: X=%d mdps, Y=%d mdps, Z=%d mdps\r\n", data.gyro_x, data.gyro_y, data.gyro_z);

        usleep(500000); // 延时500ms
    }

    close(fd);
    return 0;
}
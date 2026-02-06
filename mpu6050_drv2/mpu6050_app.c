#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

/* MPU6050 原始数据结构 */
struct mpu_raw_data
{
    uint8_t accel_x_h;
    uint8_t accel_x_l;
    uint8_t accel_y_h;
    uint8_t accel_y_l;
    uint8_t accel_z_h;
    uint8_t accel_z_l;
    uint8_t temp_h;
    uint8_t temp_l;
    uint8_t gyro_x_h;
    uint8_t gyro_x_l;
    uint8_t gyro_y_h;
    uint8_t gyro_y_l;
    uint8_t gyro_z_h;
    uint8_t gyro_z_l;
};

int main(int argc, char *argv[])
{
    int fd;
    struct mpu_raw_data raw;
    short accel_x, accel_y, accel_z;
    short temp;
    short gyro_x, gyro_y, gyro_z;
    float temp_c;

    fd = open("/dev/mpu6050_drv", O_RDONLY);
    if (fd < 0)
    {
        perror("Open device failed");
        return -1;
    }

    printf("Starting MPU6050 read loop...\n");

    while (1)
    {
        // 读取 14 字节
        if (read(fd, &raw, sizeof(raw)) == sizeof(raw))
        {

            // 数据合并 (大端转主机序)
            accel_x = (raw.accel_x_h << 8) | raw.accel_x_l;
            accel_y = (raw.accel_y_h << 8) | raw.accel_y_l;
            accel_z = (raw.accel_z_h << 8) | raw.accel_z_l;

            temp = (raw.temp_h << 8) | raw.temp_l;

            gyro_x = (raw.gyro_x_h << 8) | raw.gyro_x_l;
            gyro_y = (raw.gyro_y_h << 8) | raw.gyro_y_l;
            gyro_z = (raw.gyro_z_h << 8) | raw.gyro_z_l;

            // 温度转换公式: Temp_degC = ((Temp_out - RoomTemp_Offset)/Temp_Sensitivity) + 21degC
            // MPU6050 简化公式: Temp = raw / 340.0 + 36.53
            temp_c = temp / 340.0f + 36.53f;

            // 打印
            // \r 可以在同一行刷新，方便观察
            printf("Acc: %6d %6d %6d | Gyro: %6d %6d %6d | Temp: %.2f C \r",
                   accel_x, accel_y, accel_z,
                   gyro_x, gyro_y, gyro_z,
                   temp_c);
            fflush(stdout);
        }
        else
        {
            printf("Read error!\n");
        }

        usleep(100000); // 100ms 刷新一次
    }

    close(fd);
    return 0;
}
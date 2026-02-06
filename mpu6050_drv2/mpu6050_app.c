#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>

#define RAD_TO_DEG 57.295779513082320876

/* --- 卡尔曼滤波器结构体 --- */
typedef struct {
    float Q_angle; // 过程噪声协方差 (角度) - 越小越相信预测
    float Q_bias;  // 过程噪声协方差 (偏差)
    float R_measure; // 测量噪声协方差 - 越大越不相信加速度计
    
    float angle; // 输出角度
    float bias;  // 输出陀螺仪偏差
    float P[2][2]; // 误差协方差矩阵
} Kalman_t;

void Kalman_Init(Kalman_t *k) {
    k->Q_angle = 0.001f;
    k->Q_bias = 0.003f;
    k->R_measure = 0.03f;
    
    k->angle = 0.0f;
    k->bias = 0.0f;
    
    k->P[0][0] = 0.0f; k->P[0][1] = 0.0f;
    k->P[1][0] = 0.0f; k->P[1][1] = 0.0f;
}

// 卡尔曼核心算法
// newAngle: 加速度计计算出的角度
// newRate: 陀螺仪角速度
// dt: 采样周期
float Kalman_GetAngle(Kalman_t *k, float newAngle, float newRate, float dt) {
    // 1. 预测 (Predict)
    float rate = newRate - k->bias;
    k->angle += dt * rate;

    k->P[0][0] += dt * (dt*k->P[1][1] - k->P[0][1] - k->P[1][0] + k->Q_angle);
    k->P[0][1] -= dt * k->P[1][1];
    k->P[1][0] -= dt * k->P[1][1];
    k->P[1][1] += k->Q_bias * dt;

    // 2. 更新 (Update)
    float S = k->P[0][0] + k->R_measure; // 创新协方差
    float K[2]; // 卡尔曼增益
    K[0] = k->P[0][0] / S;
    K[1] = k->P[1][0] / S;

    float y = newAngle - k->angle; // 创新 (Innovation)
    
    k->angle += K[0] * y;
    k->bias += K[1] * y;

    float P00_temp = k->P[0][0];
    float P01_temp = k->P[0][1];

    k->P[0][0] -= K[0] * P00_temp;
    k->P[0][1] -= K[0] * P01_temp;
    k->P[1][0] -= K[1] * P00_temp;
    k->P[1][1] -= K[1] * P01_temp;

    return k->angle;
}

/* 必须与驱动层保持一致 */
struct mpu_raw_data
{
    uint8_t accel_x_h; uint8_t accel_x_l;
    uint8_t accel_y_h; uint8_t accel_y_l;
    uint8_t accel_z_h; uint8_t accel_z_l;
    uint8_t temp_h;    uint8_t temp_l;
    uint8_t gyro_x_h;  uint8_t gyro_x_l;
    uint8_t gyro_y_h;  uint8_t gyro_y_l;
    uint8_t gyro_z_h;  uint8_t gyro_z_l;
};

double get_time_sec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main(int argc, char *argv[])
{
    int fd;
    struct mpu_raw_data raw;
    short ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw, temp_raw;
    float ax, ay, az, gx, gy, gz, temp_c;
    
    // 实例化两个卡尔曼滤波器 (Roll 和 Pitch)
    Kalman_t kalmanX, kalmanY;
    Kalman_Init(&kalmanX);
    Kalman_Init(&kalmanY);

    double last_time, current_time, dt;

    fd = open("/dev/mpu6050", O_RDONLY);
    if (fd < 0) { perror("Open failed"); return -1; }

    printf("Starting Kalman Filter Fusion...\n");
    last_time = get_time_sec();

    while (1)
    {
        if (read(fd, &raw, sizeof(raw)) == sizeof(raw))
        {
            current_time = get_time_sec();
            dt = current_time - last_time;
            last_time = current_time;
            if (dt <= 0) dt = 0.01; // 防止除零

            // 数据解析
            ax_raw = (raw.accel_x_h << 8) | raw.accel_x_l;
            ay_raw = (raw.accel_y_h << 8) | raw.accel_y_l;
            az_raw = (raw.accel_z_h << 8) | raw.accel_z_l;
            gx_raw = (raw.gyro_x_h << 8) | raw.gyro_x_l;
            gy_raw = (raw.gyro_y_h << 8) | raw.gyro_y_l;
            gz_raw = (raw.gyro_z_h << 8) | raw.gyro_z_l;
            temp_raw = (raw.temp_h << 8) | raw.temp_l;

            // 物理单位转换
            ax = ax_raw / 16384.0f;
            ay = ay_raw / 16384.0f;
            az = az_raw / 16384.0f;
            gx = gx_raw / 131.0f; // deg/s
            gy = gy_raw / 131.0f;
            gz = gz_raw / 131.0f;
            temp_c = temp_raw / 340.0f + 36.53f;

            // --- 卡尔曼滤波步骤 ---

            // 1. 计算加速度计的角度 (观测值)
            // Roll (绕X轴): atan2(Y, Z)
            // Pitch (绕Y轴): atan2(-X, sqrt(Y^2 + Z^2))
            float acc_roll  = atan2(ay, az) * RAD_TO_DEG;
            float acc_pitch = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;

            // 2. 传入卡尔曼滤波器
            // 注意：GX 对应 Roll 的变化率，GY 对应 Pitch 的变化率
            float filter_roll = Kalman_GetAngle(&kalmanX, acc_roll, gx, dt);
            float filter_pitch = Kalman_GetAngle(&kalmanY, acc_pitch, gy, dt);

            // 打印
            printf("Roll: %6.1f | Pitch: %6.1f | Temp: %.1f \r", 
                   filter_roll, filter_pitch, temp_c);
            fflush(stdout);
        }
        else {
             usleep(5000);
        }
    }
    close(fd);
    return 0;
}

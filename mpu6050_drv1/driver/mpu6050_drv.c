#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/miscdevice.h>
#include <linux/jiffies.h>

#define DEV_NAME "mpu6050"
#define DEV_CNT 1
#define MPU6050_I2C_ADDR 0x68

#define MPU6050_SMPLRT_DIV 0x19
#define MPU6050_CONFIG 0x1A
#define MPU6050_GYRO_CONFIG 0X1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_ACCEL_XOUT_H 0X3B
#define MPU6050_ACCEL_XOUT_L 0x3C
#define MPU6050_ACCEL_YOUT_H 0X3D
#define MPU6050_ACCEL_YOUT_L 0X3E
#define MPU6050_ACCEL_ZOUT_H 0X3F
#define MPU6050_ACCEL_ZOUT_L 0x40
#define MPU6050_TEMP_OUT_H 0x41
#define MPU6050_TEMP_OUT_L 0x42
#define MPU6050_GYRO_XOUT_H 0x43
#define MPU6050_GYRO_XOUT_L 0x44
#define MPU6050_GYRO_YOUT_H 0X45
#define MPU6050_GYRO_YOUT_L 0x46
#define MPU6050_GYRO_ZOUT_H 0x47
#define MPU6050_GYRO_ZOUT_L 0x48
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_PWR_MGMT_2 0x6C
#define MPU6050_WHO_AM_I 0x75

/* MPU6050 WHO_AM_I value */
#define MPU6050_WHO_AM_I_ID 0x68

/* Integer scale denominators for fixed-point conversion
 * Output units:
 *  - accel: mg (1/1000 g)
 *  - gyro : mdps (1/1000 deg/s)
 *  - temp : m°C
 */
#define MPU6050_ACCEL_DENOM_2G 16384
#define MPU6050_ACCEL_DENOM_4G 8192
#define MPU6050_ACCEL_DENOM_8G 4096
#define MPU6050_ACCEL_DENOM_16G 2048

/* Gyro scale denominators scaled by 10 to avoid fractions
 * 250dps: 131.0 -> 1310, 500dps: 65.5 -> 655, 1000dps: 32.8 -> 328, 2000dps: 16.4 -> 164
 */
#define MPU6050_GYRO_DENOM10_250DPS 1310
#define MPU6050_GYRO_DENOM10_500DPS 655
#define MPU6050_GYRO_DENOM10_1000DPS 328
#define MPU6050_GYRO_DENOM10_2000DPS 164

#define MPU6050_TEMP_DENOM 340       /* LSB/°C */
#define MPU6050_TEMP_OFFSET_mC 36530 /* 36.53°C in milli-deg C */

struct mpu6050_dev
{
    struct i2c_client *client;
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    struct device_node *nd; /* 设备节点 */
    // void *private_data;      /* 私有数据 */
    /* 互斥锁 */
    struct mutex lock;

    bool initialized;
};

static struct mpu6050_dev mpu6050dev;

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

static int mpu6050_write_reg(struct mpu6050_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    int ret;
    struct i2c_client *client = dev->client;

    ret = i2c_master_send(client, buf, 2);
    if (ret < 0)
    {
        printk(KERN_DEBUG "mpu6050: Failed to write reg 0x%02x\n", reg);
        return -1;
    }

    return 0;
}

static int mpu6050_read_reg(struct mpu6050_dev *dev, uint8_t reg, uint8_t *val)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = dev->client;

    /* Write register address */
    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &reg;

    /* Read data */
    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 1;
    msg[1].buf = val;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret < 0)
    {
        printk(KERN_DEBUG "mpu6050: Failed to read reg 0x%02x\n", reg);
        return -1;
    }

    return 0;
}

static int mpu6050_read_bytes(struct mpu6050_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = dev->client;

    /* Write register address */
    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &reg;

    /* Read data */
    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = len;
    msg[1].buf = buf;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret < 0)
    {
        printk(KERN_DEBUG "mpu6050: Failed to read reg 0x%02x\n", reg);
        return -1;
    }

    return 0;
}

static int mpu6050_read_sensor_data(struct mpu6050_dev *dev, struct mpu6050_sensor_data *sensor_data)
{
    uint8_t buf[14];
    int ret;

    /* Read all sensor data in one transaction */
    ret = mpu6050_read_bytes(dev, MPU6050_ACCEL_XOUT_H, buf, 14);
    if (ret < 0)
        return -1;

    /* Parse accelerometer data (big-endian) */
    sensor_data->accel_x = (buf[0] << 8) | buf[1];
    sensor_data->accel_y = (buf[2] << 8) | buf[3];
    sensor_data->accel_z = (buf[4] << 8) | buf[5];

    /* Parse temperature data */
    sensor_data->temp = (buf[6] << 8) | buf[7];

    /* Parse gyroscope data */
    sensor_data->gyro_x = (buf[8] << 8) | buf[9];
    sensor_data->gyro_y = (buf[10] << 8) | buf[11];
    sensor_data->gyro_z = (buf[12] << 8) | buf[13];

    return 0;
}

static int mpu6050_init(struct mpu6050_dev *dev)
{
    uint8_t who_am_i;
    int ret;

    /* Check WHO_AM_I register */
    ret = mpu6050_read_reg(dev, MPU6050_WHO_AM_I, &who_am_i);
    if (ret < 0)
    {
        printk("mpu6050: Failed to read WHO_AM_I register\n");
        return -1;
    }

    if (who_am_i != MPU6050_WHO_AM_I_ID)
    {
        printk("mpu6050: WHO_AM_I mismatch: expected 0x%02x, got 0x%02x\n", MPU6050_WHO_AM_I_ID, who_am_i);
        return -1;
    }
    printk("mpu6050: WHO_AM_I register OK: 0x%02x\n", who_am_i);

    /* Reset device */
    ret = mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x80); // 复位设备
    if (ret)
        return ret;

    msleep(100);

    /* Wake up device and select clock source */
    ret = mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00); // 选择内部时钟
    if (ret < 0)
        return -1;

    /* Set sample rate to 1kHz by writing SMPLRT_DIV register */
    ret = mpu6050_write_reg(dev, MPU6050_SMPLRT_DIV, 0x07);
    if (ret < 0)
        return -1;

    /* Set accelerometer range to ±2g */
    ret = mpu6050_write_reg(dev, MPU6050_ACCEL_CONFIG, 0x06);
    if (ret < 0)
        return -1;

    /* Set gyroscope range to ±250°/s */
    ret = mpu6050_write_reg(dev, MPU6050_GYRO_CONFIG, 0x01);
    if (ret < 0)
        return -1;

    dev->initialized = true;
    printk("mpu6050: Initialization complete\n");

    return 0;
}

/*字符设备操作函数集，open函数实现*/
static int mpu6050_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &mpu6050dev;
    printk("mpu6050: Device opened\n");
    mpu6050_init(&mpu6050dev);
    return 0;
}
/*字符设备操作函数集，.read函数实现*/
static ssize_t mpu6050_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    struct mpu6050_dev *dev = (struct mpu6050_dev *)filp->private_data;
    struct mpu6050_sensor_data raw_data;
    int ret;

    /* We copy raw_data to userspace, so validate against its size */
    if (cnt < sizeof(raw_data))
        return -EINVAL;

    mutex_lock(&dev->lock);

    if (!dev->initialized)
    {
        mutex_unlock(&dev->lock);
        return -ENODEV;
    }

    /* Read raw sensor data */
    ret = mpu6050_read_sensor_data(dev, &raw_data);
    if (ret < 0)
    {
        mutex_unlock(&dev->lock);
        return ret;
    }

    /* Copy raw sensor data to user buffer */
    if (copy_to_user(buf, &raw_data, sizeof(raw_data)))
    {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    mutex_unlock(&dev->lock);
    return sizeof(raw_data);
}
/*字符设备操作函数集，.release函数实现*/
static int mpu6050_release(struct inode *inode, struct file *filp)
{
    printk("mpu6050: Device closed\n");
    return 0;
}
/*字符设备操作函数集*/
static struct file_operations mpu6050_chr_dev_fops =
    {
        .owner = THIS_MODULE,
        .open = mpu6050_open,
        .read = mpu6050_read,
        .release = mpu6050_release,
};

/*i2c总线设备函数集*/
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    mutex_init(&mpu6050dev.lock);
    printk("mpu6050 driver and device matched!\r\n");
    // 采用动态分配的方式，获取设备编号，次设备号为0，
    // 设备名称mpu6050，可通过命令cat  /proc/devices查看
    // DEV_CNT为1，当前只申请一个设备编号
    if (alloc_chrdev_region(&mpu6050dev.devid, 0, DEV_CNT, DEV_NAME) < 0)
    {
        printk("mpu6050: Failed to allocate char dev region\n");
        return -1;
    }

    cdev_init(&mpu6050dev.cdev, &mpu6050_chr_dev_fops);
    cdev_add(&mpu6050dev.cdev, mpu6050dev.devid, DEV_CNT);

    mpu6050dev.class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(mpu6050dev.class))
    {
        unregister_chrdev_region(mpu6050dev.devid, DEV_CNT);
        return PTR_ERR(mpu6050dev.class);
    }

    mpu6050dev.device = device_create(mpu6050dev.class, NULL, mpu6050dev.devid, NULL, DEV_NAME);
    if (IS_ERR(mpu6050dev.device))
    {
        class_destroy(mpu6050dev.class);
        unregister_chrdev_region(mpu6050dev.devid, DEV_CNT);
        return PTR_ERR(mpu6050dev.device);
    }

    mpu6050dev.client = client;

    return 0;
}
static void mpu6050_remove(struct i2c_client *client)
{
    /*删除设备*/
    device_destroy(mpu6050dev.class, mpu6050dev.devid);
    class_destroy(mpu6050dev.class);
    cdev_del(&mpu6050dev.cdev);
    unregister_chrdev_region(mpu6050dev.devid, DEV_CNT);
}

/* 传统匹配方式 ID 列表 */
static const struct i2c_device_id mpu6050_id[] = {
    {"gm,mpu6050", 0},
    {}};

/* 设备树匹配列表 */
static const struct of_device_id mpu6050_of_match_table[] = {
    {.compatible = "gm,mpu6050"},
    {/* sentinel */}};

/* i2c驱动结构*/
struct i2c_driver mpu6050_driver = {
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .id_table = mpu6050_id,
    .driver = {
        .name = "mpu6050_plat_drv",
        .owner = THIS_MODULE,
        .of_match_table = mpu6050_of_match_table,
    },
};

/*
 * 驱动初始化函数
 */
static int __init mpu6050_driver_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&mpu6050_driver);
    return ret;
}

/*
 * 驱动注销函数
 */
static void __exit mpu6050_driver_exit(void)
{
    i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_driver_init);
module_exit(mpu6050_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("guming");
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/interrupt.h> // 中断核心头文件
#include <linux/wait.h>      // 等待队列头文件
#include <linux/sched.h>

#define DRIVER_NAME "mpu6050"

// MPU6050 寄存器
#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_INT_PIN_CFG     0x37  // 中断引脚配置
#define REG_INT_ENABLE      0x38  // 中断使能
#define REG_INT_STATUS      0x3A  // 中断状态
#define REG_ACCEL_XOUT_H    0x3B
#define REG_PWR_MGMT_1      0x6B
#define REG_WHO_AM_I        0x75

struct mpu6050_dev
{
    dev_t dev_id;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct i2c_client *client;

    // --- 中断相关 ---
    int irq;                   // 中断号
    wait_queue_head_t read_wq; // 等待队列
    int data_ready;            // 数据就绪标志位 (1:有数据, 0:无数据)
};

/* 中断处理 Bottom Half (Threaded IRQ)
 * 运行在内核线程中，允许睡眠 (I2C 读写)
 */
static irqreturn_t mpu6050_irq_thread(int irq, void *dev_id)
{
    struct mpu6050_dev *mpu = dev_id;

    // 读取状态寄存器，清除中断标志
    // 虽然设置了 INT_RD_CLEAR，但这里显式读取更保险，能防止中断卡住
    i2c_smbus_read_byte_data(mpu->client, REG_INT_STATUS);

    // 1. 设置标志位
    mpu->data_ready = 1;

    // 2. 唤醒在 read 函数中睡觉的进程
    wake_up_interruptible(&mpu->read_wq);

    return IRQ_HANDLED;
}

static ssize_t mpu6050_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    struct mpu6050_dev *mpu = filp->private_data;
    u8 data[14];
    int ret;

    if (len != 14)
        return -EINVAL;

    // --- 阻塞等待 ---
    // wait_event_interruptible 宏解析：
    // 如果 data_ready 为 0，进程进入休眠，让出 CPU。
    // 当 wake_up 被调用且 data_ready 变为 1 时，进程被唤醒继续往下走。
    ret = wait_event_interruptible(mpu->read_wq, mpu->data_ready != 0);
    if (ret)
        return -ERESTARTSYS; // 被信号打断 (如 Ctrl+C)

    // --- 醒来干活 ---
    mpu->data_ready = 0; // 清除标志位

    // 读取数据 (I2C Block Read)
    ret = i2c_smbus_read_i2c_block_data(mpu->client, REG_ACCEL_XOUT_H, 14, data);
    if (ret < 0)
        return -EIO;

    // 拷贝给用户
    if (copy_to_user(buf, data, 14))
        return -EFAULT;

    return 14;
}

static int mpu6050_open(struct inode *inode, struct file *filp)
{
    struct mpu6050_dev *mpu = container_of(inode->i_cdev, struct mpu6050_dev, cdev);
    filp->private_data = mpu;
    return 0;
}

static const struct file_operations mpu6050_fops = {
    .owner = THIS_MODULE,
    .open = mpu6050_open,
    .read = mpu6050_read,
};

static int mpu6050_init_hw(struct i2c_client *client)
{
    // 1. 复位/唤醒
    i2c_smbus_write_byte_data(client, REG_PWR_MGMT_1, 0x00);

    // 2. 设置采样率 (比如 1kHz / (1+9) = 100Hz)
    i2c_smbus_write_byte_data(client, REG_SMPLRT_DIV, 9);

    // 3. 配置 DLPF (数字低通滤波), Bandwidth 44Hz
    i2c_smbus_write_byte_data(client, REG_CONFIG, 0x03);

    // --- 4. 中断配置 (关键) ---
    // 0x37 INT_PIN_CFG:
    // Bit 7 (INT_LEVEL): 1=Active Low, 0=Active High
    // Bit 5 (LATCH_INT_EN): 1=Latch until read, 0=50us pulse
    // Bit 4 (INT_RD_CLEAR): 1=Clear on read status or any read
    // 设置为 0xA0 (INT_LEVEL=1, LATCH_INT_EN=1)
    // Active Low + Latch + Edge Trigger Falling
    i2c_smbus_write_byte_data(client, REG_INT_PIN_CFG, 0xA0);

    // 0x38 INT_ENABLE: 先关闭中断！防止 request_irq 时此时引脚已经拉高导致中断风暴
    i2c_smbus_write_byte_data(client, REG_INT_ENABLE, 0x00);
    // 读一次状态寄存器，清除可能残留的 Status
    i2c_smbus_read_byte_data(client, REG_INT_STATUS);

    return 0;
}

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct mpu6050_dev *mpu;
    int ret;

    // 1. 申请内存
    mpu = devm_kzalloc(&client->dev, sizeof(*mpu), GFP_KERNEL);
    mpu->client = client;
    i2c_set_clientdata(client, mpu);

    // 2. 初始化等待队列
    init_waitqueue_head(&mpu->read_wq);

    // 3. 硬件初始化
    mpu6050_init_hw(client);

    // 4. 注册字符设备 (略写，参考之前代码)
    alloc_chrdev_region(&mpu->dev_id, 0, 1, DRIVER_NAME);
    cdev_init(&mpu->cdev, &mpu6050_fops);
    cdev_add(&mpu->cdev, mpu->dev_id, 1);
    mpu->class = class_create(THIS_MODULE, DRIVER_NAME);
    mpu->device = device_create(mpu->class, NULL, mpu->dev_id, NULL, DRIVER_NAME);

    // 5. 申请中断 (核心)
    // client->irq 会由内核自动解析 DTS 填入
    if (client->irq)
    {
        mpu->irq = client->irq;
        dev_info(&client->dev, "Requesting IRQ: %d\n", mpu->irq);

        // 申请中断
        // 改为 request_threaded_irq，允许在中断处理中执行 I2C 操作
        // IRQF_TRIGGER_FALLING: 下降沿触发 (配合 Active Low)
        // IRQF_ONESHOT: Threaded IRQ 必须加
        ret = devm_request_threaded_irq(&client->dev, mpu->irq,
                                        NULL,               // Primary handler (内核默认)
                                        mpu6050_irq_thread, // Thread handler
                                        IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_SHARED,
                                        DRIVER_NAME,
                                        mpu);
        if (ret)
        {
            dev_err(&client->dev, "Failed to request IRQ: %d\n", ret);
            return ret;
        }

        // 6. 最后一步：使能 MPU6050 内部中断
        // 此时 IRQ handler 已经注册好，硬件也准备好了
        i2c_smbus_write_byte_data(client, REG_INT_ENABLE, 0x01);
    }
    else
    {
        dev_err(&client->dev, "No IRQ provided in DTS\n");
        return -EINVAL;
    }

    dev_info(&client->dev, "MPU6050 Interrupt Driver Ready!\n");
    return 0;
}

static void mpu6050_remove(struct i2c_client *client)
{
    // devm_request_irq 会自动释放中断
    // 只需要销毁字符设备
    struct mpu6050_dev *mpu = i2c_get_clientdata(client);
    device_destroy(mpu->class, mpu->dev_id);
    class_destroy(mpu->class);
    cdev_del(&mpu->cdev);
    unregister_chrdev_region(mpu->dev_id, 1);
}

static const struct of_device_id mpu6050_of_match[] = {
    {.compatible = "my,mpu6050"},
    {}};
MODULE_DEVICE_TABLE(of, mpu6050_of_match);

static const struct i2c_device_id mpu6050_id[] = {
    {"mpu6050", 0},
    {}};
MODULE_DEVICE_TABLE(i2c, mpu6050_id);

static struct i2c_driver mpu6050_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = mpu6050_of_match,
    },
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .id_table = mpu6050_id,
};
module_i2c_driver(mpu6050_driver);
MODULE_LICENSE("GPL");
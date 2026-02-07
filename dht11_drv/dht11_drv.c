#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio/consumer.h> // 新版GPIO API
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/of.h>

#define DRIVER_NAME "dht11"

struct dht11_dev
{
    dev_t dev_id;           // 存放设备号 (主设备号+次设备号)
    struct cdev cdev;       // 内核字符设备的核心结构体
    struct class *class;    // 用于在 /sys/class 下创建分类
    struct device *device;  // 用于在 /dev 下创建节点
    struct gpio_desc *gpio; // 现代 GPIO 描述符 (替代旧的 int gpio_num)
    struct mutex lock;      // 互斥锁，保护设备访问
};

/* * DHT11 核心读取逻辑
 * 数据格式: 8bit湿度整数 + 8bit湿度小数 + 8bit温度整数 + 8bit温度小数 + 8bit校验
 */
static int dht11_read_sensor(struct dht11_dev *dht11, unsigned char *buf)
{
    int i, j;
    unsigned char data[5] = {0};
    unsigned long flags;
    int time_cnt;

    // 1. 发送开始信号
    gpiod_direction_output(dht11->gpio, 0); // 拉低
    mdelay(20);                             // 至少18ms
    gpiod_set_value(dht11->gpio, 1);        // 拉高
    udelay(30);                             // 延时20-40us

    // 2. 切换为输入模式准备读取
    gpiod_direction_input(dht11->gpio);

    // --- 关键时序区：关闭中断防止被抢占 ---
    local_irq_save(flags);

    // 3. 检测DHT11响应 (低电平80us -> 高电平80us)
    // 允许一定的宽容度等待传感器拉低总线
    time_cnt = 0;
    while (gpiod_get_value(dht11->gpio))
    {
        udelay(1);
        if (++time_cnt > 100) // 等待超过100us仍为高，说明无响应
        {
            local_irq_restore(flags);
            return -EIO; // 未响应
        }
    }

    // 等待低电平80us结束
    time_cnt = 0;
    while (!gpiod_get_value(dht11->gpio))
    {
        udelay(1);
        if (++time_cnt > 100)
        {
            local_irq_restore(flags);
            return -EIO; // 响应超时
        }
    }

    // 等待高电平80us结束 (准备传输bit)
    time_cnt = 0;
    while (gpiod_get_value(dht11->gpio))
    {
        udelay(1);
        if (++time_cnt > 100)
        {
            local_irq_restore(flags);
            return -EIO; // 响应超时
        }
    }

    // 4. 读取40位数据
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            // 等待每个bit的起始低电平(50us)结束
            time_cnt = 0;
            while (!gpiod_get_value(dht11->gpio))
            {
                udelay(1);
                if (++time_cnt > 100)
                {
                    local_irq_restore(flags);
                    return -EIO; // 数据传输超时
                }
            }

            // 判决：高电平持续时间决定是0(26-28us)还是1(70us)
            // 延时40us后采样
            udelay(40);

            if (gpiod_get_value(dht11->gpio))
            {
                data[i] |= (1 << (7 - j));
                // 等待剩余的高电平结束
                time_cnt = 0;
                while (gpiod_get_value(dht11->gpio))
                {
                    udelay(1);
                    if (++time_cnt > 100)
                    {
                        local_irq_restore(flags);
                        return -EIO; // 数据传输超时
                    }
                }
            }
        }
    }

    // --- 恢复中断 ---
    local_irq_restore(flags);

    // 5. 校验
    if (data[4] == (data[0] + data[1] + data[2] + data[3]))
    {
        // 校验成功，拷贝数据
        // buf[0]: 湿度, buf[1]: 温度 (DHT11小数部分通常为0)
        buf[0] = data[0];
        buf[1] = data[2];
        return 0;
    }

    return -EFAULT; // 校验失败
}

static ssize_t dht11_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    unsigned char data[2]; // 0: humi, 1: temp
    int ret;
    struct dht11_dev *dht11 = filp->private_data;

    if (len != 2)
        return -EINVAL;

    mutex_lock(&dht11->lock);

    ret = dht11_read_sensor(dht11, data);

    mutex_unlock(&dht11->lock);

    if (ret == 0)
    {
        ret = copy_to_user(buf, data, 2);
        return ret ? -EFAULT : 2;
    }
    else
    {
        return -EIO;
    }
}

static int dht11_open(struct inode *inode, struct file *filp)
{
    // inode->i_cdev 指向 struct cdev 类型的成员
    // 我们需要获取包含这个 cdev 的整个 dht11_dev 结构
    // 通过结构提成员反推结构体地址的 container_of 宏来实现
    struct dht11_dev *dht11 = container_of(inode->i_cdev, struct dht11_dev, cdev);
    // 将设备私有数据保存到 filp->private_data，供其他方法比如read、write等函数使用
    filp->private_data = dht11;
    return 0;
}


static const struct file_operations dht11_fops = {
    .owner = THIS_MODULE,
    .open = dht11_open,
    .read = dht11_read,
};

static int dht11_probe(struct platform_device *pdev)
{
    int ret;
    struct device *dev = &pdev->dev;
    struct dht11_dev *dht11;

    // devm_kzalloc 是 设备资源托管 的内存分配函数，会自动在设备移除时释放内存。
    dht11 = devm_kzalloc(dev, sizeof(*dht11), GFP_KERNEL);
    if (!dht11)
        return -ENOMEM;
    // 将私有数据保存到 pdev 中，方便驱动的 probe(), remove() 访问使用
    platform_set_drvdata(pdev, dht11);

    // 初始化锁
    mutex_init(&dht11->lock);

    // 1. 从DTS获取GPIO ("dht11-gpios")
    dht11->gpio = devm_gpiod_get(dev, "data", GPIOD_OUT_HIGH);
    if (IS_ERR(dht11->gpio))
    {
        dev_err(dev, "Failed to get GPIO\n");
        return PTR_ERR(dht11->gpio);
    }

    // 2. 注册字符设备，将设备和具体的操作函数关联起来
    ret = alloc_chrdev_region(&dht11->dev_id, 0, 1, DRIVER_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&dht11->cdev, &dht11_fops);
    ret = cdev_add(&dht11->cdev, dht11->dev_id, 1);
    if (ret < 0)
        goto fail_free;

    // 3. 创建节点，将内核内部的资源映射到用户空间的文件系统
    dht11->class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(dht11->class))
    {
        ret = PTR_ERR(dht11->class);
        goto fail_cdev;
    }

    dht11->device = device_create(dht11->class, NULL, dht11->dev_id, NULL, DRIVER_NAME);
    if (IS_ERR(dht11->device))
    {
        ret = PTR_ERR(dht11->device);
        goto fail_class;
    }

    dev_info(dev, "DHT11 Driver Probed!\n");
    return 0;

fail_class:
    class_destroy(dht11->class);
fail_cdev:
    cdev_del(&dht11->cdev);
fail_free:
    unregister_chrdev_region(dht11->dev_id, 1);
    return ret;
}

static int dht11_remove(struct platform_device *pdev)
{
    struct dht11_dev *dht11 = platform_get_drvdata(pdev);

    device_destroy(dht11->class, dht11->dev_id);
    class_destroy(dht11->class);
    cdev_del(&dht11->cdev);
    unregister_chrdev_region(dht11->dev_id, 1);
    return 0;
}

// 匹配DTS中的 compatible 属性
static const struct of_device_id dht11_match[] = {
    {.compatible = "my,dht11"},
    {/* sentinel */}};

// 设备树中有对应的节点时，系统会自动帮你把驱动加载进内存。
MODULE_DEVICE_TABLE(of, dht11_match);

// 驱动结构体
static struct platform_driver dht11_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = dht11_match,
    },
    .probe = dht11_probe,
    .remove = dht11_remove,
};

// 驱动模块入口和出口，减少驱动注册代码量
module_platform_driver(dht11_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gm");

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h> // 新版GPIO API
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "dht22-sensor"
#define DEVICE_NAME "dht22"
#define CLASS_NAME "dht22"
#define DHT22_MAX_RETRY 5
#define DHT22_TIMEOUT_US 200
#define DHT22_MIN_INTERVAL_MS 2000

struct dht22_dev
{
    dev_t dev_id;
    struct cdev cdev;
    struct class* class;
    struct device* device;
    struct gpio_desc* gpio;
    struct mutex lock;
    unsigned long last_read_time;
    unsigned char cached_data[4];
    bool data_valid;
};

static int dht22_read_sensor(struct dht22_dev* dht22, unsigned char* buf)
{
    int i, j;
    unsigned char data[5] = {0};
    unsigned long flags;
    int time_cnt;
    int retry;

    for (retry = 0; retry < DHT22_MAX_RETRY; retry++)
    {
        int ret = 0;

        if (retry > 0)
            msleep(100);

        gpiod_direction_output(dht22->gpio, 0);
        msleep(2);
        gpiod_set_value(dht22->gpio, 1);
        udelay(40);

        gpiod_direction_input(dht22->gpio);

        local_irq_save(flags);

        time_cnt = 0;
        while (gpiod_get_value(dht22->gpio))
        {
            udelay(1);
            if (++time_cnt > DHT22_TIMEOUT_US)
            {
                local_irq_restore(flags);
                ret = -EIO;
                goto retry_continue;
            }
        }

        time_cnt = 0;
        while (!gpiod_get_value(dht22->gpio))
        {
            udelay(1);
            if (++time_cnt > DHT22_TIMEOUT_US)
            {
                local_irq_restore(flags);
                ret = -EIO;
                goto retry_continue;
            }
        }

        time_cnt = 0;
        while (gpiod_get_value(dht22->gpio))
        {
            udelay(1);
            if (++time_cnt > DHT22_TIMEOUT_US)
            {
                local_irq_restore(flags);
                ret = -EIO;
                goto retry_continue;
            }
        }

        for (i = 0; i < 5; i++)
        {
            for (j = 0; j < 8; j++)
            {
                time_cnt = 0;
                while (!gpiod_get_value(dht22->gpio))
                {
                    udelay(1);
                    if (++time_cnt > DHT22_TIMEOUT_US)
                    {
                        local_irq_restore(flags);
                        ret = -EIO;
                        goto retry_continue;
                    }
                }

                udelay(35);

                if (gpiod_get_value(dht22->gpio))
                {
                    data[i] |= (1 << (7 - j));
                    time_cnt = 0;
                    while (gpiod_get_value(dht22->gpio))
                    {
                        udelay(1);
                        if (++time_cnt > DHT22_TIMEOUT_US)
                        {
                            local_irq_restore(flags);
                            ret = -EIO;
                            goto retry_continue;
                        }
                    }
                }
            }
        }

        local_irq_restore(flags);

        if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
        {
            buf[0] = data[0];
            buf[1] = data[1];
            buf[2] = data[2];
            buf[3] = data[3];
            return 0;
        }

    retry_continue:
        if (ret == -EIO)
            continue;
    }

    return -EFAULT;
}

static ssize_t dht22_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    unsigned char data[4];
    int ret;
    struct dht22_dev* dht22 = filp->private_data;
    unsigned long current_time = jiffies;

    if (len != 4)
        return -EINVAL;

    mutex_lock(&dht22->lock);

    if (time_after(current_time, dht22->last_read_time + msecs_to_jiffies(DHT22_MIN_INTERVAL_MS)) ||
        !dht22->data_valid)
    {
        ret = dht22_read_sensor(dht22, data);
        if (ret == 0)
        {
            dht22->cached_data[0] = data[0];
            dht22->cached_data[1] = data[1];
            dht22->cached_data[2] = data[2];
            dht22->cached_data[3] = data[3];
            dht22->last_read_time = current_time;
            dht22->data_valid = true;
        }
    }
    else
    {
        data[0] = dht22->cached_data[0];
        data[1] = dht22->cached_data[1];
        data[2] = dht22->cached_data[2];
        data[3] = dht22->cached_data[3];
        ret = 0;
    }

    mutex_unlock(&dht22->lock);

    if (ret == 0)
    {
        ret = copy_to_user(buf, data, 4);
        return ret ? -EFAULT : 4;
    }
    else
    {
        return -EIO;
    }
}

static int dht22_open(struct inode* inode, struct file* filp)
{
    struct dht22_dev* dht22 = container_of(inode->i_cdev, struct dht22_dev, cdev);
    filp->private_data = dht22;
    return 0;
}

static const struct file_operations dht22_fops = {
    .owner = THIS_MODULE,
    .open = dht22_open,
    .read = dht22_read,
};

static int dht22_probe(struct platform_device* pdev)
{
    int ret;
    struct device* dev = &pdev->dev;
    struct dht22_dev* dht22;

    dht22 = devm_kzalloc(dev, sizeof(*dht22), GFP_KERNEL);
    if (!dht22)
        return -ENOMEM;
    platform_set_drvdata(pdev, dht22);

    mutex_init(&dht22->lock);
    dht22->data_valid = false;

    dht22->gpio = devm_gpiod_get(dev, "data", GPIOD_OUT_HIGH);
    if (IS_ERR(dht22->gpio))
    {
        dev_err(dev, "Failed to get GPIO\n");
        return PTR_ERR(dht22->gpio);
    }

    ret = alloc_chrdev_region(&dht22->dev_id, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&dht22->cdev, &dht22_fops);
    ret = cdev_add(&dht22->cdev, dht22->dev_id, 1);
    if (ret < 0)
        goto fail_free;

    dht22->class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(dht22->class))
    {
        ret = PTR_ERR(dht22->class);
        goto fail_cdev;
    }

    dht22->device = device_create(dht22->class, NULL, dht22->dev_id, NULL, DEVICE_NAME);
    if (IS_ERR(dht22->device))
    {
        ret = PTR_ERR(dht22->device);
        goto fail_class;
    }

    dev_info(dev, "DHT22 Driver Probed!\n");
    return 0;

fail_class:
    class_destroy(dht22->class);
fail_cdev:
    cdev_del(&dht22->cdev);
fail_free:
    unregister_chrdev_region(dht22->dev_id, 1);
    return ret;
}

static int dht22_remove(struct platform_device* pdev)
{
    struct dht22_dev* dht22 = platform_get_drvdata(pdev);

    device_destroy(dht22->class, dht22->dev_id);
    class_destroy(dht22->class);
    cdev_del(&dht22->cdev);
    unregister_chrdev_region(dht22->dev_id, 1);
    return 0;
}

static const struct of_device_id dht22_match[] = {{.compatible = "my,dht11"}, {}};

MODULE_DEVICE_TABLE(of, dht22_match);

static struct platform_driver dht22_driver = {
    .driver =
        {
            .name = DRIVER_NAME,
            .of_match_table = dht22_match,
        },
    .probe = dht22_probe,
    .remove = dht22_remove,
};

module_platform_driver(dht22_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gm");

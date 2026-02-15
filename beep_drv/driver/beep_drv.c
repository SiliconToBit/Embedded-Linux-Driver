#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "beep"
#define DEVICE_NAME "beep"
#define CLASS_NAME "beep"

struct beep_dev
{
    dev_t dev_id;
    struct cdev cdev;
    struct class* class;
    struct device* device;
    struct gpio_desc* beep_gpio;
};

static ssize_t beep_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
    u8 val;
    int ret;
    struct beep_dev* beep = filp->private_data;

    if (len != 1)
        return -EINVAL;

    ret = copy_from_user(&val, buf, 1);
    if (ret)
        return -EFAULT;

    gpiod_set_value(beep->beep_gpio, val ? 1 : 0);

    return 1;
}

static int beep_open(struct inode* inode, struct file* filp)
{
    struct beep_dev* beep = container_of(inode->i_cdev, struct beep_dev, cdev);
    filp->private_data = beep;
    return 0;
}

static const struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .write = beep_write,
};

static int beep_probe(struct platform_device* pdev)
{
    int ret;
    struct device* dev = &pdev->dev;
    struct beep_dev* beep;

    dev_info(dev, "Beep driver probing...\n");

    beep = devm_kzalloc(dev, sizeof(*beep), GFP_KERNEL);
    if (!beep)
        return -ENOMEM;

    platform_set_drvdata(pdev, beep);

    beep->beep_gpio = devm_gpiod_get(dev, "beep", GPIOD_OUT_LOW);
    if (IS_ERR(beep->beep_gpio))
    {
        dev_err(dev, "Failed to get beep GPIO: %ld\n", PTR_ERR(beep->beep_gpio));
        return PTR_ERR(beep->beep_gpio);
    }

    ret = alloc_chrdev_region(&beep->dev_id, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&beep->cdev, &beep_fops);
    ret = cdev_add(&beep->cdev, beep->dev_id, 1);
    if (ret < 0)
        goto fail_free;

    beep->class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(beep->class))
    {
        ret = PTR_ERR(beep->class);
        goto fail_cdev;
    }

    beep->device = device_create(beep->class, NULL, beep->dev_id, NULL, DEVICE_NAME);
    if (IS_ERR(beep->device))
    {
        ret = PTR_ERR(beep->device);
        goto fail_class;
    }

    dev_info(dev, "Beep driver probed successfully!\n");
    return 0;

fail_class:
    class_destroy(beep->class);
fail_cdev:
    cdev_del(&beep->cdev);
fail_free:
    unregister_chrdev_region(beep->dev_id, 1);
    return ret;
}

static int beep_remove(struct platform_device* pdev)
{
    struct beep_dev* beep = platform_get_drvdata(pdev);

    gpiod_set_value(beep->beep_gpio, 0);

    device_destroy(beep->class, beep->dev_id);
    class_destroy(beep->class);
    cdev_del(&beep->cdev);
    unregister_chrdev_region(beep->dev_id, 1);
    dev_info(&pdev->dev, "Beep driver removed\n");
    return 0;
}

static const struct of_device_id beep_of_match[] = {{.compatible = "my,beep"}, {}};
MODULE_DEVICE_TABLE(of, beep_of_match);

static struct platform_driver beep_driver = {
    .driver =
        {
            .name = DRIVER_NAME,
            .of_match_table = beep_of_match,
        },
    .probe = beep_probe,
    .remove = beep_remove,
};

module_platform_driver(beep_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gm");

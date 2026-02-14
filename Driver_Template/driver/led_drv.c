#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "led"

struct led_dev
{
    dev_t dev_id;
    struct cdev cdev;
    struct class* class;
    struct device* device;
    struct gpio_desc* led_gpiod;
};

static ssize_t led_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
    struct led_dev* led = filp->private_data;
    int val;
    int ret;

    if (len != 1)
        return -EINVAL;

    ret = copy_from_user(&val, buf, 1);
    if (ret)
        return -EFAULT;

    if (val == 0)
        gpiod_set_value(led->led_gpiod, 0);
    else
        gpiod_set_value(led->led_gpiod, 1);

    return 1;
}

static int led_open(struct inode* inode, struct file* filp)
{
    struct led_dev* led = container_of(inode->i_cdev, struct led_dev, cdev);
    filp->private_data = led;
    return 0;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
};

static int led_probe(struct platform_device* pdev)
{
    struct led_dev* led;

    dev_info(&pdev->dev, "LED driver probing...\n");

    led = devm_kzalloc(&pdev->dev, sizeof(*led), GFP_KERNEL);
    if (!led)
        return -ENOMEM;

    platform_set_drvdata(pdev, led);

    led->led_gpiod = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led->led_gpiod))
    {
        dev_err(&pdev->dev, "Failed to get LED GPIO\n");
        return PTR_ERR(led->led_gpiod);
    }

    alloc_chrdev_region(&led->dev_id, 0, 1, DRIVER_NAME);
    cdev_init(&led->cdev, &led_fops);
    cdev_add(&led->cdev, led->dev_id, 1);
    led->class = class_create(THIS_MODULE, DRIVER_NAME);
    led->device = device_create(led->class, NULL, led->dev_id, NULL, DRIVER_NAME);

    dev_info(&pdev->dev, "LED driver probed successfully!\n");
    return 0;
}

static int led_remove(struct platform_device* pdev)
{
    struct led_dev* led = platform_get_drvdata(pdev);
    device_destroy(led->class, led->dev_id);
    class_destroy(led->class);
    cdev_del(&led->cdev);
    unregister_chrdev_region(led->dev_id, 1);
    dev_info(&pdev->dev, "LED driver removed\n");
    return 0;
}

static const struct of_device_id led_of_match[] = {{.compatible = "my,led"}, {}};
MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_driver = {
    .driver =
        {
            .name = DRIVER_NAME,
            .of_match_table = led_of_match,
        },
    .probe = led_probe,
    .remove = led_remove,
};

module_platform_driver(led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gm");

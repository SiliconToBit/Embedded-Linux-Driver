#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "beep"

struct beep_dev {
  dev_t dev_id;
  struct cdev cdev;
  struct class *class;
  struct device *device;
  int beep_gpio;
};

static struct beep_dev *beep;

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t len,
                          loff_t *off) {
  int val;
  int ret;

  if (len != 1)
    return -EINVAL;

  ret = copy_from_user(&val, buf, 1);
  if (ret)
    return -EFAULT;

  if (val == 0)
    gpio_set_value(beep->beep_gpio, 0);
  else
    gpio_set_value(beep->beep_gpio, 1);

  return 1;
}

static int beep_open(struct inode *inode, struct file *filp) {
  filp->private_data = beep;
  return 0;
}

static const struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .write = beep_write,
};

static int beep_probe(struct platform_device *pdev) {
  int ret;
  struct device_node *np = pdev->dev.of_node;

  dev_info(&pdev->dev, "Beep driver probing...\n");

  beep = devm_kzalloc(&pdev->dev, sizeof(*beep), GFP_KERNEL);
  if (!beep)
    return -ENOMEM;

  platform_set_drvdata(pdev, beep);

  beep->beep_gpio = of_get_named_gpio(np, "beep-gpio", 0);
  if (!gpio_is_valid(beep->beep_gpio)) {
    dev_err(&pdev->dev, "Invalid beep GPIO\n");
    return -EINVAL;
  }

  ret = devm_gpio_request(&pdev->dev, beep->beep_gpio, "beep");
  if (ret) {
    dev_err(&pdev->dev, "Failed to request beep GPIO\n");
    return ret;
  }

  gpio_direction_output(beep->beep_gpio, 0);

  alloc_chrdev_region(&beep->dev_id, 0, 1, DRIVER_NAME);
  cdev_init(&beep->cdev, &beep_fops);
  cdev_add(&beep->cdev, beep->dev_id, 1);
  beep->class = class_create(THIS_MODULE, DRIVER_NAME);
  beep->device =
      device_create(beep->class, NULL, beep->dev_id, NULL, DRIVER_NAME);

  dev_info(&pdev->dev, "Beep driver probed successfully!\n");
  return 0;
}

static int beep_remove(struct platform_device *pdev) {
  device_destroy(beep->class, beep->dev_id);
  class_destroy(beep->class);
  cdev_del(&beep->cdev);
  unregister_chrdev_region(beep->dev_id, 1);
  dev_info(&pdev->dev, "Beep driver removed\n");
  return 0;
}

static const struct of_device_id beep_of_match[] = {{.compatible = "my,beep"},
                                                    {}};
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h> // Platform 总线头文件
#include <linux/of.h>              // 设备树头文件
#include <linux/of_device.h>

// [修改指南]: 定义设备名称
#define DEVICE_NAME "my_platform_device"
#define DRIVER_NAME "my_platform_driver"

static int major_number;

// -----------------------------------------------------------------------------
// 字符设备操作函数 (与硬件无关的通用逻辑)
// -----------------------------------------------------------------------------

static int my_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s: Device opened\n", DRIVER_NAME);
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s: Device closed\n", DRIVER_NAME);
    return 0;
}

// [修改指南]: 根据需要添加 read/write/ioctl 等函数
static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
};

// -----------------------------------------------------------------------------
// Platform 驱动核心函数 (Probe & Remove)
// -----------------------------------------------------------------------------

/*
 * my_probe - 驱动与设备匹配成功后执行
 * @pdev: 指向匹配到的 platform_device 结构体
 */
static int my_probe(struct platform_device *pdev) {
    printk(KERN_INFO "%s: Probe matched! Device tree node found.\n", DRIVER_NAME);
    
    // [修改指南]: 获取设备树资源 (如寄存器地址、GPIO、中断等)
    // struct resource *res;
    // res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    // if (!res) { ... }

    // 注册字符设备
    major_number = register_chrdev(0, DEVICE_NAME, &my_fops);
    if (major_number < 0) {
        printk(KERN_ALERT "%s: Failed to register a major number\n", DRIVER_NAME);
        return major_number;
    }
    
    printk(KERN_INFO "%s: Registered correctly with major number %d\n", DRIVER_NAME, major_number);

    // [修改指南]: 创建设备节点 (class_create, device_create) 以便自动生成 /dev/xxx
    
    return 0;
}

/*
 * my_remove - 驱动卸载或设备移除时执行
 * @pdev: 指向 platform_device 结构体
 */
static int my_remove(struct platform_device *pdev) {
    printk(KERN_INFO "%s: Remove called\n", DRIVER_NAME);

    // 注销字符设备
    unregister_chrdev(major_number, DEVICE_NAME);
    
    return 0;
}

// -----------------------------------------------------------------------------
// 驱动匹配表
// -----------------------------------------------------------------------------

// [修改指南]: 修改 compatible 字符串以匹配你的设备树节点
// 例如在设备树中: 
// my_node {
//     compatible = "vendor,my-custom-device";
//     ...
// };
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-custom-device" },
    { /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_of_match);

// -----------------------------------------------------------------------------
// Platform Driver 结构体定义
// -----------------------------------------------------------------------------

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = my_of_match,
        .owner = THIS_MODULE,
    },
};

// -----------------------------------------------------------------------------
// 模块入口/出口 (使用宏简化)
// -----------------------------------------------------------------------------

// module_platform_driver() 宏会自动生成 init 和 exit 函数，
// 并在其中调用 platform_driver_register 和 platform_driver_unregister
module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux Platform Driver Template");

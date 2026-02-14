#!/usr/bin/env python3
import os
import sys

def write_file(path, content):
    dir_path = os.path.dirname(path)
    if dir_path and not os.path.exists(dir_path):
        os.makedirs(dir_path, exist_ok=True)
    with open(path, 'w') as f:
        f.write(content)

def main():
    template_dir = '/home/gm/Workspace/LinuxDriver/Driver_Template'
    
    if not os.path.exists(template_dir):
        print(f'Error: {template_dir} does not exist')
        sys.exit(1)
    
    print(f'Upgrading Driver_Template at {template_dir}...')
    
    # 1. .clangd
    clangd_content = '''CompileFlags:
  Add:
    - --target=arm-none-linux-gnueabihf
    - -nostdinc
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/arch/arm/include
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/arch/arm/include/generated
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/include
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/include/uapi
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/include/generated
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/include/generated/uapi
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/arch/arm/include/uapi
    - -I/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel/drivers
    - -D__KERNEL__
    - -DMODULE
    - -Wall
    - -Wundef
    - -Wstrict-prototypes
    - -Wno-trigraphs
    - -fno-strict-aliasing
    - -fno-common
    - -fshort-wchar
    - -std=gnu11
    - -O2
  Remove:
    - -W*
'''
    write_file(os.path.join(template_dir, '.clangd'), clangd_content)
    
    # 2. .vscode/settings.json
    vscode_settings = '''{
  "clangd.path": "/usr/bin/clangd",
  "clangd.arguments": [
    "--background-index",
    "--clang-tidy",
    "--completion-style=detailed",
    "--header-insertion=iwyu",
    "--pch-storage=memory",
    "-j=8"
  ],
  "C_Cpp.intelliSenseEngine": "disabled",
  "C_Cpp.autocomplete": "disabled",
  "C_Cpp.errorSquiggles": "disabled",
  "editor.formatOnSave": true,
  "editor.tabSize": 4,
  "editor.insertSpaces": true,
  "files.associations": {
    "*.c": "c",
    "*.h": "c"
  }
}
'''
    write_file(os.path.join(template_dir, '.vscode', 'settings.json'), vscode_settings)
    
    # 3. .vscode/extensions.json
    vscode_extensions = '''{
  "recommendations": [
    "llvm-vs-code-extensions.vscode-clangd",
    "xaver.clang-format"
  ],
  "unwantedRecommendations": [
    "ms-vscode.cpptools"
  ]
}
'''
    write_file(os.path.join(template_dir, '.vscode', 'extensions.json'), vscode_extensions)
    
    # 4. 根目录 Makefile
    root_makefile = '''KDIR:=/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel
ARCH=arm
CROSS_COMPILE=/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
export  ARCH  CROSS_COMPILE
PWD?=$(shell pwd)

DRIVER_DIR := $(PWD)/driver
APP_DIR := $(PWD)/app

APP_SRCS := $(wildcard $(APP_DIR)/*.c)
APP_BINS := $(patsubst %.c,%,$(APP_SRCS))

all: modules app

modules:
	make -C $(KDIR) M=$(DRIVER_DIR) modules

app: $(APP_BINS)

$(APP_DIR)/%: $(APP_DIR)/%.c
	$(CROSS_COMPILE)gcc $< -o $@

clean:
	make -C $(KDIR) M=$(DRIVER_DIR) clean
	rm -f $(APP_BINS)
	rm -f $(DRIVER_DIR)/.*.cmd
	rm -rf $(DRIVER_DIR)/.tmp_versions

.PHONY: all modules app clean
'''
    write_file(os.path.join(template_dir, 'Makefile'), root_makefile)
    
    # 5. driver/Makefile
    driver_makefile = '''obj-m += led_drv.o
'''
    write_file(os.path.join(template_dir, 'driver', 'Makefile'), driver_makefile)
    
    # 6. driver/led_drv.c (LED Platform 驱动示例 - 使用 GPIO 描述符，无静态全局变量)
    led_driver = '''#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

#define DRIVER_NAME "led"
#define DEVICE_NAME "led"
#define CLASS_NAME "led"

struct led_dev
{
    dev_t dev_id;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct gpio_desc *led_gpio;
};

static ssize_t led_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    int val;
    int ret;
    struct led_dev *led = filp->private_data;

    if (len != 1)
        return -EINVAL;

    ret = copy_from_user(&val, buf, 1);
    if (ret)
        return -EFAULT;

    gpiod_set_value(led->led_gpio, val ? 1 : 0);

    return 1;
}

static int led_open(struct inode *inode, struct file *filp)
{
    struct led_dev *led = container_of(inode->i_cdev, struct led_dev, cdev);
    filp->private_data = led;
    return 0;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
};

static int led_probe(struct platform_device *pdev)
{
    int ret;
    struct device *dev = &pdev->dev;
    struct led_dev *led;

    dev_info(dev, "LED driver probing...\\n");

    led = devm_kzalloc(dev, sizeof(*led), GFP_KERNEL);
    if (!led)
        return -ENOMEM;

    platform_set_drvdata(pdev, led);

    led->led_gpio = devm_gpiod_get(dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led->led_gpio))
    {
        dev_err(dev, "Failed to get LED GPIO: %ld\\n", PTR_ERR(led->led_gpio));
        return PTR_ERR(led->led_gpio);
    }

    ret = alloc_chrdev_region(&led->dev_id, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&led->cdev, &led_fops);
    ret = cdev_add(&led->cdev, led->dev_id, 1);
    if (ret < 0)
        goto fail_free;

    led->class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(led->class))
    {
        ret = PTR_ERR(led->class);
        goto fail_cdev;
    }

    led->device = device_create(led->class, NULL, led->dev_id, NULL, DEVICE_NAME);
    if (IS_ERR(led->device))
    {
        ret = PTR_ERR(led->device);
        goto fail_class;
    }

    dev_info(dev, "LED driver probed successfully!\\n");
    return 0;

fail_class:
    class_destroy(led->class);
fail_cdev:
    cdev_del(&led->cdev);
fail_free:
    unregister_chrdev_region(led->dev_id, 1);
    return ret;
}

static int led_remove(struct platform_device *pdev)
{
    struct led_dev *led = platform_get_drvdata(pdev);

    device_destroy(led->class, led->dev_id);
    class_destroy(led->class);
    cdev_del(&led->cdev);
    unregister_chrdev_region(led->dev_id, 1);
    dev_info(&pdev->dev, "LED driver removed\\n");
    return 0;
}

static const struct of_device_id led_of_match[] = {
    {.compatible = "my,led"},
    {}};
MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = led_of_match,
    },
    .probe = led_probe,
    .remove = led_remove,
};

module_platform_driver(led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gm");
'''
    write_file(os.path.join(template_dir, 'driver', 'led_drv.c'), led_driver)
    
    # 7. app/led_app.c (LED 测试应用)
    led_app = '''#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fd;
    unsigned char val;

    if (argc != 2)
    {
        printf("Usage: %s <0|1>\\n", argv[0]);
        printf("  0: Turn off LED\\n");
        printf("  1: Turn on LED\\n");
        return -1;
    }

    val = atoi(argv[1]);

    fd = open("/dev/led", O_WRONLY);
    if (fd < 0)
    {
        perror("Open failed");
        return -1;
    }

    if (write(fd, &val, 1) != 1)
    {
        perror("Write failed");
        close(fd);
        return -1;
    }

    printf("LED %s\\n", val ? "ON" : "OFF");

    close(fd);
    return 0;
}
'''
    write_file(os.path.join(template_dir, 'app', 'led_app.c'), led_app)
    
    # 8. scripts/generate_compile_commands.py
    gen_cc = '''#!/usr/bin/env python3
import os
import json
import argparse

def main():
    parser = argparse.ArgumentParser(description='Generate compile_commands.json for Linux driver project')
    parser.add_argument('--kdir', default='/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel',
                        help='Kernel source directory')
    parser.add_argument('--cross-compile', 
                        default='/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-',
                        help='Cross compiler prefix')
    parser.add_argument('--output', default='compile_commands.json',
                        help='Output file path')
    
    args = parser.parse_args()
    
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    driver_dir = os.path.join(project_root, 'driver')
    app_dir = os.path.join(project_root, 'app')
    
    compile_commands = []
    
    kernel_includes = [
        f'-I{args.kdir}/arch/arm/include',
        f'-I{args.kdir}/arch/arm/include/generated',
        f'-I{args.kdir}/include',
        f'-I{args.kdir}/include/uapi',
        f'-I{args.kdir}/include/generated',
        f'-I{args.kdir}/include/generated/uapi',
        f'-I{args.kdir}/arch/arm/include/uapi',
    ]
    
    common_flags = [
        '-nostdinc',
        '-D__KERNEL__',
        '-DMODULE',
        '-Wall',
        '-Wundef',
        '-Wstrict-prototypes',
        '-Wno-trigraphs',
        '-fno-strict-aliasing',
        '-fno-common',
        '-fshort-wchar',
        '-std=gnu11',
        '-O2'
    ]
    
    gcc = f'{args.cross_compile}gcc'
    
    driver_files = [f for f in os.listdir(driver_dir) if f.endswith('.c')]
    for file in driver_files:
        cmd = [gcc, '-c'] + kernel_includes + common_flags + [file]
        compile_commands.append({
            'directory': driver_dir,
            'command': ' '.join(cmd),
            'file': file
        })
    
    app_files = [f for f in os.listdir(app_dir) if f.endswith('.c')]
    for file in app_files:
        cmd = [gcc, '-c', f'-I{args.kdir}/include', '-std=gnu11', '-O2', file]
        compile_commands.append({
            'directory': app_dir,
            'command': ' '.join(cmd),
            'file': file
        })
    
    output_path = os.path.join(project_root, args.output)
    with open(output_path, 'w') as f:
        json.dump(compile_commands, f, indent=2)
    
    print(f'Generated {output_path} with {len(compile_commands)} entries')

if __name__ == '__main__':
    main()
'''
    write_file(os.path.join(template_dir, 'scripts', 'generate_compile_commands.py'), gen_cc)
    
    # 9. scripts/create_new_driver.py
    create_driver = '''#!/usr/bin/env python3
import os
import sys
import shutil
import argparse

def main():
    parser = argparse.ArgumentParser(description='Create a new Linux driver project from Driver_Template')
    parser.add_argument('name', help='Name of the new driver (e.g., dht11_drv)')
    parser.add_argument('--path', '-p', 
                        help='Target directory path (default: same directory as template)')
    
    args = parser.parse_args()
    
    template_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    if args.path:
        new_project_path = os.path.abspath(os.path.join(args.path, args.name))
    else:
        repo_root = os.path.dirname(template_path)
        new_project_path = os.path.join(repo_root, args.name)
    
    if os.path.exists(new_project_path):
        print(f'Error: Project "{args.name}" already exists at {new_project_path}')
        sys.exit(1)
    
    parent_dir = os.path.dirname(new_project_path)
    if not os.path.exists(parent_dir):
        os.makedirs(parent_dir, exist_ok=True)
    
    print(f'Creating new driver project: {args.name}')
    print(f'Template: {template_path}')
    print(f'Target: {new_project_path}')
    
    ignore_patterns = [
        '.git',
        '__pycache__',
        '*.ko',
        '*.o',
        '*.mod.c',
        '*.mod',
        '*.symvers',
        '*.order',
        '.tmp_versions',
        '.*.cmd',
        'app/led_app',
    ]
    
    def ignore_func(dir, files):
        ignored = []
        for f in files:
            for pattern in ignore_patterns:
                if f == pattern or (pattern.startswith('*') and f.endswith(pattern[1:])):
                    ignored.append(f)
                    break
        return ignored
    
    shutil.copytree(template_path, new_project_path, ignore=ignore_func)
    
    driver_makefile = os.path.join(new_project_path, 'driver', 'Makefile')
    with open(driver_makefile, 'r') as f:
        content = f.read()
    
    content = content.replace('obj-m += led_drv.o', f'obj-m += {args.name}.o')
    
    with open(driver_makefile, 'w') as f:
        f.write(content)
    
    print(f'\\n✅ Project created successfully!')
    print(f'\\nNext steps:')
    print(f'  1. cd {new_project_path}')
    print(f'  2. Replace driver/led_drv.c with your driver code (rename to {args.name}.c)')
    print(f'  3. Replace app/led_app.c with your test application (optional)')
    print(f'  4. Run: ./scripts/generate_compile_commands.py')
    print(f'  5. Run: make')

if __name__ == '__main__':
    main()
'''
    write_file(os.path.join(template_dir, 'scripts', 'create_new_driver.py'), create_driver)
    
    # 10. README.md
    readme = '''# Linux 驱动开发工程模板 - LED Platform 驱动示例

这是一个标准的 Linux 驱动开发工程模板，包含：
- LED Platform 驱动示例（使用设备树和 GPIO）
- 完整的 clangd 配置（代码补全、索引）
- 通用的 Makefile 构建系统
- 快速创建新工程的脚本

## 目录结构

```
Driver_Template/
├── .clangd                    # clangd 配置文件
├── compile_commands.json      # 编译数据库（运行脚本后生成）
├── Makefile                   # 根 Makefile
├── README.md
├── .vscode/
│   ├── settings.json          # VS Code 设置
│   └── extensions.json        # 推荐扩展
├── driver/
│   ├── Makefile               # 驱动 Makefile
│   └── led_drv.c              # LED Platform 驱动示例
├── app/
│   └── led_app.c              # LED 测试应用
└── scripts/
    ├── generate_compile_commands.py  # 生成编译数据库
    └── create_new_driver.py          # 从模板创建新工程
```

## 快速开始

### 1. 编译示例工程

```bash
cd Driver_Template

# 生成 clangd 编译数据库
./scripts/generate_compile_commands.py

# 编译
make
```

### 2. 创建新驱动工程

```bash
cd Driver_Template

# 创建名为 my_sensor_drv 的新工程
./scripts/create_new_driver.py my_sensor_drv

# 或者指定自定义路径
./scripts/create_new_driver.py my_sensor_drv -p ~/my_projects
```

### 3. 新工程使用流程

```bash
cd ../my_sensor_drv

# 1. 替换驱动代码（⚠️ 必须命名为 my_sensor_drv.c）
cp /your/driver.c driver/my_sensor_drv.c
rm driver/led_drv.c

# 2. 替换测试应用（可选，任意文件名）
cp /your/test_app.c app/
rm app/led_app.c

# 3. 生成 clangd 索引
./scripts/generate_compile_commands.py

# 4. 编译
make
```

## LED 驱动说明

这是一个基于 Platform 总线的 LED 驱动示例，特点：
- 使用现代 GPIO 描述符 API（struct gpio_desc *）
- 不使用静态全局变量，支持多实例
- 使用设备树配置 GPIO
- 自动创建 /dev/led 设备节点
- 通过 write 系统调用控制 LED 开关

### 不使用静态全局变量的好处

1. **支持多设备实例**：同一个驱动可以同时驱动多个相同的硬件设备
2. **更好的封装**：设备数据与设备实例绑定，代码结构更清晰
3. **线程安全**：避免全局变量竞态条件
4. **资源管理**：使用 devm_* 系列函数，设备移除时自动释放资源
5. **符合 Linux 内核最佳实践**：这是现代 Linux 驱动的标准写法

### 关键实现方式

- `platform_set_drvdata(pdev, dev)`: 在 probe 中保存设备数据
- `platform_get_drvdata(pdev)`: 在 remove 中获取设备数据
- `container_of(inode->i_cdev, struct xxx_dev, cdev)`: 通过 cdev 成员反推完整设备结构体
- `filp->private_data`: 在 open 中保存设备指针，供 read/write 使用

### 设备树配置示例

```dts
led {
    compatible = "my,led";
    status = "okay";
    led-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
};
```

注意：使用 GPIO 描述符 API 时，设备树属性名使用 `-gpios` 后缀（复数）。

### 测试 LED

```bash
# 打开 LED
./app/led_app 1

# 关闭 LED
./app/led_app 0
```

## 关键文件说明

| 文件 | 说明 |
|------|------|
| `.clangd` | 定义内核头文件路径、编译标志 |
| `driver/Makefile` | 需与驱动文件名匹配（create_new_driver.py 会自动设置） |
| `scripts/create_new_driver.py` | 从模板创建新工程的一键脚本 |
| `scripts/generate_compile_commands.py` | 生成 clangd 所需的编译数据库 |

## 注意事项

⚠️ **驱动文件名要求**：使用 `create_new_driver.py` 创建的工程，驱动文件必须与项目同名
- 例如：项目叫 `mpu6050_drv` → 驱动文件叫 `mpu6050_drv.c`
- `driver/Makefile` 中会自动设置为 `obj-m += mpu6050_drv.o`

## 扩展推荐

在 VS Code 中安装：
- **llvm-vs-code-extensions.vscode-clangd** - clangd 语言服务
- **xaver.clang-format** - 代码格式化

**禁用**：ms-vscode.cpptools（微软 C/C++ 插件，与 clangd 冲突）
'''
    write_file(os.path.join(template_dir, 'README.md'), readme)
    
    # 删除旧的示例文件
    old_files = [
        os.path.join(template_dir, 'driver', 'my_driver.c'),
        os.path.join(template_dir, 'app', 'my_app.c'),
    ]
    for f in old_files:
        if os.path.exists(f):
            os.remove(f)
    
    # 设置脚本权限
    os.chmod(os.path.join(template_dir, 'scripts', 'generate_compile_commands.py'), 0o755)
    os.chmod(os.path.join(template_dir, 'scripts', 'create_new_driver.py'), 0o755)
    
    print('\n✅ Driver_Template upgraded successfully!')
    print(f'\nNew features:')
    print('  - LED Platform 驱动示例')
    print('  - Full clangd support')
    print('  - Auto-detect Makefiles')
    print('  - create_new_driver.py script')
    print(f'\nNext step: cd {template_dir} && make')

if __name__ == '__main__':
    main()

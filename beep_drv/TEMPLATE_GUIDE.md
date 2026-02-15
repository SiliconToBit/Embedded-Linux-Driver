# 📦 驱动工程模板使用指南

## ✅ 快速开始（3 步创建新驱动）

### 方法 1：使用脚本自动创建（推荐）

```bash
cd /home/gm/Workspace/LinuxDriver/beep_drv

# 创建名为 my_sensor_drv 的新驱动
./scripts/create_new_driver.py my_sensor_drv
```

### 方法 2：手动复制

```bash
cd /home/gm/Workspace/LinuxDriver

# 复制整个 beep_drv 目录
cp -r beep_drv my_new_drv

# 清理编译产物
cd my_new_drv
make clean

# 重要：修改 driver/Makefile，将 beep_drv.o 改为 your_driver_name.o
# 然后将 driver/beep_drv.c 重命名为 your_driver_name.c
```

---

## 📝 新驱动创建后需要做的事

### 1. 替换驱动代码
```bash
cd my_new_drv

# 删除原来的驱动文件
rm driver/beep_drv.c

# 放入你的驱动代码，必须重命名为与项目同名！
# 例如：项目叫 my_sensor_drv → 驱动文件叫 my_sensor_drv.c
cp /path/to/your/driver.c driver/my_sensor_drv.c
```

### 2. 替换测试应用（可选）
```bash
# 删除原来的测试应用
rm app/beep_app.c

# 放入你的测试代码（任意文件名均可）
cp /path/to/your/test_app.c app/
```

### 3. 生成 clangd 编译数据库
```bash
./scripts/generate_compile_commands.py
```

### 4. 编译测试
```bash
make
```

---

## 🔧 文件说明

| 文件 | 说明 |
|------|------|
| `driver/Makefile` | 需与驱动文件名匹配（脚本会自动设置） |
| `Makefile` (根目录) | 自动编译 `app/` 目录下所有 `.c` 文件 |
| `scripts/create_new_driver.py` | 创建新工程时自动配置 driver/Makefile |
| `scripts/generate_compile_commands.py` | 自动扫描源文件并生成编译数据库 |

---

## 📂 目录结构

```
my_new_drv/
├── .clangd                    # clangd 配置
├── compile_commands.json      # 运行脚本后自动生成
├── Makefile                   # 根 Makefile
├── .vscode/
│   ├── settings.json
│   └── extensions.json
├── driver/
│   ├── Makefile               # 驱动 Makefile（obj-m += my_new_drv.o）
│   └── my_new_drv.c           # ⚠️ 必须与项目同名！
├── app/
│   └── [你的测试代码].c       # 任意文件名均可
├── scripts/
│   ├── generate_compile_commands.py
│   └── create_new_driver.py
└── docs/
```

---

## 💡 常见问题

### Q: 驱动文件名有什么要求？
**A:** 
- 使用 `create_new_driver.py` 创建的工程：**驱动文件必须与项目同名**
  - 例如：项目叫 `mpu6050_drv` → 驱动文件叫 `mpu6050_drv.c`
- `driver/Makefile` 中会自动设置为 `obj-m += mpu6050_drv.o`

### Q: 需要手动修改 Makefile 吗？
**A:** 
- 使用 `create_new_driver.py` 创建：不需要，脚本会自动配置
- 手动复制：需要修改 `driver/Makefile` 中的 `obj-m += xxx.o`

### Q: 测试应用文件名有要求吗？
**A:** 没有！`app/` 目录下任意 `.c` 文件都会被自动编译。

### Q: clangd 索引需要重新生成吗？
**A:** 只有当添加/删除源文件时需要运行：
```bash
./scripts/generate_compile_commands.py
```

### Q: make 后没有生成 .ko 文件？
**A:** 检查：
1. 驱动文件名是否与 `driver/Makefile` 中的 `obj-m` 匹配
2. 驱动代码是否有编译错误（查看 make 输出）

---

## 🎯 完整示例

```bash
# 1. 创建新驱动
cd /home/gm/Workspace/LinuxDriver/beep_drv
./scripts/create_new_driver.py mpu9250_drv

# 2. 进入新目录
cd ../mpu9250_drv

# 3. 放入你的驱动代码（⚠️ 必须命名为 mpu9250_drv.c）
cp ~/my_mpu9250_driver.c driver/mpu9250_drv.c
rm driver/beep_drv.c

# 4. 放入测试代码（可选，任意文件名）
cp ~/my_mpu9250_test.c app/
rm app/beep_app.c

# 5. 生成 clangd 索引
./scripts/generate_compile_commands.py

# 6. 编译
make
```

就这么简单！🎉

---

## 🌳 设备树（DTS）配置指南

### 什么是设备树？

设备树（Device Tree）是一种描述硬件信息的数据结构，用于将硬件信息与驱动代码分离。驱动通过设备树获取硬件配置（如 GPIO 引脚、中断号、寄存器地址等）。

### 设备树基本语法

```dts
/ {
    // 节点名称@基地址
    device_name@address {
        compatible = "vendor,device";    // 兼容性字符串，用于匹配驱动
        reg = <address size>;            // 寄存器地址和大小
        status = "okay";                 // 状态：okay（启用）或 disabled（禁用）
        
        // 其他属性...
    };
};
```

---

### GPIO 设备树配置示例

#### 1. Beep（蜂鸣器）设备树

```dts
/ {
    beep {
        compatible = "my,beep";          // 必须与驱动中的 of_device_id 匹配
        status = "okay";
        beep-gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>;  // GPIO 引脚配置
    };
};
```

#### 2. LED 设备树

```dts
/ {
    led {
        compatible = "my,led";
        status = "okay";
        led-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
    };
};
```

#### 3. DHT11 温湿度传感器设备树

```dts
/ {
    dht11 {
        compatible = "my,dht11";
        status = "okay";
        data-gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;
    };
};
```

---

### GPIO 属性详解

#### `xxx-gpios` 属性格式

```dts
xxx-gpios = <&gpio_controller pin_num flags>;
```

| 参数 | 说明 | 示例 |
|------|------|------|
| `gpio_controller` | GPIO 控制器引用 | `&gpio0`, `&gpio1` |
| `pin_num` | GPIO 引脚编号 | `0`, `5`, `10` |
| `flags` | GPIO 标志 | `GPIO_ACTIVE_HIGH`, `GPIO_ACTIVE_LOW` |

#### 常用 GPIO 标志

| 标志 | 说明 |
|------|------|
| `GPIO_ACTIVE_HIGH` | 高电平有效（默认） |
| `GPIO_ACTIVE_LOW` | 低电平有效 |
| `GPIO_OPEN_DRAIN` | 开漏输出 |
| `GPIO_OPEN_SOURCE` | 开源输出 |

---

### 驱动与设备树的匹配

#### 驱动代码中的匹配表

```c
static const struct of_device_id beep_of_match[] = {
    { .compatible = "my,beep" },    // 必须与设备树中的 compatible 一致
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, beep_of_match);
```

#### 驱动中获取 GPIO

```c
// 方法：使用 GPIO 描述符 API（推荐）
// 设备树属性名：beep-gpios（注意是复数 gpios）
// 函数参数：去掉 -gpios 后缀的部分，即 "beep"

beep->beep_gpio = devm_gpiod_get(dev, "beep", GPIOD_OUT_LOW);
```

#### 属性名对应关系

| 设备树属性名 | devm_gpiod_get 参数 |
|-------------|-------------------|
| `beep-gpios` | `"beep"` |
| `led-gpios` | `"led"` |
| `data-gpios` | `"data"` |
| `reset-gpios` | `"reset"` |

---

### 完整设备树示例

```dts
/ {
    // 蜂鸣器
    beep {
        compatible = "my,beep";
        status = "okay";
        beep-gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>;
    };
    
    // LED
    led {
        compatible = "my,led";
        status = "okay";
        led-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
    };
    
    // DHT11 温湿度传感器
    dht11 {
        compatible = "my,dht11";
        status = "okay";
        data-gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;
    };
    
    // I2C 设备示例
    mpu6050: mpu6050@68 {
        compatible = "inv,mpu6050";
        reg = <0x68>;
        status = "okay";
        interrupt-parent = <&gpio0>;
        interrupts = <10 IRQ_TYPE_EDGE_RISING>;
    };
    
    // SPI 设备示例
    spi_flash: spi-flash@0 {
        compatible = "jedec,spi-nor";
        reg = <0>;
        spi-max-frequency = <10000000>;
        status = "okay";
    };
};
```

---

### 设备树文件位置

设备树文件通常位于内核源码目录：

```
kernel/arch/arm/boot/dts/
├── rk3506.dtsi           // 芯片级设备树（包含 SoC 基本信息）
├── rk3506-xxx.dts        // 板级设备树（具体开发板配置）
└── overlays/             // 设备树插件目录
```

### 添加自定义设备树节点

1. **方法一：修改板级 DTS 文件**
   ```bash
   vim kernel/arch/arm/boot/dts/rk3506-your-board.dts
   ```

2. **方法二：使用设备树插件（推荐）**
   ```bash
   # 创建设备树插件
   vim overlays/my-driver.dts
   ```

3. **编译设备树**
   ```bash
   make dtbs
   ```

---

### 验证设备树

#### 查看设备树节点

```bash
# 在开发板上执行
ls /proc/device-tree/
cat /proc/device-tree/beep/compatible
cat /proc/device-tree/beep/status
```

#### 查看 GPIO 状态

```bash
cat /sys/kernel/debug/gpio
```

#### 查看设备是否被驱动匹配

```bash
ls /sys/bus/platform/devices/
ls /sys/bus/platform/drivers/
```

---

### 常见问题

#### Q: 驱动加载后没有 probe？
**A:** 检查：
1. `compatible` 字符串是否与驱动中的 `of_device_id` 匹配
2. 设备树节点 `status` 是否为 `"okay"`
3. 设备树是否正确编译并加载

#### Q: GPIO 获取失败？
**A:** 检查：
1. 设备树属性名是否使用 `-gpios` 后缀（复数）
2. GPIO 引脚是否被其他驱动占用
3. GPIO 控制器引用是否正确（`&gpio0` 或 `&gpio1`）

#### Q: 如何查看 GPIO 编号？
**A:** 
```bash
# 查看所有 GPIO
cat /sys/kernel/debug/gpio

# 计算 GPIO 编号
# GPIO0_A5 = 0 * 32 + 0 * 8 + 5 = 5
# GPIO1_B3 = 1 * 32 + 1 * 8 + 3 = 43
```

---

### 更多资源

- [Linux 内核设备树文档](https://www.kernel.org/doc/Documentation/devicetree/)
- [GPIO 绑定文档](https://www.kernel.org/doc/Documentation/devicetree/bindings/gpio/)
- [RK3506 数据手册](https://www.rock-chips.com/)

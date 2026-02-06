# MPU6050 Linux 驱动使用说明

## 1. 设备树 (Device Tree) 配置

在编译内核或加载驱动之前，需要在板级设备树文件（通常是 `.dts` 或 `.dtsi`）中添加对应的节点信息。

请找到你的 I2C 控制器节点（例如 `&i2c1`），在其中添加如下内容：

```dts
&i2c1 {
    /* 确保 i2c 控制器已启用 */
    status = "okay";

    mpu6050@68 {
        /*Compatible 必须与驱动源码中的 mpu6050_of_match 保持一致 */
        compatible = "my,mpu6050";
        
        /* I2C 从机地址，AD0接地为0x68，接VCC为0x69 */
        reg = <0x68>;  
        
        /* 
         * 中断引脚配置 
         * interrupt-parent:即使引脚所属的 GPIO 组，例如 &gpio1
         * interrupts: <引脚号 触发标志>
         */
        interrupt-parent = <&gpio1>;
        
        /* 
         * <18 4> 的含义：
         * 18: GPIO 引脚索引 (请根据实际硬件连接修改)
         * 4:  IRQ_TYPE_LEVEL_HIGH (高电平触发)
         * 注意：驱动代码中使用了 IRQF_TRIGGER_HIGH，此处必须配置为高电平触发 (4)
         */
        interrupts = <18 4>; 
    };
};
```

## 2. 关键属性说明

*   **`compatible = "my,mpu6050";`**
    *   驱动源码中定义了 `{.compatible = "my,mpu6050"}`。如果设备树中的此属性不匹配，驱动的 `probe` 函数将不会被执行。

*   **`reg = <0x68>;`**
    *   MPU6050 的 I2C 地址。
    *   可以通过在终端运行 `i2cdetect -y 1` (假设是 I2C-1) 来确认设备是否在线以及具体的地址。

*   **`interrupts = <18 4>;`**
    *   驱动源码中 `devm_request_irq`使用了 `IRQF_TRIGGER_HIGH`。
    *   设备树中的触发标志位（第二个数值）必须包含 `4` (High level)。
    *   常见标志位定义:
        *   1 = Rising edge (上升沿)
        *   2 = Falling edge (下降沿)
        *   4 = High level (高电平)
        *   8 = Low level (低电平)

## 3. Rockchip (RK) 平台配置示例

如果是 Rockchip 平台（如 RK3568, RK3588 等），引脚名称通常类似 `GPIO1_B3`。

### 3.1 怎么把 GPIO1_B3 转换成设备树代码？

在 Rockchip 的设备树中，引用 GPIO 通常有两种方式：使用 **宏定义** (推荐) 或 **手算编号**。

假设引脚为 **GPIO1_B3**：

**方式一：使用宏定义（推荐，需包含头文件）**
Rockchip内核通常定义了方便的宏，格式为 `RK_P<Group><Index>`。
*   `GPIO1` 对应 `interrupt-parent = <&gpio1>;`
*   `B3` 对应 `RK_PB3`

```dts
mpu6050@68 {
    compatible = "my,mpu6050";
    reg = <0x68>;
    interrupt-parent = <&gpio1>;
    /* 使用宏 RK_PB3，触发方式 4 (高电平) */
    interrupts = <RK_PB3 4>; 
};
```

**方式二：手动计算编号**
如果你的环境不支持宏，需要计算引脚索引。公式：`pin = group * 8 + index`
*   Group A = 0
*   Group B = 1
*   Group C = 2
*   Group D = 3

对于 **GPIO1_B3**:
*   属于 `&gpio1`
*   Group 是 **B** (=1)
*   Index 是 **3**
*   计算结果：`1 * 8 + 3 = 11`

```dts
mpu6050@68 {
    compatible = "my,mpu6050";
    reg = <0x68>;
    interrupt-parent = <&gpio1>;
    /* 11 代表 PB3，触发方式 4 (高电平) */
    interrupts = <11 4>; 
};
```

**关于后缀 "_d"**:
通常原理图上的 `_d` (如 `GPIO1_B3_d`) 代表该引脚的某种复用功能或驱动能力后缀，在设备树填写中断号时，只需要关注它是 **GPIO1** 的 **B3** 引脚即可。

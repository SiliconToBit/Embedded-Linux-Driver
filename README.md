# Embedded Linux Driver Repository

这是一个包含多个传感器和外设的嵌入式 Linux 驱动代码仓库。

- 平台为 luckfox rk3506
- Linux 内核 6.1.99

## 📂 项目目录 (Project Directory)

点击下方链接可快速跳转到对应驱动目录：

| 模块名称 | 路径 (点击跳转) | 说明 |
| :--- | :--- | :--- |
| **DHT11** | [dht11_drv](./dht11_drv) | DHT11 温湿度传感器驱动与测试应用 |
| **MPU6050 (v1)** | [mpu6050_drv1](./mpu6050_drv1) | MPU6050 六轴传感器驱动 (第一版，不使用中断) |
| **MPU6050 (v2)** | [mpu6050_drv2](./mpu6050_drv2) | MPU6050 六轴传感器驱动 (第二版，使用中断) |
| **BEEP** | [beep_drv](./beep_drv) | 蜂鸣器驱动与测试应用 (基于platform驱动) |
| **Driver Template** | [Driver_Template](./Driver_Template) | 驱动工程模板，包含完整的工程结构和配置 |

## 📁 工程结构

所有驱动工程都采用统一的目录结构：

```
工程名/
├── .vscode/           # VSCode 配置
├── .clangd            # Clangd 语言服务器配置
├── .clang-format       # 代码格式化配置
├── Makefile            # 顶层 Makefile
├── compile_commands.json # 编译命令数据库
├── app/               # 应用程序目录
├── driver/             # 驱动目录
└── scripts/            # 工具脚本目录
```

## 🛠️ 构建说明

每个驱动目录下通常包含独立的 `Makefile`。进入相应目录后，通常可以使用以下命令进行编译：

```bash
cd <驱动目录>
make
```

## 📝 备注

* **DHT11**: 数字温湿度传感器
* **MPU6050**: 包含 3 轴陀螺仪和 3 轴加速度计的运动处理组件
* **BEEP**: 蜂鸣器（有源/无源蜂鸣器控制）
* **Driver_Template**: 标准驱动工程模板，可用于快速创建新的驱动工程
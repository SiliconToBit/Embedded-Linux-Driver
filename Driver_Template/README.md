# Linux 驱动开发工程模板 - LED Platform 驱动示例

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
- 使用设备树配置 GPIO
- 自动创建 /dev/led 设备节点
- 通过 write 系统调用控制 LED 开关

### 设备树配置示例

```dts
led {
    compatible = "my,led";
    status = "okay";
    led-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
};
```

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

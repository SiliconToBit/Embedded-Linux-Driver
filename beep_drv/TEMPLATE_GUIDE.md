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

# Linux 驱动开发通用工程模板

这是一个标准的 Linux 驱动开发工程结构，包含了驱动模块和测试应用的构建系统。

## 目录结构

```
.
├── .vscode/              # VS Code 配置目录
│   └── c_cpp_properties.json # IntelliSense 包含路径配置 (含 SDK 路径)
├── Makefile              # 顶层构建脚本 (通常只需修改 APP_NAME)
├── app/                  # 应用程序目录
│   └── my_app.c          # 应用程序源码模板 (参考修改)
└── driver/               # 驱动程序目录
    ├── Makefile          # 驱动构建脚本 (修改 object 目标名)
    └── my_driver.c       # 驱动源码模板 (参考修改)
```

## 快速上手指南

### 1. 新建立工程
将此目录复制为你的新工程目录：
`cp -r Driver_Template MyNewProject`

### 2. 修改驱动
1. 进入 `driver/` 目录。
2. 将 `my_driver.c` 重命名为你想要的名字，例如 `led_drv.c`。
3. 修改 `driver/Makefile`，将 `obj-m += my_driver.o` 改为 `obj-m += led_drv.o`。
4. 编辑 `.c` 文件实现你的驱动逻辑。

### 3. 修改应用
1. 进入 `app/` 目录。
2. 将 `my_app.c` 重命名为你想要的名字，例如 `led_test.c`。
3. 编辑 `.c` 文件实现你的测试逻辑。

### 4. 配置编译
1. 回到根目录。
2. 打开 `Makefile`。
3. 修改 `APP_NAME` 变量：`APP_NAME := led_test` (注意不要加 .c)。
4. (可选) 如果更换了开发板或SDK，请更新 `KDIR` 和 `CROSS_COMPILE` 路径，模板默认已配置为当前环境。

### 5. 编译与清理
* **编译全部**: `make`
* **清理**: `make clean`

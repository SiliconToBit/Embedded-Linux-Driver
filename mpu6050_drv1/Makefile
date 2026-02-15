KDIR:=/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel
ARCH=arm
CROSS_COMPILE=/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
export  ARCH  CROSS_COMPILE
PWD?=$(shell pwd)

DRIVER_DIR := $(PWD)/driver
APP_DIR := $(PWD)/app

all: modules app

modules:
	make -C $(KDIR) M=$(DRIVER_DIR) modules

app:
	$(CROSS_COMPILE)gcc $(APP_DIR)/mpu6050_app.c -o $(APP_DIR)/mpu6050_app

clean:
	make -C $(KDIR) M=$(DRIVER_DIR) clean
	rm -f $(APP_DIR)/mpu6050_app
	rm -f $(DRIVER_DIR)/.*.cmd
	rm -rf $(DRIVER_DIR)/.tmp_versions

.PHONY: all modules app clean

KDIR:=/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel
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

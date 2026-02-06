obj-m += mpu6050_drv.o
# KDIR:=/home/alientek/linux_sdk/luckfox_rk3506_sdk/kernel-6.1
KDIR:=/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel
ARCH=arm
CROSS_COMPILE=/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
export  ARCH  CROSS_COMPILE
PWD?=$(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	echo $(PWD)
clean:
	rm -f *.ko *.o *.mod *.mod.o *.mod.c *.symvers *.order
output:
	$(CROSS_COMPILE)gcc mpu6050_app.c -o mpu6050_app
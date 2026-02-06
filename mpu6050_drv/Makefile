obj-m += mpu6050_drv.o
KDIR:=/home/alientek/linux_sdk/luckfox_rk3506_sdk/kernel-6.1
# KDIR:=/home/alientek/linux_sdk/atk_dlrk3506_linux_sdk/kernel
ARCH=arm
CROSS_COMPILE=arm-none-linux-gnueabihf-
export  ARCH  CROSS_COMPILE
PWD?=$(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	echo $(PWD)
clean:
	rm -f *.ko *.o *.mod *.mod.o *.mod.c *.symvers *.order
output:
	arm-none-linux-gnueabihf-gcc mpu6050_app.c -o mpu6050_app
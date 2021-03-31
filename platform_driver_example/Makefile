obj-m:=platform_device.o platform_driver.o
KERNELDIR:=/lib/modules/$(shell uname -r)/build
CONFIG_MODULE_SIG=n
PWD:=$(shell pwd)
modules:
	make -C $(KERNELDIR) M=$(PWD) modules
	gcc test_01.c -o test1
	gcc test_02.c -o test2
clean:
	rm *.o *.order *.mod.c *.symvers *.ko test1 test2
insmod:
	sudo insmod platform_driver.ko
	sudo insmod platform_device.ko 
rmmod:
	sudo rmmod platform_driver.ko
	sudo rmmod platform_device.ko 
reinsmod:
	sudo rmmod platform_driver.ko
	sudo rmmod platform_device.ko 
	sudo insmod platform_driver.ko
	sudo insmod platform_device.ko 

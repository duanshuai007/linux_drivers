obj-m:=rtc_device.o rtc_driver.o
KDIR:=/home/duan/work/kernel/linux-3.6.6
PWD:=$(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clear:
	rm *.o *.order *.mod.c *.symvers *.ko

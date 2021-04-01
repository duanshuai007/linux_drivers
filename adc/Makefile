obj-m:=adc_device.o adc_driver_sync.o
#KDIR:=/home/duan/work/kernel/linux-3.6.6
KDIR:=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
#rm *.o *.order *.mod.c *.symvers *.ko

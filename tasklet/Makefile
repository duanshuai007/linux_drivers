obj-m:=tasklet.o
KERNELDIR:=/lib/modules/$(shell uname -r)/build
CONFIG_MODULE_SIG=n
#KERNELDIR=/home/duan/crosstool/kernel/3.6.6/linux-3.6.6_back
PWD:=$(shell pwd)
modules:
	make -C $(KERNELDIR) M=$(PWD) modules
	gcc test_read.c -o read
	gcc test_write.c -o write
	gcc test_select.c -o select
clean:
	@rm -rf *.o *.ko *.mod.c *.order *.symvers read write select a.out

insmod:
	sudo insmod tasklet.ko
rmmod:
	sudo rmmod tasklet
reinsmod:
	sudo rmmod tasklet
	sudo insmod tasklet.ko

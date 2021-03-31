obj-m = lcd_driver.o
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	gcc test.c -o test
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm test
insmod:
	sudo insmod lcd_driver.ko
rmmod:
	sudo rmmod lcd_driver
reinsmod:
	sudo rmmod lcd_driver
	sudo insmod lcd_driver.ko

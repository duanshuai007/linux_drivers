obj-m = spi_device.o spi_driver.o
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	gcc test.c -o test
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm test
insmod:
	sudo insmod spi_device.ko
	sudo insmod spi_driver.ko
rmmod:
	sudo rmmod spi_device.ko
	sudo rmmod spi_driver.ko
reinsmod:
	sudo rmmod spi_device.ko
	sudo rmmod spi_driver.ko
	sudo insmod spi_device.ko
	sudo insmod spi_driver.ko

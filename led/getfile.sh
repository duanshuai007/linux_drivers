!#/bin/sh

ifconfig eth0 192.168.199.222
tftp -g 192.168.199.212 -r led_driver.ko
tftp -g 192.168.199.212 -r led_device.ko
chmod 555 led_driver.ko
chmod 555 led_device.ko
insmod led_driver.ko
insmod led_device.ko

echo "install driver and device finished\r\n"

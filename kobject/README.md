## KOBJECT EXAMPLE

用来测试kobject的例子

执行

make

make insmod

后会在/dev/目录下生成kobject_test_driver文件

在/sys目录下生成kobject_test目录

```
cat /sys/kobject_test/child/led
status = 0
sudo cat /sys/kobject_test/child/led_status 
status = 0

在root用户下

echo on > /sys/kobject_test/child/led_status
echo off > /sys/kobject_test/child/led_status

```


也可以使用test文件进行测试

./test

#!/bin/sh

#0 是脚本名
name=$0
#1 是一个参数
module_name=$1

#echo "name=$name par=$par"

insmod ${module_name}
#echo "aTest.ko" | awk -F "\." {'print $1'}
tmp_name=`echo "${module_name}" | awk -F "." {'print $1'}`
echo ${tmp_name}
major=`cat /proc/devices | grep ${tmp_name} | awk -F " " {'print $1'}`
echo ${major}
mknod /dev/${tmp_name} c ${major} 0

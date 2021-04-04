# Device Tree!!!!!!!!!!!!!!!!

## 1.Edit device tree at run time

- work on ubuntu

Install the tool

`sudo apt-get install device-tree-compiler`

After this should have these tools installed: fdtdump fdtget fdtput


for example:

#### 打印dtb的内容，在实际使用时发现打印的值与实际值不一致。
- fdtdump example.dtb

#### 获取dtb文件中的某一个节点的某一属性值
- fdtget example.dtb /spi reg
- fdtget example.dtb /external-bus/ethernet compatible

#### 设置dtb文件中某一节点的某一属性值
- fdtput -t i example.dtb /spi clock-frequency 100000
- fdtput -t i example.dtb /spi reg 269570048 1000


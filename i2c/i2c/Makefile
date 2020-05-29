CFLAGS= -Wall -O2
CC=arm-S3C2440-linux-gnueabi-gcc

i2c: eeprog.o 24cXX.o
	$(CC) $(CFLAGS) -o i2c eeprog.o 24cXX.o

clean: 
	rm -f i2c 24cXX.o eeprog.o


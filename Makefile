obj-m += test_dev.o
obj-m += ram_dev.o
obj-m += partition.o

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

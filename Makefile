#obj-m := dor.o
#dor-objs := ram_dev.o partition.o ram_disk.o

obj-m := test.o
test-objs := test_dev.o partition.o ram_disk.o
module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

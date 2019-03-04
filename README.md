# GPT-Disk-On-Ram
Linux block device driver that builds a virtual disk in RAM using the GPT partitioning scheme. 
This project aims to let the user spawn a disk in RAM that the operating system recognizes as a disk drive that it can perform operations on as it could on a normal drive. 

The disk that spawns when loading dor.ko has 1 partition of 2MB, but theoretically supports up to 128 partitions on the disk. This disk is fully operable, and can be edited with your favorite disk partitoning program such as Gparted. 

# Compiling the driver
First make sure that you have your current kernel's latest module-header files. As we are not invoking the kernels build system, we need to have our own headers.

-- Install the lastest kernel headers 

In Arch based systems this can be done by installing the meta-package and then selecting your kernel's headers by typing

$ sudo pacman -Sy linux-headers

In Debian based systems this can be similiarly done with

$sudo apt-get install linux-headers-$(uname -r)

-- Compile the driver

Enter the GPT-Disk-On-Ram directory and type 

$ make

If it complains that it cannot find the directory /lib/modules/$(KERNEL_VERSION) then you need to install the latest kernel headers for your currently running kernel. 
If all is well you should see a dor.ko file. 

# Running the driver
With the dor.ko file, we will now load the module. The great thing about Linux kernel modules is that we can add new kernel code without having to re-compile the kernel. Neat! 

-- Loading the driver

To load the driver type

$ sudo insmod dor.ko

This will load the driver. If it does not let you, make sure that your kernel headers match the currently running kernel.
If all is well, the module should now be loaded and running. 

-- Testing the driver

To see if the module was successfully loaded type
$ dmesg | grep "dor"
There should be a status message saying that the driver was loaded or an error message telling you why it did not load. 
If all is well the kernel should have registered the disk as being active and usable. 
Type
$ sudo fdisk -l
To see a list of disk, our kernel driver should be one of them with the name "/dev/dor" and a partition named "/dev/dor1"
The disk should now be accessible and ready to use. 

# Info 
This project was done mainly to learn about block device driver fundamentals and to learn about partitioning schemes.
Please feel free to give me any feedback about style, design, or anything that needs improvement. 
If you have any questions please email me at lnowakow@ucsd.edu

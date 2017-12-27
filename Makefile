obj-m := coop.o
CONFIG_MODULE_SIG=n
# KDIR := /path/to/kernel/sources/root/directory
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all: coop.c 
	make -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean
run: 
	sudo dmesg --clear && sudo insmod coop.ko 
remove:
	sudo rmmod coop
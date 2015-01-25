#
# Makefile for the PARDg5-V ControlPanel Adaptor driver
#

DISK := /opt/m5-system/prm/disks/PARDg5GM.img
KDIR := /opt/m5-system/prm/binaries/linux-2.6.28.4-prm/
PWD := $(shell pwd)
EXTRA_CFLAGS := -std=gnu99

obj-m := cpa.o
cpa-objs := init.o access.o


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf modules.order mmaptest ioctltest

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

install: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt
	sudo cp -rf cpa.ko .mnt/lib/modules/2.6.28.4/cpa.ko
	sudo umount .mnt

mount: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt

umount:
	sudo umount .mnt


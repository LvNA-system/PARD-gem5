#
# Makefile for the PARDg5-V ControlPanel driver
#

#
# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

#
# PATH settings
DISK := /opt/m5-system/prm/disks/PARDg5GM.img
KDIR := /opt/m5-system/prm/binaries/linux-2.6.28.4-prm/
PWD := $(shell pwd)
CPAINCDIR := $(PWD)/../includes

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif
EXTRA_CFLAGS  += $(DEBFLAGS) -std=gnu99 -I$(CPAINCDIR)

obj-m := pardg5v.o
ALL_CTRLPLANE := sys.o ioh.o ich.o #cache.o memcntrl.o
pardg5v-objs := init.o ctrlplane.o cpfops.o $(ALL_CTRLPLANE) # generic.o 


all:
	echo $(EXTRA_CFLAGS)
	$(MAKE) -C $(KDIR) M=$(PWD) CPAINCDIR=$(PWD)/../includes modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf modules.order mmaptest ioctltest

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

install: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt
	sudo cp -rf pardg5v.ko .mnt/lib/modules/2.6.28.4/pardg5v.ko
	sudo umount .mnt

mount: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt

umount:
	sudo umount .mnt


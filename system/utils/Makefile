#
# Makefile for the PARDg5-V ControlPanel Adaptor driver
#

DISK := /opt/m5-system/prm/disks/PARDg5GM.img
CFLAGS := -std=gnu99 -static -I../modules/includes/ -g

UTILS := ioctltest mmaptest biosloader kickstart

all: $(UTILS)

clean:
	rm -rf $(UTILS)

mmaptest: mmaptest.c

ioctltest: ioctltest.c

biosbuilder: biosbuilder.c

biosloader: biosloader.c

install: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt
	sudo cp -rf $(UTILS) .mnt/utils/
	sudo umount .mnt

mount: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt

umount:
	sudo umount .mnt


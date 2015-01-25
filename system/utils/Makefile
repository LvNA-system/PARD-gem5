#
# Makefile for the PARDg5-V ControlPanel Adaptor driver
#

DISK := /opt/m5-system/prm/disks/PARDg5GM.img
EXTRA_CFLAGS := -std=gnu99 -static -I../modules/includes/

UTILS := ioctltest mmaptest biosloader biosbuilder

all: $(UTILS)

mmaptest: mmaptest.c
	gcc $< -o $@ $(EXTRA_CFLAGS)

ioctltest: ioctltest.c
	gcc $< -o $@ $(EXTRA_CFLAGS)

biosbuilder: biosbuilder.c
	gcc $< -o $@ $(EXTRA_CFLAGS) -g

biosloader: biosloader.c
	gcc $< -o $@ $(EXTRA_CFLAGS) -g

install: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt
	sudo cp -rf $(UTILS) .mnt/utils/
	sudo umount .mnt

mount: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt

umount:
	sudo umount .mnt


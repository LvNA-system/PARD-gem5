#
# Makefile for the PARDg5-V ControlPanel Adaptor driver
#

DISK := /opt/m5-system/prm/disks/PARDg5GM.img
CFLAGS := -std=gnu99 -I../../modules/includes/ -g
LDFLAGS := -static


ioh: ioh.o ioh_list.o ioh_util.o

install: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt
	sudo cp -rf ioh .mnt/utils/
	sudo umount .mnt

install-script:
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt
	sudo cp -rf kickstart iohlist .mnt/utils/
	sudo umount .mnt

mount: 
	sudo mount -o loop,offset=32256 $(DISK) ./.mnt

umount:
	sudo umount .mnt

clean:
	rm -rf *.o ioh

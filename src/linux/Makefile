#include the Product make.h
include ./make.h
.PHONY: all clean mbr gbd inode superblock filerecovery gui ntfs assign4 assign5 assign7
all:
	make -C $(BASE_DIR)/mbr
	make -C $(BASE_DIR)/gbd
	make -C $(BASE_DIR)/inode
	make -C $(BASE_DIR)/superblock
	make -C $(BASE_DIR)/filerecovery
	make -C $(BASE_DIR)/assign4
	make -C $(BASE_DIR)/assign5	
	make -C $(BASE_DIR)/assign7	
	#make -C $(BASE_DIR)/gui
	cp $(BASE_DIR)/mbr/exe/* $(BASE_DIR)/bin
	cp $(BASE_DIR)/gbd/exe/* $(BASE_DIR)/bin
	cp $(BASE_DIR)/inode/exe/* $(BASE_DIR)/bin
	cp $(BASE_DIR)/superblock/exe/* $(BASE_DIR)/bin
	cp $(BASE_DIR)/filerecovery/exe/* $(BASE_DIR)/bin
	cp $(BASE_DIR)/assign4/exe/* $(BASE_DIR)/bin
	cp $(BASE_DIR)/assign5/exe/* $(BASE_DIR)/bin
	cp $(BASE_DIR)/assign7/exe/* $(BASE_DIR)/bin
	#cp $(BASE_DIR)/gui/exe/* $(BASE_DIR)/bin

mbr:
	make -w -C $(BASE_DIR)/mbr
	cp $(BASE_DIR)/mbr/exe/* $(BASE_DIR)/bin

gbd:
	make -w -C $(BASE_DIR)/gbd
	cp $(BASE_DIR)/gbd/exe/* $(BASE_DIR)/bin

inode:
	make -C $(BASE_DIR)/inode
	cp $(BASE_DIR)/inode/exe/* $(BASE_DIR)/bin

superblock:
	make -C $(BASE_DIR)/superblock
	cp $(BASE_DIR)/superblock/exe/* $(BASE_DIR)/bin

filerecovery:
	make -C $(BASE_DIR)/filerecovery
	cp $(BASE_DIR)/filerecovery/exe/* $(BASE_DIR)/bin

gui:
	make -C $(BASE_DIR)/gui
	cp $(BASE_DIR)/gui/exe/* $(BASE_DIR)/bin

ntfs:
	make -C $(BASE_DIR)/ntfs
	cp $(BASE_DIR)/ntfs/exe/* $(BASE_DIR)/bin

assign4:
	make -C $(BASE_DIR)/assign4
	cp $(BASE_DIR)/assign4/exe/* $(BASE_DIR)/bin

assign5:
	make -C $(BASE_DIR)/assign5
	cp $(BASE_DIR)/assign5/exe/* $(BASE_DIR)/bin
	
assign7:
	make -C $(BASE_DIR)/assign7
	cp $(BASE_DIR)/assign7/exe/* $(BASE_DIR)/bin

clean:
	make -w -C $(BASE_DIR)/mbr clean
	make -w -C $(BASE_DIR)/gbd clean
	make -w -C $(BASE_DIR)/inode clean
	make -w -C $(BASE_DIR)/superblock clean
	make -w -C $(BASE_DIR)/filerecovery clean
	make -w -C $(BASE_DIR)/gui clean
	make -w -C $(BASE_DIR)/ntfs clean
	rm $(BASE_DIR)/bin/*

debug:
	make -w -C $(BASE_DIR)/common
	make -w -C $(BASE_DIR)/node
	cp $(BASE_DIR)/node/exe/* $(BASE_DIR)/bin


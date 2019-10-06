#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "inodeinc.h"

extern int init(char *disk);
extern int getInodesPerGroup();
extern int getBlockSize();
extern int getInodeSize();

void readInode(int inode, FILE *fp)
{
	printf("----- BEGIN INODE READING -----\n");
	struct ext3_inode Inode;
	struct ext3_group_desc GroupDes;
	int inodesPerGroup = getInodesPerGroup();
	int blockSize = getBlockSize();
	printf("Inodes per block group: %d\n", inodesPerGroup);
	int bgNumber = (inode - 1) / inodesPerGroup;
	int offset = (inode - 1) % inodesPerGroup;
	offset *= getInodeSize();
	printf("Target block group: %d\n", bgNumber);
	// add block size to skip the superblock
	int groupDescriptorOffset = bgNumber * sizeof(struct ext3_group_desc) + blockSize;
	printf("Group Descriptor offset: %d\n", groupDescriptorOffset);
	fseek(fp, groupDescriptorOffset, SEEK_SET);
	fread(&GroupDes, sizeof(GroupDes), 1, fp);
	int inodeTableLoc = GroupDes.bg_inode_table * blockSize;
	printf("Inode table location: %d\n", inodeTableLoc);
	printf("Inode offset into table: %d\n", offset);
	fseek(fp, inodeTableLoc + offset, SEEK_SET);
	fread(&Inode, sizeof(Inode), 1, fp);
	int i = 0;
	printf("File size: %d\n", Inode.i_size);
	for(; i < EXT3_N_BLOCKS; i++)
	{
		printf("Block pointer %d: %d\n", i, Inode.i_block[i]);
	}
	
}

int main(int argc, char **argv) 
{ 
	if (argc != 3) 
	{
		printf("Error: %s\n", strerror(errno));
		return -1;
	}
	FILE *fp = fopen(argv[1], "rb");
	if (fp < 0)
	{
		printf("Error %s\n", strerror(errno));
		return -1;
	}
	int inode = atoi(argv[2]);
	printf("Target Inode %d\n", inode);
	init(fp);
	readInode(inode, fp);
	fclose(fp);
	return 0;
}

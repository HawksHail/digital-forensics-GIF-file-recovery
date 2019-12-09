#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "superblock.h"

// CS 4398 Final Project - GIF File Recovery
// Authors:
// Michael Barre
// Zion Mantey
// David Mendoza

#define DIR_REC_LEN(DirEntry) (ceil((float)(DirEntry.name_len + \
											sizeof(UINT8)) /    \
									4) *                        \
							   4);

// Author: N/A, copied from source code
// given a buffer and a location, read the Inode Directory entry into pDirEntry
// used to read all the inodes in a directory block so that a new INode entry can be inserted at the end
INT4 InodeDirReadRecord(CHAR *pEntries, UINT4 u4StartPos,
						struct ext3_dir_entry_2 *pDirEntry)
{
	CHAR *pPos = NULL;

	if (pEntries == NULL)
	{
		return -1;
	}

	pPos = pEntries + u4StartPos;
	memcpy(pDirEntry, pPos, sizeof(UINT8));
	strncpy(pDirEntry->name, pPos + sizeof(UINT8),
			pDirEntry->name_len);
	return 1;
}

// Author: N/A, copied from source code
/*
This utility function compares the hex values of two character arrays upto a certain length (n)
Used to search blocks for the GIF file header
*/
int compareHexValues(unsigned char string1[], unsigned char string2[], int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		if ((int)string1[i] == (int)string2[i])
		{
			if (i == n - 1)
			{
				return 1;
			}
		}
		else
		{
			return 0;
		}
	}
	return 0;
}

// Author: Zion Mantey
// given an input stream of blocks, search through the blocks for the GIF file header
// returns the block number of the first header found, this is the block where file recovery should start
int searchGIF(int fp)
{
	printf("GIF");
	unsigned char header[4];
	header[0] = 0x47;
	header[1] = 0x49;
	header[2] = 0x46;
	header[3] = 0x38;

	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	int blocks_in_partition = gPrimarySuperblock.s_blocks_count;
	printf(" %d\n", block_size);

	unsigned char b[8];
	int headerLength = 4;

	if (lseek(fp, 0, SEEK_SET) < 0)
	{
		perror("lseek() error");
	}
	if (read(fp, b, headerLength) < 0)
	{
		perror("read() error");
	}

	// for (int i = 0; i < headerLength; i++)
	// {
	// 	printf("%x", header[i]);
	// }
	// printf("\n");

	for (int i = 0; i < blocks_in_partition; i++)
	{
		// for (int i = 0; i < headerLength; i++)
		// {
		// 	printf("%x", b[i]);
		// }
		// printf(":%d\n", i);
		if (compareHexValues(b, header, headerLength) == 1)
		{
			printf("File header found at block %d\n", i);
			return i;
		}
		if (lseek(fp, block_size - headerLength, SEEK_CUR) < 0)
		{
			perror("lseek() error");
		}
		if (read(fp, b, headerLength) < 0)
		{
			perror("read() error");
		}
	}
}

// Author: Michael Barre
// fp is the I/O stream for the filesystem
// block is a block containing directory records
// inode is the number of the inode to be added to the end of the list of records
// Used to place a recovered file into a directory, after the inode has been created
void insertInodeIntoRoot(int fp, int block, int inode)
{
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	lseek(fp, block * block_size, SEEK_SET);
	unsigned char buf[block_size];
	read(fp, buf, block_size);
	// go through all the current records
	struct ext3_dir_entry_2 DirEntry;
	unsigned int nextRecord = 0;
	unsigned int readSoFar = 0;
	while (1)
	{
		memset(&DirEntry, 0, sizeof(DirEntry));
		InodeDirReadRecord(buf, nextRecord, &DirEntry);
		nextRecord += DirEntry.rec_len;
		// when we find the last entry, exit the loop
		if (nextRecord == block_size)
			break;
		else
			readSoFar += DirEntry.rec_len;
	}
	DirEntry.rec_len = DIR_REC_LEN(DirEntry);
	memcpy(buf + readSoFar, &DirEntry, DirEntry.rec_len);

	struct ext3_dir_entry_2 NewEntry;
	memset(&NewEntry, 0, sizeof(NewEntry));
	NewEntry.inode = inode;
	NewEntry.rec_len = block_size - (readSoFar + DirEntry.rec_len);
	NewEntry.file_type = 1;
	char *name = "zfile.gif";
	strcpy(NewEntry.name, name);
	NewEntry.name_len = 9;
	memcpy(buf + readSoFar + DirEntry.rec_len, &NewEntry, NewEntry.rec_len);
	lseek(fp, block * block_size, SEEK_SET);
	write(fp, buf, block_size);
}

// Author: N/A, copied from source code
//Converts 4 byte hexadecimal to decimal
long long hexToNum(unsigned char hex[])
{
	return ((long long)hex[3] << 24) | (hex[2] << 16) | (hex[1] << 8) | hex[0];
}

// Author: N/A, copied from source code
/*
This utility function compares the hex values of two character arrays upto a certain length (n)
Used for detecting indirect blocks
*/
int compareHexValuesSequential(unsigned char string1[], unsigned char string2[], int n)
{
	int i;
	int num1;
	for (i = 0; i < n; i++)
	{
		if (i == 0)
		{
			num1 = (int)string1[i] + 1;
		}
		else
		{
			num1 = string1[i];
		}

		if (num1 == (int)string2[i])
		{
			if (i == n - 1)
			{
				return 1;
			}
		}
		else
		{
			return 0;
		}
	}
	return 0;
}

// Author: N/A, copied from source code
// from an I/O stream of a filesystem, set gPrimarySuperblock (a global variable from included header files) to the first superblock in the filesystem
void sbGetPrimarySuperblock(int pFile)
{
	// printf("GET SUPERBLOCK\n");
	tSuperblock primarySB;

	if (lseek(pFile, SB_OFFSET, SEEK_SET) < 0)
	{
		perror("lseek() error");
		exit(1);
	}

	if (read(pFile, &primarySB, sizeof(tSuperblock)) < 0)
	{
		perror("read() error");
		exit(1);
	}

	gPrimarySuperblock = primarySB;
}

// Author: David Mendoza
// Modified by: entire team during final project work
// calculate information about an indirect block
// fp is an I/O stream for a filesystem
// block_num is the number of the block being tested
// last_block is the last entry in the indirect block, which will later be searched for the GIF EOF
// returns index of last used block in order to accurately calculate file size
int isIndirectBlock(int fp, int block_num, int *last_block)
{
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	unsigned char block[block_size];
	if (lseek(fp, block_num * block_size, SEEK_SET) < 0)
	{
		perror("lseek() error");
	}
	if (read(fp, block, block_size) < 0)
	{
		perror("read() error");
	}

	int count = 0;
	//int last = 0; //count # of consecutive blocks
	int size = block_size / 4;
	unsigned char b1[4];
	unsigned char b2[4];
	int index = -1;

	for (int i = 0; i < size - 1; i++)
	{
		memcpy(b1, block + (i * 4), sizeof(unsigned char) * 4);
		memcpy(b2, block + ((i + 1) * 4), sizeof(unsigned char) * 4);
		if (compareHexValuesSequential(b1, b2, 4) == 1)
		{
			//printf("b1: %d(%llu)\t b2: %d(%llu)\n", (int) b1[0], hexToNum(b1), (int)b2[0], hexToNum(b2));
			count++;
			// last = i + 1;
			index = i;
		}
		else if (hexToNum(b1) == 0 && hexToNum(b2) == 0)
		{
			index = i - 1;
			break; // no more entries in block
		}
	}

	// //get first and last entries
	// memcpy(b1, block, sizeof(unsigned char) * 4);
	// memcpy(b2, block + (last*4), sizeof(unsigned char)*4);

	if (count > 3)
	{ //found indirect block -- greater than 3 in case there are 2 consecutive hex values
		//printf("consecutive: %d -- \n", count);
		// printf("Block: %d\t First Entry: %llu\t Last Entry: %llu\n", block_num, hexToNum(b1), hexToNum(b2));
		memcpy(b1, block + (index * 4), sizeof(unsigned char) * 4);
		*last_block = (int)hexToNum(b1);
		return index;
	}

	return 0;
}

/*
This utility function is used to get the hexadecimal address of the block address
*/
char *decimalToHexStringInReverseOrder(int decimalNumber)
{
	char *signs = (char *)malloc(sizeof(char) * 4);
	signs[0] = decimalNumber & 0xff;
	signs[1] = (decimalNumber >> 8) & 0xff;
	signs[2] = (decimalNumber >> 16) & 0xff;
	signs[3] = (decimalNumber >> 24) & 0xff;

	return signs;
}

// Author: David Mendoza
// fp is the I/O stream for a filesystem
// firstDatablock is the first block containing a file's data
// searches the file system for an indirect block with blocks of this file and returns the block number
int findIndirectPointerBlock(int fd, int firstDatablock)
{
	int addrLength = 16;
	unsigned char *addr = malloc(sizeof(char) * addrLength); //store 4 adjacent addresses of pointers
	int indBlock = firstDatablock + 12;
	addr = decimalToHexStringInReverseOrder(indBlock);
	for (int i = 0; i < 3; i++)
	{
		addr[4 * (i + 1)] = addr[4 * i] + 1;
		addr[4 * (i + 1) + 1] = addr[1];
		addr[4 * (i + 1) + 2] = addr[2];
		addr[4 * (i + 1) + 3] = addr[3];
	}
	//    for (int i=0; i<16; i++)
	//    {
	//        printf("%x\n", addr[i]);
	//    }
	unsigned char b[16];
	long long offset = 0;
	lseek64(fd, offset, SEEK_SET);
	read(fd, b, addrLength);
	int n = 0;
	// printf("Searching for indirect pointer block\n");
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	for (int i = 0; i < block_size * 8; i++)
	{
		if (compareHexValues(b, addr, addrLength) == 1)
		{
			// printf("Indirect pointer block found at block %d, its first entry points to block %d\n", i, indBlock);
			return i;
			n++;
		}
		lseek64(fd, block_size - addrLength, SEEK_CUR);
		read(fd, b, addrLength);
	}

	if (n == 0)
	{
		printf("Unable to find indirect pointer block\n");
	}
	free(addr);
	return 0;
}

// Author: Entire team during final project work
// Search a block for the gif EOF marker, starting from the bottom of the block and going until the EOF is found
// returns the length of non-null data in the block
// fp is an I/O stream for a file system
// last_block is the block number that is being searched for the GIF EOF
int findEOF(int fp, int last_block)
{
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	unsigned char block[block_size];
	if (lseek(fp, last_block * block_size, SEEK_SET) < 0)
	{
		perror("lseek() error");
	}
	if (read(fp, block, block_size) < 0)
	{
		perror("read() error");
	}

	for (int i = block_size - 1; i >= 0; i--)
	{
		if (block[i] == 0x3B)
		{
			return i;
		}
	}
}

// int findIndirectBlocks(int fp)
// {
// 	// lseek(fp, 0, SEEK_SET);

// 	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
// 	int blocks_in_partition = gPrimarySuperblock.s_blocks_count;

// 	printf("Block Size: %d\n", block_size);

// 	unsigned char block[block_size];

// 	if (lseek(fp, 0, SEEK_SET) < 0)
// 	{
// 		perror("lseek() error");
// 	}
// 	if (read(fp, block, block_size) < 0)
// 	{
// 		perror("read() error");
// 	}

// 	for (int i = 0; i < blocks_in_partition; i++)
// 	{
// 		//check if its an indirect block
// 		if(isIndirectBlock(block, block_size, i) == 1){
// 			return i;
// 		}

// 		if (read(fp, block, block_size) < 0)
// 		{
// 			perror("read() error");
// 		}
// 	}
// }

// Author: Entire team during final project work
// Inserts an inode into the first free inode in the filesystem
// fp is the I/O stream for a filesystem
// inode_struct is the inode that needs to be inserted
// used once a file that needs to be recovered is fully located and the inode needs to be stored in the filesystem
int insertInodeIntoGD(int fp, struct ext3_inode inode_struct)
{
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;

	struct ext3_group_desc GroupDes;
	if (lseek(fp, 1 * block_size, SEEK_SET) < 0)
	{
		perror("lseek() error");
	}
	int count = -1;
	do
	{
		count++;
		if (read(fp, &GroupDes, sizeof(GroupDes)) < 0)
		{
			perror("read() error");
		}
	} while (GroupDes.bg_free_inodes_count == 0);

	unsigned char block[block_size];

	if (lseek(fp, GroupDes.bg_inode_bitmap * block_size, SEEK_SET) < 0) //read inode bitmap
	{
		perror("lseek() error");
	}
	if (read(fp, &block, sizeof(block)) < 0)
	{
		perror("read() error");
	}

	int inode = 0;
	for(;inode < gPrimarySuperblock.s_inodes_per_group; inode++){
		if((block[inode/8] & (1 <<(inode % 8))) == 0){
			break;
		}
	}

	// printf("%d\n", count * gPrimarySuperblock.s_inodes_per_group + inode + 1);

	block[inode/8] |= (1 <<(inode % 8)); //mark inode busy

	if (lseek(fp, GroupDes.bg_inode_bitmap * block_size, SEEK_SET) < 0)
	{
		perror("lseek() error");
	}
	write(fp, block, block_size); //write inode bitmap back


	
	if (lseek(fp, GroupDes.bg_inode_table * block_size + gPrimarySuperblock.s_inode_size * inode, SEEK_SET) < 0) //read inode table
	{
		perror("lseek() error");
	}

	if (write(fp, &inode_struct, sizeof(inode_struct)) < 0)
	{
		perror("write() error");
	}

	return count * gPrimarySuperblock.s_inodes_per_group + inode + 1;
}

// Author: Entire team during final project work
// main function for GIF file recovery
// expects 2 command line arguments, the device with the filesystem
// and the block number of the directory entries that the recovered file should be inserted into
int main(int argc, char **argv)
{
	// argument validation
	if (argc != 3)
	{
		printf("Error: Wrong number of args, 2 expected\n");
		return -1;
	}
	// open the filesystem
	int fp = open(argv[1], O_RDWR);
	// printf("FP:%d\n", fp);

	if (fp < 0)
	{
		printf("Error %s\n", strerror(errno));
		return -1;
	}
	// set the primary super block, which will be used in several places
	sbGetPrimarySuperblock(fp);
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	
	// find the first block in the filesystem with a GIF header, this is the file we need to recover
	int first_block = searchGIF(fp);
	printf("%d\n", first_block);

	// find the indirect block for the same file
	int indr_block = findIndirectPointerBlock(fp, first_block);
	printf("%d\n", indr_block);

	int last_block;
	// get how many entries are in the indirect block for calculating file size
	int indr_block_count = isIndirectBlock(fp, indr_block, &last_block);
	printf("%d %d\n", (indr_block_count + 12) * 4096, last_block);
	
	// find the EOF marker in the last block of the file for calculating file size
	int bytes = findEOF(fp, last_block);

	// the final file size is the bytes from the last block, plus the space taken up by the other indirect blocks and the first 12 blocks
	int total_bytes = bytes + 1 + (indr_block_count + 12) * 4096;
	printf("size bytes: %d\n", total_bytes);

	// create a new inode struct and fill out all the necessary information to recover a file
	struct ext3_inode new_inode;
	memset(&new_inode, 0, sizeof(new_inode));

	new_inode.i_size = total_bytes;
	for (int i = 0; i < 12; i++)
	{
		new_inode.i_block[i] = first_block + i;
	}
	new_inode.i_block[12] = indr_block;

	// hardcoded value to set the permissions on the file
	new_inode.i_mode = 33279;
	// new_inode.i_gid = 1000;
	new_inode.i_links_count = 1;
	// i_blocks is the number of sectors the file takes up
	new_inode.i_blocks = (indr_block_count + 12 + 2) * (block_size / 512);
	printf("blocks: %d\n", new_inode.i_blocks);

	// insert the new inode struct into the first free inode
	int inode_number = insertInodeIntoGD(fp, new_inode);
	printf("free inode: %d\n", inode_number);

	// add the new inode to the directory entries in the block originally provided
	insertInodeIntoRoot(fp, atoi(argv[2]), inode_number);

	close(fp);
	return 0;
}

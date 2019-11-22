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

//Converts 4 byte hexadecimal to decimal
long long hexToNum(unsigned char hex[]){
	return ((long long) hex[3] << 24) | (hex[2] << 16) | (hex[1] << 8) | hex[0];
} 

/*
This utility function compares the hex values of two character arrays upto a certain length (n)
*/
int compareHexValues(unsigned char string1[], unsigned char string2[], int n)
{
	int i;int num1;
	for (i = 0; i < n; i++)
	{
		if(i == 0){
			num1 = (int) string1[i] +1;
		}else{
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

int isIndirectBlock(unsigned char block[], int block_size, int block_num){
	int count = 0; int last = 0; //count # of consecutive blocks	
	int size = block_size/4;	
	unsigned char b1[4];
	unsigned char b2[4];

	for(int i = 0; i < size - 1; i++){
		memcpy(b1, block+(i*4), sizeof(unsigned char)*4);
		memcpy(b2, block+((i+1)*4), sizeof(unsigned char)*4);
		if(compareHexValues(b1, b2, 4) == 1){
			//printf("b1: %d(%llu)\t b2: %d(%llu)\n", (int) b1[0], hexToNum(b1), (int)b2[0], hexToNum(b2));			
			count++;
			last = i + 1;
		}else if(hexToNum(b1)==0 && hexToNum(b2) == 0){		
			break; // no more entries in block
		}

	}

	//get first and last entries
	memcpy(b1, block, sizeof(unsigned char) * 4);
	memcpy(b2, block + (last*4), sizeof(unsigned char)*4);
	
	if(count > 3){ //found indirect block -- greater than 3 in case there are 2 consecutive hex values
		//printf("consecutive: %d -- \n", count);
		printf("Block: %d\t First Entry: %llu\t Last Entry: %llu\n", block_num, hexToNum(b1), hexToNum(b2));
		return 1;
	} 
			

	return 0;	 
}

void findIndirectBlocks(int fp)
{
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	int blocks_in_partition = gPrimarySuperblock.s_blocks_count;
	
	printf("Block Size: %d\n", block_size);

	unsigned char block[block_size];

	if (lseek(fp, 0, SEEK_SET) < 0)
	{
		perror("lseek() error");
	}
	if (read(fp, block, block_size) < 0)
	{
		perror("read() error");
	}

	for (int i = 0; i < blocks_in_partition; i++)
	{
		//check if its an indirect block		
		if(isIndirectBlock(block, block_size, i) == 1){
			//printf("FOUND BLOCK");
		}
		
		if (read(fp, block, block_size) < 0)
		{
			perror("read() error");
		}
	}

	//printf("DONE READING!\n");
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Error: Wrong number of args, 2 expected\n");
		return -1;
	}
	int fp = open(argv[1], O_RDONLY);
	// printf("FP:%d\n", fp);

	if (fp < 0)
	{
		printf("Error %s\n", strerror(errno));
		return -1;
	}
	sbGetPrimarySuperblock(fp);
	findIndirectBlocks(fp);
	close(fp);
	return 0;
}

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
long hexToNum(unsigned char hex[]){
	return ((long) hex[3] << 24) | (hex[2] << 16) | (hex[1] << 8) | hex[0];
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

void searchGIF(int fp)
{
	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	int blocks_in_partition = gPrimarySuperblock.s_blocks_count;
	int totalBlockNum = block_size/4;
	
	printf("Block Size: %d\tExpected Block Numbers: %d\n", block_size, totalBlockNum);

	unsigned char currentBlockNum[4];
	unsigned char prevBlockNum[4];
	int blockLength = 4;

	if (lseek(fp, 0, SEEK_SET) < 0)
	{
		perror("lseek() error");
	}
	if (read(fp, prevBlockNum, blockLength) < 0)
	{
		perror("read() error");
	}

	for (int i = 0; i < blocks_in_partition; i++)
	{
		long firstBlock = hexToNum(prevBlockNum); long lastBlock = -1;
		
		//traverse through current block
		int j = 1; int n = 0;
		for(; j < totalBlockNum; j++){			
			
			if(read(fp, currentBlockNum, blockLength) < 0){
				perror("read() error");
			}

			long prev = firstBlock; long cur = hexToNum(currentBlockNum);
			//compare blocks
			if(prev + 1 == cur){
				n++;
			}else{
				if(j > 3 && n == 0){
					j++;
					break; //not an indirect block
				}else if(j > 3 && n > 1){
					j++;
					lastBlock = prev; 
					break;
				}
			}
			
			prev = cur;				
		} 
		
		//print indirect block
		if(firstBlock != -1 && lastBlock != -1){
			printf("Block Number: %d\tFirst Block: %lu\tLast Block: %lu Total Number: %d\n", i, firstBlock, lastBlock, n);
		}		

		if (lseek(fp, block_size - (blockLength * j), SEEK_CUR) < 0)
		{
			printf("lseek() error %d, %d, %d", block_size, blockLength, j);
			perror("ERROR\n");
		}
		if (read(fp, prevBlockNum, blockLength) < 0)
		{
			perror("read() error");
		}
	}

	printf("DONE READING!\n");
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
	searchGIF(fp);
	close(fp);
	return 0;
}

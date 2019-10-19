#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "superblock.h"

/*
This utility function compares the hex values of two character arrays upto a certain length (n)
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

void sbGetPrimarySuperblock(int *pFile)
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

void searchGIF(int *fp)
{
	// printf("GIF");
	unsigned char header[4];
	header[0] = 0x47;
	header[1] = 0x49;
	header[2] = 0x46;
	header[3] = 0x38;

	unsigned int block_size = 1024 << gPrimarySuperblock.s_log_block_size;
	int blocks_in_partition = gPrimarySuperblock.s_blocks_count;
	// printf(" %d\n", block_size);

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

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Error: Wrong number of args, 2 expected\n");
		return -1;
	}
	int *fp = open(argv[1], O_RDONLY);
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

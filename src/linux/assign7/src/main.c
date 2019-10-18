#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "construct.c"

extern int init(char *disk);
extern int getBlockSize();
extern int getBlocksPerGroup();

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

void searchGIF(int *fp)
{
	printf("GIF");
	unsigned char header[4];
	header[0] = 0x47;
	header[1] = 0x49;
	header[2] = 0x46;
	header[3] = 0x38;

	int block_size = 4096;			   //getBlockSize();
	int Data_blocks_per_group = 32768; //getBlocksPerGroup();
	printf(" %d\n", block_size);

	unsigned char b[8];
	int headerLength = 4;
	lseek64(fp, 0, SEEK_SET);
	read(fp, b, headerLength);

	for (int i = 0; i < headerLength; i++)
	{
		printf("%x", header[i]);
	}
	printf("\n");

	for (int i = 0; i < Data_blocks_per_group; i++)
	{
		for (int i = 0; i < headerLength; i++)
		{
			printf("%x", b[i]);
		}
		printf(":%d\n", i);
		if (compareHexValues(b, header, headerLength) == 1)
		{
			printf("File header found at block %d\n", i);
		}
		lseek64(fp, block_size - headerLength, SEEK_CUR);
		read(fp, b, headerLength);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Error: Missing args\n");
		return -1;
	}
	int *fp = open(argv[1], O_RDONLY);
	if (fp < 0)
	{
		printf("Error %s\n", strerror(errno));
		return -1;
	}
	// init(fp);
	searchGIF(fp);
	fclose(fp);
	return 0;
}

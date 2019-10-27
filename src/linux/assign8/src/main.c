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

#define DIR_REC_LEN(DirEntry) (ceil((float)(DirEntry.name_len + \
                            sizeof(UINT8)) / 4) * 4); 

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

void insertInode(int fp, int block, int inode)
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
	char* name = "zfile.gif";
	strcpy(NewEntry.name, name);
	NewEntry.name_len = 9;
	memcpy(buf + readSoFar + DirEntry.rec_len, &NewEntry, NewEntry.rec_len);
	lseek(fp, block * block_size, SEEK_SET);
	write(fp, buf, block_size);
}

// argv[1] is the device, argv[2] is the block number, argv[3] is the inode to insert
int main(int argc, char **argv)
{
	if (argc != 4)
	{
		printf("Error: Wrong number of args, 4 expected\n");
		return -1;
	}
	int fp = open(argv[1], O_RDWR);
	// printf("FP:%d\n", fp);

	if (fp < 0)
	{
		printf("Error %s\n", strerror(errno));
		return -1;
	}
	sbGetPrimarySuperblock(fp);
	insertInode(fp, atoi(argv[2]), atoi(argv[3]));
	close(fp);
	return 0;
}

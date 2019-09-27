/* Digital Forensics - Spring 2016
 *
 * GBD.c - writes the group descriptor table data into the output file(hex dump as well as key:value).
 * compares group descriptor table of 2 different block group number.
 * @author - Anoop S Somashekar
 * @author of checkAddrBlock - Mahesh KS
 */

#include "Group_Desc.h"
//#include "conio.h"

extern struct ext3_group_desc *gGrpDescTable;

/* returns the maximum element from 2 numbers */
int bgdMax(int a, int b) {
    if (a > b)
        return a;
    else
        return b;
}

/*Group descriptor will be duplicated at 0,1 and powers of 3, 5, 7
 For e.g it is duplicated at block group number 0, 1, 3, 5, 7, 9, 25, 27, 49, 81, 125 etc....*/
long long bgdGetGroupDescStartOffset(int index) {
    /* blockSize * 8 gives the number of blocks in a block group index is the block group number
     * from which we need the block group descriptor table since first block of any block group
     * is super block, 1 is added to get the starting offset for group desc table (index * blockSize * 8 + 1)
     * is the block number. Since we need byte offset to read the data, it is multiplied by blocksize.*/
    if (index == 0) {
        if (gBlockSize == 1024) {
            return 2 * gBlockSize;
        } else {
            return gBlockSize;
        }
    }
    long long value = (index * gBlockSize * BITS_IN_A_BYTE) + 1;
    value *= gBlockSize;
    return value;
}

/* returns the block size for the given drive name */
int bgdGetBlockSize(char driveName[]) {
    int fd = open(driveName, O_RDONLY);
    if (fd < 0) {
        fputs("memory error", stderr);
        exit(2);
    }

    int offset = SUPER_BLOCK_OFFSET + BLOCK_SIZE_VALUE_OFFSET;
    int blockSize;
    unsigned char buffer[4];
    lseek(fd, offset, SEEK_CUR);
    read(fd, buffer, sizeof(int));
    memcpy(&blockSize, buffer, sizeof(int));
    blockSize = MIN_BLOCK_SIZE << blockSize;
    return blockSize;
}

int bgdGetNumberOfBlocksInGroup(int fd) {

    int offset = SUPER_BLOCK_OFFSET;
    int totalNumOfInodes, totalNumOfBlocks, numOfBlocksInGroup, numOfInodesInGroup;
    unsigned char buffer[4];

    /* read 4 bytes data(int) i.e. total number of inodes */
    lseek(fd, offset, SEEK_SET);
    read(fd, buffer, sizeof(int));
    memcpy(&totalNumOfInodes, buffer, sizeof(int));

    /* read 4 bytes of int data i.e. total number of blocks */
    read(fd, buffer, sizeof(int));
    memcpy(&totalNumOfBlocks, buffer, sizeof(int));

    /* read 4 bytes of data(int) i.e. number of blocks in a group at offset 32 */
    lseek(fd, NUM_OF_BLOCKS_IN_A_GROUP_REL_OFFSET, SEEK_CUR);
    read(fd, buffer, sizeof(int));
    memcpy(&numOfBlocksInGroup, buffer, sizeof(int));

    return numOfBlocksInGroup;
}

/* Returns the total number of block on the drive which is passed as file descriptor*/
int bgdGetTotalNumberOfBlocks(int fd) {

    int offset = SUPER_BLOCK_OFFSET;
    int totalNumOfInodes, totalNumOfBlocks, numOfBlocksInGroup, numOfInodesInGroup;
    unsigned char buffer[4];

    /* read 4 bytes data(int) i.e. total number of inodes */
    lseek(fd, offset, SEEK_CUR);
    read(fd, buffer, sizeof(int));
    memcpy(&totalNumOfInodes, buffer, sizeof(int));

    /* read 4 bytes of int data i.e. total number of blocks */
    read(fd, buffer, sizeof(int));
    memcpy(&totalNumOfBlocks, buffer, sizeof(int));

    return totalNumOfBlocks;
}

/* returns the total number of block groups in the group descriptor table */
int bgdGetNumberofBlockGroups(char driveName[]) {
    int fd = open(driveName, O_RDONLY);
    if (fd < 0) {
        fputs("memory error", stderr);
        exit(2);
    }

    int offset = SUPER_BLOCK_OFFSET;
    int totalNumOfInodes, totalNumOfBlocks, numOfBlocksInGroup, numOfInodesInGroup;
    unsigned char buffer[4];

    /* read 4 bytes data(int) i.e. total number of inodes */
    lseek(fd, offset, SEEK_CUR);
    read(fd, buffer, sizeof(int));
    memcpy(&totalNumOfInodes, buffer, sizeof(int));

    /* read 4 bytes of int data i.e. total number of blocks */
    read(fd, buffer, sizeof(int));
    memcpy(&totalNumOfBlocks, buffer, sizeof(int));

    /* read 4 bytes of data(int) i.e. number of blocks in a group at offset 32 */
    lseek(fd, NUM_OF_BLOCKS_IN_A_GROUP_REL_OFFSET, SEEK_CUR);
    read(fd, buffer, sizeof(int));
    memcpy(&numOfBlocksInGroup, buffer, sizeof(int));

    /* read 4 bytes of data(int) i.e. number of indoes in a group at a relative offset 40 */
    lseek(fd, NUM_OF_INODES_IN_A_GROUP_REL_OFFSET, SEEK_CUR);
    read(fd, buffer, sizeof(int));
    memcpy(&numOfInodesInGroup, buffer, sizeof(int));

    int numOfBlockGroups1 = totalNumOfInodes / numOfInodesInGroup;
    int numOfBlockGroups2 = totalNumOfBlocks / numOfBlocksInGroup;
    return bgdMax(numOfBlockGroups1, numOfBlockGroups2);
}

/*Function returns 1 if the number is power of 3 or 5 or 7.
 else returns 0. */
int bgdIsPowerOf3_5_7(int number) {
    int num = number;
    if (num == 0)
        return 1;

    /* If the number is power of 3 */
    while (num % 9 == 0) {
        num /= 9;
    }

    if (num == 1 || num == 3)
        return 1;

    num = number;
    /* If the number is power of 5 */
    while (num % 5 == 0) {
        num /= 5;
    }

    if (num == 1)
        return 1;

    num = number;
    /* If the number is power of 7 */
    while (num % 7 == 0) {
        num /= 7;
    }

    if (num == 1)
        return 1;

    /* If the number is neither power of 3 or 5 or 7 then return 0. */
    return 0;
}

/*returns empty string if group descriptor table at blkGrpNum1 and blkGrpNum2 are identical.
 else returns the diff string*/
char *bgdCompareGrpDesc(int blkGrpNum1, int blkGrpNum2, char driveName[]) {
    int fd1 = open(driveName, O_RDONLY);
    int fd2 = open(driveName, O_RDONLY);

    if (!bgdIsPowerOf3_5_7(blkGrpNum1) || !bgdIsPowerOf3_5_7(blkGrpNum2)) {
        return "Invalid block group number";
    }

    /* get the starting offset of the group descriptor table
     * for the block group numbers blkGrpNum1 and blkGrpNum2 */
    long long offset1 = bgdGetGroupDescStartOffset(blkGrpNum1);
    long long offset2 = bgdGetGroupDescStartOffset(blkGrpNum2);

    lseek64(fd1, offset1, SEEK_SET);
    lseek64(fd2, offset2, SEEK_SET);

    int bg_iterator = 0;
    struct ext3_group_desc *gdesc1 = (struct ext3_group_desc *) malloc(sizeof(struct ext3_group_desc));
    struct ext3_group_desc *gdesc2 = (struct ext3_group_desc *) malloc(sizeof(struct ext3_group_desc));

    char *buff1 = (char *) malloc(sizeof(struct ext3_group_desc));
    char *buff2 = (char *) malloc(sizeof(struct ext3_group_desc));
    char *compResult = (char *) malloc(sizeof(struct ext3_group_desc) * gBlockGroupCount * 100);
    memset(compResult, 0, sizeof(struct ext3_group_desc) * gBlockGroupCount * 100);
    while (bg_iterator < gBlockGroupCount) {
        read(fd1, buff1, sizeof(struct ext3_group_desc));
        memcpy((void *) gdesc1, (void *) buff1, sizeof(struct ext3_group_desc));

        read(fd2, buff2, sizeof(struct ext3_group_desc));
        memcpy((void *) gdesc2, (void *) buff2, sizeof(struct ext3_group_desc));

        char temp[100];
        if (gdesc1->bg_block_bitmap != gdesc2->bg_block_bitmap) {
            memset(temp, 0, 100);
            sprintf(temp, "Block bitmap[%d]  ->  %ld | %ld\n", bg_iterator, gdesc1->bg_block_bitmap,
                    gdesc2->bg_block_bitmap);
            strcat(compResult, temp);
        }
        if (gdesc1->bg_inode_bitmap != gdesc2->bg_inode_bitmap) {
            memset(temp, 0, 100);
            sprintf(temp, "Inode bitmap[%d]  ->  %ld | %ld\n", bg_iterator, gdesc1->bg_inode_bitmap,
                    gdesc2->bg_inode_bitmap);
            strcat(compResult, temp);
        }
        if (gdesc1->bg_inode_table != gdesc2->bg_inode_table) {
            memset(temp, 0, 100);
            sprintf(temp, "Inode table[%d]   ->  %ld | %ld\n", bg_iterator, gdesc1->bg_inode_table,
                    gdesc2->bg_inode_table);
            strcat(compResult, temp);
        }
        if (gdesc1->bg_free_blocks_count != gdesc2->bg_free_blocks_count) {
            memset(temp, 0, 100);
            sprintf(temp, "Free blocks[%d]   ->  %d | %d\n", bg_iterator, gdesc1->bg_free_blocks_count,
                    gdesc2->bg_free_blocks_count);
            strcat(compResult, temp);
        }
        if (gdesc1->bg_free_inodes_count != gdesc2->bg_free_inodes_count) {
            memset(temp, 0, 100);
            sprintf(temp, "Free inodes[%d]   ->  %d | %d\n", bg_iterator, gdesc1->bg_free_inodes_count,
                    gdesc2->bg_free_inodes_count);
            strcat(compResult, temp);
        }
        if (gdesc1->bg_used_dirs_count != gdesc2->bg_used_dirs_count) {
            memset(temp, 0, 100);
            sprintf(temp, "Used dirs[%d]     ->  %d | %d\n", bg_iterator, gdesc1->bg_used_dirs_count,
                    gdesc2->bg_used_dirs_count);
            strcat(compResult, temp);
        }
        bg_iterator++;
    }
    free(buff1);
    free(buff2);
    free(gdesc1);
    free(gdesc2);
    return compResult;
}

/* init function which will be called at the start to update block size
 * block group count and group descriptor table */
int bgdInit(char *dName) {
    gBlockSize = bgdGetBlockSize(dName);
    gBlockGroupCount = bgdGetNumberofBlockGroups(dName);

    gGrpDescTable = bgdGetGrpDescTable(dName, 0, 0);
    return gBlockSize;
}


/* iterate through all the blocks of the blockGroupNo
 * update the bitmap to indicate the null blocks among
 * unused blocks */
void bgdUpdateNullBlocks(int fd, int blockGroupNo, char *colorCodeBitmap) {
    int maxBlocks = gBlockSize * BITS_IN_A_BYTE;
    int i;
    int count = 0;
    for (i = 0; i < maxBlocks; ++i) {
        if (!colorCodeBitmap[i]) {
            int blockNum = i + (blockGroupNo * gBlockSize * BITS_IN_A_BYTE);
            long long offset = blockNum * gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            unsigned char content[4] = {0};
            read(fd, content, 4);

            /* if the string length of the first four bytes is zero
             * it can be concluded that the data block is empty */
            if (strlen(content) == 0) {
                colorCodeBitmap[i] = (int) NULL_BLOCK;
                count++;
            }
        }
    }
    printf("Total number of null blocks(unreserved) are %d\n", count);
}

/*new function added to check block and classify it in address block, Null block, Text block*/
int bgdUpdateBlocks(int fd, int blockGroupNo, char *colorCodeBitmap, int totalNumberOfBlocks,struct ext3_deleted_dir_entry *delDir) {
    int maxBlocks = gBlockSize * BITS_IN_A_BYTE;
    int i;
    int null_count = 0;
    int addr_count = 0;
    int text_count = 0;
    int dir_count = 0;
    int dirCount = 0;
    int entryCount = 0;
    for (i = 0; i < maxBlocks; ++i) {
        if (!colorCodeBitmap[i]) {

            /* check for address block*/
            int blockNum = i + (blockGroupNo * gBlockSize * BITS_IN_A_BYTE);
            UINT8 offset = (UINT8) blockNum * gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            char *data = (char *) malloc(gBlockSize);
            char *freeThisPointer = data;
            memset((void *) data, 0, gBlockSize);
            read(fd, data, gBlockSize);
            int addr1, addr2;

            /* read first eight bytes of data into an integer variable */
            memcpy(&addr1, data, sizeof(int));
            data += sizeof(int);
            memcpy(&addr2, data, sizeof(int));

            /* if the difference between two integers at the first 8 bytes
             * is 1 then it can be concluded that it's an address block */
            if (abs(addr1 - addr2) == 1 && addr1 < totalNumberOfBlocks) {
                colorCodeBitmap[i] = (int) ADDR_BLOCK;
                addr_count++;
                free(freeThisPointer);
                continue;
            } else {
                /* If the integer value of the first 8 bytes are not consecutive
                 * then try next 8 bytes else conclude that it's not an address block */
                data += sizeof(int);
                memcpy(&addr1, data, sizeof(int));
                data += sizeof(int);
                memcpy(&addr2, data, sizeof(int));
                if (abs(addr1 - addr2) == 1 && addr1 < totalNumberOfBlocks) {
                    colorCodeBitmap[i] = (int) ADDR_BLOCK;
                    addr_count++;
                    free(freeThisPointer);
                    continue;
                }
            }

            unsigned char content[4] = {0};
            //read(fd, content, 4);
            content[0] = data[0];
            content[1] = data[1];
            content[2] = data[2];
            content[3] = data[3];

            /* if the string length of the first four bytes is zero
             * it can be concluded that the data block is empty */
            if (strlen(content) == 0) {
                colorCodeBitmap[i] = (int) NULL_BLOCK;
                null_count++;
            }

            /*Text and Dir Block*/
            int k;
            int found = 0;

            /* read the block data into directory structure */
            struct ext3_dir_entry_2 DirEntry;
            memset(&DirEntry, 0, sizeof(DirEntry));
            memcpy(&DirEntry, data, sizeof(DirEntry));

            /* First entry of any directory block is ".". After reading block data
             * into a directory structure, if it is "." and length is 1 then
             * it can be concluded that the data block is a directory block */
            if ((strcmp(DirEntry.name, ".")) == 0 && (DirEntry.name_len == 1)) {
                found = 1;
                colorCodeBitmap[i] = (int) DIR_BLOCK;
                dir_count++;
                bgdUpdateDirPath(fd, delDir, blockNum, dirCount, &entryCount);
                dirCount++;
                //continue;
            } else {
                for (k = 0; k < gBlockSize; ++k) {
                    /* If all the bytes in the data blocks are in
                     * the ASCII range then it can be concluded that
                     * the data block is a text block */
                    if (data[k] > LAST_ASCII_HEX) {
                        found = 1;
                        break;
                    }
                    /*else if(data[k] < 0x20)
                     {
                     if(data[k] != 0x09 && data[k] != 0x0A)
                     {
                     found = 1;
                     break;
                     }
                     }*/
                }
            }
            if (found == 0) {
                colorCodeBitmap[i] = (int) TEXT_BLOCK;
                text_count++;
            }
            /*end of text and Dir data*/


            free(freeThisPointer); // free up data block

        } //if (!colorCodeBitmap[i])
    }
    printf("Total number of null blocks(unreserved) are %d\n", null_count);
    printf("Total number of Address blocks are %d\n", addr_count);
    printf("Total number of Text blocks are %d\n", text_count);
    printf("Total number of Dir blocks are %d\n", dir_count);
    return dirCount + entryCount;
}

/* iterate through all the blocks of the blockGroupNo
 * update the bitmap to indicate the address blocks
 * among unused blocks*/
void bgdUpdateAddrBlocks(int fd, int blockGroupNo, char *colorCodeBitmap) {
    int maxBlocks = gBlockSize * BITS_IN_A_BYTE;
    int i;
    int count = 0;

    for (i = 0; i < maxBlocks; ++i) {
        if (!colorCodeBitmap[i]) {
            int blockNum = i + (blockGroupNo * gBlockSize * BITS_IN_A_BYTE);
            long long offset = blockNum * gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            char *data = (char *) malloc(gBlockSize);
            char *freeThisPointer = data;
            memset((void *) data, 0, gBlockSize);
            read(fd, data, gBlockSize);
            int addr1, addr2;

            /* read first eight bytes of data into an integer variable */
            memcpy(&addr1, data, sizeof(int));
            data += sizeof(int);
            memcpy(&addr2, data, sizeof(int));

            /* if the difference between two integers at the first 8 bytes
             * is 1 then it can be concluded that it's an address block */
            if (abs(addr1 - addr2) == 1) {
                colorCodeBitmap[i] = (int) ADDR_BLOCK;
                count++;
            } else {
                /* If the integer value of the first 8 bytes are not consecutive
                 * then try next 8 bytes else conclude that it's not an address block */
                data += sizeof(int);
                memcpy(&addr1, data, sizeof(int));
                data += sizeof(int);
                memcpy(&addr2, data, sizeof(int));
                if (abs(addr1 - addr2) == 1) {
                    colorCodeBitmap[i] = (int) ADDR_BLOCK;
                    count++;
                }
            }
            free(freeThisPointer);
        }
    }
    printf("Total number of address blocks(unreserved) are %d\n", count);
}

void createGlobalDeletedAddressBlocks() {
    //gDeletedAddressBlocks = (struct deleted_address_blocks) malloc (sizeof(struct deleted_address_blocks));
    gDeletedAddressBlocks.firstLevelHead = NULL;
    gDeletedAddressBlocks.secondLevelHead = NULL;
    gDeletedAddressBlocks.thirdLevelHead = NULL;
}

/* Check if a block passed as parameter is an address block and categorize it as first level, second level, or third level indirect blocks*/

int checkAddrBlock(int fd, char *colorAddressBitmap, int blockNum, int level) {
    int totalNumOfBlocks = bgdGetTotalNumberOfBlocks(fd);
    if (blockNum > totalNumOfBlocks) {
        return 0;
    }
    //static int blockGroupNo = bgdGetTotalNumberOfBlocks(fd);
    if (colorAddressBitmap[blockNum] != '\0') {
        if (colorAddressBitmap[blockNum] == ADDR_BLOCK) {
            // do nothing because we want to further analyze this
        } else if (colorAddressBitmap[blockNum] == NOT_AN_ADDRESS_BLOCK) {
            printf("Return 0\n");
            return 0;
        } else if (colorAddressBitmap[blockNum] == FIRST_LEVEL_INDIRECT_BLOCK) {
            printf("Return 1\n");
            return 1;
        } else if (colorAddressBitmap[blockNum] == SECOND_LEVEL_INDIRECT_BLOCK) {
            printf("Return 2\n");
            return 2;
        } else if (colorAddressBitmap[blockNum] == THIRD_LEVEL_INDIRECT_BLOCK) {
            printf("Return 3\n");
            return 3;
        }
    }

    UINT8 offset = (UINT8) blockNum * gBlockSize;
    lseek64(fd, offset, SEEK_SET);
    char *data = (char *) malloc(gBlockSize);
    char *buffer;
    buffer = data;
    memset((void *) data, 0, gBlockSize);
    read(fd, data, gBlockSize);
    int numberOfArrayEntries = gBlockSize / (sizeof(int));
    int addr1, addr2;
    int loop_counter = 0;
    int difference_counter = 0;

    /* read first eight bytes of data into an integer variable */
    memcpy(&addr1, data, sizeof(int));
    data += sizeof(int);
    memcpy(&addr2, data, sizeof(int));

    /* if the difference between two integers at the first 8 bytes
     * is 1 then it can be concluded that it's an address block */
    if (abs(addr1 - addr2) == 1) {
        difference_counter = 1;
    } else {
        /* If the integer value of the first 8 bytes are not consecutive
         * then try next 8 bytes else conclude that it's not an address block */
        data += sizeof(int);
        memcpy(&addr1, data, sizeof(int));
        data += sizeof(int);
        memcpy(&addr2, data, sizeof(int));
        if (abs(addr1 - addr2) == 1) {
            difference_counter = 1;
        }
    }

    int next_level = 0;

    if (difference_counter > 0 && blockNum <= totalNumOfBlocks) {
        // this is an address block
        next_level = checkAddrBlock(fd, colorAddressBitmap, addr1, level); //check which level address block this is
        lseek64(fd, offset, SEEK_SET);
        data = buffer;
        memset((void *) data, 0, gBlockSize);
        read(fd, data, gBlockSize);
        int *addr = (int *) malloc(numberOfArrayEntries * sizeof(int));
        // initialize the array of addresses
        for (loop_counter = 0; loop_counter < numberOfArrayEntries; loop_counter++) {
            memcpy(&addr[loop_counter], data, sizeof(int));
            data += sizeof(int);
        }
        if (next_level == 0) { // that is this is the first level indirect block

            // mark this block as the FIRST_LEVEL_INDIRECT_BLOCK
            colorAddressBitmap[blockNum] = FIRST_LEVEL_INDIRECT_BLOCK;

            //mark every block it is pointing to as NOT_AN_ADDRESS_BLOCK
            for (loop_counter = 0; loop_counter < numberOfArrayEntries; loop_counter++) {
                if (addr[loop_counter] < totalNumOfBlocks) {
                    colorAddressBitmap[addr[loop_counter]] = TEXT_BLOCK;
                }
            }

        } else if (next_level == 1) {
            //mark every block that this block pointss to as FIRST_LEVEL_INDIRECT_BLOCK
            for (loop_counter = 0; loop_counter < numberOfArrayEntries; loop_counter++) {
                if (addr[loop_counter] < totalNumOfBlocks) {
                    colorAddressBitmap[addr[loop_counter]] = FIRST_LEVEL_INDIRECT_BLOCK;
                }
            }

            // now mark this block as a second level indirect block
            //printf("SECOND_LEVEL_INDIRECT_BLOCK %d\n", blockNum );
            colorAddressBitmap[blockNum] = SECOND_LEVEL_INDIRECT_BLOCK;
        } else if (next_level == 2) {
            //mark every block that this block pointss to as FIRST_LEVEL_INDIRECT_BLOCK
            for (loop_counter = 0; loop_counter < numberOfArrayEntries; loop_counter++) {
                if (addr[loop_counter] < totalNumOfBlocks) {
                    colorAddressBitmap[addr[loop_counter]] = SECOND_LEVEL_INDIRECT_BLOCK;
                }
            }

            // now mark this block as a second level indirect block
            //printf("THIRD_LEVEL_INDIRECT_BLOCK %d\n", blockNum );
            colorAddressBitmap[blockNum] = THIRD_LEVEL_INDIRECT_BLOCK;
        } else if (next_level > 3) {
            printf("something isnt quite right! Level got was %d\n", next_level);
        } else {
            printf("this block was marked already so sent -1 back\n so the block number %d is unmarked now :(\n",
                   blockNum);
        }
        //free the resources this resulted in memory leak because I had skipped this step
        free(addr);
        free(buffer);
        //return next_level+1 for the calling function
        return (next_level + 1);
    } else {

        if (blockNum <= totalNumOfBlocks) {
            //Not sure if marking this as NOT_AN_ADDRESS_BLOCK is the right idea so please revisit this logic
            colorAddressBitmap[blockNum] = NOT_AN_ADDRESS_BLOCK;
        }
        free(buffer);
        return 0; // if not an address block mark it and send back 0
    }


    // return level;
}

//new method for finding levels of address blocks
int checkAddrBlock1(int fd, char *colorAddressBitmap, int blockNum, int level,int blocklimit) {
    int totalNumOfBlocks = blocklimit;

    if (blockNum > totalNumOfBlocks) {
        return 0;
    }
    if (colorAddressBitmap[blockNum] != '\0') {
        if (colorAddressBitmap[blockNum] == ADDR_BLOCK) {
            // do nothing because we want to further analyze this
        } else if (colorAddressBitmap[blockNum] == NOT_AN_ADDRESS_BLOCK) {
            printf("Return 0\n");
            return 0;
        } else if (colorAddressBitmap[blockNum] == FIRST_LEVEL_INDIRECT_BLOCK) {
            printf("Return 1\n");
            return 1;
        } else if (colorAddressBitmap[blockNum] == SECOND_LEVEL_INDIRECT_BLOCK) {
            printf("Return 2\n");
            return 2;
        } else if (colorAddressBitmap[blockNum] == THIRD_LEVEL_INDIRECT_BLOCK) {
            printf("Return 3\n");
            return 3;
        }
    }

    printf("\n Level not found, going for level check\n");
    UINT8 offset = (UINT8) blockNum * gBlockSize;
    lseek64(fd, offset, SEEK_SET);
    char *data = (char *) malloc(gBlockSize);
    char *buffer;
    buffer = data;
    memset((void *) data, 0, gBlockSize);
    read(fd, data, gBlockSize);
    int numberOfArrayEntries = gBlockSize / (sizeof(int));
    int addr1, addr2;
    int loop_counter = 0;
    int difference_counter = 0;

    /* read first eight bytes of data into an integer variable */
    memcpy(&addr1, data, sizeof(int));
    data += sizeof(int);


    if (addr1 < totalNumOfBlocks)
    if (colorAddressBitmap[addr1] != (int) ADDR_BLOCK) {

        colorAddressBitmap[blockNum] = (int) FIRST_LEVEL_INDIRECT_BLOCK;
        free(buffer);
        return 1;
    }
    else
    {
       if(checkAddrBlock1(fd, colorAddressBitmap, addr1, 0,blocklimit) == 1)
       {
           colorAddressBitmap[blockNum] = (int) SECOND_LEVEL_INDIRECT_BLOCK;

           int i = 0;
           for (i=0; i < numberOfArrayEntries - 2; i++)
           {
                addr1 = 0;
                memcpy(&addr1, data, sizeof(int));
                data += sizeof(int);
                colorAddressBitmap[addr1] = (int) FIRST_LEVEL_INDIRECT_BLOCK;
           }
           free(buffer);
           return 2;
       } //if block next to current is first level means current is second level
       else
        if(checkAddrBlock1(fd, colorAddressBitmap, addr1, 0,blocklimit) == 2)
       {
            colorAddressBitmap[blockNum] = (int) THIRD_LEVEL_INDIRECT_BLOCK;

           int i = 0;
           for (i=0; i < numberOfArrayEntries - 2; i++)
           {
                addr1 = 0;
                memcpy(&addr1, data, sizeof(int));
                data += sizeof(int);
                colorAddressBitmap[addr1] = (int) SECOND_LEVEL_INDIRECT_BLOCK;
           }
           free(buffer);
           return 3;
       }
    }

}

/* Helper function that takes a list head and adds the addressnode to the end of the list.*/
struct address_node *addAddrBlockToList(struct address_node *nodeHead, int blkNum) {

    struct address_node *temp;
    struct address_node *newNode;
    temp = nodeHead;

    newNode = (struct address_node *) malloc(sizeof(struct address_node));

    newNode->address = blkNum;
    newNode->next = NULL;

    if (nodeHead == NULL) {
        nodeHead = &newNode;
    } else {
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = &newNode;
    }
    return nodeHead;
}

/* depending on the level the addressblock belongs to add it to the appropriate list of global structure Deleted_address_blocks*/
void addAddressBlock(int blkNum, int level) {
    if (level == 1) {
        gDeletedAddressBlocks.firstLevelHead = addAddrBlockToList(gDeletedAddressBlocks.firstLevelHead, blkNum);
    } else if (level == 2) {
        gDeletedAddressBlocks.secondLevelHead = addAddrBlockToList(gDeletedAddressBlocks.secondLevelHead, blkNum);
    } else if (level == 3) {
        gDeletedAddressBlocks.thirdLevelHead = addAddrBlockToList(gDeletedAddressBlocks.thirdLevelHead, blkNum);
    }
}


/* iterate through all the blocks of the blockGroupNo
 * update the bitmap to indicate the text blocks among
 * unused blocks*/
int bgdUpdateTextBlocks(int fd, int blockGroupNo, char *colorCodeBitmap, struct ext3_deleted_dir_entry *delDir) {
    int maxBlocks = gBlockSize * BITS_IN_A_BYTE;
    int i;
    // int first = 1;
    int count = 0;
    int dirCount = 0;
    int entryCount = 0;
    char path[gBlockSize];
    memset(&path, 0, sizeof(gBlockSize));

    for (i = 0; i < maxBlocks; ++i) {
        if (!colorCodeBitmap[i]) {
            int blockNum = i + (blockGroupNo * gBlockSize * BITS_IN_A_BYTE);
            long long offset = (long long)blockNum * (long long)gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            char *data = (char *) malloc(gBlockSize);
            memset((void *) data, 0, gBlockSize);
            read(fd, data, gBlockSize);
            int k;
            int found = 0;

            /* read the block data into directory structure */
            struct ext3_dir_entry_2 DirEntry;
            memset(&DirEntry, 0, sizeof(DirEntry));
            memcpy(&DirEntry, data, sizeof(DirEntry));

            /* First entry of any directory block is ".". After reading block data
             * into a directory structure, if it is "." and length is 1 then
             * it can be concluded that the data block is a directory block */
            if ((strcmp(DirEntry.name, ".")) == 0 && (DirEntry.name_len == 1)) {
                found = 1;
                colorCodeBitmap[i] = (int) DIR_BLOCK;
                bgdUpdateDirPath(fd, delDir, blockNum, dirCount, &entryCount);
                dirCount++;
                continue;
            } else {
                for (k = 0; k < gBlockSize; ++k) {
                    /* If all the bytes in the data blocks are in
                     * the ASCII range then it can be concluded that
                     * the data block is a text block */
                    if (data[k] > LAST_ASCII_HEX) {
                        found = 1;
                        break;
                    }
                    /*else if(data[k] < 0x20)
                     {
                     if(data[k] != 0x09 && data[k] != 0x0A)
                     {
                     found = 1;
                     break;
                     }
                     }*/
                }
            }
            if (found == 0) {
                colorCodeBitmap[i] = (int) TEXT_BLOCK;
                count++;
            }
            free(data);
        }
    }
    printf("Total number of text blocks(unreserved) are %d\n", count);
    printf("Total number of directory blocks(unreserved) are %d\n", dirCount);
    return dirCount + entryCount;
}

/* Iterate through different blocks.
 * Update the bitmap to indicate the text blocks among
 * unused blocks
 * Author: Chengkai Huang
 */
void bgdUpdateDirPath(int fd, struct ext3_deleted_dir_entry *delDir, int blockNum, int dirCount, int *entryCount) {
    int maxBlocks = gBlockSize * BITS_IN_A_BYTE;
    int i;
    char path[gBlockSize];
    memset(&path, 0, sizeof(gBlockSize));
    long long offset = (long) blockNum * (long) gBlockSize;

    lseek64(fd, offset, SEEK_SET);
    char *data = (char *) malloc(gBlockSize);
    memset((void *) data, 0, gBlockSize);
    read(fd, data, gBlockSize);
    int size = 0;

    /* read the block data into directory structure */
    struct ext3_dir_entry_2 DirEntry;
    memset(&DirEntry, 0, sizeof(DirEntry));
    memcpy(&DirEntry, data, sizeof(DirEntry));

    unsigned int u4Index;
    unsigned int u4NextPos = 0;
    int8_t *pPos = NULL;
    unsigned int baseInode = DirEntry.inode;

    for (u4Index = 0; u4Index < gBlockSize; u4Index++) {
        if ((strcmp(DirEntry.name, "..")) == 0 && (DirEntry.name_len == 2)) {
            baseInode = DirEntry.inode;
        }

        if(DirEntry.inode >= baseInode) {
            if (u4Index > 0)
                (*entryCount)++;

            int dirTotalCount = dirCount + *entryCount;
            delDir[0].count = dirTotalCount;
            delDir[dirTotalCount].block_num = blockNum;
            delDir[dirTotalCount].inode = DirEntry.inode;
            delDir[dirTotalCount].rec_len = DirEntry.rec_len;
            delDir[dirTotalCount].name_len = DirEntry.name_len;
            delDir[dirTotalCount].file_type = DirEntry.file_type;
            strncpy(delDir[dirTotalCount].name, DirEntry.name, DirEntry.name_len);
        }

        // Calculate actual directory entry length and compare with DirEntry.rec_len later.
        int quotient =  DirEntry.name_len / 4;
        int mult = (DirEntry.name_len % 4 == 0) ?  quotient : quotient + 1;
        int len = 8 + mult * 4;
        // There may have some entries without file name and inode nubmer zeros. Shift to next under this situation.
        // The least entry value is 12.
        if(len < 12) {
            len = 12;
        }

        // dummy entries has zero inode number.
        if((DirEntry.rec_len > len || DirEntry.inode == 0 || DirEntry.inode == 1) && DirEntry.inode < baseInode) {
            data += len;
            u4NextPos += len;
        } else {
            if(DirEntry.inode >= baseInode && DirEntry.rec_len > len) {
                data += len;
                u4NextPos += len;
            } else {
                data += DirEntry.rec_len;
                u4NextPos += DirEntry.rec_len;
            }
        }

        if (u4NextPos >= gBlockSize) {
            break;
        }
        memcpy(&DirEntry, data, sizeof(DirEntry));
    }
}

/* given a drive name and the block group number, this function
 * first reads the block bitmap and copies the corresponding bitmap
 * data and then updates null blocks, text blocks, address blocks,
 * directory blocks among the unused blocks i.e. the one with bitmap
 * entry as zero */
unsigned char *bgdReadBlockBitmap(char driveName[], int blockGroupNo) {
    int fd = open(driveName, O_RDONLY);

    if (fd < 0) {
        fputs("drive read failed", stderr);
        exit(2);
    }

    int totalNumberOfBlocks = bgdGetTotalNumberOfBlocks(fd);
    unsigned long blockBitMapBlockNum = getBlockBitMapAddr(blockGroupNo);
    UINT8 offset = blockBitMapBlockNum * gBlockSize;

    int numberOfBlocksInGroup = bgdGetNumberOfBlocksInGroup(fd);
    unsigned char *colorCodeBitmap = (unsigned char *) malloc(numberOfBlocksInGroup);
    memset((void *) colorCodeBitmap, 0, numberOfBlocksInGroup);

    lseek64(fd, offset, SEEK_SET);
    unsigned char *data = (unsigned char *) malloc(gBlockSize);
    memset((void *) data, 0, gBlockSize);
    read(fd, data, gBlockSize);
    int i, counter;
    counter = 0;
    struct ext3_deleted_dir_entry delDir[gBlockSize];
    memset(&delDir, 0, sizeof(delDir));

    for (i = 0; i < gBlockSize; ++i) {
        int index = i * BITS_IN_A_BYTE;
        if (data[i] & 0x01) {
            counter++;
            colorCodeBitmap[index] = USED_BLOCK;
        }
        if (data[i] & 0x02) {
            counter++;
            colorCodeBitmap[index + 1] = USED_BLOCK;
        }
        if (data[i] & 0x04) {
            counter++;
            colorCodeBitmap[index + 2] = USED_BLOCK;
        }
        if (data[i] & 0x08) {
            counter++;
            colorCodeBitmap[index + 3] = USED_BLOCK;
        }
        if (data[i] & 0x10) {
            counter++;
            colorCodeBitmap[index + 4] = USED_BLOCK;
        }
        if (data[i] & 0x20) {
            counter++;
            colorCodeBitmap[index + 5] = USED_BLOCK;
        }
        if (data[i] & 0x40) {
            counter++;
            colorCodeBitmap[index + 6] = USED_BLOCK;
        }
        if (data[i] & 0x80) {
            counter++;
            colorCodeBitmap[index + 7] = USED_BLOCK;
        }
    }
    free(data);

    int deletedDirCount =  bgdUpdateBlocks(fd, blockGroupNo, colorCodeBitmap,totalNumberOfBlocks,delDir);
    getDeletedPath(delDir);

    if (deletedDirCount > 0) {
        printf("-------------------------------------------------------------------------------\n");
        printf("| BlockNum  |  Inode no.   | rec len |            name         |      path    |\n");
        printf("-------------------------------------------------------------------------------\n");
        int blockNum = 0;
        for (i = 0; i < deletedDirCount; i++) {
            if (blockNum > 0 && delDir[i].block_num != blockNum) {
                printf("-------------------------------------------------------------------------------\n");
            }
            printf("| %9d | %12d | %7d |%25s| %12s |\n", delDir[i].block_num, delDir[i].inode, delDir[i].rec_len,
                   delDir[i].name, delDir[i].path);

            blockNum = delDir[i].block_num;
        }
        printf("-------------------------------------------------------------------------------\n");
    }
    counter = 0;
    int maxBlocks = numberOfBlocksInGroup;
    for (i = 0; i < maxBlocks; ++i) {
        if (!colorCodeBitmap[i])
            counter++;
    }
    printf("Total number of blocks marked as free is %d\n", counter);


    return colorCodeBitmap;
}

/* almost same as above bgdReadBlockBitmap function
 * except for handle for deleted entries. This function updates deleted directory path.
 */
void bgdReadDeletedBlockBitmap(char driveName[], int blockGroupNo, struct ext3_deleted_dir_entry *delDirectory) {
    int fd = open(driveName, O_RDONLY);
    if (fd < 0) {
        fputs("drive read failed", stderr);
        exit(2);
    }
    unsigned long blockBitMapBlockNum = getBlockBitMapAddr(blockGroupNo);
    long long offset = blockBitMapBlockNum * gBlockSize;
    int numberOfBlocksInGroup = bgdGetNumberOfBlocksInGroup(fd);
    unsigned char *colorCodeBitmap = (unsigned char *) malloc(numberOfBlocksInGroup);
    memset((void *) colorCodeBitmap, 0, numberOfBlocksInGroup);

    lseek64(fd, offset, SEEK_SET);
    unsigned char *data = (unsigned char *) malloc(gBlockSize);
    memset((void *) data, 0, gBlockSize);
    read(fd, data, gBlockSize);
    int i, counter;
    counter = 0;
    struct ext3_deleted_dir_entry delDir[gBlockSize];
    memset(&delDir, 0, sizeof(delDir));

    for (i = 0; i < gBlockSize; ++i) {
        int index = i * BITS_IN_A_BYTE;
        if (data[i] & 0x01) {
            counter++;
            colorCodeBitmap[index] = USED_BLOCK;
        }
        if (data[i] & 0x02) {
            counter++;
            colorCodeBitmap[index + 1] = USED_BLOCK;
        }
        if (data[i] & 0x04) {
            counter++;
            colorCodeBitmap[index + 2] = USED_BLOCK;
        }
        if (data[i] & 0x08) {
            counter++;
            colorCodeBitmap[index + 3] = USED_BLOCK;
        }
        if (data[i] & 0x10) {
            counter++;
            colorCodeBitmap[index + 4] = USED_BLOCK;
        }
        if (data[i] & 0x20) {
            counter++;
            colorCodeBitmap[index + 5] = USED_BLOCK;
        }
        if (data[i] & 0x40) {
            counter++;
            colorCodeBitmap[index + 6] = USED_BLOCK;
        }
        if (data[i] & 0x80) {
            counter++;
            colorCodeBitmap[index + 7] = USED_BLOCK;
        }
    }
    free(data);
    bgdUpdateNullBlocks(fd, blockGroupNo, colorCodeBitmap);
    bgdUpdateAddrBlocks(fd, blockGroupNo, colorCodeBitmap);
    int deletedDirCount = bgdUpdateTextBlocks(fd, blockGroupNo, colorCodeBitmap, delDir);
    int totalCount = 0;
    int existCount = delDirectory[0].count;
    int bg_iterator = 0;

    if (deletedDirCount > 0) {
        getDeletedPath(delDir);
        totalCount = existCount + deletedDirCount;
        delDirectory[0].count = totalCount;
    }

    for (bg_iterator = 0; bg_iterator < totalCount; bg_iterator++) {
        int cur = bg_iterator + existCount;
        memcpy((void *) &delDirectory[bg_iterator + existCount], (void *) &delDir[bg_iterator],
               sizeof(struct ext3_deleted_dir_entry));
    }

    if (deletedDirCount > 0) {
        printf("has %d deleted \n", deletedDirCount);
    }
}

/*
 * Main entrance for read deleted files. Allocate
 *
 * Author: Chengkai Huang
 */
void bgdReadDeletedDir(char driveName[], int blockGroupNo, int fileWrite) {
    // s_block_per_group: 32768. Use 10000 as reserved entries for 16GB device total size are: 638,880,000.
    // Can also reserved 20000 entries which maxAllocSize will be 2420000 for 16GB device
    // and total reserved spaces: 1277760000. It is easy to overflow with huge heap size.
    int i;
    int maxCount;
    FILE *output_file;
    long long totalAllocSize = NUM_RESERVED_ENTRIES * gBlockGroupCount;
    long long allocSize = totalAllocSize * (long long)sizeof(struct ext3_deleted_dir_entry);
    if(allocSize > MEM_MAX_VALUE) {
        allocSize = MEM_MAX_VALUE;
    }

    struct ext3_deleted_dir_entry *delDirectory = (struct ext3_deleted_dir_entry *) malloc(allocSize);
    if(!delDirectory) { //malloc return NULL
        printf("Error!! heap memory not big enough allocSize:%lld\n", allocSize);
        return;
    }
    delDirectory[0].count = 0;
    int bg_iterator = 0;
    if (blockGroupNo >= 0) {
            bgdReadDeletedBlockBitmap(driveName, blockGroupNo, delDirectory);
    } else {
        while (bg_iterator < gBlockGroupCount) //iterator through block group.
        {
            printf("scan block:%d\n",bg_iterator);
            /* read each group descriptor and keep deleted entries in delDirectory. */
            bgdReadDeletedBlockBitmap(driveName, bg_iterator, delDirectory);
            bg_iterator++;
        }
    }
    deletedDiretory = delDirectory;

    maxCount = deletedDiretory[0].count;

    if (fileWrite) {
        char fileName[MAX_FILENAME_LENGTH];
        sprintf(fileName, "deletedDirectory.csv");
        output_file = fopen(fileName, "w");
        // fprintf(output_file, "-------------------------------------------------------------------------------\n");
        // fprintf(output_file, "Block group num,Inode no.,rec len,name,path\n");
        // fprintf(output_file, "-------------------------------------------------------------------------------\n");

        int blockNum = 0;
        for (i = 0; i < maxCount; ++i) {
            // if (blockNum > 0 && deletedDiretory[i].block_num != blockNum) {
                // fprintf(output_file,
                //         "-------------------------------------------------------------------------------\n");
            // }
            fprintf(output_file, "%d,%d,%d,%s,%s\n", deletedDiretory[i].block_num,
                    deletedDiretory[i].inode, deletedDiretory[i].rec_len, deletedDiretory[i].name,
                    deletedDiretory[i].path);

            blockNum = deletedDiretory[i].block_num;
        }
        // fprintf(output_file, "-------------------------------------------------------------------------------\n");

        close(output_file);
    }

    //print deleted entries
    for (i = 0; i < maxCount; i++) {

        printf("---------------------------------------------------------------------------------------\n");
        printf("| BlockNum  |  Inode no.   | rec len |            name         |          path        |\n");
        printf("---------------------------------------------------------------------------------------\n");
        int blockNum = 0;
        for (i = 0; i < maxCount; i++) {
            if (blockNum > 0 && deletedDiretory[i].block_num != blockNum) {
                printf("---------------------------------------------------------------------------------------\n");
            }
            printf("| %9d | %12d | %7d |%25s| %20s |\n", deletedDiretory[i].block_num, deletedDiretory[i].inode,
                   deletedDiretory[i].rec_len, deletedDiretory[i].name, deletedDiretory[i].path);

            blockNum = deletedDiretory[i].block_num;
        }
        printf("---------------------------------------------------------------------------------------\n");
    }
}

/*
 * update first level path name from inode csv file. Then update subfolder path name accordingly.
 *
 * Author: Chengkai Huang
 */
void getDeletedPath(struct ext3_deleted_dir_entry *delDir) {
    int count = delDir[0].count + 1;
    int dotIdx[count];
    int dotCount = 0;
    int i, j;
    struct htDeletedDirEntry *htDeletedDir;
    struct htDeletedDirEntry *htTemp;

    htDeletedDir = NULL;

    readDeletedFilesCSVIntoHT(&htDeletedDir);

    for (i = 0; i < count; ++i) {
        if ((strcmp(delDir[i].name, ".")) == 0 && (delDir[i].name_len == 1)) {
            dotIdx[dotCount++] = i;
        }
    }

    /* update root directory path */
    for (i = 0; i < count; i++) {
        for (htTemp = htDeletedDir; htTemp != NULL; htTemp = htTemp->hh.next) {
            if (strcmp(delDir[i].name, ".")==0 && delDir[i].name_len==1 && delDir[i].inode == htTemp->u4InodeNo) {
                char path[gBlockSize];
                char tmp[gBlockSize];
                memset(&path, 0, sizeof(path));
                memset(&tmp, 0, sizeof(tmp));

                strncpy(path, htTemp->path, strlen(htTemp->path));
                strncpy(tmp, delDir[i].path, strlen(delDir[i].path));
                strcat(path, tmp);
                strncpy(delDir[i].path, path, strlen(path));
                break;  //if found this folder name than next
            }
        }
    }

    //search parent folder name for every directory folder created before children folder,
    //start search from first entry.
    for (i = 0; i < dotCount; i++) {
        for (j = 0; j < count; j++) {
            if (delDir[j].inode == delDir[dotIdx[i]].inode) {
                char path[gBlockSize];
                memset(&path, 0, sizeof(path));
                int pathIdx = getBlockPathIdx(delDir, j);

                if (strlen(delDir[pathIdx].path) > 0) {
                    strncpy(path, delDir[pathIdx].path, strlen(delDir[pathIdx].path));
                }

                if(strcmp(delDir[j].name, ".") != 0) {
                    strcat(path, "/");
                    strcat(path, delDir[j].name);
                }

                strncpy(delDir[dotIdx[i]].path, path, strlen(path));
                break;  //if found this folder name than next
            }
        }
    }
}

/*
 * Get the index of .
 */
int getBlockPathIdx(struct ext3_deleted_dir_entry *delDir, int idx) {
    int i = 0;

    for (i = idx; i >= 0; i--) {
        if ((strcmp(delDir[i].name, ".")) == 0 && (delDir[i].name_len == 1) && (strlen(delDir[i].path) > 0)) {
            return i;
        }
    }
    return 0;
}

/* given a block group number, this function returns the
 * block bitmap address by looking at the group descriptor
 * table which was built at the start */
unsigned long getBlockBitMapAddr(int blockGroupNo) {
    return gGrpDescTable[blockGroupNo].bg_block_bitmap;
}

/* given a block group number and drive name, reads group descriptor table and output
 * its contents to an output file named output.txt and hexdump.txt. Init function uses
 * this module with block group number as zero to store the group descriptor table into
 * the global variable gGrpDescTable for future usage. */
struct ext3_group_desc *bgdGetGrpDescTable(char driveName[], int blockGroupNo, int fileWrite) {
    long long offset = 0;
    FILE *output_file;
    FILE *hex_dump;
    int index;
    char *buff = (char *) malloc(sizeof(struct ext3_group_desc));
    struct ext3_group_desc *gdesc = (struct ext3_group_desc *) malloc(sizeof(struct ext3_group_desc));

    /* structure to store all block group descriptors */
    struct ext3_group_desc *gDescTable = (struct ext3_group_desc *) malloc(
            gBlockGroupCount * sizeof(struct ext3_group_desc));

    /* The different fields in the block group descriptor */
    unsigned char block_group_descriptor[8][30] = {"Blocks bitmap block", "Inodes bitmap block", "Inode table block",
                                                   "Free blocks count", "Free inodes count",
                                                   "Used dirs count"/*,"bg_pad","bg_reserved"*/};
    int fd = open(driveName, O_RDONLY);
    if (fd < 0) {
        fputs("memory error", stderr);
        exit(2);
    }

    /* block group desc table size is one block group desc size(32) * total number of block groups */
    int blockGroupTblSize = gBlockGroupCount * BLOCK_GROUP_DESC_SIZE;
    unsigned char buffer[blockGroupTblSize];
    offset = bgdGetGroupDescStartOffset(blockGroupNo);

    /* Go to the first block group descriptor in the second block */
    lseek64(fd, offset, SEEK_SET);

    /* create output file if required */
    if (fileWrite) {
        char fileName[MAX_FILENAME_LENGTH];
        sprintf(fileName, "output%d.txt", blockGroupNo);
        output_file = fopen(fileName, "write");
        //open the file for hex dump
        memset(fileName, 0, MAX_FILENAME_LENGTH);
        sprintf(fileName, "hexdump%d.txt", blockGroupNo);
        hex_dump = fopen(fileName, "write");
    }

    /* read block group desc table into buffer which will be used for hex dump */
    int retVal = read(fd, buffer, blockGroupTblSize);
    if (retVal <= 0) {
        fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
        return;
    }

    /* hex dump i.e 16 bytes in each line */
    if (fileWrite) {
        int byteCount = 0;
        for (index = 0; index < blockGroupTblSize; index++) {
            fprintf(hex_dump, "%02x ", buffer[index]);
            byteCount++;
            if (byteCount == HEX_DUMP_LINE_SIZE) {
                fprintf(hex_dump, "\n");
                byteCount = 0;
            }
        }
    }

    if (fileWrite) {
        /* print the group descriptor field descriptions in the output file */
        for (index = 0; index < 8; ++index) {
            fprintf(output_file, "%s    ", block_group_descriptor[index]);
        }

        fprintf(output_file, "\n");
    }
    lseek64(fd, offset, SEEK_SET);
    //iterate through all the group descriptors in the group descriptor table
    int bg_iterator = 0;
    while (bg_iterator < gBlockGroupCount) {
        /* read each group descriptor and write it to output file. */
        read(fd, buff, sizeof(struct ext3_group_desc));
        memcpy((void *) &gDescTable[bg_iterator], (void *) buff, sizeof(struct ext3_group_desc));
        if (fileWrite) {
            memcpy((void *) gdesc, (void *) buff, sizeof(struct ext3_group_desc));
            fprintf(output_file, "%15ld|", gdesc->bg_block_bitmap);
            fprintf(output_file, "%18ld|", gdesc->bg_inode_bitmap);
            fprintf(output_file, "%20ld|", gdesc->bg_inode_table);
            fprintf(output_file, "%22d|", gdesc->bg_free_blocks_count);
            fprintf(output_file, "%18d|", gdesc->bg_free_inodes_count);
            fprintf(output_file, "%16d\n", gdesc->bg_used_dirs_count);
        }
        bg_iterator++;
    }
    if (fileWrite) {
        close(output_file);
        close(hex_dump);
    }
    free(gdesc);
    free(buff);
    return gDescTable;
}

/* Reads a single indirect block located at blockNumber and
 * read each 4 bytes into an integer variable which is an address
 * pointing to a data block and print the content of data block to
 * the file fDataOutput.Also, copy the content of single indirect
 * block of addresses to the file fBlockAddr */
void bgdReadSingleIndirectBlocks(int blockNumber, FILE *fDataOutput, int fp, int fileSizeInBlocks, FILE *fBlockAddr,
                                 FILE *fBin) {
    char *data = (char *) malloc(gBlockSize);
    long long offset = blockNumber * gBlockSize;
    printf("Block addresses block number is %d and the offset is %lld\n", blockNumber, offset);
    lseek64(fp, offset, SEEK_SET);
    memset((void *) data, 0, gBlockSize);
    read(fp, data, gBlockSize);
    printf("%s\n", data);
    int addr;
    int counter = 1;
    int maxCount = fileSizeInBlocks - DIRECT_BLOCKS_COUNT;
    if (maxCount > SINGLE_INDIRECT_BLOCKS_COUNT)
        maxCount = SINGLE_INDIRECT_BLOCKS_COUNT;

    char *content = (char *) malloc(gBlockSize);
    while (counter <= maxCount) {
        memcpy(&addr, (void *) data, sizeof(int));
        if (addr > 0) {
            memset((void *) content, 0, gBlockSize);
            long long byteOffset = addr * gBlockSize;
            lseek64(fp, byteOffset, SEEK_SET);
            read(fp, content, gBlockSize);
            //printf("%s", content);
            fprintf(fDataOutput, "%s", content);
            fwrite(&content, sizeof(content), 1, fBin);
            fprintf(fBlockAddr, "%7d ", addr);
            if (counter % 16 == 0)
                fprintf(fBlockAddr, "\n");
        } else {
            break;
        }
        data += sizeof(int);
        counter++;
    }
    printf(".......Finished Indirect Block.........and the counter is %d\n", counter);
    fprintf(fBlockAddr, "\n\n");
    //free(data);
    //free(content);
}

/* Reads the double indirect block located at blockNumber and
 * read 4 bytes into an integer variable and call bgdReadSingleIndirectBlocks
 * by passing the integer variable as a blockNumber till the end of double
 * indirect block */
void bgdReadDoubleIndirectBlocks(int blockNumber, FILE *fDataOutput, int fp, int fileSizeInBlocks, FILE *fBlockAddr,
                                 FILE *fBin) {
    char *data = (char *) malloc(gBlockSize);
    long long offset = blockNumber * gBlockSize;
    lseek64(fp, offset, SEEK_SET);
    memset((void *) data, 0, gBlockSize);
    read(fp, data, gBlockSize);
    int addr;
    int counter = 0;
    int maxCount = fileSizeInBlocks - DIRECT_BLOCKS_COUNT - SINGLE_INDIRECT_BLOCKS_COUNT;
    if (maxCount > DOUBLE_INDIRECT_BLOCKS_COUNT)
        maxCount = DOUBLE_INDIRECT_BLOCKS_COUNT;

    printf("... Double Indirect Block and max count is %d\n", maxCount);

    while (counter < maxCount) {
        memcpy(&addr, (void *) data, sizeof(int));
        if (addr > 0) {
            bgdReadSingleIndirectBlocks(addr, fDataOutput, fp, fileSizeInBlocks, fBlockAddr, fBin);
            printf("Finished single indirect block %d in double indirect block\n", counter + 1);
        } else
            break;
        data += sizeof(int);
        counter++;
    }
    //free(data);
}

/* Reads a triple indirect block located at blockNumber and call
 * bgdReadDoubleIndirectBlocks by reading each four bytes into an
 * integer variable */
void bgdReadTripleIndirectBlocks(int blockNumber, FILE *fDataOutput, int fp, int fileSizeInBlocks, FILE *fBlockAddr,
                                 FILE *fBin) {
    char *data = (char *) malloc(gBlockSize);
    long long offset = blockNumber * gBlockSize;
    lseek64(fp, offset, SEEK_SET);
    memset((void *) data, 0, gBlockSize);
    read(fp, data, gBlockSize);
    int addr;
    int counter = 0;
    int maxCount = fileSizeInBlocks - DIRECT_BLOCKS_COUNT - SINGLE_INDIRECT_BLOCKS_COUNT - DOUBLE_INDIRECT_BLOCKS_COUNT;
    /*if(maxCount > TRIPLE_INDIRECT_BLOCKS_COUNT)
     maxCount = TRIPLE_INDIRECT_BLOCKS_COUNT;*/

    printf("... Triple Indirect Block and max count is %d\n", maxCount);

    while (counter < maxCount) {
        memcpy(&addr, (void *) data, sizeof(int));
        if (addr > 0) {
            bgdReadDoubleIndirectBlocks(addr, fDataOutput, fp, fileSizeInBlocks, fBlockAddr, fBin);
            printf("Finished double indirect block %d in triple indirect block\n", counter + 1);
        } else
            break;
        data += sizeof(int);
        counter++;
    }
    //free(data);
}

/* Reads all the data blocks for the given inode number
 * by reading i_block entry of the inode table which
 * contains 12 direct block addresses, 1 single indirect block
 * address, 1 double indirect address and 1 triple indirect address.*/
void bgdReadFromInode(int inode, char dName[]) {
    char *buffer = (char *) malloc(DEFAULT_EXT3_INODE_SIZE);
    struct ext3_inode *inode_tab = (struct ext3_inode *) malloc(DEFAULT_EXT3_INODE_SIZE);
    int blockGroupNo, inodeTableIndex;
    // int inodePerBlock = 8192;

    /* get the block group number where the file resides */
    blockGroupNo = inode / gBlockSize; //inodePerBlock

    /* get the index of the inode table */
    inodeTableIndex = (inode % gBlockSize) - 1;  //inodePerBlock

    /* read inode table starting block number from group descriptor table */
    long inodeTableBlockNum = gGrpDescTable[blockGroupNo].bg_inode_table;
    printf("blockGroupNo:%d inodeTableIndex:%d\n", blockGroupNo, inodeTableIndex);
    /* calculate the offset in bytes for the file seek */
    long long offset = (inodeTableBlockNum * gBlockSize) + (inodeTableIndex * DEFAULT_EXT3_INODE_SIZE);
    // long long offset = (inodeTableBlockNum * inodePerBlock) + (inodeTableIndex * DEFAULT_EXT3_INODE_SIZE);
    int fd = open(dName, O_RDONLY);
    lseek64(fd, offset, SEEK_SET);
    read(fd, buffer, DEFAULT_EXT3_INODE_SIZE);
    memcpy((void *) inode_tab, (void *) buffer, DEFAULT_EXT3_INODE_SIZE);
    int fileSizeInBlocks;
    printf("File size is %d\n", inode_tab->i_size);
    // if(inode_tab->i_size % inodePerBlock == 0)
    if (inode_tab->i_size % gBlockSize == 0)
        fileSizeInBlocks = inode_tab->i_size / gBlockSize;
    else
        fileSizeInBlocks = (inode_tab->i_size / gBlockSize) + 1;

    char *data = (char *) malloc(gBlockSize);
    int index = 0;
    int fp = open(dName, O_RDONLY);
    int blockNumber;

    char fileName[MAX_FILENAME_LENGTH];
    sprintf(fileName, "inode%d.txt", inode);
    FILE *file_data = fopen(fileName, "write");
    memset((void *) fileName, 0, MAX_FILENAME_LENGTH);
    sprintf(fileName, "baddr_inode%d.txt", inode);
    FILE *block_addr = fopen(fileName, "write");

    memset((void *) fileName, 0, MAX_FILENAME_LENGTH);
    sprintf(fileName, "inode%d.bin", inode);
    FILE *file_bin = fopen(fileName, "ab");

    printf("File Size in Blocks is %d\n", fileSizeInBlocks);
    lseek(fp, 0, SEEK_SET);

    /* read the direct blocks */
    if (fileSizeInBlocks <= DIRECT_BLOCKS_COUNT ||
        fileSizeInBlocks > DIRECT_BLOCKS_COUNT) {
        int maxCount = fileSizeInBlocks;
        if (maxCount > DIRECT_BLOCKS_COUNT)
            maxCount = DIRECT_BLOCKS_COUNT;

        fprintf(block_addr, "The direct block addresses are:\n");

        while (index < maxCount) {
            offset = 0;
            memset((void *) data, 0, gBlockSize);
            blockNumber = 0;
            blockNumber = inode_tab->i_block[index++];
            offset = blockNumber * gBlockSize;
            printf("Block number %d ..... and Offset %lld\n", blockNumber, offset);
            lseek64(fp, offset, SEEK_SET);
            read(fp, data, gBlockSize);
            //printf("%s", data);
            fwrite(&data, sizeof(data), 1, file_bin);
            fprintf(file_data, "%s", data);
            fprintf(block_addr, "%d ", blockNumber);
        }
        fprintf(block_addr, "\n\n");
    }

    /* read single indirect block */
    if (fileSizeInBlocks > DIRECT_BLOCKS_COUNT &&
        (fileSizeInBlocks <= SINGLE_INDIRECT_BLOCKS_COUNT ||
         fileSizeInBlocks > SINGLE_INDIRECT_BLOCKS_COUNT)) {
        blockNumber = inode_tab->i_block[index++];
        fprintf(block_addr, "The Single Indirect block addresses are:\n");
        bgdReadSingleIndirectBlocks(blockNumber, file_data, fp, fileSizeInBlocks, block_addr, file_bin);
        fprintf(block_addr, "\n\n");
    }

    /* read double indirect block */
    if (fileSizeInBlocks > SINGLE_INDIRECT_BLOCKS_COUNT &&
        (fileSizeInBlocks <= DOUBLE_INDIRECT_BLOCKS_COUNT ||
         fileSizeInBlocks > DOUBLE_INDIRECT_BLOCKS_COUNT)) {
        blockNumber = inode_tab->i_block[index++];
        fprintf(block_addr, "The Double Indirect block addresses are:\n");
        bgdReadDoubleIndirectBlocks(blockNumber, file_data, fp, fileSizeInBlocks, block_addr, file_bin);
        fprintf(block_addr, "\n\n");
    }

    /* read triple indirect block */
    if (fileSizeInBlocks > DOUBLE_INDIRECT_BLOCKS_COUNT) {
        blockNumber = inode_tab->i_block[index++];
        fprintf(block_addr, "The Triple Indirect block addresses are:\n");
        bgdReadTripleIndirectBlocks(blockNumber, file_data, fp, fileSizeInBlocks, block_addr, file_bin);
    }
    close(file_data);
    close(block_addr);
    close(file_bin);
    free(buffer);
    free(inode_tab);
    //free(data);
}

/* This function returns the block group descriptor of the
 * given blockGroupNum by reading global block group descriptor
 * table with | as the delimiter. */
char *bgdGetBlockGroupInfo(int blockGroupNum) {
    //    if(!bgdIsPowerOf3_5_7(blockGroupNum))
    //    {
    //        return "";
    //    }
    char *bgdInfo = (char *) malloc(BLOCK_GROUP_INFO_MAX_SIZE);
    memset((void *) bgdInfo, 0, BLOCK_GROUP_INFO_MAX_SIZE);
    char target[BLOCK_GROUP_FIELD_MAX_SIZE] = {0};
    sprintf(target, "%ld", gGrpDescTable[blockGroupNum].bg_block_bitmap);
    strcpy(bgdInfo, target);
    strcat(bgdInfo, " ");
    memset((void *) target, 0, BLOCK_GROUP_FIELD_MAX_SIZE);
    sprintf(target, "%ld", gGrpDescTable[blockGroupNum].bg_inode_bitmap);
    strcat(bgdInfo, target);
    strcat(bgdInfo, " ");
    memset((void *) target, 0, BLOCK_GROUP_FIELD_MAX_SIZE);
    sprintf(target, "%ld", gGrpDescTable[blockGroupNum].bg_inode_table);
    strcat(bgdInfo, target);
    strcat(bgdInfo, " ");
    memset((void *) target, 0, BLOCK_GROUP_FIELD_MAX_SIZE);
    sprintf(target, "%d", gGrpDescTable[blockGroupNum].bg_free_inodes_count);
    strcat(bgdInfo, target);
    strcat(bgdInfo, " ");
    memset((void *) target, 0, BLOCK_GROUP_FIELD_MAX_SIZE);
    sprintf(target, "%d", gGrpDescTable[blockGroupNum].bg_free_blocks_count);
    strcat(bgdInfo, target);
    strcat(bgdInfo, " ");
    memset((void *) target, 0, BLOCK_GROUP_FIELD_MAX_SIZE);
    sprintf(target, "%d", gGrpDescTable[blockGroupNum].bg_used_dirs_count);
    strcat(bgdInfo, target);
    return bgdInfo;
}

void readDeletedFilesCSVIntoHT(struct htDeletedDirEntry **pHTDeletedDirEntry) {
    FILE *deletdFilesCSV;
    char *line = NULL;
    size_t len = 0;
    char *token;
    char *delim = ",";
    struct htDeletedDirEntry *htOneEntry;
    char *entryArray[5];

    deletdFilesCSV = fopen(DELETED_FILES_CSV_PATH, "r");
    if (deletdFilesCSV == NULL) {
        printf("there is no such file exist, please run list in inode module to generate file");
        return;
    }
    while (getline(&line, &len, deletdFilesCSV) != -1) {
        int count = 0;

        token = strtok(line, delim);
        htOneEntry = malloc(sizeof(struct htDeletedDirEntry));
        memset(entryArray, sizeof(entryArray), 0);
        while (token != NULL) {
            switch (count) {
                case 0:
                    entryArray[0] = token;
                    break;
                case 1:
                    entryArray[1] = token;
                    break;
                case 2:
                    entryArray[2] = token;
                    break;
                case 3:
                    entryArray[3] = token;
                    break;
                case 4:
                    entryArray[4] = token;
                    break;
                default:
                    printf("error");
                    break;
            }
            token = strtok(NULL, delim);
            count++;
        }
        if (atoi(entryArray[4]) == DIRECTORY_TYPE) {
            htOneEntry->u4InodeNo = (UINT4) atoi(entryArray[1]);
            char tmp[gBlockSize];
            strcpy(tmp, entryArray[3]);
            strcat(tmp, entryArray[2]);
            strcpy(htOneEntry->path, tmp);
            HASH_ADD_INT(*pHTDeletedDirEntry, u4InodeNo, htOneEntry);
        }
    }
    free(line);
    fclose(deletdFilesCSV);
}


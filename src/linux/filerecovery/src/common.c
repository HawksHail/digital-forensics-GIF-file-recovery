/*
 * This file utility is used to recover the file types based on the file starting marker and the ending marker
 * and a generic algorithm to find the first, second indirect blocks and direct data blocks.
 */
#include <sys/stat.h>
#include <errno.h>
#include <linux/hdreg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "Group_Desc.c"
#include "construct.c"
#include <inttypes.h>


typedef struct node {
    long address;
    struct node *next;
} node;

typedef struct file_recovery {
    long long directBlockOffset[12];
    long long firstIndirectBlockOffset;
    long long secondIndirectBlockOffset;
    long long thirdIndirectBlockOffset;
} file_recovery;

typedef struct fileNode {
    file_recovery *file;
    struct fileNode *next;
} fileNode;

long long Inode_count = 488640; //super block
long long Inodes_per_group = 16; // super block 8144
long long Inode_blocks_per_group = 509; // super block
long long Inode_size = 256; // super block
long long Inode_table_location = 1024 + (481 * 4096); //Can be find from group descriptor
long long First_data_block_location = 1024 + (400 * 4096); //Can be find from group descriptor
int Block_size = 4096; // super block
long long Data_blocks_per_group = 18000; // super block
char *EOI = "FFD9"; //Change this value according to the file type
char *Disk = "/dev/sdb";
int gBlocksPerGroup;
int gTotalNumberOfBlocks;
int gBlockSize;
char *gBlockBitmap;

char *decimalToHexStringInReverseOrder(int decimalNumber);

int compareHexValues(unsigned char *string1, unsigned char *string2, int n);

void findInodeLocation(long long blockAddress);

long long findIndirectAddressLocation(long long blockAddress);

void constructFile(file_recovery *file, long long startingBlockNumber);

long long hex2intInReverseOrder(char *a, int len);

char *MyStrStr(const char *str, const char *target);

struct node *indirectAddressesList = NULL;


/*
 * This function will scan the file descriptor for different
 * file type by theirs specific signature in the file header
 */
int findHeader(int fd, char fileType[])
{
    unsigned char header[2];
    if(strcmp(fileType, "jpg") == 0)
    {
        printf("Searching for jpg file\n");
        header[0] = 0xff;
        header[1] = 0xd8;
    }
    else if(strcmp(fileType, "png") == 0)
    {
        printf("Searching for png file\n");
        header[0] = 0x89;
        header[1] = 0x50;
    }
    else if(strcmp(fileType, "bmp") == 0)
    {
        printf("Searching for bmp file\n");
        header[0] = 0x42;
        header[1] = 0x4d;
    }
    else if(strcmp(fileType, "mp3") == 0)
    {
        printf("Searching for mp3 file\n");
        header[0] = 0x49;
        header[1] = 0x44;
    }
    else
    {
        printf("Unsupported file type\n");
        return 1;
    }

    unsigned char b[4];
    int headerLength = 2;
    lseek64(fd, 0, SEEK_SET);
    read(fd, b, headerLength);

    for (int i = 0; i < gTotalNumberOfBlocks; i++)
    {
        if(compareHexValues(b, header, headerLength) == 1)
        {
            printf("File header found at block %d\n", i);
//            for (int i=0; i<4; i++)
//            {
//                printf("%x\n", b[i]);
//            }
        }
        lseek64(fd, Block_size - headerLength, SEEK_CUR);
        read(fd, b, headerLength);
    }

    return 0;
}

/*
 * This function will scan the file descriptor for single
 * indirect pointer block
 */
int findIndirectPointerBlock(int fd, int firstDatablock)
{
    int addrLength = 16;
    unsigned char *addr = malloc(sizeof(char) * addrLength); //store 4 adjacent addresses of pointers
    int indBlock = firstDatablock+12;
    addr = decimalToHexStringInReverseOrder(indBlock);
    for (int i=0; i<3; i++)
    {
        addr[4*(i+1)] = addr[4*i]+1;
        addr[4*(i+1)+1] = addr[1];
        addr[4*(i+1)+2] = addr[2];
        addr[4*(i+1)+3] = addr[3];
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
    printf("Searching for indirect pointer block\n");
    for (int i = 0; i < Data_blocks_per_group; i++)
    {
        if(compareHexValues(b, addr, addrLength) == 1)
        {
            printf("Indirect pointer block found at block %d, its first entry points to block %d\n", i, indBlock);
            n++;
        }
        lseek64(fd, Block_size - addrLength, SEEK_CUR);
        read(fd, b, addrLength);
    }

    if (n == 0)
    {
        printf("Unable to find indirect pointer block\n");
    }
    free(addr);
    return 0;
}

int main(int argc, char *argv[]) {

    int i = 0, j = 0;
    i = 0;

    if(argc == 2)
      init(argv[1]);

    if (argc < 5) {
        if (argc != 3)
            return -1;
    }



    int option;
    char *dName;
    int fd;

    if (argc != 3) {
        option = atoi(argv[6]);
        dName = argv[3];
        fd = open(argv[3], O_RDONLY);
    } else {
    	//specifically for filerecovery
		//example: ./filerecovery /dev/sdxx 0
        option = atoi(argv[2]);
        dName = argv[1];
        fd = open(argv[1], O_RDONLY);
    }

/////////////////////////////////////////////////////////////
//    find file header
//     ./recovery /dev/sdb1 header mp3
//
//    find indirect pointer block
//     ./recovery /dev/sdb1 ind firstDataBlock
//
//    construct file
//     ./recovery /dev/sdb1 construct indirectPointerblock
/////////////////////////////////////////////////////////////
    char* disk = argv[1];
    char* argv2 = argv[2];
    if(strcmp(argv2, "header") == 0)
    {
        //search for file header
        char *fileType;
        fileType = argv[3];
        findHeader(fd, fileType);
    }
    else if(strcmp(argv2, "ind") == 0)
    {
        //search for indirect pointer block
        int firstDataBlock;
        firstDataBlock = atoi(argv[3]);
        findIndirectPointerBlock(fd,firstDataBlock);
    }
    else if(strcmp(argv2, "construct") == 0)
    {
        int indirectPointerBlock = atoi(argv[3]);
        //construct file
        construct(disk, Block_size, indirectPointerBlock);
    }
    else if (option == 0) {
        listDeletedDirectory(dName);
        return 1;
    } else if (option == 1) {
        // if option is 1 recover ODT files
        recoverOdtFiles(dName, fd);
        return 1;
    } else if (option == 2) {
        // if option is 2 recover PNG files
        recoverPngFiles(dName, fd);
        return 1;
    } else if (option == 3) {
        // if option is 3 recover DOC files
        recoverDocFiles(dName, fd);
        return 1;
    } else {

        int headerLength = strtol(argv[1], NULL, 0);
        int headerOffset = strtol(argv[2], NULL, 0);
        Disk = argv[3];
        unsigned char header[headerLength];
        for (i = 1; i <= headerLength; i++) {
            header[i - 1] = strtol(argv[i + 3], NULL, 16);
        }
        EOI = argv[argc - 1];

        file_recovery *current_file;
        fileNode *recovered_files;
        fileNode *last_recovered_file = NULL;
        int fd = open(Disk, O_RDONLY);
        if (fd < 0) {
            fputs("memory error", stderr);
            exit(2);
        }
        struct node *link;
        indirectAddressesList = (struct node *) malloc(sizeof(struct node));
        indirectAddressesList->address = 400;
        struct node *current = indirectAddressesList;
        for (i = 401; i < 5000; i++) {
            //create a link
            link = (struct node *) malloc(sizeof(struct node));
            link->address = i;
            //point it to old first node
            current->next = link;
            //point first to new first node
            current = link;
        }

        unsigned char b[Block_size];
        long long offset = 1024;
        lseek64(fd, 1024, SEEK_SET);
        read(fd, b, headerLength);
        // TODO: Needs to use list of unallocated blocks instead of fixed number
        for (i = 0; i < Data_blocks_per_group; i++) {
            if (compareHexValues(b, header, headerLength) == 1) {
                current_file = malloc(sizeof(*current_file));
                if (last_recovered_file == NULL) {
                    last_recovered_file = malloc(sizeof(*last_recovered_file));
                    last_recovered_file->file = current_file;
                    recovered_files = last_recovered_file;
                } else {
                    last_recovered_file->next = malloc(sizeof(*last_recovered_file));
                    last_recovered_file->file = current_file;
                    last_recovered_file = last_recovered_file->next;
                }
                fprintf(stdout,
                        "Found file at offset %llu and block address %d\n",
                        offset, i);
                constructFile(current_file, i);
                for (j = 0; j < 12; j++) {
                    printf("Direct Block number %d is %llu\n", j,
                           current_file->directBlockOffset[j]);
                }
                printf("First Indirect Block number is %llu\n",
                       current_file->firstIndirectBlockOffset);
                printf("Double Indirect Block number is %llu\n",
                       current_file->secondIndirectBlockOffset);
                printf("Triple Indirect Block number is %llu\n",
                       current_file->thirdIndirectBlockOffset);
            }
            lseek64(fd, Block_size - headerLength, SEEK_CUR);
            offset += Block_size;
            read(fd, b, headerLength);

        }
        //TODO: recover_all_the_files(recovered_files.files); //Program to save all the data blocks into another disk or same disk
        close(fd);
        return 0;
    }
}

void init(char driveName[]) {
    int loop_counter = 0;
    char *colorAddressBitmap,*colorAddressBitmap1;
    int fd = open(driveName, O_RDONLY);
    if (fd < 0) {
        fputs("memory error here2", stderr);
        exit(2);
    }
    int totalNumberOfBlocks = bgdGetTotalNumberOfBlocks(fd);
    Block_size = bgdGetBlockSize(driveName);
    gBlockSize = bgdGetBlockSize(driveName);
    //call the init function of the gbd source code Group_Desc.c
    bgdInit(driveName);
    //use Group_Desc.c to get the total number of block groups
    int totalNumberOfBlockGroups = bgdGetNumberofBlockGroups(driveName);
    int numberOfBlocksInGroup = bgdGetNumberOfBlocksInGroup(fd);
    // set the global variable too
    gBlocksPerGroup = numberOfBlocksInGroup;
    gTotalNumberOfBlocks = numberOfBlocksInGroup * totalNumberOfBlockGroups;
    //create a bitmap for the whole drive
    colorAddressBitmap = (char *) malloc(numberOfBlocksInGroup * totalNumberOfBlockGroups + 1);
    //set it to 0 to begin with
    memset((void *) colorAddressBitmap, 0, numberOfBlocksInGroup * totalNumberOfBlockGroups);
    // read the block group table using existing code in gbd.c and create initial bitmap
    char *tempBitmapBuilder = colorAddressBitmap;

    int size_of_bitmap_returned = numberOfBlocksInGroup;
    for (loop_counter = 0; loop_counter < totalNumberOfBlockGroups; loop_counter++) {
        printf("Analyzing block group number %d\n", loop_counter);
        char *colorCodeBitmapReturned = bgdReadBlockBitmap(driveName, loop_counter);
        memcpy(tempBitmapBuilder, colorCodeBitmapReturned, size_of_bitmap_returned);
        if (loop_counter != totalNumberOfBlockGroups) {
            tempBitmapBuilder += size_of_bitmap_returned;
        }
        free(colorCodeBitmapReturned);
    }

    colorAddressBitmap1 = (char *) malloc(numberOfBlocksInGroup * totalNumberOfBlockGroups + 1);
    //set it to 0 to begin with
    memset((void *) colorAddressBitmap1, 0, numberOfBlocksInGroup * totalNumberOfBlockGroups);

    int addr_cnt = 0;

    while (loop_counter < totalNumberOfBlocks) {
        //copy bit map to other bit map copy
        colorAddressBitmap1[loop_counter] = colorAddressBitmap[loop_counter];
        loop_counter++;
    }

    int i = 0;
    int data_block_marked = 0;

    for(i=0;i< totalNumberOfBlocks;i++)
    {
        if(colorAddressBitmap1[i] == (int)ADDR_BLOCK)
        {
            addr_cnt++;
            CHAR *pBuffer;
            CHAR Buffer[gBlockSize];
            memset(Buffer, 0, sizeof(Buffer));
            pBuffer = Buffer;

            int numofaddr = gBlockSize / sizeof(int);

            int loop_cnt1 = 0;

            while(loop_cnt1 < (numofaddr-1))
            {
                int addr1 = 0;
                memcpy(&addr1, pBuffer, sizeof(int));
                pBuffer+= sizeof(int);
                if(colorAddressBitmap1[addr1] != (int)ADDR_BLOCK && colorAddressBitmap1[addr1] != (int) USED_BLOCK)
                {
                   colorAddressBitmap1[addr1] = USED_BLOCK;
                   data_block_marked++;
                }

                loop_cnt1++;
            }

        }

    }

    printf("\n%d Data blocks marked as used\n", data_block_marked);
    printf("\nTotal address block count %d\n",addr_cnt);

    loop_counter = 0;
    //calling checkAddrBlock on all blocks that are marked as ADDR_BLOCKS so we can classify them as first level, second level, etc
    //this function is present in Group_Desc.c under the gbd folder
    while (loop_counter < totalNumberOfBlocks) {
        if (colorAddressBitmap1[loop_counter] == ADDR_BLOCK) {
            //printf("\n\n Checking address block %d for level check \n\n",loop_counter);
            checkAddrBlock1(fd, colorAddressBitmap1, loop_counter, 0,totalNumberOfBlocks);
        }
        loop_counter++;
    }

    loop_counter = 1;
    printf("\n\n***********Address block levels******\n\n");
    while(loop_counter < totalNumberOfBlocks)
    {
        if(colorAddressBitmap1[loop_counter] == FIRST_LEVEL_INDIRECT_BLOCK || colorAddressBitmap1[loop_counter] == SECOND_LEVEL_INDIRECT_BLOCK
           || colorAddressBitmap1[i]== THIRD_LEVEL_INDIRECT_BLOCK)
        {
            printf("%d-->%d\n",loop_counter,colorAddressBitmap1[loop_counter]);

        }
        loop_counter++;

    }

    gBlockBitmap = colorAddressBitmap;
}

/*
This utility function is used to construct the file recovery structure (12 direct blocks, 3 indirect blocks).
The input given to the function is the starting block number of the file which is identified by getting the information from the header of the file or some other means.
*/
void constructFile(file_recovery *file, long long startingBlockNumber) {
    char *pch;
    int fd = open(Disk, O_RDONLY);
    int i = 0;
    unsigned char b[Block_size];
    unsigned char dataBlock[4];
    long long IND = 0, DIND = 0, TIND = 0;
    long long lastBlockAddressInIND = 0, lastBlockAddressInDIND = 0;

    for (i = 0; i < 12; i++) {
        file->directBlockOffset[i] = 0;
    }
    file->firstIndirectBlockOffset = 0;
    file->secondIndirectBlockOffset = 0;
    file->thirdIndirectBlockOffset = 0;

    lseek64(fd, (1024 + startingBlockNumber * Block_size), SEEK_SET);
    for (i = 0; i < 12; i++) {
        read(fd, b, Block_size);
        file->directBlockOffset[i] = startingBlockNumber + i;
        pch = strstr(b, EOI);
        if (pch) {
            printf("End of file found");
            return;
        }
    }
    //TODO: Check whether that data block is part of the actual file
    IND = findIndirectAddressLocation(startingBlockNumber + 12);
    if (IND > 0) {
        file->firstIndirectBlockOffset = IND;
        lseek64(fd, (1024 + IND * 4096), SEEK_SET);
        read(fd, b, Block_size);
        strncpy(dataBlock, &b[Block_size - 4], 4);
        if (compareHexValues(dataBlock, "\x0\x0\x0\x0", 4) == 1) {
            printf("Reached the end of the file \n");
            close(fd);
            return;
        }
        lastBlockAddressInIND = hex2intInReverseOrder(dataBlock, 4);
        lseek64(fd, (1024 + lastBlockAddressInIND * 4096), SEEK_SET);
        read(fd, b, Block_size);
        pch = strstr(b, EOI);
        if (pch) {
            printf("End of file found");
            close(fd);
            return;
        }
        //TODO: Check whether that data block is part of the actual file
        DIND = findIndirectAddressLocation(
                findIndirectAddressLocation(lastBlockAddressInIND + 1));
        if (DIND > 0) {
            file->secondIndirectBlockOffset = DIND;
            lseek64(fd, (1024 + DIND * 4096), SEEK_SET);
            read(fd, b, Block_size);
            strncpy(dataBlock, &b[Block_size - 4], 4);
            if (compareHexValues(dataBlock, "\x0\x0\x0\x0", 4) == 1) {
                printf("Reached the end of the file \n");
                close(fd);
                return;
            }
            lastBlockAddressInDIND = hex2intInReverseOrder(dataBlock, 4);
            lseek64(fd, (1024 + lastBlockAddressInDIND * 4096), SEEK_SET);
            read(fd, b, Block_size);
            strncpy(dataBlock, &b[Block_size - 4], 4);
            if (compareHexValues(dataBlock, "\x0\x0\x0\x0", 4) == 1) {
                printf("Reached the end of the file \n");
                close(fd);
                return;
            }
            lastBlockAddressInIND = hex2intInReverseOrder(dataBlock, 4);
            lseek64(fd, (1024 + lastBlockAddressInIND * 4096), SEEK_SET);
            read(fd, b, Block_size);

            pch = strstr(b, EOI);
            if (pch) {
                printf("End of file found");
                close(fd);
                return;
            }

            //TODO: Check whether that data block is part of the actual file
            TIND = findIndirectAddressLocation(findIndirectAddressLocation(
                    findIndirectAddressLocation(lastBlockAddressInIND + 1)));
            if (TIND > 0) {
                file->thirdIndirectBlockOffset = TIND;
            }

        }
    }

    close(fd);
}

/*
This utility function is used to retrieve the inode offset from the block address of a file
*/
void findInodeLocation(long long blockAddress) {
    char *hexAddressOfBlock = decimalToHexStringInReverseOrder(blockAddress);
    int fd = open(Disk, O_RDONLY);
    int i = 0, j = 0;
    unsigned char b[Inode_size];
    unsigned char directBlock0[4];
    long long offset = Inode_table_location;
    if (fd < 0) {
        fputs("memory error", stderr);
        exit(2);
    }
    lseek64(fd, offset, SEEK_SET);
    for (; i < Inodes_per_group; i++) {
        read(fd, b, Inode_size);
        offset += Inode_size;
        strncpy(directBlock0, &b[40], 4);
        if (compareHexValues(directBlock0, hexAddressOfBlock, 4) == 1) {
            printf("The inode is present in the offset %llu \n", offset);
            close(fd);
            return;
        }
    }
    close(fd);
}

/*
This utility function is used to find the indirect address location.
This is identified by looking at the data in all the blocks that the particular block address of the file (in hex) is present in that block or not.
*/
long long findIndirectAddressLocation(long long blockAddress) {
    char *hexAddressOfBlock = decimalToHexStringInReverseOrder(blockAddress);
    int fd = open(Disk, O_RDONLY);
    int i = 0, j = 0;
    unsigned char b[Block_size];
    unsigned char directBlock0[4];
    long long offset = First_data_block_location;
    if (fd < 0) {
        fputs("memory error", stderr);
        exit(2);
    }
    struct node *current = indirectAddressesList;
    struct node *previous = indirectAddressesList;
    long long currentBlock = 0;
    while (current->next != NULL) {
        currentBlock = current->address;
        offset = 1024 + Block_size * (currentBlock);
        lseek64(fd, offset, SEEK_SET);
        read(fd, b, Block_size);
        for (j = 0; j < (Block_size / 4); j++) {
            strncpy(directBlock0, &b[j * 4], 4);
            if (compareHexValues(directBlock0, hexAddressOfBlock, 4) == 1) {
                printf(
                        "The data block %llu is present in the offset 0x%x and block number %llu\n",
                        blockAddress, j * 4, currentBlock);
                close(fd);
                if (current == indirectAddressesList) {
                    indirectAddressesList = indirectAddressesList->next;
                } else {
                    previous->next = current->next;
                }
                free(current);
                return currentBlock;
            }
        }
        current = current->next;
    }
    return 0;
    close(fd);
}

/*
This utility function comapres the hex values of two character arrays upto a certain length (n)
*/
int compareHexValues(unsigned char string1[], unsigned char string2[], int n) {
    int i;
    for (i = 0; i < n; i++) {
        if ((int) string1[i] == (int) string2[i]) {
            if (i == n - 1) {
                return 1;
            }
        } else {
            return 0;
        }
    }
    return 0;
}

/*
This utility function is used to get the hexadecimal address of the block address
*/
char *decimalToHexStringInReverseOrder(int decimalNumber) {
    char *signs = (char *) malloc(sizeof(char) * 4);
    signs[0] = decimalNumber & 0xff;
    signs[1] = (decimalNumber >> 8) & 0xff;
    signs[2] = (decimalNumber >> 16) & 0xff;
    signs[3] = (decimalNumber >> 24) & 0xff;

    return signs;
}

/*
This utility function is used to get the decimal value from a character array (hex values)
*/
long long hex2intInReverseOrder(char *a, int len) {
    int i;
    long long val = 0;

    for (i = len - 1; i >= 0; i--)
        if (a[i] <= 57)
            val += (a[i] - 48) * (1 << (4 * (len - 1 - i)));
        else
            val += (a[i] - 55) * (1 << (4 * (len - 1 - i)));

    return val;
}

/*
This utility function is used to find whether a string str is present in another big string or not (target)
*/
char *MyStrStr(const char *str, const char *target) {
    if (!*target)
        return str;
    char *p1 = (char *) str, *p2 = (char *) target;
    char *p1Adv = (char *) str;
    while (*++p2)
        p1Adv++;
    while (*p1Adv) {
        char *p1Begin = p1;
        p2 = (char *) target;
        while (*p1 && *p2 && *p1 == *p2) {
            p1++;
            p2++;
        }
        if (!*p2)
            return p1Begin;
        p1 = p1Begin + 1;
        p1Adv++;
    }
    return NULL;
}

/* Run gbd funtion.
 * scan all disk blocks and save results to file */
void listDeletedDirectory(char dName[]) {
    bgdInit(dName);
    // scan all data blocks for dName and store deleted directories and files at deletedDiretory(Group_Desc.h)
    bgdReadDeletedDir(dName, -1, 1);
}

/* this function looks for ODT header of 0x50 and 0x4b at the start of non address and non used block */
void recoverOdtFiles(char dName[], int fd) {
    // now call the init function that does the whole categorization of the blocks for every blockgroup and sets up the global bitmap
    init(dName);
    unsigned char buffer[Block_size];
    int loopBlockNum = 0;
    int n = 2;// control the number of char/byte size entries to read into the buffer inside the loop

    for (loopBlockNum = 0; loopBlockNum < gTotalNumberOfBlocks; loopBlockNum++) {
        if (gBlockBitmap[loopBlockNum] == TEXT_BLOCK || gBlockBitmap[loopBlockNum] == USED_BLOCK ||
            gBlockBitmap == UNUSED_BLOCK) {
            //calculate the offset of this blockNumber
            int offset = loopBlockNum * gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            unsigned char header[n];
            int retVal = read(fd, header, n);
            if ((header[0] == 0x50) &&
                (header[1] == 0x4b)) { // Depends on the file type this if condition will be changed
                fprintf(stdout,
                        "Found ODT FILE file at offset %llu and block address %d\n",
                        offset, loopBlockNum);
                findIndirectAddressBlock(fd, loopBlockNum + 12);
            }
        }
    }
}


/* this function looks for PNG header at the start of non address and non used block */
void recoverPngFiles(char dName[], int fd) {
    // now call the init function that does the whole categorization of the blocks for every blockgroup and sets up the global bitmap
    init(dName);
    unsigned char buffer[Block_size];
    int loopBlockNum = 0;
    int n = 8;// control the number of char/byte size entries to read into the buffer inside the loop
    int j = 0;
    for (loopBlockNum = 0; loopBlockNum < gTotalNumberOfBlocks; loopBlockNum++) {
        if (gBlockBitmap[loopBlockNum] == TEXT_BLOCK || gBlockBitmap[loopBlockNum] == USED_BLOCK ||
            gBlockBitmap == UNUSED_BLOCK) {
            //calculate the offset of this blockNumber
            int offset = loopBlockNum * gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            unsigned char header[n];
            int retVal = read(fd, header, n);
            if ((header[0] == 0x89) && (header[1] == 0x50) && (header[2] == 0x4E) && (header[3] == 0x47) &&
                (header[4] == 0x0D) && (header[5] == 0x0A) && (header[6] == 0x1A) &&
                (header[7] == 0x0A)) { // Depends on the file type this if condition will be changed
                fprintf(stdout,
                        "Found PNG FILE file at offset %llu and block address %d\n",
                        offset, loopBlockNum);

                findIndirectAddressBlock(fd, loopBlockNum + 12);
            }

        }
    }

}

// My changes was pushed by Vinay on behalf of me, refer to commit number 538880 for my changes
/* this function looks for DOC header at the start of non address and non used block */
void recoverDocFiles(char dName[], int fd) {
    // now call the init function that does the whole categorization of the blocks for every blockgroup and sets up the global bitmap
    init(dName);
    unsigned char buffer[Block_size];
    int loopBlockNum = 0;
    int n = 8;// control the number of char/byte size entries to read into the buffer inside the loop
    int j = 0;
    for (loopBlockNum = 0; loopBlockNum < gTotalNumberOfBlocks; loopBlockNum++) {
        if (gBlockBitmap[loopBlockNum] == TEXT_BLOCK || gBlockBitmap[loopBlockNum] == USED_BLOCK ||
            gBlockBitmap == UNUSED_BLOCK) {
            //calculate the offset of this blockNumber
            int offset = loopBlockNum * gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            unsigned char header[n];
            int retVal = read(fd, header, n);
            if ((header[0] == 0xD0) && (header[1] == 0xCF) && (header[2] == 0x11) && (header[3] == 0xE0) &&
                (header[4] == 0xA1) && (header[5] == 0xB1) && (header[6] == 0x1A) &&
                (header[7] == 0xE1)) { // Depends on the file type this if condition will be changed
                fprintf(stdout,
                        "Found DOC FILE file at offset %llu and block address %d\n",
                        offset, loopBlockNum);

                findIndirectAddressBlock(fd, loopBlockNum + 12);
            }

        }
    }

}

//given a block number this function returns the blockNum of the indirect address block
int findIndirectAddressBlock(int fd, int blockNum) {
    int i = 0, j = 0;
    unsigned char b[Block_size];
    unsigned char directBlock0[4];
    int loop_counter = 0;

    for (loop_counter = 0; loop_counter < gTotalNumberOfBlocks; loop_counter++) {
        //if ((gBlockBitmap[loop_counter] == FIRST_LEVEL_INDIRECT_BLOCK)||(gBlockBitmap[loop_counter] == ADDR_BLOCK))
        if (1) {
            int *buffer = (int *) malloc(gBlockSize);
            long offset = loop_counter * gBlockSize;
            lseek64(fd, offset, SEEK_SET);
            read(fd, (void *) buffer, gBlockSize);
            int i = 0;
            for (i = 0; i < gBlockSize / (sizeof(int)); i++) {
                if (buffer[i] == blockNum) {
                    printf("found indirect address block numbered %d\n", loop_counter);
                    return loop_counter;
                }
            }
            free(buffer);
        }
    }
    printf("WAS UNABLE TO FIND THE FIRST_LEVEL_INDIRECT_BLOCK\n");
    return -1;
}

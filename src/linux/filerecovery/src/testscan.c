#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "construct.c"

int Block_size = 4096; // from super block
long long Data_blocks_per_group = 32768; // from super block
char *decimalToHexStringInReverseOrder(int decimalNumber);
int compareHexValues(unsigned char *string1, unsigned char *string2, int n);


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

    for (int i = 0; i < Data_blocks_per_group; i++)
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

int main(int argc, char* argv[]) {

//    find file header
//     ./recovery /dev/sdb1 header mp3
//
//    find indirect pointer block
//     ./recovery /dev/sdb1 ind firstDataBlock
//
//    construct file
//     ./recovery /dev/sdb1 construct indirectPointerblock
    char* argv2 = argv[2];
    int option = atoi(argv[2]);
    char *disk = argv[1];

    int fd = open(disk, O_RDONLY);
    if (fd < 0) {
        fputs("Unable to read disk\n", stderr);
        exit(2);
    }

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
    else
    {
        return 1;
    }

    close(fd);
    return 0;
}

/*
This utility function compares the hex values of two character arrays upto a certain length (n)
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

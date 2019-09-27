/* Digital Forensics - Spring 2016 
 * 
 * GBD_Test.c - This is just a test file which contains main function 
 * to test the GDB functionality.
 * @author - Anoop S Somashekar
*/

#include "Group_Desc.h"

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Invalid argument format: Usage %s \\dev\\sdx1 inode_number\n", argv[0]);
        exit(1);
    }
    char *dName = argv[1];
    int inodeNumber = atoi(argv[2]);
    int fd = open(argv[1],O_RDONLY);
    if(fd < 0)
    {
        fputs("memory error",stderr); 
        exit (2);
    }

    bgdInit(dName);

    /*int i, j;
    for(i=0;i<gBlockGroupCount;++i)
    {
        if(!bgdIsPowerOf3_5_7(i))
            continue;
        else
            for(j=i+1;j<gBlockGroupCount;++j)
            {
                if(bgdIsPowerOf3_5_7(j))
                {            
                    char *res = bgdCompareGrpDesc(i, j, dName);
                    if(res[0] == '\0')
                    {
                        printf("GDT at block group %d and block group %d are identical\n", i, j);
                    }
                    else
                    {
                        printf("GDT at block group %d and block group %d are not identical\n", i, j);
                        printf("%s\n", res);
                    }
                }
            }
    }*/
    unsigned char *val;
    int k, counter=0;;
    //inodeNumber here is group number
    // val = bgdReadBlockBitmap(dName, inodeNumber);
    //scan all data blocks for dName and store deleted directories and files at deletedDiretory(Group_Desc.h)
    bgdReadDeletedDir(dName, inodeNumber, 1);
    /*for(k=0;k<gBlockSize * BITS_IN_A_BYTE;++k)
    {
        counter++;
        printf("%u ", val[k]);
        if(counter == 30)
        {
            counter = 0;
            printf("\n");
        }
    }*/
    //inodeNumber here is inodeNumber
    // bgdReadFromInode(inodeNumber, dName);
    printf("%s\n",bgdGetBlockGroupInfo(inodeNumber));
    free(gGrpDescTable);

    bgdInit(dName);
    // scan all data blocks for dName and store deleted directories and files at deletedDiretory(Group_Desc.h)
    return 0;
}

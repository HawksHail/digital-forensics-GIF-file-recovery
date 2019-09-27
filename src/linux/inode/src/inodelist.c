/*******************************************************************
 *
 * File         : inodelist.c
 * Author       : Xinyi Li
 * Date created : Oct 10th, 2016
 * Description  : This file contains functions to operate on the list
 *                all delete files' inode and their path
 *
 *******************************************************************/
#include <wchar.h>
#include <stddef.h>
#include "inodeinc.h"

/*! *************************************************************************
 * Function Name : InodeListStoreRegioDirFiles
 *
 * Author        : Xinyi Li
 *
 * Description   : This function is to store one region's (from start position to
 *                  the end position) files into existingFiles Hashtable, and
 *                  tempDeletedFilesHashTable separately
 *
 * Input         : u4StartPos   -   The start position of an Entry in the block
 *                 u4EndPos     -   The end position of an Entry in the block
 *                 pBuffer      -   The buffer which store this block's content
 *                 i4DeletedFlag-   A flag to check if this region is deleted
 *
 * Output        : existingFilesInOneFir     - hashtable storing existing files in
 *                                             this region
 *                 tempDeletedFilesInOneDir  - hashtable storing "maybe" deletd
 *                                             files in this region
 *
 * Returns       : INODE_SUCCESS, if read and store this region files successfully
 *                 INODE_FAILURE otherwise
 *
 **************************************************************************/
INT4 InodeListStoreRegionDirFiles(UINT4 u4StartPos, UINT4 u4EndPos, CHAR *pBuffer, INT4 i4DeletedFlag,
                                  struct htFilesStoreInOneDir **existingFilesInOneDir,
                                  struct htFilesStoreInOneDir **tempDeletedFilesInOneDir, char *path) {
    UINT4 u4RealLength;
    UINT2 u2RecLength;
    UINT4 u4NextPos;
    char inPath[1000];
    //TODO: modify no hardcode!
    struct ext3_dir_entry_2 DirEntry;
    struct ext3_dir_entry_2 NullDirEntry;
    struct htFilesStoreInOneDir *FileEntryInfo;
    struct DirEntryInfo *DirEntryInfo;

    u4RealLength = 0;
    u2RecLength = 0;
    u4NextPos = 0;
    memset(&DirEntry, 0, sizeof(DirEntry));
    memset(&NullDirEntry, 0, sizeof(NullDirEntry));
    memset(inPath, 0, sizeof(inPath));
    DirEntryInfo = malloc(sizeof(struct DirEntryInfo));
    FileEntryInfo = malloc(sizeof(struct htFilesStoreInOneDir));
    strcat(inPath, path);

    /*condition 1: reach the end, return*/
    if (u4StartPos == u4EndPos) {
        return INODE_SUCCESS;
    } else {
        memset(&DirEntry, 0, sizeof(DirEntry));
        /* Parse and read a directory record entry in the data block */
        if (InodeDirReadRecord(pBuffer, u4StartPos, &DirEntry) == INODE_FAILURE) {
            DEBUG_LOG3("ERROR: Failed to read directory entry from block: %d %s:%d\n",
                       u4BlockNo, __FILE__, __LINE__);
            return INODE_FAILURE;
        }

        /*condition 2: if the following data are 0, this always happens when the deleted files are in the last part*/
        if (memcmp(&DirEntry, &NullDirEntry, sizeof(NullDirEntry)) == 0) {
            return INODE_SUCCESS;
        } else {
            u2RecLength = DirEntry.rec_len;
            if (DirEntry.name_len % 4 != 0) {
                u4RealLength = (DirEntry.name_len + (4 - DirEntry.name_len % 4) + sizeof(UINT8));
            } else {
                u4RealLength = DirEntry.name_len + sizeof(UINT8);
            }

            /*generate an entry in the hashtable*/
            DirEntryInfo->DirEntry = DirEntry;
            strcpy(DirEntryInfo->path, path);
            FileEntryInfo->u4InodeNo = DirEntry.inode;
            FileEntryInfo->entryInfo = DirEntryInfo;

            /*add existing files and maybe deleted files*/
            if (i4DeletedFlag == NOT_DELETED_FILE) {
                if (strcmp(DirEntry.name, ".") != 0 && strcmp(DirEntry.name, "..") != 0 && (DirEntry.file_type == 2)) {
                    strncat(inPath, DirEntry.name, DirEntry.name_len);
                    strcat(inPath, "/");
                    InodeListScanFromThisDirInode(DirEntry.inode, inPath);
                }
                if (DirEntry.inode == 0) {
                    HASH_ADD_INT(*tempDeletedFilesInOneDir, u4InodeNo, FileEntryInfo);
                } else {
                    HASH_ADD_INT(*existingFilesInOneDir, u4InodeNo, FileEntryInfo);
                }
                /*real length is not equal to rec_length, that caused by the next entries are deleted*/
                if (u2RecLength != u4RealLength) {
                    InodeListStoreRegionDirFiles(u4StartPos + u4RealLength, u4StartPos + u2RecLength, pBuffer,
                                                 DELETED_FILE,
                                                 existingFilesInOneDir, tempDeletedFilesInOneDir, path);
                }
                u4NextPos = u2RecLength;
            } else {
                u4NextPos = u4RealLength;
                if (DirEntry.inode != 0) {
                    struct htFilesStoreInOneDir *temp;
                    HASH_FIND_INT(*tempDeletedFilesInOneDir, &FileEntryInfo->u4InodeNo, temp);
                    if (temp == NULL) {
                        HASH_ADD_INT(*tempDeletedFilesInOneDir, u4InodeNo, FileEntryInfo);
                    } else {
                        struct htFilesStoreInOneDir *replaced;
                        HASH_REPLACE_INT(*tempDeletedFilesInOneDir, u4InodeNo, FileEntryInfo, replaced);
                    }
                }
                if (u4StartPos + u4NextPos != u4EndPos) {
                    i4DeletedFlag = DELETED_FILE;
                } else {
                    i4DeletedFlag = NOT_DELETED_FILE;
                }
            }
            return InodeListStoreRegionDirFiles(u4StartPos + u4NextPos, u4EndPos, pBuffer, i4DeletedFlag,
                                                existingFilesInOneDir, tempDeletedFilesInOneDir, path);
        }
    }
}

/*! *************************************************************************
 * Function Name : InodeListScanFromThisDirInode
 *
 * Author        : Xinyi Li
 *
 * Description   : This Function is used to read a file system block
 *
 * Input         : u4BlockNo    -   Block number to read
 *                 path         -   the path of current dir inode
 *
 * Output        : None
 *
 * Returns       : INODE_SUCCESS, if the scan succeeds
 *                 INODE_FAILURE otherwise
 *
 **************************************************************************/
INT4 InodeListScanFromThisDirInode(UINT4 u4InodeNo, char *path) {
    struct ext3_inode Inode;
    INT4 i4RetVal;
    UINT4 u4BlockIndex;
    UINT4 u4BlockNumber;
    CHAR *pBlockBuffer;
    CHAR Buffer[MAX_BLOCK_SIZE];
    struct ext3_dir_entry_2 CurDirEntry;
    UINT4 u4CurrentDirInodeNo;
    struct htSortByCurDirInode *existingFileEntry;
    struct htSortByCurDirInode *tempDeletedFileEntry;

    memset(&Inode, 0, sizeof(Inode));
    i4RetVal = 0;
    u4BlockIndex = 0;
    u4BlockNumber = 0;
    memset(Buffer, 0, sizeof(Buffer));
    pBlockBuffer = Buffer;
    memset(&CurDirEntry, 0, sizeof(struct ext3_dir_entry_2));
    u4CurrentDirInodeNo = 0;
    existingFileEntry = malloc(sizeof(struct htSortByCurDirInode));
    tempDeletedFileEntry = malloc(sizeof(struct htSortByCurDirInode));

    if (InodeUtilReadInode(u4InodeNo, &Inode) == INODE_FAILURE) {
        DEBUG_LOG3("ERROR: Failed to read Inode: %d %s:%d\n", u4InodeNo, __FILE__,
                   __LINE__);
        return INODE_FAILURE;
    }

    /*this part is to validate if this inode is used, we have validate this before this
     * function, however, you can umcomment if you want to use it in the future*/
#if 0
    if (InodeUtilIsFreeInode(&Inode) == TRUE) {
        printf(" Inode unused\n");
        return INODE_SUCCESS;
    }
#endif
    /*Read first dir data block*/
    i4RetVal = InodeUtilReadDataBlock(Inode.i_block[FIRST_DIR_BLOCK_INDEX], 0, pBlockBuffer, gu4BlockSize);
    if (i4RetVal == INODE_FAILURE) {
        DEBUG_LOG3("ERROR: Failed to read first Data Block of this inode: %d %s:%d\n",
                   *(pBlockNoBuffer + (u4Index * gu4BlockSize)),
                   __FILE__, __LINE__);
        return INODE_FAILURE;
    }

    /*get current dir inode number*/
    if (InodeDirReadRecord(pBlockBuffer, DIR_CURR_ENTRY_OFFSET, &CurDirEntry) == INODE_FAILURE) {
        DEBUG_LOG("ERROR: Failed to retrieval the current dir inode number");
        //TODO: modify debug;
        return INODE_FAILURE;
    }
    u4CurrentDirInodeNo = CurDirEntry.inode;

    existingFileEntry->u4CurrentDirInodeNo = u4CurrentDirInodeNo;
    existingFileEntry->htFileStoreInThisDir = NULL;
    tempDeletedFileEntry->u4CurrentDirInodeNo = u4CurrentDirInodeNo;
    tempDeletedFileEntry->htFileStoreInThisDir = NULL;
    HASH_ADD_INT(existingFiles, u4CurrentDirInodeNo, existingFileEntry);
    HASH_ADD_INT(tempDeletedFiles, u4CurrentDirInodeNo, tempDeletedFileEntry);

    /*read every block belong to this dir inode*/
    while (Inode.i_block[u4BlockIndex] != 0) {
        u4BlockNumber = Inode.i_block[u4BlockIndex];
        memset(Buffer, 0, sizeof(Buffer));

        /*Read this Data block*/
        i4RetVal = InodeUtilReadDataBlock(u4BlockNumber, 0, pBlockBuffer, gu4BlockSize);
        if (i4RetVal == INODE_FAILURE) {
            DEBUG_LOG3("ERROR: Failed to read Data Block: %d %s:%d\n", *(pBlockNoBuffer + (u4Index * gu4BlockSize)),
                       __FILE__, __LINE__);
            return INODE_FAILURE;
        }

        /*read the region of the dir recursively*/
        if (InodeListStoreRegionDirFiles(0, gu4BlockSize, pBlockBuffer, NOT_DELETED_FILE,
                                         &(existingFileEntry->htFileStoreInThisDir),
                                         &(tempDeletedFileEntry->htFileStoreInThisDir), path) == INODE_FAILURE) {
            DEBUG_LOG("ERROR: Failed to list root Dir");
            //TODO: modify the debug information
            return INODE_FAILURE;
        }
        u4BlockIndex++;
    }
}


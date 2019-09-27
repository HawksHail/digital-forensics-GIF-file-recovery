#ifndef NTFS_READER
#define NTFS_READER

#include <string.h>
#include <vector>
#include <stdlib.h>

#include "DiskReader.h"
#include "MFTEntry.h"
#include "NTFSConst.h"
#include "NTFSStructs.h"

class NTFSReader {
private:
	DiskReader disk;
	BootSector bs;
	MFTEntry mft;
	std::vector<ClusterRun> mftRun;
	
	uint8_t *mftBitmap;
	uint64_t mftBitmapLength;
	
	uint8_t *diskBitmap;
	uint64_t diskBitmapLength;

	uint64_t bytesPerClust;
	uint64_t mftByteOffset;
	unsigned int maxIdx;

	//void setMFTRun(std::vector<NTFS_ATTRIBUTE> attrs);*/
	
	uint64_t getMFTEntryStartLoc(unsigned int idx);
	void getNonResidentData(std::vector<ClusterRun> run, uint8_t*& buffer, uint64_t& length);

public:
	// constructor
	NTFSReader(const char *filename);
	
	MFTEntry getMFTEntryAt(unsigned int idx);
	void printMFTEntryAt(unsigned int idx);
	void printDirectory(unsigned int idx);
	void listFileNames();
	std::string getPath(unsigned int idx);
	void listDeletedFiles();
	int percentRecoverable(unsigned int idx);
	
	bool isBitmapBitActive(uint8_t *bitmap, unsigned int idx);
	bool isMFTEntryFree(unsigned int idx);
	bool isClusterFree(unsigned int idx);
	bool isClobbered(unsigned int cluster, uint64_t timestamp);
};

#endif

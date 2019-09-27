/*
 * 	An MFT Entry is a 1KB (0x400 Byte) data structure that contains information about
 * 	each file and directory on disk.  It is structured as follows:
 * 
 * 	------------------------------------
 * 	|          RECORD HEADER           |
 * 	|----------------------------------|
 * 	|	        ATTRIBUTE 1            |
 * 	|           ATTRIBUTE 2            |
 * 	|               ...                |
 * 	|           ATTRIBUTE N            |
 * 	------------------------------------
 * 
 * 	Attributes are defined in Attribute.h
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */

#ifndef MFTENTRY
#define MFTENTRY

#include <vector>
#include <string>
#include <cstring>
#include <stdint.h>

#include "Attribute.h"
#include "NTFSConst.h"

struct RecordHeader {
	/* 0x00 */	uint32_t fileMagicNum;
	/* 0x04 */	uint16_t updateSeqOffset;
	/* 0x06 */	uint16_t updateSeqSize;
	/* 0x08 */	uint64_t lsn;
	/* 0x10 */	uint16_t seqNum;
	/* 0x12 */	uint16_t hardLinkCount;
	/* 0x14 */	uint16_t attrOffset;
	/* 0x16 */	uint16_t flags;				// 0x01 = in use, 0x02 = directory
	/* 0x18 */	uint32_t recordRealSize;
	/* 0x1C */	uint32_t recordAllocSize;
	/* 0x20 */	uint64_t baseFileRef;
	/* 0x28 */	uint64_t nextAttrId;
	/* 0x2A */	uint32_t unused;
	
	RecordHeader() : fileMagicNum(0) {}
	
	void print() {
		printf("--------------------------------------------------------------\n");
		printf("|                    FILE RECORD HEADER                      |\n");
		printf("--------------------------------------------------------------\n");
		printf("Magic number: 0x%08X\n", fileMagicNum);
		printf("Update sequence offset: 0x%04X\n", updateSeqOffset);
		printf("Update sequence size: 0x%04X\n", updateSeqSize);
		printf("LogFile sequence number: 0x%016lX\n", lsn);
		printf("Sequence number: 0x%04X\n", seqNum);
		printf("Hard link count: 0x%04X\n", hardLinkCount);
		printf("Attribute offset: 0x%04X\n", attrOffset);
		printf("Flags: 0x%04X\n", flags);
		printf("Record real size: 0x%08X\n", recordRealSize);
		printf("Record allocated size: 0x%08X\n", recordAllocSize);
		printf("Base file reference: 0x%016lX\n", baseFileRef);
		printf("Next attribute id: 0x%016lX\n", nextAttrId);
	}
};

class MFTEntry {
private:
	RecordHeader recHeader;
	uint16_t updateSeqNum;
	uint8_t *updateSeqArray;
	std::vector<Attribute> attribute;
	
	int findAttributeWithType(uint32_t type);

public:
	MFTEntry() {};	// default constructor
	MFTEntry(uint8_t *buffer, uint32_t length);
	
	// get attribute information
	StandardInformation getStandardInformation();
	FileName getFileName();
	Data getData();
	IndexRoot getIndexRoot();
	IndexAllocation getIndexAllocation(uint8_t *buffer, uint32_t length);
	Bitmap getBitmap();
	
	// get cluster runs for different attribute types
	std::vector<ClusterRun> getClusterRunList(uint8_t *buffer);
	std::vector<ClusterRun> getDataClusterRunList();
	std::vector<ClusterRun> getBitmapClusterRunList();
	std::vector<ClusterRun> getIndexAllocationClusterRunList();
	std::vector<IndexEntry> getIndexEntryListFromBuffer(uint8_t *buffer);
	
	// mft entry validity check
	bool isValid();
	bool isSubNodeFlagSet(uint32_t flag);
	bool isDirectory();
	
	void print();
};
#endif
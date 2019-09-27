#ifndef ATTRIBUTE
#define ATTRIBUTE

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <string>

#include "NTFSConst.h"

#pragma pack(push,1)

struct AttributeHeader {
	/* 0x00 */	uint32_t type;							// attribute type (e.g., 0x10, 0x80)
	/* 0x04 */	uint32_t length;						// length including this header
	/* 0x08 */	uint8_t nonResidentFlag;				// 0 = resident, 1 = non-resident
	/* 0x09 */	uint8_t nameLength;						// if named (N chars), then length = 2N
	/* 0x0A */	uint16_t offsetToName;					// either 0x00, 0x18, or 0x40
	/* 0x0C */	uint16_t flags;							// 0x01 = Compressed, 0x4000 = Encrypted, 0x8000 = Sparse
	/* 0x0E */	uint16_t attributeId;					// each attribute has a unique identifier
	
	union ResNonRes {
		struct ResidentHeader {
			/* 0x10 */	uint32_t attributeLength;		// length of attribute data
			/* 0x14 */	uint16_t offsetToAttribute;		// rounded up to a multiple of 4 bytes
			/* 0x16 */	uint8_t indexedFlag;
			/* 0x17 */	uint8_t padding;
		} resident;

		struct NonResidentHeader {
			/* 0x10 */	uint64_t startingVCN;
			/* 0x18 */	uint64_t lastVCN;
			/* 0x20 */	uint16_t datarunOffset;			// either 0x40 or 0x40+2N
			/* 0x22 */	uint16_t compressionUnitSize;	// unit size = 2^x clusters. 0 => uncompressed
			/* 0x24 */	uint8_t padding[4];
			/* 0x28 */	uint64_t allocAttrSize;			// rounded up to the cluster size
			/* 0x30 */	uint64_t realAttrSize;			// real size of the attribute
			/* 0x38 */	uint64_t initStreamSize;		// compressed data size
		} nonResident;
	} resNonRes;
	
	AttributeHeader() { length = 0; }
	
	void print() {
		printf("--------------------------------------------------------------\n");
		printf("|                     ATTRIBUTE HEADER                       |\n");
		printf("--------------------------------------------------------------\n");
		
		printf("Type: 0x%08X\n", type);
		printf("Length: 0x%08X\n", length);
		printf("Non-Resident Flag: 0x%02X\n", nonResidentFlag);
		printf("Name length: 0x%02X\n", nameLength);
		printf("Offset to name: 0x%04X\n", offsetToName);
		printf("Flags: 0x%04X\n", flags);
		printf("Attribute Id: 0x%04X\n", attributeId);
		
		if(nonResidentFlag) {
			printf("Starting VCN: 0x%016lX\n", resNonRes.nonResident.startingVCN);
			printf("Last VCN: 0x%016lX\n", resNonRes.nonResident.lastVCN);
			printf("Datarun offset: 0x%04X\n", resNonRes.nonResident.datarunOffset);
			printf("Compression unit size: 0x%04X\n", resNonRes.nonResident.compressionUnitSize);
			printf("Allocated attribute size: 0x%016lX\n", resNonRes.nonResident.allocAttrSize);
			printf("Real attribute size: 0x%016lX\n", resNonRes.nonResident.realAttrSize);
			printf("Compressed data size: 0x%016lX\n", resNonRes.nonResident.initStreamSize);
		} else {
			printf("Attribute Length: 0x%08X\n", resNonRes.resident.attributeLength);
			printf("Offset to attribute: 0x%04X\n", resNonRes.resident.offsetToAttribute);
			printf("Indexed flag: 0x%02X\n", resNonRes.resident.indexedFlag);
		}
		printf("\n");
	}
};

struct Attribute {
	AttributeHeader header;
	std::string name;
	uint8_t *data;
	uint32_t dataLength;
	
	void print() {
		header.print();
		
		if(header.nameLength > 0) {
			printf("Attribute name: %s\n", name.c_str());
		}
		
		printf("--------------------------------------------------------------\n");
		printf("|                      ATTRIBUTE DATA                        |\n");
		printf("--------------------------------------------------------------\n");
		
		for(int i = 0; i < dataLength; i+=16) {
			for(int j = i; j < dataLength && j < i+16; j++) {
				if(j % 4 == 0) {
					printf(" ");
				}
				printf("%02X ", data[j]);
			}
			printf("\n");
		}
	}
};

// $STANDARD_INFORMATION = 0x10, always resident

struct StandardInformationHeader {
	/* 0x00 */	uint64_t fileCreation;
	/* 0x08 */	uint64_t fileAltered;
	/* 0x10 */	uint64_t mftChanged;
	/* 0x18 */	uint64_t fileRead;
	/* 0x20 */	uint32_t dosFilePermissions;
	/* 0x24 */	uint32_t maxNumVersions;
	/* 0x28 */	uint32_t versionNum;
	/* 0x2C */	uint32_t classId;
	/* 0x30 */	uint32_t ownerId;
	/* 0x34 */	uint32_t securityId;
	/* 0x38 */	uint64_t quotaCharged;
	/* 0x40 */	uint64_t usn;
	
	void print() {
		printf("--------------------------------------------------------------\n");
		printf("|                   STANDARD INFORMATION                     |\n");
		printf("--------------------------------------------------------------\n");
		
		printf("File creation: 0x%016lX\n", fileCreation);
		printf("File altered: 0x%016lX\n", fileAltered);
		printf("MFT changed: 0x%016lX\n", mftChanged);
		printf("File read: 0x%016lX\n", fileRead);
		printf("DOS file permissions: 0x%08X\n", dosFilePermissions);
		printf("Maximun number of versions: 0x%08X\n", maxNumVersions);
		printf("Version number: 0x%08X\n", versionNum);
		printf("Class Id: 0x%08X\n", classId);
		printf("Owner Id: 0x%08X\n", ownerId);
		printf("Security Id: 0x%08X\n", securityId);
		printf("Quota charged: 0x%016lX\n", quotaCharged);
		printf("USN: 0x%016lX\n", usn);
	}
};

struct StandardInformation {
	AttributeHeader attrHeader;
	StandardInformationHeader sInfoHeader;
	
	void print() {
		attrHeader.print();
		sInfoHeader.print();
	}
};

// $FILE_NAME = 0x30, always resident

struct FileNameHeader {
	/* 0x00 */	uint64_t fileRef;			// file reference to the parent directory 
											// (6 bytes FILE record number + 2 bytes sequence number)
	/* 0x08 */	uint64_t fileCreation;
	/* 0x10 */	uint64_t fileAltered;
	/* 0x18 */	uint64_t mftChanged;
	/* 0x20 */	uint64_t fileRead;
	/* 0x28 */	uint64_t allocSize;
	/* 0x30 */	uint64_t realSize;
	/* 0x38 */	uint32_t flags;				// see below
	/* 0x3C */	uint32_t reparse;
	/* 0x40 */	uint8_t filenameLength;
	/* 0x41 */	uint8_t fileNamespace;
	
	/* Flags
	 * 0x0001 = Read-Only
	 * 0x0002 = Hidden
	 * 0x0004 = System
	 * 0x0020 = Archive
	 * 0x0040 = Device
	 * 0x0080 = Normal
	 * 0x0100 = Temporary
	 * 0x0200 = Sparse File
	 * 0x0400 = Reparse Point
	 * 0x0800 = Compressed
	 * 0x1000 = Offline
	 * 0x2000 = Not Content Indexed
	 * 0x4000 = Encrypted
	 * 0x10000000 = Directory (copy from corresponding bit in MFT record)
	 * 0x20000000 = Index View (copy from corresponding bit in MFT record)
	 */
	
	void print() {
		printf("--------------------------------------------------------------\n");
		printf("|                         FILE NAME                          |\n");
		printf("--------------------------------------------------------------\n");
		
		printf("File reference: 0x%016lX\n", fileRef);
		printf("File creation: 0x%016lX\n", fileCreation);
		printf("File altered: 0x%016lX\n", fileAltered);
		printf("MFT changed: 0x%016lX\n", mftChanged);
		printf("File read: 0x%016lX\n", fileRead);
		printf("Allocated size: 0x%016lX\n", allocSize);
		printf("Real size: 0x%016lX\n", realSize);
		printf("Flags: 0x%08X\n", flags);
		printf("Reparse point: 0x%08X\n", reparse);
		printf("Filename Length: 0x%02X\n", filenameLength);
		printf("File namespace: 0x%02X\n", fileNamespace);
	}
};

struct FileName {
	AttributeHeader attrHeader;
	FileNameHeader fNameHeader;
	std::string filename;
	
	FileName() : filename("") {}
	
	void print() {
		attrHeader.print();
		fNameHeader.print();
		printf("Filename: %s\n", filename.c_str());
	}
};

// $DATA = 0x80
// if resident then data is real data
// if non-resident then data is cluster runs

struct Data {
	AttributeHeader attrHeader;
	uint8_t *data;
	uint32_t dataLength;
	
	Data() : dataLength(0) {}
	
	void print() {
		attrHeader.print();
		
		printf("--------------------------------------------------------------\n");
		printf("|                           DATA                             |\n");
		printf("--------------------------------------------------------------\n");
		
		for(int i = 0; i < dataLength; i+=16) {
			for(int j = i; j < dataLength && j < i+16; j++) {
				if(j % 4 == 0) {
					printf(" ");
				}
				printf("%02X ", data[j]);
			}
			printf("\n");
		}
	}
};

struct IndexNodeHeader {
	/* 0x00 */	uint32_t entryOffset;		// offset to first index entry
	/* 0x04 */	uint32_t totalSize;			// total size of the index entries
	/* 0x08 */	uint32_t allocSize;			// allocated size of the node
	/* 0x0C */	uint8_t nonLeafNodeFlag;	// 1 = has sub-nodes
	/* 0x0D */	uint8_t padding[3];
	
	void print() {
		printf("Entry offset: 0x%08X\n", entryOffset);
		printf("Total size: 0x%08X\n", totalSize);
		printf("Allocated size: 0x%08X\n", allocSize);
		printf("Non-leaf node flag: 0x%02X\n", nonLeafNodeFlag);
	}
};

struct IndexEntryHeader {
	/* 0x00 */	uint64_t fileRef;
	/* 0x08 */	uint16_t indexEntryLength;
	/* 0x0A */	uint16_t streamLength;
	/* 0x0C */	uint32_t flags;				
	// 1 = Index entry points to a sub-node
	// 2 = Last index entry in the node
	
	void print() {
		printf("File reference: 0x%016lX\n", fileRef);
		printf("Index entry length: 0x%04X\n", indexEntryLength);
		printf("Stream length: 0x%04X\n", streamLength);
		printf("Flags: 0x%08X\n", flags);
	}
};

struct IndexEntry {
	/* 0x00 */	IndexEntryHeader ieHeader;
	/* 0x10 */	uint8_t *stream;			// only present when the last entry flag is NOT set
	/* L-8  */	uint64_t subNodeVCN;		// only present when the sub-node flag is set
	FileName fn;
	
	void print() {
		ieHeader.print();
		
		if(ieHeader.streamLength > 0) {
			/*printf("--------------------------------------------------------------\n");
			printf("|                           DATA                             |\n");
			printf("--------------------------------------------------------------\n");
		
			for(int i = 0; i < ieHeader.streamLength; i+=16) {
				for(int j = i; j < ieHeader.streamLength && j < i+16; j++) {
					if(j % 4 == 0) {
						printf(" ");
					}
					printf("%02X ", stream[j]);
				}
				printf("\n");
			}*/
			
			
			fn.fNameHeader.print();
			printf("Filename: %s\n", fn.filename.c_str());
		}
		
		printf("Subnode VCN: 0x%016lX\n", subNodeVCN);
	}
};

struct IndexRootHeader {
	/* 0x00 */	uint32_t attributeType;
	/* 0x04 */	uint32_t collationRule;
	/* 0x08 */	uint32_t bytesPerIndexRec;
	/* 0x0C */	uint32_t clustersPerIndexRec;
	
	void print() {
		printf("--------------------------------------------------------------\n");
		printf("|                        INDEX ROOT                          |\n");
		printf("--------------------------------------------------------------\n");
	  
		printf("Attribute type: 0x%08X\n", attributeType);
		printf("Collation rule: 0x%08X\n", collationRule);
		printf("Bytes per index record: 0x%08X\n", bytesPerIndexRec);
		printf("Clusters per index record: 0x%08X\n", clustersPerIndexRec);
	}
};

// $INDEX_ROOT = 0x90, always resident

struct IndexRoot {
	/* ~~~~ */	AttributeHeader attrHeader;
	/* 0x00 */	IndexRootHeader idxRootHeader;
	/* 0x10 */	IndexNodeHeader idxNodeHeader;
	/* 0x20 */	std::vector<IndexEntry> idxEntry;
	
	void print() {
		attrHeader.print();
		idxRootHeader.print();
		idxNodeHeader.print();
		
		printf("--------------------------------------------------------------\n");
		printf("|                     INDEX ENTRY LIST                       |\n");
		printf("--------------------------------------------------------------\n");
		
		for(int i = 0; i < idxEntry.size(); i++) {
			printf("--------------------------------------------------------------\n");
			printf("|                        ENTRY %04d                          |\n", i);
			printf("--------------------------------------------------------------\n");	
			idxEntry[i].print();
		}
	}
};

struct ClusterRun {
	uint64_t offset;
	uint64_t length;
	
	ClusterRun() : offset(0), length(0) {}
};

struct IndexRecordHeader {
	/* 0x00 */	uint32_t indxMagicNumber;
	/* 0x04 */	uint16_t updateSeqOffset;
	/* 0x06 */	uint16_t updateSeqSize;
	/* 0x08 */	uint64_t logFileSeqNum;
	/* 0x10 */	uint64_t idxRecordVCN;
	
	void print() {
		printf("INDX magic number: 0x%08X\n", indxMagicNumber);
		printf("Update sequence offset: 0x%04X\n", updateSeqOffset);
		printf("Update sequence size: 0x%04X\n", updateSeqSize);
		printf("LogFile sequence number: 0x%016lX\n", logFileSeqNum);
		printf("Index record VCN: 0x%016lX\n", idxRecordVCN);
	}
};

struct IndexRecord {
	/* 0x00 */	IndexRecordHeader iRecHeader;
	/* 0x18 */	IndexNodeHeader iNodeHeader;
	/* 0x28 */	uint16_t updateSeq;
	/* 0x2A */	uint8_t *updateSeqArray;
	/* ~~~~ */	std::vector<IndexEntry> idxEntry;
	
	void print() {
		iRecHeader.print();
		iNodeHeader.print();
		
		printf("--------------------------------------------------------------\n");
		printf("|                     INDEX ENTRY LIST                       |\n");
		printf("--------------------------------------------------------------\n");
		
		for(int i = 0; i < idxEntry.size(); i++) {
			printf("--------------------------------------------------------------\n");
			printf("|                        ENTRY %04d                          |\n", i);
			printf("--------------------------------------------------------------\n");	
			idxEntry[i].print();
		}
	}
};

// $INDEX_ALLOCATION = 0xA0
// always non-resident

struct IndexAllocation {
	AttributeHeader attrHeader;
	std::vector<IndexRecord> idxRecord;
	
	void print() {
		attrHeader.print();
		
		printf("--------------------------------------------------------------\n");
		printf("|                     INDEX RECORD LIST                      |\n");
		printf("--------------------------------------------------------------\n");
		
		for(int i = 0; i < idxRecord.size(); i++) {
			printf("--------------------------------------------------------------\n");
			printf("|                       RECORD %04d                          |\n", i);
			printf("--------------------------------------------------------------\n");	
			idxRecord[i].print();
		}
	}
};

// $BITMAP = 0xB0
// attribute used in two places: indexes (e.g., directories) and $MFT

struct Bitmap {
	AttributeHeader attrHeader;
	uint8_t *bitfield;
	uint64_t bitfieldLength;
	
	bool isValid() {
		return attrHeader.length > 0 && bitfieldLength > 0;
	}
	
	void print() {
		attrHeader.print();
	  
		printf("--------------------------------------------------------------\n");
		printf("|                          BITMAP                            |\n");
		printf("--------------------------------------------------------------\n");
		
		for(int i = 0; i < bitfieldLength; i+=16) {
			for(int j = i; j < bitfieldLength && j < i+16; j++) {
				if(j % 4 == 0) {
					printf(" ");
				}
				printf("%02X ", bitfield[j]);
			}
			printf("\n");
		}
	}
};

#pragma pack(pop)
#endif
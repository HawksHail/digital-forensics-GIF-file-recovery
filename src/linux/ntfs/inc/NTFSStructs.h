#ifndef NTFS_STRUCTS
#define NTFS_STRUCTS

#include <stdint.h>
#include <vector>

#pragma pack(push,1)

struct BootSector {
	/* 0x00 */	uint8_t jumpInstruction[3];
	/* 0x03 */	uint64_t oemId;

	struct Bpb {
	/* 0x0B */	uint16_t bytesPerSec;
	/* 0x0D */	uint8_t secPerClust;
	/* 0x0E */	uint16_t reservedSec;
	/* 0x10 */	uint8_t reserved[3];
	/* 0x13 */	uint16_t unused1;
	/* 0x15 */	uint8_t mediaDescriptor;
	/* 0x16 */	uint16_t unused2;
	/* 0x18 */	uint16_t secPerTrack;
	/* 0x1A */	uint16_t numberOfHeads;
	/* 0x1C */	uint32_t hiddenSec;
	/* 0x20 */	uint32_t zero;
	/* 0x24 */	uint32_t unused3;
	/* 0x28 */	uint64_t totalSec;
	/* 0x30 */	uint64_t mftLogicalClustNum;
	/* 0x38 */	uint64_t mftMirrLogicalClustNum;
	/* 0x40 */	uint32_t clustPerMFTRecord;
	/* 0x44 */	uint32_t clustPerIndexRecord;
	/* 0x48 */	uint64_t volumeSerialNum;
	/* 0x50 */	uint32_t checksum;
	} bpb;

	/* 0x54 */ uint8_t bootCode[426];
	/* 0x1FE */uint16_t secMark;
	
	void print() {
		printf("-------------------------------------------------------\n");
		printf("|                    BOOT SECTOR                      |\n");
		printf("-------------------------------------------------------\n");
		
		printf("OEM ID: 0x%016lX\n", oemId);
		printf("Bytes per Sector: 0x%04X\n", bpb.bytesPerSec);
		printf("Sectors per Cluster: 0x%02X\n", bpb.secPerClust);
	}
};

#endif

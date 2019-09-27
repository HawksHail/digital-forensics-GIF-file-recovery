#include "NTFSReader.h"

 NTFSReader::NTFSReader(const char *filename) {
	// try to open file
	if(!disk.openPath(filename)) {
		printf("Can't open file. Exiting\n");
		exit(0);
	}
	
	// Read NTFS Boot Sector 
	disk.readData(&bs, 0, sizeof(BootSector));

	// Validity Check
	if(bs.secMark != NTFS_END_OF_SECTOR || bs.oemId != NTFS_OEM) {
		printf("OEM ID: %lx\n", bs.oemId);
		printf("EOS Marker: %x\n", bs.secMark);
		printf("Invalid/Corrupted NTFS Boot Sector\n");
		exit(0);
	}

	// Calculate some needed info
	bytesPerClust = bs.bpb.bytesPerSec * bs.bpb.secPerClust;
	mftByteOffset = bs.bpb.mftLogicalClustNum * bytesPerClust;
	
	printf("\nBlock Size: %li Bytes\n\n", (long) bytesPerClust);

	// Read First Entry in MFT
	uint8_t buffer[MFT_ENTRY_SIZE];
	disk.readData(buffer, mftByteOffset, MFT_ENTRY_SIZE);
	mft = MFTEntry(buffer, MFT_ENTRY_SIZE);
	
	// Validity Check
	if(!mft.isValid()) {
		printf("Invalid/Corrupted MFT entry 0\n");
		exit(0);
	}
	
	// get MFT cluster runs to calculate entry locations
	mftRun = mft.getDataClusterRunList();
	
	printf("MFT Located at:\n");
	uint64_t mftSize = 0;
	for(int runIdx = 0; runIdx < mftRun.size(); runIdx++) {
		mftSize += mftRun[runIdx].length;
		
		printf("Cluster [ 0x%lX - 0x%lX ]\n", 
			mftRun[runIdx].offset, 
			mftRun[runIdx].offset+mftRun[runIdx].length);
	}
	mftSize *= bytesPerClust;
	printf("MFT Size: 0x%lX bytes\n", mftSize);
	
	// if no cluster runs were set, then error
	if(mftSize == 0) {
		printf("Couldn't get MFT Run List\n");
		exit(0);
	}

	// set maximum number of entries for bounds checking
	maxIdx = mftSize / MFT_ENTRY_SIZE;
	printf("Num entries: %d\n", maxIdx);
	
	// get MFT Bitmap
	Bitmap b = mft.getBitmap();
	
	// validate
	if(!b.isValid()) {
		printf("Couldn't get MFT Bitmap.\n");
		exit(0);
	}
	
	// if non-resident, get non-resident data
	if(b.attrHeader.nonResidentFlag) {
		getNonResidentData(mft.getBitmapClusterRunList(), mftBitmap, mftBitmapLength);
	} else {
		mftBitmap = b.bitfield;
		mftBitmapLength = b.bitfieldLength;
	}
	
	// get Disk Bitmap
	MFTEntry diskBitmapEntry = getMFTEntryAt(DISK_BITMAP_INDEX);
	std::vector<ClusterRun> bitmapRun = diskBitmapEntry.getDataClusterRunList();
	getNonResidentData(bitmapRun, diskBitmap, diskBitmapLength);
}

void NTFSReader::getNonResidentData(std::vector<ClusterRun> run, uint8_t*& buffer, uint64_t& length) {
	// first calculate length;
	length = 0;
	for(int runIdx = 0; runIdx < run.size(); runIdx++) {
		length += run[runIdx].length * bytesPerClust;
	}
	
	//printf("NonResident data length: 0x%lX\n", length);
	
	// allocate memory and start copying
	buffer = new uint8_t[length];
	uint64_t diskByteLoc = 0;
	uint64_t bufferByteLoc = 0;
	uint64_t numBytes = 0;
	
	for(int runIdx = 0; runIdx < run.size(); runIdx++) {
		diskByteLoc = run[runIdx].offset * bytesPerClust;
		numBytes = run[runIdx].length * bytesPerClust;
		disk.readData(&buffer[bufferByteLoc], diskByteLoc, numBytes);
		bufferByteLoc += numBytes;
	}
	
	return;
}

uint64_t NTFSReader::getMFTEntryStartLoc(unsigned int idx) {
	uint64_t byteLoc = 0;
	uint64_t byteOffset = idx * MFT_ENTRY_SIZE;
	
	for(int runIdx = 0; runIdx < mftRun.size(); runIdx++) {
		byteLoc = mftRun[runIdx].offset * bytesPerClust;
		if(byteOffset < mftRun[runIdx].length * bytesPerClust) {
			return byteLoc + byteOffset;
		}
		byteOffset -= mftRun[runIdx].length * bytesPerClust;
	}
	return byteLoc;
}

MFTEntry NTFSReader::getMFTEntryAt(unsigned int idx) {
	MFTEntry e;
	uint8_t buffer[MFT_ENTRY_SIZE];
	uint64_t byteLoc;
	
	// check if is in bounds
	if(!(idx < maxIdx)) {
		printf("MFTEntry %d out of bounds\n", idx);
		return e;
	}
	
	// get byte start location and get data
	byteLoc = getMFTEntryStartLoc(idx);
	//printf("Entry start byte location: 0x%lX\n", byteLoc);
	disk.readData(buffer, byteLoc, MFT_ENTRY_SIZE);
	e = MFTEntry(buffer, MFT_ENTRY_SIZE);
	
	return e;
}

void NTFSReader::printMFTEntryAt(unsigned int idx) {
	MFTEntry e = getMFTEntryAt(idx);
	e.print();
	return;
}

void NTFSReader::printDirectory(unsigned int idx) {
	MFTEntry e = getMFTEntryAt(idx);
	uint64_t parentRef = e.getFileName().fNameHeader.fileRef & FILE_REF_MASK;
	
	printf("\nDirectory, Parent Directory\n");
	printf("%d, %ld\n\n", idx, parentRef);
	
	printf("MFT Entry, isDir?, Filename\n");
	IndexRoot ir = e.getIndexRoot();
	
	for(int i = 0; i < ir.idxEntry.size(); i++) {
		std::string filename = ir.idxEntry[i].fn.filename;
		uint64_t fileRef = ir.idxEntry[i].ieHeader.fileRef & FILE_REF_MASK;
		std::string directory = "NO";
		
		if(filename.size() == 0 || fileRef >= maxIdx) {
			continue;
		}
		
		if(getMFTEntryAt(fileRef).isDirectory()) {
			directory = "YES";
		}
		
		printf("%ld, %s, %s\n", fileRef, directory.c_str(), filename.c_str());
	}
	
	std::vector<ClusterRun> run = e.getIndexAllocationClusterRunList();
	
	uint8_t *buffer;
	uint64_t length;
	getNonResidentData(run, buffer, length);
	
	IndexAllocation ia = e.getIndexAllocation(buffer, length);
	
	for(int i = 0; i < ia.idxRecord.size(); i++) {
		for(int j = 0; j < ia.idxRecord[i].idxEntry.size(); j++) {
			std::string filename = ia.idxRecord[i].idxEntry[j].fn.filename;
			uint64_t fileRef = ia.idxRecord[i].idxEntry[j].ieHeader.fileRef & FILE_REF_MASK;
			std::string directory = "NO";
		  
			if(filename.size() == 0 || fileRef >= maxIdx) {
				continue;
			}
			
			if(getMFTEntryAt(fileRef).isDirectory()) {
				directory = "YES";
			}
		
			printf("%ld, %s, %s\n", fileRef, directory.c_str(), filename.c_str());
		}
	}
}

void NTFSReader::listFileNames() {
	printf("MFT Entry, Filename, Path, Clusters, Availability\n");
	for(int i = 0; i < maxIdx; i++) {
		MFTEntry e = getMFTEntryAt(i);
		FileName fn = e.getFileName();
		
		if(fn.filename.length() > 0) {
			printf("%d, %s, ", i, fn.filename.c_str());
			
			uint64_t parentDir = fn.fNameHeader.fileRef & 0x0000FFFFFFFFFFFF;
			if(parentDir != i) {
				printf("%s", getPath(parentDir).c_str());
			} else {
				printf(".");
			}
			
			printf(", ");
			
			std::vector<ClusterRun> run = e.getDataClusterRunList();
		
			for(int runIdx = 0; runIdx < run.size(); runIdx++) {
				printf("(0x%lX:0x%lX); ", run[runIdx].offset, run[runIdx].length);
			}
		
			if(run.size() == 0) {
				printf("resident/directory");
			}
			
			printf(", ");
			
			if(isMFTEntryFree(i)) {
				printf("free (%d%%)", percentRecoverable(i));
			} else {
				printf("used");
			}
		
			printf("\n");
		}
	}
}

std::string NTFSReader::getPath(unsigned int idx) {
	// get the MFT entry at idx
	MFTEntry e = getMFTEntryAt(idx);
	FileName fn = e.getFileName();
	if(fn.filename.length() == 0 || !(fn.fNameHeader.flags & 0x10000000)) {
		return "";
	}
	
	uint64_t parentDir = fn.fNameHeader.fileRef & 0x0000FFFFFFFFFFFF;
	if(parentDir != idx) {
		return getPath(parentDir) + "/" + fn.filename;
	}
	return fn.filename;
}

bool NTFSReader::isBitmapBitActive(uint8_t *bitmap, unsigned int idx) {
	unsigned int byteIdx = idx / 8;
	unsigned int bitIdx = idx % 8;
	bool active = (bitmap[byteIdx] >> bitIdx) & 0x01;
	return active;
}

bool NTFSReader::isMFTEntryFree(unsigned int idx) {
	return !isBitmapBitActive(mftBitmap, idx);
}

bool NTFSReader::isClusterFree(unsigned int idx) {
	return !isBitmapBitActive(diskBitmap, idx);
}

bool NTFSReader::isClobbered(unsigned int cluster, uint64_t timestamp) {
	for(int entryIdx = 0; entryIdx < maxIdx; entryIdx++) {
		MFTEntry e = getMFTEntryAt(entryIdx);
		if(!e.isValid()) {
			continue;
		}
		
		FileName fn = e.getFileName();
		if(fn.filename.length() <= 0) {
			continue;
		}
		
		std::vector<ClusterRun> run = e.getDataClusterRunList();

		for(int runIdx = 0; runIdx < run.size(); runIdx++) {
			if(run[runIdx].offset < cluster 
			  && cluster < run[runIdx].offset+run[runIdx].length 
			  && fn.fNameHeader.fileCreation > timestamp) {
				return true;
			}
		}
	}
	return false;
}

int NTFSReader::percentRecoverable(unsigned int idx) {
	MFTEntry e = getMFTEntryAt(idx);
	if(!e.isValid()) {
		return 0;
	}
	
	FileName fn = e.getFileName();
	if(fn.filename.length() <= 0) {
		return 0;
	}
	
	// check to see if clusters are free
	std::vector<ClusterRun> run = e.getDataClusterRunList();
	if(run.size() == 0) {
		return 100;
	}
	
	unsigned int cluster = 0;
	unsigned int end = 0;
	unsigned int freeCount = 0;
	unsigned int totalCount = 0;
	
	for(int i = 0; i < run.size(); i++) {
		cluster = run[i].offset;
		end = cluster + run[i].length;
		
		for(; cluster < end; cluster++) {
			if(isClusterFree(cluster) && !isClobbered(cluster, fn.fNameHeader.fileCreation)) {
				freeCount++;
			}
			totalCount++;
		}
	}
	
	return 100 * freeCount / totalCount;
}

void NTFSReader::listDeletedFiles() {
	printf("MFT Entry, Filename, Path, Clusters, Availability\n");
	// get list of all deleted files
	// mftEntryFree && mftEntryValid && mftEntryHasFilename
	for(int i = 0; i < maxIdx; i++) {
		if(!isMFTEntryFree(i)) {
			continue;
		}
	  
		MFTEntry e = getMFTEntryAt(i);
		if(!e.isValid()) {
			continue;
		}
		
		FileName fn = e.getFileName();
		if(fn.filename.length() > 0) {
			printf("%d, %s, ", i, fn.filename.c_str());
			
			uint64_t parentDir = fn.fNameHeader.fileRef & 0x0000FFFFFFFFFFFF;
			if(parentDir != i) {
				printf("%s", getPath(parentDir).c_str());
			} else {
				printf(".");
			}
			
			printf(", ");
			
			std::vector<ClusterRun> run = e.getDataClusterRunList();
		
			for(int runIdx = 0; runIdx < run.size(); runIdx++) {
				printf("(0x%lX:0x%lX); ", run[runIdx].offset, run[runIdx].length);
			}
		
			if(run.size() == 0) {
				printf("resident/directory");
			}
			
			printf(", ");
			
			if(isMFTEntryFree(i)) {
				printf("free (%d%%)", percentRecoverable(i));
			} else {
				printf("used");
			}
		
			printf("\n");
		}
	}
	printf("\n");
	
	// 
}
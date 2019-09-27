#include "DiskReader.h"
DiskReader::DiskReader(const char *pathname) {
	openPath(pathname);
}

DiskReader::~DiskReader() {
	printf("\nClosing disk\n");	
	close(fd);
}

/*
 * 	openPath opens a file given the path
 * 
 * 	@param pathname : input file name
 * 	@return : true if successful, false if not 
 * 
 */

bool DiskReader::openPath(const char *pathname) {
	if(fd >= 0) {
		close(fd);
	}

	fd = open(pathname, O_RDONLY);

	if (fd < 0) {
		printf("Could not open \"%s\"\n", pathname);
		return false;
	} 

	printf("Successfully opened \"%s\"\n", pathname);
	return true;
}

/*
 * 	readData fills in a buffer with data from [start - start+size]
 * 
 * 	@param buffer : area to fill in with data
 * 	@param start : start byte to seek to
 * 	@param size : number of bytes to read in
 * 	@return : value returned from read()
 * 
 * 	@assumption : sizeof(buffer) = size
 * 
 */

int DiskReader::readData(void *buffer, uint64_t start, uint64_t size) {
	lseek(fd, start, SEEK_SET);
	int ret = read(fd, buffer, size);
	return ret;
}

/*
 * 	printData is a helper function to pretty print data in the range [start - start+size]
 * 
 * 	@param start : start byte to seek to
 * 	@param size : number to bytes to print out
 * 
 */

void DiskReader::printData(uint64_t start, uint64_t size) {
	uint8_t buffer[size];
	readData(buffer, start, size);
	printBuffer(buffer,size);	
}

/*
 * 	printBuffer pretty prints byte data
 * 	
 * 	@param buffer : binary data
 * 	@param size :  length of the data to print out
 * 
 * 	@assumption : sizeof(buffer) = size
 * 
 */

void DiskReader::printBuffer(uint8_t *buffer, uint64_t size) {	
  
	printf("------------------------------------------------------------------------\n");
  
	for(int i = 0; i < size; i+=16) {
		printf("| ");
		for(int j = 0; j < 16; j++) {
			if(i+j < size) {
				printf("%02X ", buffer[i+j]);
			} else {
				printf("   ");
			}
			if(j%4 == 3) {
				printf(" ");
			}
		}
		
		for(int j = 0; j < 16; j++) {
			if(buffer[i+j] >= 0x20 && buffer[i+j] < 0x7F && i+j < size) {
				printf("%c", buffer[i+j]);
			} else if (i+j < size) {
				printf(".");
			} else {
				printf(" ");
			}
		}
		printf(" |\n");
	}
	printf("------------------------------------------------------------------------\n");
}

/*
 * 	getSize returns the byte size of the file currently loaded
 * 
 * 	@return : size in bytes of fd
 * 
 */

long DiskReader::getSize() {
	if(fd < 0) {
		printf("Cannot get size. No open file.\n");
		return -1;
	}

	return lseek(fd, 0, SEEK_END);
}

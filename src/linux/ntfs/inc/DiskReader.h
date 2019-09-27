#ifndef DISK_READER
#define DISK_READER

#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>

class DiskReader {
private:
	char *path;
	int fd;

public:
	DiskReader() : fd(0) {} ;
	DiskReader(const char *pathname);
	
	bool openPath(const char *pathname);
	int readData(void *buffer, uint64_t start, uint64_t size);
	
	void printData(uint64_t start, uint64_t size);
	void printBuffer(uint8_t *buffer, uint64_t size);
	
	int getFileDescriptor() { return fd; }
	char *getPathName() { return path; }
	long getSize();
	
	~DiskReader();
};

#endif

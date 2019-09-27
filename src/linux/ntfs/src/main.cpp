#include <iostream>

#include "NTFSReader.h"

using namespace std;

int main(int argc, char *argv[]) {
	NTFSReader disk = NTFSReader(argv[argc-1]);
	if(argc == 2) {
		int input = -1;
		while(true) {
			cout << "\nMFT Entry to parse (-1 to exit): ";
			cin.clear();
			cin >> input;

			if(input < 0) {
				break;
			}
			disk.printMFTEntryAt(input);
			break;
		}
	} else {
		switch(argv[1][1]) {
			case 'e' : {
				int input = atoi(argv[2]);
				disk.printMFTEntryAt(input);
				break;
			} case 'd' : {
				unsigned int input = atoi(argv[2]);
				disk.printDirectory(input);
				break;
			} case 'f': {
				printf("Printing all files in MFT\n");
				disk.listFileNames();
				break;
			} case 'p':{
				int input = atoi(argv[2]);
				printf("path: %s\n", disk.getPath(input).c_str());
				break;
			} case 'l' : {
				disk.listDeletedFiles();
				break;
			}
			default : {
				break;
			}
		}
	}
	return 0;
}

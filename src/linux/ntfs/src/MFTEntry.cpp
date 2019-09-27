#include "MFTEntry.h"

MFTEntry::MFTEntry(uint8_t *buffer, uint32_t bufferLen) {
	uint32_t currLoc = 0;
  
	// parse the record header
	std::memcpy(&recHeader, buffer, sizeof(RecordHeader));
	
	// validate 'FILE' magic number
	if(recHeader.fileMagicNum != FILE_MAGIC_NUMBER) {
		//printf("Corrupted MFT Entry: 'FILE' magic number mismatch.\n");
		return;
	}
	
	// parse the update sequence if applicable
	if(recHeader.updateSeqSize > 0) {
		// allocate space for sequence array
		updateSeqArray = new uint8_t[recHeader.updateSeqSize];
		
		// start parsing
		currLoc = recHeader.updateSeqOffset;
		std::memcpy(&updateSeqNum, &buffer[currLoc], sizeof(uint16_t));
		std::memcpy(updateSeqArray, &buffer[currLoc+sizeof(uint16_t)], recHeader.updateSeqSize);
	}
	
	// parse the attributes
	currLoc = recHeader.attrOffset;
	while(currLoc < bufferLen) {
		Attribute attr;
		uint32_t offsetToAttrData = 0;
		
		// parse attribute header
		std::memcpy(&attr.header, &buffer[currLoc], sizeof(AttributeHeader));
		
		if(attr.header.type == ATTRIBUTE_LIST_TERMINATOR) {
			break;
		}
		
		// get name if applicable
		if(attr.header.nameLength > 0) {
			attr.name = std::string(attr.header.nameLength, 'x');
			
			uint32_t charStart = currLoc + attr.header.offsetToName;
			
			for(uint32_t charIdx = 0; charIdx < attr.header.nameLength; charIdx++) {
				attr.name[charIdx] = (char) buffer[charStart + 2*charIdx];
			}
		}
		
		// get offset to attribute
		if(attr.header.nonResidentFlag) {
			offsetToAttrData = NON_RESIDENT_HEADER_SIZE + 2 * attr.header.nameLength;
		} else { // resident data
			offsetToAttrData = RESIDENT_HEADER_SIZE + 2 * attr.header.nameLength;
		}
		
		// calculate data length, allocate memory, and copy
		attr.dataLength = attr.header.length - offsetToAttrData;
		attr.data = new uint8_t[attr.dataLength];
		std::memcpy(attr.data, &buffer[currLoc+offsetToAttrData], attr.dataLength*sizeof(uint8_t));
		
		// add to attribute list
		attribute.push_back(attr);
		
		// update current location to next attribute
		currLoc += attr.header.length;
	}
}

// get attribute object functions

bool MFTEntry::isValid() {
	return recHeader.fileMagicNum == FILE_MAGIC_NUMBER;
}

bool MFTEntry::isSubNodeFlagSet(uint32_t flag) {
	return flag & SUB_NODE_FLAG;
}

bool MFTEntry::isDirectory() {
	for(int i = 0; i < attribute.size(); i++) {
		if(attribute[i].header.type == INDEX_ROOT_TYPE) {
			return true;
		}
	}
	return false;
}

/*
 * 	findAttributeWithType searches through the list of attributes
 * 	for one that matches the input type
 * 
 * 	@param type: attribute type being searched for
 * 	@return: index in the attribute list (-1 for not found)
 * 
 */

int MFTEntry::findAttributeWithType(uint32_t type) {
	// search through the attribute list
	for(int attrIdx = 0; attrIdx < attribute.size(); attrIdx++) {
		// if found, return the index
		if(attribute[attrIdx].header.type == type) {
			return attrIdx;
		}
	}
	// not found, return error value
	return -1;
}

/*
 * 	Searches through the list of attributes and returns the first $STANDARD_INFORMATION
 * 	attribute if it exists.
 * 
 * 	@param : none
 * 	@return : StandardInformation object from the attribute list
 * 
 * 	assumption: error checking on returned object done by callee
 */

StandardInformation MFTEntry::getStandardInformation() {
	StandardInformation si;
	
	// get index in attribute list if exists
	int attrIdx = findAttributeWithType(STANDARD_INFORMATION_TYPE);
	if(attrIdx < 0) {
		//printf("Error, could not find Standard Information Attribute.\n");
		return si;
	}
	
	// exists, copy relevant data
	std::memcpy(&si.attrHeader, &attribute[attrIdx].header, sizeof(AttributeHeader));
	std::memcpy(&si.sInfoHeader, attribute[attrIdx].data, sizeof(StandardInformationHeader));
	
	return si;
}

/*
 * 	Searches through the list of attributes and returns the first $FILE_NAME attribute if it exists.
 * 
 * 	@param : none
 * 	@return : FileName object from the attribute list
 * 
 * 	assumption: error checking on returned object done by callee
 */

FileName MFTEntry::getFileName() {
	FileName fn;
	
	// get index in attribute list if exists
	int attrIdx = findAttributeWithType(FILE_NAME_TYPE);
	if(attrIdx < 0) {
		//printf("Error, could not find File Name Attribute.\n");
		return fn;
	}
	
	// exists, copy relevant data
	std::memcpy(&fn.attrHeader, &attribute[attrIdx].header, sizeof(AttributeHeader));
	std::memcpy(&fn.fNameHeader, attribute[attrIdx].data, sizeof(FileNameHeader));
	
	// get filename length and preallocate a string of that size
	uint8_t nameLength = fn.fNameHeader.filenameLength;
	fn.filename = std::string(nameLength, 'x');
	
	// get starting location of where the file name is stored in the data segment
	uint32_t charLoc = sizeof(FileNameHeader);
	
	// each character is stored as two bytes in the attribute data
	for(uint8_t strIdx = 0; strIdx < nameLength; strIdx++, charLoc+=2) {
		fn.filename[strIdx] = attribute[attrIdx].data[charLoc];
	}
	
	return fn;
}

/*
 * 	Searches through the list of attributes and returns the first $DATA attribute if it exists.
 * 
 * 	@param : none
 * 	@return : Data object from the attribute list
 * 
 * 	assumption: error checking on returned object done by callee
 */

Data MFTEntry::getData() {
	Data d;
	
	// get index in attribute list if exists
	int attrIdx = findAttributeWithType(DATA_TYPE);
	if(attrIdx < 0) {
		//printf("Error, could not find Data Attribute.\n");
		return d;
	}
	
	// exists, copy relevant data
	std::memcpy(&d.attrHeader, &attribute[attrIdx].header, sizeof(AttributeHeader));
	d.dataLength = attribute[attrIdx].dataLength;
	
	// if byte data exists, make a copy
	if(d.dataLength > 0) {
		d.data = new uint8_t[d.dataLength];
		std::memcpy(d.data, attribute[attrIdx].data, d.dataLength);
	}
	
	return d;
}

/*
 * 	Searches through the list of attributes and returns the first $INDEX_ROOT attribute if it exists.
 * 
 * 	@param : none
 * 	@return : IndexRoot object from the attribute list
 * 
 * 	assumption: error checking on returned object done by callee
 */

IndexRoot MFTEntry::getIndexRoot() {
	IndexRoot ir;
	
	// get index in attribute list if exists
	int attrIdx = findAttributeWithType(INDEX_ROOT_TYPE);
	if(attrIdx < 0) {
		//printf("Error, could not find Index Root Attribute.\n");
		return ir;
	}
	
	// exists, copy relevant data
	// copy attribute header
	std::memcpy(&ir.attrHeader, &attribute[attrIdx].header, sizeof(AttributeHeader));
	
	// copy root and node headers
	uint32_t byteLoc = 0;
	std::memcpy(&ir.idxRootHeader, &attribute[attrIdx].data[byteLoc], sizeof(IndexRootHeader));
	byteLoc += sizeof(IndexRootHeader);
	std::memcpy(&ir.idxNodeHeader, &attribute[attrIdx].data[byteLoc], sizeof(IndexNodeHeader));
	byteLoc += sizeof(IndexNodeHeader);
	
	// get list of index entries
	ir.idxEntry = getIndexEntryListFromBuffer(&attribute[attrIdx].data[byteLoc]);
	
	return ir;
}

/*
 * 	Searches through the list of attributes and returns the first $INDEX_ALLOCATION attribute if it exists.
 * 
 * 	@param : none
 * 	@return : IndexAllocation object from the attribute list
 * 
 * 	assumption: error checking on returned object done by callee
 */

IndexAllocation MFTEntry::getIndexAllocation(uint8_t *buffer, uint32_t bufferLen) {
	IndexAllocation ia;
  
	// get index in attribute list if exists
	int attrIdx = findAttributeWithType(INDEX_ALLOCATION_TYPE);
	if(attrIdx < 0) {
		//printf("Error, could not find Index Allocation Attribute.\n");
		return ia;
	}
	
	// exists, copy relevant data
	// copy attribute header
	std::memcpy(&ia.attrHeader, &attribute[attrIdx].header, sizeof(AttributeHeader));
	
	// parse the input buffer for blocks of index records
	for(uint32_t currByteLoc = 0; currByteLoc < bufferLen; currByteLoc += INDEX_RECORD_SIZE) {
		IndexRecord ir;
		
		// copy record and node headers
		std::memcpy(&ir.iRecHeader, &buffer[currByteLoc], sizeof(IndexRecordHeader));
		std::memcpy(&ir.iNodeHeader, &buffer[currByteLoc+sizeof(IndexRecordHeader)], sizeof(IndexNodeHeader));
		
		// get list of index entries
		uint32_t listStart = currByteLoc + ir.iNodeHeader.entryOffset + 0x18;
		ir.idxEntry = getIndexEntryListFromBuffer(&buffer[listStart]);
		ia.idxRecord.push_back(ir);
	}
	
	return ia;
}

/*
 * 	Searches through the list of attributes and returns the first $BITMAP attribute if it exists.
 * 
 * 	@param : none
 * 	@return : Bitmap object from the attribute list
 * 
 * 	assumption: error checking on returned object done by callee
 */

Bitmap MFTEntry::getBitmap() {
	Bitmap b;
	
	// get index in attribute list if exists
	int attrIdx = findAttributeWithType(BITMAP_TYPE);
	if(attrIdx < 0) {
		//printf("Error, could not find Bitmap Attribute.\n");
		return b;
	}
	
	// exists, copy attribute header
	std::memcpy(&b.attrHeader, &attribute[attrIdx].header, sizeof(AttributeHeader));
	
	// copy bitfield data
	b.bitfieldLength = attribute[attrIdx].dataLength;
	
	if(b.bitfieldLength > 0) {
		b.bitfield = new uint8_t[b.bitfieldLength];
		std::memcpy(b.bitfield, attribute[attrIdx].data, b.bitfieldLength);
	}
	
	return b;
}

/*
 * 	getIndexEntryListFromBuffer takes in binary data (buffer) as input and parses it
 * 	into a list of index entries.  This is used with IndexRoot and IndexRecord
 * 
 * 	@param buffer : pointer to binary data to parse
 * 	@return : list of IndexEntry
 * 
 */

std::vector<IndexEntry> MFTEntry::getIndexEntryListFromBuffer(uint8_t *buffer) {
	std::vector<IndexEntry> ies;
	uint32_t currByteLoc = 0;
	
	while(true) {
		IndexEntry ie;
		
		// Copy IndexEntry header
		std::memcpy(&ie.ieHeader, &buffer[currByteLoc], sizeof(IndexEntryHeader));
		
		// if the entry length is zero, then done parsing
		if(ie.ieHeader.indexEntryLength == 0) {
			break;
		}
		
		// if the entry contains a data stream then parse
		if(ie.ieHeader.streamLength != 0) {
			// allocate space for stream and copy
			ie.stream = new uint8_t[ie.ieHeader.streamLength];
			std::memcpy(ie.stream, &buffer[currByteLoc+sizeof(IndexEntryHeader)], ie.ieHeader.streamLength);
			
			// redundant, but used to parse filename from stream
			// all (maybe?) streams are FileName objects
			FileName fn;
			std::memcpy(&ie.fn.fNameHeader, ie.stream, sizeof(FileNameHeader));
	
			// parse filename
			uint8_t nameLength = ie.fn.fNameHeader.filenameLength;
			ie.fn.filename = std::string(nameLength, 'x');
			for(int charIdx = 0; charIdx < nameLength; charIdx++) {
				ie.fn.filename[charIdx] = ie.stream[sizeof(FileNameHeader)+2*charIdx];
			}
		}
		
		// if index entry has a sub-node, copy
		if(isSubNodeFlagSet(ie.ieHeader.flags)) {
			uint32_t offset = currByteLoc+ie.ieHeader.indexEntryLength-sizeof(uint64_t);
			std::memcpy(&ie.subNodeVCN, &buffer[offset], sizeof(uint64_t));
		}
		
		// dont parsing IndexEntry, add to list and increment current byte location
		ies.push_back(ie);
		currByteLoc += ie.ieHeader.indexEntryLength;
	}
	return ies;
}

/*
 * 	getClusterRunList parses a data buffer. Cluster runs or data runs are defined as follows:
 * 
 * 	1st byte : [ offset size (O) | length size (L) ] 
 * 	next L bytes : cluster run length
 * 	next O bytes : cluster run offset (relative to the previous offset)
 * 
 * 	@param buffer : byte data to parse
 * 	@return : list of ClusterRun
 * 
 */

std::vector<ClusterRun> MFTEntry::getClusterRunList(uint8_t *buffer) {
	std::vector<ClusterRun> run;
	uint8_t header, offsetSize, lengthSize;
	uint32_t currByteLoc = 0;
	uint64_t prevOffset = 0;
	
	// keep parsing until the CLUSTER_RUN_END marker (0).
	while(buffer[currByteLoc] != CLUSTER_RUN_END) {
		header = buffer[currByteLoc];
		currByteLoc += CLUSTER_RUN_HEADER_SIZE;
		
		// get the top four bits
		offsetSize = header >> 4;
		// get the bottom four bits
		lengthSize = header & 0x0F;
		
		ClusterRun r;
		
		// parse the cluster run length
		std::memcpy(&r.length, &buffer[currByteLoc], lengthSize);
		currByteLoc += lengthSize;
		
		// parse the cluster run offset and add the previous relative offset
		std::memcpy(&r.offset, &buffer[currByteLoc], offsetSize);
		r.offset += prevOffset;
		currByteLoc += offsetSize;
		
		// update previous offset with this one before continuing
		prevOffset = r.offset;
		
		// add cluster run to list
		run.push_back(r);
	}
	
	return run;
}

/*
 * 	getIndexAllocationClusterRunList gets the cluster runs for an IndexAllocation 
 * 	object if it exists.  This attribute is always non-resident.
 */

std::vector<ClusterRun> MFTEntry::getIndexAllocationClusterRunList() {
	std::vector<ClusterRun> run;
	int attrIdx = findAttributeWithType(INDEX_ALLOCATION_TYPE);
	
	// if it doesn't exist, return empty list
	if(attrIdx < 0) {
		return run;
	}
	
	// parse the cluster run list and return
	run = getClusterRunList(attribute[attrIdx].data);
	return run;
}

/*
 * 	getDataClusterRunList return a cluster run list for data if it is non-resident
 */

std::vector<ClusterRun> MFTEntry::getDataClusterRunList() {
	std::vector<ClusterRun> run;
	Data d = getData();
	
	// if there isn't data or the non-resident flag isn't set
	// return empty list
	if(d.dataLength <= 0 || !d.attrHeader.nonResidentFlag) {
		return run;
	}
	
	// parse cluster runs and return
	run = getClusterRunList(d.data);
	return run;
}

/*
 * 	getBitmapClusterRunList returns a list of cluster runs if it is non-resident
 */

std::vector<ClusterRun> MFTEntry::getBitmapClusterRunList() {
	std::vector<ClusterRun> run;
	Bitmap b = getBitmap();
	
	// if there is not bitfield data or isn't non-resident, return empty list
	if(b.bitfieldLength <= 0 || !b.attrHeader.nonResidentFlag) {
		return run;
	}
  
	run = getClusterRunList(b.bitfield);
	return run;
}

// verbose print function to output entire MFT entry

void MFTEntry::print() {
	recHeader.print();
	
	if(recHeader.updateSeqSize > 0) {
		printf("-------------------------------------------------------\n");
		printf("|                  UPDATE SEQUENCE                    |\n");
		printf("-------------------------------------------------------\n");
		printf("Update sequence number: 0x%04X\n", updateSeqNum);
	
		for(int i = 0; i < recHeader.updateSeqSize; i+=16) {
			for(int j = i; j < recHeader.updateSeqSize && j < i+16; j++) {
				if(j%4 == 0) {
					printf(" ");
				}
				printf("%02X ", updateSeqArray[j]);
			}
			printf("\n");
		}
	}
	
	for(int aIdx = 0; aIdx < attribute.size(); aIdx++) {
		switch(attribute[aIdx].header.type) {
			case STANDARD_INFORMATION_TYPE : {
				getStandardInformation().print();
				break;
			} case FILE_NAME_TYPE : {
				getFileName().print();
				break;
			} case DATA_TYPE : {
				getData().print();
				break;
			} case INDEX_ROOT_TYPE : {
				getIndexRoot().print();
				break;
			} case INDEX_ALLOCATION_TYPE : {
				break;
			} case BITMAP_TYPE : {
				getBitmap().print();
				break;
			} default : {
			  break;
			}
		}
	}
}
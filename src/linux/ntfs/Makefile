#include the make.h
include ./make.h

MODULE_OBJECT_LIST =	$(MODULE_OBJ_DIR)/DiskReader.o \
						$(MODULE_OBJ_DIR)/MFTEntry.o \
						$(MODULE_OBJ_DIR)/NTFSReader.o \
						$(MODULE_OBJ_DIR)/main.o \

MODULE_FINAL_EXE = $(MODULE_EXE_DIR)/ntfs
#MODULE_STATIC_LIBS_DIR = -L$(BASE_DIR)/common/lib/

MODULE_LIBS_LIST = -lm

#MODULE_LIBS_LIST = -lcmn \
					-lm \
					-lrt \
					-lpthread

$(MODULE_FINAL_EXE): $(MODULE_OBJECT_LIST)
	$(CPP) -g -O0 $(MODULE_OBJECT_LIST) $(MODULE_STATIC_LIBS_DIR) $(MODULE_LIBS_LIST) -o $(MODULE_FINAL_EXE)

$(MODULE_OBJ_DIR)/DiskReader.o: $(MODULE_SRC_DIR)/DiskReader.cpp
	$(CPP) $(CPP_FLAGS) $(MODULE_FINAL_INC_DIRS) $(MODULE_SRC_DIR)/DiskReader.cpp -o $(MODULE_OBJ_DIR)/DiskReader.o

$(MODULE_OBJ_DIR)/MFTEntry.o: $(MODULE_SRC_DIR)/MFTEntry.cpp
	$(CPP) $(CPP_FLAGS) $(MODULE_FINAL_INC_DIRS) $(MODULE_SRC_DIR)/MFTEntry.cpp -o $(MODULE_OBJ_DIR)/MFTEntry.o

$(MODULE_OBJ_DIR)/NTFSReader.o: $(MODULE_SRC_DIR)/NTFSReader.cpp
	$(CPP) $(CPP_FLAGS) $(MODULE_FINAL_INC_DIRS) $(MODULE_SRC_DIR)/NTFSReader.cpp -o $(MODULE_OBJ_DIR)/NTFSReader.o

$(MODULE_OBJ_DIR)/main.o: $(MODULE_SRC_DIR)/main.cpp
	$(CPP) $(CPP_FLAGS) $(MODULE_FINAL_INC_DIRS) $(MODULE_SRC_DIR)/main.cpp -o $(MODULE_OBJ_DIR)/main.o

clean:
	rm -f $(MODULE_OBJ_DIR)/*
	rm -f $(MODULE_EXE_DIR)/*						   

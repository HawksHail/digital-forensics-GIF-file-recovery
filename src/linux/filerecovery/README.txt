The syntax for running this program is

./filerecovery headerLength headerOffset disk [hex sequence of header] EndOfFile

For example in the case of JPEG file type,
./filerecovery 2 0 /dev/sdb ff d8 FFD9

headerLength  = 2
headerOffset = 0
disk = /dev/sdb
[header] = ff d8
EndOfFile = FFD9



For quict test

testscan.c is a ext3 file recovery utility for demo purpose, it will only scan the 
first block group for deleted files so that the result will show up much faster.

Syntax:
  ./testscan DISK OPTION

Examples usage: 
1. To find the header of bmp files on disk /dev/sdb3
  ./testscan /dev/sdb3 header bmp

2. Suppose the file header is in block 1058, to find the indirect pointer block
that contains block numbers belonging to the file
  ./testscan /dev/sdb3 ind 1058

3. To construct the file by reading the block numbers stored in block 582
  ./testscan /dev/sdb3 construct 582


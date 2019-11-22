As already discussed in the class. The final part of the project is putting all the pieces together to recover a file.

1. search for the first block of your file type. Your function will return all the blocks that match file signature. You can use an array to store the first blocks.

2. Find the indirect blocks in your partition.  From this list of indirect blocks, find the 1st level indirect block for your file, and the 2nd level indirect block.

3. calculate the size of the file.

4. get an idle inode.

5. fill all the information in the inode - size, blocks, time stamps etc ...

6. link this file inode to a directory inode - root directory works

Your demo will be:

1. create a new file. (not already in your partition)

2. get the inode number of this file, inode number of the root directory of this file

3. using the inode tool, show the block numbers of this file.

4. delete this file.

5. use your program to recover the file.

6. do an ls to show the file

7. use inode analysis tool to show the new file block numbers.

8. FINALLY .... we want to see the file (eg. if it is a picture - we want to see the picture)

Submit a readme.txt file.  The file should list the contribution of each member. All members must agree with each person's contribution.

If you are a single person group, then you dont need this.

submit the code.

For grading .. I will give some consideration to how modular your code is.

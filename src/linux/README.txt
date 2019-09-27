Compilation Pre-requisites:
==========================
For compiling all the code, you should firstly download the uthash into linux folder.
So you should run "git clone https://github.com/troydhanson/uthash.git" in the linux folder

For compiling the GUI version, JAVA needs to be installed on the device, and JAVA_HOME environment variable needs to be set.
Refer JAVA installation instructions for details on how to set the environment variable.

Compilation Instructions:
========================
    make all    -    compiles everything
    make inode  -    Inode module
    make gbd    -    Group Descriptor module
    make mbr    -    MBR module
    make superblock -   Super Block module
    make clean  -   removes executables and object files

    After successful compilation the executables would be placed in the bin/
    directory.

To enable debug prints:
======================
    uncomment DEBUG_FLAGS = -DTRACE_WANTED in make.h

Generate Doxygen:
================
    Doxygen developer guid is currently provided only for 'inode' module.
See 'documentation.html' from 'inode/docs' directory for developer guide.
To generate new documentation, go to inode directory and run 'make doxy'.

Coding Guidelines:
=================
    Refer the document 'CodingStandards.docx' for rules and conventions to be
    followed for coding.

Running the code:
================
    User needs to be a superuser to run the executables placed in the bin directory.

Running the MainWindowTest (JUnit)
================
Initially this whole class is commented.
Once you run the build successfully, please uncomment this.
To run it successfully you need to have the following dependency JAR's installed in $(PROJECT_DIR/lhe inux/gui/lib) 'lib' directory. If its not present please create one.

The below JAR's can be easily found in online public JAR library available to everyone in the Internet. Once you install these JAR's please configure your IDE to make it avaliable during 'Compile' time.

1) annotations-java8.jar
2) fest-assert-1.4.jar
3) fest-util-1.1.6.jar
4) hamcrest-core-1.3.jar
5) junit-4.12.jar
6) mockito-all-1.9.5.jar

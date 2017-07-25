# File-System-Simulator
This program emulates a simple file system.  It came about as the Final Project for the Operating systems class at the Univeristy of Wyoming taught by Jim Ward.  It is written for Linux based operating Systems.  In order to play around with it simply download the files and run the makefile, this will run all of the drivers sequentially.  when it is done simply open the DISK file that the program creates, using your favorite hex viewer and think about how cool your computer is.  Any Questions/Comments/Concerns/Bugs/Rants/Praise/Insults/etc. feel free to email me!

**Warning: This was written to be able to work on very old Scientific Linux Systems it will work on any Linux based machine but it currently has issues on Mac, have not tested on Ubuntu and the like yet (I'll fix the Mac issues soon).**

**NOTE:  You will need a Hexviewer to read the DISK file**

## **Update: Mac issues should be solved**





## File Breakdown
  
  **bitVector.h & bitVector.cpp:** These two files implement a Bit Vector class which is used to keep track of blocks on the DISK that have been allocated (or deallocated as the case may be).
  
  **client.h:** This class is simply a representation of a client who wishes to use the file system, this allows us to simulate multiple clients accessing the same file system.
  
  **disk.h & disk.cpp:**  These two files are in charge of creating the DISK file (the emulation of a hard disk) they also handle reading and writing from/to the disk.
  
  **diskmanager.h & diskmanager.cpp:**  These two files make up the DiskManager class.  The DiskManager is in charge of reading and writing from/to the disk just as the disk class is however the DiskManager must keep track of which partition the client is writing to.  The DiskManager makes use of the Disk class in order to read/write but it offsets the the block number from which it starts reading and writing based on which partition the block resides in.  It is also in charge to reading the SuperBlock in order to figure out if it needs to create a new set of partitions or if a definitions for partitions already exist. If they do then it simply uses the existing definitions.
  
  **DiskManagerHelper.h:**  These are just a bunch of helper functions that the DiskManager class makes use of.
  
  **partitionmanager.h & partitionmanager.cpp:**  These files make up the PartitionManager class.  The Partition Manager is in charge of the allocation or deallocation of blocks (which also includes updating the bitVector with the new info for that particular partition) as well as checking wether or not a partition is full, finding a particular file within a partition, initializing a new directory, checking if a directory is empty, printing out the bitVector in case we need to see it visually, and reading and writing from/to the DISK.  All read and write commands executed from the FileSystem class use the read/write methods in the PartitionManager class which in turn uses the read/write methods in the DiskManager class which uses the read/write methods from the disk class. Note that the FileSystem class should never access either the DiskManager or dick class methods directly.
  
  **GeneralHelperFunctions.cpp:** Just some more helper functions, these are used by various classes (mainly FileSystem class though).
  
  **PsudoRNG.c:** Used by FileSystem when generating a lock ID for a file, this way the ID's generated are just a little bit more secure than say, 1,2,3..etc.
  
  **filesystem.h & filesystem.cpp:**  These two files make up the monster class FileSystem this class does everything.  There are pleanty of comments in the code but as a swift overview the FileSystem class is in charge of: creating a new file system, opening a file, closing a file, read from a file, writing to a file, appending to a file, renaming a file, locking and unlocking a file, creating a file, deleting a file, creating a directory, deleting a directory, seeking to a location in a file two different ways based on a the flag passed (see the comments in the code), getting the attributes of a file (date created, extension etc.  I have only implemented creation date stamp and file extension but the setAttribute method could easily be modified to include more atributes such as which user created the file, date last modified, etc.)
  
  **drivers 1-6:**  Drivers 1 - 6 are in charge of testing the FileSystem class.  Driver1 tests the basic functionality such as write/read/open/close etc. for a single client (i.e. single partition), Driver2 does the same but for multi-partition, Driver3 tests the same things as driver1 but it throws in directories, Driver4 tests the same things as driver3 but for multiple partitions, Driver5 tests edge cases (incidentally this is were the program fails a few tests on Macs), Driver6 tests "added functionality" wich in this case is the set and get attributes function.

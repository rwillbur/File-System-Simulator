#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "GeneralHelperFunctions.cpp"
#include "PsudoRNG.c"
#include <time.h>
#include <cstdlib>
#include <string>
#include <ctype.h>
#include <string>

#define MAX_FILE_SIZE 1216
typedef unsigned int uint;
using namespace std;

FileSystem::FileSystem(DiskManager *dm, char fileSystemName)
{
	myDM = dm; //Disk Manager
	myfileSystemName = fileSystemName; // FileSys name
	myPM = new PartitionManager(myDM,myfileSystemName); // Partition Manager
	init_lfsrs(); // initalize random number gen
}

// This operation creates a new file whose name is pointed to by filename of
// size fnamelen characters. File names and directory names start with `/'
// character and consist of a sequence of alternating `/' and alphabet (`A' to `Z'
// and `a' to `z') characters ending with an alphabet character.
// Returns:
//   -1 if the file already exists
//   -2 if there is not enough disk space
//   -3 if invalid filename
//   -4 if the file cannot be created for some other reason
//   0 if the file is created successfully.
int FileSystem::createFile(char *filename, int fnameLen)
{
	/*  initial data integrity checks  */
	if(filename == NULL || fnameLen <= 0){return -4;} //check that data passed is good
	if(myPM->isFull() == 1){return -2;} // make sure there is disk space
	if(!nameCheck(filename, fnameLen)){return -3;}  // check if name is legal

	/*  variables  */
	char fname = filename[fnameLen-1];  //filename
	int location = 0;  //  file inode location
	char currentDir[64]; // current directory data
	char continueAdrs[4]; // address at end of directory which signals were the directory continues if block is full
	int pointer = 0;  // integer version of continueAdrs
	int directoryBlock =  travelDirectories(filename, fnameLen); //home directory
  int temp = 0; // for error checks
	clean(directoryBlock); // clean up a trailing directory

  /*  check if directory continues. If it does revise the directoryBlk accordingly  */
  if((myPM->readDiskBlock(directoryBlock, currentDir)) < 0){return -4;} // read data
  int n = 0;
  for(int i = 60; i < 64; i++){continueAdrs[n] = currentDir[i]; n++;}
  pointer = CharToInt2(continueAdrs);
  if(pointer != 0 && isFullBlk(currentDir)){directoryBlock = pointer;}
  if((myPM->readDiskBlock(directoryBlock, currentDir)) < 0){return -4;} // read data


  /*  Check if file name already exists.
   *  If it does then:
   *  A: if it's a directory return -4
   *  B: if it's a file return -1
   */
  if((temp = myPM->findFile(fname, directoryBlock)) > 0)
  {
    char temp2[64];
    myPM->readDiskBlock(temp, temp2);
    int inodeL = getLocation(temp2, fname);
    myPM->readDiskBlock(inodeL, temp2);
    if(isFile(temp2)){return -1;}
    else{return -4;}
  } 

  /*  If the current directory is NOT full simply write to it  */
  if(!isFullBlk(currentDir))
  {
    int location; // location of new fileInode
    if((location = writeFileEntry(directoryBlock, fname)) < 0){return -4;} //write the new file entry
    if(writeFileInode(location, fname) < 0){return -4;} // write the new file inode
  }
  else
  {
    /*  The directory block you are trying to write to is full:
     *  1. Create a new "fresh" directory at the next open space you can find
     *  2. Set the current directories "continuation of dir" pointer (i.e. last 4 bytes) to the new blknum you got from step 1
     *  3. Write the file to the new directory
     */
    char* free = new char[4];
    int newDir = myPM->getFreeDiskBlock(); if(location < 0){return -2;} newDir += 1;
    int k = 0;
    free = IntToChar2(newDir);
    for(int i = 60; i < 64; i++){currentDir[i] = free[k]; k++;}
    if(myPM->writeDiskBlock(directoryBlock, currentDir) < 0){return -4;}
    if(myPM->initDirectory(newDir) < 0){return -4;}

    /*  write file  */
    int location;
    if((location = writeFileEntry(newDir, fname)) < 0){return -4;} //write the new file entry
    if(writeFileInode(location, fname) < 0){return -4;} // write the new file inode

  }
  return 0;
}

// This operation creates a new directory whose name is pointed to by dirname.
// Returns:
//   -1 if the directory already exists
//   -2 if there is not enough disk space
//   -3 if invalid directory name
//   -4 if the directory cannot be created for some other reason
//   0 if the directory is created successfully.
int FileSystem::createDirectory(char *dirname, int dnameLen)
{
	/*  initial data integrity checks  */
	if(dirname == NULL || dnameLen <= 0){return -4;} // check that data passed is good
  if(!nameCheck(dirname, dnameLen)){return -3;}


	/*  variables  */
	char dname = dirname[dnameLen-1];  //directory name
	int location = 0;  //  file inode location
	char currentDir[64]; // current directory data
	char continueAdrs[4]; // address at end of directory which signals were the directory continues if block is full
	int pointer;  // integer version of continueAdrs
  int temp; // used for error checks
	int directoryBlock = travelDirectories(dirname, dnameLen); //home direc
	clean(directoryBlock); // clean up trailing directory


  /*  check if directory continues. If it does revise the directoryBlk accordingly  */
  if((myPM->readDiskBlock(directoryBlock, currentDir)) < 0){return -4;} // read data
  int n = 0;
  for(int i = 60; i < 64; i++){continueAdrs[n] = currentDir[i]; n++;}
  pointer = CharToInt2(continueAdrs);
  if(pointer != 0 && isFullBlk(currentDir)){directoryBlock = pointer;}
  if((myPM->readDiskBlock(directoryBlock, currentDir)) < 0){return -4;} // read data


  /*  Check if file name already exists.
   *  If it does then:
   *  A: if it's a directory return -4
   *  B: if it's a file return -1
   */
  if((temp = myPM->findFile(dname, directoryBlock)) > 0)
  {
    char temp2[64];
    myPM->readDiskBlock(temp, temp2);
    int inodeL = getLocation(temp2, dname);
    myPM->readDiskBlock(inodeL, temp2);
    if(!isFile(temp2)){return -1;}
    else{return -4;}
  } 

  /*  If the current directory is NOT full simply write to it  */
  if(!isFullBlk(currentDir))
  {
    int location;
    if((location = writeDirectoryEntry(directoryBlock, dname)) < 0){return -4;} //write the new file entry
    myPM->initDirectory(location);
  }
  else
  {
    /*  The directory block you are trying to write to is full:
     *  1. Create a new "fresh" directory at the next open space you can find
     *  2. Set the current directory's "continuation of dir" pointer (i.e. last 4 bytes) ,
     *     to the new blknum you got from step 1
     *  3. Write new directory to disk
     */
    char* free = new char[4];
    int newDir = myPM->getFreeDiskBlock(); if(location < 0){return -2;} newDir += 1;
    int k = 0;
    free = IntToChar2(newDir);
    for(int i = 60; i < 64; i++){currentDir[i] = free[k]; k++;}
    if(myPM->writeDiskBlock(directoryBlock, currentDir) < 0){return -4;}
    if(myPM->initDirectory(newDir) < 0){return -4;}

    /*  write directory to disk  */
    int location;
    if((location = writeDirectoryEntry(newDir, dname)) < 0){return -4;} //write the new file entry
    if(myPM->initDirectory(location) < 0){return -4;}

  }
  return 0;
}

// This operation locks a file.
// A file cannot be locked if
//   (1) it doesn't exist
//   (2) it is already locked
//   (3) it is currently opened.
// Returns:
//   greater than 0 (lock id), if the file is successfully locked
//   -1 if the file is already locked
//   -2 if the file does not exis
//   -3 if it is currently opened
//   -4 if the file cannot be locked for any other reason.
// A note, once a file is locked, it may only be opened with the lock id and the
// file cannot be deleted or renamed until the file is unlocked.
int FileSystem::lockFile(char *filename, int fnameLen)
{
	/*  initial data integrity checks  */
	if(filename == NULL || fnameLen <= 0){return -4;}  // check that data passed is good

	/*  variables  */
	char fname = filename[fnameLen-1];  //filename
	Node* n = getNode(filename); // filetable entry Node
	int directoryLoc = travelDirectories(filename, fnameLen); // home dir 

  // get blknum where file is (in case directory extends beyond 1 blk)
  directoryLoc = myPM->findFile(fname, directoryLoc); 

	/*  error checks  */
	if(myPM->findFile(fname, directoryLoc) <= 0){return -2;}  // find if file exists
	if(!isFile2(directoryLoc, fname)){return -4;} // make sure not trying to open directory
	if(getLock(filename) == nullptr){ newLock(filename);} // if file has no entry in lock table make one
	if(getLock(filename)->isLocked){return -1;}  // if file is locked return
	if(n != nullptr && n->isOpen){return -3;}  // if file has a filetable entry && if file is open return

	/*  lock the file  */
	int rand = get_random();  //generates random number (generates same sequence of numbers every time)
	getLock(filename)->isLocked = true;  //  set file to locked
	getLock(filename)->setLockID(rand);  // set lock ID

	/*  if success return lock ID  */
	return rand;
}

// This operation unlocks a file. The lockid is the lock id returned by the
// LockFile function when the file was locked.
// Returns:
//   0 if successful
//   -1 if lockid is invalid
//   -2 for any other reason.
int FileSystem::unlockFile(char *filename, int fnameLen, int lockId)
{
	/*  initial data integrity checks  */
	if(filename == NULL || fnameLen <= 0){return -2;}  // check that data passed is good

	/*  error checks  */
	if(getLock(filename) == nullptr){return -2;}  // check if entry in lock table exists
	if(getLock(filename)->getLockID() != lockId){return -1;} // check if lockId equals the real lockID

	/*  unlock file  */
	getLock(filename)->isLocked = false;

	/*  if success return 0  */
	return 0;
}

// This operation deletes the file whose name is pointed to by filename. A file
// that is currently in use (opened or locked by a client) cannot be deleted.
// Returns:
//   -1 if the file does not exist
//   -2 if the file is in use or locked
//   -3 if the file cannot be deleted for any other reason
//   0 if the file is deleted successfully.
int FileSystem::deleteFile(char *filename, int fnameLen)
{
	/*  initial data checks  */
	if(filename == nullptr || fnameLen <= 0){return -3;}
	if(nameCheck(filename, fnameLen) == false){return -3;}

	/*  variables  */
	char fname = filename[fnameLen-1];
	char dirInode[64];
	char fInode[64];
	int fInodeLoc;
	std::vector<int> directAdrss;
	std::vector<int> indirectAdrss;
	char indirectBlock[64];
	int indirectLoc;
	Node* n = getNode(filename);
	Lock* l  = getLock(filename);

	/*  traverse directories  */
	int dirBlockNum = travelDirectories(filename, fnameLen);
  dirBlockNum = myPM->findFile(fname, dirBlockNum);
  clean(dirBlockNum);
  if(dirBlockNum <= 0){return -1;}

	/*  error checks  */
	if(n != nullptr && n->isOpen){return -2;}
	if(l != nullptr && l->isLocked){return -2;}


	/*  get nescessary data + delete file  */
	if(myPM->readDiskBlock(dirBlockNum, dirInode) < 0){return -3;} // read info from current directory
	fInodeLoc = getLocation(dirInode, fname); // location on disk of target file's Inode
	if(myPM->readDiskBlock(fInodeLoc, fInode) < 0){return -3;} // get file Inode
	if(!isFile(fInode)){return -3;} // if file isnot a file but a directry return

  // get all 3 direct addresses as well as the single indirect address
	directAdrss = getDataLoc(fInode); 

  // if the indirect address is NOT '0' (i.e. the indirect address exists) see getDataLoc() def
	if(directAdrss.size() == 4)
	{
		indirectLoc = directAdrss[3]; // get address of indirect block
		if(myPM->readDiskBlock(indirectLoc, indirectBlock) < 0){return -3;}
		indirectAdrss = getIndirectLoc(indirectBlock); 
	}


  /*  delete data  */
  if(indirectAdrss.size() > 0) //if the indirect block has data 
  {
    for(uint i = 0; i < indirectAdrss.size(); i++)
    {
      char temp[64];
      clearBlock(temp); // clear block (i.e. write all 'c')
      if(myPM->writeDiskBlock(indirectAdrss[i], temp) < 0){return -3;} //update disk with cleared block
      if(myPM->returnDiskBlock(indirectAdrss[i]-1) < 0){return -3;} // update bitVector to show this block as open
    }
  }
	if(directAdrss.size() > 0) //if the file has data somewhere
	{
		for(uint i = 0; i < directAdrss.size(); i++)
		{
			char temp[64];
			clearBlock(temp); // clear block (i.e. write all 'c')
			if(myPM->writeDiskBlock(directAdrss[i], temp) < 0){return -3;} //update disk with cleared block
			if(myPM->returnDiskBlock(directAdrss[i]-1) < 0){return -3;} // update bitVector to show this block as open
		}
	}

  /*  delete the file Inode and the file's directory entry  */
  clearBlock(fInode); // clear file Inode
  if(myPM->writeDiskBlock(fInodeLoc, fInode) < 0){return -3;} // update disk with new fileInode
  if(myPM->returnDiskBlock(fInodeLoc-1) < 0){return -3;} // set bitVector at file Inode location to 0 (i.e. open)
  deleteEntry(dirInode, fname); // delete target file's entry from current directory
  if(myPM->writeDiskBlock(dirBlockNum, dirInode) < 0){return -3;} // write updated directory to disk
	return 0;
}

// This operation deletes the directory whose name is pointed to by dirname. Only
// an empty directory can be deleted.
// Returns:
//   -1 if the directory does not exist
//   -2 if the directory is not empty
//   -3 if the directory cannot be deleted for any other reason
//   0 if the directory is deleted successfully.
int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{
	/*  initial data checks  */
	if(dirname == nullptr || dnameLen <= 0){return -3;}


	/*  variables  */
	char dname = dirname[dnameLen-1]; // name of target directory
	char homeDirectoryInode[64]; // buffer for data of directory were "target dir" resides
	int targetDirBlknm;  //  location of target directory
	char targetDirInode[64]; // buffer for target directory data

	/* traverse directories  */
	int homeDirBlockNum = travelDirectories(dirname, dnameLen);
  homeDirBlockNum = myPM->findFile(dname, homeDirBlockNum);
  clean(homeDirBlockNum);
  if(homeDirBlockNum <= 0){return -1;}

	if(myPM->readDiskBlock(homeDirBlockNum, homeDirectoryInode) < 0){return -3;}
	targetDirBlknm = getLocation(homeDirectoryInode, dname); // get target directory location

	if(myPM->readDiskBlock(targetDirBlknm, targetDirInode) < 0){return -3;} // get target directory data
	if(isFile(targetDirInode)){return -1;}// if target directory is infact a fileInode; quit


	/* check that directory does not continue to another block
	 * a directory could easily have all of its files deleted in one location
	 * but still have entries in another location (i.e. its continuation)
   */
  if(myPM->isEmptyDir(targetDirBlknm) == 1){return -2;}
	
  /*  delete directory inode and remove its entry from the home dir  */
	clearBlock(targetDirInode); // clear the directory block (i.e. set every byte to 'c')
	if(myPM->writeDiskBlock(targetDirBlknm, targetDirInode) < 0){return -3;} // write updated target directory to disk
	if(myPM->returnDiskBlock(targetDirBlknm-1) < 0){return -3;} // reset bitVector value to '0' (free) at the target dir's location

	deleteEntry(homeDirectoryInode, dname); // delete target directory entry from home directory
	if(myPM->writeDiskBlock(homeDirBlockNum, homeDirectoryInode) < 0){return -3;} // write updated home directory to disk

	return 0;
}

// This operation opens a file whose name is pointed to by filename.
//   If mode ='r', the file is opened for read only
//   If mode = 'w', the file is opened for write only
//   If mode = 'm', the file is opened for read and write.
// An existing file cannot be opened if
//   (1) the file is locked and lock_id doesn'tmatch with lock_id
//       returned by the lockFile function when the file was locked
//   (2) the file is not locked and lock id != -1.
// Returns:
//   -1 if the file does not exist
//   -2 if mode is invalid
//   -3 if the file cannot be opened because of locking restrictions
//   -4 for any other reason, 
//	  a unique positive integer (file descriptor) if the file is opened successfully.
//
// If the file is opened successfully, a rw pointer (read-write pointer) is
// associated with this file descriptor. This rw pointer is used by some of
// the operations described later for determining the access point in a file.
// The initial value of an rw pointer is 0 (beginning of the file).
int FileSystem::openFile(char *filename, int fnameLen, char mode, int lockId)
{
	/* data integrity checks  */
	if(filename == nullptr || fnameLen <= 0){return -4;}
	if(mode != 'r' && mode != 'w' && mode != 'm'){return -2;} // check mode validity
	if(!nameCheck(filename, fnameLen)){return -1;}

	/*  variables  */
	char fname = filename[fnameLen-1];
	Lock* lock = getLock(filename);
	int floc;
	char buffer[64];
	char inode[64];

	/*  more error checks  */
	if(lock != nullptr && lock->isLocked && lockId != lock->getLockID()){return -3;} // lock restriction check
	if(lock != nullptr && !lock->isLocked && lockId != -1){return -3;} // lock restriction check (file already open?)

	/*  traverse directories  */
	int dirBlknum = travelDirectories(filename, fnameLen);

	/*  error checks + data gathering  */
	if((floc = myPM->findFile(fname, dirBlknum)) <= 0){return -1;} // find if file exists

	// get file inode location
	if(myPM->readDiskBlock(floc, buffer) < 0){return -4;}
	floc = getLocation(buffer, fname);

	//  check if file is actually a file or if it's a directory
	if(myPM->readDiskBlock(floc, inode) < 0){return -4;}
	if(!isFile(inode)){return -1;}

	/*  create fileTable entry & assign rw pointer  */
	int desc = newNode(filename, mode, fnameLen);

	/*  if a file is open multiple times syncronize the "first data block" across all instances in the table
	 *  if this is not done whenever a new entry in the table is made for the file:
	 *  1. it will be set to < 0, 0 >
	 *  2. this will mean that if the entry is picked to write to, by virtue of how writeFile() is written
	 *			it will overwrite the file's 1st direct address block this is NOT good
	 */
	std::vector<int> dAdress = getDataLoc(inode);
	for(uint i = 0; i < fileTable.size(); i++)
	{
		if(fileTable[i].name == filename && fileTable[i].rwpointer.first == 0)
		{
			if(dAdress.size() > 0)
			{
				fileTable[i].rwpointer.first = dAdress[0];
        fileTable[i].rwpointer.second = 0;
      }
    }
  }
  return desc;
}

// This operation closes the file with file descriptor filedesc.
// Returns:
//   -1 if the file descriptor is invalid
//   -2 for any other reason
//   0 if successful.
int FileSystem::closeFile(int fileDesc)
{
	if(fileTable.size() <= 0){return -1;}
	for(uint i = 0; i < fileTable.size(); i++)
	{
		if(fileTable[i].fdesc == fileDesc)
		{
      /*  erase the entry from the table  */
			fileTable.erase(fileTable.begin() + i);
			return 0;
		}
	}
	return -1;
}

// Perform read operations on a file whose file descriptor is fileDesc.
//   len is the number of bytes to be read from into the buffer pointed to by data.
// Returns:
//   -1 if the file descriptor is invalid
//   -2 if length is negative
//   -3 if the operation is not permitted
//   Number of bytes read if successful
// The read operation operates from the byte pointed to by the rw pointer.
// The read operation may read less number of bytes than len if end of file
// is reached earlier. After the read is done the rw pointer is updated to point
// to the byte following the last byte read.
int FileSystem::readFile(int fileDesc, char *data, int len)
{
  /*  initial data checks  */
  int fileIndex;  // index of file in file table
  if ((fileIndex = fileDescExists(fileDesc)) == -1) { return -1; }
  if (len < 0) { return -2; }
  if (fileTable[fileIndex].mode != 'r' && fileTable[fileIndex].mode != 'm') { return -3; }

  /*  get file name  */
  int nameLen = fileTable[fileIndex].nameSize; // size of file path
  char* filename = fileTable[fileIndex].name; // file path
  char fname = filename[nameLen - 1];  // filename
  
  /*  get directory where file resides  */
  int dirLoc = travelDirectories(filename, nameLen);
  dirLoc = myPM->findFile(fname, dirLoc);
  char directory[64];
  if (myPM->readDiskBlock(dirLoc, directory) < 0) { return -3; }

  /*  get file inode and other info  */
  int inodeLoc = getLocation(directory, fname); // location of file Inode
  char inode[64]; // file inode data
  char indirect[64]; // file's "indirect block" data
  int indirectLoc; // file's "indirect block" location
  if (myPM->readDiskBlock(inodeLoc, inode) < 0) { return -3; }
  int fileSize = getFileSize(inode);  // file size

  /*  set up address vectors  */
  std::vector<int> indirectBlks; // the file's indirect-blk locations
  std::vector<int> directBlks = getDataLoc(inode); // the file's direct-block locations as integers
  indirectLoc = getIndirectBlknum(inode); // get file's "indirect block" location

  /*  only fill the indirect block vector if the file has an indirect block  */
  if (indirectLoc != 0) { if (myPM->readDiskBlock(indirectLoc, indirect) < 0) { return -3; } } 
  if (indirectLoc != 0) { indirectBlks = getIndirectLoc(indirect); } 

  /*  current read/write pointer  */
  int currentBlk = fileTable[fileIndex].rwpointer.first; // rwpointer "block we are writing to" (3->partitionSize)
  int indexInBlk = fileTable[fileIndex].rwpointer.second; // rwpointer location within the "block we are writing to" (0->64)
  int readPos = fileTable[fileIndex].readPos;

  /*  consolidated all addresses excpet for the location of the inode  */
  std::vector<int> allAddresses;
  if(directBlks.size() > 0){for(uint i = 0; i < directBlks.size(); i++){if(i != 3){allAddresses.push_back((directBlks[i]));}}}
  if(indirectBlks.size() > 0){ for(uint i = 0; i < indirectBlks.size(); i++){allAddresses.push_back((indirectBlks[i]));} }

  // if the file is "fresh" return 0
  if(currentBlk == 0){return 0;} 

  // if the file has nothing written don't read
  if(fileSize == 0){return 0;}

  if(len < 64)// if you read less than a full blk
  {
    uint i = 0;
    int n = readPos;
    int count = 0;
    char current[64];

    /*  find out where your current addrs is in the address array  (used later) */
    if (myPM->readDiskBlock(currentBlk, current) < 0) { return -3; }
    for(; i < allAddresses.size(); i++){if(allAddresses[i] == currentBlk){break;}}

    if((indexInBlk + len) >= 64)
    {
      /* read until you read a 'c' or "clear" */
      for(int j = indexInBlk; j < 64; j++)
      { if(current[j] != 'c'){data[n] = current[j]; n++; count++;} }

      /* if you are last block stop reading and set pointer to end of block */
      if(allAddresses[allAddresses.size()-1] == currentBlk)
      {
        fileTable[fileIndex].rwpointer.first = currentBlk;
        fileTable[fileIndex].rwpointer.second = 64;
        fileTable[fileIndex].readPos = 0;
        return count;
      }
      else /* go to next block and keep reading */
      {
        fileTable[fileIndex].rwpointer.first = allAddresses[i+1];
        fileTable[fileIndex].rwpointer.second = 0;
        fileTable[fileIndex].readPos = n;
        len -= count;
        return readFile(fileDesc, data, len) + len;
      }
    }
    else /*  you will not read past a block  */
    {
      for(; i < allAddresses.size(); i++){if(allAddresses[i] == currentBlk){break;}}
      for(int j = indexInBlk; j < len+indexInBlk; j++)
      { if(current[j] != 'c'){data[n] = current[j]; n++; count++;} }

      /* increase the pointer by len read */
      fileTable[fileIndex].rwpointer.first = currentBlk; 
      fileTable[fileIndex].rwpointer.second = indexInBlk + len; 
      fileTable[fileIndex].readPos = 0;

      /*  if you read less than the total length (e.g. you hit file end) return the count instead of length */
      if(count < len){fileTable[fileIndex].rwpointer.second = indexInBlk + count; return count;}
      return len;
    }
  }
  else /* your read length > 64 */
  {
    uint i = 0;
    int n = readPos;
    int count = 0;
    char current[64];

    /* get location in address array */
    if (myPM->readDiskBlock(currentBlk, current) < 0) { return -3; }
    for(; i < allAddresses.size(); i++){if(allAddresses[i] == currentBlk){break;}}

    /* read until 'c' ("clear") */
    for(int j = indexInBlk; j < 64; j++)
    { if(current[j] != 'c'){data[n] = current[j]; n++; count++;} }

    if(allAddresses[allAddresses.size()-1] == currentBlk)
    {
      /* stop at end of file  */
      fileTable[fileIndex].rwpointer.first = allAddresses[i];
      fileTable[fileIndex].rwpointer.second = indexInBlk + count;
      fileTable[fileIndex].readPos = 0;
      return count;
    }
    else /* continue reading  */
    {
      fileTable[fileIndex].rwpointer.first = allAddresses[i+1];
      fileTable[fileIndex].rwpointer.second = 0;
      fileTable[fileIndex].readPos = n;
      len -= count;
      return readFile(fileDesc, data, len) + count;
    }
  }

  return 0;
}

// Perform write operations on a file whose file descriptor is fileDesc.
//   len is the number of bytes to write into the buffer pointed to by data.
// Returns:
//   -1 if the file descriptor is invalid
//   -2 if length is negative
//   -3 if the operation is not permitted
//   Number of bytes written if successful
// The write operation operates from the byte pointed to by the rw pointer.
// The write operation overwrites the existing data in the file and may increase
// the size of the file.
// After the write is done, the rw pointer is updated to point to the byte following
// the last written.
int FileSystem::writeFile(int fileDesc, char *data, int len)
{
  /*  initial data checks  */
  int fileIndex = 0;  // index of file in file table
  if ((fileIndex = fileDescExists(fileDesc)) == -1) { return -1; }
  if (len < 0) { return -2; }
  if (fileTable[fileIndex].mode != 'w' && fileTable[fileIndex].mode != 'm') { return -3; }

  /*  get file name  */
  int nameLen = fileTable[fileIndex].nameSize; // size of file path
  char* filename = fileTable[fileIndex].name; // file path
  char fname = filename[nameLen - 1];  // filename

  /*  get directory were file resides  */
  int dirLoc = travelDirectories(filename, nameLen);
  dirLoc = myPM->findFile(fname, dirLoc);
  char directory[64];
  if (myPM->readDiskBlock(dirLoc, directory) < 0) { return -3; }

  /*  get file inode and other info  */
  int inodeLoc = getLocation(directory, fname); // location of file Inode
  char inode[64]; // file inode data
  char indirect[64]; // file's "indirect block" data
  int indirectLoc; // file's "indirect block" location
  bool sizeInc = false;
  if (myPM->readDiskBlock(inodeLoc, inode) < 0) { return -3; }

  /*  get current fileSize  */
  int fileSize = getFileSize(inode);  // file size
  
  /*  set up the vectors that will store the direct addresses in the file inode
   *    as well as all of the addresses in the indirect inode
   */
  std::vector<int> indirectBlks;
  indirectLoc = getIndirectBlknum(inode); // get file's "indirect block" location
  if(indirectLoc != 0) { if (myPM->readDiskBlock(indirectLoc, indirect) < 0) { return -3; } } // prevent read from superblock
  if(indirectLoc != 0) { indirectBlks = getIndirectLoc(indirect); } // if there is no inderect block don't do this
  std::vector<int> directBlks = getDataLoc(inode); // the file's direct-block locations as integers

  /*  get the current rw-pointer info  */
  int currentBlk = fileTable[fileIndex].rwpointer.first; // rwpointer "block we are writing to" (3-partitionSize)
  int indexInBlk = fileTable[fileIndex].rwpointer.second; // rwpointer location within the "block we are writing to" (0-64)
  if(indirectBlks.size() > 0)
  { if(currentBlk == indirectBlks[indirectBlks.size()-1] && indexInBlk + len > MAX_FILE_SIZE){return -3;} }


  /*  if this is abrand new file (has not been written to at all)  */
  if(currentBlk == 0)
  {
    if((currentBlk = myPM->getFreeDiskBlock()) == -1) { return -3; } // get the location of a free data block
    currentBlk += 1; // adjust for offsett between bitVector and partition
    writeDirectAddress(currentBlk, 1, inode); // write the new direct address to the file inode
    if(myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; } // write the updated file inode to disk
    directBlks.push_back(currentBlk); // increase the size of of the directBlks vector (see further down for why)

    /*  all file table inodes begin with their rw pointer set to  < 0,  0 >  if they have no data associated with them
     *  this will cause issues if a file is opened multiple times so we need to update that here
     */
    for (uint i = 0; i < fileTable.size(); i++)
    {
      if (fileTable[i].name == fileTable[fileIndex].name && fileTable[i].rwpointer.first == 0)
      {
        /* synchronize the starting block for a file (i.e. rw-pointer.first)
         *    accross all instances of the file that are "open" (i.e. in the file table)
        */
        fileTable[i].rwpointer.first = currentBlk;
      }
    }
  }



  /*       BEGIN WRITING THE DATA TO THE FILE      */
  if((indexInBlk + len) <= 64) // as long as what we need to write can fit in one block
  {
    /* write to a buffer then write that buffer to disk  */
    char toWrite[64]; // what we need to write
    int n = 0;
    if (myPM->readDiskBlock(currentBlk, toWrite) < 0) { return -3; }
    if(toWrite[indexInBlk] == 'c'){sizeInc = true;} // "will this read increment the size?"
    for (int j = indexInBlk; j < len + indexInBlk; j++) // write the actual data to 'toWrite'
    { toWrite[j] = data[n]; n++; }
    if (myPM->writeDiskBlock(currentBlk, toWrite) < 0) { return -3; } // write to disk

    /*  set the new rw-pointer  */
    fileTable[fileIndex].rwpointer.first = currentBlk;
    fileTable[fileIndex].rwpointer.second = indexInBlk + len;

    /*  write out new size  */
    int size = 0;
    if(sizeInc){ size = fileSize + len; }
    else { size = fileSize; }
    writeSize(inode, size);  // write new size to file inode
    if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; } // write the updated file inode to disk
    fileTable[fileIndex].endfile.first = fileTable[fileIndex].rwpointer.first;
    fileTable[fileIndex].endfile.second = fileTable[fileIndex].rwpointer.second;
    return len;
  }
  else if ((indexInBlk + len) > 64)  // you are attempting to write past the end of a block
  {
    /*  this is were you set up the data array's for writing  */
    char maxWrite[64];
    if (myPM->readDiskBlock(currentBlk, maxWrite) < 0) { return -3; } // read data already on disk (we want to extend OR overwrite)

    /*  write the info we need to the respective data arrays (note that we start from where the pointer is)  */
    int count = 0;  // how much did you actually write (need for fileSize calc)
    int k = 0; // index count for data[] array 
    for (int i = indexInBlk; i < 64; i++)
    {
      maxWrite[i] = data[k];
      count++; // increment "how much will I write?" count
      k++; // how much you have read from data (were you will start from next time)
    }

    /*  calculate how much will be "left to write" after taking away K bytes  */
    int diff = len - count;
    char* leftOver;
    if (diff > 0) { leftOver = new char[diff]; }
    for (int i = 0; i < diff; i++) { leftOver[i] = data[k]; k++; }

    /* write the maxWrite buffer to disk  */
    if (myPM->writeDiskBlock(currentBlk, maxWrite) < 0) { return -3; } 


    /* not in the indirect and not the last direct */
    if (directBlks.size() < 4 && directBlks.size() != 3)
    {
      /* last current is last direct address #2 */
      if (currentBlk == directBlks[directBlks.size() - 1])
      {
          /*  make new direct address block  */
        if ((currentBlk = myPM->getFreeDiskBlock()) == -1) { return -3; }
        currentBlk += 1;
        writeDirectAddress(currentBlk, directBlks.size() + 1, inode);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }

          /*  update rw-pointer  */
        fileTable[fileIndex].rwpointer.first = currentBlk;
        fileTable[fileIndex].rwpointer.second = 0;


           /*  update size  */
        writeSize(inode, count + fileSize);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }
        return writeFile(fileDesc, leftOver, diff) + count;
      }
      else /* currentBlk is direct address #1 */
      {
          /*  get index in vector of next  direct address  */
        uint i = 0;
        for (; i < directBlks.size(); i++) { if (directBlks[i] == currentBlk) { break; } }

          /*  update rwpointer  */
        fileTable[fileIndex].rwpointer.first = directBlks[i + 1];
        fileTable[fileIndex].rwpointer.second = 0;

          /*  write out new size  */
        writeSize(inode, fileSize);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }
        return writeFile(fileDesc, leftOver, diff) + count;
      }
    }
    else if (directBlks.size() == 3) // you are at direct address #3
    {
      /* need to make bothe indirect block and direct address in the new indirect blk */
      if (currentBlk == directBlks[directBlks.size() - 1])
      {
          /* get block for indirect inode and update file inode with new info  */
        if ((currentBlk = myPM->getFreeDiskBlock()) == -1) { return -3; }
        currentBlk += 1;
        writeDirectAddress(currentBlk, directBlks.size() + 1, inode);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }

          /*  get a new direct block and write it to indirect inode  then update disk  */
        indirectLoc = currentBlk;
        if (myPM->readDiskBlock(indirectLoc, indirect) < 0) { return -3; }
        if ((currentBlk = myPM->getFreeDiskBlock()) == -1) { return -3; }
        currentBlk += 1;
        writeIndirectAddress(currentBlk, indirect);
        if (myPM->writeDiskBlock(indirectLoc, indirect) < 0) { return -3; }

          /*  update rwpointer  */
        fileTable[fileIndex].rwpointer.first = currentBlk;
        fileTable[fileIndex].rwpointer.second = 0;


          /* update size  */
        writeSize(inode, count + fileSize);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }
        return writeFile(fileDesc, leftOver, diff) + count;
      }
      else /* move to next direct address */
      {
        uint i = 0;
        /*  get index in vector of next  direct address  */
        for (; i < directBlks.size(); i++) { if (directBlks[i] == currentBlk) { break; } }

        // set rwpointer
        fileTable[fileIndex].rwpointer.first = directBlks[i + 1];
        fileTable[fileIndex].rwpointer.second = 0;

        /*  update file size  */
        writeSize(inode, fileSize);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; } // write the updated file inode to disk
        return writeFile(fileDesc, leftOver, diff) + count;
      }
    }
    else if (directBlks.size() == 4) /* already have an indirect block */
    {
      /*  set rwpointer to first direct address in indirect block  */
      if (currentBlk == directBlks[2])
      {
        fileTable[fileIndex].rwpointer.first = indirectBlks[0];
        fileTable[fileIndex].rwpointer.second = 0;

        /*  update file size  */
        writeSize(inode, fileSize);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }
        return writeFile(fileDesc, leftOver, diff) + count;
      }
      else if (currentBlk == directBlks[0] || currentBlk == directBlks[1]) /* jump to next direct address */
      {
        /*  get index in vector of next  direct address  */
        uint i = 0;
        for (; i < directBlks.size(); i++) { if (directBlks[i] == currentBlk) { break; } }

        /*  update rwpointer  */
        fileTable[fileIndex].rwpointer.first = directBlks[i + 1];
        fileTable[fileIndex].rwpointer.second = 0;

        /* update file size  */
        writeSize(inode, fileSize);
        if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }
        return writeFile(fileDesc, leftOver, diff) + count;
      }
      else
      {
        /*  make a new direct address block in the indirect inode  */
        if (currentBlk == indirectBlks[indirectBlks.size() - 1])
        {
          if ((currentBlk = myPM->getFreeDiskBlock()) == -1) { return -3; }
          currentBlk += 1;
          writeIndirectAddress(currentBlk, indirect);
          if (myPM->writeDiskBlock(indirectLoc, indirect) < 0) { return -3; }

          /*  update rw-pointer  */
          fileTable[fileIndex].rwpointer.first = currentBlk;
          fileTable[fileIndex].rwpointer.second = 0;

          /*  update size  */
          writeSize(inode, count + fileSize);
          if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }
          return writeFile(fileDesc, leftOver, diff) + count;
        }
        else  // simply set rwpointer to point to next direct address within indirect block
        {
          /*  get index in vector of next  direct address  */
          uint i = 0;
          for (; i < indirectBlks.size(); i++) { if (indirectBlks[i] == currentBlk) { break; } }

          // set rwpointer
          fileTable[fileIndex].rwpointer.first = indirectBlks[i + 1];
          fileTable[fileIndex].rwpointer.second = 0;

          /*  update size  */
          writeSize(inode, fileSize);
          if (myPM->writeDiskBlock(inodeLoc, inode) < 0) { return -3; }
          return writeFile(fileDesc, leftOver, diff) + count;
        }
      }
    }
  }
  /* if something goes horribly wrong  */
  return -3;
}

// Perform append operations on a file whose file descriptor is fileDesc.
//   len is the number of bytes to be appended into the buffer pointed to by data.
// Returns:
//   -1 if the file descriptor is invalid
//   -2 if length is negative
//   -3 if the operation is not permitted
//   Number of bytes appended if successful
// The append operation appends the data at the end of the file.
// After the append is done, the rw pointer is updated to point to the byte following
// the last append.
int FileSystem::appendFile(int fileDesc, char *data, int len)
{
  /* variables */
  char inode[64];
  char directory[64];
  char* filename;
  int inodeLoc;
  int fileSize;
  int fnameLen;
  int fileIndex;
  char fname;

  /*  get info from file table  */
  if((fileIndex = fileDescExists(fileDesc)) < 0){return -1;}
  filename = fileTable[fileIndex].name;
  fnameLen = fileTable[fileIndex].nameSize;
  fname = filename[fnameLen-1];

  /* check that mode is correct */
  if (fileTable[fileIndex].mode != 'w' && fileTable[fileIndex].mode != 'm') { return -3; }

  /* get home dir */
  int directoryLoc = travelDirectories(filename, fnameLen);
  directoryLoc = myPM->findFile(fname, directoryLoc);
  if(myPM->readDiskBlock(directoryLoc, directory) < 0){return -3;}

  /* get fileInode and location */
  inodeLoc = getLocation(directory, fname);
  if(myPM->readDiskBlock(inodeLoc, inode) < 0){return -3;}
  fileSize = getFileSize(inode);

  /* check that we won't go over max file size */
  if (fileSize == MAX_FILE_SIZE || (fileSize + len) > MAX_FILE_SIZE) { return -3; }

  /* if teh file size is zero simply write the file normally else seek to end then write */
  if(fileSize == 0){return writeFile(fileDesc, data, len);}
  if(seekFile(fileDesc, fileSize, 1) < 0){return -5;}
  return writeFile(fileDesc, data, len);;
}

// This operation modifies the rw pointer of the file whose file descriptor is
// filedesc.
//   if flag = 0; the rw pointer is moved offset bytes forward
//   else; it is set to byte number offset in the file.
// Returns:
//   -1 if the file descriptor, offset or flag is invalid
//   -2 if an attempt to go outside the file bounds is made (end of file or
//     beginning of file)
//   0 if successful.
// A negative offset is valid.
int FileSystem::seekFile(int fileDesc, int offset, int flag)
{
  /* variables */
  char* filename;
  int fileIndex;
  int fnameLen;
  int fileSize;
  char inode[64];
  char directory[64];
  char indirect[64];
  int inodeLoc;
  int indirectLoc;
  int offsetCopy = offset;
  int count = 0;

  /* get infor from file Table */
  if((fileIndex = fileDescExists(fileDesc)) < 0){return -1;}
  fileTable[fileIndex].fdesc = fileDesc;
  filename = fileTable[fileIndex].name;
  fnameLen = fileTable[fileIndex].nameSize;
  char fname = filename[fnameLen-1];

  /* get home Dir */
  int directoryLoc = travelDirectories(filename, fnameLen);
  directoryLoc = myPM->findFile(fname, directoryLoc);
  if(myPM->readDiskBlock(directoryLoc, directory) < 0){return -3;}

  /* get file inode and location */
  inodeLoc = getLocation(directory, fname);
  if(myPM->readDiskBlock(inodeLoc, inode) < 0){return -3;}
  fileSize = getFileSize(inode);

  /* set up the direct block address vectors */
  std::vector<int> directAd = getDataLoc(inode);

  // if there are no direct address you are seeking out of bounds
  if(directAd.size() == 0){return -2;}

  /* set up the indirect block address vector */
  indirectLoc = getIndirectBlknum(inode);
  std::vector<int> indirectAd;
  if(indirectLoc != 0) // if no indirect don't do this stuff
  {
    if(myPM->readDiskBlock(indirectLoc, indirect) < 0){return -3;}
    indirectAd = getIndirectLoc(indirect);
  }

  /* get current r/w pointer */
  int currentBlk = fileTable[fileIndex].rwpointer.first;
  int currentIndex = fileTable[fileIndex].rwpointer.second;


  /* do the pointer arithmetic based on flag passed
   * is recursive since:
   *  moving forward X bytes is the same as setting the the rwpointer to byte (current + X) 
   *  our rwpointer is a pair < blknum, blknumIndex > so this function calculates absolute byte value as well
  */
  if(flag == 0)
  {
    if((offset + currentIndex) > fileSize){return -2;}
    if((offset + currentIndex) < 0 && directAd.size() == 1){return -2;}
    if((offset*-1) == fileSize)
    {
      fileTable[fileIndex].rwpointer.first = directAd[0]; 
      fileTable[fileIndex].rwpointer.second = 0; 
      return 0;
    }
    int blkByteVal = 0;
    if(directAd.size() <= 4)
    {
      for(uint index = 0; index < directAd.size(); index++)
      { 
        if(directAd[index] == currentBlk && index != 0)
        { blkByteVal = (64 * index) + currentIndex; return seekFile(fileDesc, offset+blkByteVal, 1);} 
        else if(directAd[index] == currentBlk && index == 0)
        {
          if(offset < 0){blkByteVal = currentIndex + offset; return seekFile(fileDesc, blkByteVal, 1);}
          else{return seekFile(fileDesc, offset+blkByteVal+currentIndex, 1);}
        }
      }
      for(uint index = 0; index < indirectAd.size(); index++)
      { 
        if(indirectAd[index] == currentBlk && index != 0){blkByteVal = (64 * index) + currentIndex + (64 * 3); break;} 
        else if(indirectAd[index] == currentBlk && index == 0){blkByteVal = currentIndex + (64 * 3); break;} 
      }
    }
    return seekFile(fileDesc, offset+blkByteVal, 1);
  }
  else
  {
    if(offset > fileSize){return -2;}
    if(offset < 0){return -1;}
    if(offset > 0)
    {
      while(offsetCopy > 64){count++; offsetCopy -= 64;}
      if(count < 3){fileTable[fileIndex].rwpointer.first = directAd[count], fileTable[fileIndex].rwpointer.second = offsetCopy;}
      else if(count == 4 && offsetCopy < 64){fileTable[fileIndex].rwpointer.first = currentBlk, fileTable[fileIndex].rwpointer.second = offsetCopy;}
      else if(count == 4){fileTable[fileIndex].rwpointer.first = indirectAd[0], fileTable[fileIndex].rwpointer.second = offsetCopy;}
      else{fileTable[fileIndex].rwpointer.first = indirectAd[count-3], fileTable[fileIndex].rwpointer.second = offsetCopy;}
    }
    else if(offset == 0)
      { fileTable[fileIndex].rwpointer.first = directAd[0], fileTable[fileIndex].rwpointer.second = 0; }
  }
  return 0;
}
// This operation changes the name of the file whose name is pointed to by fname1
// to the name pointed to by fname2.
// Returns:
//   -1 invalid filename
//   -2 if the file does not exist
//   -3 if there already exists a file whose name is the same as the name pointed
//     to by fname2
//   -4 if file is opened or locked
//   -5 for any other reason
//   0 if successful.
// A note, you can rename a directory with this operation.
int FileSystem::renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2)
{
	/*  initial data integrity checks  */
  if(filename1 == nullptr || filename2 == nullptr || fnameLen1 <=0 || fnameLen2 <= 0){return -5;}
  if(!nameCheck(filename1, fnameLen1)){return -1;}
  if(!nameCheck(filename2, fnameLen2)){return -1;}

	/*  variables  */
	char fname1 = filename1[fnameLen1-1]; // old file name
	char fname2 = filename2[fnameLen2-1]; // new file name
	int dirBlknum = travelDirectories(filename1, fnameLen1);
  dirBlknum = myPM->findFile(fname1, dirBlknum);
	Node* n = getNode(filename1); // file table entry
	Lock* l = getLock(filename1); // lock table entry
	char directory[64];  // home directory data
	int finode;    // file inode location
	char inode[64]; // file inode data

	/*  error checks  */
	if(myPM->findFile(fname1, dirBlknum) <= 0){return -2;} // if old file does not exist
	if(myPM->findFile(fname2, dirBlknum) > 0){return -3;} // if new file name already exists
	if(n != nullptr && n->isOpen){return -4;} // if file is open
	if(l != nullptr && l->isLocked){return -4;} // if file is locked


	// change file name
	if(myPM->readDiskBlock(dirBlknum, directory) < 0){return -5;}
	for(int  i = 0; i < 64; i++)
		{ if(directory[i] == fname1){ directory[i] = fname2; } }

	if(myPM->writeDiskBlock(dirBlknum, directory) < 0){return -5;} // write new filename to disk

	// need to change file inode name as well assuming this is not a directory
	finode = getLocation(directory, fname2); //get inode location
	if(myPM->readDiskBlock(finode, inode) < 0){return -5;} //get data
	if(!isFile(inode)){return 0;}  // check if it is a file or not

	inode[0] = fname2; // change the first character of the file inode (i.e. filename)
	if(myPM->writeDiskBlock(finode, inode) < 0){return -5;} // write updated file inode to disk

  /*  need to change fileTable also  */
  for(uint i = 0; i < fileTable.size(); i++)
  { if(fileTable[i].name == filename1){fileTable[i].name = filename2;} }

	return 0;
}

// Get the attributes of a file whose name is pointed to by filename.
// *Work out the details of these operations based on the file attributes
// you choose. You must choose a minimum of two attributes for your filesystem.*
// returns null if fail for any reason
string FileSystem::getAttribute(char *filename, int fnameLen)
{
  if(!nameCheck(filename, fnameLen)){return "NULL";}
  if(fnameLen <= 0 || filename == nullptr){return "NULL";}
  char inode[64];
  char directory[64];
  int inodeLoc;
  string string_statement = "Creation Date/Time: "; //begin constructing of the character array.
  time_t dateTimeCreated; //datetime storage
  int intTime; //datetime int storage
  char timeBuff[10];
  char fname = filename[fnameLen-1];

  int directoryLoc = travelDirectories(filename, fnameLen);
  directoryLoc = myPM->findFile(fname, directoryLoc);
  if (directoryLoc <= 0) { return "NULL"; }
  if(myPM->readDiskBlock(directoryLoc, directory) < 0){return "NULL";}

  inodeLoc = getLocation(directory, fname);
  if(myPM->readDiskBlock(inodeLoc, inode) < 0){return "NULL";}
  if(!isFile(inode)){return "NULL";}

  //grab the epoch time from the character array
  int i = 22;
  for (int timeOffset = 0; i < 32; i++) 
  { 
    timeBuff[timeOffset] = inode[i];
    timeOffset++;
  }
  intTime = atoi(timeBuff);
  dateTimeCreated = (time_t)intTime;

  //build rest of string to return 
  string_statement += asctime(localtime(&dateTimeCreated));
  string_statement += "Extension: ";
  for(int i = 32; i < 37; i++){if(inode[i] != 'c'){string_statement += inode[i];}}
  string_statement += "\nFile: ";
  string_statement += filename;

  return string_statement;
}

// Set the attributes of a file whose name is pointed to by filename.
// *Work out the details of these operations based on the file attributes
// you choose. You must choose a minimum of two attributes for your filesystem.*
// -1 if filename not valid
// -2 if file does not exist
// -3 if file is a directory
// -4 else
int FileSystem::setAttribute(char *filename, int fnameLen , char* extension, int extLen)
{
  if(!nameCheck(filename, fnameLen)){return -1;}
  if(extLen <= 0 || extLen > 4 || fnameLen <= 0 || 
    filename == nullptr || extension == nullptr || extension[0] == ' '){return -4;}

  char inode[64];
  char directory[64];
  char ext[extLen+1];
  int inodeLoc;
  int n = 0;
  int k = 0;
  char fname = filename[fnameLen-1];
  ext[0] = '.';
  for(int i = 1; i < extLen+1; i++){ext[i] = extension[k]; k++; }
  int directoryLoc = travelDirectories(filename, fnameLen);
  directoryLoc = myPM->findFile(fname, directoryLoc);
  if(directoryLoc <= 0){return -2;}

  if(myPM->readDiskBlock(directoryLoc, directory) < 0){return -4;}
  
  inodeLoc = getLocation(directory, fname);
  if(myPM->readDiskBlock(inodeLoc, inode) < 0){return -4;}
  if(!isFile(inode)){return -3;}

  //max extension length = 4
  for(int i = 32; i < 32+extLen+1; i++){inode[i] = ext[n]; n++;}
  //write the inode to disk
  if(myPM->writeDiskBlock(inodeLoc, inode) < 0){return -4;} //write inode to disk

  return 0; // successful setAttribute
}






/*  EXTRA STUFF  */
//----------------------------------------------------------------


/* special version of isFile */
bool FileSystem::isFile2(int directoryBlknum, char name)
{
	int inodeLoc;
	char directory[64];
	char fileInode[64];

	if(myPM->readDiskBlock(directoryBlknum, directory) < 0){return false;}
	inodeLoc = getLocation(directory, name);

	if(myPM->readDiskBlock(inodeLoc, fileInode) < 0){return false;}
	if(fileInode[1] == 'f'){return true;}
	return false;
}


/*  check if filedescriptor is correct  */
int FileSystem::fileDescExists(int fdesc)
{
	for(uint i = 0; i < fileTable.size(); i++)
	{ if(fileTable[i].fdesc == fdesc){return i;} }
	return -1;
}
//----------------------------------------------------------------



/*  wrapper function for traverse directories  */
int FileSystem::travelDirectories(char* filename, int fnameLen)
{
	int directoryBlock = 2;
	std::vector<char> path;
	path = convertToVec(filename, fnameLen);
	if(path.size() > 0){path.pop_back();}
	directoryBlock = traverse(&path, directoryBlock);
	return directoryBlock;
}
//----------------------------------------------------------------



/*  write a file Inode to disk  */
int FileSystem::writeFileInode(int blknum, char fname)
{
  /*  variables  */
  int inodeSize = 33; // size include attribute
  char node[inodeSize]; // inode
  char buff[64];
  int i = 0;     // node array index
  time_t dateTimeCreated = time(0); //snag time as epoch time
  int intTime = (int)dateTimeCreated; //convert our date into an int to store in attributes
  char timeBuff[10];
  sprintf(timeBuff, "%d", intTime);

  /*  Write Node  */
  clearBlock(buff);
  if(myPM->readDiskBlock(blknum, buff) < 0){return -4;}

  node[i] = fname; // get filename
  i++;

  node[i] = 'f';  //set type
  i++;

  for(; i < 6; i++){node[i] = '0';} // set file size (fresh file has size == 0)
  for(; i < 18; i++){node[i] = '0';} // set direct addresses (fresh file has no addresses nothing has been written yet)
  for(; i < 22; i++){node[i] = '0';} // set indirect addresses (fresh file has no indirect addresses)

  // set datetime in file
  for (int timeOffset = 0; i < 32; i++) 
  { 
    node[i] = timeBuff[timeOffset];
    timeOffset++;
  }

  for(int l = 0; l < 64; l++) // write inode to buffer
  { 
    if(l < inodeSize){buff[l] = node[l];}
    else{buff[l] = ' ';}
  } 
  if(myPM->writeDiskBlock(blknum, buff) < 0){return -4;} //write inode to disk

  /*  if success return 0  */
  return 0;
}
//----------------------------------------------------------------




/*  write a file entry to a directory  */
int FileSystem::writeFileEntry(int blknum, char fname)
{
	/*  variables  */
	char entry[6];  // directory entry
	char buff[64];  // data buffer
	char *temp;    // store location as string
	int n = 0;     // entry array index
	int j = 0;     // second entry array index (for clarity)

	/*  write directory entry  */
	entry[n] = fname; // set first value of entry to filename
	n++; //incr counter

	int loc = myPM->getFreeDiskBlock(); // get a free disk location for Inode
	if(loc < 0){return -4;}
	loc = loc+1; //fix bitVector vs partition offset

	temp = IntToChar2(loc);  // convert location to char array

	for(int i = 0; i < 4; i++){entry[n] = temp[i]; n++;} // write Inode location to entry
	entry[n] = 'f'; // write thing "type" ('f' or 'd' in this case only 'f') to entry


	if(myPM->readDiskBlock(blknum, buff) < 0){return -4;}  // get current directory info from disk (need to add to it)
	for(int i = 0; i < 64; i++)   
	{
		if(buff[i] == ' ' &&  j < 6)  // If there is free space write out the entry
		{
			buff[i] = entry[j];
			j++;
		}
	}

	if(myPM->writeDiskBlock(blknum, buff) < 0){return -4;}  // write updated directory to disk

	return loc;  // return location of file Inode
}
//----------------------------------------------------------------



/*  write a "directory entry" to a directory  */
int FileSystem::writeDirectoryEntry(int blknum, char dname)
{
	/*  variables  */
	char entry[6];  // directory entry
	char buff[64];  // data buffer
	char *temp;    // store location as string
	int n = 0;     // entry array index
	int j = 0;     // second entry array index (for clarity)


	/*  write directory entry  */
	entry[n] = dname; // set first value of entry to dirname
	n++; //incr counter

	int loc = myPM->getFreeDiskBlock(); // get a free disk location for Inode
	if(loc < 0){return -4;}
	loc = loc+1; //fix obitVector vs partition offset

	temp = IntToChar2(loc);  // convert location to char array

	for(int i = 0; i < 4; i++){entry[n] = temp[i]; n++;} // write Inode location to entry
	entry[n] = 'd'; // write thing "type" ('f' or 'd' in this case only 'd') to entry


	if(myPM->readDiskBlock(blknum, buff) < 0){return -4;}  // get current directory info from disk (need to add to it)
	for(int i = 0; i < 64; i++)   
	{
		if(buff[i] == ' ' &&  j < 6)  // If there is a free space write out the entry
		{
			buff[i] = entry[j];
			j++;
		}
	}

	if(myPM->writeDiskBlock(blknum, buff) < 0){return -4;}  // write updated directory to disk
	return loc;
}
//----------------------------------------------------------------


/*  check if block is full  */
bool FileSystem::isFullBlk(char* buffer)
{
	for(int i = 0; i < 60; i++)
	{
		if(buffer[i] == ' '){return false;}
	}
	return true;
}
//----------------------------------------------------------------



/*  recursively traverse directories (used by traveldirectories())  */
int FileSystem::traverse(std::vector<char>* name, int dirBlknum)
{
	if(name->size() > 0)
	{
		char crntDirInode[64];
		char trgtDirLoc[4];
		int tdloc = 0;
		int dirBlock = 0;
		if((dirBlock = myPM->findFile(name->at(0), dirBlknum)) <= 0){return -1;}

		myPM->readDiskBlock(dirBlock, crntDirInode);
		for(int i = 0; i < 64; i++)
		{
			if(crntDirInode[i] == name->at(0))
			{
				int n = i+1;
				for(int  j = 0; j < 4; j++)
				{
					trgtDirLoc[j] = crntDirInode[n];
					n++;
				}
				tdloc = CharToInt2(trgtDirLoc);
				break;
			}
		}
		name->erase(name->begin());
		return traverse(name, tdloc);
	}
	else
	{
		return dirBlknum;
	}
}
//----------------------------------------------------------------



/*  make a new (fresh) lock object  */
void FileSystem::newLock(char* name)
{
	Lock l;
	l.name = name;
	l.isLocked = false;
	l.setLockID(0);
	lockTable.push_back(l);
}
//----------------------------------------------------------------



/*  make a new (fresh) node object  */
int FileSystem::newNode(char* name, char mode, int nameSize)
{
	Node n;
	n.isOpen = true;
	n.name = name;
	n.mode = mode;
	n.rwpointer.first = 0;
	n.rwpointer.second = 0;
	n.fdesc = fileTable.size()+1;
	n.nameSize = nameSize;
        n.readPos = 0;
	fileTable.push_back(n);

	return n.fdesc;
}
//----------------------------------------------------------------



/*  get a lock object  */
Lock* FileSystem::getLock(char* name)
{
	for( uint i = 0; i < lockTable.size(); i++)
	{
		if(name == lockTable[i].name){return &lockTable[i];}
	}
	return nullptr;
}
//----------------------------------------------------------------



/*  get a Node object  */
Node* FileSystem::getNode(char* name)
{
	for(uint i = 0; i < fileTable.size(); i++)
	{
		if(name == fileTable[i].name){return &fileTable[i];}
	}
	return nullptr;
}
//----------------------------------------------------------------



/*  print the file and lock tables  */
void FileSystem::printTables()
{
	printf("\n\n");
	printf("-------------------------------\n");
	printf("|          Lock Table         |\n");
	printf("-------------------------------\n");

	if(lockTable.size() == 0){printf("         Table is Empty        \n\n");}
	for(uint i = 0; i < lockTable.size(); i++)
	{
		printf("[%d] -->  name: %s  |  isLocked: %d  |  lockID: %d  \n", i, 
			lockTable[i].name, lockTable[i].isLocked, lockTable[i].getLockID());
	}
	printf("\n\n");


	printf("\n\n");
	printf("-------------------------------\n");
	printf("|          File Table         |\n");
	printf("-------------------------------\n");

	if(fileTable.size() == 0){printf("         Table is Empty        \n\n");}
	for(uint i = 0; i < fileTable.size(); i++)
	{
		printf("[%d] -->  name: %s  |  name length: %d  |  fDesc: %d  |  mode: %c |  read/write pointers (< blockID, location >):  < %d, %d > \n", i, 
			fileTable[i].name, fileTable[i].nameSize, fileTable[i].fdesc, fileTable[i].mode, fileTable[i].rwpointer.first, fileTable[i].rwpointer.second);
	}
}
//----------------------------------------------------------------



/*  print the Bit Vector  */
void FileSystem::printBV()
{ myPM->printBitVec(); }
//----------------------------------------------------------------



/* clean up any trailing directories */
void FileSystem::clean(int dirBlknum)
{
  // needs more (recursion) but meh it works
	char dir[64];
	char nextdir[4];
	int next = 0;
	int n = 0;
	bool empty = true;
	myPM->readDiskBlock(dirBlknum, dir);
	
	for(int i = 60; i < 64; i++){nextdir[n] = dir[i]; n++;}
	next = CharToInt2(nextdir);

	if(next != 0)
	{
		myPM->readDiskBlock(next, dir);
		for(int i = 0; i <60; i++){if(dir[i] != ' '){empty = false;}}
		if(empty)
		{
			char clear[64];
			clearBlock(clear);
			myPM->returnDiskBlock(next-1);
			myPM->writeDiskBlock(next, clear);
			
			myPM->readDiskBlock(dirBlknum, clear);
			for(int i = 60; i <64; i++){clear[i] = '0';}
			myPM->writeDiskBlock(dirBlknum, clear);
			
		}
	}
}
//----------------------------------------------------------------



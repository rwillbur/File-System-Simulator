
#include <utility>
#include <vector>
#include <stdio.h>
#include <string>

#ifndef FILE_SYS_H
#define FILE_SYS_H

class Lock //used with lockTable
{
  int lockID;
public:
  bool isLocked;
  char* name;
  void setLockID(int ID){lockID = ID;}
  int getLockID(){return lockID;}

};

class Node // used in fileTable
{
public:
 int fdesc;
 bool isOpen;
 char* name;
 int nameSize;
 int readPos; 
 std::pair<int,int> endfile;
 std::pair<int, int> rwpointer;
 char mode;
};

class FileSystem {
  DiskManager *myDM;
  PartitionManager *myPM;
  char myfileSystemName;

  std::vector<Lock> lockTable;
  std::vector<Node> fileTable;
  int traverse(std::vector<char>* dir, int dirBlknum);
  
  /* declare other private members here */

public:
  FileSystem(DiskManager *dm, char fileSystemName);
  int createFile(char *filename, int fnameLen);
  int createDirectory(char *dirname, int dnameLen);
  int lockFile(char *filename, int fnameLen);
  int unlockFile(char *filename, int fnameLen, int lockId);
  int deleteFile(char *filename, int fnameLen);
  int deleteDirectory(char *dirname, int dnameLen);
  int openFile(char *filename, int fnameLen, char mode, int lockId);
  int closeFile(int fileDesc);
  int readFile(int fileDesc, char *data, int len);
  int writeFile(int fileDesc, char *data, int len);
  int appendFile(int fileDesc, char *data, int len);
  int seekFile(int fileDesc, int offset, int flag);
  int renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2);
  std::string getAttribute(char *filename, int fnameLen);
  int setAttribute(char *filename, int fnameLen, char* extension, int extLen);
  //--------------------------------------------------------------------------------------||

  Lock* getLock(char* name); 
  void newLock(char* name);
  int newNode(char* name, char mode, int nameSize);
  Node* getNode(char* name);
  void printTables();
  bool isFullBlk(char* buffer);
  int writeFileEntry(int blknum, char fname);
  int writeDirectoryEntry(int blknum, char fname);
  int writeFileInode(int location, char fname);
  int travelDirectories(char* fname, int fnameLen);
  int fileDescExists(int fdesc);
  bool isFile2(int blknum, char name);
  void printBV();
  void clean(int dirBlknum);
  

    /* declare other public members here */

};

#endif


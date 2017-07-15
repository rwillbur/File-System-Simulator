#include "bitvector.h"
#include <stdio.h>

#ifndef PART_MAN_H
#define PART_MAN_H

class PartitionManager {
  DiskManager *myDM;
  BitVector *dmBV;  

public:
  char myPartitionName;
  int myPartitionSize;
  PartitionManager(DiskManager *dm, char partitionname);
  ~PartitionManager();
  int readDiskBlock(int blknum, char *blkdata);
  int writeDiskBlock(int blknum, char *blkdata);
  int getBlockSize();
  int getFreeDiskBlock();
  int returnDiskBlock(int blknum);
//---------------------------------------------

  int isFull();
  int findFile(char name, int dirBlock);
  int findDir(char name, int dirBlock);
  int isEmptyDir(int dirBlock);
  int initDirectory(int loc);
  int getRoot();
  int testBit(int blknum);
  void printBitVec();

/*
  Convert Character to Integer
*/
  char* IntToChar(int integer)
  {
    char* num = new char[4];
    sprintf(num, "%d", integer);
    for(int i=0; i<4; i++)
    {
      if(num[i] == '\0'){num[i] = '_';}
    }
    return num;
  }

/*
  Convert Character to Integer
*/

  int CharToInt(char* char_array)
  {
    int i;
    sscanf(char_array, "%d", &i);
    return i;
  }

};

#endif

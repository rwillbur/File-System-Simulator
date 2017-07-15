#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include <iostream>
#include <ctype.h>
using namespace std;


PartitionManager::PartitionManager(DiskManager *dm, char partitionname)
{
  myDM = dm;
  myPartitionName = partitionname;
  myPartitionSize = myDM->getPartitionSize(myPartitionName);

  /* If needed, initialize bit vector to keep track of free and allocted
     blocks in this partition */

  dmBV = new BitVector(myPartitionSize); //Declare bit vector

  char buffer[64]; //buffer

  if(readDiskBlock(1, buffer) < 0){} //read in block 0 (i.e. bit vector block) of partition

  if(buffer[0] == 'c') // if empty
  {
    dmBV->setBit(0); // set bitVec[0] to 1 (i.e. "taken")
    dmBV->setBit(1);
    dmBV->getBitVector((unsigned int *)buffer);  // write bitVector to buffer

    if(writeDiskBlock(1, buffer) < 0){} // write buffer to partition block 0
    initDirectory(2); //set up "fresh" directory
  }
  else //initialize bitVec with the data already in partition block 0
  { dmBV->setBitVector((unsigned int *)buffer); }

}
//----------------------------------------------------




//Destructor
PartitionManager::~PartitionManager(){}



/* Allocates a block
 * return blocknum, -1 otherwise
 */
int PartitionManager::getFreeDiskBlock()
{
  char buff[64];  // data storage array
  for(int i = 0; i < 64; i++){buff[i] = 'c';} //initialize buffer to remove garbage memory

  if(readDiskBlock(1,buff) < 0){return -1;} // get bitVector info from partition block 1
  // set the Partition Manager's bit Vector to the values pulled from partition block 1
  dmBV->setBitVector((unsigned int*) buff); 
  
  for(int i = 0; i < myPartitionSize; i++) //loop until free block is found
  {
    if(dmBV->testBit(i) == 0) // if free block is found
    {
      dmBV->setBit(i); // set bit to "used"
      dmBV->getBitVector((unsigned int*) buff); // write updated BitVector to buffer
      if(writeDiskBlock(1,buff) < 0){return -1;} // write updated bitVector to Disk (partition block 1)

      return i; // return block num
    }
  }


  return -1;
}
//----------------------------------------------------



/* Deallocates a block
 * return 0 for sucess, -1 otherwise
 */
int PartitionManager::returnDiskBlock(int blknum)
{

  char buff[64];  // data storage array
  for(int i = 0; i < 64; i++){buff[i] = 'c';} //initialize buffer to remove garbage memory
  if(readDiskBlock(1,buff) < 0){return -1;}  //get BitVector from Disk (partition block 1)

  // set the Partition Manager's bit Vector to the values pulled from partition block 1
  dmBV->setBitVector((unsigned int*) buff);
  dmBV->resetBit(blknum);

  dmBV->getBitVector((unsigned int*) buff); // write updated BitVector to buffer
  if(writeDiskBlock(1,buff) < 0){return -1;} // write updated bitVector to Disk (partition block 1)

  return 0;
}
//----------------------------------------------------




/*
 * return 1 if full, 0 if not full, -1 if problem occured
 */
int PartitionManager::isFull()
{
  char buff[64];  // data storage array
  for(int i = 0; i < 64; i++){buff[i] = 'c';} //initialize buffer to remove garbage memory
  if(readDiskBlock(1,buff) < 0){return -1;} // get bitVector from Disk (partition block 1)

  // set the Partition Manager's bit Vector to the values pulled from partition block 1
  dmBV->setBitVector((unsigned int*) buff); 
  for(int i = 0; i < myPartitionSize; i++)
  {
    if(dmBV->testBit(i) == 0){return 0;}  // if there is an empty spot return 0
  }
  return 1;  //if there are no empty blocks return 1

}
//----------------------------------------------------




/*
 * returns directory block num if name is found, 0 if does not exist, -1 otherwise
 */
int PartitionManager::findFile(char name, int dirBlock)
{

  //Needs to search through directory

  char buff[64];  // directory inode
  char dirPointer[4]; // pointer to next directory inode
  int nextDir; // integer version of pointer to next directory inode
  int n = 0; // index for dirPointer array

  if(readDiskBlock(dirBlock,buff) < 0){return -1;} // read the directory inode

  for(int j = 60; j < 64; j++){dirPointer[n] = buff[j]; n++;} // store the pointer to the next directory inode
  nextDir = CharToInt(dirPointer); // convert the stored pointer to integer
  //printf("nextDir: %d\n", nextDir);

  for(int i = 0; i < 64; i++) // loop until you either find (or don't find) the file
  { if(buff[i] == name && isdigit(buff[i+1]) ){return dirBlock;} }

  int found = 0;  // store return value of findFile in order to send it back up the chain
  if(nextDir != 0){found = findFile(name, nextDir);} // travel to next directory to continue the search
  if(found > 0){return found;} // if the file was found in a sub directory return blknum

  return 0; //if the file is not found return 0
}
//----------------------------------------------------




/* check if directory is empty 
 * return 0 if empty; 1 if not
*/
int PartitionManager::isEmptyDir(int dirBlock)
{
  char buff[64];  // directory inode
  char dirPointer[4]; // pointer to next directory inode
  int nextDir; // integer version of pointer to next directory inode
  int n = 0; // index for dirPointer array

  if(readDiskBlock(dirBlock,buff) < 0){return -1;} // read the directory inode

  for(int j = 59; j < 64; j++){dirPointer[n] = buff[j]; n++;} // store the pointer to the next directory inode
  nextDir = CharToInt(dirPointer); // convert the stored pointer to integer

  for(int i = 0; i < 60; i++) // loop to make sure ther is nothing in the directory
  { if(buff[i] != ' ' ){return 1;}}

  int found = 0;  // store return value of isEmpty in order to send it back up the chain
  if(nextDir != 0){found = isEmptyDir(nextDir);} // travel accross dir to continue search
  if(found > 0){return found;} // return past recursive returns

  return 0; // directory is empty

}
//----------------------------------------------------



/*
 * sets up a "fresh" directory with 0000 as the pointer and spaces
 */
int PartitionManager::initDirectory(int dirBlockLocation)
{
  char buff[64];
  for(int i = 0; i < 64; i++)
  {
    if(i > 59){buff[i] = '0';}
    else{buff[i] = ' ';}
  }

  if(writeDiskBlock(dirBlockLocation,buff) < 0){return -1;}
  return 0;
}


//----------------------------------------------------

/* print out this partition's BitVector */
void PartitionManager::printBitVec()
{
	for(int  i =0; i <myPartitionSize; i++)
	{
		cout << "BitVec[ " << i << " ]: ";
		if(dmBV->testBit(i) == 0){cout << "0" << endl;}
		else
		{
			cout << "1" << endl;
		}
	}
}
//----------------------------------------------------



int PartitionManager::readDiskBlock(int blknum, char *blkdata)
{ return myDM->readDiskBlock(myPartitionName, blknum, blkdata); }

int PartitionManager::writeDiskBlock(int blknum, char *blkdata)
{ return myDM->writeDiskBlock(myPartitionName, blknum, blkdata); }

int PartitionManager::getBlockSize() 
{ return myDM->getBlockSize(); }

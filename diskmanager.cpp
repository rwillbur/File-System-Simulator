#include "disk.h"
#include "diskmanager.h"
#include "DiskManagerHelper.h"
#include <iostream>




DiskManager::DiskManager(Disk *d, int partcount, DiskPartition *dp)
{

  myDisk = d; //disk
  myDisk->initDisk();  //initalize disk (might require check witch requires c++11)
  partCount = partcount; //number of partitions (at most 4)

  char buffer[64];  //  buffer for partition info
  for(int i=0; i<64; i++){buffer[i] = 'c';} // initalize buffer

  /* Set up Info for Default Partitions */
  int disk_Size = myDisk->getBlockCount() * myDisk->getBlockSize(); //get disk size

  int partition_Size = disk_Size / 4; //In Bytes

  int partition_block_count = partition_Size / 64;  //In Blocks

  char partition_names[4] = {'A', 'B', 'C', 'D'};  //For default partition creation


  /* If DiskPartition (dp) passed to constructor is empty: generate a default setup 
   *
   *	Default set up creates 4 partitions of equal size named A, B, C, D
   *
  */
  if(dp[0].partitionName == '\0') //checking the first partition is sufficient
  {
  	diskP = new DiskPartition[partCount];
  	for(int i=0; i < partCount; i++)
  	{
  		diskP[i].partitionName = partition_names[i];
  		diskP[i].partitionSize = partition_block_count;
  	}
  }
  else  /* else read in the partition information from the DiskPartition passed (dp) */
  {
  	diskP = new DiskPartition[partCount];
  	for(int i = 0; i < partCount; i++)
  	{
  		diskP[i].partitionName = dp[i].partitionName;
  		diskP[i].partitionSize = dp[i].partitionSize;
  	}
  }

  /* Need to write partition info to buffer so we can write it to the disk as a superblock */
  int k = 0;  //Buffer index
  int last_end = 0;  //end value of last partition (usually size)



  for(int i = 0; i < partCount; i++)  //set up buffer (write each partition's info to buffer)
  {

  	buffer[k] = diskP[i].partitionName;  //write partition name

  	k++; //inc buffer index

	//convert (size of current partition + size of last partition) to char array
	//   e.g.:    1st partition starts at 1  let's say its size is 30, 
	//		that would mean that the next partition starts at 31,
	//  	and ends at 30 + the size of the new partition
  	char* end = IntToChar(diskP[i].partitionSize + last_end);
  	char* begin;

  	if(i == 0 ){begin = IntToChar(1);}  //If first partition then begin is at block 1
  	else{begin = IntToChar(last_end+1);}  // else begin is the last partition end block + 1

  	buffer[k] = 's'; //Used to differentiate between start value and end value
  	k++; // inc buffer index

  	//  write the begin value to buffer
  	for(int l = 0; l < 4; l++)
  	{
  		buffer[k] = begin[l];
  		k++;
  	}

  	buffer[k] = 'e';//Used to differentiate between start value and end value
  	k++; // inc buffer index

  	//  write the end value to buffer
  	for(int j = 0; j < 4; j++)
  	{
  		buffer[k] = end[j];
  		k++;
  	}
  	last_end = diskP[i].partitionSize + last_end; // set last_end value

  }
  /* END LOOP */

  //Need to write buffer to DISK as superblock
  myDisk->writeDiskBlock(0,buffer);
  
  /* END CONSTRUCTOR */
}

  

/*
 *   returns: 
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char *blkdata)
{

  //  check that partition exists
 	if(!partition_exists_bool(diskP, partitionname, partCount)){return -3;}

 	//adjust for offset between partition indexing and disk class indexing
  int adjusted_block = adjusted(myDisk, partitionname, blknum);

  //write to disk
  int r = myDisk->readDiskBlock(adjusted_block,blkdata);

  //disk write error checks
  if(r == -1){return -1;}
  else if(r == -2){return -2;}

  return 0;
}


/*
 *   returns: 
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::writeDiskBlock(char partitionname, int blknum, char *blkdata)
{
  //  check that partition exists
 	if(!partition_exists_bool(diskP, partitionname, partCount)){return -3;}

 	//adjust for offset between partition indexing and disk class indexing
  int adjusted_block = adjusted(myDisk, partitionname, blknum); 

	//write to disk
  int r = myDisk->writeDiskBlock(adjusted_block,blkdata);

  //disk write error checks
  if(r == -1){return -1;}
  else if(r == -2){return -2;}

  return 0;
}



/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{

 //Iterate through diskP array until you match the partition
 //  If you match return that partition's size
 //  else return failure (-1) i.e. partition does not exist
  for(int i = 0; i < partCount; i++)
 	{
 		if(partitionname == diskP[i].partitionName)
 			{ return diskP[i].partitionSize; }
 	}
 	return -1;
}








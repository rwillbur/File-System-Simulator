/////////////////////////////////////
//
// Adrian Barberis
//
// April 3rd 2017
//
// diskmanager.cpp helper functions
//
//////////////////////////////////////



#include <stdio.h>
#include <iostream>
#include <algorithm>


#ifndef DM_HELP_H
#define DM_HELP_H

/*
	Returns the index of the partition if it exists 
*/
int partition_exists(DiskPartition *diskP, char partitionname, int partitionCount)
{
	if(diskP != NULL)
	{
		for(int i = 0; i < partitionCount; i++ )
		{
			if(partitionname == diskP[i].partitionName){return i;}
		}
	}
	return -1;
}


/*  
	If all you need is a quick check this version returns true if the partions exists false otherwise 
*/
bool partition_exists_bool(DiskPartition *diskP, char partitionname, int partitionCount)
{
	if(diskP != NULL)
	{
		for(int i = 0; i < partitionCount; i++ )
		{
			if(partitionname == diskP[i].partitionName){return true;}
		}
	}
	return false;
}



/*
	Convert Integer to Character
*/
char* IntToChar(int integer)
{
	char* num = new char[4];
	sprintf(num, "%d", integer);
	for(int i=0; i<4; i++)
	{
		if(num[i] == '\0'){num[i] = ' ';}
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



/*
	Adjust Partition block num to Disk block num

	partition block #13 is not nescecarrily disk bloack #13 it could be disk block #34
	This helper function adjust the blocknum value in order to write tot he correct disk block
*/
int adjusted(Disk *myDisk, char partitionname, int blknum)
{
	char buff[64];  //buffer for superblock
	char start[4]; // buffer for start integer (for later conversion)

	myDisk->readDiskBlock(0,buff); // Get the superblock (it holds the partition start/end information and we need that)

	 // get the index in the superblock buffer of the partition you want to write to
	// i.e. I want to write to partition 'B' I need to get the index of 'B' in the superblock so that I can find its start value
	int x = std::distance(buff, std::find(buff, buff + 64, partitionname));

	x += 2; //This is index of actual number  x = index of 'B' x+1 = index of 's' x+2 = index of the beginning of 'start' value
				//  If you look at the DISK1 file and how disk manager initializes the superblock this will make more sense

	//  I was getting really bad garbage values when I had this in a loop so since we always read 4 bytes I just did it by hand
	start[0] = buff[x];
	start[1] = buff[x+1];
	start[2] = buff[x+2];
	start[3] = buff[x+3];

	//return the adjusted block number
	return CharToInt(start) + blknum - 1;
}

#endif

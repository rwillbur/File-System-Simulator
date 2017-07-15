///////////////////////////////////
//
//  Adrian Barberis
//
//  COSC 4740
//  Apil 6th 2017
//
//  General Helper Functions
//
/////////////////////////////////


#include <ctype.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#ifndef GEN_HELP
#define GEN_HELP


/*
	Convert Integer to Character
*/
char* IntToChar2(int integer)
{
	char* num = new char[4];
	for(int i = 0; i < 4; i++){num[i] = '\0';}
	sprintf(num, "%d", integer);
	return num;
}
//----------------------------------------



/*
	Convert Character to Integer
*/
int CharToInt2(char* char_array)
{
	int i = 0;
	sscanf(char_array, "%d", &i);
	return i;
}
//----------------------------------------



/*   print an array  */
void printBuffer(char* buff, int leng)
{
	if(buff == NULL){return;}
	for(int i = 0; i < leng; i++)
	{
		std::cout << buff[i];
	}
}
//----------------------------------------


/* check file name to make sure it's legal */
bool nameCheck(char * name, int leng)
{
	if(!isalpha(name[leng-1])){return false;}
	for(int i = 0; i < leng; i++)
	{
		if(i % 2 == 0){ if(name[i] != '/'){ return false;} }
		else if(i % 2  != 0){ if(isalpha(name[i]) == false){ return false;} }
	}
	return true;
}
//----------------------------------------


/* convert a path to a vector made up of its constituent alpahbet char */
std::vector<char> convertToVec(char* name, int nameLen)
{
	std::vector<char> path;
	for(int i = 0; i < nameLen; i++)
	{
		if(isalpha(name[i])){path.push_back(name[i]);}
	}
	return path;
}
//----------------------------------------


/* return the location of a fileInode or directory Inode 
 * For use with directories only
*/
int getLocation(char* buffer, char name)
{
	char location[4];
	for(int i = 0; i < 64; i++)
	{
		if(buffer[i] == name && isdigit(buffer[i+1]))
		{
			int n = i+1;
			for(int j = 0; j < 4; j++)
			{	
				location[j] = buffer[n];
				n++;
			}
			break;
		}
	}	
	int k = CharToInt2(location);
	return k;
}
//----------------------------------------


/* delete an entry from a directory */
void deleteEntry(char* directory, char name)
{
	for(int i = 0; i < 64; i++)
	{
		if(directory[i] == name && isdigit(directory[i+1]))
		{
			int n = i;
			for(; n < i+6; n++)
			{
				directory[n] = ' ';
			}
			break;
		}
	}
}
//----------------------------------------



/* reset a block to all 'c' */
void clearBlock(char* block)
{
	for(int i = 0; i < 64; i++)
	{
		block[i] = 'c';
	}
}
//----------------------------------------



/* get first 3 direct addresses and store them in a vector and return it */
std::vector<int> getDataLoc(char* fileInode)
{
	std::vector<int> locations;
	char loc[4];
	int address;
	for(int i = 6; i < 22; i+=4)
	{
		int n = i;
		for(int j = 0; j < 4; j++)
		{
			loc[j] = fileInode[n];
			n++;
		}
		address = CharToInt2(loc);
		if(address != 0){locations.push_back(address);}
	}
	return locations;
}
//----------------------------------------



/* get all direct addresses in the indirect block */
std::vector<int> getIndirectLoc(char* indirectBlock)
{
	std::vector<int> locations;
	char loc[4];
	int address;
	for(int i = 0; i < 64; i+=4)
	{
		int n = i;
    if(isdigit(indirectBlock[n]))
    {
		  for(int j = 0; j < 4; j++)
		  {
         loc[j] = indirectBlock[n];
			   n++;
		  }
		  address = CharToInt2(loc);
		  if(address != 0){locations.push_back(address);}
    }
	}
	return locations;
}
//----------------------------------------



/* if file is indeed a file or a directory */
bool isFile(char* fInode)
{
	if(fInode[1] != 'f'){return false;}
	return true;
}
//----------------------------------------



/* return the size of a file 
 * Use with fileInode only
*/
int getFileSize(char* inode)
{
	char size[4];
	int n = 0;
	for(int i = 2; i < 6; i++)
	{
		size[n] = inode[i];
		n++;
	}
	return CharToInt2(size);
}
//----------------------------------------



/* write the integer size to the fileInode */
void writeSize(char* inode, int size)
{
	char* csize;
	int n = 0;
	csize = IntToChar2(size);
	for(int i = 2; i < 6; i++)
	{
		inode[i] = csize[n];
		n++;
	}

}
//----------------------------------------



/* get the location of the indirect addresses */
int getIndirectBlknum(char* inode)
{
	char loc[4];
	int n = 0;
	for(int i = 18; i < 22; i++)
		{loc[n] = inode[i]; n++;}
	return CharToInt2(loc);
}
//----------------------------------------



/* write a direct address to a fileInode */
void writeDirectAddress(int blknum, int addressNum, char* inode)
{
	if(addressNum > 4 || addressNum <= 0){return;}
	char* blk = IntToChar2(blknum);
	int n = 0;
	if(addressNum == 1)
	{
		for(int i = 6; i < 10; i++)
			{inode[i] = blk[n]; n++;}
	}
	if(addressNum == 2)
	{
		for(int i = 10; i < 14; i++)
			{inode[i] = blk[n]; n++;}
	}
	if(addressNum == 3)
	{
		for(int i = 14; i < 18; i++)
			{inode[i] = blk[n]; n++;}
	}
	if(addressNum == 4)
	{
		for(int i = 18; i < 22; i++)
			{inode[i] = blk[n]; n++;}
	}

}
//----------------------------------------



/* write an a direct address to an indirect inode */
void writeIndirectAddress(int blknum, char* indirectNode)
{

	char* blk = IntToChar2(blknum);
	for(int i = 0; i < 64; i++)
	{
		if(indirectNode[i] == 'c')
		{
			int n = i;
			for(int j = 0; j < 4; j++)
			{
				indirectNode[n] = blk[j];
				n++;
			}
			break;
		}
	}
}
//----------------------------------------



/* fill a block with null */
void nullBlock(char* buffer)
{
	for(int i = 0; i < 64; i++){buffer[i] = '\0';}
}
//----------------------------------------


#endif







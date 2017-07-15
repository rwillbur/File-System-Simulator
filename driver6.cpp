/* Driver 6*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "client.h"

using namespace std;

/*
  This driver will test the getAttribute() and setAttribute()
  functions. You need to complete this driver according to the
  Attribute you have implemented in your file system, before
  testing your program.
*/

int main()
{

  Disk *d = new Disk(300, 64, const_cast<char *>("DISK1"));
  DiskPartition *dp = new DiskPartition[3];

  dp[0].partitionName = 'A';
  dp[0].partitionSize = 100;
  dp[1].partitionName = 'B';
  dp[1].partitionSize = 75;
  dp[2].partitionName = 'C';
  dp[2].partitionSize = 105;

  DiskManager *dm = new DiskManager(d, 3, dp);
  FileSystem *fs1 = new FileSystem(dm, 'A');
  FileSystem *fs2 = new FileSystem(dm, 'B');
  FileSystem *fs3 = new FileSystem(dm, 'C');
  Client *c1 = new Client(fs1);
  Client *c2 = new Client(fs2);
  Client *c3 = new Client(fs3);
  Client *c4 = new Client(fs1);
  Client *c5 = new Client(fs2);



  int r;
  string s; // get attribute returns a string

  r = c1->myFS->setAttribute(const_cast<char *>("/e/f"),4, const_cast<char *>("E"), 1);
  cout << "rv from setAttribute /e/f (fs1 / c1) is " << r <<(r==0 ? " correct": " fail") <<endl;
  s = c1->myFS->getAttribute(const_cast<char *>("/e/f"), 4);
  cout << "rv from getAttribute /e/f (fs1 / c1) is " <<(s!="NULL" ? " correct": " fail") <<endl<<endl;
  cout << s << endl << endl;

  r = c4->myFS->setAttribute(const_cast<char *>("/e/b"),4, const_cast<char *>("PS1"), 3);
  cout << "rv from setAttribute /e/b (fs1 / c4) is " << r <<(r==0 ? " correct": " fail") <<endl;
  s = c4->myFS->getAttribute(const_cast<char *>("/e/b"), 4);
  cout << "rv from getAttribute /e/b (fs1 / c4) is "<<(s!= "NULL" ? " correct": " fail") <<endl<<endl;
  cout << s << endl << endl;

  s = c1->myFS->getAttribute(const_cast<char *>("/p"), 2);  //should fail
  cout << "rv from getAttribute /p (fs1 / c1 ) is " << s <<(s=="NULL" ? " correct file does not exist": " fail") <<endl;
  r = c4->myFS->setAttribute(const_cast<char *>("/p"),2,  const_cast<char *>("Y"), 1);  //should failed!
  cout << "rv from setAttribute /p (fs1 / c4) is " << r <<(r==-2 ? " correct file does not exist": " fail") <<endl;
  
  r = c2->myFS->setAttribute(const_cast<char *>("/f"),2, const_cast<char *>("A"), 1);
  cout << "rv from setAttribute /f (fs2 / c2) is " << r <<(r==-3 ? " correct /f is a directory": " fail") <<endl;
   s = c2->myFS->getAttribute(const_cast<char *>("/f"), 2);
  cout << "rv from getAttribute /f (fs2 / c2 ) is "<< s <<(s=="NULL" ? " correct /f is a directory": " fail") <<endl;

  cout << "\n\n";

  r = c5->myFS->setAttribute(const_cast<char *>("/z"),2, const_cast<char *>("B"), 1);
  cout << "rv from setAttribute /z (fs2 / c5) is " << r <<(r==0 ? " correct": " fail") <<endl;
  s = c5->myFS->getAttribute(const_cast<char *>("/z"), 2);
  cout << "rv from getAttribute /z (fs2 / c5) is "<<(s!= "NULL" ? " correct": " fail") <<endl<<endl;
  cout << s << endl << endl;

  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/l"),10, const_cast<char *>("C"), 1);
  cout << "rv from setAttribute /o/o/o/a/l (fs3 / c3 ) is " << r <<(r==-2 ? " correct file does not exist": " fail") <<endl;
  s = c3->myFS->getAttribute(const_cast<char *>("/o/o/o/a/l"), 10);
  cout << "rv from getAttribute /o/o/o/a/l (fs3 / c3) is " << s <<(s=="NULL" ? " correct file does not exist": " fail") <<endl;

  r = c2->myFS->setAttribute(const_cast<char *>("/e/b/d/x"),8, const_cast<char *>("D"), 1);
  cout << "rv from setAttribute /e/b/d/x (fs2 / c2 ) is " << r <<(r==0 ? " correct": " fail") <<endl;
  s = c2->myFS->getAttribute(const_cast<char *>("/e/b/d/x"), 8);
  cout << "rv from getAttribute /e/b/d/x (fs2 / c2) is " << s <<(s!= "NULL" ? " correct": " fail") <<endl<<endl;
  cout << s << endl << endl;
  
  r = c5->myFS->setAttribute(const_cast<char *>("/z"),2, const_cast<char *>("X"), 1);
  cout << "rv from setAttribute /z is " << r <<(r==0 ? " correct": " fail") <<endl;
  s = c5->myFS->getAttribute(const_cast<char *>("/z"), 2);
  cout << "rv from getAttribute /z is "<<(s!= "NULL" ? " correct": " fail") <<endl;
  cout << s << endl << endl;

  r = c3->myFS->setAttribute(const_cast<char *>("o/o/o/a/l"),9, const_cast<char *>("YUP"), 3);
  cout << "rv from setAttribute o/o/o/a/l (fs3 / c3 ) is " << r <<(r==-1 ? " correct invalid filename": " fail") <<endl;

  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/$/a/d"),10, const_cast<char *>("eh"), 2);
  cout << "rv from setAttribute /o/o/$/a/d (fs3 / c3 ) is " << r <<(r==-1 ? " correct invalid filename": " fail") <<endl;

  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/d"),10, const_cast<char *>("adadasdad"), 9);
  cout << "rv from setAttribute /o/o/o/a/d (fs3 / c3 ) is " << r <<(r==-4 ? " correct bad extension length: adadasdad": " fail") <<endl;

  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/d"),10, const_cast<char *>(""), 0);
  cout << "rv from setAttribute /o/o/o/a/d (fs3 / c3 ) is " << r <<(r==-4 ? " correct bad extension: null character ": " fail") <<endl;
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/d"),10, const_cast<char *>(" "), 1);
  cout << "rv from setAttribute /o/o/o/a/d (fs3 / c3 ) is " << r <<(r==-4 ? " correct bad extension: space chararcter ": " fail") <<endl;

  return 0;
}
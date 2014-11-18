

#ifndef _MEM_RDWR_H_
#define _MEM_RDWR_H_

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <osa_file.h>

#define CMD_MEM_WR      (0)
#define CMD_MEM_RD      (1)
#define CMD_FILE_SAVE   (2)
#define CMD_FILE_LOAD   (3)

typedef struct
{
  unsigned int cmdType;
  unsigned int memAddr;
  unsigned int memSize;
  unsigned int mmapMemAddr;
  unsigned int mmapMemSize;  
  unsigned int memOffset;
  unsigned int wrMemValue;
  unsigned int numCols;
  
  char filename[1024];
  
  int    memDevFd;
  volatile unsigned int *pMemVirtAddr;
  
} MEM_RDWR_Ctrl;

#define MMAP_MEM_PAGE_ALIGN  (32*1024-1) 
#define RDWR_ADDR_ALIGN      (4-1)
#define MAX_READ_SIZE        (10*1024)

int xstrtoi(char *hex);      // hex string to integer

#endif


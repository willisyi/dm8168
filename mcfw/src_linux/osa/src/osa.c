
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <osa.h>



int OSA_attachSignalHandler(int sigId, void (*handler)(int ) )
{
  struct sigaction sigAction;

  /* insure a clean shutdown if user types ctrl-c */
  sigAction.sa_handler = handler;
  sigemptyset(&sigAction.sa_mask);
  sigAction.sa_flags = 0;
  sigaction(sigId, &sigAction, NULL);
  
  return OSA_SOK;
}

Uint32 OSA_getCurTimeInMsec()
{
  static int isInit = FALSE;
  static Uint32 initTime=0;
  struct timeval tv;

  if(isInit==FALSE)
  {
      isInit = TRUE;
      
      if (gettimeofday(&tv, NULL) < 0) 
        return 0;

      initTime = (Uint32)(tv.tv_sec * 1000u + tv.tv_usec/1000u);
  }

  if (gettimeofday(&tv, NULL) < 0) 
    return 0;

  return (Uint32)(tv.tv_sec * 1000u + tv.tv_usec/1000u)-initTime;
}

void   OSA_waitMsecs(Uint32 msecs)
{
  #if 1
  struct timespec delayTime, elaspedTime;
  
  delayTime.tv_sec  = msecs/1000;
  delayTime.tv_nsec = (msecs%1000)*1000000;

  nanosleep(&delayTime, &elaspedTime);
  #else
  usleep(msecs*1000);
  #endif  
}

Int32 OSA_mapMem(UInt32 physAddr, UInt32 memSize , Ptr *pMemVirtAddrPtr)
{
    unsigned int mmapMemAddr;
    unsigned int mmapMemSize;
    unsigned int memOffset;
    int    memDevFd;
    volatile unsigned int *pMemVirtAddr;

    *pMemVirtAddrPtr = NULL;
    memDevFd = open("/dev/mem",O_RDWR|O_SYNC);

    if(memDevFd < 0)
    {
      printf(" ERROR: /dev/mem open failed !!!\n");
      return -1;
    }

    memOffset   = physAddr & (sysconf(_SC_PAGE_SIZE) - 1);
    mmapMemAddr = physAddr - memOffset;
    mmapMemSize = OSA_align((memSize + memOffset),sysconf(_SC_PAGE_SIZE));
    printf("mmap of [0x%x:%d]\n",mmapMemAddr,mmapMemSize);
    pMemVirtAddr = mmap(
           (void *)NULL,
                   mmapMemSize,
                  (PROT_READ|PROT_WRITE),
                   MAP_SHARED,
                   memDevFd,
                   mmapMemAddr);

   close(memDevFd);
   if (pMemVirtAddr == MAP_FAILED)
   {
     printf(" ERROR: mmap() failed !!!\n");
     return -1;
   }
    *pMemVirtAddrPtr = (Ptr)((UInt32)pMemVirtAddr + memOffset);
    printf("mmap virt addresss:%p\n",*pMemVirtAddrPtr);
    return 0;
}

Int32 OSA_unmapMem(Ptr pMemVirtAddrPtr,UInt32 memSize)
{
    Int32 status;
    unsigned int mmapMemAddr;
    unsigned int mmapMemSize;
    unsigned int memOffset;

    memOffset   = ((UInt32)pMemVirtAddrPtr) & (sysconf(_SC_PAGE_SIZE) - 1);
    mmapMemAddr = ((UInt32)pMemVirtAddrPtr) - memOffset;
    mmapMemSize = OSA_align((memSize + memOffset),sysconf(_SC_PAGE_SIZE));

    printf("munmap of [0x%x:%d]\n",mmapMemAddr,mmapMemSize);
    status = munmap((void*)mmapMemAddr, mmapMemSize);

    return status;
}

static char xtod(char c) {
  if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='F') return c-'A'+10;
  if (c>='a' && c<='f') return c-'a'+10;
  return c=0;        // not Hex digit
}
  
static int HextoDec(char *hex, int l)
{
  if (*hex==0) 
    return(l);

  return HextoDec(hex+1, l*16+xtod(*hex)); // hex+1?
}
  
int xstrtoi(char *hex)      // hex string to integer
{
  return HextoDec(hex,0);
}

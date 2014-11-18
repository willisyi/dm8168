

#include <osa.h>
#include <sys/time.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


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

Uint8 *OSA_dmaMapMem(Uint8 *physAddr, Uint32 size)
{
  int memDevFd;
  Uint8 *virtAddr;

  memDevFd = open("/dev/mem",O_RDWR|O_SYNC);

  if(memDevFd < 0)
  {
    printf(" ERROR: /dev/mem open failed !!!\n");
    return NULL;
  }

	virtAddr = mmap(
	        (void	*)physAddr,
	        size,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					memDevFd,
					(unsigned int)physAddr
					);

  close(memDevFd);

	if (virtAddr==NULL)
	{
		printf(" ERROR: mmap() failed !!!\n");
		return NULL;
	}

  return virtAddr;
}

int OSA_dmaUnmapMem(Uint8 *virtAddr, Uint32 size)
{
  if(virtAddr)
    munmap((void*)virtAddr, size);

  return 0;
}

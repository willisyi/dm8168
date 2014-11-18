/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2010 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <remote_debug_client.h>

STATIC char *gRemoteDebug_corePrefix[] =
{
  REMOTE_DEBUG_CORE_PREFIX_C6XDSP,
  REMOTE_DEBUG_CORE_PREFIX_M3VIDEO,
  REMOTE_DEBUG_CORE_PREFIX_M3VPSS
};

STATIC RemoteDebug_ClientObj gRemoteDebug_clientObj;

STATIC int RemoteDebug_mapMem()
{
  gRemoteDebug_clientObj.memDevFd = open("/dev/mem",O_RDWR|O_SYNC);
  if(gRemoteDebug_clientObj.memDevFd < 0)
  {
    printf(" ERROR: /dev/mem open failed !!!\n");
    return -1;
  }

	gRemoteDebug_clientObj.coreObjVirtBaseAddr = mmap(
	        (void	*)gRemoteDebug_clientObj.coreObjPhysBaseAddr,
	        gRemoteDebug_clientObj.coreObjTotalMemSize,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					gRemoteDebug_clientObj.memDevFd,
					gRemoteDebug_clientObj.coreObjPhysBaseAddr
					);

	if (gRemoteDebug_clientObj.coreObjVirtBaseAddr==NULL)
	{
		printf(" ERROR: mmap() failed !!!\n");
		return -1;
	}

  return 0;
}

STATIC int RemoteDebug_unmapMem()
{
  if(gRemoteDebug_clientObj.coreObjVirtBaseAddr)
    munmap((void*)gRemoteDebug_clientObj.coreObjVirtBaseAddr, gRemoteDebug_clientObj.coreObjTotalMemSize);

  if(gRemoteDebug_clientObj.memDevFd >= 0)
    close(gRemoteDebug_clientObj.memDevFd);

  return 0;
}

STATIC int RemoteDebug_clientInit(unsigned int coreObjPhysBaseAddr, int resetShm)
{
  unsigned int coreId;
  int status;
  volatile RemoteDebug_CoreObj *pCoreObj;

  memset(&gRemoteDebug_clientObj, 0, sizeof(gRemoteDebug_clientObj));

  gRemoteDebug_clientObj.coreObjPhysBaseAddr = coreObjPhysBaseAddr;
  gRemoteDebug_clientObj.coreObjTotalMemSize = sizeof(RemoteDebug_CoreObj)*REMOTE_DEBUG_CORE_ID_MAX;

  status = RemoteDebug_mapMem();
  if(status!=0)
    return -1;

  for(coreId=0; coreId<REMOTE_DEBUG_CORE_ID_MAX; coreId++)
  {
    gRemoteDebug_clientObj.pCoreObj[coreId] = (RemoteDebug_CoreObj*)(gRemoteDebug_clientObj.coreObjVirtBaseAddr + sizeof(RemoteDebug_CoreObj)*coreId);

    if(resetShm)
    {
      pCoreObj = gRemoteDebug_clientObj.pCoreObj[coreId];

      pCoreObj->serverIdx = 0;
      pCoreObj->clientIdx = 0;
      pCoreObj->serverFlags[0] = pCoreObj->serverFlags[1] = 0;
      pCoreObj->clientFlags[0] = pCoreObj->clientFlags[1] = 0;
      pCoreObj->reserved = 0;
      pCoreObj->headerTag = REMOTE_DEBUG_HEADER_TAG;
    }

    printf(" %s Remote Debug Shared Memory @ 0x%08x\n", gRemoteDebug_corePrefix[coreId], (unsigned int)gRemoteDebug_clientObj.coreObjPhysBaseAddr+sizeof(RemoteDebug_CoreObj)*coreId);
  }

  return 0;
}

STATIC int RemoteDebug_clientDeInit()
{
  RemoteDebug_unmapMem();

  return 0;
}

STATIC int RemoteDebug_putChar(unsigned int coreObjPhysBaseAddr, unsigned int coreId, char ch)
{
    volatile RemoteDebug_CoreObj *pCoreObj;
    int status;

    status = RemoteDebug_clientInit(coreObjPhysBaseAddr, 0);
    if(status!=0)
      return status;

    if(coreId>=REMOTE_DEBUG_CORE_ID_MAX)
        return -1;

    pCoreObj = gRemoteDebug_clientObj.pCoreObj[coreId];

    if(pCoreObj->headerTag == REMOTE_DEBUG_HEADER_TAG)
    {
      pCoreObj->serverFlags[0] = (REMOTE_DEBUG_FLAG_TYPE_CHAR | ch);
    }

    RemoteDebug_clientDeInit();

    return 0;
}

STATIC int RemoteDebug_putStr(unsigned int coreObjPhysBaseAddr, unsigned int coreId, char *str)
{
    volatile RemoteDebug_CoreObj *pCoreObj;
    int status,length;

    status = RemoteDebug_clientInit(coreObjPhysBaseAddr, 0);
    if(status!=0)
        return status;

    if(coreId>=REMOTE_DEBUG_CORE_ID_MAX)
        return -1;

    pCoreObj = gRemoteDebug_clientObj.pCoreObj[coreId];

    length = strlen(str);
    if(pCoreObj->headerTag == REMOTE_DEBUG_HEADER_TAG)
    {
        if(length>0 && length<REMOTE_DEBUG_PARAM_BUF_SIZE)
        {
            strcpy((char*)pCoreObj->serverParamBuf , str );
            pCoreObj->serverFlags[1] = length;
            pCoreObj->serverFlags[0] = (REMOTE_DEBUG_FLAG_TYPE_STRING);
        }
    }
    RemoteDebug_clientDeInit();

    return 0;
}


STATIC int RemoteDebug_getChar(unsigned int coreObjPhysBaseAddr, unsigned int coreId, char expectedChar, unsigned int timeout)
{
    volatile RemoteDebug_CoreObj *pCoreObj;
    volatile int value;
    char receivedChar;

    int status;

    status = RemoteDebug_clientInit(coreObjPhysBaseAddr, 0);
    if(status!=0)
      return status;

    if(coreId>=REMOTE_DEBUG_CORE_ID_MAX)
        return -1;

    pCoreObj = gRemoteDebug_clientObj.pCoreObj[coreId];

    while(1)
    {
        if(pCoreObj->headerTag != REMOTE_DEBUG_HEADER_TAG)
            return -1;

        value = pCoreObj->clientFlags[0];
        if(value & REMOTE_DEBUG_FLAG_TYPE_CHAR)
        {
            receivedChar = value & 0xFF;
            pCoreObj->clientFlags[0] = 0;
            if(receivedChar==expectedChar)
            {
              printf(" %s Received character '%c'\n", gRemoteDebug_corePrefix[coreId], receivedChar );
              break;
            }
        }
        if(timeout==(unsigned int)-1)
            OSA_waitMsecs(10);
        else
        	return -1;
    }

    RemoteDebug_clientDeInit();

    return 0;
}

STATIC int RemoteDebug_clientGetLine(unsigned int coreId, char * pString)
{
    unsigned int numBytes, copyBytes=0, serverIdx, clientIdx;
    volatile unsigned char *pSrc;
    volatile unsigned char curChar;

    volatile RemoteDebug_CoreObj *pCoreObj = gRemoteDebug_clientObj.pCoreObj[coreId];

    if(pCoreObj->headerTag != REMOTE_DEBUG_HEADER_TAG)
        return -1;

    serverIdx = pCoreObj->serverIdx;
    clientIdx = pCoreObj->clientIdx;

    if(clientIdx>serverIdx)
        numBytes = (REMOTE_DEBUG_LOG_BUF_SIZE - clientIdx) + serverIdx;
    else
        numBytes = serverIdx - clientIdx;

    if(numBytes>0)
    {
        pSrc = &pCoreObj->serverLogBuf[0];

        for(copyBytes=0; copyBytes<numBytes; copyBytes++)
        {
          if(clientIdx>=REMOTE_DEBUG_LOG_BUF_SIZE)
            clientIdx = 0;

          curChar = pSrc[clientIdx];

          clientIdx++;

          if(curChar==0xA0||curChar=='\r'||curChar=='\n'||curChar==0||copyBytes>=REMOTE_DEBUG_LINE_BUF_SIZE)
            break;
          else
            *pString++ = curChar;
        }

        pCoreObj->clientIdx = clientIdx;
    }

    *pString = 0;

    return copyBytes;
}

int RemoteDebug_clientRun(unsigned int coreObjPhysBaseAddr)
{
  unsigned int coreId;
  int numBytes, status;
  unsigned int doWait;

  status = RemoteDebug_clientInit(coreObjPhysBaseAddr, 1);
  if(status!=0)
    return status;

  while(1)
  {
    doWait = 1;
#if !defined (TI_8107_BUILD)
    for(coreId=0; coreId<REMOTE_DEBUG_CORE_ID_MAX; coreId++)
    {
#else
    for(coreId=1; coreId<REMOTE_DEBUG_CORE_ID_MAX; coreId++)
    {
#endif
        do {
            numBytes = RemoteDebug_clientGetLine( coreId, gRemoteDebug_clientObj.lineBuf );
            if(numBytes>0)
            {
                printf(" %s %s\n", gRemoteDebug_corePrefix[coreId], gRemoteDebug_clientObj.lineBuf );
                doWait = 0;
            }
        }
        while(numBytes);
    }
    if(doWait)
      OSA_waitMsecs(10);
  }

  RemoteDebug_clientDeInit();

  return 0;
}

STATIC int RemoteDebug_printCmdLineArgs(char *str)
{
  printf(" \n");
  printf(" Remote Debug Client Utility, (c) Texas Instruments 2010\n");
  printf(" \n");
  printf(" %s <remote debug shared memory physical address in Hex> \n", str);
  printf(" \n");
  printf(" The shared memory physical address MUST be 4KB aligned. \n");
  printf(" \n");

  return 0;
}

unsigned int RemoteDebug_getCoreId(char *coreName)
{
    if(strcmp(coreName,"c6xdsp")==0)
        return REMOTE_DEBUG_CORE_ID_C6XDSP;
    if(strcmp(coreName,"m3vpss")==0)
        return REMOTE_DEBUG_CORE_ID_M3VPSS;
    if(strcmp(coreName,"m3video")==0)
        return REMOTE_DEBUG_CORE_ID_M3VIDEO;

    return REMOTE_DEBUG_CORE_ID_M3VPSS;
}

int main(int argc, char **argv)
{
  unsigned int remoteDebugShmPhysAddr, coreId;

  if(argc<2)
  {
    RemoteDebug_printCmdLineArgs(argv[0]);
    exit(0);
  }

  remoteDebugShmPhysAddr = xstrtoi(argv[1]);
  if(remoteDebugShmPhysAddr==0 || (remoteDebugShmPhysAddr & 0xFFF))
  {
    RemoteDebug_printCmdLineArgs(argv[0]);
    exit(0);
  }

  if(argc==5 || argc==4)
  {
    if(argc==5)
        coreId = RemoteDebug_getCoreId(argv[4]);
    else
        coreId = REMOTE_DEBUG_CORE_ID_M3VPSS;

    if(strcmp(argv[2], "--putch")==0)
    {
      RemoteDebug_putChar(remoteDebugShmPhysAddr, coreId, argv[3][0]);
    }
    if(strcmp(argv[2], "--putstr")==0)
    {
      RemoteDebug_putStr(remoteDebugShmPhysAddr, coreId, argv[3]);
    }
    if(strcmp(argv[2], "--waitch")==0)
    {
      RemoteDebug_getChar(remoteDebugShmPhysAddr, coreId, argv[3][0], (unsigned int)-1);
    }
  }
  else
  {
    RemoteDebug_clientRun(remoteDebugShmPhysAddr);
  }

  return 0;
}

STATIC void   OSA_waitMsecs(unsigned int msecs)
{
  struct timespec delayTime, elaspedTime;

  delayTime.tv_sec  = msecs/1000;
  delayTime.tv_nsec = (msecs%1000)*1000000;

  nanosleep(&delayTime, &elaspedTime);
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

STATIC int xstrtoi(char *hex)      // hex string to integer
{
  return HextoDec(hex,0);
}



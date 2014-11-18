/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2010 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _REMOTE_DEBUG_CLIENT_
#define _REMOTE_DEBUG_CLIENT_

#include <remote_debug_if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define STATIC static 

#define REMOTE_DEBUG_CORE_PREFIX_M3VPSS    "[m3vpss ]"
#define REMOTE_DEBUG_CORE_PREFIX_M3VIDEO   "[m3video]"
#define REMOTE_DEBUG_CORE_PREFIX_C6XDSP    "[c6xdsp ]"

#define REMOTE_DEBUG_LINE_BUF_SIZE         (1024)

typedef struct {

  unsigned int coreObjPhysBaseAddr;
  unsigned char *coreObjVirtBaseAddr;
  unsigned int coreObjTotalMemSize;
  unsigned int memDevFd;
    
  volatile RemoteDebug_CoreObj *pCoreObj[REMOTE_DEBUG_CORE_ID_MAX];
  
  char lineBuf[REMOTE_DEBUG_LINE_BUF_SIZE];

} RemoteDebug_ClientObj;

STATIC int xstrtoi(char *hex);
STATIC void OSA_waitMsecs(unsigned int msecs);

#endif /* _REMOTE_DEBUG_CLIENT_ */


/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/



#ifndef _SD_DEMO_SCD_BIT_WR_H_
#define _SD_DEMO_SCD_BIT_WR_H_

#include <osa.h>
#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <link_api/ipcLink.h>
//#define IPC_BITS_DEBUG
#define MAX_IN_STR_SIZE                      (128)
typedef struct {

    UInt32 totalDataSize;
    UInt32 numKeyFrames;
    UInt32 numFrames;
    UInt32 maxWidth;
    UInt32 minWidth;
    UInt32 maxHeight;
    UInt32 minHeight;
    UInt32 maxLatency;
    UInt32 minLatency;

} SD_Demo_Scd_ChInfo;

typedef struct {

    OSA_ThrHndl wrThrHndl;
    OSA_SemHndl wrSem;
    Bool exitWrThr;
    Bool isWrThrStopDone;

    SD_Demo_Scd_ChInfo chInfo[16];

    UInt32 statsStartTime;

    Bool fileWriteEnable;
    char fileWriteName[512];
    UInt32  chId;
    UInt32  fileWriteChn;
} SD_Demo_Scd_Ctrl;


extern SD_Demo_Scd_Ctrl gSD_Demo_Scd_ctrl;


int SD_Demo_ScdResetStatistics();
int SD_Demo_ScdPrintStatistics(Bool resetStats);

int SD_Demo_ScdBitsWriteCreate();
int SD_Demo_ScdBitsWriteDelete();
Void SD_Demo_ScdBitsWriteStop();

Void SD_Demo_ipcBitsInitCreateParams_BitsInHLOSVcap(IpcBitsInLinkHLOS_CreateParams *cp);
#endif

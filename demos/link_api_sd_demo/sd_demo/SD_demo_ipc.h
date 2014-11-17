/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SD_DEMO_IPC_H_
#define _SD_DEMO_IPC_H_

#include <sys/mman.h>

#include <osa.h>
#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>
#include <link_api/ipcLink.h>

//#define CUSTOM_SD_DEMO
//#define IPC_BITS_DEBUG

#define MMAP_MEM_PAGEALIGN         (4*1024-1)

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

} SD_Demo_ChInfo;

typedef struct SD_Demo_IpcFramesCtrlThrObj {
    OSA_ThrHndl thrHandleFramesInOut;
    OSA_SemHndl framesInNotifySem;
    volatile Bool exitFramesInOutThread;
} SD_Demo_IpcFramesCtrlThrObj;

typedef struct SD_Demo_IpcFramesCtrl {
    Bool  noNotifyFramesInHLOS;
    Bool  noNotifyFramesOutHLOS;
    SD_Demo_IpcFramesCtrlThrObj  thrObj;
} SD_Demo_IpcFramesCtrl;

typedef struct {

    OSA_ThrHndl wrThrHndl;
    OSA_SemHndl wrSem;
    Bool exitWrThr;
    Bool isWrThrStopDone;

    SD_Demo_ChInfo chInfo[16];

    UInt32 statsStartTime;

    Bool fileWriteEnable;
    char fileWriteName[512];
    int  fileWriteChn;
    SD_Demo_IpcFramesCtrl ipcFrames;

    SD_Demo_ChInfo chFrameInfo[16];
    FILE *fp;
    Bool fileFrameWriteEnable;
    char fileFrameWriteName[512];
    int  fileFrameWriteChn;
    int fileFrameWriteState;
} SD_Demo_Ipc_Ctrl;

typedef struct
{
  unsigned int memAddr;
  unsigned int memSize;
  unsigned int mmapMemAddr;
  unsigned int mmapMemSize;  
  unsigned int memOffset;

  int    memDevFd;
  volatile unsigned int *pMemVirtAddr;
  
} SD_Demo_mMapCtrl;

extern SD_Demo_Ipc_Ctrl gSD_Demo_Ipc_Ctrl;


Int32 SD_Demo_resetStatistics();
Int32 SD_Demo_printStatistics(Bool resetStats);

Int32 SD_Demo_bitsWriteCreate();
Int32 SD_Demo_bitsWriteDelete();

Int32 SD_Demo_ipcFramesCreate();
Int32 SD_Demo_ipcFramesDelete();
Void  SD_Demo_ipcFramesStop(void);
Void  SD_Demo_ipcFramesInSetCbInfo (IpcFramesInLinkHLOS_CreateParams *createParams);

Void  SD_Demo_ipcFramesFileWriteCreate();
Void  SD_Demo_ipcFramesFileOpen();
//Void  SD_Demo_ipcFrameFileWrite(VIDEO_FRAMEBUF_LIST_S * pFrameBufList);

Int32 SD_Demo_mMap(UInt32 physAddr, Uint32 memSize , UInt32 * pMemVirtAddr);
Int32 SD_Demo_unmapMem();
#endif

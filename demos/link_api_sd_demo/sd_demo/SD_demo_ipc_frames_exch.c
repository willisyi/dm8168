/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file demo_vcap_venc_vdis_ipc_frames_exch.c
    \brief
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>
#include "SD_demo_ipc.h"
#include <link_api/ipcLink.h>
#include "SD_demo.h"
#include <demos/link_api_sd_demo/sd_demo/SD_demo_ipc.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>
#include <osa_dma.h>

SD_Demo_mMapCtrl gSD_Demo_MMapCtrl;
#define FILE_WRITE_STOPPED  (0)
#define FILE_WRITE_RUNNING  (1)

#define SD_DEMO_IPC_FRAMES_NONOTIFYMODE_FRAMESIN                          (TRUE)
#define SD_DEMO_IPC_FRAMES_NONOTIFYMODE_FRAMESOUT                         (TRUE)

#define SD_DEMO_IPCFRAMES_SENDRECVFXN_TSK_PRI                             (2)
#define SD_DEMO_IPCFRAMES_SENDRECVFXN_TSK_STACK_SIZE                      (0) /* 0 means system default will be used */
#define SD_DEMO_IPCFRAMES_SENDRECVFXN_PERIOD_MS                           (16)


#define SD_DEMO_IPCFRAMES_INFO_PRINT_INTERVAL                             (8192)

#define SD_DEMO_IPCFRAMES_MAX_PENDING_RECV_SEM_COUNT                      (10)
#define SD_DEMO_IPC_FRAMES_TRACE_ENABLE_FXN_ENTRY_EXIT                    (1)
#define SD_DEMO_IPC_FRAMES_TRACE_INFO_PRINT_INTERVAL                      (8192)


#if SD_DEMO_IPC_FRAMES_TRACE_ENABLE_FXN_ENTRY_EXIT
#define SD_DEMO_IPC_FRAMES_TRACE_FXN(str,...)         do {                           \
                                                     static Int printInterval = 0;\
                                                     if ((printInterval % SD_DEMO_IPC_FRAMES_TRACE_INFO_PRINT_INTERVAL) == 0) \
                                                     {                                                          \
                                                         OSA_printf("SD_DEMO_IPCFRAMES:%s function:%s",str,__func__);     \
                                                         OSA_printf(__VA_ARGS__);                               \
                                                     }                                                          \
                                                     printInterval++;                                           \
                                                   } while (0)
#define SD_DEMO_IPC_FRAMES_TRACE_FXN_ENTRY(...)                  SD_DEMO_IPC_FRAMES_TRACE_FXN("Entered",__VA_ARGS__)
#define SD_DEMO_IPC_FRAMES_TRACE_FXN_EXIT(...)                   SD_DEMO_IPC_FRAMES_TRACE_FXN("Leaving",__VA_ARGS__)
#else
#define SD_DEMO_IPC_FRAMES_TRACE_FXN_ENTRY(...)
#define SD_DEMO_IPC_FRAMES_TRACE_FXN_EXIT(...)
#endif


Void SD_Demo_ipcFrameFileWrite(VIDFrame_BufList *pFrameBufList)
{
    UInt32 writeDataSize;
    VIDFrame_Buf *pBuf;
    SD_Demo_ChInfo *pChInfo;
    UInt32 frameSize = 0, frameId;
    for(frameId=0; frameId<pFrameBufList->numFrames; frameId++)
    {
        pBuf = &pFrameBufList->frames[frameId];
        if(pBuf->channelNum<16)
        {

            pChInfo = &gSD_Demo_Ipc_Ctrl.chFrameInfo[pBuf->channelNum];

            frameSize = (pBuf->frameWidth * pBuf->frameHeight) << 1;
            
            pChInfo->totalDataSize += frameSize;

            pChInfo->numFrames++;

            if(pBuf->frameWidth > pChInfo->maxWidth)
                pChInfo->maxWidth = pBuf->frameWidth;

            if(pBuf->frameHeight > pChInfo->maxHeight)
                pChInfo->maxHeight = pBuf->frameHeight;

        }
        if(gSD_Demo_Ipc_Ctrl.fileFrameWriteEnable)
        {

            if(pBuf->channelNum== gSD_Demo_Ipc_Ctrl.fileFrameWriteChn && gSD_Demo_Ipc_Ctrl.fileFrameWriteState == FILE_WRITE_RUNNING)
            {
                UInt32 pMemVirtAddr;
                pMemVirtAddr = 0;

                /* MMAP Address mapping*/
                SD_Demo_mMap((UInt32)(pBuf->phyAddr[0][0]), frameSize, &pMemVirtAddr);
                
                writeDataSize = fwrite((Ptr) pMemVirtAddr, 1, frameSize, gSD_Demo_Ipc_Ctrl.fp);
                if(writeDataSize!=frameSize)
                {
                    gSD_Demo_Ipc_Ctrl.fileFrameWriteState = FILE_WRITE_STOPPED;
                    fclose(gSD_Demo_Ipc_Ctrl.fp);
                    printf(" Closing file [%s] for CH%d\n", gSD_Demo_Ipc_Ctrl.fileFrameWriteName, gSD_Demo_Ipc_Ctrl.fileFrameWriteChn);
                }
                /* MMAP Address unmapping*/
                SD_Demo_unmapMem();
            }
        }
    }
}

static void  SD_Demo_ipcFramesPrintFrameInfo(VIDFrame_Buf *buf)
{
    OSA_printf("SD_DEMO_IPCFRAMES:VIDFRAME_INFO:"
               "chNum:%d\t"
               "fid:%d\t"
               "frameWidth:%d\t"
               "frameHeight:%d\t"
               "timeStamp:%d\t"
               "virtAddr[0][0]:%p\t"
               "phyAddr[0][0]:%p",
                buf->channelNum,
                buf->fid,
                buf->frameWidth,
                buf->frameHeight,
                buf->timeStamp,
                buf->addr[0][0],
                buf->phyAddr[0][0]);

}


static void  SD_Demo_ipcFramesPrintFullFrameListInfo(VIDFrame_BufList *bufList,
                                                char *listName)
{
    static Int printStatsInterval = 0;
    if ((printStatsInterval % SD_DEMO_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
    {
        Int i;

        OSA_printf("SD_DEMO_IPCFRAMES:VIDFRAMELIST_INFO:%s\t"
                   "numFullFrames:%d",
                   listName,
                   bufList->numFrames);
        for (i = 0; i < bufList->numFrames; i++)
        {
             SD_Demo_ipcFramesPrintFrameInfo(&bufList->frames[i]);
        }
    }
    printStatsInterval++;
}


static Void * SD_Demo_ipcFramesSendRecvFxn(Void * prm)
{
     SD_Demo_IpcFramesCtrlThrObj *thrObj = ( SD_Demo_IpcFramesCtrlThrObj *) prm;
    static Int printStatsInterval = 0;
    VIDFrame_BufList bufList;
    UInt32 status = 0;


    SD_Demo_ipcFramesFileOpen();


    OSA_printf("SD_DEMO_IPCFRAMES:%s:Entered...",__func__);
    OSA_semWait(&thrObj->framesInNotifySem,OSA_TIMEOUT_FOREVER);
    OSA_printf("SD_DEMO_IPCFRAMES:Received first frame notify...");

    while (FALSE == thrObj->exitFramesInOutThread)
    {
        status =  IpcFramesInLink_getFullVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0,
                                                     &bufList);
        OSA_assert(0 == status);
        if (bufList.numFrames)
        {
             SD_Demo_ipcFramesPrintFullFrameListInfo(&bufList,"FullFrameList");

             SD_Demo_ipcFrameFileWrite(&bufList);

            status = IpcFramesOutLink_putFullVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0,
                                                        &bufList);

            OSA_assert(0 == status);
        }
        OSA_assert(0 == status);
        status =  IpcFramesOutLink_getEmptyVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0,
                                                       &bufList);
        OSA_assert(0 == status);

        if (bufList.numFrames)
        {
             SD_Demo_ipcFramesPrintFullFrameListInfo(&bufList,"EmptyFrameList");
            status = IpcFramesInLink_putEmptyVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0,
                                                         &bufList);

            OSA_assert(0 == status);
        }
#ifdef IPC_BITS_DEBUG
        if ((printStatsInterval % SD_DEMO_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("SD_DEMO_IPCFRAMES:%s:INFO: periodic print..",__func__);
        }
#endif
        printStatsInterval++;
        OSA_waitMsecs(SD_DEMO_IPCFRAMES_SENDRECVFXN_PERIOD_MS);
    }

    if(gSD_Demo_Ipc_Ctrl.fileFrameWriteEnable)
    {
        if(gSD_Demo_Ipc_Ctrl.fileFrameWriteState==FILE_WRITE_RUNNING)
        {
            fclose(gSD_Demo_Ipc_Ctrl.fp);
            printf(" Closing file [%s] for CH%d\n", gSD_Demo_Ipc_Ctrl.fileFrameWriteName, gSD_Demo_Ipc_Ctrl.fileFrameWriteChn);
        }
    }

    OSA_printf("SD_DEMO_IPCFRAMES:%s:Leaving...",__func__);
    return NULL;
}


static Void  SD_Demo_ipcFramesInitThrObj( SD_Demo_IpcFramesCtrlThrObj *thrObj)
{

    OSA_semCreate(&thrObj->framesInNotifySem,
                  SD_DEMO_IPCFRAMES_MAX_PENDING_RECV_SEM_COUNT,0);
    thrObj->exitFramesInOutThread = FALSE;
    OSA_thrCreate(&thrObj->thrHandleFramesInOut,
                   SD_Demo_ipcFramesSendRecvFxn,
                  SD_DEMO_IPCFRAMES_SENDRECVFXN_TSK_PRI,
                  SD_DEMO_IPCFRAMES_SENDRECVFXN_TSK_STACK_SIZE,
                  thrObj);

}

static Void  SD_Demo_ipcFramesDeInitThrObj( SD_Demo_IpcFramesCtrlThrObj *thrObj)
{
    thrObj->exitFramesInOutThread = TRUE;
    OSA_thrDelete(&thrObj->thrHandleFramesInOut);
    OSA_semDelete(&thrObj->framesInNotifySem);
}


static
Void  SD_Demo_ipcFramesInCbFxn (Ptr cbCtx)
{
     SD_Demo_IpcFramesCtrl *ipcFramesCtrl;
    static Int printInterval;

    OSA_assert(cbCtx = &gSD_Demo_Ipc_Ctrl.ipcFrames);
    ipcFramesCtrl = cbCtx;
    OSA_semSignal(&ipcFramesCtrl->thrObj.framesInNotifySem);
    if ((printInterval % SD_DEMO_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("SD_DEMO_IPCFRAMES: Callback function:%s",__func__);
    }
    printInterval++;
}

Void SD_Demo_ipcFramesInSetCbInfo (IpcFramesInLinkHLOS_CreateParams *createParams)
{
    createParams->cbCtx = &gSD_Demo_Ipc_Ctrl;
    createParams->cbFxn = SD_Demo_ipcFramesInCbFxn;
}

Int32  SD_Demo_ipcFramesCreate()
{

     SD_Demo_ipcFramesFileWriteCreate();

     SD_Demo_ipcFramesInitThrObj(&gSD_Demo_Ipc_Ctrl.ipcFrames.thrObj);
    return OSA_SOK;
}

Void  SD_Demo_ipcFramesStop(void)
{
    gSD_Demo_Ipc_Ctrl.ipcFrames.thrObj.exitFramesInOutThread = TRUE;
}

Int32  SD_Demo_ipcFramesDelete()
{
    OSA_printf("Entered:%s...",__func__);
    SD_Demo_ipcFramesDeInitThrObj(&gSD_Demo_Ipc_Ctrl.ipcFrames.thrObj);
    OSA_printf("Leaving:%s...",__func__);
    return OSA_SOK;
}

Void SD_Demo_ipcFramesFileWriteCreate()
{
    printf("\n Enable RAW Frame Write\n");
    gSD_Demo_Ipc_Ctrl.fileFrameWriteChn = 0;

    gSD_Demo_Ipc_Ctrl.fileFrameWriteEnable = SD_Demo_getFileWriteEnable();

    if(gSD_Demo_Ipc_Ctrl.fileFrameWriteEnable)
    {
        char path[256];

        SD_Demo_getFileWritePath(path, "/dev/shm");

        gSD_Demo_Ipc_Ctrl.fileFrameWriteChn = SD_Demo_getChId("FRAME File Write", gSD_Demo_ctrl.numCapChannels );

        sprintf(gSD_Demo_Ipc_Ctrl.fileFrameWriteName, "%s/VID_CH%02d.yuv", path, gSD_Demo_Ipc_Ctrl.fileFrameWriteChn);
    }
}

Void SD_Demo_ipcFramesFileOpen()
{
    gSD_Demo_Ipc_Ctrl.fp = NULL;

    gSD_Demo_Ipc_Ctrl.fileFrameWriteState = FILE_WRITE_STOPPED;
    if(gSD_Demo_Ipc_Ctrl.fileFrameWriteEnable)
    {
        gSD_Demo_Ipc_Ctrl.fp = fopen(gSD_Demo_Ipc_Ctrl.fileFrameWriteName, "wb");
        if(gSD_Demo_Ipc_Ctrl.fp!=NULL)
        {
            gSD_Demo_Ipc_Ctrl.fileFrameWriteState = FILE_WRITE_RUNNING;
            printf(" Opened file [%s] for writing CH%d\n", gSD_Demo_Ipc_Ctrl.fileFrameWriteName, gSD_Demo_Ipc_Ctrl.fileFrameWriteChn);
        }
        else
        {
            printf(" ERROR: File open [%s] for writing CH%d FAILED !!!!\n", gSD_Demo_Ipc_Ctrl.fileFrameWriteName, gSD_Demo_Ipc_Ctrl.fileFrameWriteChn);
        }
    }
}
Int32 SD_Demo_mMap(UInt32 physAddr, Uint32 memSize , UInt32 *pMemVirtAddr)
{
    gSD_Demo_MMapCtrl.memDevFd = open("/dev/mem",O_RDWR|O_SYNC);

    if(gSD_Demo_MMapCtrl.memDevFd < 0)
    {
      printf(" ERROR: /dev/mem open failed !!!\n");
      return -1;
    }


    gSD_Demo_MMapCtrl.memOffset   = physAddr & MMAP_MEM_PAGEALIGN;

    gSD_Demo_MMapCtrl.mmapMemAddr = physAddr - gSD_Demo_MMapCtrl.memOffset;

    gSD_Demo_MMapCtrl.mmapMemSize = memSize + gSD_Demo_MMapCtrl.memOffset;

    gSD_Demo_MMapCtrl.pMemVirtAddr = mmap(	
           (void	*)gSD_Demo_MMapCtrl.mmapMemAddr,
           gSD_Demo_MMapCtrl.mmapMemSize,
           PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
           gSD_Demo_MMapCtrl.memDevFd,
           gSD_Demo_MMapCtrl.mmapMemAddr
           );

   if (gSD_Demo_MMapCtrl.pMemVirtAddr==NULL)
   {
     printf(" ERROR: mmap() failed !!!\n");
     return -1;
   }
    *pMemVirtAddr = (UInt32)((UInt32)gSD_Demo_MMapCtrl.pMemVirtAddr + gSD_Demo_MMapCtrl.memOffset);

    return 0;
}

Int32 SD_Demo_unmapMem()
{
    if(gSD_Demo_MMapCtrl.pMemVirtAddr)
      munmap((void*)gSD_Demo_MMapCtrl.pMemVirtAddr, gSD_Demo_MMapCtrl.mmapMemSize);
      
    if(gSD_Demo_MMapCtrl.memDevFd >= 0)
      close(gSD_Demo_MMapCtrl.memDevFd);
      
    return 0;
}

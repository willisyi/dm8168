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
#include "demo_vcap_venc_vdis.h"
#include "mcfw/interfaces/ti_vcap.h"
#include "mcfw/interfaces/ti_vdis.h"

#ifdef CUSTOM_SD_DEMO
#include <osa_dma.h>

VcapVenc_mMapCtrl gMMapCtrl;
#define FILE_WRITE_STOPPED  (0)
#define FILE_WRITE_RUNNING  (1)
#endif

#define MCFW_IPC_FRAMES_NONOTIFYMODE_FRAMESIN                          (TRUE)
#define MCFW_IPC_FRAMES_NONOTIFYMODE_FRAMESOUT                         (TRUE)

#define MCFW_IPCFRAMES_SENDRECVFXN_TSK_PRI                             (2)
#define MCFW_IPCFRAMES_SENDRECVFXN_TSK_STACK_SIZE                      (0) /* 0 means system default will be used */
#define MCFW_IPCFRAMES_SENDRECVFXN_PERIOD_MS                           (16)


#define MCFW_IPCFRAMES_INFO_PRINT_INTERVAL                             (8192)

#define MCFW_IPCFRAMES_MAX_PENDING_RECV_SEM_COUNT                      (10)
#define MCFW_IPC_FRAMES_TRACE_ENABLE_FXN_ENTRY_EXIT                    (1)
#define MCFW_IPC_FRAMES_TRACE_INFO_PRINT_INTERVAL                      (8192)


#if MCFW_IPC_FRAMES_TRACE_ENABLE_FXN_ENTRY_EXIT
#define MCFW_IPC_FRAMES_TRACE_FXN(str,...)         do {                           \
                                                     static Int printInterval = 0;\
                                                     if ((printInterval % MCFW_IPC_FRAMES_TRACE_INFO_PRINT_INTERVAL) == 0) \
                                                     {                                                          \
                                                         OSA_printf("MCFW_IPCFRAMES:%s function:%s",str,__func__);     \
                                                         OSA_printf(__VA_ARGS__);                               \
                                                     }                                                          \
                                                     printInterval++;                                           \
                                                   } while (0)
#define MCFW_IPC_FRAMES_TRACE_FXN_ENTRY(...)                  MCFW_IPC_FRAMES_TRACE_FXN("Entered",__VA_ARGS__)
#define MCFW_IPC_FRAMES_TRACE_FXN_EXIT(...)                   MCFW_IPC_FRAMES_TRACE_FXN("Leaving",__VA_ARGS__)
#else
#define MCFW_IPC_FRAMES_TRACE_FXN_ENTRY(...)
#define MCFW_IPC_FRAMES_TRACE_FXN_EXIT(...)
#endif



static void  VcapVenc_ipcFramesPrintFrameInfo(VIDEO_FRAMEBUF_S *buf)
{
    OSA_printf("MCFW_IPCFRAMES:VIDFRAME_INFO:"
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


static void  VcapVenc_ipcFramesPrintFullFrameListInfo(VIDEO_FRAMEBUF_LIST_S *bufList,
                                                char *listName)
{
    static Int printStatsInterval = 0;
    if ((printStatsInterval % MCFW_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
    {
        Int i;

        OSA_printf("MCFW_IPCFRAMES:VIDFRAMELIST_INFO:%s\t"
                   "numFullFrames:%d",
                   listName,
                   bufList->numFrames);
        for (i = 0; i < bufList->numFrames; i++)
        {
             VcapVenc_ipcFramesPrintFrameInfo(&bufList->frames[i]);
        }
    }
    printStatsInterval++;
}


static Void * VcapVenc_ipcFramesSendRecvFxn(Void * prm)
{
     VcapVenc_IpcFramesCtrlThrObj *thrObj = ( VcapVenc_IpcFramesCtrlThrObj *) prm;
    static Int printStatsInterval = 0;
    VIDEO_FRAMEBUF_LIST_S bufList;
    Int status;

#ifdef CUSTOM_SD_DEMO
    VcapVenc_ipcFramesFileOpen();
#endif

    OSA_printf("MCFW_IPCFRAMES:%s:Entered...",__func__);
    OSA_semWait(&thrObj->framesInNotifySem,OSA_TIMEOUT_FOREVER);
    OSA_printf("MCFW_IPCFRAMES:Received first frame notify...");
    while (FALSE == thrObj->exitFramesInOutThread)
    {
        status =  Vcap_getFullVideoFrames(&bufList,0);
        OSA_assert(0 == status);
        if (bufList.numFrames)
        {
             VcapVenc_ipcFramesPrintFullFrameListInfo(&bufList,"FullFrameList");
#ifdef CUSTOM_SD_DEMO
             VcapVenc_ipcFrameFileWrite(&bufList);
#endif
            status = Vdis_putFullVideoFrames(&bufList);
            OSA_assert(0 == status);
        }
        status =  Vdis_getEmptyVideoFrames(&bufList,0);
        OSA_assert(0 == status);

        if (bufList.numFrames)
        {
             VcapVenc_ipcFramesPrintFullFrameListInfo(&bufList,"EmptyFrameList");
            status = Vcap_putEmptyVideoFrames(&bufList);
            OSA_assert(0 == status);
        }
#ifdef IPC_BITS_DEBUG
        if ((printStatsInterval % MCFW_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("MCFW_IPCFRAMES:%s:INFO: periodic print..",__func__);
        }
#endif
        printStatsInterval++;
        OSA_waitMsecs(MCFW_IPCFRAMES_SENDRECVFXN_PERIOD_MS);
    }
#ifdef CUSTOM_SD_DEMO
    if(gVcapVenc_ctrl.fileFrameWriteEnable)
    {
        if(gVcapVenc_ctrl.fileFrameWriteState==FILE_WRITE_RUNNING)
        {
            fclose(gVcapVenc_ctrl.fp);
            printf(" Closing file [%s] for CH%d\n", gVcapVenc_ctrl.fileFrameWriteName, gVcapVenc_ctrl.fileFrameWriteChn);
        }
    }
#endif
    OSA_printf("MCFW_IPCFRAMES:%s:Leaving...",__func__);
    return NULL;
}


static Void  VcapVenc_ipcFramesInitThrObj( VcapVenc_IpcFramesCtrlThrObj *thrObj)
{

    OSA_semCreate(&thrObj->framesInNotifySem,
                  MCFW_IPCFRAMES_MAX_PENDING_RECV_SEM_COUNT,0);
    thrObj->exitFramesInOutThread = FALSE;
    OSA_thrCreate(&thrObj->thrHandleFramesInOut,
                   VcapVenc_ipcFramesSendRecvFxn,
                  MCFW_IPCFRAMES_SENDRECVFXN_TSK_PRI,
                  MCFW_IPCFRAMES_SENDRECVFXN_TSK_STACK_SIZE,
                  thrObj);

}

static Void  VcapVenc_ipcFramesDeInitThrObj( VcapVenc_IpcFramesCtrlThrObj *thrObj)
{
    thrObj->exitFramesInOutThread = TRUE;
    OSA_thrDelete(&thrObj->thrHandleFramesInOut);
    OSA_semDelete(&thrObj->framesInNotifySem);
}


static
Void  VcapVenc_ipcFramesInCbFxn (Ptr cbCtx)
{
     VcapVenc_IpcFramesCtrl *ipcFramesCtrl;
    static Int printInterval;

    OSA_assert(cbCtx = &gVcapVenc_ctrl.ipcFrames);
    ipcFramesCtrl = cbCtx;
    OSA_semSignal(&ipcFramesCtrl->thrObj.framesInNotifySem);
    if ((printInterval % MCFW_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("MCFW_IPCFRAMES: Callback function:%s",__func__);
    }
    printInterval++;
}


Void  VcapVenc_ipcFramesInSetCbInfo ()
{
    VCAP_CALLBACK_S vcapCallback;

    vcapCallback.newDataAvailableCb =  VcapVenc_ipcFramesInCbFxn;

    Vcap_registerCallback(&vcapCallback, &gVcapVenc_ctrl.ipcFrames);
}

Int32  VcapVenc_ipcFramesCreate(int demoId)
{

#ifdef CUSTOM_SD_DEMO
    if(demoId == DEMO_VCAP_VENC_VDIS)
    {
         VcapVenc_ipcFramesFileWriteCreate();
    }
    else
    {
        gVcapVenc_ctrl.fileFrameWriteChn    = 0;
        gVcapVenc_ctrl.fileFrameWriteEnable = FALSE;
    
    }
#endif
     VcapVenc_ipcFramesInitThrObj(&gVcapVenc_ctrl.ipcFrames.thrObj);
    return OSA_SOK;
}

Void  VcapVenc_ipcFramesStop(void)
{
    gVcapVenc_ctrl.ipcFrames.thrObj.exitFramesInOutThread = TRUE;
}

Int32  VcapVenc_ipcFramesDelete()
{
    OSA_printf("Entered:%s...",__func__);
    VcapVenc_ipcFramesDeInitThrObj(&gVcapVenc_ctrl.ipcFrames.thrObj);
    OSA_printf("Leaving:%s...",__func__);
    return OSA_SOK;
}

#ifdef CUSTOM_SD_DEMO

Void VcapVenc_ipcFramesFileWriteCreate()
{
    printf("\nEnable RAW Frame Write\n\n");
    gVcapVenc_ctrl.fileFrameWriteChn = 0;

    gVcapVenc_ctrl.fileFrameWriteEnable = Demo_getFileWriteEnable();

    if(gVcapVenc_ctrl.fileFrameWriteEnable)
    {
        char path[256];

        Demo_getFileWritePath(path, "/dev/shm");

        gVcapVenc_ctrl.fileFrameWriteChn = Demo_getChId("FRAME File Write", gDemo_info.maxVcapChannels);

        sprintf(gVcapVenc_ctrl.fileFrameWriteName, "%s/VID_CH%02d.yuv", path, gVcapVenc_ctrl.fileFrameWriteChn);
    }
}

Void VcapVenc_ipcFramesFileOpen()
{
    gVcapVenc_ctrl.fp = NULL;

    gVcapVenc_ctrl.fileFrameWriteState = FILE_WRITE_STOPPED;
    if(gVcapVenc_ctrl.fileFrameWriteEnable)
    {
        gVcapVenc_ctrl.fp = fopen(gVcapVenc_ctrl.fileFrameWriteName, "wb");
        if(gVcapVenc_ctrl.fp!=NULL)
        {
            gVcapVenc_ctrl.fileFrameWriteState = FILE_WRITE_RUNNING;
            printf(" Opened file [%s] for writing CH%d\n", gVcapVenc_ctrl.fileFrameWriteName, gVcapVenc_ctrl.fileFrameWriteChn);
        }
        else
        {
            printf(" ERROR: File open [%s] for writing CH%d FAILED !!!!\n", gVcapVenc_ctrl.fileFrameWriteName, gVcapVenc_ctrl.fileFrameWriteChn);
        }
    }
}
Void VcapVenc_ipcFrameFileWrite(VIDEO_FRAMEBUF_LIST_S *pFrameBufList)
{
    UInt32 writeDataSize;
    VIDEO_FRAMEBUF_S *pBuf;
    VcapVenc_ChInfo *pChInfo;
    UInt32 frameSize = 0, frameId;
    
    for(frameId=0; frameId<pFrameBufList->numFrames; frameId++)
    {
        pBuf = &pFrameBufList->frames[frameId];
        if(pBuf->channelNum<VCAP_CHN_MAX)
        {

            pChInfo = &gVcapVenc_ctrl.chFrameInfo[pBuf->channelNum];

            frameSize = (pBuf->frameWidth * pBuf->frameHeight) << 1;
            
            pChInfo->totalDataSize += frameSize;

            pChInfo->numFrames++;

            if(pBuf->frameWidth > pChInfo->maxWidth)
                pChInfo->maxWidth = pBuf->frameWidth;

            if(pBuf->frameHeight > pChInfo->maxHeight)
                pChInfo->maxHeight = pBuf->frameHeight;

        }
        if(gVcapVenc_ctrl.fileFrameWriteEnable)
        {
            if(pBuf->channelNum== gVcapVenc_ctrl.fileFrameWriteChn && gVcapVenc_ctrl.fileFrameWriteState == FILE_WRITE_RUNNING)
            {
                UInt32 pMemVirtAddr;
                pMemVirtAddr = 0;
                VcapVenc_mMap((UInt32)(pBuf->phyAddr[0][0]), frameSize, &pMemVirtAddr);
                
                writeDataSize = fwrite((Ptr) pMemVirtAddr, 1, frameSize, gVcapVenc_ctrl.fp);
                if(writeDataSize!=frameSize)
                {
                    gVcapVenc_ctrl.fileFrameWriteState = FILE_WRITE_STOPPED;
                    fclose(gVcapVenc_ctrl.fp);
                    printf(" Closing file [%s] for CH%d\n", gVcapVenc_ctrl.fileFrameWriteName, gVcapVenc_ctrl.fileFrameWriteChn);
                }
                VcapVenc_unmapMem();
            }
        }
    }
}
Int32 VcapVenc_mMap(UInt32 physAddr, Uint32 memSize , UInt32 *pMemVirtAddr)
{
    gMMapCtrl.memDevFd = open("/dev/mem",O_RDWR|O_SYNC);

    if(gMMapCtrl.memDevFd < 0)
    {
      printf(" ERROR: /dev/mem open failed !!!\n");
      return -1;
    }


    gMMapCtrl.memOffset   = physAddr & MMAP_MEM_PAGEALIGN;

    gMMapCtrl.mmapMemAddr = physAddr - gMMapCtrl.memOffset;

    gMMapCtrl.mmapMemSize = memSize + gMMapCtrl.memOffset;

    gMMapCtrl.pMemVirtAddr = mmap(	
           (void	*)gMMapCtrl.mmapMemAddr,
           gMMapCtrl.mmapMemSize,
           PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
           gMMapCtrl.memDevFd,
           gMMapCtrl.mmapMemAddr
           );

   if (gMMapCtrl.pMemVirtAddr==NULL)
   {
     printf(" ERROR: mmap() failed !!!\n");
     return -1;
   }
    *pMemVirtAddr = (UInt32)((UInt32)gMMapCtrl.pMemVirtAddr + gMMapCtrl.memOffset);

    return 0;
}

Int32 VcapVenc_unmapMem()
{
    if(gMMapCtrl.pMemVirtAddr)
      munmap((void*)gMMapCtrl.pMemVirtAddr, gMMapCtrl.mmapMemSize);
      
    if(gMMapCtrl.memDevFd >= 0)
      close(gMMapCtrl.memDevFd);
      
    return 0;
}

#endif

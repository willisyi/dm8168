/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file chains_ipcFrames.c
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
#include <demos/link_api_demos/common/chains_ipcFrames.h>

/* Set to TRUE to prevent doing fgets */
#define CHAINS_IPC_FRAMES_DISABLE_USER_INPUT                             (FALSE)


#define CHAINS_IPC_FRAMES_MAX_NUM_CHANNELS                               (16)
#define CHAINS_IPC_FRAMES_NONOTIFYMODE_FRAMESIN                          (TRUE)
#define CHAINS_IPC_FRAMES_NONOTIFYMODE_FRAMESOUT                         (TRUE)
#define CHAINS_IPC_FRAMES_ENABLE_FILE_WRITE                              (TRUE)

#define CHAINS_IPCFRAMES_SENDRECVFXN_TSK_PRI                   (2)

#define CHAINS_IPCFRAMES_SENDRECVFXN_TSK_STACK_SIZE            (0) /* 0 means system default will be used */


#define CHAINS_IPCFRAMES_SENDRECVFXN_PERIOD_MS                 (16)


#define CHAINS_IPCFRAMES_INFO_PRINT_INTERVAL                    (8192)
#define CHAINS_IPCFRAMES_WRITE_FILE_INTERVAL                    (600)

#define CHAINS_IPCFRAMES_MAX_PENDING_RECV_SEM_COUNT             (10)
#define CHAINS_IPC_FRAMES_TRACE_ENABLE_FXN_ENTRY_EXIT           (1)
#define CHAINS_IPC_FRAMES_TRACE_INFO_PRINT_INTERVAL             (8192)


#if CHAINS_IPC_FRAMES_TRACE_ENABLE_FXN_ENTRY_EXIT
#define CHAINS_IPC_FRAMES_TRACE_FXN(str,...)         do {                           \
                                                     static Int printInterval = 0;\
                                                     if ((printInterval % CHAINS_IPC_FRAMES_TRACE_INFO_PRINT_INTERVAL) == 0) \
                                                     {                                                          \
                                                         OSA_printf("CHAINS_IPCFRAMES:%s function:%s",str,__func__);     \
                                                         OSA_printf(__VA_ARGS__);                               \
                                                     }                                                          \
                                                     printInterval++;                                           \
                                                   } while (0)
#define CHAINS_IPC_FRAMES_TRACE_FXN_ENTRY(...)                  CHAINS_IPC_FRAMES_TRACE_FXN("Entered",__VA_ARGS__)
#define CHAINS_IPC_FRAMES_TRACE_FXN_EXIT(...)                   CHAINS_IPC_FRAMES_TRACE_FXN("Leaving",__VA_ARGS__)
#else
#define CHAINS_IPC_FRAMES_TRACE_FXN_ENTRY(...)
#define CHAINS_IPC_FRAMES_TRACE_FXN_EXIT(...)
#endif


typedef struct Chains_IpcFramesCtrlThrObj {
    OSA_ThrHndl thrHandleFramesInOut;
    OSA_ThrHndl thrHandleWriteFile;
    OSA_SemHndl framesInNotifySem;
    OSA_SemHndl writeFileNotifySem;
    volatile Bool exitFramesInOutThread;
    volatile Bool exitWriteFileThread;
    UInt8 buffer[1920*1200*2];
    UInt32 bufSize;
} Chains_IpcFramesCtrlThrObj;

typedef struct Chains_IpcFramesCtrl {
    Bool  enableFWrite;
    UInt32 writeFileInterval;
    Bool  noNotifyFramesInHLOS;
    Bool  noNotifyFramesOutHLOS;
    Chains_IpcFramesCtrlThrObj  thrObj;

} Chains_IpcFramesCtrl;

Chains_IpcFramesCtrl gChains_ipcFramesCtrl =
{
    .enableFWrite          = CHAINS_IPC_FRAMES_ENABLE_FILE_WRITE,
    .writeFileInterval     = CHAINS_IPCFRAMES_WRITE_FILE_INTERVAL,
    .noNotifyFramesInHLOS  = CHAINS_IPC_FRAMES_NONOTIFYMODE_FRAMESIN,
    .noNotifyFramesOutHLOS = CHAINS_IPC_FRAMES_NONOTIFYMODE_FRAMESOUT,
};

static Void Chains_ipcFramesPrintFrameInfo(VIDFrame_Buf *buf)
{
    OSA_printf("CHAINS_IPCFRAMES:VIDFRAME_INFO:"
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

static Void Chains_ipcFramesPrintFullFrameListInfo(VIDFrame_BufList *bufList,
                                                   char *listName)
{
    static Int printStatsInterval = 0;
    if ((printStatsInterval % CHAINS_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
    {
        Int i;

        OSA_printf("CHAINS_IPCFRAMES:VIDFRAMELIST_INFO:%s\t"
                   "numFullFrames:%d",
                   listName,
                   bufList->numFrames);
        for (i = 0; i < bufList->numFrames; i++)
        {
            Chains_ipcFramesPrintFrameInfo(&bufList->frames[i]);
        }
    }
    printStatsInterval++;
}


static Void *Chains_ipcFramesSendRecvFxn(Void * prm)
{
    Chains_IpcFramesCtrlThrObj *thrObj = (Chains_IpcFramesCtrlThrObj *) prm;
    static Int printStatsInterval = 0;
    VIDFrame_BufList bufList;
    Int status;

    Int32 i = 0;
    VIDFrame_Buf *pFrame;
    static Int32 frameNum = 0;

    OSA_printf("CHAINS_IPCFRAMES:%s:Entered...",__func__);
    OSA_semWait(&thrObj->framesInNotifySem,OSA_TIMEOUT_FOREVER);
    OSA_printf("CHAINS_IPCFRAMES:Received first frame notify...");
    while (FALSE == thrObj->exitFramesInOutThread)
    {
        status =  IpcFramesInLink_getFullVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0,
                                                     &bufList);
        OSA_assert(0 == status);

        for (i=0; i<bufList.numFrames; i++) {
            frameNum += 1;
            if (frameNum % gChains_ipcFramesCtrl.writeFileInterval == 0) {
                pFrame = &bufList.frames[0];
                thrObj->bufSize = pFrame->frameWidth * pFrame->frameHeight * 2;
                memcpy(thrObj->buffer, pFrame->addr[0][0], thrObj->bufSize);
                OSA_semSignal(&gChains_ipcFramesCtrl.thrObj.writeFileNotifySem);
            }
        }

        if (bufList.numFrames)
        {
            Chains_ipcFramesPrintFullFrameListInfo(&bufList,"FullFrameList");
            status = IpcFramesOutLink_putFullVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0,
                                                        &bufList);
            OSA_assert(0 == status);
        }
        status =  IpcFramesOutLink_getEmptyVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0,
                                                       &bufList);
        OSA_assert(0 == status);

        if (bufList.numFrames)
        {
            Chains_ipcFramesPrintFullFrameListInfo(&bufList,"EmptyFrameList");
            status = IpcFramesInLink_putEmptyVideoFrames(SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0,
                                                         &bufList);
            OSA_assert(0 == status);
        }
        if ((printStatsInterval % CHAINS_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCFRAMES:%s:INFO: periodic print..",__func__);
        }
        printStatsInterval++;

        OSA_waitMsecs(CHAINS_IPCFRAMES_SENDRECVFXN_PERIOD_MS);
        //OSA_semWait(&thrObj->framesInNotifySem,OSA_TIMEOUT_FOREVER);
    }
    OSA_printf("CHAINS_IPCFRAMES:%s:Leaving...",__func__);
    return NULL;
}

static void *Chains_ipcFramesWriteFileFxn(Void * prm)
{
    Chains_IpcFramesCtrlThrObj *thrObj = (Chains_IpcFramesCtrlThrObj *) prm;
    Int32 num = 0;
    FILE *fp = NULL;
    char filename[256];

    while (FALSE == thrObj->exitWriteFileThread) {
        OSA_semWait(&thrObj->writeFileNotifySem,OSA_TIMEOUT_FOREVER);
        num ++;
        sprintf(filename, "%d.raw", num);
        printf("\n%s", filename);
        fp = fopen(filename, "wb");
        fwrite(thrObj->buffer, 1, thrObj->bufSize, fp);
        fclose(fp);
    }
    return NULL;
}

static Void Chains_ipcFramesInitThrObj(Chains_IpcFramesCtrlThrObj *thrObj)
{
    OSA_semCreate(&thrObj->framesInNotifySem,
                  CHAINS_IPCFRAMES_MAX_PENDING_RECV_SEM_COUNT,0);
    thrObj->exitFramesInOutThread = FALSE;
    OSA_thrCreate(&thrObj->thrHandleFramesInOut,
                  Chains_ipcFramesSendRecvFxn,
                  CHAINS_IPCFRAMES_SENDRECVFXN_TSK_PRI,
                  CHAINS_IPCFRAMES_SENDRECVFXN_TSK_STACK_SIZE,
                  thrObj);

    if (gChains_ipcFramesCtrl.enableFWrite) {
        OSA_semCreate(&thrObj->writeFileNotifySem,
                      CHAINS_IPCFRAMES_MAX_PENDING_RECV_SEM_COUNT,0);
        thrObj->exitWriteFileThread = FALSE;
        OSA_thrCreate(&thrObj->thrHandleWriteFile,
                  Chains_ipcFramesWriteFileFxn,
                  CHAINS_IPCFRAMES_SENDRECVFXN_TSK_PRI,
                  CHAINS_IPCFRAMES_SENDRECVFXN_TSK_STACK_SIZE,
                  thrObj);
    }
}

static Void Chains_ipcFramesDeInitThrObj(Chains_IpcFramesCtrlThrObj *thrObj)
{
    if (gChains_ipcFramesCtrl.enableFWrite) {
        thrObj->exitWriteFileThread = TRUE;
        OSA_semSignal(&gChains_ipcFramesCtrl.thrObj.writeFileNotifySem);
        OSA_thrDelete(&thrObj->thrHandleWriteFile);
        OSA_semDelete(&thrObj->writeFileNotifySem);
    }
    thrObj->exitFramesInOutThread = TRUE;
    OSA_thrDelete(&thrObj->thrHandleFramesInOut);
    OSA_semDelete(&thrObj->framesInNotifySem);
}

static
Void Chains_ipcFramesInCbFxn (Ptr cbCtx)
{
    Chains_IpcFramesCtrl *chains_ipcFramesCtrl;
    static Int printInterval;

    OSA_assert(cbCtx = &gChains_ipcFramesCtrl);
    chains_ipcFramesCtrl = cbCtx;
    OSA_semSignal(&chains_ipcFramesCtrl->thrObj.framesInNotifySem);
    if ((printInterval % CHAINS_IPCFRAMES_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("CHAINS_IPCFRAMES: Callback function:%s",__func__);
    }
    printInterval++;
}


Void Chains_ipcFramesInSetCbInfo (IpcFramesInLinkHLOS_CreateParams *createParams)
{
    createParams->cbCtx = &gChains_ipcFramesCtrl;
    createParams->cbFxn = Chains_ipcFramesInCbFxn;
}

Int32 Chains_ipcFramesInit()
{
    Chains_ipcFramesInitThrObj(&gChains_ipcFramesCtrl.thrObj);
    return OSA_SOK;
}

Void Chains_ipcFramesStop(void)
{
    gChains_ipcFramesCtrl.thrObj.exitWriteFileThread = TRUE;
    gChains_ipcFramesCtrl.thrObj.exitFramesInOutThread = TRUE;
}

Int32 Chains_ipcFramesExit()
{
    OSA_printf("Entered:%s...",__func__);
    Chains_ipcFramesDeInitThrObj(&gChains_ipcFramesCtrl.thrObj);
    OSA_printf("Leaving:%s...",__func__);
    return OSA_SOK;
}



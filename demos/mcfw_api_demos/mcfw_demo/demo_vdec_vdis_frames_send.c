/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file demo_vdec_vdis_frames_send.c
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
#include "demo_vdec_vdis.h"
#include "mcfw/interfaces/ti_vcap.h"
#include "mcfw/interfaces/ti_vdis.h"


#define MCFW_IPCFRAMES_INFO_PRINT_INTERVAL                             (8192)

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



static void  VdecVdis_ipcFramesPrintFrameInfo(VIDEO_FRAMEBUF_S *buf)
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


static void  VdecVdis_ipcFramesPrintFullFrameListInfo(VIDEO_FRAMEBUF_LIST_S *bufList,
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
             VdecVdis_ipcFramesPrintFrameInfo(&bufList->frames[i]);
        }
    }
    printStatsInterval++;
}

static Void VdecVdis_ipcFramesFillBufInfo(VIDEO_FRAMEBUF_LIST_S *bufList,
                                          VdecVdis_IpcFramesBufObj      frameObj[],
                                          UInt32  numFrames)
{
    Int i,j;

    bufList->numFrames = 0;
    for (i = 0; i < numFrames;i++)
    {
        if (frameObj[i].refCnt == 0)
        {
            /* Send the same buffer to all enabled channels */
            for (j = 0; j < frameObj[i].numCh; j++)
            {
                if (bufList->numFrames >= OSA_ARRAYSIZE(bufList->frames))
                {
                    OSA_assert(bufList->numFrames ==
                               OSA_ARRAYSIZE(bufList->frames));
                    break;
                }
                bufList->frames[bufList->numFrames].addr[0][0] =
                  frameObj[i].bufVirt;
                bufList->frames[bufList->numFrames].phyAddr[0][0] =
                  (Ptr)frameObj[i].bufPhy;
                bufList->frames[bufList->numFrames].frameWidth =
                  frameObj[i].curWidth;
                bufList->frames[bufList->numFrames].frameHeight =
                  frameObj[i].curHeight;
                bufList->frames[bufList->numFrames].fid =
                  VIDEO_FID_TYPE_FRAME;
                bufList->frames[bufList->numFrames].channelNum =
                  j;
                bufList->frames[bufList->numFrames].timeStamp  = 0;
                bufList->numFrames++;
            }
            frameObj[i].refCnt += bufList->numFrames;
        }
    }
}

static Void   VdecVdis_ipcFramesFreeFrameBuf(VIDEO_FRAMEBUF_LIST_S *bufList,
                                             VdecVdis_IpcFramesBufObj      frameObj[],
                                             UInt32  numFrames)
{
    Int i,j;

    for (i = 0; i < bufList->numFrames; i++)
    {
        for (j = 0; j < numFrames; j++)
        {
            if (frameObj[j].bufPhy ==
                (UInt32)bufList->frames[i].phyAddr[0][0])
            {
                OSA_assert(frameObj[j].refCnt > 0);
                frameObj[j].refCnt--;
            }
        }
    }
}


static Void * VdecVdis_ipcFramesSendFxn(Void * prm)
{
    VdecVdis_IpcFramesCtrl *ipcFramesObj = (VdecVdis_IpcFramesCtrl *) prm;
    VdecVdis_IpcFramesCtrlThrObj *thrObj = &ipcFramesObj->thrObj;
    static Int printStatsInterval = 0;
    VIDEO_FRAMEBUF_LIST_S bufList;
    Int status;

    OSA_printf("MCFW_IPCFRAMES:%s:Entered...",__func__);
    OSA_semWait(&thrObj->thrStartSem,OSA_TIMEOUT_FOREVER);
    OSA_printf("MCFW_IPCFRAMES:Received start signal...");
    while (FALSE == thrObj->exitFramesOutThread)
    {
        VdecVdis_ipcFramesFillBufInfo(&bufList,
                                      ipcFramesObj->frameObj,
                                      OSA_ARRAYSIZE(ipcFramesObj->frameObj));
        if (bufList.numFrames)
        {
            status = Vdis_putFullVideoFrames(&bufList);
            OSA_assert(0 == status);
        }
        status =  Vdis_getEmptyVideoFrames(&bufList,0);
        OSA_assert(0 == status);

        if (bufList.numFrames)
        {
            VdecVdis_ipcFramesPrintFullFrameListInfo(&bufList,"EmptyFrameList");
            VdecVdis_ipcFramesFreeFrameBuf(&bufList,
                                           ipcFramesObj->frameObj,
                                           OSA_ARRAYSIZE(ipcFramesObj->frameObj));
        }
        printStatsInterval++;
        OSA_waitMsecs(MCFW_IPCFRAMES_SENDFXN_PERIOD_MS);
    }
    OSA_printf("MCFW_IPCFRAMES:%s:Leaving...",__func__);
    return NULL;
}


static Void  VdecVdis_ipcFramesInitThrObj( VdecVdis_IpcFramesCtrl *ipcFramesObj)
{
    VdecVdis_IpcFramesCtrlThrObj *thrObj;

    thrObj = &ipcFramesObj->thrObj;
    thrObj->exitFramesOutThread = FALSE;
    OSA_semCreate(&thrObj->thrStartSem,1,0);
    OSA_thrCreate(&thrObj->thrHandleFramesOut,
                   VdecVdis_ipcFramesSendFxn,
                  MCFW_IPCFRAMES_SENDFXN_TSK_PRI,
                  MCFW_IPCFRAMES_SENDFXN_TSK_STACK_SIZE,
                  ipcFramesObj);

}

static Void  VdecVdis_ipcFramesDeInitThrObj( VdecVdis_IpcFramesCtrlThrObj *thrObj)
{
    thrObj->exitFramesOutThread = TRUE;
    OSA_thrDelete(&thrObj->thrHandleFramesOut);
    OSA_semDelete(&thrObj->thrStartSem);
}

static Void  VdecVdis_ipcFramesInitFrame(VdecVdis_IpcFramesBufObj *frameObj,
                                         Vsys_AllocBufInfo        *bufInfo,
                                         UInt32                   maxWidth,
                                         UInt32                   maxHeight,
                                         UInt32                   numCh)
{
    frameObj->bufPhy  = (UInt32)bufInfo->physAddr;
    frameObj->bufVirt = bufInfo->virtAddr;
    frameObj->maxWidth = maxWidth;
    frameObj->maxHeight = maxHeight;
    frameObj->curWidth = maxWidth;
    frameObj->curHeight = maxHeight;
    frameObj->numCh     = numCh;
    frameObj->refCnt    = 0;
}

static Void VdecVdis_ipcFramesInitFrameBuf(char *fileName,
                                           VdecVdis_IpcFramesBufObj *frameObj,
                                           UInt32 bufSize)
{
    FILE *fp;
    UInt32 numBytesRead;

    fp = fopen(fileName,"rb");
    if (fp)
    {
        numBytesRead =  fread(frameObj->bufVirt,  sizeof(char), bufSize, fp);
        if (numBytesRead != bufSize)
        {
            printf ("DEMO_IPCFRAMES_SEND:!!WARNING.File size [%d] < frameSize [%d]\n",
                    numBytesRead,bufSize);
        }
        fclose(fp);
    }
    else
    {
        printf ("DEMO_IPCFRAMES_SEND:!!WARNING.Unable to open file: %s. "
                "Frame content will not be initialized\n",
                fileName);
    }
}

static Void  VdecVdis_ipcFramesGetInputFileName(char *path,
                                                UInt32 frameNum,
                                                UInt32 maxFileNameSize)
{
    printf("\n");
    printf("Enter input frame file for frame:%d",frameNum);
    fflush(stdin);
    fgets(path, maxFileNameSize, stdin);
}

static Void  VdecVdis_ipcFramesInitFrameObj(VdecVdis_IpcFramesBufObj frameObj[],
                                            UInt32 numFrames,
                                            VIDEO_CHANNEL_LIST_INFO_S *chInfoList)
{
    Int i;
    Vsys_AllocBufInfo bufInfo;
    Int32 status;
    UInt32 frameSize;
    char   frameFileName[MAX_INPUT_STR_SIZE];
    UInt32 minChWidth = MCFW_IPCFRAMES_FRAME_INVALID_WIDTH;
    UInt32 minChHeight = MCFW_IPCFRAMES_FRAME_INVALID_HEIGHT;

    OSA_assert(chInfoList->numCh > 0);
    for (i = 0; i < chInfoList->numCh;i++)
    {
        if (chInfoList->chInfo[i].width < minChWidth)
        {
            minChWidth = chInfoList->chInfo[i].width;
        }
        if (chInfoList->chInfo[i].height < minChHeight)
        {
            minChHeight = chInfoList->chInfo[i].height;
        }
    }
    frameSize = MCFW_IPCBITS_GET_YUV422_FRAME_SIZE(minChWidth,
                                                   minChHeight);
    memset(frameObj,0,sizeof(VdecVdis_IpcFramesBufObj) * numFrames);
    for (i = 0; i < numFrames;i++)
    {
        status =
            Vsys_allocBuf(MCFW_IPCFRAMES_SRID,
                          frameSize,
                          MCFW_IPCFRAMES_FRAME_BUF_ALIGN,
                          &bufInfo);
        if (ERROR_NONE == status)
        {
            VdecVdis_ipcFramesInitFrame(&frameObj[i],
                                        &bufInfo,
                                        minChWidth,
                                        minChHeight,
                                        chInfoList->numCh);
            VdecVdis_ipcFramesGetInputFileName(frameFileName,
                                               i,
                                               sizeof(frameFileName)); 
            VdecVdis_ipcFramesInitFrameBuf(frameFileName,
                                           &frameObj[i],
                                           frameSize);
        }
    }

}

static Void  VdecVdis_ipcFramesDeInitFrameObj(VdecVdis_IpcFramesBufObj frameObj[],
                                              UInt32 numFrames)
{
    Int i;
    Int status;

    for (i = 0; i < numFrames;i++)
    {
        if (frameObj[i].bufVirt)
        {
            status =
            Vsys_freeBuf(MCFW_IPCFRAMES_SRID,
                         frameObj[i].bufVirt,
                         MCFW_IPCBITS_GET_YUV422_FRAME_SIZE(frameObj[i].maxWidth,
                                                            frameObj[i].maxHeight));
            OSA_assert(status ==0);
        }
    }
}

Int32  VdecVdis_ipcFramesCreate()
{
    VdecVdis_ipcFramesInitThrObj(&gVdecVdis_config.ipcFrames);
    return OSA_SOK;
}

Void  VdecVdis_ipcFramesStop(void)
{
    gVdecVdis_config.ipcFrames.thrObj.exitFramesOutThread = TRUE;
}

Void  VdecVdis_ipcFramesStart(void)
{
    VIDEO_CHANNEL_LIST_INFO_S chInfoList;
    Int32 status;

    /* We need to initialize the frameObj only at start time and
     * not at create time. This is because the SharedRegion is
     * initialized only after Vsys_create().
     * Also the number of channels to be displayed can be queries
     * from VDIS module only after Vdis_create()
     */
    status = Vdis_getChannelInfo(&chInfoList);
    OSA_assert(status == ERROR_NONE);
    VdecVdis_ipcFramesInitFrameObj(gVdecVdis_config.ipcFrames.frameObj,
                                   MCFW_IPCFRAMES_MAX_NUM_ALLOC_FRAMES,
                                   &chInfoList);

    OSA_semSignal(&gVdecVdis_config.ipcFrames.thrObj.thrStartSem);
}

Int32  VdecVdis_ipcFramesDelete()
{
    OSA_printf("Entered:%s...",__func__);
    VdecVdis_ipcFramesDeInitThrObj(&gVdecVdis_config.ipcFrames.thrObj);
    VdecVdis_ipcFramesDeInitFrameObj(gVdecVdis_config.ipcFrames.frameObj,
                                     MCFW_IPCFRAMES_MAX_NUM_ALLOC_FRAMES);
    OSA_printf("Leaving:%s...",__func__);
    return OSA_SOK;
}



/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <stddef.h>
#include "ipcFramesOutLink_priv.h"
#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/syslink/utils/Cache.h>

#define IPC_FRAMESOUT_ENABLE_PROFILE                                       (TRUE)

IpcFramesOutLink_Obj gIpcFramesOutLink_obj[IPC_FRAMES_OUT_LINK_OBJ_MAX];



Void IpcFramesOutLink_notifyCb(OSA_TskHndl * pTsk)
{
    Int32 status;
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;

    if (pObj->prd.numPendingCmd < IPC_FRAMESOUT_MAX_PENDING_CMDS)
    {
        pObj->prd.numPendingCmd++;
        status = OSA_tskSendMsg(pTsk, NULL, SYSTEM_IPC_CMD_RELEASE_FRAMES, NULL, 0);
    }
    else
    {
        UInt32 curTime = OSA_getCurTimeInMsec();

        OSA_printf("IPC_FRAMESOUTLINK:!WARNING!.Commands not being processed by link."
                   "TimeSinceLastPutFull:%d,TimeSinceLastGetFree:%d",
                  (curTime - pObj->stats.lastPutFullBufTime),
                  (curTime - pObj->stats.lastGetFreeBufTime));
    }
}

static Void IpcFramesOutLink_initStats(IpcFramesOutLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}

Void *IpcFramesOutLink_periodicTaskFxn(Void * prm)
{
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) prm;
    Int32 status;

    while (FALSE == pObj->prd.exitThread)
    {
        OSA_waitMsecs(IPC_FRAMESOUT_LINK_DONE_PERIOD_MS);
        if (pObj->prd.numPendingCmd < IPC_FRAMESOUT_MAX_PENDING_CMDS)
        {
            pObj->prd.numPendingCmd++;
            status = System_sendLinkCmd(pObj->tskId, SYSTEM_IPC_CMD_RELEASE_FRAMES);
        }
        else
        {
            UInt32 curTime = OSA_getCurTimeInMsec();

            OSA_printf("IPC_FRAMESOUTLINK:!WARNING!.Commands not being processed by link."
                   "TimeSinceLastPutFull:%d,TimeSinceLastGetFree:%d",
                  (curTime - pObj->stats.lastPutFullBufTime),
                  (curTime - pObj->stats.lastGetFreeBufTime));
        }
    }
    return NULL;
}


static Int32 IpcFramesOutLink_createPrdObj(IpcFramesOutLink_Obj * pObj)
{
    pObj->prd.numPendingCmd = 0;
    pObj->prd.exitThread = FALSE;
    OSA_thrCreate(&pObj->prd.thrHandle,
                  IpcFramesOutLink_periodicTaskFxn,
                  IPC_LINK_TSK_PRI, IPC_LINK_TSK_STACK_SIZE, pObj);
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

static
Int32 IpcFramesOutLink_deletePrdObj(IpcFramesOutLink_Obj * pObj)
{
    pObj->prd.exitThread = TRUE;
    OSA_thrDelete(&pObj->prd.thrHandle);
    pObj->prd.numPendingCmd = 0;
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

Int32 IpcFramesOutLink_create(IpcFramesOutLink_Obj * pObj,
                            IpcFramesOutLinkRTOS_CreateParams * pPrm)
{
    Int32 status, elemId;

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_OUT   : Create in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif

    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    status = System_ipcListMPReset(pObj->listMPOutHndl, pObj->listMPInHndl);
    OSA_assert(status == OSA_SOK);

    status = OSA_queCreate(&pObj->listElemQue,
                             SYSTEM_IPC_FRAMES_MAX_LIST_ELEM);
    OSA_assert(status == OSA_SOK);

    for (elemId = 0; elemId < SYSTEM_IPC_FRAMES_MAX_LIST_ELEM; elemId++)
    {
        SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pObj->listElem[elemId]->bufState);
        SYSTEM_IPC_FRAMES_SET_BUFSTATE(pObj->listElem[elemId]->bufState,
                                     IPC_FRAMEBUF_STATE_FREE);
        status =
            OSA_quePut(&pObj->listElemQue,
                       (Int32)pObj->listElem[elemId],
                       OSA_TIMEOUT_NONE);
        OSA_assert(status == OSA_SOK);
    }

    status = OSA_queCreate(&pObj->emptyFrameBufQue,
                         SYSTEM_IPC_FRAMES_MAX_LIST_ELEM);
    OSA_assert(status == OSA_SOK);
    pObj->info.numQue = 1;
    pObj->info.queInfo[0] = pObj->createArgs.inQueInfo;

    IpcFramesOutLink_initStats(pObj);
    if (TRUE == pObj->createArgs.baseCreateParams.noNotifyMode)
    {
        IpcFramesOutLink_createPrdObj(pObj);
    }

    pObj->startProcessing = TRUE;

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_OUT   : Create Done !!!\n", OSA_getCurTimeInMsec());
#endif

    return OSA_SOK;
}

Int32 IpcFramesOutLink_delete(IpcFramesOutLink_Obj * pObj)
{
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_OUT   : Delete in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif

    IPCFRAMESOUTLINK_INFO_LOG(pObj->tskId,
                            "RECV:%d,DROPPED:%d,FREED:%d,AVGLATENCY:%d",
                            pObj->stats.recvCount,
                            pObj->stats.droppedCount,
                            pObj->stats.freeCount,
                    OSA_DIV(pObj->stats.totalRoundTrip ,
                            pObj->stats.freeCount));


    OSA_queDelete(&pObj->listElemQue);
    OSA_queDelete(&pObj->emptyFrameBufQue);
    if (TRUE == pObj->createArgs.baseCreateParams.noNotifyMode)
    {
        IpcFramesOutLink_deletePrdObj(pObj);
    }
    pObj->startProcessing = FALSE;
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_OUT   : Delete Done !!!\n", OSA_getCurTimeInMsec());
#endif

    return OSA_SOK;
}


static UInt32 IpcFrameOutLink_mapDataFormat2NumPlanes(System_VideoDataFormat format)
{
    UInt32 numPlanes = 0;
    
    switch(format)
    {
        case SYSTEM_DF_YUV422I_UYVY:
        case SYSTEM_DF_YUV422I_YUYV:
        case SYSTEM_DF_YUV422I_YVYU:
        case SYSTEM_DF_YUV422I_VYUY:
        case SYSTEM_DF_YUV444I:
        case SYSTEM_DF_RGB16_565:
        case SYSTEM_DF_ARGB16_1555:
        case SYSTEM_DF_RGBA16_5551:
        case SYSTEM_DF_ARGB16_4444:
        case SYSTEM_DF_RGBA16_4444:
        case SYSTEM_DF_ARGB24_6666:
        case SYSTEM_DF_RGBA24_6666:
        case SYSTEM_DF_RGB24_888:
        case SYSTEM_DF_ARGB32_8888:
        case SYSTEM_DF_RGBA32_8888:
        case SYSTEM_DF_BGR16_565:
        case SYSTEM_DF_ABGR16_1555:
        case SYSTEM_DF_ABGR16_4444:
        case SYSTEM_DF_BGRA16_5551:
        case SYSTEM_DF_BGRA16_4444:
        case SYSTEM_DF_ABGR24_6666:
        case SYSTEM_DF_BGR24_888:
        case SYSTEM_DF_ABGR32_8888:
        case SYSTEM_DF_BGRA24_6666:
        case SYSTEM_DF_BGRA32_8888:
            numPlanes = 1;
            break;
        case SYSTEM_DF_YUV422SP_UV:
        case SYSTEM_DF_YUV422SP_VU:
        case SYSTEM_DF_YUV420SP_UV:
        case SYSTEM_DF_YUV420SP_VU:
            numPlanes = 2;
            break;
        case SYSTEM_DF_YUV422P:
        case SYSTEM_DF_YUV420P:
        case SYSTEM_DF_YUV444P:
            numPlanes = 3;
            break;
        default:
            numPlanes = 0;
            break;
    }
    OSA_assert(numPlanes != 0);
    return numPlanes;
}


static Void IpcFramesOutLink_copyFrameInfoEmptyFrame(IpcFramesOutLink_Obj * pObj,
                                                     VIDFrame_Buf  *src,
                                                     VIDFrame_Buf  *dst)
{
    Int i;
    UInt32 numPlanes = 0;
    UInt32 fieldIdx;

    OSA_assert(pObj->info.numQue == 1);
    OSA_assert(src->channelNum <
                 pObj->info.queInfo[0].numCh);
    numPlanes =
      IpcFrameOutLink_mapDataFormat2NumPlanes((System_VideoDataFormat)
        pObj->info.queInfo[0].chInfo[src->channelNum].dataFormat);
    for (i = 0; i < numPlanes; i++)
    {
        if (src->fid == VIDFRAME_FID_BOTTOM)
        {
            fieldIdx = 1;
        }
        else
        {
            fieldIdx = 0;
        }
        /* force fieldIdx to zero since always addr[0] is only populated */
        fieldIdx = 0;
        dst->phyAddr[fieldIdx][i] = src->phyAddr[fieldIdx][i];
        dst->addr[fieldIdx][i]    = src->addr[fieldIdx][i];
    }
    dst->channelNum = src->channelNum;
    dst->fid        = src->fid;
    dst->timeStamp  = src->timeStamp;
    dst->frameWidth =  src->frameWidth;
    dst->frameHeight = src->frameHeight;
    dst->framePitch[0] = src->framePitch[0];
    dst->framePitch[1] = src->framePitch[1];
    dst->linkPrivate = src->linkPrivate;
}


static Void IpcFramesOutLink_copyFrameInfo(IpcFramesOutLink_Obj * pObj,
                                          VIDFrame_Buf  *src,
                                          VIDFrame_Buf *dst,
                                          volatile SharedRegion_SRPtr srPtr[VIDFRAME_MAX_FIELDS][VIDFRAME_MAX_PLANES])
{
    Int i;
    UInt32 numPlanes;
    UInt32 fieldIdx;
    
    OSA_assert(pObj->info.numQue == 1);
    OSA_assert(src->channelNum <
                 pObj->info.queInfo[0].numCh);
    numPlanes = 
      IpcFrameOutLink_mapDataFormat2NumPlanes((System_VideoDataFormat)
        pObj->info.queInfo[0].chInfo[src->channelNum].dataFormat);
    for (i = 0; i < numPlanes; i++)
    {
        if (src->fid == VIDFRAME_FID_BOTTOM)
        {
            fieldIdx = 1;
        }
        else
        {
            fieldIdx = 0;
        }
        /* force fieldIdx to zero since always addr[0] is only populated */
        fieldIdx = 0;
        dst->phyAddr[fieldIdx][i] = src->phyAddr[fieldIdx][i];
        dst->addr[fieldIdx][i]    = src->addr[fieldIdx][i];

        if (SYSTEM_MT_TILEDMEM == 
            pObj->info.queInfo[0].chInfo[src->channelNum].memType)
        {
            srPtr[fieldIdx][i] = (SharedRegion_SRPtr)IPC_LINK_INVALID_SRPTR;
        }
        else
        {
            UInt16 srId;

            srId = SharedRegion_getId(src->addr[fieldIdx][i]);
            if(srId != SharedRegion_INVALIDREGIONID)
            {
                srPtr[fieldIdx][i] =
                     SharedRegion_getSRPtr(src->addr[fieldIdx][i], srId);
            }
            else
            {
                srPtr[fieldIdx][i] = IPC_LINK_INVALID_SRPTR;
            }
        }
    }
    dst->channelNum = src->channelNum;
    dst->fid        = src->fid;
    dst->timeStamp  = src->timeStamp;
    dst->frameWidth =  src->frameWidth;
    dst->frameHeight = src->frameHeight;
    dst->framePitch[0] = src->framePitch[0];
    dst->framePitch[1] = src->framePitch[1];
    dst->linkPrivate = src->linkPrivate;
}

static
Int32 IpcFramesOutLink_copyFrameBufInfo2ListElem(IpcFramesOutLink_Obj * pObj,
                                             SystemIpcFrames_ListElem * pListElem,
                                             VIDFrame_Buf * pFrameBuf)
{
    IpcFramesOutLink_copyFrameInfo(pObj, pFrameBuf,
                                   &pListElem->frameBuf,
                                   pListElem->srBufPtr);
    /* No cache ops done since pListElem is allocated from non-cached memory */
    OSA_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

static
Int32 IpcFramesOutLink_mapListElem2FrameBuf(IpcFramesOutLink_Obj * pObj,
                                        SystemIpcFrames_ListElem * pListElem,
                                        VIDFrame_Buf ** pFrameBufPtr)
{
    *pFrameBufPtr = &pListElem->frameBuf;
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

Int32 IpcFramesOutLink_processFrameBufs(IpcFramesOutLink_Obj * pObj,
                                        VIDFrame_BufList *bufList)
{
    VIDFrame_Buf *pFrameBuf = NULL;
    SystemIpcFrames_ListElem *pListElem;
    Int32 status;
    Int32 bufId;
    UInt32 curTime;


    curTime = OSA_getCurTimeInMsec();
    if (bufList->numFrames)
    {
#ifdef SYSTEM_DEBUG_IPC_RT
        OSA_printf(" %d: IPC_FRAMES_OUT   : Received %d framebufs !!!\n",
                   OSA_getCurTimeInMsec(), bufList->numFrames);
#endif

        OSA_assert(bufList->numFrames <= VIDFRAME_MAX_FRAME_BUFS);
        pObj->stats.recvCount += bufList->numFrames;
        for (bufId = 0; bufId < bufList->numFrames; bufId++)
        {
            pFrameBuf = &bufList->frames[bufId];
            OSA_assert(pFrameBuf != NULL);
            status =
                OSA_queGet(&pObj->listElemQue, (Int32 *) & pListElem,
                           OSA_TIMEOUT_NONE);
            OSA_assert (status == OSA_SOK)
            OSA_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                         == IPC_FRAMEBUF_STATE_FREE);
            OSA_assert(SYSTEM_IPC_FRAMES_GET_BUFOWNERPROCID(pListElem->bufState)
                         == System_getSelfProcId());
            SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                         IPC_FRAMEBUF_STATE_ALLOCED);
            IpcFramesOutLink_copyFrameBufInfo2ListElem(pObj, pListElem, pFrameBuf);
            pListElem->timeStamp = curTime;
            SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                         IPC_FRAMEBUF_STATE_OUTQUE);
            status =
                ListMP_putTail(pObj->listMPOutHndl, (ListMP_Elem *) pListElem);
            OSA_assert(status == ListMP_S_SUCCESS);
        }

        if (pObj->createArgs.baseCreateParams.notifyNextLink)
        {
            OSA_assert(pObj->createArgs.baseCreateParams.numOutQue == 1);
            System_ipcSendNotify(pObj->createArgs.baseCreateParams.outQueParams[0].
                nextLink);
        }

    }

    return OSA_SOK;
}

static
Void IpcFramesOutLink_putListElemInFreeQ(IpcFramesOutLink_Obj * pObj,
                                         SystemIpcFrames_ListElem *pListElem)
{
    Int32 status;

    /* release ListElem back to queue */
    SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pListElem->bufState);
    SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                 IPC_FRAMEBUF_STATE_FREE);
    status = OSA_quePut(&pObj->listElemQue, (Int32)pListElem,
                        OSA_TIMEOUT_NONE);
    OSA_assert(status == OSA_SOK);
}

static
Int32 IpcFramesOutLink_releaseFrameBufs(IpcFramesOutLink_Obj * pObj)
{
    SystemIpcFrames_ListElem *pListElem = NULL;
    Int32 status;
    UInt32 curTime, roundTripTime;
    VIDFrame_Buf *pFrameBuf = NULL;
    UInt32 numFramesFreed;

    numFramesFreed = 0;
    curTime = OSA_getCurTimeInMsec();
    do
    {
        pListElem = ListMP_getHead(pObj->listMPInHndl);
        if (pListElem != NULL)
        {
            OSA_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                         == IPC_FRAMEBUF_STATE_INQUE);
            IpcFramesOutLink_mapListElem2FrameBuf(pObj, pListElem, &pFrameBuf);
            OSA_assert(pFrameBuf != NULL);
            if (curTime > pListElem->timeStamp)
            {
                roundTripTime = curTime - pListElem->timeStamp;
                pObj->stats.totalRoundTrip += roundTripTime;
            }
            status = OSA_quePut(&pObj->emptyFrameBufQue,
                                (Int32)pFrameBuf,
                                OSA_TIMEOUT_NONE);
            OSA_assert(status == OSA_SOK);
        }
    } while (pListElem != NULL);
#ifdef SYSTEM_DEBUG_IPC_RT
     OSA_printf(" %d: IPC_FRAMES_OUT   : Releasing %d framebufs !!!\n",
                   OSA_getCurTimeInMsec(), numFramesFreed);
#endif
    return OSA_SOK;
}

Int32 IpcFramesOutLink_getLinkInfo(OSA_TskHndl * pTsk, System_LinkInfo * info)
{
    System_LinkInfo linkInfo;
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;

    linkInfo.numQue = 1;
    linkInfo.queInfo[0] = pObj->createArgs.inQueInfo;
    memcpy(info, &linkInfo, sizeof(*info));

    return OSA_SOK;
}

static
Int IpcFramesOutLink_tskMain(struct OSA_TskHndl * pTsk, OSA_MsgHndl * pMsg,
                            Uint32 curState)
{
    UInt32 cmd = OSA_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int status = IPC_FRAMESOUT_LINK_S_SUCCESS;
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        OSA_tskAckOrFreeMsg(pMsg, OSA_EFAIL);
        return status;
    }

    status = IpcFramesOutLink_create(pObj, OSA_msgGetPrm(pMsg));

    OSA_tskAckOrFreeMsg(pMsg, status);

    if (status != OSA_SOK)
        return status;

    done = FALSE;
    ackMsg = FALSE;

    while (!done)
    {
        status = OSA_tskWaitMsg(pTsk, &pMsg);
        if (status != OSA_SOK)
            break;

        cmd = OSA_msgGetCmd(pMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;
            case SYSTEM_IPC_CMD_RELEASE_FRAMES:
                OSA_tskAckOrFreeMsg(pMsg, status);

#ifdef SYSTEM_DEBUG_IPC_RT
                OSA_printf(" %d: IPC_FRAMES_OUT   : Received Notify !!!\n",
                           OSA_getCurTimeInMsec());
#endif

                OSA_assert(pObj->prd.numPendingCmd > 0);
                pObj->prd.numPendingCmd--;
                IpcFramesOutLink_releaseFrameBufs(pObj);
                break;

            default:
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcFramesOutLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_FRAMES_OUT
    OSA_printf(" %d: IPC_FRAMES_OUT   : Delete Done !!!\n", OSA_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        OSA_tskAckOrFreeMsg(pMsg, status);

    return status;
}

Int32 IpcFramesOutLink_allocListElem(IpcFramesOutLink_Obj * pObj)
{
    UInt32 shAddr;
    UInt32 elemId;

    shAddr = System_ipcListMPAllocListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                              sizeof(SystemIpcFrames_ListElem) *
                                              SYSTEM_IPC_FRAMES_MAX_LIST_ELEM);

    for (elemId = 0; elemId < SYSTEM_IPC_FRAMES_MAX_LIST_ELEM; elemId++)
    {
        pObj->listElem[elemId] =
            (SystemIpcFrames_ListElem *) (shAddr +
                                        elemId *
                                        sizeof(SystemIpcFrames_ListElem));
    }

    return OSA_SOK;
}

Int32 IpcFramesOutLink_freeListElem(IpcFramesOutLink_Obj * pObj)
{
    System_ipcListMPFreeListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    (UInt32) pObj->listElem[0],
                                    sizeof(SystemIpcFrames_ListElem) *
                                    SYSTEM_IPC_FRAMES_MAX_LIST_ELEM);

    return OSA_SOK;
}

Int32 IpcFramesOutLink_initListMP(IpcFramesOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPCreate(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    pObj->tskId,
                                    &pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    OSA_assert(status == OSA_SOK);

    IpcFramesOutLink_allocListElem(pObj);

    return status;
}

Int32 IpcFramesOutLink_deInitListMP(IpcFramesOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPDelete(&pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    OSA_assert(status == OSA_SOK);

    IpcFramesOutLink_freeListElem(pObj);

    return status;
}

Int32 IpcFramesOutLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcFramesOutId;
    IpcFramesOutLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    //UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcFrames_ListElem, frameBuf) == 0);
    OSA_COMPILETIME_ASSERT(offsetof(VIDFrame_Buf, reserved) == 0);
    OSA_COMPILETIME_ASSERT(offsetof(SystemIpcFrames_ListElem, frameBuf) == 0);
    OSA_COMPILETIME_ASSERT(sizeof(((SystemIpcFrames_ListElem *) 0)->frameBuf.reserved) ==
                             sizeof(ListMP_Elem));
    for (ipcFramesOutId = 0; ipcFramesOutId < IPC_FRAMES_OUT_LINK_OBJ_MAX;
         ipcFramesOutId++)
    {
        pObj = &gIpcFramesOutLink_obj[ipcFramesOutId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_FRAMES_OUT_0) + ipcFramesOutId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.getLinkInfo = IpcFramesOutLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        OSA_SNPRINTF(tskName, "IPC_FRAMES_OUT%d", ipcFramesOutId);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcFramesOutLink_notifyCb);

        IpcFramesOutLink_initListMP(pObj);
        pObj->startProcessing = FALSE;
        status = OSA_tskCreate(&pObj->tsk,
                               IpcFramesOutLink_tskMain,
                               IPC_LINK_TSK_PRI,
                               IPC_LINK_TSK_STACK_SIZE, 0, pObj);
        OSA_assert(status == OSA_SOK);
    }

    return status;
}

Int32 IpcFramesOutLink_deInit()
{
    UInt32 ipcFramesOutId;
    IpcFramesOutLink_Obj *pObj;

    for (ipcFramesOutId = 0; ipcFramesOutId < IPC_FRAMES_OUT_LINK_OBJ_MAX;
         ipcFramesOutId++)
    {
        pObj = &gIpcFramesOutLink_obj[ipcFramesOutId];

        pObj->startProcessing = FALSE;
        OSA_tskDelete(&pObj->tsk);

        IpcFramesOutLink_deInitListMP(pObj);

    }
    return OSA_SOK;
}

static
Int32 IpcFramesOutLink_getEmptyFrames(IpcFramesOutLink_Obj *pObj,
                                      VIDFrame_BufList * pFrameBufList)
{
    UInt32 idx;
    Int32 status;
    VIDFrame_Buf *pFrame;

    for (idx = 0; idx < VIDFRAME_MAX_FRAME_BUFS; idx++)
    {
        status =
            OSA_queGet(&pObj->emptyFrameBufQue,
                       (Int32 *) & pFrame,
                         OSA_TIMEOUT_NONE);
        if (status != OSA_SOK)
            break;
        IpcFramesOutLink_copyFrameInfoEmptyFrame(pObj,
                                       pFrame,
                                       &pFrameBufList->frames[idx]);
        IpcFramesOutLink_putListElemInFreeQ(pObj,
                                            (SystemIpcFrames_ListElem *)pFrame);
    }

    pFrameBufList->numFrames = idx;
    pObj->stats.freeCount += pFrameBufList->numFrames;

    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

Int32 IpcFramesOutLink_putFullVideoFrames(UInt32 linkId,
                                          VIDFrame_BufList *bufList)
{
    OSA_TskHndl * pTsk;
    IpcFramesOutLink_Obj * pObj;
    Int status = IPC_FRAMESOUT_LINK_S_SUCCESS;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0 + IPC_FRAMES_OUT_LINK_OBJ_MAX))))
    {
        return IPC_FRAMESOUT_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
#ifdef SYSTEM_DEBUG_IPC_RT
    OSA_printf(" %d: IPC_FRAMES_OUT   : Sending %d framebufs !!!\n",
               OSA_getCurTimeInMsec(), bufList->numFrames);
#endif

    if (bufList->numFrames)
    {
        IpcFramesOutLink_processFrameBufs(pObj,bufList);
    }
    return status;
}


Int32 IpcFramesOutLink_getEmptyVideoFrames(UInt32 linkId,
                                           VIDFrame_BufList *bufList)
{
    OSA_TskHndl * pTsk;
    IpcFramesOutLink_Obj * pObj;
    Int status;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0 + IPC_FRAMES_OUT_LINK_OBJ_MAX))))
    {
        return IPC_FRAMESOUT_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    status = IpcFramesOutLink_getEmptyFrames(pObj,bufList);
    return status;
}

Int32 IpcFramesOutLink_getInQueInfo(UInt32 linkId,
                                    System_LinkQueInfo *inQueInfo)
{
    OSA_TskHndl * pTsk;
    IpcFramesOutLink_Obj * pObj;

    OSA_assert(inQueInfo != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0 + IPC_FRAMES_OUT_LINK_OBJ_MAX))))
    {
        return IPC_FRAMESOUT_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    if (pObj->startProcessing)
    {
        *inQueInfo = pObj->createArgs.inQueInfo;
    }
    else
    {
        inQueInfo->numCh = 0;
    }
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}



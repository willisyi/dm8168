/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <stddef.h>
#include "ipcFramesInLink_priv.h"

#pragma DATA_ALIGN(gIpcFramesInLink_tskStack, 32)
#pragma DATA_SECTION(gIpcFramesInLink_tskStack, ".bss:taskStackSection")
UInt8
    gIpcFramesInLink_tskStack[IPC_FRAMES_IN_LINK_OBJ_MAX][IPC_LINK_TSK_STACK_SIZE];

IpcFramesInLink_Obj gIpcFramesInLink_obj[IPC_FRAMES_IN_LINK_OBJ_MAX];

static Int32 IpcFramesInLink_reconfigPrdObj(IpcFramesInLink_Obj * pObj,
                                          UInt period);

Void IpcFramesInLink_notifyCb(Utils_TskHndl * pTsk)
{
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;

    IpcFramesInLink_reconfigPrdObj(pObj, IPC_FRAMES_IN_LINK_DONE_PERIOD_MS);
    Utils_tskSendCmd(pTsk, SYSTEM_CMD_NEW_DATA);
}

static IpcFramesInLink_initStats(IpcFramesInLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}

static Void IpcFramesInLink_prdCalloutFcn(UArg arg)
{
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->tskId, SYSTEM_CMD_NEW_DATA);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("IPCFRAMEINLINK:[%s:%d]:"
                   "System_sendLinkCmd SYSTEM_IPC_CMD_RELEASE_FRAMES failed"
                   "errCode = %d", __FILE__, __LINE__, status);
        #endif
    }
}

static Int32 IpcFramesInLink_createPrdObj(IpcFramesInLink_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->prd.clkHandle == NULL);

    Clock_construct(&(pObj->prd.clkStruct),
                    IpcFramesInLink_prdCalloutFcn, 1, &clockParams);

    pObj->prd.clkHandle = Clock_handle(&pObj->prd.clkStruct);
    pObj->prd.clkStarted = FALSE;

    return IPC_FRAMES_IN_LINK_S_SUCCESS;

}

static Int32 IpcFramesInLink_deletePrdObj(IpcFramesInLink_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->prd.clkHandle);
    Clock_destruct(&(pObj->prd.clkStruct));
    pObj->prd.clkHandle = NULL;
    pObj->prd.clkStarted = FALSE;

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static Int32 IpcFramesInLink_startPrdObj(IpcFramesInLink_Obj * pObj, UInt period,
                                       Bool oneShotMode)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (FALSE == pObj->prd.clkStarted)
    {
        if (TRUE == oneShotMode)
        {
            Clock_setPeriod(pObj->prd.clkHandle, 0);
        }
        else
        {
            Clock_setPeriod(pObj->prd.clkHandle, period);
        }
        Clock_setTimeout(pObj->prd.clkHandle, period);
        Clock_start(pObj->prd.clkHandle);
        pObj->prd.clkStarted = TRUE;
    }

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static Int32 IpcFramesInLink_reconfigPrdObj(IpcFramesInLink_Obj * pObj, UInt period)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (TRUE == pObj->prd.clkStarted)
    {
        Clock_stop(pObj->prd.clkHandle);
    }
    Clock_setPeriod(pObj->prd.clkHandle, 0);
    Clock_setTimeout(pObj->prd.clkHandle, period);
    Clock_start(pObj->prd.clkHandle);
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Void IpcFramesInLink_setOutQueInfo(IpcFramesInLink_Obj *pObj)
{
    UInt32 totalNumInputChannels = 0;
    Int i;
    UInt32 chPerQueue,chId;
    UInt32 outChId,queueId;
    UInt32 inChId,inQueueId;

    for (i = 0; i < pObj->inQueInfo.numQue;i++)
    {
        totalNumInputChannels += pObj->inQueInfo.queInfo[i].numCh;
    }
    chPerQueue = (totalNumInputChannels / pObj->createArgs.baseCreateParams.numOutQue);
    pObj->info.numQue = pObj->createArgs.baseCreateParams.numOutQue;

    inQueueId = 0;
    inChId    = 0;
    for (chId = 0; chId < totalNumInputChannels;chId++)
    {
        outChId = chId % chPerQueue;
        queueId = chId / chPerQueue;
        UTILS_assert(queueId < pObj->info.numQue);

        pObj->info.queInfo[queueId].numCh = chPerQueue;
        UTILS_assert((inQueueId < pObj->inQueInfo.numQue)
                      &&
                      (inChId   < pObj->inQueInfo.queInfo[inQueueId].numCh));
        pObj->info.queInfo[queueId].chInfo[outChId] =
            pObj->inQueInfo.queInfo[inQueueId].chInfo[inChId];
        inChId++;
        if (inChId >= pObj->inQueInfo.queInfo[inQueueId].numCh)
        {
            inChId = 0;
            inQueueId++;
        }
    }
}

Int32 IpcFramesInLink_create(IpcFramesInLink_Obj * pObj,
                           IpcFramesInLinkRTOS_CreateParams * pPrm)
{
    Int32 status;
    System_LinkInQueParams *pInQueParams;
    Int   i;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_IN   : Create in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    UTILS_MEMLOG_USED_START();
    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    status =
        System_ipcListMPOpen(pObj->createArgs.baseCreateParams.inQueParams.
                             prevLinkId, &pObj->listMPOutHndl,
                             &pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pObj->listMPInHndl != NULL);

    while(!ListMP_empty(pObj->listMPInHndl))
    {
        Ptr listElem =
        ListMP_getHead(pObj->listMPInHndl);
        if (listElem != NULL)
        {
            IPCFRAMESINLINK_INFO_LOG(pObj->tskId,
                                   "InList not empty!!!"
                                   "Stale entry:[%p]",
                                   listElem);
        }
    }


    status = Utils_queCreate(&pObj->outFrameBufQue,
                             SYSTEM_IPC_FRAMES_MAX_LIST_ELEM,
                             pObj->outFrameBufQueMem,
                             UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == FVID2_SOK);

    status = Utils_queCreate(&pObj->freeFrameQue,
                             SYSTEM_IPC_FRAMES_MAX_LIST_ELEM,
                             pObj->freeFrameQueMem,
                             UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == FVID2_SOK);

    for (i = 0; i < SYSTEM_IPC_FRAMES_MAX_LIST_ELEM; i++)
    {
        pObj->freeFrameMem[i].appData = &pObj->freeFrameInfoMem[i];
        Utils_quePut(&pObj->freeFrameQue,&pObj->freeFrameMem[i],BIOS_NO_WAIT);
    }
    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    UTILS_assert(status == FVID2_SOK);

    IpcFramesInLink_setOutQueInfo(pObj);
    IpcFramesInLink_initStats(pObj);

    IpcFramesInLink_createPrdObj(pObj);

    if (TRUE == pObj->createArgs.baseCreateParams.noNotifyMode)
    {
        if (FALSE == pObj->prd.clkStarted)
        {
            IpcFramesInLink_startPrdObj(pObj,
                                      IPC_FRAMES_IN_LINK_DONE_PERIOD_MS, FALSE);
        }
    }
    pObj->state = IPC_FRAMES_IN_LINK_STATE_ACTIVE;
    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("IPC_FRAMES_IN",
                       pObj->memUsed,
                       UTILS_ARRAYSIZE(pObj->memUsed));

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_IN   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_freeQueuedInputBufs(IpcFramesInLink_Obj * pObj)
{
    SystemIpcFrames_ListElem *pListElem;
    Int32 status;
    UInt32 freeCount;

    freeCount = 0;
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        UTILS_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                     == IPC_FRAMEBUF_STATE_OUTQUE);
        SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                     IPC_FRAMEBUF_STATE_INQUE);
        status = ListMP_putTail(pObj->listMPInHndl, (ListMP_Elem *) pListElem);
        UTILS_assert(status == ListMP_S_SUCCESS);
        pObj->stats.droppedCount++;
        freeCount++;
    }

    if (freeCount)
    {
        if(pObj->createArgs.baseCreateParams.notifyPrevLink)
        {
            System_ipcSendNotify(pObj->createArgs.baseCreateParams.inQueParams.
                                 prevLinkId);
        }
    }

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_stop(IpcFramesInLink_Obj * pObj)
{
    pObj->state = IPC_FRAMES_IN_LINK_STATE_INACTIVE;
    IpcFramesInLink_freeQueuedInputBufs(pObj);
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

Int32 IpcFramesInLink_delete(IpcFramesInLink_Obj * pObj)
{
    Int32 status;

    IPCFRAMESINLINK_INFO_LOG(pObj->tskId,
                           "RECV:%d\tFREE:%d,DROPPED:%d,AVGLATENCY:%d",
                           pObj->stats.recvCount,
                           pObj->stats.freeCount,
                           pObj->stats.droppedCount,
                 UTILS_DIV(pObj->stats.totalRoundTrip ,
                            pObj->stats.freeCount));
#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_IN   : Delete in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    status = System_ipcListMPClose(&pObj->listMPOutHndl, &pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    Utils_queDelete(&pObj->outFrameBufQue);
    IpcFramesInLink_deletePrdObj(pObj);

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_IN   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static UInt32 IpcFrameInLink_mapDataFormat2NumPlanes(FVID2_DataFormat format)
{
    UInt32 numPlanes = 0;

    switch(format)
    {
        case FVID2_DF_YUV422I_UYVY:
        case FVID2_DF_YUV422I_YUYV:
        case FVID2_DF_YUV422I_YVYU:
        case FVID2_DF_YUV422I_VYUY:
        case FVID2_DF_YUV444I:
        case FVID2_DF_RGB16_565:
        case FVID2_DF_ARGB16_1555:
        case FVID2_DF_RGBA16_5551:
        case FVID2_DF_ARGB16_4444:
        case FVID2_DF_RGBA16_4444:
        case FVID2_DF_ARGB24_6666:
        case FVID2_DF_RGBA24_6666:
        case FVID2_DF_RGB24_888:
        case FVID2_DF_ARGB32_8888:
        case FVID2_DF_RGBA32_8888:
        case FVID2_DF_BGR16_565:
        case FVID2_DF_ABGR16_1555:
        case FVID2_DF_ABGR16_4444:
        case FVID2_DF_BGRA16_5551:
        case FVID2_DF_BGRA16_4444:
        case FVID2_DF_ABGR24_6666:
        case FVID2_DF_BGR24_888:
        case FVID2_DF_ABGR32_8888:
        case FVID2_DF_BGRA24_6666:
        case FVID2_DF_BGRA32_8888:
            numPlanes = 1;
            break;
        case FVID2_DF_YUV422SP_UV:
        case FVID2_DF_YUV422SP_VU:
        case FVID2_DF_YUV420SP_UV:
        case FVID2_DF_YUV420SP_VU:
            numPlanes = 2;
            break;
        case FVID2_DF_YUV422P:
        case FVID2_DF_YUV420P:
        case FVID2_DF_YUV444P:
            numPlanes = 3;
            break;
        default:
            numPlanes = 0;
    }
    UTILS_assert(numPlanes != 0);
    return numPlanes;
}

static Void IpcFramesInLink_copyFrameInfo(IpcFramesInLink_Obj * pObj,
                                          VIDFrame_Buf *src,
                                          FVID2_Frame  *dst,
                                          volatile SharedRegion_SRPtr srPtr[VIDFRAME_MAX_FIELDS][VIDFRAME_MAX_PLANES])
{
    Int i;
    UInt32 numPlanes;
    Uint32 fieldIdx;
    System_FrameInfo *frmInfo;

    //UTILS_assert(pObj->inQueInfo.numQue == 1);
    UTILS_COMPILETIME_ASSERT(VIDFRAME_MAX_FIELDS == FVID2_MAX_FIELDS);
    UTILS_COMPILETIME_ASSERT(VIDFRAME_MAX_PLANES == FVID2_MAX_PLANES);
    numPlanes =
      IpcFrameInLink_mapDataFormat2NumPlanes((FVID2_DataFormat)
        pObj->inQueInfo.queInfo[0].chInfo[src->channelNum].dataFormat);
    for (i = 0; i < numPlanes; i++)
    {
        if (src->fid == FVID2_FID_BOTTOM)
        {
            fieldIdx = 1;
        }
        else
        {
            fieldIdx = 0;
        }
        /* Force field idx to 0 irrespective of top/bot field since
         * other links expect in this format
         */
        fieldIdx = 0;

        if (SYSTEM_MT_TILEDMEM ==
            pObj->inQueInfo.queInfo[0].chInfo[src->channelNum].memType)
        {
            dst->addr[fieldIdx][i] = (Ptr)src->phyAddr[fieldIdx][i];
        }
        else
        {
            if (srPtr[fieldIdx][i] != IPC_LINK_INVALID_SRPTR)
            {
                dst->addr[fieldIdx][i] = SharedRegion_getPtr(srPtr[fieldIdx][i]);
            }
            else
            {
                /*!WARNING. Assume phyiscalAddr == VirtualAddr on RTOS side.
                 * This will break once we have mmu on RTOS side
                 * TODO: Invoke syslink API on RTOS side to map virt2phy
                 */
                dst->addr[fieldIdx][i] = src->phyAddr[fieldIdx][i];
            }
        }
    }
    dst->channelNum = src->channelNum;
    UTILS_assert(dst->channelNum < pObj->info.queInfo[0].numCh);
    dst->fid        = src->fid;
    dst->timeStamp  = src->timeStamp;
    dst->blankData    = NULL;
    dst->drvData      = NULL;
    dst->perFrameCfg  = NULL;
    dst->subFrameInfo = NULL;
    frmInfo           = dst->appData;
    UTILS_assert(frmInfo != NULL);
    frmInfo->pOrgListMPElem  = src;

    frmInfo->rtChInfo.height  = src->frameHeight;
    frmInfo->rtChInfo.width  = src->frameWidth;
    frmInfo->rtChInfo.pitch[0]  = src->framePitch[0];
    frmInfo->rtChInfo.pitch[1]  = src->framePitch[1];
    /* HACK HACK HACK !!! modify rtChInfoUpdate = TRUE once the 
       next link support dynamic resolution change */
    frmInfo->rtChInfoUpdate = TRUE;//FALSE;

}


static Int32 IpcFramesInLink_getFrameBuf(IpcFramesInLink_Obj * pObj,
                                     SystemIpcFrames_ListElem * pListElem,
                                     FVID2_Frame ** pFrameBufPtr)
{
    FVID2_Frame *freeFrameBuf;
    Int status;


    UTILS_assert(pListElem != NULL);
    /* No cache ops done since pListElem is allocated from non-cached memory */
    UTILS_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    status = Utils_queGet(&pObj->freeFrameQue,
                          (Ptr *)&freeFrameBuf,1,BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);

    IpcFramesInLink_copyFrameInfo(pObj,
                                  &pListElem->frameBuf,
                                  freeFrameBuf,
                                  pListElem->srBufPtr);
    *pFrameBufPtr = freeFrameBuf;
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

Int32 IpcFramesInLink_processFrameBufs(IpcFramesInLink_Obj * pObj)
{
    FVID2_Frame *pFrameBuf;
    SystemIpcFrames_ListElem *pListElem;
    UInt32 numFrameBufs;
    Int32 status;
    UInt32 curTime;

    numFrameBufs = 0;
    curTime = Utils_getCurTimeInMsec();
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        IpcFramesInLink_getFrameBuf(pObj, pListElem, &pFrameBuf);
        UTILS_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                     == IPC_FRAMEBUF_STATE_OUTQUE);
        pListElem->frameBuf.reserved[0] = curTime;
        SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                     IPC_FRAMEBUF_STATE_DEQUEUED);
        pObj->stats.recvCount++;
        status = Utils_quePut(&pObj->outFrameBufQue, pFrameBuf, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);

        numFrameBufs++;
    }

#ifdef SYSTEM_DEBUG_IPC_RT
    Vps_printf(" %d: IPC_FRAMES_IN   : Recevived %d framebufs !!!\n",
               Utils_getCurTimeInMsec(), numFrameBufs);
#endif

    if (numFrameBufs && pObj->createArgs.baseCreateParams.notifyNextLink)
    {
        UTILS_assert(pObj->createArgs.baseCreateParams.numOutQue == 1);
        System_sendLinkCmd(pObj->createArgs.baseCreateParams.outQueParams[0].
                           nextLink, SYSTEM_CMD_NEW_DATA);
    }

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

Int32 IpcFramesInLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                   FVID2_FrameList * pFrameBufList)
{
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;
    UInt32 idx;
    Int32 status;

    if (IPC_FRAMES_IN_LINK_STATE_ACTIVE == pObj->state)
    {
        for (idx = 0; idx < FVID2_MAX_FVID_FRAME_PTR; idx++)
        {
          status =
            Utils_queGet(&pObj->outFrameBufQue, (Ptr *) & pFrameBufList->frames[idx],
                   1, BIOS_NO_WAIT);
          if (status != FVID2_SOK)
            break;
        }
 
        pFrameBufList->numFrames = idx;
    }
    else
    {
        Vps_printf(" %d: IPC_FRAMES_IN:Warning! Get Frames invoked after link delete\n",
                    Utils_getCurTimeInMsec());
        pFrameBufList->numFrames = 0;
    }

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

Int32 IpcFramesInLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                    FVID2_FrameList * pFrameBufList)
{
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;
    UInt32 bufId;
    FVID2_Frame *pFrameBuf;
    Int32 status;
    SystemIpcFrames_ListElem *pListElem = NULL;
    UInt32 curTime;
    System_FrameInfo *frmInfo;

#ifdef SYSTEM_DEBUG_IPC_RT
    Vps_printf(" %d: IPC_FRAMES_IN   : Releasing %d framebufs !!!\n",
               Utils_getCurTimeInMsec(), pFrameBufList->numFrames);
#endif

    if ((pFrameBufList->numFrames)
         &&
         (IPC_FRAMES_IN_LINK_STATE_ACTIVE == pObj->state))
    {
        UTILS_assert(pFrameBufList->numFrames <= FVID2_MAX_FVID_FRAME_PTR);
        curTime = Utils_getCurTimeInMsec();
        pObj->stats.freeCount += pFrameBufList->numFrames;
        for (bufId = 0; bufId < pFrameBufList->numFrames; bufId++)
        {
            pFrameBuf = pFrameBufList->frames[bufId];
            frmInfo   = pFrameBuf->appData;
            UTILS_assert(frmInfo != NULL);

            pListElem = (SystemIpcFrames_ListElem *) frmInfo->pOrgListMPElem;
            UTILS_assert(pListElem->frameBuf.phyAddr[0][0]  ==
                         pFrameBuf->addr[0][0]);
            UTILS_assert(UTILS_ARRAYISVALIDENTRY(pFrameBuf,pObj->freeFrameMem));
            UTILS_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                          == IPC_FRAMEBUF_STATE_DEQUEUED);
            status = Utils_quePut(&pObj->freeFrameQue,pFrameBuf,BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
            if (curTime > pListElem->frameBuf.reserved[0])
            {
                pObj->stats.totalRoundTrip += (curTime - pListElem->frameBuf.reserved[0]);
            }
            SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                         IPC_FRAMEBUF_STATE_INQUE);
            status =
                ListMP_putTail(pObj->listMPInHndl, (ListMP_Elem *) pListElem);
            UTILS_assert(status == ListMP_S_SUCCESS);
        }

        if (pObj->createArgs.baseCreateParams.notifyPrevLink)
        {
            System_ipcSendNotify(pObj->createArgs.baseCreateParams.inQueParams.
                                 prevLinkId);
        }
    }
    if (IPC_FRAMES_IN_LINK_STATE_INACTIVE == pObj->state)
    {
        Vps_printf(" %d: IPC_FRAMES_IN:Warning! Free Frames invoked after link delete\n",
                     Utils_getCurTimeInMsec());
    }
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

Int32 IpcFramesInLink_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

Void IpcFramesInLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = IpcFramesInLink_create(pObj, Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);

    if (status != FVID2_SOK)
        return;

    done = FALSE;
    ackMsg = FALSE;

    while (!done)
    {
        status = Utils_tskRecvMsg(pTsk, &pMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        cmd = Utils_msgGetCmd(pMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;
            case SYSTEM_CMD_NEW_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

                IpcFramesInLink_processFrameBufs(pObj);
                break;
            case SYSTEM_CMD_STOP:
                IpcFramesInLink_stop(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcFramesInLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_FRAMES_IN
    Vps_printf(" %d: IPC_FRAMES_IN   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 IpcFramesInLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcFramesInId;
    IpcFramesInLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

   // UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcFrames_ListElem, frameBuf) == 0);
    UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcFrames_ListElem, frameBuf) == 0);
    UTILS_COMPILETIME_ASSERT(offsetof(VIDFrame_Buf, reserved) == 0);
    UTILS_COMPILETIME_ASSERT(sizeof(((SystemIpcFrames_ListElem *) 0)->frameBuf.reserved) ==
                             sizeof(ListMP_Elem));
    for (ipcFramesInId = 0; ipcFramesInId < IPC_FRAMES_IN_LINK_OBJ_MAX; ipcFramesInId++)
    {
        pObj = &gIpcFramesInLink_obj[ipcFramesInId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_FRAMES_IN_0) + ipcFramesInId;

        pObj->state = IPC_FRAMES_IN_LINK_STATE_INACTIVE;
        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.linkGetFullFrames = IpcFramesInLink_getFullFrames;
        linkObj.linkPutEmptyFrames = IpcFramesInLink_putEmptyFrames;
        linkObj.getLinkInfo = IpcFramesInLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        UTILS_SNPRINTF(tskName, "IPC_FRAMES_IN%d", ipcFramesInId);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcFramesInLink_notifyCb);

        status = Utils_tskCreate(&pObj->tsk,
                                 IpcFramesInLink_tskMain,
                                 IPC_LINK_TSK_PRI,
                                 gIpcFramesInLink_tskStack[ipcFramesInId],
                                 IPC_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);	
    }

    return status;
}

Int32 IpcFramesInLink_deInit()
{
    UInt32 ipcFramesInId;

    for (ipcFramesInId = 0; ipcFramesInId < IPC_FRAMES_IN_LINK_OBJ_MAX; ipcFramesInId++)
    {
        Utils_tskDelete(&gIpcFramesInLink_obj[ipcFramesInId].tsk);
    }
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

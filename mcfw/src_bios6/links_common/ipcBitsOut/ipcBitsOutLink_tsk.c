/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "ipcBitsOutLink_priv.h"

#define IPC_BITS_IN_ENABLE_PROFILE                                       (TRUE)

#pragma DATA_ALIGN(gIpcBitsOutLink_tskStack, 32)
#pragma DATA_SECTION(gIpcBitsOutLink_tskStack, ".bss:taskStackSection")
UInt8
    gIpcBitsOutLink_tskStack[IPC_BITS_OUT_LINK_OBJ_MAX]
    [IPC_LINK_TSK_STACK_SIZE];

IpcBitsOutLink_Obj gIpcBitsOutLink_obj[IPC_BITS_OUT_LINK_OBJ_MAX];

static Int32 IpcBitsOutLink_reconfigPrdObj(IpcBitsOutLink_Obj * pObj,
                                           UInt period);

Void IpcBitsOutLink_notifyCb(Utils_TskHndl * pTsk)
{
    IpcBitsOutLink_Obj *pObj = (IpcBitsOutLink_Obj *) pTsk->appData;

    //IpcBitsOutLink_reconfigPrdObj(pObj, IPC_BITSOUT_LINK_DONE_PERIOD_MS);
    Utils_tskSendCmd(pTsk, SYSTEM_IPC_CMD_RELEASE_FRAMES);
}

static IpcBitsOutLink_initStats(IpcBitsOutLink_Obj * pObj)
{
    Utils_PrfTsHndl *tsHndlRestore;

    UTILS_assert(pObj->stats.tsHandle != NULL);
    tsHndlRestore = pObj->stats.tsHandle;
    memset(&pObj->stats, 0, sizeof(pObj->stats));
    pObj->stats.tsHandle = tsHndlRestore;
    /* Reset the timestamp counter */
    Utils_prfTsReset(pObj->stats.tsHandle);

}

static Void IpcBitsOutLink_prdCalloutFcn(UArg arg)
{
    IpcBitsOutLink_Obj *pObj = (IpcBitsOutLink_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->tskId, SYSTEM_IPC_CMD_RELEASE_FRAMES);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("IPCBITOUTLINK:[%s:%d]:"
                   "System_sendLinkCmd SYSTEM_IPC_CMD_RELEASE_FRAMES failed"
                   "errCode = %d", __FILE__, __LINE__, status);

        #endif
    }

}

static Int32 IpcBitsOutLink_createPrdObj(IpcBitsOutLink_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->prd.clkHandle == NULL);

    Clock_construct(&(pObj->prd.clkStruct),
                    IpcBitsOutLink_prdCalloutFcn, 1, &clockParams);

    pObj->prd.clkHandle = Clock_handle(&pObj->prd.clkStruct);
    pObj->prd.clkStarted = FALSE;

    return IPC_BITSOUT_LINK_S_SUCCESS;

}

static Int32 IpcBitsOutLink_deletePrdObj(IpcBitsOutLink_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->prd.clkHandle);
    Clock_destruct(&(pObj->prd.clkStruct));
    pObj->prd.clkHandle = NULL;
    pObj->prd.clkStarted = FALSE;

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static Int32 IpcBitsOutLink_startPrdObj(IpcBitsOutLink_Obj * pObj, UInt period,
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

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static Int32 IpcBitsOutLink_reconfigPrdObj(IpcBitsOutLink_Obj * pObj,
                                           UInt period)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (TRUE == pObj->prd.clkStarted)
    {
        Clock_stop(pObj->prd.clkHandle);
    }
    Clock_setPeriod(pObj->prd.clkHandle, 0);
    Clock_setTimeout(pObj->prd.clkHandle, period);
    Clock_start(pObj->prd.clkHandle);
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_create(IpcBitsOutLink_Obj * pObj,
                            IpcBitsOutLinkRTOS_CreateParams * pPrm)
{
    Int32 status, elemId;
    System_LinkInQueParams *pInQueParams;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_OUT   : Create in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif
    UTILS_MEMLOG_USED_START();
    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    status = System_ipcListMPReset(pObj->listMPOutHndl, pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    status = Utils_queCreate(&pObj->listElemQue,
                             SYSTEM_IPC_BITS_MAX_LIST_ELEM,
                             pObj->listElemQueMem,
                             UTILS_QUE_FLAG_BLOCK_QUE_GET);
    UTILS_assert(status == FVID2_SOK);

    for (elemId = 0; elemId < SYSTEM_IPC_BITS_MAX_LIST_ELEM; elemId++)
    {
        SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(pObj->listElem[elemId]->bufState);
        SYSTEM_IPC_BITS_SET_BUFSTATE(pObj->listElem[elemId]->bufState,
                                     IPC_BITBUF_STATE_FREE);
        status =
            Utils_quePut(&pObj->listElemQue, pObj->listElem[elemId],
                         BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
        pObj->listElem2BitBufMap[elemId] = NULL;
    }

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    UTILS_assert(status == FVID2_SOK);

    IpcBitsOutLink_initStats(pObj);
    IpcBitsOutLink_createPrdObj(pObj);
    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("IPCBITSOUT",
                       pObj->memUsed,
                       UTILS_ARRAYSIZE(pObj->memUsed));

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_OUT   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 IpcBitsOutLink_delete(IpcBitsOutLink_Obj * pObj)
{
#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_OUT   : Delete in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    IPCBITSOUTLINK_INFO_LOG(pObj->tskId,
                            "RECV:%d\tFREE:%d,DROPPED:%d,AVGLATENCY:%d",
                            pObj->stats.recvCount,
                            pObj->stats.freeCount,
                            pObj->stats.droppedCount,
                  UTILS_DIV(pObj->stats.totalRoundTrip ,
                             pObj->stats.freeCount));

#ifdef IPC_BITS_IN_ENABLE_PROFILE
    /* Print the timestamp differnce and reset the counter */
    Utils_prfTsPrint(pObj->stats.tsHandle, TRUE);
#endif                                                     /* IPC_BITS_IN_ENABLE_PROFILE
                                                            */

    Utils_queDelete(&pObj->listElemQue);
    IpcBitsOutLink_deletePrdObj(pObj);

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_OUT   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

static
Int32 IpcBitsOutLink_copyBitBufInfo2ListElem(IpcBitsOutLink_Obj * pObj,
                                             SystemIpcBits_ListElem * pListElem,
                                             Bitstream_Buf * pBitBuf)
{
    UInt32 listIdx;
    UInt16 srId;

    memcpy(&pListElem->bitBuf, pBitBuf, sizeof(*pBitBuf));
    srId = SharedRegion_getId(pBitBuf->addr);
    UTILS_assert(srId != SharedRegion_INVALIDREGIONID);
    pListElem->srBufPtr = SharedRegion_getSRPtr(pBitBuf->addr, srId);
    /* No cache ops done since pListElem is allocated from non-cached memory */
    UTILS_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    listIdx = pListElem - pObj->listElem[0];
    UTILS_assert(listIdx < UTILS_ARRAYSIZE(pObj->listElem2BitBufMap));
    UTILS_assert(pObj->listElem2BitBufMap[listIdx] == NULL);
    pObj->listElem2BitBufMap[listIdx] = pBitBuf;
    pListElem->ipcPrivData = pBitBuf;
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
Int32 IpcBitsOutLink_mapListElem2BitBuf(IpcBitsOutLink_Obj * pObj,
                                        SystemIpcBits_ListElem * pListElem,
                                        Bitstream_Buf ** pBitBufPtr)
{
    UInt32 listIdx;

    *pBitBufPtr = NULL;
    listIdx = pListElem - pObj->listElem[0];
    UTILS_assert(listIdx < UTILS_ARRAYSIZE(pObj->listElem2BitBufMap));
    UTILS_assert(pObj->listElem2BitBufMap[listIdx] != NULL);
    UTILS_assert(SharedRegion_getPtr(pListElem->srBufPtr) ==
                 pObj->listElem2BitBufMap[listIdx]->addr);
    *pBitBufPtr = pObj->listElem2BitBufMap[listIdx];
    UTILS_assert(((Ptr) (*pBitBufPtr)) == pListElem->ipcPrivData);
    pObj->listElem2BitBufMap[listIdx] = NULL;
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_processBitBufs(IpcBitsOutLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    Bitstream_BufList bufList;
    Bitstream_Buf *pBitBuf;
    SystemIpcBits_ListElem *pListElem;
    Int32 status;
    Int32 bufId;
    UInt32 curTime;

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    bufList.numBufs = 0;
    System_getLinksFullBufs(pInQueParams->prevLinkId,
                            pInQueParams->prevLinkQueId, &bufList);

    pObj->freeBitBufList.numBufs = 0;
    curTime = Utils_getCurTimeInMsec();
    if (bufList.numBufs)
    {
#ifdef SYSTEM_DEBUG_IPC_RT
        Vps_printf(" %d: IPC_BITS_OUT   : Received %d bitbufs !!!\n",
                   Utils_getCurTimeInMsec(), bufList.numBufs);
#endif

        UTILS_assert(bufList.numBufs <= VIDBITSTREAM_MAX_BITSTREAM_BUFS);
        pObj->stats.recvCount += bufList.numBufs;
#ifdef IPC_BITS_IN_ENABLE_PROFILE
        Utils_prfTsBegin(pObj->stats.tsHandle);
#endif                                                     /* IPC_BITS_IN_ENABLE_PROFILE
                                                            */
        for (bufId = 0; bufId < bufList.numBufs; bufId++)
        {
            pBitBuf = bufList.bufs[bufId];
            status =
                Utils_queGet(&pObj->listElemQue, (Ptr *) & pListElem, 1,
                             BIOS_WAIT_FOREVER);
            if (status != FVID2_SOK)
            {
                /* normally this condition should not happen, if it happens
                 * return the bitbuf back to its generator */
#if 0
                Vps_printf(" IPC_OUT: Dropping bitbuf\n");
#endif

                UTILS_assert(pObj->freeBitBufList.numBufs <
                             VIDBITSTREAM_MAX_BITSTREAM_BUFS);
                pObj->freeBitBufList.bufs[pObj->freeBitBufList.numBufs] =
                    pBitBuf;
                pObj->freeBitBufList.numBufs++;
                pObj->stats.droppedCount++;
                continue;
            }
            UTILS_assert(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
                         == IPC_BITBUF_STATE_FREE);
            UTILS_assert(SYSTEM_IPC_BITS_GET_BUFOWNERPROCID(pListElem->bufState)
                         == System_getSelfProcId());
            UTILS_assert(pBitBuf != NULL);
            SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                         IPC_BITBUF_STATE_ALLOCED);
            IpcBitsOutLink_copyBitBufInfo2ListElem(pObj, pListElem, pBitBuf);
            pBitBuf->timeStamp = curTime;
            SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                         IPC_BITBUF_STATE_OUTQUE);
            status =
                ListMP_putTail(pObj->listMPOutHndl, (ListMP_Elem *) pListElem);
            UTILS_assert(status == ListMP_S_SUCCESS);
        }

#ifdef IPC_BITS_IN_ENABLE_PROFILE
        Utils_prfTsEnd(pObj->stats.tsHandle, bufList.numBufs);
#endif                                                     /* IPC_BITS_IN_ENABLE_PROFILE
                                                            */

        if (pObj->freeBitBufList.numBufs)
        {
            System_putLinksEmptyBufs(pInQueParams->prevLinkId,
                                     pInQueParams->prevLinkQueId,
                                     &pObj->freeBitBufList);
        }

        if (pObj->createArgs.baseCreateParams.notifyNextLink)
        {
            UTILS_assert(pObj->createArgs.baseCreateParams.numOutQue == 1);
            System_ipcSendNotify(pObj->createArgs.baseCreateParams.outQueParams[0].
                                 nextLink);
        }
        if (pObj->createArgs.baseCreateParams.noNotifyMode)
        {
            if (FALSE == pObj->prd.clkStarted)
            {
                IpcBitsOutLink_startPrdObj(pObj,
                                           IPC_BITSOUT_LINK_DONE_PERIOD_MS,
                                           FALSE);
            }
        }
    }

    return FVID2_SOK;
}

Int32 IpcBitsOutLink_releaseBitBufs(IpcBitsOutLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    SystemIpcBits_ListElem *pListElem = NULL;
    Int32 status;
    UInt32 curTime, roundTripTime;
    Bitstream_Buf *pBitBuf = NULL;

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    do
    {
        pObj->freeBitBufList.numBufs = 0;

        curTime = Utils_getCurTimeInMsec();
        while (pObj->freeBitBufList.numBufs < VIDBITSTREAM_MAX_BITSTREAM_BUFS)
        {
            pListElem = ListMP_getHead(pObj->listMPInHndl);
            if (pListElem == NULL)
                break;
            UTILS_assert(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
                         == IPC_BITBUF_STATE_INQUE);
            IpcBitsOutLink_mapListElem2BitBuf(pObj, pListElem, &pBitBuf);
            UTILS_assert(pBitBuf != NULL);
            if (curTime > pBitBuf->timeStamp)
            {
                roundTripTime = curTime - pBitBuf->timeStamp;
                pObj->stats.totalRoundTrip += roundTripTime;
            }
            /* Restore the original timestamp as it may be used by next link */
            pBitBuf->timeStamp = pListElem->bitBuf.timeStamp;
            pObj->freeBitBufList.bufs[pObj->freeBitBufList.numBufs] = pBitBuf;
            pObj->stats.freeCount++;
            pObj->freeBitBufList.numBufs++;
            UTILS_assert(pObj->freeBitBufList.numBufs <=
                         VIDBITSTREAM_MAX_BITSTREAM_BUFS);
            /* release ListElem back to queue */
            SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(pListElem->bufState);
            SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                         IPC_BITBUF_STATE_FREE);
            status = Utils_quePut(&pObj->listElemQue, pListElem, BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
        }

#ifdef SYSTEM_DEBUG_IPC_RT
        Vps_printf(" %d: IPC_BITS_OUT   : Releasing %d bitbufs !!!\n",
                   Utils_getCurTimeInMsec(), pObj->freeBitBufList.numBufs);
#endif

        if (pObj->freeBitBufList.numBufs)
        {

            System_putLinksEmptyBufs(pInQueParams->prevLinkId,
                                     pInQueParams->prevLinkQueId,
                                     &pObj->freeBitBufList);
            if (pObj->createArgs.baseCreateParams.notifyPrevLink)
            {
                System_sendLinkCmd(pObj->createArgs.baseCreateParams.
                                   inQueParams.prevLinkId, SYSTEM_CMD_NEW_DATA);
            }
        }
    } while (pListElem != NULL);

    return FVID2_SOK;
}

Int32 IpcBitsOutLink_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcBitsOutLink_Obj *pObj = (IpcBitsOutLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->inQueInfo, sizeof(*info));

    return FVID2_SOK;
}

Void IpcBitsOutLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    IpcBitsOutLink_Obj *pObj = (IpcBitsOutLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = IpcBitsOutLink_create(pObj, Utils_msgGetPrm(pMsg));

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

                IpcBitsOutLink_processBitBufs(pObj);
                IpcBitsOutLink_releaseBitBufs(pObj);
                break;

            case SYSTEM_IPC_CMD_RELEASE_FRAMES:
                Utils_tskAckOrFreeMsg(pMsg, status);

#ifdef SYSTEM_DEBUG_IPC_RT
                Vps_printf(" %d: IPC_BITS_OUT   : Received Notify !!!\n",
                           Utils_getCurTimeInMsec());
#endif

                IpcBitsOutLink_releaseBitBufs(pObj);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcBitsOutLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_BITS_OUT
    Vps_printf(" %d: IPC_BITS_OUT   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 IpcBitsOutLink_allocListElem(IpcBitsOutLink_Obj * pObj)
{
    UInt32 shAddr;
    UInt32 elemId;

    shAddr = System_ipcListMPAllocListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                              sizeof(SystemIpcBits_ListElem) *
                                              SYSTEM_IPC_BITS_MAX_LIST_ELEM);

    for (elemId = 0; elemId < SYSTEM_IPC_BITS_MAX_LIST_ELEM; elemId++)
    {
        pObj->listElem[elemId] =
            (SystemIpcBits_ListElem *) (shAddr +
                                        elemId *
                                        sizeof(SystemIpcBits_ListElem));
    }

    return FVID2_SOK;
}

Int32 IpcBitsOutLink_freeListElem(IpcBitsOutLink_Obj * pObj)
{
    System_ipcListMPFreeListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    (UInt32) pObj->listElem[0],
                                    sizeof(SystemIpcBits_ListElem) *
                                    SYSTEM_IPC_BITS_MAX_LIST_ELEM);

    return FVID2_SOK;
}

Int32 IpcBitsOutLink_initListMP(IpcBitsOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPCreate(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    pObj->tskId,
                                    &pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    IpcBitsOutLink_allocListElem(pObj);

    return status;
}

Int32 IpcBitsOutLink_deInitListMP(IpcBitsOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPDelete(&pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    IpcBitsOutLink_freeListElem(pObj);

    return status;
}

Int32 IpcBitsOutLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcBitsOutId;
    IpcBitsOutLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcBits_ListElem, bitBuf) == 0);
    UTILS_COMPILETIME_ASSERT(offsetof(Bitstream_Buf, reserved) == 0);
    UTILS_COMPILETIME_ASSERT(sizeof(((Bitstream_Buf *) 0)->reserved) ==
                             sizeof(ListMP_Elem));
    for (ipcBitsOutId = 0; ipcBitsOutId < IPC_BITS_OUT_LINK_OBJ_MAX;
         ipcBitsOutId++)
    {
        pObj = &gIpcBitsOutLink_obj[ipcBitsOutId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_BITS_OUT_0) + ipcBitsOutId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.getLinkInfo = IpcBitsOutLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        UTILS_SNPRINTF(tskName, "IPC_BITS_OUT%d", ipcBitsOutId);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcBitsOutLink_notifyCb);

        IpcBitsOutLink_initListMP(pObj);

        pObj->stats.tsHandle = Utils_prfTsCreate(tskName);

        UTILS_assert(pObj->stats.tsHandle != NULL);

        status = Utils_tskCreate(&pObj->tsk,
                                 IpcBitsOutLink_tskMain,
                                 IPC_LINK_TSK_PRI,
                                 gIpcBitsOutLink_tskStack[ipcBitsOutId],
                                 IPC_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 IpcBitsOutLink_deInit()
{
    UInt32 ipcBitsOutId;
    IpcBitsOutLink_Obj *pObj;

    for (ipcBitsOutId = 0; ipcBitsOutId < IPC_BITS_OUT_LINK_OBJ_MAX;
         ipcBitsOutId++)
    {
        pObj = &gIpcBitsOutLink_obj[ipcBitsOutId];

        Utils_tskDelete(&pObj->tsk);

        IpcBitsOutLink_deInitListMP(pObj);

        UTILS_assert(pObj->stats.tsHandle != NULL);
        Utils_prfTsDelete(pObj->stats.tsHandle);
    }
    return FVID2_SOK;
}

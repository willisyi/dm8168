/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <stddef.h>
#include "ipcBitsInLink_priv.h"

#pragma DATA_ALIGN(gIpcBitsInLink_tskStack, 32)
#pragma DATA_SECTION(gIpcBitsInLink_tskStack, ".bss:taskStackSection")
UInt8
    gIpcBitsInLink_tskStack[IPC_BITS_IN_LINK_OBJ_MAX][IPC_LINK_TSK_STACK_SIZE];

IpcBitsInLink_Obj gIpcBitsInLink_obj[IPC_BITS_IN_LINK_OBJ_MAX];

static Int32 IpcBitsInLink_reconfigPrdObj(IpcBitsInLink_Obj * pObj,
                                          UInt period);

Void IpcBitsInLink_notifyCb(Utils_TskHndl * pTsk)
{
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) pTsk->appData;

    //IpcBitsInLink_reconfigPrdObj(pObj, IPC_BITS_IN_LINK_DONE_PERIOD_MS);
    Utils_tskSendCmd(pTsk, SYSTEM_CMD_NEW_DATA);
}

static IpcBitsInLink_initStats(IpcBitsInLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}

static Void IpcBitsInLink_prdCalloutFcn(UArg arg)
{
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->tskId, SYSTEM_CMD_NEW_DATA);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("IPCBITINLINK:[%s:%d]:"
                   "System_sendLinkCmd SYSTEM_IPC_CMD_RELEASE_FRAMES failed"
                   "errCode = %d", __FILE__, __LINE__, status);
        #endif
    }

}

static Int32 IpcBitsInLink_createPrdObj(IpcBitsInLink_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->prd.clkHandle == NULL);

    Clock_construct(&(pObj->prd.clkStruct),
                    IpcBitsInLink_prdCalloutFcn, 1, &clockParams);

    pObj->prd.clkHandle = Clock_handle(&pObj->prd.clkStruct);
    pObj->prd.clkStarted = FALSE;

    return IPC_BITS_IN_LINK_S_SUCCESS;

}

static Int32 IpcBitsInLink_deletePrdObj(IpcBitsInLink_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->prd.clkHandle);
    Clock_destruct(&(pObj->prd.clkStruct));
    pObj->prd.clkHandle = NULL;
    pObj->prd.clkStarted = FALSE;

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static Int32 IpcBitsInLink_startPrdObj(IpcBitsInLink_Obj * pObj, UInt period,
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

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static Int32 IpcBitsInLink_reconfigPrdObj(IpcBitsInLink_Obj * pObj, UInt period)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (TRUE == pObj->prd.clkStarted)
    {
        Clock_stop(pObj->prd.clkHandle);
    }
    Clock_setPeriod(pObj->prd.clkHandle, 0);
    Clock_setTimeout(pObj->prd.clkHandle, period);
    Clock_start(pObj->prd.clkHandle);
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_create(IpcBitsInLink_Obj * pObj,
                           IpcBitsInLinkRTOS_CreateParams * pPrm)
{
    Int32 status;
    System_LinkInQueParams *pInQueParams;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_IN   : Create in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

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
            IPCBITSINLINK_INFO_LOG(pObj->tskId,
                                   "InList not empty!!!"
                                   "Stale entry:[%p]",
                                   listElem);
        }
    }


    status = Utils_queCreate(&pObj->outBitBufQue,
                             SYSTEM_IPC_BITS_MAX_LIST_ELEM,
                             pObj->outBitBufQueMem,
                             UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == FVID2_SOK);

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    UTILS_assert(status == FVID2_SOK);

    IpcBitsInLink_initStats(pObj);

    IpcBitsInLink_createPrdObj(pObj);

    if (TRUE == pObj->createArgs.baseCreateParams.noNotifyMode)
    {
        if (FALSE == pObj->prd.clkStarted)
        {
            IpcBitsInLink_startPrdObj(pObj,
                                      IPC_BITS_IN_LINK_DONE_PERIOD_MS, FALSE);
        }
    }
    pObj->state = IPC_BITS_IN_LINK_STATE_ACTIVE;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_IN   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static
Int32 IpcBitsInLink_freeQueuedInputBufs(IpcBitsInLink_Obj * pObj)
{
    SystemIpcBits_ListElem *pListElem;
    Int32 status;
    UInt32 freeCount;

    freeCount = 0;
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        UTILS_assert(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
                     == IPC_BITBUF_STATE_OUTQUE);
        SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                     IPC_BITBUF_STATE_INQUE);
        status = ListMP_putTail(pObj->listMPInHndl, (ListMP_Elem *) pListElem);
        UTILS_assert(status == ListMP_S_SUCCESS);
        pObj->stats.droppedCount++;
        freeCount++;
    }

    if (freeCount && (pObj->createArgs.baseCreateParams.notifyPrevLink))
    {
        System_ipcSendNotify(pObj->createArgs.baseCreateParams.inQueParams.
                             prevLinkId);
    }

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static
Int32 IpcBitsInLink_stop(IpcBitsInLink_Obj * pObj)
{
    pObj->state = IPC_BITS_IN_LINK_STATE_INACTIVE;
    IpcBitsInLink_freeQueuedInputBufs(pObj);
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_delete(IpcBitsInLink_Obj * pObj)
{
    Int32 status;

    IPCBITSINLINK_INFO_LOG(pObj->tskId,
                           "RECV:%d\tFREE:%d,DROPPED:%d,AVGLATENCY:%d",
                           pObj->stats.recvCount,
                           pObj->stats.freeCount,
                           pObj->stats.droppedCount,
                  UTILS_DIV(pObj->stats.totalRoundTrip ,
                            pObj->stats.freeCount));
#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_IN   : Delete in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    status = System_ipcListMPClose(&pObj->listMPOutHndl, &pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    Utils_queDelete(&pObj->outBitBufQue);
    IpcBitsInLink_deletePrdObj(pObj);

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_BITS_IN   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static Int32 IpcBitsInLink_getBitBuf(IpcBitsInLink_Obj * pObj,
                                     SystemIpcBits_ListElem * pListElem,
                                     Bitstream_Buf ** pBitBufPtr)
{
    UTILS_assert(pListElem != NULL);
    /* No cache ops done since pListElem is allocated from non-cached memory */
    UTILS_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    if (FALSE == pListElem->bitBuf.flushFrame)
    {
        pListElem->bitBuf.addr = SharedRegion_getPtr(pListElem->srBufPtr);
    }
    else
    {
        IPCBITSINLINK_INFO_LOG(pObj->tskId,
                       "Flush Frame received for ch[%d]",
                       pListElem->bitBuf.channelNum);
    }
    *pBitBufPtr = &pListElem->bitBuf;
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_processBitBufs(IpcBitsInLink_Obj * pObj)
{
    Bitstream_Buf *pBitBuf;
    SystemIpcBits_ListElem *pListElem;
    UInt32 numBitBufs;
    Int32 status;
    UInt32 curTime;

    numBitBufs = 0;
    curTime = Utils_getCurTimeInMsec();
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        IpcBitsInLink_getBitBuf(pObj, pListElem, &pBitBuf);
        UTILS_assert(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
                     == IPC_BITBUF_STATE_OUTQUE);
        pBitBuf->reserved[0] = curTime;
        SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                     IPC_BITBUF_STATE_DEQUEUED);
        pObj->stats.recvCount++;
        status = Utils_quePut(&pObj->outBitBufQue, pBitBuf, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);

        numBitBufs++;
    }

#ifdef SYSTEM_DEBUG_IPC_RT
    Vps_printf(" %d: IPC_BITS_IN   : Recevived %d bitbufs !!!\n",
               Utils_getCurTimeInMsec(), numBitBufs);
#endif

    if (numBitBufs && pObj->createArgs.baseCreateParams.notifyNextLink)
    {
        UTILS_assert(pObj->createArgs.baseCreateParams.numOutQue == 1);
        System_sendLinkCmd(pObj->createArgs.baseCreateParams.outQueParams[0].
                           nextLink, SYSTEM_CMD_NEW_DATA);
    }

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_getFullBitBufs(Utils_TskHndl * pTsk, UInt16 queId,
                                   Bitstream_BufList * pBitBufList)
{
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) pTsk->appData;
    UInt32 idx;
    Int32 status;

    for (idx = 0; idx < VIDBITSTREAM_MAX_BITSTREAM_BUFS; idx++)
    {
        status =
            Utils_queGet(&pObj->outBitBufQue, (Ptr *) & pBitBufList->bufs[idx],
                         1, BIOS_NO_WAIT);
        if (status != FVID2_SOK)
            break;
    }

    pBitBufList->numBufs = idx;

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_putEmptyBitBufs(Utils_TskHndl * pTsk, UInt16 queId,
                                    Bitstream_BufList * pBitBufList)
{
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) pTsk->appData;
    UInt32 bufId;
    Bitstream_Buf *pBitBuf;
    Int32 status;
    SystemIpcBits_ListElem *pListElem = NULL;
    UInt32 curTime;

#ifdef SYSTEM_DEBUG_IPC_RT
    Vps_printf(" %d: IPC_BITS_IN   : Releasing %d bitbufs !!!\n",
               Utils_getCurTimeInMsec(), pBitBufList->numBufs);
#endif

    if (pBitBufList->numBufs)
    {
        UTILS_assert(pBitBufList->numBufs <= VIDBITSTREAM_MAX_BITSTREAM_BUFS);
        curTime = Utils_getCurTimeInMsec();
        pObj->stats.freeCount += pBitBufList->numBufs;
        for (bufId = 0; bufId < pBitBufList->numBufs; bufId++)
        {
            pBitBuf = pBitBufList->bufs[bufId];
            if (curTime > pBitBuf->reserved[0])
            {
                pObj->stats.totalRoundTrip += (curTime - pBitBuf->reserved[0]);
            }
            UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcBits_ListElem, bitBuf) ==
                                     0);
            pListElem = (SystemIpcBits_ListElem *) pBitBuf;
            if (FALSE == pListElem->bitBuf.flushFrame)
            {
                UTILS_assert(SharedRegion_getPtr(pListElem->srBufPtr) ==
                             pBitBuf->addr);
            }
            else
            {
                IPCBITSINLINK_INFO_LOG(pObj->tskId,
                               "Flush Frame freed for ch[%d]",
                               pListElem->bitBuf.channelNum);
            }
            SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                         IPC_BITBUF_STATE_INQUE);
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

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->inQueInfo, sizeof(*info));

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Void IpcBitsInLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = IpcBitsInLink_create(pObj, Utils_msgGetPrm(pMsg));

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

                IpcBitsInLink_processBitBufs(pObj);
                break;
            case SYSTEM_CMD_STOP:
                IpcBitsInLink_stop(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcBitsInLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_BITS_IN
    Vps_printf(" %d: IPC_BITS_IN   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 IpcBitsInLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcBitsInId;
    IpcBitsInLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcBits_ListElem, bitBuf) == 0);
    UTILS_COMPILETIME_ASSERT(offsetof(Bitstream_Buf, reserved) == 0);
    UTILS_COMPILETIME_ASSERT(sizeof(((Bitstream_Buf *) 0)->reserved) ==
                             sizeof(ListMP_Elem));

    for (ipcBitsInId = 0; ipcBitsInId < IPC_BITS_IN_LINK_OBJ_MAX; ipcBitsInId++)
    {
        pObj = &gIpcBitsInLink_obj[ipcBitsInId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_BITS_IN_0) + ipcBitsInId;

        pObj->state = IPC_BITS_IN_LINK_STATE_INACTIVE;
        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.linkGetFullBitBufs = IpcBitsInLink_getFullBitBufs;
        linkObj.linkPutEmptyBitBufs = IpcBitsInLink_putEmptyBitBufs;
        linkObj.getLinkInfo = IpcBitsInLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        UTILS_SNPRINTF(tskName, "IPC_BITS_IN%d", ipcBitsInId);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcBitsInLink_notifyCb);

        status = Utils_tskCreate(&pObj->tsk,
                                 IpcBitsInLink_tskMain,
                                 IPC_LINK_TSK_PRI,
                                 gIpcBitsInLink_tskStack[ipcBitsInId],
                                 IPC_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 IpcBitsInLink_deInit()
{
    UInt32 ipcBitsInId;

    for (ipcBitsInId = 0; ipcBitsInId < IPC_BITS_IN_LINK_OBJ_MAX; ipcBitsInId++)
    {
        Utils_tskDelete(&gIpcBitsInLink_obj[ipcBitsInId].tsk);
    }
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <stddef.h>
#include "ipcBitsInLink_priv.h"
#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/syslink/utils/Cache.h>

IpcBitsInLink_Obj gIpcBitsInLink_obj[IPC_BITS_IN_LINK_OBJ_MAX];

static
Int32 IpcBitsInLink_getFullBitBufs(IpcBitsInLink_Obj * pObj,
                                   Bitstream_BufList * pBitBufList);

static
Int32 IpcBitsInLink_putEmptyBitBufs(IpcBitsInLink_Obj * pObj,
                                    Bitstream_BufList * pBitBufList);

Void IpcBitsInLink_notifyCb(OSA_TskHndl * pTsk)
{
    OSA_tskSendMsg(pTsk, NULL, SYSTEM_CMD_NEW_DATA, NULL, 0);
}

Void *IpcBitsInLink_periodicTaskFxn(Void * prm)
{
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) prm;
    Int32 status;

    while (FALSE == pObj->prd.exitThread)
    {
        OSA_waitMsecs(IPC_BITS_IN_PROCESS_PERIOD_MS);
        if (pObj->prd.numPendingCmd < IPC_BITS_IN_MAX_PENDING_NEWDATA_CMDS)
        {
            pObj->prd.numPendingCmd++;
            status = System_sendLinkCmd(pObj->tskId, SYSTEM_CMD_NEW_DATA);
        }
        else
        {
            UInt32 curTime = OSA_getCurTimeInMsec();

            OSA_printf("IPC_BITSINLINK:!WARNING!.Commands not being processed by link."
                       "TimeSinceLastAlloc:%d,TimeSinceLastFree:%d",
                       (curTime - pObj->stats.lastGetBufTime),
                       (curTime - pObj->stats.lastFreeBufTime));
        }
    }
    return NULL;
}

static Void IpcBitsInLink_initStats(IpcBitsInLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}

static
Int32 IpcBitsInLink_createPrdObj(IpcBitsInLink_Obj * pObj)
{
    pObj->prd.numPendingCmd = 0;
    pObj->prd.exitThread = FALSE;
    OSA_thrCreate(&pObj->prd.thrHandle,
                  IpcBitsInLink_periodicTaskFxn,
                  IPC_LINK_TSK_PRI, IPC_LINK_TSK_STACK_SIZE, pObj);
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static
Int32 IpcBitsInLink_deletePrdObj(IpcBitsInLink_Obj * pObj)
{
    pObj->prd.exitThread = TRUE;
    OSA_thrDelete(&pObj->prd.thrHandle);
    pObj->prd.numPendingCmd = 0;
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_create(IpcBitsInLink_Obj * pObj,
                           IpcBitsInLinkHLOS_CreateParams * pPrm)
{
    Int32 status;
    System_LinkInQueParams *pInQueParams;

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_IN   : Create in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif
    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    OSA_printf(" %d: IPC_BITS_IN   : ListMPOpen start !!!\n",
               OSA_getCurTimeInMsec());
    status =
        System_ipcListMPOpen(pObj->createArgs.baseCreateParams.inQueParams.
                             prevLinkId, &pObj->listMPOutHndl,
                             &pObj->listMPInHndl);
    OSA_assert(status == OSA_SOK);
    OSA_assert(pObj->listMPInHndl != NULL);

    OSA_printf(" %d: IPC_BITS_IN   : ListMPOpen done !!!\n",
               OSA_getCurTimeInMsec());

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

    status = OSA_queCreate(&pObj->outBitBufQue, SYSTEM_IPC_BITS_MAX_LIST_ELEM);
    OSA_assert(status == OSA_SOK);

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    OSA_assert(status == OSA_SOK);
    OSA_printf(" %d: IPC_BITS_IN   : System_linkGetInfo done !!!\n",
               OSA_getCurTimeInMsec());

    IpcBitsInLink_initStats(pObj);
    if (pObj->createArgs.baseCreateParams.noNotifyMode)
    {
        IpcBitsInLink_createPrdObj(pObj);
    }

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_IN   : Create Done !!!\n",
               OSA_getCurTimeInMsec());
#endif

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_delete(IpcBitsInLink_Obj * pObj)
{
    Int32 status;

    if (pObj->createArgs.baseCreateParams.noNotifyMode)
    {
        IpcBitsInLink_deletePrdObj(pObj);
    }
    IPCBITSINLINK_INFO_LOG(pObj->tskId,
                           "RECV:%d\tFREE:%d,DROPPED:%d,AVGLATENCY:%d,AVG_APP_CB_TIME:%d",
                           pObj->stats.recvCount,
                           pObj->stats.freeCount,
                           pObj->stats.droppedCount,
                    OSA_DIV(pObj->stats.totalRoundTrip ,
                            pObj->stats.freeCount),
                    OSA_DIV(pObj->stats.totalAppCallbackTime,
                            pObj->stats.numFreeBufCalls));
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_IN   : Delete in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif

    status = System_ipcListMPClose(&pObj->listMPOutHndl, &pObj->listMPInHndl);
    OSA_assert(status == OSA_SOK);

    OSA_queDelete(&pObj->outBitBufQue);

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_IN   : Delete Done !!!\n",
               OSA_getCurTimeInMsec());
#endif

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static Int32 IpcBitsInLink_getBitBuf(IpcBitsInLink_Obj * pObj,
                                     SystemIpcBits_ListElem * pListElem,
                                     Bitstream_Buf ** pBitBufPtr)
{
    OSA_assert(pListElem != NULL);
    /* No cache ops done since pListElem is allocated from non-cached memory */
    OSA_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
               FALSE);
    pListElem->bitBuf.addr = SharedRegion_getPtr(pListElem->srBufPtr);
    if (pListElem->bitBuf.fillLength)
    {
        Cache_inv(pListElem->bitBuf.addr,
                  pListElem->bitBuf.fillLength, Cache_Type_ALL, TRUE);
    }
    if(pListElem->bitBuf.mvDataFilledSize)
	{
        Cache_inv(pListElem->bitBuf.addr + pListElem->bitBuf.mvDataOffset,
                  pListElem->bitBuf.mvDataFilledSize, Cache_Type_ALL, TRUE);	
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
    curTime = OSA_getCurTimeInMsec();
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        IpcBitsInLink_getBitBuf(pObj, pListElem, &pBitBuf);
        OSA_assert(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
                   == IPC_BITBUF_STATE_OUTQUE);
        SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                     IPC_BITBUF_STATE_DEQUEUED);
        pBitBuf->reserved[0] = curTime;
        pObj->stats.recvCount++;
        status =
            OSA_quePut(&pObj->outBitBufQue, (Int32) pBitBuf, OSA_TIMEOUT_NONE);
        OSA_assert(status == OSA_SOK);

        numBitBufs++;
    }

#ifdef SYSTEM_DEBUG_IPC_RT
    OSA_printf(" %d: IPC_BITS_IN   : Recevived %d bitbufs !!!\n",
               OSA_getCurTimeInMsec(), numBitBufs);
#endif

    if (numBitBufs)
    {
        if (pObj->createArgs.cbFxn)
        {
            pObj->createArgs.cbFxn(pObj->createArgs.cbCtx);
        }
    }

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static
Int32 IpcBitsInLink_getFullBitBufs(IpcBitsInLink_Obj * pObj,
                                   Bitstream_BufList * pBitBufList)
{
    UInt32 idx;
    Int32 status;
    UInt32 curTime;

    curTime = OSA_getCurTimeInMsec();
    for (idx = 0; idx < VIDBITSTREAM_MAX_BITSTREAM_BUFS; idx++)
    {
        status =
            OSA_queGet(&pObj->outBitBufQue,
                       (Int32 *) (&pBitBufList->bufs[idx]), OSA_TIMEOUT_NONE);
        if (status != OSA_SOK)
            break;
        pBitBufList->bufs[idx]->reserved[1] = curTime;
    }

    pBitBufList->numBufs = idx;
    pObj->stats.numGetBufCalls++;
    pObj->stats.lastGetBufTime = curTime;
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

static
Int32 IpcBitsInLink_putEmptyBitBufs(IpcBitsInLink_Obj * pObj,
                                    Bitstream_BufList * pBitBufList)
{
    UInt32 bufId;
    Bitstream_Buf *pBitBuf;
    Int32 status;
    SystemIpcBits_ListElem *pListElem = NULL;
    UInt32 curTime;

#ifdef SYSTEM_DEBUG_IPC_RT
    OSA_printf(" %d: IPC_BITS_IN   : Releasing %d bitbufs !!!\n",
               OSA_getCurTimeInMsec(), pBitBufList->numBufs);
#endif
    curTime = OSA_getCurTimeInMsec();
    if (pBitBufList->numBufs)
    {
        OSA_assert(pBitBufList->numBufs <= VIDBITSTREAM_MAX_BITSTREAM_BUFS);
        pObj->stats.freeCount += pBitBufList->numBufs;
        for (bufId = 0; bufId < pBitBufList->numBufs; bufId++)
        {
            pBitBuf = pBitBufList->bufs[bufId];
            if (curTime > pBitBuf->reserved[0])
            {
                pObj->stats.totalRoundTrip += (curTime - pBitBuf->reserved[0]);
            }
            if (curTime > pBitBuf->reserved[1])
            {
                pObj->stats.totalAppCallbackTime += (curTime - pBitBuf->reserved[1]);
            }
            OSA_COMPILETIME_ASSERT(offsetof(SystemIpcBits_ListElem, bitBuf) ==
                                   0);
            pListElem = (SystemIpcBits_ListElem *) pBitBuf;
            OSA_assert(SharedRegion_getPtr(pListElem->srBufPtr) ==
                       pBitBuf->addr);
            SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                         IPC_BITBUF_STATE_INQUE);
            status =
                ListMP_putTail(pObj->listMPInHndl, (ListMP_Elem *) pListElem);
            OSA_assert(status == ListMP_S_SUCCESS);
        }

        if (pObj->createArgs.baseCreateParams.notifyPrevLink)
        {
            System_ipcSendNotify(pObj->createArgs.baseCreateParams.inQueParams.
                                 prevLinkId);
        }
    }
    pObj->stats.lastFreeBufTime = curTime;
    pObj->stats.numFreeBufCalls++;
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_getLinkInfo(OSA_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->inQueInfo, sizeof(*info));

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int IpcBitsInLink_tskMain(struct OSA_TskHndl * pTsk, OSA_MsgHndl * pMsg,
                          Uint32 curState)
{
    UInt32 cmd = OSA_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int status = IPC_BITS_IN_LINK_S_SUCCESS;
    IpcBitsInLink_Obj *pObj = (IpcBitsInLink_Obj *) pTsk->appData;

    OSA_printf("%s:Entered", __func__);
    if (cmd != SYSTEM_CMD_CREATE)
    {
        OSA_tskAckOrFreeMsg(pMsg, OSA_EFAIL);
        return status;
    }

    status = IpcBitsInLink_create(pObj, OSA_msgGetPrm(pMsg));

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
            case SYSTEM_CMD_NEW_DATA:
                OSA_tskAckOrFreeMsg(pMsg, status);
                if (pObj->createArgs.baseCreateParams.noNotifyMode)
                {
                    OSA_assert(pObj->prd.numPendingCmd > 0);
                    pObj->prd.numPendingCmd--;
                }
                IpcBitsInLink_processBitBufs(pObj);
                break;

            default:
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcBitsInLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_BITS_IN
    OSA_printf(" %d: IPC_BITS_IN   : Delete Done !!!\n",
               OSA_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        OSA_tskAckOrFreeMsg(pMsg, status);

    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcBitsInId;
    IpcBitsInLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    OSA_COMPILETIME_ASSERT(offsetof(SystemIpcBits_ListElem, bitBuf) == 0);
    OSA_COMPILETIME_ASSERT(offsetof(Bitstream_Buf, reserved) == 0);
    OSA_COMPILETIME_ASSERT(sizeof(((Bitstream_Buf *) 0)->reserved) ==
                           sizeof(ListMP_Elem));

    for (ipcBitsInId = 0; ipcBitsInId < IPC_BITS_IN_LINK_OBJ_MAX; ipcBitsInId++)
    {
        pObj = &gIpcBitsInLink_obj[ipcBitsInId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_BITS_IN_0) + ipcBitsInId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.getLinkInfo = IpcBitsInLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        OSA_SNPRINTF(tskName, "IPC_BITS_IN%d", ipcBitsInId);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcBitsInLink_notifyCb);

        status = OSA_tskCreate(&pObj->tsk,
                               IpcBitsInLink_tskMain,
                               IPC_LINK_TSK_PRI,
                               IPC_LINK_TSK_STACK_SIZE, 0, pObj);
        OSA_assert(status == OSA_SOK);
    }

    return status;
}

Int32 IpcBitsInLink_deInit()
{
    UInt32 ipcBitsInId;

    for (ipcBitsInId = 0; ipcBitsInId < IPC_BITS_IN_LINK_OBJ_MAX; ipcBitsInId++)
    {
        OSA_tskDelete(&gIpcBitsInLink_obj[ipcBitsInId].tsk);
    }
    return IPC_BITS_IN_LINK_S_SUCCESS;
}

Int32 IpcBitsInLink_getFullVideoBitStreamBufs(UInt32 linkId,
                                              Bitstream_BufList *bufList)
{
    OSA_TskHndl * pTsk;
    IpcBitsInLink_Obj * pObj;
    Int status;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0 + IPC_BITS_IN_LINK_OBJ_MAX))))
    {
        return IPC_BITS_IN_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    bufList->numBufs = 0;
    status = IpcBitsInLink_getFullBitBufs(pObj,bufList);
    return status;
}


Int32 IpcBitsInLink_putEmptyVideoBitStreamBufs(UInt32 linkId,
                                               Bitstream_BufList *bufList)
{
    OSA_TskHndl * pTsk;
    IpcBitsInLink_Obj * pObj;
    Int status;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0 + IPC_BITS_IN_LINK_OBJ_MAX))))
    {
        return IPC_BITS_IN_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    status = IpcBitsInLink_putEmptyBitBufs(pObj,bufList);
    return status;
}

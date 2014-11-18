/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"
#include "system_priv_ipc.h"

#pragma DATA_ALIGN(gSystem_tskMsgQStack, 32)
#pragma DATA_SECTION(gSystem_tskMsgQStack, ".bss:taskStackSection")
UInt8 gSystem_tskMsgQStack[SYSTEM_MSGQ_TSK_STACK_SIZE];

Int32 System_ipcMsgQHeapCreate()
{
    UInt32 procId;
    Int32 status;
    UInt32 retryCount;
    IHeap_Handle srHeap;
    HeapMemMP_Params heapMemParams;

    procId = System_getSelfProcId();
    if (procId != SYSTEM_PROC_INVALID)
    {
        if (procId == SYSTEM_IPC_MSGQ_OWNER_PROC_ID)
        {
            /* create heap */
            Vps_printf(" %d: SYSTEM: Creating MsgQ Heap [%s] ...\n",
                       Utils_getCurTimeInMsec(), SYSTEM_IPC_MSGQ_HEAP_NAME);

            srHeap = SharedRegion_getHeap(SYSTEM_IPC_SR_NON_CACHED_DEFAULT);
            UTILS_assert(srHeap != NULL);

            gSystem_ipcObj.msgQHeapBaseAddr =
                Memory_alloc(srHeap, SYSTEM_IPC_MSGQ_HEAP_SIZE, 0, NULL);

            UTILS_assert(gSystem_ipcObj.msgQHeapBaseAddr != NULL);

            HeapMemMP_Params_init(&heapMemParams);

            heapMemParams.name = SYSTEM_IPC_MSGQ_HEAP_NAME;
            heapMemParams.sharedAddr = gSystem_ipcObj.msgQHeapBaseAddr;
            heapMemParams.sharedBufSize = SYSTEM_IPC_MSGQ_HEAP_SIZE;

            gSystem_ipcObj.msgQHeapHndl = HeapMemMP_create(&heapMemParams);

            UTILS_assert(gSystem_ipcObj.msgQHeapHndl != NULL);
        }
        else
        {
            /* open heap */
            retryCount = 10;
            while (retryCount)
            {
                Vps_printf(" %d: SYSTEM: Opening MsgQ Heap [%s] ...\n",
                           Utils_getCurTimeInMsec(), SYSTEM_IPC_MSGQ_HEAP_NAME);

                status =
                    HeapMemMP_open(SYSTEM_IPC_MSGQ_HEAP_NAME,
                                   &gSystem_ipcObj.msgQHeapHndl);
                if (status == HeapMemMP_E_NOTFOUND)
                {
                    /* Sleep for a while before trying again. */
                    Task_sleep(1000);
                }
                else if (status == HeapMemMP_S_SUCCESS)
                {
                    break;
                }
                retryCount--;
                if (retryCount <= 0)
                    UTILS_assert(0);
            }
        }

        /* Register this heap with MessageQ */
        MessageQ_registerHeap((IHeap_Handle) gSystem_ipcObj.msgQHeapHndl,
                              SYSTEM_IPC_MSGQ_HEAP);
    }
    return FVID2_SOK;
}

Int32 System_ipcMsgQHeapDelete()
{
    Int32 status;
    UInt32 procId;
    IHeap_Handle srHeap;

    Vps_printf(" %d: SYSTEM: IPC MsgQ Heap Close in progress !!!\n",
               Utils_getCurTimeInMsec());

    procId = System_getSelfProcId();

    MessageQ_unregisterHeap(SYSTEM_IPC_MSGQ_HEAP);

    if (procId == SYSTEM_IPC_MSGQ_OWNER_PROC_ID)
    {
        /* delete heap */
        status = HeapMemMP_delete(&gSystem_ipcObj.msgQHeapHndl);
        UTILS_assert(status == FVID2_SOK);

        srHeap = SharedRegion_getHeap(SYSTEM_IPC_SR_NON_CACHED_DEFAULT);
        UTILS_assert(srHeap != NULL);

        Memory_free(srHeap,
                    gSystem_ipcObj.msgQHeapBaseAddr, SYSTEM_IPC_MSGQ_HEAP_SIZE);
    }
    else
    {
        /* close heap */
        status = HeapMemMP_close(&gSystem_ipcObj.msgQHeapHndl);
        UTILS_assert(status == FVID2_SOK);
    }

    Vps_printf(" %d: SYSTEM: IPC MsgQ Heap Close DONE !!!\n", Utils_getCurTimeInMsec());

    return FVID2_SOK;
}

Int32 System_ipcMsgQTskCreate()
{
    Semaphore_Params semParams;
    Task_Params tskParams;

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;

    gSystem_ipcObj.msgQLock = Semaphore_create(1u, &semParams, NULL);
    UTILS_assert(gSystem_ipcObj.msgQLock != NULL);

    /*
     * Create task
     */
    Task_Params_init(&tskParams);

    tskParams.priority = SYSTEM_MSGQ_TSK_PRI_HIGH;
    tskParams.stack = gSystem_tskMsgQStack;
    tskParams.stackSize = sizeof(gSystem_tskMsgQStack);

    gSystem_ipcObj.msgQTask = Task_create(System_ipcMsgQTaskMain,
                                          &tskParams, NULL);

    UTILS_assert(gSystem_ipcObj.msgQTask != NULL);

    {
        Int32 status;

        status = Utils_prfLoadRegister(gSystem_ipcObj.msgQTask, "SYSTEM_MSGQ");
        UTILS_assert(status==FVID2_SOK);
    }
    return FVID2_SOK;
}

Int32 System_ipcMsgQTskDelete()
{
    UInt32 sleepTime = 8;                                  /* in OS ticks */

    Vps_printf(" %d: SYSTEM: IPC MsgQ Task Delete in progress !!!\n",
               Utils_getCurTimeInMsec());

    /* unblock task */
    MessageQ_unblock(gSystem_ipcObj.selfMsgQ);

    /* wait for command to be received and task to be exited */
    Task_sleep(1);

    while (Task_Mode_TERMINATED != Task_getMode(gSystem_ipcObj.msgQTask))
    {

        Task_sleep(sleepTime);

        sleepTime >>= 1;

        if (sleepTime == 0)
        {
            UTILS_assert(0);
        }
    }

    Utils_prfLoadUnRegister(gSystem_ipcObj.msgQTask);

    Task_delete(&gSystem_ipcObj.msgQTask);

    Semaphore_delete(&gSystem_ipcObj.msgQLock);

    Vps_printf(" %d: SYSTEM: IPC MsgQ Task Delete DONE !!!\n",
               Utils_getCurTimeInMsec());

    return FVID2_SOK;
}

Int32 System_ipcMsgQOpen(UInt32 procId)
{
    UInt32 retryCount;
    char msgQName[64];
    char ackMsgQName[64];
    Int32 status;

    if (gSystem_ipcObj.isOpenRemoteProcMsgQ[procId])
        return 0;

    System_ipcGetMsgQName(procId, msgQName, ackMsgQName);

    retryCount = 10;
    while (retryCount)
    {
        Vps_printf(" %d: SYSTEM: Opening MsgQ [%s] ...\n",
                   Utils_getCurTimeInMsec(), msgQName);

        status =
            MessageQ_open(msgQName, &gSystem_ipcObj.remoteProcMsgQ[procId]);
        if (status == MessageQ_E_NOTFOUND)
            Task_sleep(1000);
        else if (status == MessageQ_S_SUCCESS)
            break;

        retryCount--;
        if (retryCount <= 0)
            UTILS_assert(0);
    }

    /* no need to open ack msgq, since ack msgq id is embeeded in the
     * received message */

    gSystem_ipcObj.isOpenRemoteProcMsgQ[procId] = TRUE;

    return 0;
}

Int32 System_ipcMsgQCreate()
{
    UInt32 procId;
    MessageQ_Params msgQParams;
    char msgQName[64];
    char ackMsgQName[64];

    procId = System_getSelfProcId();

    System_ipcGetMsgQName(procId, msgQName, ackMsgQName);

    /* create MsgQ */
    MessageQ_Params_init(&msgQParams);

    Vps_printf(" %d: SYSTEM: Creating MsgQ [%s] ...\n",
               Utils_getCurTimeInMsec(), msgQName);

    gSystem_ipcObj.selfMsgQ = MessageQ_create(msgQName, &msgQParams);
    UTILS_assert(gSystem_ipcObj.selfMsgQ != NULL);

    MessageQ_Params_init(&msgQParams);

    Vps_printf(" %d: SYSTEM: Creating MsgQ [%s] ...\n",
               Utils_getCurTimeInMsec(), ackMsgQName);

    gSystem_ipcObj.selfAckMsgQ = MessageQ_create(ackMsgQName, &msgQParams);
    UTILS_assert(gSystem_ipcObj.selfMsgQ != NULL);

    for (procId = 0; procId < SYSTEM_PROC_MAX; procId++)
    {
        gSystem_ipcObj.isOpenRemoteProcMsgQ[procId] = FALSE;
    }

    return FVID2_SOK;
}

Int32 System_ipcMsgQDelete()
{
    UInt32 procId;
    Int32 status;

    Vps_printf(" %d: SYSTEM: IPC MsgQ Close in progress !!!\n",
               Utils_getCurTimeInMsec());

    for (procId = 0; procId < SYSTEM_PROC_MAX; procId++)
    {
        if (procId == System_getSelfProcId())
        {
            /* delete MsgQ */

            status = MessageQ_delete(&gSystem_ipcObj.selfMsgQ);
            UTILS_assert(status == 0);

            status = MessageQ_delete(&gSystem_ipcObj.selfAckMsgQ);
            UTILS_assert(status == 0);
        }
        else
        {
            if (gSystem_ipcObj.isOpenRemoteProcMsgQ[procId])
            {
                status = MessageQ_close(&gSystem_ipcObj.remoteProcMsgQ[procId]);
                UTILS_assert(status == 0);

                /* no need to close ack msgq */
            }
        }
    }

    Vps_printf(" %d: SYSTEM: IPC MsgQ Close DONE !!!\n", Utils_getCurTimeInMsec());

    return FVID2_SOK;
}

Int32 System_ipcMsgQInit()
{
    System_ipcMsgQHeapCreate();
    System_ipcMsgQCreate();
    System_ipcMsgQTskCreate();

    return FVID2_SOK;
}

Int32 System_ipcMsgQDeInit()
{
    Vps_printf(" %d: SYSTEM: IPC MsgQ de-init in progress !!!\n",
               Utils_getCurTimeInMsec());

    System_ipcMsgQTskDelete();
    System_ipcMsgQDelete();
    System_ipcMsgQHeapDelete();

    Vps_printf(" %d: SYSTEM: IPC MsgQ de-init DONE !!!\n", Utils_getCurTimeInMsec());

    return FVID2_SOK;
}

Void System_ipcMsgQTaskMain(UArg arg0, UArg arg1)
{
    UInt32 prmSize;
    SystemIpcMsgQ_Msg *pMsgCommon;
    Void *pPrm;
    Int32 status;

    while (1)
    {
        status =
            MessageQ_get(gSystem_ipcObj.selfMsgQ, (MessageQ_Msg *) & pMsgCommon,
                         BIOS_WAIT_FOREVER);

        if (status == MessageQ_E_UNBLOCKED)
            break;

        if (status != MessageQ_S_SUCCESS)
        {
            Vps_printf(" %d: MSGQ: MsgQ get failed !!!\n", Utils_getCurTimeInMsec());
            continue;
        }

#if 0
        Vps_printf
            (" %d: MSGQ: Received command [0x%04x] (prmSize = %d) for [%s][%02d] (waitAck=%d)\n",
             Utils_getCurTimeInMsec(), pMsgCommon->cmd, pMsgCommon->prmSize,
             MultiProc_getName(SYSTEM_GET_PROC_ID(pMsgCommon->linkId)),
             SYSTEM_GET_LINK_ID(pMsgCommon->linkId), pMsgCommon->waitAck);
#endif

        prmSize = pMsgCommon->prmSize;

        pPrm = SYSTEM_IPC_MSGQ_MSG_PAYLOAD_PTR(pMsgCommon);

        if (pMsgCommon->cmd == SYSTEM_CMD_GET_INFO)
        {
            UTILS_assert(prmSize == sizeof(System_LinkInfo));

            pMsgCommon->status =
                System_linkGetInfo_local(pMsgCommon->linkId, pPrm);
        }
        else
        {
            pMsgCommon->status = System_linkControl_local(pMsgCommon->linkId,
                                                          pMsgCommon->cmd,
                                                          pPrm,
                                                          prmSize,
                                                          pMsgCommon->waitAck);
        }
        if (pMsgCommon->waitAck)
        {
            MessageQ_QueueId replyMsgQ;

            replyMsgQ = MessageQ_getReplyQueue(pMsgCommon);

#if 0
        Vps_printf
            (" %d: MSGQ: Acked command [0x%04x] (prmSize = %d) for [%s][%02d] (waitAck=%d)\n",
             Utils_getCurTimeInMsec(), pMsgCommon->cmd, pMsgCommon->prmSize,
             MultiProc_getName(SYSTEM_GET_PROC_ID(pMsgCommon->linkId)),
             SYSTEM_GET_LINK_ID(pMsgCommon->linkId), pMsgCommon->waitAck);
#endif

            status = MessageQ_put(replyMsgQ, (MessageQ_Msg) pMsgCommon);

            if (status != MessageQ_S_SUCCESS)
            {
                Vps_printf(" %d: MSGQ: MsgQ Ack put failed !!!\n",
                           Utils_getCurTimeInMsec());
                MessageQ_free((MessageQ_Msg) pMsgCommon);
            }
        }
        else
        {
            MessageQ_free((MessageQ_Msg) pMsgCommon);
        }
    }
}

Int32 System_ipcMsgQSendMsg(UInt32 linkId, UInt32 cmd, Void * pPrm,
                            UInt32 prmSize, Bool waitAck)
{
    Int32 status = FVID2_SOK;
    SystemIpcMsgQ_Msg *pMsgCommon;
    UInt32 procId;
    Void *pMsgPrm;

    UTILS_assert(prmSize <= SYSTEM_IPC_MSGQ_MSG_SIZE_MAX);
    procId = SYSTEM_GET_PROC_ID(linkId);

    UTILS_assert(procId < SYSTEM_PROC_MAX);

    pMsgCommon = (SystemIpcMsgQ_Msg *) MessageQ_alloc(SYSTEM_IPC_MSGQ_HEAP,
                                                      sizeof(*pMsgCommon) +
                                                      prmSize);

    UTILS_assert(pMsgCommon != NULL);

    if (prmSize && pPrm)
    {
        pMsgPrm = SYSTEM_IPC_MSGQ_MSG_PAYLOAD_PTR(pMsgCommon);
        memcpy(pMsgPrm, pPrm, prmSize);
    }

    pMsgCommon->linkId = linkId;
    pMsgCommon->cmd = cmd;
    pMsgCommon->prmSize = prmSize;
    pMsgCommon->waitAck = waitAck;
    pMsgCommon->status = FVID2_SOK;

    MessageQ_setReplyQueue(gSystem_ipcObj.selfAckMsgQ,
                           (MessageQ_Msg) pMsgCommon);
    MessageQ_setMsgId(pMsgCommon, linkId);

    Semaphore_pend(gSystem_ipcObj.msgQLock, BIOS_WAIT_FOREVER);

    /* open message Q is not opened already */
    System_ipcMsgQOpen(procId);

    status =
        MessageQ_put(gSystem_ipcObj.remoteProcMsgQ[procId],
                     (MessageQ_Msg) pMsgCommon);
    if (status != MessageQ_S_SUCCESS)
    {
        Vps_printf(" %d: MSGQ: MsgQ put for [%s] failed !!!\n",
                   Utils_getCurTimeInMsec(), MultiProc_getName(procId));
        MessageQ_free((MessageQ_Msg) pMsgCommon);
        Semaphore_post(gSystem_ipcObj.msgQLock);
        return status;
    }

    if (waitAck)
    {
        SystemIpcMsgQ_Msg *pAckMsg;

        status =
            MessageQ_get(gSystem_ipcObj.selfAckMsgQ, (MessageQ_Msg *) & pAckMsg,
                         BIOS_WAIT_FOREVER);
        if (status != MessageQ_S_SUCCESS)
        {
            Vps_printf(" %d: MSGQ: MsgQ Ack get from [%s] failed !!!\n",
                       Utils_getCurTimeInMsec(), MultiProc_getName(procId));
            MessageQ_free((MessageQ_Msg) pMsgCommon);
            Semaphore_post(gSystem_ipcObj.msgQLock);
            return status;
        }

        if (prmSize && pPrm)
        {
            pMsgPrm = SYSTEM_IPC_MSGQ_MSG_PAYLOAD_PTR(pAckMsg);
            memcpy(pPrm, pMsgPrm, prmSize);
        }

        status = pAckMsg->status;

        MessageQ_free((MessageQ_Msg) pAckMsg);
    }

    Semaphore_post(gSystem_ipcObj.msgQLock);

    return status;
}

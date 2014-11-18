/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "ipcInM3Link_priv.h"

#pragma DATA_ALIGN(gIpcInM3Link_tskStack, 32)
#pragma DATA_SECTION(gIpcInM3Link_tskStack, ".bss:taskStackSection")
UInt8 gIpcInM3Link_tskStack[IPC_IN_M3_LINK_OBJ_MAX][IPC_LINK_TSK_STACK_SIZE];

IpcInM3Link_Obj gIpcInM3Link_obj[IPC_IN_M3_LINK_OBJ_MAX];

static Int32 IpcInM3Link_reconfigPrdObj(IpcInM3Link_Obj * pObj, UInt period);

Void IpcInM3Link_notifyCb(Utils_TskHndl * pTsk)
{
    IpcInM3Link_Obj *pObj = (IpcInM3Link_Obj *) pTsk->appData;

    //IpcInM3Link_reconfigPrdObj(pObj, IPC_M3IN_LINK_DONE_PERIOD_MS);
    Utils_tskSendCmd(pTsk, SYSTEM_CMD_NEW_DATA);
}

static Void IpcInM3Link_prdCalloutFcn(UArg arg)
{
    IpcInM3Link_Obj *pObj = (IpcInM3Link_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->tskId, SYSTEM_CMD_NEW_DATA);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("IPCINM3LINK:[%s:%d]:"
                   "System_sendLinkCmd SYSTEM_CMD_NEW_DATA failed"
                   "errCode = %d", __FILE__, __LINE__, status);
        #endif
    }

}

static Int32 IpcInM3Link_createPrdObj(IpcInM3Link_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->prd.clkHandle == NULL);

    Clock_construct(&(pObj->prd.clkStruct),
                    IpcInM3Link_prdCalloutFcn, 1, &clockParams);

    pObj->prd.clkHandle = Clock_handle(&pObj->prd.clkStruct);
    pObj->prd.clkStarted = FALSE;

    return IPC_M3IN_LINK_S_SUCCESS;

}

static Int32 IpcInM3Link_deletePrdObj(IpcInM3Link_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->prd.clkHandle);
    Clock_destruct(&(pObj->prd.clkStruct));
    pObj->prd.clkHandle = NULL;
    pObj->prd.clkStarted = FALSE;

    return IPC_M3IN_LINK_S_SUCCESS;
}

static Int32 IpcInM3Link_startPrdObj(IpcInM3Link_Obj * pObj, UInt period,
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

    return IPC_M3IN_LINK_S_SUCCESS;
}

static Int32 IpcInM3Link_reconfigPrdObj(IpcInM3Link_Obj * pObj, UInt period)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (TRUE == pObj->prd.clkStarted)
    {
        Clock_stop(pObj->prd.clkHandle);
    }
    Clock_setPeriod(pObj->prd.clkHandle, 0);
    Clock_setTimeout(pObj->prd.clkHandle, period);
    Clock_start(pObj->prd.clkHandle);
    return IPC_M3IN_LINK_S_SUCCESS;
}

Int32 IpcInM3Link_create(IpcInM3Link_Obj * pObj, IpcLink_CreateParams * pPrm)
{
    Int32 status;
    System_LinkInQueParams *pInQueParams;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_IN_M3   : Create in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    status = System_ipcListMPOpen(pObj->createArgs.inQueParams.prevLinkId,
                                  &pObj->listMPOutHndl, &pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    status = Utils_queCreate(&pObj->outFrameQue,
                             SYSTEM_IPC_M3_MAX_LIST_ELEM,
                             pObj->outFrameQueMem, UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == FVID2_SOK);

    pInQueParams = &pObj->createArgs.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    UTILS_assert(status == FVID2_SOK);

    IpcInM3Link_createPrdObj(pObj);

    if (TRUE == pObj->createArgs.noNotifyMode)
    {
        if (FALSE == pObj->prd.clkStarted)
        {
            IpcInM3Link_startPrdObj(pObj,
                                      IPC_M3IN_LINK_DONE_PERIOD_MS, FALSE);
        }
    }

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_IN_M3   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 IpcInM3Link_delete(IpcInM3Link_Obj * pObj)
{
    Int32 status;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_IN_M3   : Delete in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    status = System_ipcListMPClose(&pObj->listMPOutHndl, &pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    Utils_queDelete(&pObj->outFrameQue);
    IpcInM3Link_deletePrdObj(pObj);

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_IN_M3   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 IpcInM3Link_processFrames(IpcInM3Link_Obj * pObj)
{
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo;
    SystemIpcM3_ListElem *pListElem;
    UInt32 numFrames;
    Int32 status;

    numFrames = 0;
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        pFrame = pListElem->pFrame;
        UTILS_assert(pFrame != NULL);

        pFrameInfo = (System_FrameInfo *) pFrame->appData;

        UTILS_assert(pFrameInfo != NULL);
        pFrameInfo->pOrgListMPElem = pListElem;

        status = Utils_quePut(&pObj->outFrameQue, pFrame, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);

        numFrames++;
    }

#ifdef SYSTEM_DEBUG_IPC_RT
    Vps_printf(" %d: IPC_IN_M3   : Recevived %d frames !!!\n", Utils_getCurTimeInMsec(),
               numFrames);
#endif

    if (numFrames && pObj->createArgs.notifyNextLink)
    {
        UTILS_assert(pObj->createArgs.numOutQue == 1);
        System_sendLinkCmd(pObj->createArgs.outQueParams[0].nextLink,
                           SYSTEM_CMD_NEW_DATA);
    }

    return FVID2_SOK;
}

Int32 IpcInM3Link_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                FVID2_FrameList * pFrameList)
{
    IpcInM3Link_Obj *pObj = (IpcInM3Link_Obj *) pTsk->appData;
    UInt32 idx;
    Int32 status;

    for (idx = 0; idx < FVID2_MAX_FVID_FRAME_PTR; idx++)
    {
        status =
            Utils_queGet(&pObj->outFrameQue, (Ptr *) & pFrameList->frames[idx],
                         1, BIOS_NO_WAIT);
        if (status != FVID2_SOK)
            break;
    }

    pFrameList->numFrames = idx;

    return FVID2_SOK;
}

Int32 IpcInM3Link_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                 FVID2_FrameList * pFrameList)
{
    IpcInM3Link_Obj *pObj = (IpcInM3Link_Obj *) pTsk->appData;
    UInt32 frameId;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo;
    Int32 status;

#ifdef SYSTEM_DEBUG_IPC_RT
    Vps_printf(" %d: IPC_IN_M3   : Releasing %d frames !!!\n", Utils_getCurTimeInMsec(),
               pFrameList->numFrames);
#endif

    if (pFrameList->numFrames)
    {
        for (frameId = 0; frameId < pFrameList->numFrames; frameId++)
        {
            pFrame = pFrameList->frames[frameId];

            pFrameInfo = (System_FrameInfo *) pFrame->appData;

            UTILS_assert(pFrameInfo != NULL);
            UTILS_assert(pFrameInfo->pOrgListMPElem != NULL);

            /* no need to do cache ops since this is inter-M3 list */
            status =
                ListMP_putTail(pObj->listMPInHndl,
                               (ListMP_Elem *) pFrameInfo->pOrgListMPElem);
            UTILS_assert(status == ListMP_S_SUCCESS);
        }

        if (pObj->createArgs.notifyPrevLink)
        {
            System_ipcSendNotify(pObj->createArgs.inQueParams.prevLinkId);
        }
    }

    return FVID2_SOK;
}

Int32 IpcInM3Link_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcInM3Link_Obj *pObj = (IpcInM3Link_Obj *) pTsk->appData;

    memcpy(info, &pObj->inQueInfo, sizeof(*info));

    return FVID2_SOK;
}

Void IpcInM3Link_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    IpcInM3Link_Obj *pObj = (IpcInM3Link_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = IpcInM3Link_create(pObj, Utils_msgGetPrm(pMsg));

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

                IpcInM3Link_processFrames(pObj);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcInM3Link_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_IN_M3
    Vps_printf(" %d: IPC_IN_M3   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 IpcInM3Link_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcInM3Id;
    IpcInM3Link_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    for (ipcInM3Id = 0; ipcInM3Id < IPC_IN_M3_LINK_OBJ_MAX; ipcInM3Id++)
    {
        pObj = &gIpcInM3Link_obj[ipcInM3Id];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId, SYSTEM_LINK_ID_IPC_IN_M3_0) + ipcInM3Id;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = IpcInM3Link_getFullFrames;
        linkObj.linkPutEmptyFrames = IpcInM3Link_putEmptyFrames;
        linkObj.getLinkInfo = IpcInM3Link_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "IPC_IN_M3%d", ipcInM3Id);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcInM3Link_notifyCb);

        status = Utils_tskCreate(&pObj->tsk,
                                 IpcInM3Link_tskMain,
                                 IPC_LINK_TSK_PRI,
                                 gIpcInM3Link_tskStack[ipcInM3Id],
                                 IPC_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 IpcInM3Link_deInit()
{
    UInt32 ipcInM3Id;

    for (ipcInM3Id = 0; ipcInM3Id < IPC_IN_M3_LINK_OBJ_MAX; ipcInM3Id++)
    {
        Utils_tskDelete(&gIpcInM3Link_obj[ipcInM3Id].tsk);
    }
    return FVID2_SOK;
}

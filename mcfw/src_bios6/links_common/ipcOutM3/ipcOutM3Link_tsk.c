/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "ipcOutM3Link_priv.h"

#pragma DATA_ALIGN(gIpcOutM3Link_tskStack, 32)
#pragma DATA_SECTION(gIpcOutM3Link_tskStack, ".bss:taskStackSection")
UInt8 gIpcOutM3Link_tskStack[IPC_OUT_M3_LINK_OBJ_MAX][IPC_LINK_TSK_STACK_SIZE];

IpcOutM3Link_Obj gIpcOutM3Link_obj[IPC_OUT_M3_LINK_OBJ_MAX];

static Int32 IpcOutM3Link_reconfigPrdObj(IpcOutM3Link_Obj * pObj, UInt period);


Int32 IpcOutM3Link_SetFrameRate(IpcOutM3Link_Obj * pObj, IpcOutM3Link_ChFpsParams * params)
{
    Int32 status = FVID2_SOK;
    IpcOutM3Link_ChObj *pChObj;

    UTILS_assert(params->chId < (IPC_OUTM3_LINK_MAX_CH));
    pChObj = &pObj->chObj[params->chId];

    pChObj->frameSkipCtx.firstTime = TRUE;
    pChObj->frameSkipCtx.inputFrameRate = params->inputFrameRate;
    pChObj->frameSkipCtx.outputFrameRate = params->outputFrameRate;

    return (status);
}


Int32 IpcOutM3Link_resetStatistics(IpcOutM3Link_Obj *pObj)
{
    UInt32 chId;
    IpcOutM3Link_ChObj *pChObj;

    for (chId = 0; chId < pObj->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameUserSkipCount = 0;
        pChObj->inFrameProcessCount = 0;
    }

    pObj->totalFrameCount = 0;

    pObj->statsStartTime = Utils_getCurTimeInMsec();

    return 0;
}


Int32 IpcOutM3Link_printStatistics (IpcOutM3Link_Obj *pObj, Bool resetAfterPrint)
{
    UInt32 chId;
    IpcOutM3Link_ChObj *pChObj;
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    Vps_printf( " \n"
            " *** IpcOutM3 Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " Total Fields Processed : %d \n"
            " Total Fields FPS       : %d FPS\n"
            " \n"
            " \n"
            " CH  | In Recv In Process In Skip\n"
            " Num | FPS     FPS        FPS    \n"
            " --------------------------------\n",
            elaspedTime,
                    pObj->totalFrameCount
            , pObj->totalFrameCount / (elaspedTime)
                    );

    for (chId = 0; chId < pObj->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        Vps_printf( " %3d | %7d %10d %7d\n",
            chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameProcessCount/elaspedTime,
            pChObj->inFrameUserSkipCount/elaspedTime
             );
    }

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        IpcOutM3Link_resetStatistics(pObj);
    }
    return FVID2_SOK;
}

Void IpcOutM3Link_notifyCb(Utils_TskHndl * pTsk)
{
    IpcOutM3Link_Obj *pObj = (IpcOutM3Link_Obj *) pTsk->appData;

    //IpcOutM3Link_reconfigPrdObj(pObj, IPC_M3OUT_LINK_DONE_PERIOD_MS);
    Utils_tskSendCmd(pTsk, SYSTEM_IPC_CMD_RELEASE_FRAMES);
}

static Void IpcOutM3Link_prdCalloutFcn(UArg arg)
{
    IpcOutM3Link_Obj *pObj = (IpcOutM3Link_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->tskId, SYSTEM_IPC_CMD_RELEASE_FRAMES);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("IPCOUTM3LINK:[%s:%d]:"
                   "System_sendLinkCmd SYSTEM_IPC_CMD_RELEASE_FRAMES failed"
                   "errCode = %d", __FILE__, __LINE__, status);
        #endif
    }

}

static Int32 IpcOutM3Link_createPrdObj(IpcOutM3Link_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->prd.clkHandle == NULL);

    Clock_construct(&(pObj->prd.clkStruct),
                    IpcOutM3Link_prdCalloutFcn, 1, &clockParams);

    pObj->prd.clkHandle = Clock_handle(&pObj->prd.clkStruct);
    pObj->prd.clkStarted = FALSE;

    return IPC_M3OUT_LINK_S_SUCCESS;

}

static Int32 IpcOutM3Link_deletePrdObj(IpcOutM3Link_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->prd.clkHandle);
    Clock_destruct(&(pObj->prd.clkStruct));
    pObj->prd.clkHandle = NULL;
    pObj->prd.clkStarted = FALSE;

    return IPC_M3OUT_LINK_S_SUCCESS;
}

static Int32 IpcOutM3Link_startPrdObj(IpcOutM3Link_Obj * pObj, UInt period,
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

    return IPC_M3OUT_LINK_S_SUCCESS;
}

static Int32 IpcOutM3Link_reconfigPrdObj(IpcOutM3Link_Obj * pObj, UInt period)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (TRUE == pObj->prd.clkStarted)
    {
        Clock_stop(pObj->prd.clkHandle);
    }
    Clock_setPeriod(pObj->prd.clkHandle, 0);
    Clock_setTimeout(pObj->prd.clkHandle, period);
    Clock_start(pObj->prd.clkHandle);
    return IPC_M3OUT_LINK_S_SUCCESS;
}

Int32 IpcOutM3Link_create(IpcOutM3Link_Obj * pObj, IpcLink_CreateParams * pPrm)
{
    Int32 status, elemId;
    System_LinkInQueParams *pInQueParams;
    System_LinkQueInfo *pInQueInfo;
    IpcOutM3Link_ChObj *pChObj;
    UInt32 chId;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_OUT_M3   : Create in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif
    pObj->totalFrameCount = 0;
    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    status = System_ipcListMPReset(pObj->listMPOutHndl, pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    status = Utils_queCreate(&pObj->listElemQue,
                             SYSTEM_IPC_M3_MAX_LIST_ELEM,
                             pObj->listElemQueMem, UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == FVID2_SOK);

    for (elemId = 0; elemId < SYSTEM_IPC_M3_MAX_LIST_ELEM; elemId++)
    {
        status =
            Utils_quePut(&pObj->listElemQue, pObj->listElem[elemId],
                         BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    pInQueParams = &pObj->createArgs.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    UTILS_assert(status == FVID2_SOK);

    pInQueInfo =
        &pObj->inQueInfo.queInfo[
                pObj->createArgs.inQueParams.prevLinkQueId
                ];

    UTILS_assert(pInQueInfo->numCh <= IPC_OUTM3_LINK_MAX_CH);

    /* set number of input / output channels */
    pObj->numCh = pInQueInfo->numCh;



    IpcOutM3Link_createPrdObj(pObj);
    IpcOutM3Link_resetStatistics(pObj);
    for (chId = 0; chId < pObj->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->frameSkipCtx.firstTime = TRUE;
        pChObj->frameSkipCtx.inputFrameRate = pObj->createArgs.inputFrameRate;
        pChObj->frameSkipCtx.outputFrameRate = pObj->createArgs.outputFrameRate;
    }
#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_OUT_M3   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 IpcOutM3Link_delete(IpcOutM3Link_Obj * pObj)
{
#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_OUT_M3   : Delete in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    Utils_queDelete(&pObj->listElemQue);
    IpcOutM3Link_deletePrdObj(pObj);

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_OUT_M3   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 IpcOutM3Link_processFrames(IpcOutM3Link_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    FVID2_FrameList frameList;
    FVID2_Frame *pFrame;
    SystemIpcM3_ListElem *pListElem;
    Int32 status;
    Int32 frameId;
    IpcOutM3Link_ChObj *pChObj;

    pInQueParams = &pObj->createArgs.inQueParams;

    System_getLinksFullFrames(pInQueParams->prevLinkId,
                              pInQueParams->prevLinkQueId, &frameList);

    pObj->freeFrameList.numFrames = 0;

    if (frameList.numFrames)
    {
#ifdef SYSTEM_DEBUG_IPC_RT
        Vps_printf(" %d: IPC_OUT_M3   : Received %d frames !!!\n",
                   Utils_getCurTimeInMsec(), frameList.numFrames);
#endif

        pObj->totalFrameCount += frameList.numFrames;
        for (frameId = 0; frameId < frameList.numFrames; frameId++)
        {
            Bool          doFrameDrop;
            pFrame = frameList.frames[frameId];
            pChObj = &pObj->chObj[pFrame->channelNum];
            pChObj->inFrameRecvCount++;
            doFrameDrop = Utils_doSkipFrame(&(pChObj->frameSkipCtx));

            /* frame skipped due to user setting */
            if(doFrameDrop)
            {
                pChObj->inFrameUserSkipCount++;
                pObj->freeFrameList.frames[pObj->freeFrameList.numFrames] =
                    pFrame;
                pObj->freeFrameList.numFrames++;
                UTILS_assert(pObj->freeFrameList.numFrames <=
                             FVID2_MAX_FVID_FRAME_PTR);
                continue;;
            }

            status =
                Utils_queGet(&pObj->listElemQue, (Ptr *) & pListElem, 1,
                             BIOS_NO_WAIT);
            if (status != FVID2_SOK)
            {
                /* normally this condition should not happen, if it happens
                 * return the frame back to its generator */
#if 0
                Vps_printf(" IPC_OUT: Dropping frame\n");
#endif

                pObj->freeFrameList.frames[pObj->freeFrameList.numFrames] =
                    pFrame;
                pObj->freeFrameList.numFrames++;
                UTILS_assert(pObj->freeFrameList.numFrames <=
                             FVID2_MAX_FVID_FRAME_PTR);
                pChObj->inFrameUserSkipCount++;
                continue;
            }

            pListElem->pFrame = pFrame;

            /* no need to do cache ops since this is inter-M3 list */
            status = ListMP_putTail(pObj->listMPOutHndl, &pListElem->listElem);
            UTILS_assert(status == ListMP_S_SUCCESS);
            pChObj->inFrameProcessCount++;
        }

        if (pObj->freeFrameList.numFrames)
        {
            System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                       pInQueParams->prevLinkQueId,
                                       &pObj->freeFrameList);
        }

        if (pObj->createArgs.notifyNextLink)
        {
            UTILS_assert(pObj->createArgs.numOutQue == 1);
            System_ipcSendNotify(pObj->createArgs.outQueParams[0].nextLink);
        }

        if (TRUE == pObj->createArgs.noNotifyMode)
        {
            if (FALSE == pObj->prd.clkStarted)
            {
                IpcOutM3Link_startPrdObj(pObj,
                                          IPC_M3OUT_LINK_DONE_PERIOD_MS, FALSE);
            }
        }
    }

    return FVID2_SOK;
}

Int32 IpcOutM3Link_releaseFrames(IpcOutM3Link_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    SystemIpcM3_ListElem *pListElem = NULL;
    Int32 status;

    pInQueParams = &pObj->createArgs.inQueParams;

    do
    {
        pObj->freeFrameList.numFrames = 0;

        while (pObj->freeFrameList.numFrames < FVID2_MAX_FVID_FRAME_PTR)
        {
            pListElem = ListMP_getHead(pObj->listMPInHndl);
            if (pListElem == NULL)
                break;

            pObj->freeFrameList.frames[pObj->freeFrameList.numFrames] =
                pListElem->pFrame;
            pObj->freeFrameList.numFrames++;

            UTILS_assert(pObj->freeFrameList.numFrames <=
                         FVID2_MAX_FVID_FRAME_PTR);

            /* release ListElem back to queue */
            status = Utils_quePut(&pObj->listElemQue, pListElem, BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
        }

#ifdef SYSTEM_DEBUG_IPC_RT
        Vps_printf(" %d: IPC_OUT_M3   : Releasing %d frames !!!\n",
                   Utils_getCurTimeInMsec(), pObj->freeFrameList.numFrames);
#endif

        if (pObj->freeFrameList.numFrames)
        {

            System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                       pInQueParams->prevLinkQueId,
                                       &pObj->freeFrameList);
        }
    } while (pListElem != NULL);

    return FVID2_SOK;
}

Int32 IpcOutM3Link_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcOutM3Link_Obj *pObj = (IpcOutM3Link_Obj *) pTsk->appData;

    memcpy(info, &pObj->inQueInfo, sizeof(*info));

    return FVID2_SOK;
}

Void IpcOutM3Link_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    IpcOutM3Link_Obj *pObj = (IpcOutM3Link_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = IpcOutM3Link_create(pObj, Utils_msgGetPrm(pMsg));

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

                IpcOutM3Link_processFrames(pObj);
                IpcOutM3Link_releaseFrames(pObj);
                break;

            case IPCOUTM3_LINK_CMD_SET_FRAME_RATE:
                {
                    IpcOutM3Link_ChFpsParams *params;

                    params = (IpcOutM3Link_ChFpsParams *) Utils_msgGetPrm(pMsg);
                    IpcOutM3Link_SetFrameRate(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case IPCOUTM3_LINK_CMD_PRINT_STATISTICS:
                IpcOutM3Link_printStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;


            case SYSTEM_IPC_CMD_RELEASE_FRAMES:
                Utils_tskAckOrFreeMsg(pMsg, status);

#ifdef SYSTEM_DEBUG_IPC_RT
                Vps_printf(" %d: IPC_OUT_M3   : Received Notify !!!\n",
                           Utils_getCurTimeInMsec());
#endif

                IpcOutM3Link_releaseFrames(pObj);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcOutM3Link_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_OUT_M3
    Vps_printf(" %d: IPC_OUT_M3   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 IpcOutM3Link_allocListElem(IpcOutM3Link_Obj * pObj)
{
    UInt32 shAddr;
    UInt32 elemId;

    shAddr = System_ipcListMPAllocListElemMem(SYSTEM_IPC_SR_M3_LIST_MP,
                                              sizeof(SystemIpcM3_ListElem) *
                                              SYSTEM_IPC_M3_MAX_LIST_ELEM);

    for (elemId = 0; elemId < SYSTEM_IPC_M3_MAX_LIST_ELEM; elemId++)
    {
        pObj->listElem[elemId] =
            (SystemIpcM3_ListElem *) (shAddr +
                                      elemId * sizeof(SystemIpcM3_ListElem));
    }

    return FVID2_SOK;
}

Int32 IpcOutM3Link_freeListElem(IpcOutM3Link_Obj * pObj)
{
    System_ipcListMPFreeListElemMem(SYSTEM_IPC_SR_M3_LIST_MP,
                                    (UInt32) pObj->listElem[0],
                                    sizeof(SystemIpcM3_ListElem) *
                                    SYSTEM_IPC_M3_MAX_LIST_ELEM);

    return FVID2_SOK;
}

Int32 IpcOutM3Link_initListMP(IpcOutM3Link_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPCreate(SYSTEM_IPC_SR_M3_LIST_MP,
                                    pObj->tskId,
                                    &pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    IpcOutM3Link_allocListElem(pObj);

    return status;
}

Int32 IpcOutM3Link_deInitListMP(IpcOutM3Link_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPDelete(&pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    IpcOutM3Link_freeListElem(pObj);

    return status;
}

Int32 IpcOutM3Link_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcOutM3Id;
    IpcOutM3Link_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    for (ipcOutM3Id = 0; ipcOutM3Id < IPC_OUT_M3_LINK_OBJ_MAX; ipcOutM3Id++)
    {
        pObj = &gIpcOutM3Link_obj[ipcOutM3Id];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_OUT_M3_0) + ipcOutM3Id;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.getLinkInfo = IpcOutM3Link_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "IPC_OUT_M3%d", ipcOutM3Id);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcOutM3Link_notifyCb);

        IpcOutM3Link_initListMP(pObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 IpcOutM3Link_tskMain,
                                 IPC_LINK_TSK_PRI,
                                 gIpcOutM3Link_tskStack[ipcOutM3Id],
                                 IPC_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 IpcOutM3Link_deInit()
{
    UInt32 ipcOutM3Id;
    IpcOutM3Link_Obj *pObj;

    for (ipcOutM3Id = 0; ipcOutM3Id < IPC_OUT_M3_LINK_OBJ_MAX; ipcOutM3Id++)
    {
        pObj = &gIpcOutM3Link_obj[ipcOutM3Id];

        Utils_tskDelete(&pObj->tsk);

        IpcOutM3Link_deInitListMP(pObj);
    }
    return FVID2_SOK;
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "mergeLink_priv.h"

#pragma DATA_ALIGN(gMergeLink_tskStack, 32)
#pragma DATA_SECTION(gMergeLink_tskStack, ".bss:taskStackSection")
UInt8 gMergeLink_tskStack[MERGE_LINK_OBJ_MAX][MERGE_LINK_TSK_STACK_SIZE];

MergeLink_Obj gMergeLink_obj[MERGE_LINK_OBJ_MAX];

static Void MergeLink_drvInitStats(MergeLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}

Int32 MergeLink_drvCreate(MergeLink_Obj * pObj, MergeLink_CreateParams * pPrm)
{
    Int32 status;
    Semaphore_Params semParams;
    System_LinkQueInfo *pInQueInfo, *pOutQueInfo;
    UInt32 inQue, numOutCh, chId;

    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    UTILS_assert(pPrm->numInQue <= MERGE_LINK_MAX_IN_QUE);

    status = Utils_bufCreate(&pObj->outFrameQue, FALSE, FALSE);
    UTILS_assert(status == FVID2_SOK);

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;

    pObj->lock = Semaphore_create(1u, &semParams, NULL);

    UTILS_assert(pObj->lock != NULL);

    numOutCh = 0;

    pOutQueInfo = &pObj->info.queInfo[0];

    for (inQue = 0; inQue < pPrm->numInQue; inQue++)
    {
        status =
            System_linkGetInfo(pPrm->inQueParams[inQue].prevLinkId,
                               &pObj->inTskInfo[inQue]);
        UTILS_assert(status == FVID2_SOK);
        UTILS_assert(pPrm->inQueParams[inQue].prevLinkQueId <
                     pObj->inTskInfo[inQue].numQue);

        pInQueInfo = &pObj->inTskInfo[inQue].queInfo
            [pPrm->inQueParams[inQue].prevLinkQueId];

        UTILS_assert(pInQueInfo->numCh <= SYSTEM_MAX_CH_PER_OUT_QUE);
        for (chId = 0; chId < pInQueInfo->numCh; chId++)
        {
            memcpy(&pOutQueInfo->chInfo[numOutCh],
                   &pInQueInfo->chInfo[chId],
                   sizeof(pOutQueInfo->chInfo[numOutCh]));

            pObj->inQueChNumMap[inQue][chId] = numOutCh;
            pObj->outQueChToInQueMap[numOutCh] = inQue;
            pObj->outQueChMap[numOutCh] = chId;

            numOutCh++;
            UTILS_assert(numOutCh <= SYSTEM_MAX_CH_PER_OUT_QUE);
        }

        pObj->inQueMaxCh[inQue] = pInQueInfo->numCh;
    }

    pObj->info.numQue = 1;
    pOutQueInfo->numCh = numOutCh;
    MergeLink_drvInitStats(pObj);

#ifdef SYSTEM_DEBUG_MERGE
    Vps_printf(" %d: MERGE   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 MergeLink_drvDelete(MergeLink_Obj * pObj)
{
    Int32 status;

    Semaphore_delete(&pObj->lock);

    status = Utils_bufDelete(&pObj->outFrameQue);
    UTILS_assert(status == FVID2_SOK);

#ifdef SYSTEM_DEBUG_MERGE
    Vps_printf(" %d: MERGE   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 MergeLink_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    MergeLink_Obj *pObj = (MergeLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 MergeLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList)
{
    Int32 status;
    MergeLink_Obj *pObj = (MergeLink_Obj *) pTsk->appData;

    status =  Utils_bufGetFull(&pObj->outFrameQue, pFrameList, BIOS_NO_WAIT);

    if (status == 0)
    {
        pObj->stats.forwardCount += pFrameList->numFrames;
    }
    return status;
}

Int32 MergeLink_drvProcessFrames(MergeLink_Obj * pObj)
{
    MergeLink_CreateParams *pCreateArgs;
    UInt32 inQue, frameId;
    Int32 status;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo;
    Bool newDataAvailable;

    newDataAvailable = FALSE;

    pCreateArgs = &pObj->createArgs;

    for (inQue = 0; inQue < pCreateArgs->numInQue; inQue++)
    {
        System_getLinksFullFrames(pCreateArgs->inQueParams[inQue].prevLinkId,
                                  pCreateArgs->inQueParams[inQue].prevLinkQueId,
                                  &pObj->inFrameList);

        if (pObj->inFrameList.numFrames)
        {
            pObj->stats.recvCount[inQue] += pObj->inFrameList.numFrames;
            for (frameId = 0; frameId < pObj->inFrameList.numFrames; frameId++)
            {
                /* remap channel number */
                pFrame = pObj->inFrameList.frames[frameId];

                UTILS_assert(pFrame->channelNum < pObj->inQueMaxCh[inQue]);

                pFrame->channelNum =
                    pObj->inQueChNumMap[inQue][pFrame->channelNum];

                pFrameInfo = (System_FrameInfo *) pFrame->appData;

                UTILS_assert(pFrameInfo != NULL);

                pFrameInfo->mergeChannelNum = pFrame->channelNum;
            }

            status = Utils_bufPutFull(&pObj->outFrameQue, &pObj->inFrameList);
            UTILS_assert(status == FVID2_SOK);

            newDataAvailable = TRUE;
        }
    }

    if (pCreateArgs->notifyNextLink && newDataAvailable)
    {
        System_sendLinkCmd(pCreateArgs->outQueParams.nextLink,
                           SYSTEM_CMD_NEW_DATA);
    }

    return FVID2_SOK;
}

Int32 MergeLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                               FVID2_FrameList * pFrameList)
{
    MergeLink_Obj *pObj = (MergeLink_Obj *) pTsk->appData;
    UInt32 frameId, inQue;
    MergeLink_CreateParams *pCreateArgs;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo;

    Semaphore_pend(pObj->lock, BIOS_WAIT_FOREVER);

    pCreateArgs = &pObj->createArgs;

    for (inQue = 0; inQue < pCreateArgs->numInQue; inQue++)
    {
        pObj->freeFrameList[inQue].numFrames = 0;
    }

    for (frameId = 0; frameId < pFrameList->numFrames; frameId++)
    {
        pFrame = pFrameList->frames[frameId];

        UTILS_assert(pFrame != NULL);

        pFrameInfo = (System_FrameInfo *) pFrame->appData;

        UTILS_assert(pFrameInfo != NULL);

        pFrame->channelNum = pFrameInfo->mergeChannelNum;

        inQue = pObj->outQueChToInQueMap[pFrame->channelNum];

        pFrame->channelNum = pObj->outQueChMap[pFrame->channelNum];

        pFrameInfo->mergeChannelNum = pFrame->channelNum;

        UTILS_assert(inQue < pCreateArgs->numInQue);

        pObj->freeFrameList[inQue].frames
            [pObj->freeFrameList[inQue].numFrames] = pFrame;

        pObj->freeFrameList[inQue].numFrames++;
        pObj->stats.releaseCount[inQue]++;
    }

    for (inQue = 0; inQue < pCreateArgs->numInQue; inQue++)
    {
        if (pObj->freeFrameList[inQue].numFrames)
        {
            System_putLinksEmptyFrames(pCreateArgs->inQueParams[inQue].
                                       prevLinkId,
                                       pCreateArgs->inQueParams[inQue].
                                       prevLinkQueId,
                                       &pObj->freeFrameList[inQue]);
        }
    }

    Semaphore_post(pObj->lock);

    return FVID2_SOK;
}

static
Int32 MergeLink_drvGetInLinkChInfo(MergeLink_Obj *pObj,
                                   MergeLink_InLinkChInfo *inChInfo)
{
    Int inQue;
    Int32 status = FVID2_SOK;

    UTILS_assert(inChInfo != NULL);
    inChInfo->numCh = 0;
    inChInfo->startChNum = 0;
    for (inQue = 0; inQue < pObj->createArgs.numInQue; inQue++)
    {
        if (inChInfo->inLinkID == pObj->createArgs.inQueParams[inQue].prevLinkId)
        {
            break;
        }
    }
    if (inQue < pObj->createArgs.numInQue)
    {
        inChInfo->numCh      =  pObj->inTskInfo[inQue].queInfo[pObj->createArgs.inQueParams[inQue].prevLinkQueId].numCh;
        inChInfo->startChNum =  pObj->inQueChNumMap[inQue][0];
    }
    else
    {
        Vps_printf("%d:MERGELINK[%x]:Input link ID passed to MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO is invalid[%x]",
                   Utils_getCurTimeInMsec(),
                   pObj->tskId,
                   inChInfo->inLinkID);
        status = FVID2_EFAIL;
    }
    return status;
}


Void MergeLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    MergeLink_Obj *pObj = (MergeLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = MergeLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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

                MergeLink_drvProcessFrames(pObj);
                break;
            case MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO:
                status = MergeLink_drvGetInLinkChInfo(pObj,Utils_msgGetPrm(pMsg));
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    MergeLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 MergeLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 mergeId;
    MergeLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    for (mergeId = 0; mergeId < MERGE_LINK_OBJ_MAX; mergeId++)
    {
        pObj = &gMergeLink_obj[mergeId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId, SYSTEM_LINK_ID_MERGE_0) + mergeId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = MergeLink_getFullFrames;
        linkObj.linkPutEmptyFrames = MergeLink_putEmptyFrames;
        linkObj.getLinkInfo = MergeLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "MERGE%d", mergeId);

        status = Utils_tskCreate(&pObj->tsk,
                                 MergeLink_tskMain,
                                 MERGE_LINK_TSK_PRI,
                                 gMergeLink_tskStack[mergeId],
                                 MERGE_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 MergeLink_deInit()
{
    UInt32 mergeId;

    for (mergeId = 0; mergeId < MERGE_LINK_OBJ_MAX; mergeId++)
    {
        Utils_tskDelete(&gMergeLink_obj[mergeId].tsk);
    }
    return FVID2_SOK;
}

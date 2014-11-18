/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "dupLink_priv.h"

#pragma DATA_ALIGN(gDupLink_tskStack, 32)
#pragma DATA_SECTION(gDupLink_tskStack, ".bss:taskStackSection")
UInt8 gDupLink_tskStack[DUP_LINK_OBJ_MAX][DUP_LINK_TSK_STACK_SIZE];

DupLink_Obj gDupLink_obj[DUP_LINK_OBJ_MAX];

static Void DupLink_drvInitStats(DupLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}


Int32 DupLink_drvCreate(DupLink_Obj * pObj, DupLink_CreateParams * pPrm)
{
    UInt32 outId, frameId;
    Int32 status;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo;
    Semaphore_Params semParams;

    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));
    UTILS_assert(pObj->createArgs.numOutQue <= DUP_LINK_MAX_OUT_QUE);

    pObj->putFrameCount = 0;
    pObj->getFrameCount = 0;

    memset(pObj->frames, 0, sizeof(pObj->frames));
    memset(pObj->frameInfo, 0, sizeof(pObj->frameInfo));

    status = System_linkGetInfo(pPrm->inQueParams.prevLinkId, &pObj->inTskInfo);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pPrm->inQueParams.prevLinkQueId < pObj->inTskInfo.numQue);

    pObj->info.numQue = pObj->createArgs.numOutQue;

    memcpy(&pObj->info.queInfo[0],
           &pObj->inTskInfo.queInfo[pPrm->inQueParams.prevLinkQueId],
           sizeof(pObj->inTskInfo.queInfo[0]));

    for (outId = 1; outId < pObj->info.numQue; outId++)
    {
        memcpy(&pObj->info.queInfo[outId],
               &pObj->info.queInfo[0], sizeof(pObj->info.queInfo[0]));
    }

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;

    pObj->lock = Semaphore_create(1u, &semParams, NULL);

    for (outId = 0; outId < DUP_LINK_MAX_OUT_QUE; outId++)
    {
        status = Utils_bufCreate(&pObj->outFrameQue[outId], FALSE, FALSE);
        UTILS_assert(status == FVID2_SOK);

        for (frameId = 0; frameId < DUP_LINK_MAX_FRAMES_PER_OUT_QUE; frameId++)
        {
            pFrame =
                &pObj->frames[DUP_LINK_MAX_FRAMES_PER_OUT_QUE * outId +
                              frameId];
            pFrameInfo =
                &pObj->frameInfo[DUP_LINK_MAX_FRAMES_PER_OUT_QUE * outId +
                                 frameId];

            pFrame->appData = pFrameInfo;

            status = Utils_bufPutEmptyFrame(&pObj->outFrameQue[outId], pFrame);
            UTILS_assert(status == FVID2_SOK);
        }
    }
    DupLink_drvInitStats(pObj);
#ifdef SYSTEM_DEBUG_DUP
    Vps_printf(" %d: DUP   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 DupLink_drvDelete(DupLink_Obj * pObj)
{
    UInt32 outId;
    Int32 status;

    for (outId = 0; outId < DUP_LINK_MAX_OUT_QUE; outId++)
    {
        status = Utils_bufDelete(&pObj->outFrameQue[outId]);
        UTILS_assert(status == FVID2_SOK);
    }

    Semaphore_delete(&pObj->lock);

#ifdef SYSTEM_DEBUG_DUP
    Vps_printf(" %d: DUP   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 DupLink_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    DupLink_Obj *pObj = (DupLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 DupLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                            FVID2_FrameList * pFrameList)
{
    Int32 status;
    DupLink_Obj *pObj = (DupLink_Obj *) pTsk->appData;

    UTILS_assert(queId < DUP_LINK_MAX_OUT_QUE);
    UTILS_assert(queId < pObj->createArgs.numOutQue);

    status = Utils_bufGetFull(&pObj->outFrameQue[queId], pFrameList,
                              BIOS_NO_WAIT);
    if (status == 0)
    {
        pObj->stats.forwardCount[queId]+= pFrameList->numFrames;
    }
    return status;
}

Int32 DupLink_drvProcessFrames(DupLink_Obj * pObj)
{
    UInt32 frameId, outId;
    FVID2_Frame *pFrame, *pOrgFrame;
    System_FrameInfo *pFrameInfo, *pOrgFrameInfo;
    Int32 status;
    DupLink_CreateParams *pCreateArgs;

    pCreateArgs = &pObj->createArgs;

    System_getLinksFullFrames(pCreateArgs->inQueParams.prevLinkId,
                              pCreateArgs->inQueParams.prevLinkQueId,
                              &pObj->inFrameList);

    if (pObj->inFrameList.numFrames)
    {
        pObj->getFrameCount += pObj->inFrameList.numFrames;
        pObj->stats.recvCount += pObj->inFrameList.numFrames;

        for (outId = 0; outId < pCreateArgs->numOutQue; outId++)
        {
            pObj->outFrameList[outId].numFrames = 0;
        }

        for (frameId = 0; frameId < pObj->inFrameList.numFrames; frameId++)
        {
            pOrgFrame = pObj->inFrameList.frames[frameId];
            if (pOrgFrame == NULL)
                continue;

            pOrgFrameInfo = (System_FrameInfo *) pOrgFrame->appData;
            UTILS_assert(pOrgFrameInfo != NULL);

            pOrgFrameInfo->dupCount = pCreateArgs->numOutQue;

            for (outId = 0; outId < pCreateArgs->numOutQue; outId++)
            {
                status = Utils_bufGetEmptyFrame(&pObj->outFrameQue[outId],
                                                &pFrame, BIOS_NO_WAIT);
                UTILS_assert(status == FVID2_SOK);
                UTILS_assert(pFrame != NULL);

                pFrameInfo = (System_FrameInfo *) pFrame->appData;
                UTILS_assert(pFrameInfo != NULL);

                memcpy(pFrame, pOrgFrame, sizeof(*pOrgFrame));
                memcpy(pFrameInfo, pOrgFrameInfo, sizeof(*pOrgFrameInfo));

                pFrame->appData = pFrameInfo;

                pFrameInfo->pDupOrgFrame = pOrgFrame;

                pObj->outFrameList[outId].frames[pObj->outFrameList[outId].
                                                 numFrames] = pFrame;

                pObj->outFrameList[outId].numFrames++;
            }
        }

        for (outId = 0; outId < pCreateArgs->numOutQue; outId++)
        {
            status =
                Utils_bufPutFull(&pObj->outFrameQue[outId],
                                 &pObj->outFrameList[outId]);
            UTILS_assert(status == FVID2_SOK);

            if (pCreateArgs->notifyNextLink)
            {
                System_sendLinkCmd(pCreateArgs->outQueParams[outId].nextLink,
                                   SYSTEM_CMD_NEW_DATA);
            }
        }
    }

    return FVID2_SOK;
}

Int32 DupLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList)
{
    DupLink_Obj *pObj = (DupLink_Obj *) pTsk->appData;
    FVID2_FrameList freeFrameList;
    UInt32 frameId;
    FVID2_Frame *pFrame, *pOrgFrame;
    System_FrameInfo *pFrameInfo, *pOrgFrameInfo;
    Int32 status;

    UTILS_assert(queId < DUP_LINK_MAX_OUT_QUE);
    UTILS_assert(queId < pObj->createArgs.numOutQue);

    freeFrameList.numFrames = 0;

    Semaphore_pend(pObj->lock, BIOS_WAIT_FOREVER);

    pObj->stats.releaseCount[queId] += pFrameList->numFrames;
    for (frameId = 0; frameId < pFrameList->numFrames; frameId++)
    {
        pFrame = pFrameList->frames[frameId];
        if (pFrame == NULL)
            continue;

        pFrameInfo = (System_FrameInfo *) pFrame->appData;
        UTILS_assert(pFrameInfo != NULL);

        pOrgFrame = pFrameInfo->pDupOrgFrame;
        UTILS_assert(pOrgFrame != NULL);

        pOrgFrameInfo = (System_FrameInfo *) pOrgFrame->appData;
        UTILS_assert(pOrgFrameInfo != NULL);

        pOrgFrameInfo->dupCount--;

        if (pOrgFrameInfo->dupCount == 0)
        {
            freeFrameList.frames[freeFrameList.numFrames] = pOrgFrame;
            freeFrameList.numFrames++;
        }
    }

    pObj->putFrameCount += freeFrameList.numFrames;

    System_putLinksEmptyFrames(pObj->createArgs.inQueParams.prevLinkId,
                               pObj->createArgs.inQueParams.prevLinkQueId,
                               &freeFrameList);

    Semaphore_post(pObj->lock);

    status = Utils_bufPutEmpty(&pObj->outFrameQue[queId], pFrameList);
    UTILS_assert(status == FVID2_SOK);

    return FVID2_SOK;
}

Void DupLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    DupLink_Obj *pObj = (DupLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = DupLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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

                DupLink_drvProcessFrames(pObj);
                break;
            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    DupLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 DupLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 dupId;
    DupLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    for (dupId = 0; dupId < DUP_LINK_OBJ_MAX; dupId++)
    {
        pObj = &gDupLink_obj[dupId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId = SYSTEM_MAKE_LINK_ID(procId, SYSTEM_LINK_ID_DUP_0) + dupId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = DupLink_getFullFrames;
        linkObj.linkPutEmptyFrames = DupLink_putEmptyFrames;
        linkObj.getLinkInfo = DupLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "DUP%d", dupId);

        status = Utils_tskCreate(&pObj->tsk,
                                 DupLink_tskMain,
                                 DUP_LINK_TSK_PRI,
                                 gDupLink_tskStack[dupId],
                                 DUP_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 DupLink_deInit()
{
    UInt32 dupId;

    for (dupId = 0; dupId < DUP_LINK_OBJ_MAX; dupId++)
    {
        Utils_tskDelete(&gDupLink_obj[dupId].tsk);
    }
    return FVID2_SOK;
}

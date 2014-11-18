/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "selectLink_priv.h"

#pragma DATA_ALIGN(gSelectLink_tskStack, 32)
#pragma DATA_SECTION(gSelectLink_tskStack, ".bss:taskStackSection")
UInt8 gSelectLink_tskStack[SELECT_LINK_OBJ_MAX][SELECT_LINK_TSK_STACK_SIZE];

SelectLink_Obj gSelectLink_obj[SELECT_LINK_OBJ_MAX];

Int32 SelectLink_drvCreate(SelectLink_Obj * pObj, SelectLink_CreateParams * pPrm)
{
    UInt32 outQueId, inChId, outChId;
    Int32 status;
    System_LinkQueInfo *pInQueInfo;
    System_LinkQueInfo *pOutQueInfo;
    SelectLink_ChInfo  *pInChInfo;
    SelectLink_OutQueChInfo *pOutQueChInfo;

    /* copy create args */
    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));
    UTILS_assert(pObj->createArgs.numOutQue <= SELECT_LINK_MAX_OUT_QUE);

    /* get previous link info */
    status = System_linkGetInfo(pPrm->inQueParams.prevLinkId, &pObj->inTskInfo);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pPrm->inQueParams.prevLinkQueId < pObj->inTskInfo.numQue);

    /* point to correct previous link output que info */
    pInQueInfo = &pObj->inTskInfo.queInfo[pPrm->inQueParams.prevLinkQueId];

    /* mark all Chs as not mapped */
    for(inChId=0; inChId<SYSTEM_MAX_CH_PER_OUT_QUE; inChId++)
    {
        pInChInfo = &pObj->inChInfo[inChId];

        pInChInfo->queId = SELECT_LINK_CH_NOT_MAPPED;
        pInChInfo->outChNum = 0;
        pInChInfo->rtChInfoUpdate = FALSE;

        memset(&pInChInfo->rtChInfo, 0, sizeof(pInChInfo->rtChInfo));
    }

    /* copy previous link channel info to local params */
    for(inChId=0; inChId<pInQueInfo->numCh; inChId++)
    {
        pInChInfo = &pObj->inChInfo[inChId];

        pInChInfo->rtChInfo = pInQueInfo->chInfo[inChId];
    }

    /* create out que info */
    pObj->info.numQue = pObj->createArgs.numOutQue;

    for (outQueId = 0; outQueId < pObj->createArgs.numOutQue; outQueId++)
    {
        status = Utils_bufCreate(&pObj->outFrameQue[outQueId], FALSE, FALSE);
        UTILS_assert(status == FVID2_SOK);

        pObj->prevOutQueChInfo[outQueId].outQueId = outQueId;
        pObj->prevOutQueChInfo[outQueId].numOutCh = 0;

        pOutQueChInfo = &pObj->createArgs.outQueChInfo[outQueId];
        pOutQueInfo   = &pObj->info.queInfo[outQueId];

        pOutQueChInfo->outQueId = outQueId;

        status = SelectLink_drvSetOutQueChInfo(pObj, pOutQueChInfo);
        UTILS_assert(status==FVID2_SOK);

        pOutQueInfo->numCh = pOutQueChInfo->numOutCh;

        for(outChId=0; outChId<pOutQueInfo->numCh; outChId++)
        {
            inChId = pOutQueChInfo->inChNum[outChId];

            if(inChId >= SYSTEM_MAX_CH_PER_OUT_QUE)
            {
                Vps_printf(" %d: SELECT   : Invalid input channel number (%d) specified, ignoring it !!!\n",
                        Utils_getCurTimeInMsec(),
                        inChId
                    );
                continue;
            }

            pInChInfo = &pObj->inChInfo[inChId];

            pOutQueInfo->chInfo[outChId] = pInChInfo->rtChInfo;
        }
    }


#ifdef SYSTEM_DEBUG_SELECT
    Vps_printf(" %d: SELECT   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 SelectLink_drvDelete(SelectLink_Obj * pObj)
{
    UInt32 outQueId;
    Int32 status;

    for(outQueId=0; outQueId<pObj->createArgs.numOutQue; outQueId++)
    {
        status = Utils_bufDelete(&pObj->outFrameQue[outQueId]);
        UTILS_assert(status == FVID2_SOK);
    }

#ifdef SYSTEM_DEBUG_SELECT
    Vps_printf(" %d: SELECT   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 SelectLink_drvSetOutQueChInfo(SelectLink_Obj * pObj, SelectLink_OutQueChInfo *pPrm)
{
    UInt32 inChNum, outChNum;
    SelectLink_ChInfo *pChInfo;
    SelectLink_OutQueChInfo *pPrevPrm;

    if(pPrm->numOutCh > SYSTEM_MAX_CH_PER_OUT_QUE)
        return FVID2_EFAIL;

    if(pPrm->outQueId > pObj->createArgs.numOutQue)
        return FVID2_EFAIL;

    pPrevPrm = &pObj->prevOutQueChInfo[pPrm->outQueId];

    /* remove prev output queue channel mapping */
    for(outChNum=0; outChNum<pPrevPrm->numOutCh; outChNum++)
    {
        inChNum = pPrevPrm->inChNum[outChNum];

        if(inChNum >= SYSTEM_MAX_CH_PER_OUT_QUE)
        {
            Vps_printf(" %d: SELECT   : Invalid input channel number (%d) specified, ignoring it !!!\n",
                    Utils_getCurTimeInMsec(),
                    inChNum
                );
            continue;
        }

        pChInfo = &pObj->inChInfo[inChNum];

        pChInfo->queId = SELECT_LINK_CH_NOT_MAPPED;
        pChInfo->outChNum = 0;
        pChInfo->rtChInfoUpdate = FALSE;
    }

    /* mapped input to output channels */
    for(outChNum=0; outChNum<pPrm->numOutCh; outChNum++)
    {
        inChNum = pPrm->inChNum[outChNum];

        if(inChNum >= SYSTEM_MAX_CH_PER_OUT_QUE)
        {
            Vps_printf(" %d: SELECT   : Invalid input channel number (%d) specified, ignoring it !!!\n",
                    Utils_getCurTimeInMsec(),
                    inChNum
                );
            continue;
        }

        pChInfo = &pObj->inChInfo[inChNum];

        pChInfo->queId = pPrm->outQueId;
        pChInfo->outChNum = outChNum;
        pChInfo->rtChInfoUpdate = TRUE;

        #ifdef SYSTEM_DEBUG_SELECT
        Vps_printf(" %d: SELECT: OUT QUE%d: OUT CH%d: IN CH%d: %d x %d, pitch = (%d, %d) [%s] [%s]\n",
                Utils_getCurTimeInMsec(),
                pChInfo->queId ,
                outChNum,
                inChNum,
                pChInfo->rtChInfo.width,
                pChInfo->rtChInfo.height,
                pChInfo->rtChInfo.pitch[0],
                pChInfo->rtChInfo.pitch[1],
                gSystem_nameScanFormat[pChInfo->rtChInfo.scanFormat],
                gSystem_nameMemoryType[pChInfo->rtChInfo.memType]
            );
        #endif

    }

    *pPrevPrm = *pPrm;

    return FVID2_SOK;
}

Int32 SelectLink_drvGetOutQueChInfo(SelectLink_Obj * pObj, SelectLink_OutQueChInfo *pPrm)
{
    UInt32 outChNum;
    SelectLink_OutQueChInfo *pPrevPrm;

    pPrm->numOutCh = 0;

    if(pPrm->outQueId > pObj->createArgs.numOutQue)
        return FVID2_EFAIL;

    pPrevPrm = &pObj->prevOutQueChInfo[pPrm->outQueId];

    /* copy current output que info to user supplied pointer */
    pPrm->numOutCh = pPrevPrm->numOutCh;

    if(pPrm->numOutCh > SYSTEM_MAX_CH_PER_OUT_QUE)
    {
        pPrm->numOutCh = 0;
        return FVID2_EFAIL;
    }

    for(outChNum=0; outChNum<pPrevPrm->numOutCh; outChNum++)
    {
        pPrm->inChNum[outChNum] = pPrevPrm->inChNum[outChNum];
    }

    return FVID2_SOK;
}

Int32 SelectLink_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    SelectLink_Obj *pObj = (SelectLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 SelectLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList)
{
    SelectLink_Obj *pObj = (SelectLink_Obj *) pTsk->appData;

    if(queId >= pObj->createArgs.numOutQue)
    {
        Vps_printf(
            " %d: SELECT   : WARNING: Que ID of %d is greater than number of output Que's supported %d\n",
            Utils_getCurTimeInMsec(),
            queId,
            pObj->createArgs.numOutQue
            );
        return FVID2_SOK;
    }

    return Utils_bufGetFull(&pObj->outFrameQue[queId], pFrameList, BIOS_NO_WAIT);
}

Int32 SelectLink_drvProcessFrames(SelectLink_Obj * pObj)
{
    SelectLink_CreateParams *pCreateArgs;
    SelectLink_ChInfo       *pChInfo;
    UInt32 frameId, queId;
    Int32 status;
    FVID2_Frame *pFrame;
    FVID2_FrameList frameList;
    System_FrameInfo *pFrameInfo;
    UInt32 newDataAvailable;
    UInt32 freeFrameNum;

    newDataAvailable = 0;

    freeFrameNum = 0;

    pCreateArgs = &pObj->createArgs;

    System_getLinksFullFrames(pCreateArgs->inQueParams.prevLinkId,
                              pCreateArgs->inQueParams.prevLinkQueId,
                              &frameList);

    if (frameList.numFrames)
    {
        for (frameId = 0; frameId < frameList.numFrames; frameId++)
        {
            /* remap channel number */
            pFrame = frameList.frames[frameId];

            UTILS_assert(pFrame->channelNum < SYSTEM_MAX_CH_PER_OUT_QUE);

            pChInfo = &pObj->inChInfo[pFrame->channelNum];

            if(pChInfo->queId >= pCreateArgs->numOutQue)
            {
                /* input channel not mapped to any output que, release the frame */
                frameList.frames[freeFrameNum] = pFrame;
                freeFrameNum++;
            }
            else
            {
                /* input channel mapped to a output que */
                pFrameInfo = (System_FrameInfo *) pFrame->appData;

                UTILS_assert(pFrameInfo != NULL);

                /* if rt params are updated copy to local rt params as well */
                if(pFrameInfo->rtChInfoUpdate)
                {
                    pChInfo->rtChInfo = pFrameInfo->rtChInfo;
                }

                /* if channel mapping is changed dynamically then set rtChInfoUpdate = TRUE for first time */
                if(pChInfo->rtChInfoUpdate)
                {
                    pChInfo->rtChInfoUpdate = FALSE;

                    pFrameInfo->rtChInfoUpdate = TRUE;

                    pFrameInfo->rtChInfo = pChInfo->rtChInfo;
                }

                /* save original channelNum so that we can restore it later while releasing the frame */
                pFrameInfo->selectOrgChannelNum = pFrame->channelNum;

                /* channel channel number according to output que channel number */
                pFrame->channelNum = pChInfo->outChNum;

                /* put the frame in output que */
                status = Utils_bufPutFullFrame(&pObj->outFrameQue[pChInfo->queId], pFrame);
                UTILS_assert(status == FVID2_SOK);

                /* mark as data put in output que so that we can send a notification to next link */
                newDataAvailable |= (1<<pChInfo->queId);
            }
        }

        if(freeFrameNum)
        {
            /* free channels not mapped to any output queue */
            frameList.numFrames = freeFrameNum;
            System_putLinksEmptyFrames(pCreateArgs->inQueParams.prevLinkId,
                                       pCreateArgs->inQueParams.prevLinkQueId,
                                       &frameList);

        }

        for(queId=0; queId<pCreateArgs->numOutQue; queId++)
        {
            /* send notification if needed */
            if (newDataAvailable & (1<<queId))
            {
                System_sendLinkCmd(pCreateArgs->outQueParams[queId].nextLink,
                               SYSTEM_CMD_NEW_DATA);
            }
        }

    }

    return FVID2_SOK;
}

Int32 SelectLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                               FVID2_FrameList * pFrameList)
{
    SelectLink_Obj *pObj = (SelectLink_Obj *) pTsk->appData;
    UInt32 frameId;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo;

    for (frameId = 0; frameId < pFrameList->numFrames; frameId++)
    {
        pFrame = pFrameList->frames[frameId];
        if (pFrame == NULL)
            continue;

        pFrameInfo = (System_FrameInfo *) pFrame->appData;
        UTILS_assert(pFrameInfo != NULL);

        pFrame->channelNum = pFrameInfo->selectOrgChannelNum;
    }

    System_putLinksEmptyFrames(pObj->createArgs.inQueParams.prevLinkId,
                               pObj->createArgs.inQueParams.prevLinkQueId,
                               pFrameList);

    return FVID2_SOK;
}

Void SelectLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    SelectLink_Obj *pObj = (SelectLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = SelectLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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

                SelectLink_drvProcessFrames(pObj);
                break;

            case SELECT_LINK_CMD_SET_OUT_QUE_CH_INFO:
                SelectLink_drvSetOutQueChInfo(
                    pObj,
                    (SelectLink_OutQueChInfo*)Utils_msgGetPrm(pMsg)
                  );
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SELECT_LINK_CMD_GET_OUT_QUE_CH_INFO:
                SelectLink_drvGetOutQueChInfo(
                    pObj,
                    (SelectLink_OutQueChInfo*)Utils_msgGetPrm(pMsg)
                  );
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    SelectLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 SelectLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 selectId;
    SelectLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    for (selectId = 0; selectId < SELECT_LINK_OBJ_MAX; selectId++)
    {
        pObj = &gSelectLink_obj[selectId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId, SYSTEM_LINK_ID_SELECT_0) + selectId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = SelectLink_getFullFrames;
        linkObj.linkPutEmptyFrames = SelectLink_putEmptyFrames;
        linkObj.getLinkInfo = SelectLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "SELECT%d", selectId);

        status = Utils_tskCreate(&pObj->tsk,
                                 SelectLink_tskMain,
                                 SELECT_LINK_TSK_PRI,
                                 gSelectLink_tskStack[selectId],
                                 SELECT_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 SelectLink_deInit()
{
    UInt32 selectId;

    for (selectId = 0; selectId < SELECT_LINK_OBJ_MAX; selectId++)
    {
        Utils_tskDelete(&gSelectLink_obj[selectId].tsk);
    }
    return FVID2_SOK;
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
 *  \file nullSrcLink_drv.c
 *
 *  \brief This file implements Video Source link as alternate
     of capture link.
 *
    This link will submit preloaded frames to the next links
 *
 */

#include "nullSrcLink_priv.h"

#pragma DATA_ALIGN(gNullSrcLink_tskStack, 32)
#pragma DATA_SECTION(gNullSrcLink_tskStack, ".bss:taskStackSection")
UInt8
    gNullSrcLink_tskStack[NULL_SRC_LINK_OBJ_MAX][NULL_SRC_LINK_TSK_STACK_SIZE];

NullSrcLink_Obj gNullSrcLink_obj[NULL_SRC_LINK_OBJ_MAX];

Void NullSrcLink_drvTimerCb(UArg arg)
{
    NullSrcLink_Obj *pObj = (NullSrcLink_Obj *) arg;

    Utils_tskSendCmd(&pObj->tsk, SYSTEM_CMD_NEW_DATA);
}

Int32 NullSrcLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    NullSrcLink_Obj *pObj = (NullSrcLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 NullSrcLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                FVID2_FrameList * pFrameList)
{
    NullSrcLink_Obj *pObj = (NullSrcLink_Obj *) pTsk->appData;

    return Utils_bufGetFull(&pObj->bufOutQue, pFrameList, BIOS_NO_WAIT);
}

Int32 NullSrcLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                 FVID2_FrameList * pFrameList)
{
    NullSrcLink_Obj *pObj = (NullSrcLink_Obj *) pTsk->appData;

    Utils_bufPutEmpty(&pObj->bufOutQue, pFrameList);

    return FVID2_SOK;
}

Int32 NullSrcLink_drvProcessFrames(NullSrcLink_Obj * pObj)
{
    System_LinkQueInfo *pChInfo;
    FVID2_FrameList frameList;
    UInt32 chId;
    FVID2_Frame *pFrame;
    Int32 status;

    pChInfo = &pObj->createArgs.inputInfo;

    frameList.numFrames = 0;

    for (chId = 0; chId < pChInfo->numCh; chId++)
    {
        status =
            Utils_bufGetEmptyFrame(&pObj->bufOutQue, &pFrame, BIOS_NO_WAIT);
        if (status != FVID2_SOK)
            continue;

        frameList.frames[frameList.numFrames] = pFrame;
        frameList.numFrames++;

        pFrame->channelNum = chId;

        if (pChInfo->chInfo[chId].scanFormat == FVID2_SF_INTERLACED)
        {
            pFrame->fid = pObj->chNextFid[chId];

            pObj->chNextFid[chId] ^= 1;
        }
    }

    Utils_bufPutFull(&pObj->bufOutQue, &frameList);

    System_sendLinkCmd(pObj->createArgs.outQueParams.nextLink,
                       SYSTEM_CMD_NEW_DATA);

    return FVID2_SOK;
}

Int32 NullSrcLink_drvStart(NullSrcLink_Obj * pObj)
{
    Clock_start(pObj->timer);

#ifdef SYSTEM_DEBUG_NULL
    Vps_printf(" %d: NULL_SRC: Start Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 NullSrcLink_drvStop(NullSrcLink_Obj * pObj)
{
    Clock_stop(pObj->timer);

#ifdef SYSTEM_DEBUG_NULL
    Vps_printf(" %d: NULL_SRC: Stop Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 NullSrcLink_drvDelete(NullSrcLink_Obj * pObj)
{
    Int32 status;

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: NULL_SRC: Delete in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    status = Utils_bufDelete(&pObj->bufOutQue);
    UTILS_assert(status == FVID2_SOK);

    Clock_delete(&pObj->timer);

    Utils_memFrameFree(&pObj->outFormat, pObj->outFrames, 1);

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: NULL_SRC: Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 NullSrcLink_fillDataPattern(FVID2_Format * pFormat,
                                  FVID2_Frame * pFrame, UInt16 numFrames)
{
    UInt32 size[2], frameId;

    size[0] = pFormat->pitch[0] * pFormat->height;
    size[1] = 0;

    switch (pFormat->dataFormat)
    {
        case FVID2_DF_YUV422SP_UV:
            /* for C plane */
            size[1] = pFormat->pitch[1] * pFormat->height;
            break;

        case FVID2_DF_YUV420SP_UV:
            /* for C plane */
            size[1] = pFormat->pitch[1] * pFormat->height / 2;
            break;
    }

    for (frameId = 0; frameId < numFrames; frameId++)
    {
        if (pFrame->addr[0][0] && size[0])
        {
            memset(pFrame->addr[0][0], 16 * frameId, size[0]);
        }
        if (pFrame->addr[0][1] && size[1])
        {
            memset(pFrame->addr[0][1], 0x80, size[1]);
        }
    }

    return FVID2_SOK;
}

Int32 NullSrcLink_drvCreate(NullSrcLink_Obj * pObj,
                            NullSrcLink_CreateParams * pPrm)
{
    Int32 status;
    UInt32 chId, frameId;
    Clock_Params clockParams;
    System_LinkChInfo *pChInfo;
    FVID2_Frame *pFrame;

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: NULL_SRC: Create in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    UTILS_assert(pPrm->inputInfo.numCh <= SYSTEM_MAX_CH_PER_OUT_QUE);

    status = Utils_bufCreate(&pObj->bufOutQue, FALSE, FALSE);
    UTILS_assert(status == FVID2_SOK);

    if (pPrm->timerPeriod == 0 || pPrm->timerPeriod > 200)
        pPrm->timerPeriod = 16;

    Clock_Params_init(&clockParams);
    clockParams.period = pPrm->timerPeriod;
    clockParams.arg = (UArg) pObj;

    pObj->timer = Clock_create(NullSrcLink_drvTimerCb,
                               pPrm->timerPeriod, &clockParams, NULL);
    UTILS_assert(pObj->timer != NULL);

    pObj->info.numQue = 1;
    pObj->info.queInfo[0].numCh = pPrm->inputInfo.numCh;

    pChInfo = &pPrm->inputInfo.chInfo[0];

    pObj->outFormat.channelNum = 0;
    pObj->outFormat.width = pChInfo->width;
    pObj->outFormat.height = pChInfo->height;
    pObj->outFormat.pitch[0] = pChInfo->pitch[0];
    pObj->outFormat.pitch[1] = pChInfo->pitch[1];
    pObj->outFormat.pitch[2] = pChInfo->pitch[2];
    pObj->outFormat.fieldMerged[0] = FALSE;
    pObj->outFormat.fieldMerged[1] = FALSE;
    pObj->outFormat.fieldMerged[2] = FALSE;
    pObj->outFormat.dataFormat = pChInfo->dataFormat;
    pObj->outFormat.scanFormat = pChInfo->scanFormat;
    pObj->outFormat.bpp = FVID2_BPP_BITS16;
    pObj->outFormat.reserved = NULL;

    status = Utils_memFrameAlloc(&pObj->outFormat, pObj->outFrames, 1);
    UTILS_assert(status == FVID2_SOK);

    NullSrcLink_fillDataPattern(&pObj->outFormat, pObj->outFrames, 1);

    #ifndef SYSTEM_USE_TILER
    if (pObj->createArgs.tilerEnable)
    {
        Vps_printf("NULLSRC:!!WARNING.FORCIBLY DISABLING TILER since tiler is disabled at build time");
        pObj->createArgs.tilerEnable = FALSE;
    }
    #endif

    for (chId = 0; chId < pPrm->inputInfo.numCh; chId++)
    {
        memcpy(&pObj->info.queInfo[0].chInfo[chId],
               &pPrm->inputInfo.chInfo[chId],
               sizeof(pPrm->inputInfo.chInfo[chId]));

        pObj->info.queInfo[0].chInfo[chId].memType = VPS_VPDMA_MT_NONTILEDMEM;
        if (pObj->createArgs.tilerEnable)
        {
            pObj->info.queInfo[0].chInfo[chId].memType = VPS_VPDMA_MT_TILEDMEM;
        }
        pObj->chNextFid[chId] = 0;
    }

    for (frameId = 0;
         frameId < pPrm->inputInfo.numCh * SYSTEM_LINK_FRAMES_PER_CH; frameId++)
    {
        pFrame = &pObj->outFrames[frameId];

        memcpy(pFrame, &pObj->outFrames[0], sizeof(*pFrame));

        pFrame->appData = &pObj->frameInfo[frameId];

        memset(&pObj->frameInfo[frameId], 0, sizeof(pObj->frameInfo[frameId]));

        status = Utils_bufPutEmptyFrame(&pObj->bufOutQue, pFrame);
        UTILS_assert(status == FVID2_SOK);
    }

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: NULL_SRC: Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 NullSrcLink_tskRun(NullSrcLink_Obj * pObj, Utils_TskHndl * pTsk,
                         Utils_MsgHndl ** pMsg, Bool * done, Bool * ackMsg)
{
    Int32 status = FVID2_SOK;
    Bool runDone, runAckMsg;
    Utils_MsgHndl *pRunMsg;
    UInt32 cmd;

    *done = FALSE;
    *ackMsg = FALSE;

    runDone = FALSE;
    runAckMsg = FALSE;

    *pMsg = NULL;

    while (!runDone)
    {
        status = Utils_tskRecvMsg(pTsk, &pRunMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        cmd = Utils_msgGetCmd(pRunMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_NEW_DATA:
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                status = NullSrcLink_drvProcessFrames(pObj);
                if (status != FVID2_SOK)
                {
                    runDone = TRUE;
                    runAckMsg = FALSE;
                }
                break;

            case SYSTEM_CMD_STOP:
                runDone = TRUE;
                runAckMsg = TRUE;
                break;
            case SYSTEM_CMD_DELETE:
                *done = TRUE;
                *ackMsg = TRUE;
                *pMsg = pRunMsg;
                runDone = TRUE;
                break;
            default:
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
        }

    }

    NullSrcLink_drvStop(pObj);

    if (runAckMsg)
        Utils_tskAckOrFreeMsg(pRunMsg, status);

    return status;
}

Void NullSrcLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    NullSrcLink_Obj *pObj;

    pObj = (NullSrcLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = NullSrcLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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
            case SYSTEM_CMD_START:
                status = NullSrcLink_drvStart(pObj);

                Utils_tskAckOrFreeMsg(pMsg, status);

                if (status == FVID2_SOK)
                {
                    status =
                        NullSrcLink_tskRun(pObj, pTsk, &pMsg, &done, &ackMsg);
                }

                break;
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    NullSrcLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 NullSrcLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 nullSrcId;
    NullSrcLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    for (nullSrcId = 0; nullSrcId < NULL_SRC_LINK_OBJ_MAX; nullSrcId++)
    {
        pObj = &gNullSrcLink_obj[nullSrcId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId, SYSTEM_LINK_ID_NULL_SRC_0) + nullSrcId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NullSrcLink_getFullFrames;
        linkObj.linkPutEmptyFrames = NullSrcLink_putEmptyFrames;
        linkObj.getLinkInfo = NullSrcLink_getInfo;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "NULL_SRC%d", nullSrcId);

        status = Utils_tskCreate(&pObj->tsk,
                                 NullSrcLink_tskMain,
                                 NULL_SRC_LINK_TSK_PRI,
                                 gNullSrcLink_tskStack[nullSrcId],
                                 (NULL_SRC_LINK_TSK_STACK_SIZE), pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 NullSrcLink_deInit()
{
    UInt32 nullSrcId;

    for (nullSrcId = 0; nullSrcId < NULL_SRC_LINK_OBJ_MAX; nullSrcId++)
    {
        Utils_tskDelete(&gNullSrcLink_obj[nullSrcId].tsk);
    }
    return FVID2_SOK;
}

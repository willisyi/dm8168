/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "deiLink_priv.h"

#pragma DATA_ALIGN(gDeiLink_tskStack, 32)
#pragma DATA_SECTION(gDeiLink_tskStack, ".bss:taskStackSection")
UInt8 gDeiLink_tskStack[DEI_LINK_OBJ_MAX][DEI_LINK_TSK_STACK_SIZE];

DeiLink_Obj gDeiLink_obj[DEI_LINK_OBJ_MAX];

Void DeiLink_tskMain(struct Utils_TskHndl *pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done, stopDone;
    Int32 status;
    DeiLink_ChannelInfo * channelInfo;
    DeiLink_Obj *pObj;
    UInt32 flushCmds[2];

    pObj = (DeiLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = DeiLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);

    if (status != FVID2_SOK)
        return;

    done = FALSE;
    ackMsg = FALSE;
    stopDone = FALSE;

    while (!done)
    {
        status = Utils_tskRecvMsg(pTsk, &pMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        cmd = Utils_msgGetCmd(pMsg);

        if(stopDone==TRUE && cmd!=SYSTEM_CMD_DELETE)
        {
            /* once stop is done, only DELETE command should be accepted */
            Utils_tskAckOrFreeMsg(pMsg, status);
            continue;
        }

        switch (cmd)
        {
            case SYSTEM_CMD_NEW_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

				flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);

                DeiLink_drvProcessData(pObj);
                break;

            case DEI_LINK_CMD_GET_PROCESSED_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

                DeiLink_drvGetProcessedData(pObj);
                break;
            case DEI_LINK_CMD_PRINT_STATISTICS:
                DeiLink_printStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case DEI_LINK_CMD_PRINT_BUFFER_STATISTICS:
                Utils_tskAckOrFreeMsg(pMsg, status);
                DeiLink_printBufferStatus(pObj);
                break;

            case DEI_LINK_CMD_GET_OUTPUTRESOLUTION:
                {
                    DeiLink_chDynamicSetOutRes *params;

                    params = (DeiLink_chDynamicSetOutRes *) Utils_msgGetPrm(pMsg);
                    DeiLink_drvGetChDynamicOutputRes(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case DEI_LINK_CMD_SET_OUTPUTRESOLUTION:
                {
                    DeiLink_chDynamicSetOutRes *params;

                    params = (DeiLink_chDynamicSetOutRes *) Utils_msgGetPrm(pMsg);
                    DeiLink_drvSetChDynamicOutputRes(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case DEI_LINK_CMD_SET_FRAME_RATE:
                {
                    DeiLink_ChFpsParams *params;

                    params = (DeiLink_ChFpsParams *) Utils_msgGetPrm(pMsg);
                    DeiLink_SetFrameRate(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case DEI_LINK_CMD_DISABLE_CHANNEL:
            case DEI_LINK_CMD_ENABLE_CHANNEL:

                #ifdef SYSTEM_VERBOSE_PRINTS
                Vps_rprintf(" %d: DEI: Channel Enable/Disable in progress ... !!!\n",
                                       Utils_getCurTimeInMsec());
                #endif

                channelInfo = (DeiLink_ChannelInfo *) Utils_msgGetPrm(pMsg);

                DeiLink_drvSetChannelInfo(pObj,channelInfo);

                Utils_tskAckOrFreeMsg(pMsg, status);

                #ifdef SYSTEM_VERBOSE_PRINTS
                Vps_rprintf(" %d: DEI: Channel Enable/Disable in progress ... DONE !!!\n",
                                       Utils_getCurTimeInMsec());
                #endif

                break;

            case DEI_LINK_CMD_FLUSH_CHANNEL_INPUT:
                {
                    DeiLink_ChFlushParams *params;

                    params = (DeiLink_ChFlushParams *) Utils_msgGetPrm(pMsg);
                    DeiLink_drvFlushChannel(pObj, params->chId);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case DEI_LINK_CMD_SET_EVEN_ODD_EVEN_PATTERN_DEI:
                {
                    DeiLink_ChSetEvenOddEvenPatternDeiParams *params;
                    params = (DeiLink_ChSetEvenOddEvenPatternDeiParams *) Utils_msgGetPrm(pMsg);
                    DeiLink_drvSetEvenOddEvenDei(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case SYSTEM_CMD_STOP:
                DeiLink_drvStop(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                stopDone = TRUE;
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

    DeiLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 DeiLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    DeiLink_Obj *pObj;

    UInt32 objId;

    for (objId = 0; objId < DEI_LINK_OBJ_MAX; objId++)
    {
        pObj = &gDeiLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));
#ifdef TI_816X_BUILD
        pObj->linkId = SYSTEM_LINK_ID_DEI_HQ_0 + objId;
#endif                                                     


#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        pObj->linkId = SYSTEM_LINK_ID_DEI_HQ_0 + objId;
#endif                                                     

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = DeiLink_getFullFrames;
        linkObj.linkPutEmptyFrames = DeiLink_putEmptyFrames;
        linkObj.getLinkInfo = DeiLink_getInfo;

        sprintf(pObj->name, "DEI%d    ", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 DeiLink_tskMain,
                                 DEI_LINK_TSK_PRI,
                                 gDeiLink_tskStack[objId],
                                 DEI_LINK_TSK_STACK_SIZE, pObj, pObj->name);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 DeiLink_deInit()
{
    UInt32 objId;
    DeiLink_Obj *pObj;

    for (objId = 0; objId < DEI_LINK_OBJ_MAX; objId++)
    {
        pObj = &gDeiLink_obj[objId];

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

Int32 DeiLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    DeiLink_Obj *pObj = (DeiLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 DeiLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                            FVID2_FrameList * pFrameList)
{
    DeiLink_Obj *pObj = (DeiLink_Obj *) pTsk->appData;

    UTILS_assert(queId < DEI_LINK_MAX_OUT_QUE);

    return Utils_bufGetFull(&pObj->outObj[queId].bufOutQue, pFrameList,
                            BIOS_NO_WAIT);
}

Int32 DeiLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList)
{
    DeiLink_Obj *pObj = (DeiLink_Obj *) pTsk->appData;
    UInt32 idx;
    UInt32 chId;
    DeiLink_OutObj *pOutObj = &pObj->outObj[queId];
    Int32 status = FVID2_SOK;

    UTILS_assert(queId < DEI_LINK_MAX_OUT_QUE);
    UTILS_assert(pFrameList != NULL);
    UTILS_assert(pFrameList->numFrames <= FVID2_MAX_FVID_FRAME_PTR);

    for (idx = 0; idx < pFrameList->numFrames; idx++)
    {
        chId = pFrameList->frames[idx]->channelNum;
        UTILS_assert(chId < DEI_LINK_MAX_CH);
        status = Utils_quePut(&pOutObj->emptyBufQue[chId], pFrameList->frames[idx], BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }
    return status;
}

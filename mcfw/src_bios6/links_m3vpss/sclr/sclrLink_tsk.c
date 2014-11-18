/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "sclrLink_priv.h"

#pragma DATA_ALIGN(gSclrLink_tskStack, 32)
#pragma DATA_SECTION(gSclrLink_tskStack, ".bss:taskStackSection")
UInt8 gSclrLink_tskStack[SCLR_LINK_OBJ_MAX][SCLR_LINK_TSK_STACK_SIZE];

SclrLink_Obj gSclrLink_obj[SCLR_LINK_OBJ_MAX];

Void SclrLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    SclrLink_Obj *pObj;
    SclrLink_ChannelInfo * channelInfo;
    UInt32 flushCmds[2];

    pObj = (SclrLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = SclrLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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
            case SYSTEM_CMD_NEW_DATA:
            
                Utils_tskAckOrFreeMsg(pMsg, status);

                flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);

                SclrLink_drvProcessData(pObj);
                break;

            case SCLR_LINK_CMD_GET_PROCESSED_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

                SclrLink_drvGetProcessedData(pObj);
                break;

            case SYSTEM_CMD_STOP:
                SclrLink_drvStop(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SCLR_LINK_CMD_SET_FRAME_RATE:
                {
                    SclrLink_ChFpsParams *params;

                    params = (SclrLink_ChFpsParams *) Utils_msgGetPrm(pMsg);
                    SclrLink_SetFrameRate(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case SCLR_LINK_CMD_GET_OUTPUTRESOLUTION:
                {
                    SclrLink_chDynamicSetOutRes *params;

                    params = (SclrLink_chDynamicSetOutRes *) Utils_msgGetPrm(pMsg);
                    SclrLink_drvGetChDynamicOutputRes(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
                
            case SCLR_LINK_CMD_SET_OUTPUTRESOLUTION:
                {
                    SclrLink_chDynamicSetOutRes *params;

                    params = (SclrLink_chDynamicSetOutRes *) Utils_msgGetPrm(pMsg);
                    SclrLink_drvSetChDynamicOutputRes(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case SCLR_LINK_CMD_SKIP_FID_TYPE:
                {
                    SclrLink_chDynamicSkipFidType *params;

                    params = (SclrLink_chDynamicSkipFidType *) Utils_msgGetPrm(pMsg);
                    SclrLink_drvDynamicSkipFidType(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case SCLR_LINK_CMD_DISABLE_CHANNEL:
            case SCLR_LINK_CMD_ENABLE_CHANNEL:

#ifdef SYSTEM_DEBUG_SCLR
                 Vps_rprintf(" %d: SCLR	: Channel Enable/Disable in progress ... !!!\n", Utils_getCurTimeInMsec());
#endif

                channelInfo = (SclrLink_ChannelInfo *) Utils_msgGetPrm(pMsg);

                SclrLink_drvSetChannelInfo(pObj,channelInfo);
                
                Utils_tskAckOrFreeMsg(pMsg, status);                

#ifdef SYSTEM_DEBUG_SCLR
                Vps_rprintf(" %d: SCLR	: Channel Enable/Disable in progress ... DONE !!!\n", Utils_getCurTimeInMsec());
#endif
                break;

            case SYSTEM_CMD_DELETE:
                SclrLink_drvStop(pObj);
                done = TRUE;
                ackMsg = TRUE;
                break;

            case SCLR_LINK_CMD_PRINT_STATISTICS:
                SclrLink_drvPrintStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

        }
    }

    SclrLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 SclrLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    SclrLink_Obj *pObj;
    UInt32 objId;

    for (objId = 0; objId < SCLR_LINK_OBJ_MAX; objId++)
    {
        pObj = &gSclrLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->linkId = SYSTEM_LINK_ID_SCLR_INST_0 + objId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = SclrLink_getFullFrames;
        linkObj.linkPutEmptyFrames = SclrLink_putEmptyFrames;
        linkObj.getLinkInfo = SclrLink_getInfo;

        UTILS_SNPRINTF(pObj->name, "Scalar%d   ", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 SclrLink_tskMain,
                                 SCALAR_LINK_TSK_PRI,
                                 gSclrLink_tskStack[objId],
                                 SCLR_LINK_TSK_STACK_SIZE, pObj, pObj->name);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 SclrLink_deInit()
{
    UInt32 objId;
    SclrLink_Obj *pObj;

    for (objId = 0; objId < SCLR_LINK_OBJ_MAX; objId++)
    {
        pObj = &gSclrLink_obj[objId];

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

Int32 SclrLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    SclrLink_Obj *pObj = (SclrLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 SclrLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList)
{
    SclrLink_Obj *pObj = (SclrLink_Obj *) pTsk->appData;

    return Utils_bufGetFull(&pObj->outObj.bufOutQue, pFrameList, BIOS_NO_WAIT);
}

Int32 SclrLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList)
{
    SclrLink_Obj *pObj = (SclrLink_Obj *) pTsk->appData;

    return Utils_bufPutEmpty(&pObj->outObj.bufOutQue, pFrameList);
}

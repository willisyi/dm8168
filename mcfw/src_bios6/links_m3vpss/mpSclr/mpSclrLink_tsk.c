/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/* TODO
    1. Fine tune the stack required
 */

#include "mpSclrLink_priv.h"

#pragma DATA_ALIGN(gMpSclrLink_tskStackFwdQ, 32)
#pragma DATA_SECTION(gMpSclrLink_tskStackFwdQ, ".bss:taskStackSection")
UInt8 gMpSclrLink_tskStackFwdQ[MP_SCLR_LINK_OBJ_MAX][MP_SCLR_LINK_TSK_STACK_SIZE];

#pragma DATA_ALIGN(gMpSclrLink_tskStackSclr, 32)
#pragma DATA_SECTION(gMpSclrLink_tskStackSclr, ".bss:taskStackSection")
UInt8 gMpSclrLink_tskStackSclr[MP_SCLR_LINK_OBJ_MAX][MP_SCLR_LINK_TSK_STACK_SIZE];

MpSclrLink_Obj gMpSclrLink_obj[MP_SCLR_LINK_OBJ_MAX];

/* Local functions */
Int32 MpSclrLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList);
Int32 MpSclrLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList);
Int32 MpSclrLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);


Void MpSclrLink_tskSclr(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    MpSclrLink_Obj *pObj;
    UInt32 flushCmds[2];

    pObj = (MpSclrLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    Utils_tskAckOrFreeMsg(pMsg, FVID2_SOK);
    
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
                MpSclrLink_drvProcessMpFrames(pObj);
                break;

            case MP_SCLR_LINK_CMD_SCALING_DONE:
                Utils_tskAckOrFreeMsg(pMsg, status);
                MpSclrLink_drvMpPostProcessedFrames(pObj);
                break;

            case MP_SCLR_LINK_CMD_PRINT_STATISTICS:
                MpSclrLink_drvPrintStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case MP_SCLR_LINK_CMD_SET_OUTPUTRESOLUTION:
                {
                    MpSclrLink_chDynamicSetOutRes *params;

                    params = 
                        (MpSclrLink_chDynamicSetOutRes *) Utils_msgGetPrm(pMsg);
                    MpSclrLink_drvSetChOutputRes(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;

            default :
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    MpSclrLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}


Void MpSclrLink_tskFwdQ(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    MpSclrLink_Obj *pObj;
    UInt32 flushCmds[2];

    pObj = (MpSclrLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = MpSclrLink_drvCreate(pObj, 
        (MpSclrLink_CreateParams *) Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);
    if (status != FVID2_SOK)
        return;

    /* Create the MP Scalar Task */
    status = Utils_tskSendCmd(&pObj->mpScTskHndl, SYSTEM_CMD_CREATE);
    if (status != FVID2_SOK)
    {
        MpSclrLink_drvDelete(pObj);
        return;
    }
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

                MpSclrLink_drvMpProcessAllCh(pObj);
                break;

            case MP_SCLR_LINK_CMD_GET_OUTPUTRESOLUTION:
                {
                    MpSclrLink_chDynamicSetOutRes *params;

                    params = 
                        (MpSclrLink_chDynamicSetOutRes *) Utils_msgGetPrm(pMsg);
                    MpSclrLink_drvGetChOutputRes(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
                
            case MP_SCLR_LINK_CMD_SET_OUTPUTRESOLUTION:
                {
                    MpSclrLink_chDynamicSetOutRes *params;

                    params = 
                        (MpSclrLink_chDynamicSetOutRes *) Utils_msgGetPrm(pMsg);
                    MpSclrLink_drvSetChOutputRes(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case SYSTEM_CMD_DELETE:
                /* The SCALAR Task will release all acquired resources, as this
                    this does not use any resorces post this command. We should
                    not release resources here, as scalar task might still be
                    using them */
                status = Utils_tskSendCmd(&pObj->mpScTskHndl, 
                        SYSTEM_CMD_DELETE);
                UTILS_assert(status == FVID2_SOK);
                done = TRUE;
                ackMsg = TRUE;
                break;

            case MP_SCLR_LINK_CMD_PRINT_STATISTICS:
                status = Utils_tskSendCmd(&pObj->mpScTskHndl, 
                                    MP_SCLR_LINK_CMD_PRINT_STATISTICS);
                UTILS_assert(status == FVID2_SOK);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

        }
    }

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 MpSclrLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    MpSclrLink_Obj *pObj;
    UInt32 objId;

    for (objId = 0; objId < MP_SCLR_LINK_OBJ_MAX; objId++)
    {
        pObj = &gMpSclrLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->linkId = SYSTEM_LINK_ID_MP_SCLR_INST_0 + objId;

        linkObj.pTsk = &pObj->mainTskHndl;
        linkObj.linkGetFullFrames = MpSclrLink_getFullFrames;
        linkObj.linkPutEmptyFrames = MpSclrLink_putEmptyFrames;
        linkObj.getLinkInfo = MpSclrLink_getInfo;

        sprintf(pObj->mainTskName, "MP_SCLR_FWD_Q%d ", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->mainTskHndl,
                                 MpSclrLink_tskFwdQ,
                                 MP_SCALAR_LINK_TSK_PRI,
                                 gMpSclrLink_tskStackFwdQ[objId],
                                 MP_SCLR_LINK_TSK_STACK_SIZE, pObj, pObj->mainTskName);
        UTILS_assert(status == FVID2_SOK);
        
        sprintf(pObj->mpScTskName, "MP_SCLR%d ", objId);

        status = Utils_tskCreate(&pObj->mpScTskHndl,
                                 MpSclrLink_tskSclr,
                                 MP_SCALAR_LINK_TSK_PRI,
                                 gMpSclrLink_tskStackSclr[objId],
                                 MP_SCLR_LINK_TSK_STACK_SIZE, pObj, pObj->mpScTskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 MpSclrLink_deInit()
{
    UInt32 objId;
    MpSclrLink_Obj *pObj;

    for (objId = 0; objId < MP_SCLR_LINK_OBJ_MAX; objId++)
    {
        pObj = &gMpSclrLink_obj[objId];

        Utils_tskDelete(&pObj->mainTskHndl);
        Utils_tskDelete(&pObj->mpScTskHndl);
    }

    return FVID2_SOK;
}

Int32 MpSclrLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    MpSclrLink_Obj *pObj = (MpSclrLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 MpSclrLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList)
{
    Int32 status;

    MpSclrLink_Obj *pObj = (MpSclrLink_Obj *) pTsk->appData;
    status =  Utils_bufGetFull(&pObj->linkBufQ, pFrameList, BIOS_NO_WAIT);
    return status;
}

Int32 MpSclrLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList)
{
    Int32 status;
    UInt32 i, currIndex;
    FVID2_Frame *pFrame;
    FVID2_FrameList freeFrameList;
    MpSclrLink_Obj *pObj = (MpSclrLink_Obj *) pTsk->appData;

    status = FVID2_SOK;
    /* Check if the frame belong to MP SCLR, if not put back to previous link */
    currIndex = 0;
    for (i = 0; i < pFrameList->numFrames; i++)
    {
        pFrame = pFrameList->frames[i];
        if ((((System_FrameInfo *)pFrame->appData)->isMpFrame) == 0)
        {
            /* Frame belongs to previous link */
            freeFrameList.frames[currIndex] = pFrame;
            currIndex++;
        }
        else
        {
            /* This frame belongs to MP SCLR */
            debugMpSclrPrintFrame(pFrame, "Next Link Return Output Frame");
            status = Utils_bufPutEmptyFrame(&pObj->linkBufQ, pFrame);

        }
    }
    if (currIndex)
    {
        freeFrameList.numFrames = currIndex;
        status =
        System_putLinksEmptyFrames(pObj->createArgs.inQueParams.prevLinkId,
                                   pObj->createArgs.inQueParams.prevLinkQueId, 
                                   &freeFrameList);
    }

    return status;
}

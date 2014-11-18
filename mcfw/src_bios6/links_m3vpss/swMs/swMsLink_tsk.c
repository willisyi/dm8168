/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "swMsLink_priv.h"

#pragma DATA_ALIGN(gSwMsLink_tskStack, 32)
#pragma DATA_SECTION(gSwMsLink_tskStack, ".bss:taskStackSection")
UInt8 gSwMsLink_tskStack[SW_MS_LINK_OBJ_MAX][SW_MS_LINK_TSK_STACK_SIZE];

SwMsLink_Obj gSwMsLink_obj[SW_MS_LINK_OBJ_MAX];

Int32 SwMsLink_tskRun(SwMsLink_Obj * pObj, Utils_TskHndl * pTsk,
                      Utils_MsgHndl ** pMsg, Bool * done, Bool * ackMsg)
{
    Int32 status = FVID2_SOK;
    Bool runDone, runAckMsg;
    Utils_MsgHndl *pRunMsg;
    UInt32 cmd;
    Void *pPrm;
    UInt32 flushCmds[4];

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

				flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);

                SwMsLink_drvProcessData(pObj);
                break;

            case SW_MS_LINK_CMD_DO_SCALING:
                Utils_tskAckOrFreeMsg(pRunMsg, status);

				flushCmds[0] = SYSTEM_CMD_NEW_DATA;
				flushCmds[1] = SW_MS_LINK_CMD_DO_SCALING;
                Utils_tskFlushMsg(pTsk, flushCmds, 2);

				SwMsLink_drvProcessData(pObj);
                SwMsLink_drvDoScaling(pObj);
                break;

            case SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT:
                pPrm = Utils_msgGetPrm(pRunMsg);

                status = SwMsLink_drvSwitchLayout(pObj, pPrm, FALSE);

                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

           case SYSTEM_SW_MS_LINK_CMD_GET_LAYOUT_PARAMS:
                pPrm = Utils_msgGetPrm(pRunMsg);

                status = SwMsLink_drvGetLayoutParams(pObj, pPrm);

                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case SYSTEM_SW_MS_LINK_CMD_GET_INPUT_CHNL_INFO:
                 pPrm = Utils_msgGetPrm(pRunMsg);

                 status = SwMsLink_drvGetInputChInfoFromWinId(pObj, pPrm);

                 Utils_tskAckOrFreeMsg(pRunMsg, status);
                 break;

            case SYSTEM_SW_MS_LINK_CMD_SET_CROP_PARAM:
                 pPrm = Utils_msgGetPrm(pRunMsg);

                 status = SwMsLink_drvSetCropParam(pObj, pPrm);

                 Utils_tskAckOrFreeMsg(pRunMsg, status);
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
            case SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS:
                SwMsLink_drvPrintStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
            case SYSTEM_SW_MS_LINK_CMD_PRINT_BUFFER_STATISTICS:
                SwMsLink_printBufferStatus(pObj);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
            case SYSTEM_SW_MS_LINK_CMD_FLUSH_BUFFERS:
                pPrm = Utils_msgGetPrm(pRunMsg);
                status = SwMsLink_flushBuffers(pObj,pPrm);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
            default:
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
        }

    }

    SwMsLink_drvStop(pObj);

    if (runAckMsg)
        Utils_tskAckOrFreeMsg(pRunMsg, status);

    return status;
}

Void SwMsLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    SwMsLink_Obj *pObj;
    Void *pPrm;

    pObj = (SwMsLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = SwMsLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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
                status = SwMsLink_drvStart(pObj);

                Utils_tskAckOrFreeMsg(pMsg, status);

                if (status == FVID2_SOK)
                {
                    status = SwMsLink_tskRun(pObj, pTsk, &pMsg, &done, &ackMsg);
                }

                break;
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;

            case SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT:
                pPrm = Utils_msgGetPrm(pMsg);

                status = SwMsLink_drvSwitchLayout(pObj, pPrm, FALSE);

                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

			case SYSTEM_SW_MS_LINK_CMD_GET_LAYOUT_PARAMS:
				 pPrm = Utils_msgGetPrm(pMsg);

				 status = SwMsLink_drvGetLayoutParams(pObj, pPrm);

				 Utils_tskAckOrFreeMsg(pMsg, status);
				 break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    SwMsLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 SwMsLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    SwMsLink_Obj *pObj;
    UInt32 objId;

    for (objId = 0; objId < SW_MS_LINK_OBJ_MAX; objId++)
    {
        pObj = &gSwMsLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->linkId = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0 + objId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = SwMsLink_getFullFrames;
        linkObj.linkPutEmptyFrames = SwMsLink_putEmptyFrames;
        linkObj.getLinkInfo = SwMsLink_getInfo;

        sprintf(pObj->name, "SWMS%d", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 SwMsLink_tskMain,
                                 SW_MS_LINK_TSK_PRI,
                                 gSwMsLink_tskStack[objId],
                                 SW_MS_LINK_TSK_STACK_SIZE, pObj, pObj->name);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 SwMsLink_deInit()
{
    UInt32 objId;
    SwMsLink_Obj *pObj;

    for (objId = 0; objId < SW_MS_LINK_OBJ_MAX; objId++)
    {
        pObj = &gSwMsLink_obj[objId];

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

Int32 SwMsLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    SwMsLink_Obj *pObj = (SwMsLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 SwMsLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList)
{
    SwMsLink_Obj *pObj = (SwMsLink_Obj *) pTsk->appData;

    return Utils_bufGetFull(&pObj->bufOutQue, pFrameList, BIOS_NO_WAIT);
}

Int32 SwMsLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList)
{
    Int status;
    SwMsLink_Obj *pObj = (SwMsLink_Obj *) pTsk->appData;
    if (pObj->createArgs.enableOuputDup)
    {
        status = SwMsLink_drvFreeProcessedFrames(pObj, pFrameList);
    }
    else
    {
        status = Utils_bufPutEmpty(&pObj->bufOutQue, pFrameList);
    }
    return (status);
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "nsfLink_priv.h"

#pragma DATA_ALIGN(gNsfLink_tskStack, 32)
#pragma DATA_SECTION(gNsfLink_tskStack, ".bss:taskStackSection")
UInt8 gNsfLink_tskStack[NSF_LINK_OBJ_MAX][NSF_LINK_TSK_STACK_SIZE];

NsfLink_Obj gNsfLink_obj[NSF_LINK_OBJ_MAX];

Int32 NsfLink_tskRun(NsfLink_Obj * pObj,
                     Utils_TskHndl * pTsk, Utils_MsgHndl ** pMsg, Bool * done,
                     Bool * ackMsg)
{
    Int32 status = FVID2_SOK;
    Bool runDone, runAckMsg;
    Utils_MsgHndl *pRunMsg;
    UInt32 flushCmds[4];
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
                flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);

                status = NsfLink_drvProcessData(pObj);
                if (status != FVID2_SOK)
                {
                    runDone = TRUE;
                    runAckMsg = TRUE;
                }
                break;

            case NSF_LINK_CMD_SET_FRAME_RATE:
                {
                    NsfLink_ChFpsParams *params;
                    params = (NsfLink_ChFpsParams *) Utils_msgGetPrm(pRunMsg);
                    NsfLink_SetFrameRate(pObj, params);
                    Utils_tskAckOrFreeMsg(pRunMsg, status);
                }
                break;

            case NSF_LINK_CMD_PRINT_STATISTICS:
                NsfLink_drvPrintStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case NSF_LINK_CMD_PRINT_BUFFER_STATISTICS:
                NsfLink_printBufferStatus(pObj);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case SYSTEM_CMD_STOP:
                flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);
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

    if (runAckMsg)
        Utils_tskAckOrFreeMsg(pRunMsg, status);

    return status;
}

Void NsfLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    NsfLink_Obj *pObj;

    pObj = (NsfLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = NsfLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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

                Utils_tskAckOrFreeMsg(pMsg, status);

                status = NsfLink_tskRun(pObj, pTsk, &pMsg, &done, &ackMsg);

                break;

            case NSF_LINK_CMD_SET_FRAME_RATE:
                {
                    NsfLink_ChFpsParams *params;
                    params = (NsfLink_ChFpsParams *) Utils_msgGetPrm(pMsg);
                    NsfLink_SetFrameRate(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
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

    NsfLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 NsfLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    NsfLink_Obj *pObj;
    UInt32 objId;

    for (objId = 0; objId < NSF_LINK_OBJ_MAX; objId++)
    {
        pObj = &gNsfLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));
        
        pObj->linkId = SYSTEM_LINK_ID_NSF_0 + objId;
        
        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NsfLink_getFullFrames;
        linkObj.linkPutEmptyFrames = NsfLink_putEmptyFrames;
        linkObj.getLinkInfo = NsfLink_getInfo;

        UTILS_SNPRINTF(pObj->name, "NSF%d", objId);

        System_registerLink( pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 NsfLink_tskMain,
                                 NSF_LINK_TSK_PRI,
                                 gNsfLink_tskStack[objId],
                                 NSF_LINK_TSK_STACK_SIZE, pObj, pObj->name);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 NsfLink_deInit()
{
    UInt32 objId;
    NsfLink_Obj *pObj;

    for (objId = 0; objId < NSF_LINK_OBJ_MAX; objId++)
    {
        pObj = &gNsfLink_obj[objId];

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

Int32 NsfLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    NsfLink_Obj *pObj = (NsfLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 NsfLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                            FVID2_FrameList * pFrameList)
{
    NsfLink_Obj *pObj = (NsfLink_Obj *) pTsk->appData;
    Int32 status;

    if (queId < NSF_LINK_MAX_OUT_QUE)
        status =
            Utils_bufGetFull(&pObj->bufOutQue[queId], pFrameList, BIOS_NO_WAIT);
    else
        status =
            Utils_bufGetFull(&pObj->bufEvenFieldOutQue, pFrameList,
                             BIOS_NO_WAIT);

    return status;
}

Int32 NsfLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList)
{
    NsfLink_Obj *pObj = (NsfLink_Obj *) pTsk->appData;

    if (queId < NSF_LINK_MAX_OUT_QUE)
        NsfLink_drvPutEmptyFrames(pObj, pFrameList);

    return FVID2_SOK;
}

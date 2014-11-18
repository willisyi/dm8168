/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "grpxLink_priv.h"
#include "mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h"


#pragma DATA_ALIGN(gGrpxLink_tskStack, 32)
#pragma DATA_SECTION(gGrpxLink_tskStack, ".bss:taskStackSection")
UInt8 gGrpxLink_tskStack[GRPX_LINK_OBJ_MAX][GRPX_LINK_TSK_STACK_SIZE];

GrpxLink_Obj gGrpxLink_obj[GRPX_LINK_OBJ_MAX];

Void GrpxLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    GrpxLink_Obj *pObj = (GrpxLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = GrpxLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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
                status = GrpxLink_drvStart(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SYSTEM_CMD_STOP:
                status = GrpxLink_drvStop(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case GRPX_LINK_CMD_DO_DEQUE:
                Utils_tskAckOrFreeMsg(pMsg, status);
                GrpxLink_drvProcessFrames(pObj);
                break;

            case GRPX_LINK_CMD_SET_DYNAMIC_PARAMS:
                status = GrpxLink_drvSetDynamicParams(pObj, Utils_msgGetPrm(pMsg));
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case GRPX_LINK_CMD_GET_BUFFER_INFO:
                status = GrpxLink_drvGetBufferInfo(pObj, Utils_msgGetPrm(pMsg));
                Utils_tskAckOrFreeMsg(pMsg, status);
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

    GrpxLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 GrpxLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 grpxId;
    GrpxLink_Obj *pObj;
    char tskName[32];

    for (grpxId = 0; grpxId < GRPX_LINK_OBJ_MAX; grpxId++)
    {
        pObj = &gGrpxLink_obj[grpxId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId = SYSTEM_LINK_ID_GRPX_0 + grpxId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.getLinkInfo = NULL;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "GRPX%d", grpxId);

        status = Utils_tskCreate(&pObj->tsk,
                                 GrpxLink_tskMain,
                                 GRPX_LINK_TSK_PRI,
                                 gGrpxLink_tskStack[grpxId],
                                 GRPX_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 GrpxLink_deInit()
{
    UInt32 grpxId;

    for (grpxId = 0; grpxId < GRPX_LINK_OBJ_MAX; grpxId++)
    {
        Utils_tskDelete(&gGrpxLink_obj[grpxId].tsk);
    }
    return FVID2_SOK;
}

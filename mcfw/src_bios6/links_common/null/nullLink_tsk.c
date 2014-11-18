/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "nullLink_priv.h"

#pragma DATA_ALIGN(gNullLink_tskStack, 32)
#pragma DATA_SECTION(gNullLink_tskStack, ".bss:taskStackSection")
UInt8 gNullLink_tskStack[NULL_LINK_OBJ_MAX][NULL_LINK_TSK_STACK_SIZE];

NullLink_Obj gNullLink_obj[NULL_LINK_OBJ_MAX];

Int32 NullLink_drvCreate(NullLink_Obj * pObj, NullLink_CreateParams * pPrm)
{
    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));
    UTILS_assert(pObj->createArgs.numInQue < NULL_LINK_MAX_IN_QUE);

    pObj->receviedFrameCount = 0;

#ifdef SYSTEM_DEBUG_NULL
    Vps_printf(" %d: NULL   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 NullLink_drvProcessFrames(NullLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    FVID2_FrameList frameList;
    UInt32 queId;

    for (queId = 0; queId < pObj->createArgs.numInQue; queId++)
    {
        pInQueParams = &pObj->createArgs.inQueParams[queId];

        System_getLinksFullFrames(pInQueParams->prevLinkId,
                                  pInQueParams->prevLinkQueId, &frameList);

        if (frameList.numFrames)
        {
#ifdef SYSTEM_DEBUG_NULL_RT
            Vps_printf(" %d: NULL   : Received and returning %d frames !!!\n",
                       Utils_getCurTimeInMsec(), frameList.numFrames);
#endif

            pObj->receviedFrameCount += frameList.numFrames;

            System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                       pInQueParams->prevLinkQueId, &frameList);
        }
    }
    return FVID2_SOK;
}

Void NullLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    NullLink_Obj *pObj = (NullLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = NullLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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

                NullLink_drvProcessFrames(pObj);
                break;
            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

#ifdef SYSTEM_DEBUG_NULL
    Vps_printf(" %d: NULL   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 NullLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 nullId;
    NullLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    for (nullId = 0; nullId < NULL_LINK_OBJ_MAX; nullId++)
    {
        pObj = &gNullLink_obj[nullId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId, SYSTEM_LINK_ID_NULL_0) + nullId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.getLinkInfo = NULL;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "NULL%d", nullId);

        status = Utils_tskCreate(&pObj->tsk,
                                 NullLink_tskMain,
                                 NULL_LINK_TSK_PRI,
                                 gNullLink_tskStack[nullId],
                                 NULL_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 NullLink_deInit()
{
    UInt32 nullId;

    for (nullId = 0; nullId < NULL_LINK_OBJ_MAX; nullId++)
    {
        Utils_tskDelete(&gNullLink_obj[nullId].tsk);
    }
    return FVID2_SOK;
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "displayLink_priv.h"
#include "mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h"


#pragma DATA_ALIGN(gDisplayLink_tskStack, 32)
#pragma DATA_SECTION(gDisplayLink_tskStack, ".bss:taskStackSection")
UInt8 gDisplayLink_tskStack[DISPLAY_LINK_OBJ_MAX][DISPLAY_LINK_TSK_STACK_SIZE];

DisplayLink_Obj gDisplayLink_obj[DISPLAY_LINK_OBJ_MAX];

Int32 DisplayLink_tskRun(DisplayLink_Obj * pObj, Utils_TskHndl * pTsk,
                         Utils_MsgHndl ** pMsg, Bool * done, Bool * ackMsg)
{
    Int32 status = FVID2_SOK;
    Bool runDone, runAckMsg;
    Utils_MsgHndl *pRunMsg;
    UInt32 cmd;
    DisplayLink_RtParams * params;


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
            case DISPLAY_LINK_CMD_DO_DEQUE:
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                status = DisplayLink_drvProcessData(pObj);
                if (status != FVID2_SOK)
                {
                    runDone = TRUE;
                    runAckMsg = TRUE;
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

            case DISPLAY_LINK_CMD_SWITCH_CH:
                status =
                    DisplayLink_drvSwitchCh(pObj,
                                            Utils_msgGetPrm(pRunMsg));
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case DISPLAY_LINK_CMD_PRINT_STATISTICS:
                DisplayLink_drvPrintStatistics(pObj);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case DISPLAY_LINK_CMD_PRINT_BUFFER_STATISTICS:
                DisplayLink_printBufferStatus(pObj);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case DISPLAY_LINK_CMD_SWITCH_INPUT_MODE:
                status = DisplayLink_drvSwitchInputMode(
                                pObj,
                                Utils_msgGetPrm(pRunMsg)
                            );
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case DISPLAY_LINK_CMD_SWITCH_ACTIVE_QUEUE:
                status =
                    DisplayLink_drvSwitchActiveQueue(pObj,
                                                     Utils_msgGetPrm(pRunMsg));
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
            case DISPLAY_LINK_CMD_CHANGE_RESOLUTION:
                params = (DisplayLink_RtParams *) Utils_msgGetPrm(pRunMsg);
                System_getOutSize(params->resolution,
                                  &pObj->displayFormat.width,
                                  &pObj->displayFormat.height);

                DisplayLink_drvSetFmt(pObj, &pObj->displayFormat);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case DISPLAY_LINK_CMD_STOP_DRV:
                DisplayLink_drvStop(pObj);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case DISPLAY_LINK_CMD_START_DRV:
                status = DisplayLink_drvStart(pObj);
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            default:
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
        }

    }

    DisplayLink_drvStop(pObj);

    if (runAckMsg)
        Utils_tskAckOrFreeMsg(pRunMsg, status);

    return status;
}

Void DisplayLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    DisplayLink_Obj *pObj = (DisplayLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = DisplayLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

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
                status = DisplayLink_drvStart(pObj);

                Utils_tskAckOrFreeMsg(pMsg, status);
                if (status == FVID2_SOK)
                {
                    status =
                        DisplayLink_tskRun(pObj, pTsk, &pMsg, &done, &ackMsg);
                }

                break;
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;
            case DISPLAY_LINK_CMD_SWITCH_CH:
                status =
                    DisplayLink_drvSwitchCh(pObj,
                                            Utils_msgGetPrm(pMsg));
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            case DISPLAY_LINK_CMD_SWITCH_ACTIVE_QUEUE:
                status =
                    DisplayLink_drvSwitchActiveQueue(pObj,
                                                     Utils_msgGetPrm(pMsg));
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case DISPLAY_LINK_CMD_SWITCH_INPUT_MODE:
                status = DisplayLink_drvSwitchInputMode(
                                pObj,
                                Utils_msgGetPrm(pMsg)
                            );
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    DisplayLink_drvDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 DisplayLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 displayId;
    DisplayLink_Obj *pObj;
    char tskName[32];

    for (displayId = 0; displayId < DISPLAY_LINK_OBJ_MAX; displayId++)
    {
        pObj = &gDisplayLink_obj[displayId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId = SYSTEM_LINK_ID_DISPLAY_0 + displayId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.getLinkInfo = NULL;

        System_registerLink(pObj->tskId, &linkObj);

        sprintf(tskName, "DISPLAY%d", displayId);

        status = Utils_tskCreate(&pObj->tsk,
                                 DisplayLink_tskMain,
                                 DISPLAY_LINK_TSK_PRI,
                                 gDisplayLink_tskStack[displayId],
                                 DISPLAY_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 DisplayLink_deInit()
{
    UInt32 displayId;

    for (displayId = 0; displayId < DISPLAY_LINK_OBJ_MAX; displayId++)
    {
        Utils_tskDelete(&gDisplayLink_obj[displayId].tsk);
    }
    return FVID2_SOK;
}

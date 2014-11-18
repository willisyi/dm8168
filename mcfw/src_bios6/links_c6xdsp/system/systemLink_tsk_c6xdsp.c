/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "systemLink_priv_c6xdsp.h"

#pragma DATA_ALIGN(gSystemLink_tskStack, 32)
#pragma DATA_SECTION(gSystemLink_tskStack, ".bss:taskStackSection")
UInt8 gSystemLink_tskStack[SYSTEM_TSK_STACK_SIZE];

SystemLink_Obj gSystemLink_obj;

Int32 SystemLink_cmdHandler(SystemLink_Obj * pObj, UInt32 cmd, Void * pPrm)
{
    Int32 status = FVID2_SOK;

    switch (cmd)
    {
        case SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START:
            Utils_prfLoadCalcStart();
            break;

        case SYSTEM_COMMON_CMD_CPU_LOAD_CALC_STOP:
            Utils_prfLoadCalcStop();
            break;

        case SYSTEM_COMMON_CMD_CPU_LOAD_CALC_RESET:
            Utils_prfLoadCalcReset();
            break;

        case SYSTEM_COMMON_CMD_PRINT_STATUS:
        {
            SystemCommon_PrintStatus *prm = (SystemCommon_PrintStatus *) pPrm;

            if (prm->printCpuLoad)
            {
                status = Utils_prfLoadPrintAll(prm->printTskLoad);
            }
            if (prm->printHeapStatus)
            {
                System_memPrintHeapStatus();
            }
        }
            break;

        case SYSTEM_COMMON_CMD_CORE_STATUS:
            Vps_printf(" %d: Core is active\n",Utils_getCurTimeInMsec());
            break;
        default:
            break;
    }

    return status;
}

Void SystemLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    Int32 status;
    SystemLink_Obj *pObj = (SystemLink_Obj *) pTsk->appData;

    status = SystemLink_cmdHandler(pObj,
                                   Utils_msgGetCmd(pMsg),
                                   Utils_msgGetPrm(pMsg));
    Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 SystemLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    SystemLink_Obj *pObj;
    char tskName[32];

    pObj = &gSystemLink_obj;

    memset(pObj, 0, sizeof(*pObj));

    pObj->tskId = SYSTEM_LINK_ID_M3VPSS;

    linkObj.pTsk = &pObj->tsk;
    linkObj.linkGetFullFrames = NULL;
    linkObj.linkPutEmptyFrames = NULL;
    linkObj.getLinkInfo = NULL;

    System_registerLink(pObj->tskId, &linkObj);

    sprintf(tskName, "SYSTEM_DSP%d", pObj->tskId);

    status = Utils_tskCreate(&pObj->tsk,
                             SystemLink_tskMain,
                             SYSTEM_TSK_PRI,
                             gSystemLink_tskStack,
                             SYSTEM_TSK_STACK_SIZE, pObj, tskName);
    UTILS_assert(status == FVID2_SOK);

    return status;
}

Int32 SystemLink_deInit()
{
    Utils_tskDelete(&gSystemLink_obj.tsk);

    return FVID2_SOK;
}

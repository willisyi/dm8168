/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <xdc/std.h>
#include <mcfw/interfaces/link_api/system_tiler.h>
#include <mcfw/src_bios6/utils/utils_tiler_allocator.h>
#include "systemLink_priv_m3vpss.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>


#pragma DATA_ALIGN(gSystemLink_tskStack, 32)
#pragma DATA_SECTION(gSystemLink_tskStack, ".bss:taskStackSection")
UInt8 gSystemLink_tskStack[SYSTEM_TSK_STACK_SIZE];

SystemLink_Obj gSystemLink_obj;

Int32 SystemLink_cmdHandler(SystemLink_Obj * pObj, UInt32 cmd, Void * pPrm)
{
    Int32 status = FVID2_SOK;

    switch (cmd)
    {
        case SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES:
#ifdef SYSTEM_USE_VIDEO_DECODER
            status = System_videoResetVideoDevices();
#endif
            break;

        case SYSTEM_M3VPSS_CMD_GET_PLATFORM_INFO:
        {
            SystemVpss_PlatformInfo *prm = (SystemVpss_PlatformInfo *) pPrm;

            prm->cpuRev = Vps_platformGetCpuRev();
            prm->boardId = System_getBoardId();
            prm->baseBoardRev = System_getBaseBoardRev();
            prm->dcBoardRev = System_getDcBoardRev();
        }
            break;

        case SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_INIT:
        {
            VDIS_PARAMS_S * prm = (VDIS_PARAMS_S *) pPrm;

            status = System_displayCtrlInit(prm);
        }
            break;

        case SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_DEINIT:
        {
            VDIS_PARAMS_S * prm = (VDIS_PARAMS_S *) pPrm;
            System_displayCtrlDeInit(prm);
        }
            break;

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

        case SYSTEM_COMMON_CMD_TILER_ALLOC:
        {
            SystemCommon_TilerAlloc *tilerAlloc =
                (SystemCommon_TilerAlloc *) pPrm;
            UInt32 utilsTilerCntMode =
                (tilerAlloc->cntMode - SYSTEM_TILER_CNT_FIRST) +
                UTILS_TILER_CNT_FIRST;

            if (((Int32) utilsTilerCntMode >= UTILS_TILER_CNT_FIRST)
                && (utilsTilerCntMode <= UTILS_TILER_CNT_LAST))
            {
                tilerAlloc->tileAddr =
                    Utils_tilerAllocatorAlloc(utilsTilerCntMode,
                                              tilerAlloc->width,
                                              tilerAlloc->height);
                if (UTILS_TILER_INVALID_ADDR ==
                    tilerAlloc->tileAddr)
                {
                    tilerAlloc->tileAddr = SYSTEM_TILER_INVALID_ADDR;
                }
            }
            else
            {
                tilerAlloc->tileAddr = SYSTEM_TILER_INVALID_ADDR;

            }
            break;
        }
        case SYSTEM_COMMON_CMD_TILER_FREE:
        {
            SystemCommon_TilerFree *tilerFree = (SystemCommon_TilerFree *) pPrm;

            status = Utils_tilerAllocatorFree(tilerFree->tileAddr);

            break;
        }

        case SYSTEM_COMMON_CMD_TILER_FREE_ALL:
        {
            status = Utils_tilerAllocatorFreeAll();

            break;
        }

        case SYSTEM_COMMON_CMD_TILER_DISABLE_ALLOCATOR:
        {
            status = Utils_tilerAllocatorDisable();

            break;
        }
        case SYSTEM_COMMON_CMD_TILER_ENABLE_ALLOCATOR:
        {
            status = Utils_tilerAllocatorEnable();

            break;
        }

        case SYSTEM_COMMON_CMD_TILER_IS_ALLOCATOR_DISABLED:
        {
            SystemCommon_TilerIsDisabled *tilerAllocatorStatus =
              (SystemCommon_TilerIsDisabled *) pPrm;

            tilerAllocatorStatus->isAllocatorDisabled =
            Utils_tilerAllocatorIsDisabled();

            status = 0;
            break;
        }
        case SYSTEM_COMMON_CMD_TILER_ALLOC_RAW:
        {
            SystemCommon_TilerAllocRaw *tilerRawAlloc =
                (SystemCommon_TilerAllocRaw *) pPrm;

            tilerRawAlloc->allocAddr  =
            (UInt32) Utils_tilerAllocatorAllocRaw(tilerRawAlloc->size,
                                                  tilerRawAlloc->align);
            break;
        }
        case SYSTEM_COMMON_CMD_TILER_FREE_RAW:
        {
            SystemCommon_TilerFreeRaw *tilerRawFree =
                (SystemCommon_TilerFreeRaw *) pPrm;

            Utils_tilerAllocatorFreeRaw((Ptr)tilerRawFree->allocAddr,
                                        tilerRawFree->size);

            break;
        }
        case SYSTEM_COMMON_CMD_TILER_GET_FREE_SIZE:
        {
            SystemCommon_TilerGetFreeSize *tilerFreeSize =
                (SystemCommon_TilerGetFreeSize *) pPrm;

            tilerFreeSize->freeSizeRaw = Utils_tilerAllocatorGetFreeSizeRaw();
            tilerFreeSize->freeSize8b  = Utils_tilerAllocatorGetFreeSize(UTILS_TILER_CNT_8BIT);
            tilerFreeSize->freeSize16b = Utils_tilerAllocatorGetFreeSize(UTILS_TILER_CNT_16BIT);
            tilerFreeSize->freeSize32b = Utils_tilerAllocatorGetFreeSize(UTILS_TILER_CNT_32BIT);

            break;
        }
        case SYSTEM_M3VPSS_CMD_SET_DISPLAYCTRL_VENC_OUTPUT:
        {
            VDIS_DEV_PARAM_S * prm = (VDIS_DEV_PARAM_S *) pPrm;

            status = System_displayCtrlSetVencOutput(prm);
            break;
        }

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

    sprintf(tskName, "SYSTEM_M3VPSS%d", pObj->tskId);

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

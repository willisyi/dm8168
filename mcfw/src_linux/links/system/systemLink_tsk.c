/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "systemLink_priv.h"
#include "system_priv_ipc.h"
#include <mcfw/interfaces/link_api/systemLink_common.h>

#define STRINGIFY(x)           #x
#define SYMBOL_NAME2STRING(x)  STRINGIFY(x)

SystemLink_Obj gSystemLink_obj;


Int32 System_linkGetCoreStatus(VSYS_CORE_STATUS_TBL_S *coreStatusTbl)
{
    Int32 status;
    UInt32 waitTime;  /* timein Micro Seconds */
    UInt32 timeOut = SYSTEM_LINK_CORE_STATUS_CHECK_TIMEOUT_MS;

    coreStatusTbl->numSlaveCores = 0;

    waitTime = timeOut;

    status = System_linkControlWithTimeout(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_COMMON_CMD_CORE_STATUS,
        NULL,
        0,
        TRUE,
        waitTime
        );
    OSA_assert(coreStatusTbl->numSlaveCores < OSA_ARRAYSIZE(coreStatusTbl->coreStatus));
    coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].coreId =  VSYS_CORE_SLAVE_VPSS_M3;
    if (!(OSA_ISERROR(status)))
    {
        coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].isAlive = TRUE;
    }
    else
    {
        coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].isAlive = FALSE;
    }
    coreStatusTbl->numSlaveCores++;

    status = System_linkControlWithTimeout(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_CORE_STATUS,
        NULL,
        0,
        TRUE,
        waitTime
        );

    OSA_assert(coreStatusTbl->numSlaveCores < OSA_ARRAYSIZE(coreStatusTbl->coreStatus));
    coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].coreId =  VSYS_CORE_SLAVE_VIDEO_M3;
    if (!(OSA_ISERROR(status)))
    {
        coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].isAlive = TRUE;
    }
    else
    {
        coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].isAlive = FALSE;
    }
    coreStatusTbl->numSlaveCores++;

#if !defined(TI_8107_BUILD)
    status = System_linkControlWithTimeout(
        SYSTEM_LINK_ID_DSP,
        SYSTEM_COMMON_CMD_CORE_STATUS,
        NULL,
        0,
        TRUE,
        waitTime
        );
    OSA_assert(coreStatusTbl->numSlaveCores < OSA_ARRAYSIZE(coreStatusTbl->coreStatus));
    coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].coreId =  VSYS_CORE_SLAVE_C674;
    if (!(OSA_ISERROR(status)))
    {
        coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].isAlive = TRUE;
    }
    else
    {
        coreStatusTbl->coreStatus[coreStatusTbl->numSlaveCores].isAlive = FALSE;
    }
    coreStatusTbl->numSlaveCores++;
#endif

    return OSA_SOK;
}
static Void SystemLink_ipcNotifyCbDSP(OSA_TskHndl *pTsk)
{
    OSA_tskSendMsg(pTsk, NULL, SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_DSP, NULL, 0);
}

static Void SystemLink_ipcNotifyCbVideoM3(OSA_TskHndl *pTsk)
{
    OSA_tskSendMsg(pTsk, NULL, SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_VIDEOM3, NULL, 0);
}


static Void SystemLink_ipcNotifyCbVpssM3(OSA_TskHndl *pTsk)
{
    OSA_tskSendMsg(pTsk, NULL, SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_VPSSM3, NULL, 0);
}


static
Char * SystemLink_mapSystemProcId2MultiProcCoreName(UInt16 systemProcId)
{
    Char *multiProcName = NULL;

    switch(systemProcId)
    {
        case SYSTEM_PROC_DSP:
            multiProcName = "DSP";
            break;
        case SYSTEM_PROC_M3VIDEO:
            multiProcName = "VIDEO-M3";
            break;
        case SYSTEM_PROC_M3VPSS:
            multiProcName = "VPSS-M3";
            break;
        case SYSTEM_PROC_HOSTA8:
            multiProcName = "HOST";
            break;
        default:
            UTILS_assert(FALSE);
            break;
    }
    return multiProcName;
}

static
Void  SystemLink_copySlaveCoreExceptionContext(UInt16 multiProcId,
                                               VSYS_SLAVE_CORE_EXCEPTION_INFO_S *excInfo)
{
    Int32 status;

    fflush(stdout);
    printf("%s:%d\n",__func__,__LINE__);
    fflush(stdout);
    status =
    System_ipcCopySlaveCoreSymbolContents(SYMBOL_NAME2STRING(VSYS_EXCEPTION_CONTEXT_SYMBOL_NAME),
                                    multiProcId,
                                    excInfo,
                                    sizeof(*excInfo));
    printf("%s:%d\n",__func__,__LINE__);
    fflush(stdout);
    OSA_assert(status == OSA_SOK);

}


VSYS_SLAVE_CORE_EXCEPTION_INFO_S gSlaveCoreExcInfo;

static
Void  SystemLink_handleSlaveCoreException(SystemLink_Obj *pObj,Char *multiProcName)
{
    UInt16 multiProcId;

    printf("\n\n%d:!!!SLAVE CORE [%s] DOWN!!!\n",OSA_getCurTimeInMsec(),multiProcName);
    OSA_assert(multiProcName != NULL);
    multiProcId = MultiProc_getId(multiProcName);
    SystemLink_copySlaveCoreExceptionContext(multiProcId,&gSlaveCoreExcInfo);
    if(pObj->eventHandler!=NULL)
    {
        printf("%s:%d\n",__func__,__LINE__);
        pObj->eventHandler(
                VSYS_EVENT_SLAVE_CORE_EXCEPTION,
                &gSlaveCoreExcInfo,
                pObj->eventHandlerAppData);
        printf("%s:%d\n",__func__,__LINE__);
    }
}


Int32 SystemLink_tskMain(struct OSA_TskHndl *pTsk, OSA_MsgHndl *pMsg, Uint32 curState)
{
    Int32 status = OSA_EFAIL;
    SystemLink_Obj *pObj = (SystemLink_Obj*)pTsk->appData;
    Uint16 cmd = OSA_msgGetCmd(pMsg);

    if (SYSTEM_LINK_IS_SLAVE_CORE_EXCEPTION(cmd))
    {
        Char *multiProcName;
        UInt systemProcId = SYSTEM_LINK_SLAVE_CORE_EXCEPTION_MAP_CMD_TO_SYSTEMPROCID(cmd);

        OSA_assert(systemProcId < SYSTEM_PROC_MAX);
        multiProcName = SystemLink_mapSystemProcId2MultiProcCoreName(systemProcId);
        SystemLink_handleSlaveCoreException(pObj,multiProcName);
    }
    else
    {
        if(pObj->eventHandler!=NULL)
        {
            status = pObj->eventHandler(
                    OSA_msgGetCmd(pMsg),
                    OSA_msgGetPrm(pMsg),
                    pObj->eventHandlerAppData
                    );
        }
    }

    OSA_tskAckOrFreeMsg(pMsg, status);

    return 0;
}

static Void SystemLink_registerExceptionNotifyHandler(System_LinkObj  *linkObj)
{
    System_registerLink(SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_DSP, linkObj);
    System_ipcRegisterNotifyCb(SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_DSP,SystemLink_ipcNotifyCbDSP);

    System_registerLink(SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_VIDEOM3, linkObj);
    System_ipcRegisterNotifyCb(SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_VIDEOM3,SystemLink_ipcNotifyCbVideoM3);

    System_registerLink(SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_VPSSM3, linkObj);
    System_ipcRegisterNotifyCb(SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_VPSSM3,SystemLink_ipcNotifyCbVpssM3);
}

Int32 SystemLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    SystemLink_Obj  *pObj;
    char tskName[32];

    pObj = &gSystemLink_obj;

    memset(pObj, 0, sizeof(*pObj));

    pObj->tskId = SYSTEM_LINK_ID_HOST;

    linkObj.pTsk = &pObj->tsk;
    linkObj.getLinkInfo = NULL;

    pObj->tsk.appData = pObj;

    System_registerLink(pObj->tskId, &linkObj);

    SystemLink_registerExceptionNotifyHandler(&linkObj);

    sprintf(tskName, "SYSTEM_HOST%d", pObj->tskId);


    status = OSA_tskCreate(
                &pObj->tsk,
                SystemLink_tskMain,
                SYSTEM_TSK_PRI,
                SYSTEM_TSK_STACK_SIZE,
                0,
                pObj);
    UTILS_assert(  status==OSA_SOK);

    return status;
}


Int32 SystemLink_deInit()
{
    OSA_tskDelete(&gSystemLink_obj.tsk);

    return OSA_SOK;
}

Int32 Vsys_registerEventHandler(VSYS_EVENT_HANDLER_CALLBACK callback, Ptr appData)
{
    gSystemLink_obj.eventHandlerAppData = appData;
    gSystemLink_obj.eventHandler = callback;

    return 0;
}

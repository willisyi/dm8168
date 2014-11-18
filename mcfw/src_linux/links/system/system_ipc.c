/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/



#include "system_priv_ipc.h"
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <ti/syslink/SysLink.h>
#include <ti/syslink/ProcMgr.h>

System_IpcObj gSystem_ipcObj;

UInt32 gSystem_ipcEnableProcId[] =
        {
        SYSTEM_PROC_HOSTA8,
#if defined(TI_8107_BUILD)
        SYSTEM_PROC_INVALID,
#else
        SYSTEM_PROC_DSP,
#endif
        SYSTEM_PROC_M3VIDEO,
        SYSTEM_PROC_M3VPSS,
        SYSTEM_PROC_MAX /* Last entry */
};

Int32 System_ipcInit()
{
    printf(" %u: SYSTEM: IPC init in progress !!!\n", OSA_getCurTimeInMsec());

    SysLink_setup ();

    System_ipcMsgQInit();

    System_ipcNotifyInit();


    printf(" %u: SYSTEM: IPC init DONE !!!\n", OSA_getCurTimeInMsec());

    return OSA_SOK;
}

Int32 System_ipcDeInit()
{
    printf(" %u: SYSTEM: IPC de-init in progress !!!\n", OSA_getCurTimeInMsec());

    System_ipcNotifyDeInit();

    System_ipcMsgQDeInit();

    SysLink_destroy ();

    printf(" %u: SYSTEM: IPC de-init DONE !!!\n", OSA_getCurTimeInMsec());

    return OSA_SOK;
}

UInt32 System_getSelfProcId()
{
    return MultiProc_self();
}

Int32 System_ipcGetSlaveCoreSymbolAddress(char *symbolName,UInt16 multiProcId,
                                    UInt32 *symAddressPtr)
{
    Int32 status;
    ProcMgr_Handle procMgrHandle = NULL;
    UInt32 symbolAddress = 0;
    UInt32 fileId;
    //UInt32 oldMask;
    //UInt32 traceClass = 3;
    //extern UInt32 _GT_setTrace (UInt32 mask, GT_TraceType type);

    *symAddressPtr = 0;
    status = ProcMgr_open (&procMgrHandle, multiProcId);
    //printf("ProcMgr_open status:%d\n",status);
    OSA_assert(procMgrHandle != NULL);

    fileId = ProcMgr_getLoadedFileId (procMgrHandle);
    //printf("Getting Address for symbol:%s\n",symbolName);
    /* Temporarily enable full trace */
    //oldMask = _GT_setTrace ((GT_TraceState_Enable | GT_TraceEnter_Enable |
    //                        GT_TraceSetFailure_Enable |
    //                        (traceClass << (32 - GT_TRACECLASS_SHIFT))),
    //                        GT_TraceType_Kernel);
    status = ProcMgr_getSymbolAddress(procMgrHandle,
                                      fileId,
                                      symbolName,
                                      &symbolAddress);
    //_GT_setTrace(oldMask, GT_TraceType_Kernel);
    //printf("Got Address for symbol:0x%x\n",symbolAddress);
    OSA_assert(symbolAddress != 0);
    *symAddressPtr = symbolAddress;
    status = ProcMgr_close(&procMgrHandle);
    OSA_assert(status > 0);
    return OSA_SOK;
}

Int32 System_ipcCopySlaveCoreSymbolContents(char *symbolName,
                                      UInt16 multiProcId,
                                      Ptr  dstPtr,
                                      UInt32 copySize)
{
    UInt32 symbolAddress;
    Int32 status = OSA_SOK;
    Ptr   symbolAddressMapped;

    status =
    System_ipcGetSlaveCoreSymbolAddress(symbolName,multiProcId,&symbolAddress);
    OSA_assert(OSA_SOK  == status);

    status = OSA_mapMem(symbolAddress,copySize,&symbolAddressMapped);
    OSA_assert((status == 0) && (symbolAddressMapped != NULL));

    //printf("%s:%d\n",__func__,__LINE__);
    memcpy(dstPtr,symbolAddressMapped,copySize);
    //printf("%s:%d\n",__func__,__LINE__);
    status = OSA_unmapMem(symbolAddressMapped,copySize);
    //printf("%s:%d\n",__func__,__LINE__);
    OSA_assert(status == 0);

    return status;
}

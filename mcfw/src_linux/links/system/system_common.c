/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"
#include "system_priv_ipc.h"
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/avsync_hlos.h>

System_CommonObj gSystem_objCommon;


Int32 System_init()
{
    Int32         status;

    #ifdef SYSTEM_DEBUG
    printf ( " %u: SYSTEM: System Common Init in progress !!!\n", OSA_getCurTimeInMsec());
    #endif

    memset(&gSystem_objCommon, 0, sizeof(gSystem_objCommon));
    
    System_ipcInit();

    IpcBitsInLink_init();
    IpcBitsOutLink_init();
    IpcFramesInLink_init();
    IpcFramesOutLink_init();

    AvsyncLink_init();
    status = OSA_mbxCreate(&gSystem_objCommon.mbx);
    UTILS_assert(  status==OSA_SOK);

    SystemLink_init();

    #ifdef SYSTEM_DEBUG
    printf ( " %u: SYSTEM: System Common Init Done !!!\n", OSA_getCurTimeInMsec());
    #endif

    return status;

}

Int32 System_deInit()
{
    #ifdef SYSTEM_DEBUG
    printf ( " %u: SYSTEM: System Common De-Init in progress !!!\n", OSA_getCurTimeInMsec());
    #endif

    AvsyncLink_deInit();
    SystemLink_deInit();

    IpcBitsInLink_deInit();
    IpcBitsOutLink_deInit();
    IpcFramesInLink_deInit();
    IpcFramesOutLink_deInit();

    System_ipcDeInit();

    OSA_mbxDelete(&gSystem_objCommon.mbx);

    #ifdef SYSTEM_DEBUG
    printf ( " %u: SYSTEM: System Common De-Init Done !!!\n", OSA_getCurTimeInMsec());
    #endif

    return OSA_SOK;
}


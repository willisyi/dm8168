/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IPC_IN_M3_LINK_PRIV_H_
#define _IPC_IN_M3_LINK_PRIV_H_

#include <mcfw/src_bios6/links_common/system/system_priv_ipc.h>

#define IPC_IN_M3_LINK_OBJ_MAX   (2)

/**
 *  @def   IPC_M3IN_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_M3IN_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_M3IN_LINK_DONE_PERIOD_MS
 *  @brief IPC M3 In periodic frame release in ms.
 */
#define IPC_M3IN_LINK_DONE_PERIOD_MS    (16u)

typedef struct IpcInM3Link_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} IpcInM3Link_periodicObj;

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    IpcLink_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    ListMP_Handle listMPInHndl;

    FVID2_FrameList freeFrameList;

    Utils_QueHandle outFrameQue;
    FVID2_Frame *outFrameQueMem[SYSTEM_IPC_M3_MAX_LIST_ELEM];

    System_LinkInfo inQueInfo;

    IpcInM3Link_periodicObj prd;

} IpcInM3Link_Obj;

#endif

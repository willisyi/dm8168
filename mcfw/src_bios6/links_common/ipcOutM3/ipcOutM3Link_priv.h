/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IPC_OUT_M3_LINK_PRIV_H_
#define _IPC_OUT_M3_LINK_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/links_common/system/system_priv_ipc.h>

#define IPC_OUT_M3_LINK_OBJ_MAX   (2)

#define IPC_OUTM3_LINK_MAX_CH     (64)

/**
 *  @def   IPC_M3OUT_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_M3OUT_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_M3OUT_LINK_DONE_PERIOD_MS
 *  @brief IPC M3 out periodic frame release in ms.
 */
#define IPC_M3OUT_LINK_DONE_PERIOD_MS    (16u)

typedef struct IpcOutM3Link_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} IpcOutM3Link_periodicObj;


typedef struct {
    UInt32 inFrameRecvCount;
    /**< input frame recevied from previous link */

    UInt32 inFrameUserSkipCount;
    /**< input frame rejected due mismatch in FID */

    UInt32 inFrameProcessCount;
    /**< input frames actually processed */

    Utils_frameSkipContext frameSkipCtx;
    /**< Data structure for frame skip to achieve expected output frame rate */    
} IpcOutM3Link_ChObj;

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    IpcOutM3Link_ChObj chObj[IPC_OUTM3_LINK_MAX_CH];
	
    IpcLink_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    GateMP_Handle gateMPOutHndl;
    ListMP_Handle listMPInHndl;
    GateMP_Handle gateMPInHndl;
    SystemIpcM3_ListElem *listElem[SYSTEM_IPC_M3_MAX_LIST_ELEM];

    Utils_QueHandle listElemQue;
    SystemIpcM3_ListElem *listElemQueMem[SYSTEM_IPC_M3_MAX_LIST_ELEM];

    FVID2_FrameList freeFrameList;

    System_LinkInfo inQueInfo;

    IpcOutM3Link_periodicObj prd;

    UInt32 numCh;
    UInt32 statsStartTime;
    UInt32 totalFrameCount;
} IpcOutM3Link_Obj;

#endif

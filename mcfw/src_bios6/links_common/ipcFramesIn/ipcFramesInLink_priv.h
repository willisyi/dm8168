/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IPC_FRAMES_IN_LINK_PRIV_H_
#define _IPC_FRAMES_IN_LINK_PRIV_H_

#include <xdc/std.h>
#include <ti/sysbios/knl/Clock.h>
#include <mcfw/src_bios6/utils/utils_trace.h>
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/vidframe.h>
#include <mcfw/src_bios6/links_common/system/system_priv_ipc.h>
#include <mcfw/src_bios6/utils/utils_prf.h>

#define IPC_FRAMES_IN_LINK_OBJ_MAX   (2)

/**
 *  @def   IPC_FRAMES_IN_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_FRAMES_IN_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_FRAMES_IN_LINK_DONE_PERIOD_MS
 *  @brief IPC frameouts periodic frame release in ms.
 */
#define IPC_FRAMES_IN_LINK_DONE_PERIOD_MS    (8u)

typedef struct IpcFramesInLink_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} IpcFramesInLink_periodicObj;

typedef struct IpcFramesInStats {
    UInt32 recvCount;
    UInt32 droppedCount;
    UInt32 freeCount;
    UInt32 totalRoundTrip;
} IpcFramesInStats;

typedef enum IpcFramesInLink_State {
    IPC_FRAMES_IN_LINK_STATE_INACTIVE = 0,
    IPC_FRAMES_IN_LINK_STATE_ACTIVE
} IpcFramesInLink_State;

typedef struct IpcFramesInLink_Obj {
    UInt32 tskId;

    Utils_TskHndl tsk;

    IpcFramesInLinkRTOS_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    ListMP_Handle listMPInHndl;

    FVID2_FrameList freeBufList;

    Utils_QueHandle outFrameBufQue;
    FVID2_Frame *outFrameBufQueMem[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];
    Utils_QueHandle freeFrameQue;
    FVID2_Frame *freeFrameQueMem[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];
    FVID2_Frame freeFrameMem[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];
    System_FrameInfo freeFrameInfoMem[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];
    /* IpcFramesIn link info */
    System_LinkInfo info;

    System_LinkInfo inQueInfo;

    IpcFramesInStats stats;
    IpcFramesInLink_periodicObj prd;
    volatile IpcFramesInLink_State state;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
} IpcFramesInLink_Obj;

#ifdef SYSTEM_DEBUG_IPC
#define IPCFRAMESINLINK_INFO_LOG(linkID,...)      do {                           \
                                                     Vps_printf(               \
                                                      "\n%d: IPCFRAMESIN:Link[%x]:", \
                                                      Utils_getCurTimeInMsec(),linkID); \
                                                      Vps_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCFRAMESINLINK_INFO_LOG(...)
#endif

#endif                                                     /* _IPC_FRAMES_IN_LINK_PRIV_H_ 
                                                            */

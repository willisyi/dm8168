/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IPC_BITS_IN_LINK_PRIV_H_
#define _IPC_BITS_IN_LINK_PRIV_H_

#include <xdc/std.h>
#include <ti/sysbios/knl/Clock.h>
#include <mcfw/src_bios6/utils/utils_trace.h>
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/vidbitstream.h>
#include <mcfw/src_bios6/links_common/system/system_priv_ipc.h>
#include <mcfw/src_bios6/utils/utils_prf.h>

#define IPC_BITS_IN_LINK_OBJ_MAX   (2)

/**
 *  @def   IPC_BITS_IN_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_BITS_IN_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_BITS_IN_LINK_DONE_PERIOD_MS
 *  @brief IPC bitouts periodic frame release in ms.
 */
#define IPC_BITS_IN_LINK_DONE_PERIOD_MS    (16u)

typedef struct IpcBitsOutLink_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} IpcBitsOutLink_periodicObj;

typedef struct IpcBitsInStats {
    UInt32 recvCount;
    UInt32 droppedCount;
    UInt32 freeCount;
    UInt32 totalRoundTrip;
} IpcBitsInStats;

typedef enum IpcBitsInLink_State {
    IPC_BITS_IN_LINK_STATE_INACTIVE = 0,
    IPC_BITS_IN_LINK_STATE_ACTIVE
} IpcBitsInLink_State;

typedef struct IpcBitsInLink_Obj {
    UInt32 tskId;

    Utils_TskHndl tsk;

    IpcBitsInLinkRTOS_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    ListMP_Handle listMPInHndl;

    Bitstream_BufList freeBufList;

    Utils_QueHandle outBitBufQue;
    Bitstream_Buf *outBitBufQueMem[SYSTEM_IPC_BITS_MAX_LIST_ELEM];

    System_LinkInfo inQueInfo;

    IpcBitsInStats stats;
    IpcBitsOutLink_periodicObj prd;
    volatile IpcBitsInLink_State state;
} IpcBitsInLink_Obj;

#ifdef SYSTEM_DEBUG_IPC
#define IPCBITSINLINK_INFO_LOG(linkID,...)      do {                           \
                                                     Vps_printf(               \
                                                      "\n%d: IPCBITSIN:Link[%x]:", \
                                                      Utils_getCurTimeInMsec(),linkID); \
                                                      Vps_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCBITSINLINK_INFO_LOG(...)
#endif

#endif                                                     /* _IPC_BITS_IN_LINK_PRIV_H_ 
                                                            */

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IPC_BITS_OUT_LINK_PRIV_H_
#define _IPC_BITS_OUT_LINK_PRIV_H_

#include <mcfw/src_bios6/links_common/system/system_priv_ipc.h>
#include <mcfw/src_bios6/utils/utils_prf.h>
#include <xdc/std.h>
#include <ti/sysbios/knl/Clock.h>

#define IPC_BITS_OUT_LINK_OBJ_MAX   (2)

/**
 *  @def   IPC_BITSOUT_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_BITSOUT_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_BITSOUT_LINK_DONE_PERIOD_MS
 *  @brief IPC bitouts periodic frame release in ms.
 */
#define IPC_BITSOUT_LINK_DONE_PERIOD_MS    (16u)

typedef struct IpcBitsOutLink_statsObj {
    UInt32 recvCount;
    UInt32 droppedCount;
    UInt32 freeCount;
    UInt32 totalRoundTrip;
    Utils_PrfTsHndl *tsHandle;
} IpcBitsOutLink_statsObj;

typedef struct IpcBitsOutLink_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} IpcBitsOutLink_periodicObj;

typedef struct IpcBitsOutLink_Obj {
    UInt32 tskId;

    Utils_TskHndl tsk;

    IpcBitsOutLinkRTOS_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    GateMP_Handle gateMPOutHndl;
    ListMP_Handle listMPInHndl;
    GateMP_Handle gateMPInHndl;


    SystemIpcBits_ListElem *listElem[SYSTEM_IPC_BITS_MAX_LIST_ELEM];

    Utils_QueHandle listElemQue;
    SystemIpcBits_ListElem *listElemQueMem[SYSTEM_IPC_BITS_MAX_LIST_ELEM];

    Bitstream_Buf *listElem2BitBufMap[SYSTEM_IPC_BITS_MAX_LIST_ELEM];
    Bitstream_BufList freeBitBufList;

    System_LinkInfo inQueInfo;

    IpcBitsOutLink_statsObj stats;

    IpcBitsOutLink_periodicObj prd;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
} IpcBitsOutLink_Obj;

#ifdef SYSTEM_DEBUG_IPC
#define IPCBITSOUTLINK_INFO_LOG(linkID,...)      do {                           \
                                                     Vps_printf(               \
                                                      "\n%d: IPCBITSOUT:Link[%x]:", \
                                                      Utils_getCurTimeInMsec(),linkID); \
                                                      Vps_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCBITSOUTLINK_INFO_LOG(linkID,...)
#endif

#endif                                                     /* _IPC_BITS_OUT_LINK_PRIV_H_ 
                                                            */

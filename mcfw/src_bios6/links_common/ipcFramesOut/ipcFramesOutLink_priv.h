/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IPC_FRAMES_OUT_LINK_PRIV_H_
#define _IPC_FRAMES_OUT_LINK_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/links_common/system/system_priv_ipc.h>
#include <mcfw/src_bios6/utils/utils_prf.h>
#include <xdc/std.h>
#include <ti/sysbios/knl/Clock.h>

#define IPC_FRAMES_OUT_LINK_OBJ_MAX   (3)

#define IPC_FRAMESOUT_LINK_MAX_CH     (64)
/**
 *  @def   IPC_FRAMESOUT_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_FRAMESOUT_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_FRAMESOUT_LINK_DONE_PERIOD_MS
 *  @brief IPC frameouts periodic frame release in ms.
 */
#define IPC_FRAMESOUT_LINK_DONE_PERIOD_MS    (8u)

#define IPC_FRAMESOUT_LINK_MAX_OUT_QUE  (2)

typedef struct IpcFramesOutLink_statsObj {
    UInt32 recvCount;
    UInt32 droppedCount;
    UInt32 forwardCount;
    UInt32 totalRoundTrip;
    UInt32 releaseCount;
    Utils_PrfTsHndl *tsHandle;
} IpcFramesOutLink_statsObj;

typedef struct IpcFramesOutLink_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} IpcFramesOutLink_periodicObj;


typedef struct {
    UInt32 inFrameRecvCount;
    /**< input frame recevied from previous link */

    UInt32 inFrameUserSkipCount;
    /**< input frame rejected due mismatch in FID */

    UInt32 inFrameProcessCount;
    /**< input frames actually processed */

    Utils_frameSkipContext frameSkipCtx;
    /**< Data structure for frame skip to achieve expected output frame rate */    
} IpcFramesOutLink_ChObj;


typedef struct IpcFramesOutLink_Obj {
    UInt32 tskId;

    Utils_TskHndl tsk;

    IpcFramesOutLink_ChObj chObj[IPC_FRAMESOUT_LINK_MAX_CH];

    IpcFramesOutLinkRTOS_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    GateMP_Handle gateMPOutHndl;
    ListMP_Handle listMPInHndl;
    GateMP_Handle gateMPInHndl;

    SystemIpcFrames_ListElem *listElem[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];

    Utils_QueHandle listElemQue;
    SystemIpcFrames_ListElem *listElemQueMem[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];

    FVID2_Frame *listElem2FrameBufMap[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];
    /* IpcFramesOut link info */
    System_LinkInfo info;

    System_LinkInfo inQueInfo;

    IpcFramesOutLink_statsObj stats;

    IpcFramesOutLink_periodicObj prd;
    
    Utils_QueHandle outFrameBufQue[IPC_FRAMESOUT_LINK_MAX_OUT_QUE];
    FVID2_Frame *outFrameBufQueMem[IPC_FRAMESOUT_LINK_MAX_OUT_QUE][SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];

    UInt32 numCh;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];

    UInt32 statsStartTime;
    UInt32 totalFrameCount;
} IpcFramesOutLink_Obj;

#ifdef SYSTEM_DEBUG_IPC
#define IPCFRAMESOUTLINK_INFO_LOG(linkID,...)      do {                           \
                                                     Vps_printf(               \
                                                      "\n%d: IPCFRAMESOUT:Link[%x]:", \
                                                      Utils_getCurTimeInMsec(),linkID); \
                                                      Vps_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCFRAMESOUTLINK_INFO_LOG(linkID,...)
#endif

#endif                                                     /* _IPC_FRAMES_OUT_LINK_PRIV_H_ 
                                                            */

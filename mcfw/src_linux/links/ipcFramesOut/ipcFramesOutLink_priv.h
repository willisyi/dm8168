/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
    \addtogroup HLOS_Links          HLOS links

    @{
    \defgroup   IpcFramesOutLink_HLOS  Video Frame send link

    The IpcFramesOutLink_HLOS is used to send video frames defined
    by VIDFrame_Buf to a remote core's IpcFramesInLink.
    @{
*/

/**
    \file  mcfw/src_linux/links/ipcFramesOut/ipcFramesOutLink_priv.h
    \brief Ipc Frames Import Link private data structures definition
*/

#ifndef _IPC_FRAMES_OUT_LINK_PRIV_H_
#define _IPC_FRAMES_OUT_LINK_PRIV_H_

#include <stdio.h>
#include <osa.h>
#include <osa_que.h>
#include <osa_tsk.h>
#include <osa_debug.h>
#include <ti/ipc/SharedRegion.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <mcfw/src_linux/links/system/system_priv_ipc.h>
#include <mcfw/interfaces/link_api/vidframe.h>

#define IPC_FRAMES_OUT_LINK_OBJ_MAX   (3)

/**
 *  @def   IPC_FRAMESOUT_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_FRAMESOUT_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_FRAMESOUT_LINK_E_INVALIDLINKID
 *  @brief Define used to indicate invalid linkID
 */
#define IPC_FRAMESOUT_LINK_E_INVALIDLINKID   (-1)

/**
 *  @def   IPC_FRAMESOUT_LINK_DONE_PERIOD_MS
 *  @brief IPC frameouts periodic frame release in ms.
 */
#define IPC_FRAMESOUT_LINK_DONE_PERIOD_MS    (16u)

/**
 *  @def   IPC_FRAMESOUT_MAX_PENDING_CMDS
 *  @brief Maximum number of outstanding notify CMD that can be
 *         posted by the periodic task
*/
#define IPC_FRAMESOUT_MAX_PENDING_CMDS    (8)

#define IPC_FRAMESOUT_LINK_MAX_OUT_QUE  (2)

/**
 * @brief   IpcFramesOutStats Statistics structure
 */
typedef struct IpcFramesOutStats {
    UInt32 recvCount;             /**< Number of VIDFrame_Buf received */
    UInt32 droppedCount;          /**< Number of VIDFrame_Buf dropped   */
    UInt32 freeCount;             /**< Number of VIDFrame_Buf freed   */
    UInt32 totalRoundTrip;        /**< Accumulates total time in ms from
                                       time of ListMP queue to time of free*/
    UInt32 numNoFreeBufCount;     /**< Number of time no free buffers were available */
    UInt32 numNoFullBufCount;     /**< Number of time no ful  buffers were provided by app */
    UInt32 totalAppCallbackTime;  /**< Total time spent in app callback */
    UInt32 lastAppCallbackTime;   /**< Last appCallback time */
    UInt32 lastPutFullBufTime;    /**< system time when last putFullFrames was invoked by app */
    UInt32 lastGetFreeBufTime;    /**< system time when last getEmptyFrames was invoked by app */
} IpcFramesOutStats;

/**
 * @brief   Structures defining members for supporting timer based periodic
 *          operation (noNotifyMode) of IpcFramesInLink
 */
typedef struct IpcFramesOutLink_periodicObjHLOS {
    OSA_ThrHndl thrHandle;       /**< Periodic thread handle */
    volatile UInt32 numPendingCmd; /**< Number of NEW_DATA cmds sent
                                     *  but not processed */
    volatile Bool   exitThread;    /**< Flag to indicate periodic
                                        thread should exit */
} IpcFramesOutLink_periodicObjHLOS;

typedef struct IpcFramesOutLink_Obj {
    UInt32 tskId;

    OSA_TskHndl tsk;

    IpcFramesOutLinkHLOS_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    ListMP_Handle listMPInHndl;
    GateMP_Handle gateMPOutHndl;
    GateMP_Handle gateMPInHndl;

    SystemIpcFrames_ListElem *listElem[SYSTEM_IPC_FRAMES_MAX_LIST_ELEM];

    OSA_QueHndl listElemQue;

    System_LinkInfo info;

    IpcFramesOutStats stats;

    IpcFramesOutLink_periodicObjHLOS prd;
    
    OSA_QueHndl emptyFrameBufQue;

    UInt32 numCh;

    Bool   startProcessing;

} IpcFramesOutLink_Obj;

#ifdef SYSTEM_DEBUG_IPC
#define IPCFRAMESOUTLINK_INFO_LOG(linkID,...)   do {                           \
                                                    OSA_printf(               \
                                                      "\n%d: IPCFRAMESOUT:Link[%x]:", \
                                                    OSA_getCurTimeInMsec(),linkID); \
                                                    OSA_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCFRAMESOUTLINK_INFO_LOG(linkID,...)
#endif

#endif                                                     /* _IPC_FRAMES_OUT_LINK_PRIV_H_ 
                                                            */

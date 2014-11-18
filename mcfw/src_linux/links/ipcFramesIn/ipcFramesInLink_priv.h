/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
    \addtogroup HLOS_Links          HLOS links

    @{
    \defgroup   IpcFramesInLink_HLOS  Video Frame receive link

    The IpcFramesInLink_HLOS is used to receive video frames defined
    by VIDFrame_Buf from a remote core's IpcFramesOutLink.
    On receiving the video frame  an application defined callback
    funciton is invoked. @sa IpcFramesInCbFcn
    @{
*/

/**
    \file  mcfw/src_linux/links/ipcFramesIn/ipcFramesInLink_priv.h
    \brief Ipc Frames Import Link private data structures definition
*/

#ifndef _IPC_FRAMES_IN_LINK_PRIV_H_
#define _IPC_FRAMES_IN_LINK_PRIV_H_

#include <stdio.h>
#include <osa.h>
#include <osa_que.h>
#include <osa_tsk.h>
#include <osa_debug.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/syslink/utils/List.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <mcfw/src_linux/links/system/system_priv_ipc.h>
#include <mcfw/interfaces/link_api/vidframe.h>


#define IPC_FRAMES_IN_LINK_OBJ_MAX   (2)

/**
 *  @def   IPC_FRAMES_IN_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_FRAMES_IN_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_FRAMES_IN_LINK_PROCESS_PERIOD_MS
 *  @brief IPC frameouts periodic frame release in ms.
 */
#define IPC_FRAMES_IN_LINK_PROCESS_PERIOD_MS    (16u)

/**
 *  @def   IPC_FRAMES_IN_LINK_E_INVALIDLINKID
 *  @brief Define used to indicate invalid linkID
 */
#define IPC_FRAMES_IN_LINK_E_INVALIDLINKID   (-1)

/**
 *  @def   IPC_FRAMES_IN_MAX_PENDING_NEWDATA_CMDS
 *  @brief Maximum number of outstanding new data CMD that can be
 *         posted by the periodic task
*/
#define IPC_FRAMES_IN_MAX_PENDING_NEWDATA_CMDS    (8)

/**
 *  @def   IPC_FRAMES_IN_MAX_UNIQUE_BUFADDR
 *  @brief Maximum number of unique buffers that can be
 *         received by the link
*/
#define IPC_FRAMES_IN_MAX_UNIQUE_BUFADDR          (1024)
/**
 * @brief   Structures defining members for supporting timer based periodic
 *          operation (noNotifyMode) of IpcFramesInLink
 */
typedef struct IpcFramesInLink_PeriodicObj {
    OSA_ThrHndl thrHandle;       /**< Periodic thread handle */
    volatile UInt32 numPendingCmd; /**< Number of NEW_DATA cmds sent
                                     *  but not processed */
    volatile Bool   exitThread;    /**< Flag to indicate periodic
                                        thread should exit */
} IpcFramesInLink_PeriodicObj;

typedef struct IpcFramesInStats {
    UInt32 recvCount;            /**< Number of VIDFrame_Buf_Buf received */
    UInt32 droppedCount;         /**< Number of VIDFrame_Buf dropped   */
    UInt32 freeCount;            /**< Number of VIDFrame_Buf freed   */
    UInt32 totalRoundTrip;       /**< Accumulates total time in ms from
                                      time of deque to time of free for
                                      all freed frames  */
    UInt32 numNoFullBufCount;     /**< Number of time no full  buffers
                                    *  were received */
    UInt32 totalAppCallbackTime;  /**< Total time spent in app callback */
    UInt32 lastGetBufTime;        /**< System time when last getFullBuf was invoked */
    UInt32 numGetBufCalls;        /**< Number of times GetBuf was invoked */
    UInt32 lastFreeBufTime;       /**< System time when last putFreeBuf was invoked  */
    UInt32 numFreeBufCalls;       /**< Number of times FreeBuf was invoked */

} IpcFramesInStats;

typedef struct IpcFramesInLink_phyAddrMapEntry {
    List_Elem next;
    /*!< Pointer to next entry */
    UInt32    actualAddress;
    /*!< Actual address */
    UInt32    mappedAddress;
    /*!< Mapped address */
    UInt32    size;
} IpcFramesInLink_phyAddrMapEntry;

typedef struct IpcFramesInLink_phyAddrMapTbl {
    List_Object head;
    UInt32      numMappedEntries;
    IpcFramesInLink_phyAddrMapEntry mapEntryMem[IPC_FRAMES_IN_MAX_UNIQUE_BUFADDR];
    OSA_QueHndl mapEntryFreeQ;
    UInt32      totalMapSize;
} IpcFramesInLink_phyAddrMapTbl;


typedef enum IpcFramesInLink_State {
    IPC_FRAMES_IN_LINK_STATE_INACTIVE = 0,
    IPC_FRAMES_IN_LINK_STATE_ACTIVE
} IpcFramesInLink_State;

typedef struct IpcFramesInLink_Obj {
    UInt32 tskId;

    OSA_TskHndl tsk;

    IpcFramesInLinkHLOS_CreateParams createArgs;

    ListMP_Handle listMPOutHndl;
    ListMP_Handle listMPInHndl;

    OSA_QueHndl outFrameBufQue;

    /* IpcFramesIn link info */
    System_LinkInfo info;

    System_LinkInfo inQueInfo;

    IpcFramesInStats stats;
    IpcFramesInLink_PeriodicObj prd;
    volatile IpcFramesInLink_State state;
    IpcFramesInLink_phyAddrMapTbl  phyAddrMap;
} IpcFramesInLink_Obj;

#ifdef SYSTEM_DEBUG_IPC
#define IPCFRAMESINLINK_INFO_LOG(linkID,...)      do {                           \
                                                     OSA_printf(               \
                                                      "\n%d: IPCFRAMESIN:Link[%x]:", \
                                                      OSA_getCurTimeInMsec(),linkID); \
                                                      OSA_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCFRAMESINLINK_INFO_LOG(...)
#endif

#endif                                                     /* _IPC_FRAMES_IN_LINK_PRIV_H_ 
                                                            */

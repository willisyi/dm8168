/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
    \addtogroup HLOS_Links          HLOS links

    @{
    \defgroup   IpcBitsOutLink_HLOS  Video Bitstream send link

    The IpcBitsOutLink_HLOS is used to send video BitStream defined
    by Bitstream_Buf from HLOS core to remote core.
    The Bitstream_Buf to be sent is populated by the
    application in IpcBitOutCbFcn
    @sa IpcBitOutCbFcn
    @{
*/

/**
    \file  mcfw/src_linux/links/ipcBitsOut/ipcBitsOutLink_priv.h
    \brief Ipc Bits Export Link private data structures definition
*/

#ifndef _IPC_BITS_OUT_LINK_PRIV_H_
#define _IPC_BITS_OUT_LINK_PRIV_H_

#include <stdio.h>
#include <osa.h>
#include <osa_que.h>
#include <osa_tsk.h>
#include <osa_debug.h>
#include <ti/ipc/SharedRegion.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/vidbitstream.h>
#include <mcfw/src_linux/links/system/system_priv_ipc.h>


//#define DEBUG_IPC_BITS

/**
 *  @def   IPC_BITS_OUT_LINK_OBJ_MAX
 *  @brief Maxmimum number of instances of IpcBitsOutLink
 */
#define IPC_BITS_OUT_LINK_OBJ_MAX                        (2)

/**
 *  @def   IPC_BITS_OUT_PROCESS_PERIOD_MS
 *  @brief Interval in ms when IpcBitsOutLink will check for new data by invoking
 *         IpcBitOutCbFcn when configured to operate in noNotify mode.
 *         @sa IpcBitsOutLinkHLOS_CreateParams.noNotifyMode
 */
#define IPC_BITS_OUT_PROCESS_PERIOD_MS                   (16)

/**
 *  @def   IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS
 *  @brief Maximum of different sized pools to be used for BitStream buf
 *         allocation. The input channels' resolution will be mapped into
 *         one of the IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS resolution class
 *         and bitStream buffers will be allocated.
 *         <em>  All buffers in one pool will have the same size  </em>
 */
#define IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS                 (IPC_LINK_BITS_OUT_MAX_NUM_ALLOC_POOLS)

/**
 *  @def   IPC_BITS_OUT_LINK_MAX_OUT_FRAMES_PER_CH
 *  @brief Number of bitstream buffers to be allocated per channel
 *
 *  @attention <b> Modify this define to control the number of
 *                 buffers to be allocated per channel in the
 *                 IpcBitOutHLOS link </b>
 */
#define IPC_BITS_OUT_LINK_MAX_OUT_FRAMES_PER_CH          (6)
/**
 *  @def   IPC_BITSOUT_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_BITSOUT_LINK_S_SUCCESS       (0)

/**
 *  @def   IPC_BITSOUT_E_INT_UNKNOWNRESOLUTIONCLASS
 *  @brief Internal Error Define used to indicate unknown resolution class
 */
#define IPC_BITSOUT_E_INT_UNKNOWNRESOLUTIONCLASS (-64)

/**
 *  @def   IPC_BITSOUT_LINK_E_INVALIDLINKID
 *  @brief Error indicating invalid link ID
 */
#define IPC_BITSOUT_LINK_E_INVALIDLINKID         (-1)

/**
 *  @def   IPC_BITSOUT_LINK_E_INVALIDPARAM
 *  @brief Error indicating invalid param passed to linkCMD
 */
#define IPC_BITSOUT_LINK_E_INVALIDPARAM         (-2)


/**
 *  @def   IPC_BITSOUT_STATS_WARN_INTERVAL
 *  @brief Interval for printing waring about stats.
 *
 *  The ipcBitsOutLink will collect stats on warning indicators
 *  such as NO_FREE_BUF_AVAILABLE/NO_EMPTY_BUF_AVAILABLE and
 *  print warning msg every IPC_BITSOUT_STATS_WARN_INTERVAL
*/
#define IPC_BITSOUT_STATS_WARN_INTERVAL  (1000)


/**
 *  @def   IPC_BITSOUT_MAX_PENDING_NEWDATA_CMDS
 *  @brief Maximum number of outstanding new data CMD that can be
 *         posted by the periodic task
*/
#define IPC_BITSOUT_MAX_PENDING_NEWDATA_CMDS      (8)

#define IPC_BITSOUT_INVALID_ALLOC_POOL_ID         (~0u)

#define IPC_BITSOUT_MAX_PENDING_RELEASE_FRAMES_CMDS (IPC_BITSOUT_MAX_PENDING_NEWDATA_CMDS)

#define IPC_BITSOUT_LINK_DELBUF_CMD_PENDING_WARNING_THRESHOLD_MS (500)

/**
 * \brief  Enum listing possible states of the bit buffer pool
 */
typedef enum
{
    IPCBITSOUTHLOS_BUFPOOL_STATE_CREATED,
    IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_FLUSH_DONE,
    IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_WAIT_APP_BUF_FREE,
    IPCBITSOUTHLOS_BUFPOOL_STATE_DELETED,
} IpcBitsOutHLOS_bufPoolState_e;
/**
 * @brief   IpcBitsOutStats Statistics structure
 */
typedef struct IpcBitsOutStats {
    UInt32 recvCount;             /**< Number of BitStream_Buf received */
    UInt32 droppedCount;          /**< Number of BitStream_Buf dropped   */
    UInt32 freeCount;             /**< Number of BitStream_Buf freed   */
    UInt32 totalRoundTrip;        /**< Accumulates total time in ms from
                                       time of ListMP queue to time of free*/
    UInt32 numNoFreeBufCount;     /**< Number of time no free buffers were available */
    UInt32 numNoFullBufCount;     /**< Number of time no ful  buffers were provided by app */
    UInt32 totalAppCallbackTime;  /**< Total time spent in app callback */
    UInt32 lastAppCallbackTime;   /**< Last appCallback time */
} IpcBitsOutStats;


/**
 * @brief   Structures defining bitstream buffer pool info
 *
 * A pool will alloc a single bitstream buffer of
 *  <tt>
 *  totalPoolAllocSize = IpcBitsOutLink_BufPoolInfo.bufSize *
 *                       IpcBitsOutLink_BufPoolInfo.numBufs
 *  </tt>
 *  The single bitstream buffer pointer will be stored in
 *  \c IpcBitsOutLink_Obj.bitBufPoolPtr[poolID]
 *  The total bitstream buffer  size will be stored in
 *  \c IpcBitsOutLink_Obj.bitBufPoolSize[poolID]
 *  The large bitstream buufer of the pool will be partioned into
 *  IpcBitsOutLink_BufPoolInfo.numBufs individual Bitstream_Buf
 *
 */
typedef struct IpcBitsOutLink_BufPoolInfo {
    UInt32 numBufs;             /**< Number of buffers in the pool */
    UInt32 bufSize;             /**< Size of each buffer in the pool */
} IpcBitsOutLink_BufPoolInfo;

/**
 * @brief   Structures defining number of pools and info on each pool
 */
typedef struct IpcBitsOutLink_BufAllocPoolListInfo {
    UInt32 numPools;            /**< Number of allocation pool */
    IpcBitsOutLink_BufPoolInfo bufPoolInfo[IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS];
                                /**< Array of allocation pool info */
} IpcBitsOutLink_BufAllocPoolListInfo;

/**
 * @brief   Structures defining allocation pool info and channel2Pool mapping
 */
typedef struct IpcBitsOutLink_OutQueueInfo {
    IpcBitsOutLink_BufAllocPoolListInfo allocPoolInfo;
                               /**< Stores number of pools and info
                                *   for each pool.
                                *   @sa IpcBitsOutLink_BufAllocPoolListInfo
                                */
    UInt32 ch2poolMap[SYSTEM_MAX_CH_PER_OUT_QUE];
                               /**< Table mapping channel to its corresponding
                                *   allocation pool
                                */
} IpcBitsOutLink_OutQueueInfo;

/**
 * @brief   Structures defining members for supporting timer based periodic
 *          operation (noNotifyMode) of IpcBitOutLink
 */
typedef struct IpcBitsOutLink_PeriodicObj {
    OSA_ThrHndl thrHandle;       /**< Periodic thread handle */
    volatile UInt32 numPendingCmd; /**< Number of NEW_DATA cmds sent
                                     *  but not processed */
    volatile Bool   exitThread;    /**< Flag to indicate periodic
                                        thread should exit */
} IpcBitsOutLink_PeriodicObj;

/**
 * @brief   IpcBitsOutLink instance structure
 */
typedef struct IpcBitsOutLink_Obj {
    UInt32 tskId;              /**< IpcBitsOutLink instance linkID */
    OSA_TskHndl tsk;           /**< IpcBitsInLink task handle */
    IpcBitsOutLinkHLOS_CreateParams createArgs;
                               /**< Application passed create Args */
    ListMP_Handle listMPOutHndl; /**< IPC SystemIpcBits_ListElem full queue */
    ListMP_Handle listMPInHndl;  /**< IPC SystemIpcBits_ListElem empty queue */
    GateMP_Handle gateMPInHndl;  /**< IPC Gate for listMPOutHndl */
    GateMP_Handle gateMPOutHndl; /**< IPC Gate for listMPInHndl */
    SystemIpcBits_ListElem *listElem[SYSTEM_IPC_BITS_MAX_LIST_ELEM];
                                 /**< Array of listElems populated at creat time */
    OSA_QueHndl listElemQue[IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS];
                                     /**< Queue holding free listElems */
    Ptr         bitBufPoolPtr[IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS];
                                 /**< Pool bit buffer pointer . */
    UInt32 bitBufPoolSize[IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS];
                                 /**< Size of Pools bitstream buffer. */
    volatile IpcBitsOutHLOS_bufPoolState_e   bitBufPoolState[IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS];
                                 /**< Flag indicating whether the buffer pool is created */
    UInt32 appAllocBufCnt[IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS];
                                /**< Count of number of buffers allocated by
                                 *   by application
                                 */
    OSA_QueHndl listElemFreeQue; /** Queue holding free list elements */
    Bitstream_BufList freeBitBufList; /**< BitStream buffers to be freed  */
    IpcBitsOutStats stats;            /**< Statistics accumulation member */
    IpcBitsOutLink_OutQueueInfo outQueInfo; /**< Output BitStream queue info   */
    volatile Bool startProcessing;    /**< Flag to control start of processing */
    OSA_MutexHndl apiMutex; /**< Mutex to manage access to ipcBitsOut object from
                              *  multiple context */
    OSA_MsgHndl *pDeleteBufMsg; /**< The acknowledge for IPCBITSOUT_LINK_CMD_DELETE_CH_BUFFER
                                     msg is stored and acked only when all buffers are freed
                                     and memory is actually freed */
    UInt32 delMsgReceiveTime;   /**< Time when the delete msg was received */
    IpcBitsOutLink_PeriodicObj prd; /**< Periodic thread object */

} IpcBitsOutLink_Obj;

/**
 * @def     IPCBITSOUTLINK_INFO_LOG
 * @brief   Info Log macro.
 *
 * Info logging is enabled by defining SYSTEM_DEBUG_IPC in system_debug.h
 * This macro will append the current timestamp returned by OSA_getCurTimeInMsec()
 * and linkID to the passed info
 * The first param of this macro is linkID.Addition arguments may be optionally
 * passed which will be logged as is.
 */
#ifdef SYSTEM_DEBUG_IPC
#define IPCBITSOUTLINK_INFO_LOG(linkID,...)      do {                           \
                                                     OSA_printf(                    \
                                                      "\n%d: IPCBITSOUT:Link[%x]:", \
                                                      OSA_getCurTimeInMsec(),linkID); \
                                                     OSA_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCBITSOUTLINK_INFO_LOG(linkID,...)
#endif

#endif                                                     /* _IPC_BITS_OUT_LINK_PRIV_H_
                                                            */
/* @} */
/* @} */

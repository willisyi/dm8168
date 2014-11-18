/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
    \addtogroup HLOS_Links          HLOS links

    @{
    \defgroup   IpcBitsInLink_HLOS  Video Bitstream receive link

    The IpcBitsInLink_HLOS is used to receive video BitStream defined
    by Bitstream_Buf from a remote core's IpcBitsOutLink.
    On receiving the bitStream buffer an application defined callback
    funciton is invoked. @sa IpcBitInCbFcn
    @{
*/

/**
    \file  mcfw/src_linux/links/ipcBitsIn/ipcBitsInLink_priv.h
    \brief Ipc Bits Import Link private data structures definition
*/

#ifndef _IPC_BITS_IN_LINK_PRIV_H_
#define _IPC_BITS_IN_LINK_PRIV_H_

#include <stdio.h>
#include <osa.h>
#include <osa_que.h>
#include <osa_tsk.h>
#include <osa_debug.h>
#include <ti/ipc/SharedRegion.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <mcfw/interfaces/link_api/vidbitstream.h>
#include <mcfw/src_linux/links/system/system_priv_ipc.h>

/**
 *  @def   IPC_BITS_IN_LINK_OBJ_MAX
 *  @brief Maxmimum number of instances of IpcBitsInLink
 */
#define IPC_BITS_IN_LINK_OBJ_MAX       (2)

/**
 *  @def   IPC_BITS_IN_PROCESS_PERIOD_MS
 *  @brief Interval in ms when IpcBitsInLink will check for new data in the
 *         input ListMP when configured to operate in noNotify mode.
 *         @sa IpcBitsInLinkHLOS_CreateParams.noNotifyMode
 */
#define IPC_BITS_IN_PROCESS_PERIOD_MS  (16)
/**
 *  @def   IPC_BITS_IN_LINK_S_SUCCESS
 *  @brief Define used to indicate successful execution
 */
#define IPC_BITS_IN_LINK_S_SUCCESS   (0)

/**
 *  @def   IPC_BITS_IN_LINK_E_INVALIDLINKID
 *  @brief Define used to indicate invalid linkID
 */
#define IPC_BITS_IN_LINK_E_INVALIDLINKID   (-1)

/**
 *  @def   IPC_BITS_IN_MAX_PENDING_NEWDATA_CMDS
 *  @brief Maximum number of outstanding new data CMD that can be
 *         posted by the periodic task
*/
#define IPC_BITS_IN_MAX_PENDING_NEWDATA_CMDS    (8)


/**
 * @brief   IpcBitsInLink Statistics structure
 */
typedef struct IpcBitsInLink_Stats {
    UInt32 recvCount;            /**< Number of BitStream_Buf received */
    UInt32 droppedCount;         /**< Number of BitStream_Buf dropped   */
    UInt32 freeCount;            /**< Number of BitStream_Buf freed   */
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
} IpcBitsInLink_Stats;


/**
 * @brief   Structures defining members for supporting timer based periodic
 *          operation (noNotifyMode) of IpcBitInLink
 */
typedef struct IpcBitsInLink_PeriodicObj {
    OSA_ThrHndl thrHandle;       /**< Periodic thread handle */
    volatile UInt32 numPendingCmd; /**< Number of NEW_DATA cmds sent
                                     *  but not processed */
    volatile Bool   exitThread;    /**< Flag to indicate periodic
                                        thread should exit */
} IpcBitsInLink_PeriodicObj;

/**
 * @brief   IpcBitsInLink instance structure
 */
typedef struct IpcBitsInLink_Obj {
    UInt32 tskId;                  /**< IpcBitsInLink instance linkID */

    OSA_TskHndl tsk;               /**< IpcBitsInLink task handle */

    IpcBitsInLinkHLOS_CreateParams createArgs;
                                  /**< Application passed create Args */
    ListMP_Handle listMPOutHndl;  /**< IPC SystemIpcBits_ListElem full queue */
    ListMP_Handle listMPInHndl;   /**< IPC SystemIpcBits_ListElem free queue */

    Bitstream_BufList freeBufList;/**< BitStream buffers to be freed  */

    OSA_QueHndl outBitBufQue;     /**< Full BitStream bufs queued for next link */

    System_LinkInfo inQueInfo;    /**< Prev link input queue info */

    IpcBitsInLink_Stats stats;    /**< Statistics accumulation member */

    IpcBitsInLink_PeriodicObj prd;/**< Periodic processing member */

} IpcBitsInLink_Obj;

/**
 * @def     IPCBITSINLINK_INFO_LOG
 * @brief   Info Log macro.
 *
 * Info logging is enabled by defining SYSTEM_DEBUG_IPC in system_debug.h
 * This macro will append the current timestamp returned by OSA_getCurTimeInMsec()
 * and linkID to the passed info
 * The first param of this macro is linkID.Addition arguments may be optionally
 * passed which will be logged as is.
 */
#ifdef SYSTEM_DEBUG_IPC
#define IPCBITSINLINK_INFO_LOG(linkID,...)      do {                           \
                                                    OSA_printf(                    \
                                                      "%d: IPCBITSIN:Link[%x]:",   \
                                                      OSA_getCurTimeInMsec(),linkID); \
                                                    OSA_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define IPCBITSINLINK_INFO_LOG(linkID,...)
#endif

#endif                                                     /* _IPC_BITS_IN_LINK_PRIV_H_ 
                                                            */

/* @} */
/* @} */

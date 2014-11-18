/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup IPC_LINK_API Inter-Processor Communication (IPC) Link API

    @{
*/

/**
    \file ipcLink.h
    \brief Inter-Processor Communication (IPC) Link API
*/

#ifndef _IPC_LINK_H_
#define _IPC_LINK_H_

#include <string.h>
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/vidbitstream.h>
#include <mcfw/interfaces/link_api/vidframe.h>
#include <ti/ipc/SharedRegion.h>

/** \brief Max number of bit buffer polls in IPC link */
#define IPC_LINK_BITS_OUT_MAX_NUM_ALLOC_POOLS        (SYSTEM_MAX_CH_PER_OUT_QUE)

/** \brief  IpcFramesOutLinkRTOS command to set Frame Rate */
#define IPCFRAMESOUTRTOS_LINK_CMD_SET_FRAME_RATE     (0x7000)
/** \brief IpcFramesOutLinkRTOS command to print statistics */
#define IPCFRAMESOUTRTOS_LINK_CMD_PRINT_STATISTICS   (0x7001)

/** \brief IpcOutM3Link command to set Frame Rate */
#define IPCOUTM3_LINK_CMD_SET_FRAME_RATE             (0x6000)
/** \brief IpcOutM3Link command to print statistics */
#define IPCOUTM3_LINK_CMD_PRINT_STATISTICS           (0x6001)

/** \brief IpcBitsOutLink command to print statistics */
#define IPCBITSOUT_LINK_CMD_PRINT_BUFFER_STATISTICS   (0x8001)

/** \brief IpcBitsOutLink command to delete ch buffers */
#define IPCBITSOUT_LINK_CMD_DELETE_CH_BUFFER          (0x8002)

/** \brief IpcBitsOutLink command to create ch buffers  */
#define IPCBITSOUT_LINK_CMD_CREATE_CH_BUFFER          (0x8003)

/**
 *  @internal
 *  @brief Enum indicating state of IPC bit buffer. For use internal to component
 */
typedef enum IpcBitsBufState {
    IPC_BITBUF_STATE_FREE,
    /**< Buffer is free */
    IPC_BITBUF_STATE_ALLOCED,
    /**< Buffer is alloced by IpcBitsOut link */
    IPC_BITBUF_STATE_OUTQUE,
    /**< Buffer is queued in ListMP outque (full Que) */
    IPC_BITBUF_STATE_INQUE,
    /**< Buffer is queued in ListMP inque  (free Que) */
    IPC_BITBUF_STATE_DEQUEUED,
    /**< Buffer has been dequed by IpcBitsIn Link */
} IpcBitsBufState;

/**
 * @internal
 *  @brief Invalid SharedRegion Portable address.@sa SharedRegion_SrPtr
 *  @todo  Remove this define once SharedRegion_INVALIDSRPTR is exported in
 *         a SharedRegion.h
 */
#define IPC_LINK_INVALID_SRPTR   (SharedRegion_invalidSRPtr())

/**
 * @internal
 * @brief IPC bits internal macro to set owner processor ID
 *
 * Top 4 bits store the current procID of a IPC bit buffer.
 * Invokes System_getSelfProcId() to determine current PROC ID
 */
#define SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(bufState)  (bufState =                \
                                                       ((bufState & 0x0FFFFFFFu) \
                                                       |                         \
                                                       (System_getSelfProcId() & 0xF) << 28))

/**
 * @internal
 * @brief IPC bits internal macro to set internal buffer state
 *
 * Lower 28 bits (of 32 bits bufState) store the bufState
 * newState is the new buffer state to be set.
 * @sa  IpcBitsBufState
 */
#define SYSTEM_IPC_BITS_SET_BUFSTATE(bufState,newState)  (bufState =                 \
                                                          ((bufState & 0xF0000000u)  \
                                                          |                          \
                                                          (newState & 0x0FFFFFFFu)))

/**
 * @internal
 * @brief IPC bits internal macro to get owner processor ID
 *
 * Upper 4 bits (of 32 bits bufState) store the bufState
 * ProcID returned is MultiProcID
 */
#define SYSTEM_IPC_BITS_GET_BUFOWNERPROCID(bufState)     (((bufState) >> 28) & 0x0000000Fu)

/**
 * @internal
 * @brief IPC bits internal macro to set buffer state
 *
 * Lower 28 bits (of 32 bits bufState) store the bufState
 * @sa  IpcBitsBufState
 */
#define SYSTEM_IPC_BITS_GET_BUFSTATE(bufState)           ((bufState) & 0x0FFFFFFFu)

/**
  * @internal
 *  @brief   Structure used to exchange BitBuffer across cores.
 */
typedef struct SystemIpcBits_ListElem {

    Bitstream_Buf               bitBuf;
    /**< BitStream_Buf info to be exchanged. @sa Bitstream_Buf */
    volatile SharedRegion_SRPtr srBufPtr;
    /**< SharedRegion pointer of BitStream_Buf addr element
     *    This portable pointer is used on remote core to get local Vptr */
    Int32                       bufState;
    /**< Element used to store internal bufState @sa IpcBitsBufState */
    Ptr                         ipcPrivData;
    /**< Private Pointer for exclusive use by IPCBits Link.
     *  Not to be modified by other links. */
} SystemIpcBits_ListElem;

/**
    \brief IpcOutM3Link link channel dynamic set config params

    Defines IpcOutM3Link FPS parameters that can be changed dynamically
    on a per channel basis
*/
typedef struct IpcOutM3Link_ChFpsParams
{
    UInt32 chId;
    /**< IpcOutM3Link channel number */
    UInt32 inputFrameRate;
    /**< input frame rate - 60 or 50 fps if interlaced */
    UInt32 outputFrameRate;
    /**< Expected output frame rate */
} IpcOutM3Link_ChFpsParams;

/**
    \brief Ipc link create parameters
*/
typedef struct
{
    System_LinkInQueParams   inQueParams;
    /**< Input queue information */

    System_LinkOutQueParams  outQueParams[SYSTEM_MAX_OUT_QUE];
    /**< Output queue information */

    UInt32 numOutQue;
    /**< Actual number of out queues used*/

    UInt32 processLink;
    /**< link ID that will process data of current link   */

    UInt32 notifyProcessLink;
    /**< Flag to indicate if process link is to be notified on new data */

    UInt32 notifyNextLink;
    /**< Flag to indicate if next link is to be notified on new data */

    UInt32 notifyPrevLink;
    /**< Flag to indicate if prev link is to be notified on empty buf */

    Bool noNotifyMode;
    /**< Flag to used to configure link to operate in noNotifyMode.
     *   If set to TRUE the link will not send notify events to
     *   remote core when buffer are consumed.
     */
    UInt32 inputFrameRate;
    /**< Input frame rate of the IPC link */

    UInt32 outputFrameRate;
    /**< Output frame rate of the IPC link */

    UInt32 numChPerOutQue[SYSTEM_MAX_OUT_QUE];
    /**< Actual number of channles per out queues used */

    UInt32 equallyDivideChAcrossOutQues;
    /**< Allow the system to equally divide and put the channles across
         different out queues. If equallyDivideChAcrossOutQues is TRUE,
         RDK ignore the numChPerOutQue entries and populate them again.
         If you want app/integrator to set numChPerOutQue values, then
         configure equallyDivideChAcrossOutQues as FALSE */

} IpcLink_CreateParams;

/**
    \brief IPCBITSOUT_LINK_CMD_DELETE_CH_BUFFER associated params
*/
typedef struct
{
    UInt32 chId; /**< Channel number whose buffers need to be deleted */
} IpcBitsOutHLOSLink_deleteChBufParams;


/**
    \brief IPCBITSOUT_LINK_CMD_CREATE_CH_BUFFER associated params
*/
typedef struct
{
    UInt32 chId; /**< Channel number whose buffers need to be created */
    UInt32 numBufs; /**< Number of buffers to be allocated for the channel */
    UInt32 maxWidth;  /**< Max width of frames received in this channel */
    UInt32 maxHeight; /**< Max height of frames received in this channel */
} IpcBitsOutHLOSLink_createChBufParams;

/**
 * \typedef IpcBitsInCbFcn
 * \brief   IpcBitsIn Link application Callback function
 *
 * This application passed function is invoked by IpcBitsInLink on
 * HLOS when new bitStream buffer are available.
 * This can be used by application as synch points to invoke
 * IpcBitsInLink_getFullVideoBitStreamBufs
 */
typedef Void (*IpcBitsInCbFcn) (Ptr cbCtx);


/**
    \brief Ipc bits out link create parameters

*/
typedef struct IpcBitsOutLinkHLOS_CreateParams
{
    IpcLink_CreateParams                 baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */

    System_LinkQueInfo                   inQueInfo;
    /**< Input bitStream info.Application should populate
     * max numCh and Channel info for each input bitstream channel
     * @sa System_LinkQueInfo
     */

    UInt32                               bufPoolPerCh;
    /**< Flag to indicate  each channel shouuld have have its
     * own pool of memory instead of a common pool based on
     * resolution. Choose this option if you want to
     * fix the number of buffers per each channel
     */

    UInt32  numBufPerCh[IPC_LINK_BITS_OUT_MAX_NUM_ALLOC_POOLS];
    /**< Number of buffer to allocate per channel per buffr pool */

} IpcBitsOutLinkHLOS_CreateParams;

/**
    \brief Ipc bits out link create parameters
*/
typedef struct IpcBitsOutLinkRTOS_CreateParams
{
    IpcLink_CreateParams                 baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */
} IpcBitsOutLinkRTOS_CreateParams;

/**
    \brief Ipc bits in link create parameters
*/
typedef struct IpcBitsInLinkHLOS_CreateParams
{
    IpcLink_CreateParams  baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */
    IpcBitsInCbFcn        cbFxn;
    /**< Application specified callback function.
     * @sa IpcBitInCbFcn
     */
    Ptr                   cbCtx;
    /**< cbCtx to be used in cbFxn */
} IpcBitsInLinkHLOS_CreateParams;

/**
    \brief Ipc bits in link create parameters
*/
typedef struct IpcBitsInLinkRTOS_CreateParams
{
    IpcLink_CreateParams                 baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */
} IpcBitsInLinkRTOS_CreateParams;

typedef enum IpcBitsOutLinkHLOS_BitstreamBufReqType
{
    IPC_BITSOUTHLOS_BITBUFREQTYPE_CHID   = 0x80000000,
    IPC_BITSOUTHLOS_BITBUFREQTYPE_BUFSIZE,
} IpcBitsOutLinkHLOS_BitstreamBufReqType;

/**
    \brief Structure passed as argument to IpcBitsOutLink_getEmptyVideoBitStreamBufs
*/
typedef struct IpcBitsOutLinkHLOS_BitstreamBufReqInfo
{
    UInt32 numBufs;
    /**< Number of empty buffers required. IpcBitsOutLink may return
     *   number of empty buffers less than this
     */
    IpcBitsOutLinkHLOS_BitstreamBufReqType reqType;
    /**< IpcBitsOutLinkHLOS buffer request type maybe chId or bufSize.
     *   The reqType should match the create param
     *   of ipcBitsOutLink bufPoolPerCh member
     */

    union {
        UInt32 minBufSize;
        /**< Minimum size of bitBuf for each buffer.
         *   This field should be set if
         *   IpcBitsOutLinkHLOS_CreateParams.bufPoolPerCh is set to FALSE
         */
        UInt32 chNum;
        /**< Channel number for which empty bitBuffers are requested.
         *   This field should be set if
         *   IpcBitsOutLinkHLOS_CreateParams.bufPoolPerCh is set to TRUE
         */

    } u [VIDBITSTREAM_MAX_BITSTREAM_BUFS]; /**< Array of bitBufRequest Info */
} IpcBitsOutLinkHLOS_BitstreamBufReqInfo;

/**
 *  @internal
 *  @enum  IpcBitsBufState
 *  @brief Enum indicating state of IPC bit buffer. For use internal to component
 */
typedef enum IpcFramesBufState {
    IPC_FRAMEBUF_STATE_FREE,
    /**< Buffer is free */
    IPC_FRAMEBUF_STATE_ALLOCED,
    /**< Buffer is alloced by IpcBitsOut link */
    IPC_FRAMEBUF_STATE_OUTQUE,
    /**< Buffer is queued in ListMP outque (full Que) */
    IPC_FRAMEBUF_STATE_INQUE,
    /**< Buffer is queued in ListMP inque  (free Que) */
    IPC_FRAMEBUF_STATE_DEQUEUED,
    /**< Buffer has been dequed by IpcBitsIn Link */
} IpcFramesBufState;

/**
 * @internal
 * @brief IPC frames internal macro to set owner processor ID
 *
 * Top 4 bits store the current procID of a IPC frame buffer.
 * Invokes System_getSelfProcId() to determine current PROC ID
 */
#define SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(bufState)  (bufState =                \
                                                       ((bufState & 0x0FFFFFFFu) \
                                                       |                         \
                                                       (System_getSelfProcId() & 0xF) << 28))

/**
 * @internal
 * @brief IPC frames internal macro to set internal buffer state
 *
 * Lower 28 bits (of 32 bits bufState) store the bufState
 * newState is the new buffer state to be set.
 * @sa  IpcFramesBufState
 */
#define SYSTEM_IPC_FRAMES_SET_BUFSTATE(bufState,newState)  (bufState =                 \
                                                          ((bufState & 0xF0000000u)  \
                                                          |                          \
                                                          (newState & 0x0FFFFFFFu)))

/**
 * @internal
 * @brief IPC frames internal macro to get owner processor ID
 *
 * Upper 4 bits (of 32 bits bufState) store the bufState
 * ProcID returned is MultiProcID
 */
#define SYSTEM_IPC_FRAMES_GET_BUFOWNERPROCID(bufState)     (((bufState) >> 28) & 0x0000000Fu)

/**
 * @internal
 * @brief IPC frames internal macro to set buffer state
 *
 * Lower 28 bits (of 32 bits bufState) store the bufState
 * @sa  IpcFramesBufState
 */
#define SYSTEM_IPC_FRAMES_GET_BUFSTATE(bufState)           ((bufState) & 0x0FFFFFFFu)

/**
  * @internal
 *  @brief   Structure used to exchange FrameBuffer across cores.
 */
typedef struct SystemIpcFrames_ListElem {
    VIDFrame_Buf               frameBuf;
    /**< BitStream_Buf info to be exchanged. @sa Bitstream_Buf */
    volatile SharedRegion_SRPtr srBufPtr[VIDFRAME_MAX_FIELDS][VIDFRAME_MAX_PLANES];
    /**< SharedRegion pointer of BitStream_Buf addr element
     *    This portable pointer is used on remote core to get local Vptr */
    Int32                       bufState;
    /**< Element used to store internal bufState @sa IpcBitsBufState */
    Ptr                         ipcPrivData;
    /**< Private Pointer for exclusive use by IPCBits Link.
     *  Not to be modified by other links. */
    UInt32                      timeStamp;
    /**< Time stamp for internal IPC statistics purposes*/
} SystemIpcFrames_ListElem;

/**
 * \brief   IpcFramesIn Link application Callback function
 *
 * This application passed function is invoked by IpcBitsInLink on
 * HLOS when new bitStream buffer are available.
 * This can be used by application as synch points to invoke
 * IpcBitsInLink_getFullVideoBitStreamBufs
 */
typedef Void (*IpcFramesInCbFcn) (Ptr cbCtx);


/**
    \brief Ipc frames out link create parameters

*/
typedef struct IpcFramesOutLinkHLOS_CreateParams
{
    IpcLink_CreateParams                 baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */
    System_LinkQueInfo                   inQueInfo;
    /**< Input frame info.Application should populate
     * max numCh and Channel info for each input channel
     * @sa System_LinkQueInfo
     */
} IpcFramesOutLinkHLOS_CreateParams;

/**
    \brief Ipc frames out link create parameters
*/
typedef struct IpcFramesOutLinkRTOS_CreateParams
{
    IpcLink_CreateParams                 baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */
} IpcFramesOutLinkRTOS_CreateParams;

/**
    \brief Ipc frames in link create parameters
*/
typedef struct IpcFramesInLinkHLOS_CreateParams
{
    IpcLink_CreateParams  baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */
    IpcFramesInCbFcn         cbFxn;
    /**< Application specified callback function.
     * @sa IpcBitInCbFcn
     */
    Ptr                   cbCtx;
    /**< cbCtx to be used in cbFxn */
    UInt32                exportOnlyPhyAddr;
    /**< Flag indicating whether application requies only physical address
     *   of buffers or both physical and virtual address.
     */
} IpcFramesInLinkHLOS_CreateParams;

/**
    \brief Ipc frames in link create parameters
*/
typedef struct IpcFramesInLinkRTOS_CreateParams
{
    IpcLink_CreateParams                 baseCreateParams;
    /**< Base Create params.@sa IpcLink_CreateParams */

} IpcFramesInLinkRTOS_CreateParams;

/**
    \brief Ipc Video Frame (FVID2_Frame) export (sendOut) link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 IpcOutM3Link_init();
/**
    \brief Ipc Video Frame (FVID2_Frame) import (receive) link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 IpcInM3Link_init();


/**
    \brief Ipc Video BitStream (Bitstream_Buf) import (receive) link register and init

    - Creates link task
    - Registers as a link with the system API

    \return IPC_BITS_IN_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsInLink_init();

/**
    \brief Ipc Video BitStream (Bitstream_Buf) export (sendOut) link register and init

    - Creates link task
    - Registers as a link with the system API

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsOutLink_init();

/**
    \brief Ipc Video Frame (FVID2_Frame) import (receive) link register and init

    - Creates link task
    - Registers as a link with the system API

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesInLink_init();

/**
    \brief Ipc Video Frame (FVID2_Frame) export (sendOut) link register and init

    - Creates link task
    - Registers as a link with the system API

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesOutLink_init();


/**
    \brief Ipc Video Frame (FVID2_Frame) export (sendOut) link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 IpcOutM3Link_deInit();
/**
    \brief Ipc Video Frame (FVID2_Frame) import (receive) link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 IpcInM3Link_deInit();
/**
    \brief Ipc Video Bitstream (Bitstream_Buf) import (receive) link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return IPC_BITS_IN_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsInLink_deInit();

/**
    \brief Ipc Video Bitstream (Bitstream_Buf) export (sendout) link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsOutLink_deInit();

/**
    \brief Ipc Video Frame (FVID2_Frame) import (receive) link deinit


    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesInLink_deInit();

/**
    \brief Ipc Video Frame (FVID2_Frame) export (sendOut) link deinit


    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesOutLink_deInit();

/**
    \brief Gets the filled bitstream buffers

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsInLink_getFullVideoBitStreamBufs(UInt32 linkId,
                                              Bitstream_BufList *bufList);


/**
    \brief Puts the emptied bitstream buffers

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsInLink_putEmptyVideoBitStreamBufs(UInt32 linkId,
                                               Bitstream_BufList *bufList);


/**
    \brief Gets the filled bitstream buffers

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsOutLink_getEmptyVideoBitStreamBufs(UInt32 linkId,
                                                Bitstream_BufList *bufList,
                                                IpcBitsOutLinkHLOS_BitstreamBufReqInfo *reqInfo);


/**
    \brief Puts the emptied bitstream buffers

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsOutLink_putFullVideoBitStreamBufs(UInt32 linkId,
                                               Bitstream_BufList *bufList);

/**
    \brief Returns the input queue info for the ipcBitsOut link

    \return IPC_BITSOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcBitsOutLink_getInQueInfo(UInt32 linkId,
                                  System_LinkQueInfo *inQueInfo);

/**
    \brief Gets the filled video frames

    \return IPC_FRAMES_IN_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesInLink_getFullVideoFrames(UInt32 linkId,
                                         VIDFrame_BufList *frameList);

/**
    \brief Release the emptied video frames

    \return IPC_FRAMES_IN_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesInLink_putEmptyVideoFrames(UInt32 linkId,
                                          VIDFrame_BufList *frameList);

/**
    \brief Gets the filled video frames

    \return IPC_FRAMES_IN_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesOutLink_putFullVideoFrames(UInt32 linkId,
                                          VIDFrame_BufList *frameList);

/**
    \brief Release the emptied video frames

    \return IPC_FRAMES_IN_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesOutLink_getEmptyVideoFrames(UInt32 linkId,
                                           VIDFrame_BufList *frameList);

/**
    \brief Returns the input queue info for the ipcFramesOut link

    \return IPC_FRAMESOUT_LINK_S_SUCCESS (0) on success
*/
Int32 IpcFramesOutLink_getInQueInfo(UInt32 linkId,
                                    System_LinkQueInfo *inQueInfo);

/**
 * \brief Sets default values for IpcLink_CreateParams_Init
 *
 */
static inline Void IpcLink_CreateParams_Init(IpcLink_CreateParams *prm)
{
    Int i;

    prm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    prm->inQueParams.prevLinkQueId = 0;
    prm->noNotifyMode      = FALSE;
    prm->notifyNextLink = TRUE;
    prm->notifyPrevLink = TRUE;
    prm->notifyProcessLink = TRUE;
    prm->numOutQue = 1;
    prm->inputFrameRate  = 30;
    prm->outputFrameRate = 30;

    for (i = 0; i < SYSTEM_MAX_OUT_QUE;i++)
    {
        prm->outQueParams[i].nextLink = SYSTEM_LINK_ID_INVALID;
        prm->numChPerOutQue[i] = 0;
    }
    prm->equallyDivideChAcrossOutQues = TRUE;
    prm->processLink = SYSTEM_LINK_ID_INVALID;
}

/**
 * \brief Sets default values for IpcFramesInLinkHLOS_CreateParams_Init
 *
 */
static inline Void IpcFramesInLinkHLOS_CreateParams_Init(IpcFramesInLinkHLOS_CreateParams *prm)
{
    IpcLink_CreateParams_Init(&prm->baseCreateParams);
    prm->cbCtx = NULL;
    prm->cbFxn = NULL;
    prm->exportOnlyPhyAddr = TRUE;
}

/**
 * \brief Sets default values for IpcFramesOutLinkHLOS_CreateParams
 *
 */
static inline Void IpcFramesOutLinkHLOS_CreateParams_Init(IpcFramesOutLinkHLOS_CreateParams *prm)
{
    IpcLink_CreateParams_Init(&prm->baseCreateParams);
    prm->inQueInfo.numCh = 0;
}


/**
 * \brief Sets default values for IpcFramesInLinkRTOS_CreateParams_Init
 *
 */
static inline Void IpcFramesInLinkRTOS_CreateParams_Init(IpcFramesInLinkRTOS_CreateParams *prm)
{
    IpcLink_CreateParams_Init(&prm->baseCreateParams);
}

/**
 * \brief Sets default values for IpcFramesOutLinkRTOS_CreateParams_Init
 *
 */
static inline Void IpcFramesOutLinkRTOS_CreateParams_Init(IpcFramesOutLinkRTOS_CreateParams *prm)
{
    IpcLink_CreateParams_Init(&prm->baseCreateParams);
}


/**
 * \brief Sets default values for IpcFramesOutLinkRTOS_CreateParams_Init
 *
 */
static inline Void IpcBitsInLinkRTOS_CreateParams_Init(IpcBitsInLinkRTOS_CreateParams *prm)
{
    IpcLink_CreateParams_Init(&prm->baseCreateParams);
    /* Default operation mode of IpcBitsInLinkRTOS is NoNotify */
    prm->baseCreateParams.noNotifyMode = TRUE;
    prm->baseCreateParams.notifyPrevLink = FALSE;

}

/**
 * \brief Sets default values for IpcBitsOutLinkRTOS_CreateParams_Init
 *
 */
static inline Void IpcBitsOutLinkRTOS_CreateParams_Init(IpcBitsOutLinkRTOS_CreateParams *prm)
{
    IpcLink_CreateParams_Init(&prm->baseCreateParams);
    /* Default operation mode of IpcBitsOutLinkRTOS_CreateParams_Init is NoNotify */
    prm->baseCreateParams.noNotifyMode = TRUE;
    prm->baseCreateParams.notifyNextLink = FALSE;

}

/**
 * \brief Sets default values for IpcBitsInLinkHLOS_CreateParams_Init
 *
 */
static inline Void IpcBitsInLinkHLOS_CreateParams_Init(IpcBitsInLinkHLOS_CreateParams *prm)
{
    IpcLink_CreateParams_Init(&prm->baseCreateParams);
    /* Default operation mode of BitsInHLOS is NoNotify */
    prm->baseCreateParams.noNotifyMode = TRUE;
    prm->baseCreateParams.notifyPrevLink = FALSE;
    prm->baseCreateParams.notifyNextLink = FALSE;
    /* BitInHLOS link has no next link. So set link ID to invalid */
    prm->baseCreateParams.outQueParams[0].nextLink = SYSTEM_LINK_ID_INVALID;
    prm->baseCreateParams.numOutQue = 0;

    prm->cbCtx = NULL;
    prm->cbFxn = NULL;
}

/**
 * \brief Sets default values for IpcBitsOutLinkHLOS_CreateParams_Init
 *
 */
static inline Void IpcBitsOutLinkHLOS_CreateParams_Init(IpcBitsOutLinkHLOS_CreateParams *prm)
{
    UInt32 i;

    IpcLink_CreateParams_Init(&prm->baseCreateParams);
    /* Default operation mode of BitsOutHLOS is NoNotify */
    prm->baseCreateParams.noNotifyMode = TRUE;
    prm->baseCreateParams.notifyPrevLink = FALSE;
    prm->baseCreateParams.notifyNextLink = FALSE;
    /* BitOutHLOS link has no previous link. So set link ID to invalid */
    prm->baseCreateParams.inQueParams.prevLinkId= SYSTEM_LINK_ID_INVALID;
    prm->baseCreateParams.numOutQue = 1;
    prm->inQueInfo.numCh = 0;
    prm->bufPoolPerCh    = FALSE;
    memset(&prm->inQueInfo.chInfo,0,sizeof(prm->inQueInfo.chInfo));
    /* when set 0, IpcBitsOutLink will take default value based on system
       defined default on BIOS side */
    for (i=0; i<IPC_LINK_BITS_OUT_MAX_NUM_ALLOC_POOLS; i++)
    {
        prm->numBufPerCh[i] = 0;
    }
}

static inline Void IpcBitsOutLinkHLOS_BitstreamBufReqInfo_Init(
                                  IpcBitsOutLinkHLOS_BitstreamBufReqInfo *prm)
{
    prm->numBufs = 0;
    prm->reqType = IPC_BITSOUTHLOS_BITBUFREQTYPE_BUFSIZE;
}


#endif

/*@}*/

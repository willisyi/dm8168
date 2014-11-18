/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup AVSYNC_LINK_API
    \addtogroup AVSYNC_RTOS_LINK_API    AVSYNC Link API (RTOS)


    The audio video synchronization functionality's interface exported
    to other links such as Software Mosaic link running on the
    slave core

    Interface to Avsync module to be included by other links
    like SwMs running on the slave core (VPSS_M3)

    @{
*/

/**
    \file  avsync_rtos.h
    \brief AVSYNC Link API (RTOS)
*/


#ifndef AVSYNC_RTOS_H_
#define AVSYNC_RTOS_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>      // for  FVID2_Frame
#include <mcfw/interfaces/link_api/avsync.h>
#include <mcfw/interfaces/link_api/avsync_internal.h>
#include <mcfw/src_bios6/utils/utils.h>

/**
 * \brief Function Pointer to Video SYnch Callback function which
 *        will be invoked by AVSYNC from driver's video synch context
 *
 * @param  ctx Context associated with the callback
 * @return None
 */
typedef Void (*AvsyncVidSyncCbFxn)(Ptr ctx);

/**
 * \brief Avsync Video Synch Callback Structure
 *
 * Links can register for callback at video synch interrupt.
 * THis structure has info that is used at the time
 * of registering for the callback.
 */
typedef struct AvsyncLink_VidSyncCbHookObj
{
    AvsyncVidSyncCbFxn cbFxn;
    /**< Video Synch Callback function to be invoked */
    Ptr                ctx;
    /**< Video Synch callback context .Arg of cbFxn */
} AvsyncLink_VidSyncCbHookObj;


/**
 * \brief Avsync Display Object
 *
 * Display Object is one per independent display device
 * and store info related to VSYNC interrupt
 */
typedef struct AvsyncLink_DisplayObj
{
    UInt64           vSyncSTC;
    /**< WallTime when the last VSync interrupt occured */
    UInt32           videoBackendDelay;
    /**< Video backend delay in ms. Counted from
     *   when frame is dequeued from AVSYNC video queue (in SwMs)
     *   to the instant when frame is actually displayed
     */
    AvsyncLink_VidSyncCbHookObj cbHookObj;
    /**< Video Synch Callback hook registered with this
     *   display instance */
} AvsyncLink_DisplayObj;


/**
 * \brief Avsync Video Queue Create parameters
 *
 * Video Queue handles synchronization of video
 * frames with the system time.
 */
typedef struct AvsyncLink_VidQueCreateParams
{
    UInt32                syncLinkID;
    /**< LinkID from which synchronization APIS
     *   Avsync_vidQueGet/Avsync_vidQuePut  are invoked.
     */
    UInt32                displayID;
    /**< Display linkID with which this AVSYNC video queue
     *   is associated
     */
    UInt32                chNum;
    /**< Channel number of the frames fed to this queue */
    UInt32                maxElements;
    /**< Maximum number of elements that can be queued */
    FVID2_Frame        ** queueMem;
    /**< Memory for the queue */
} AvsyncLink_VidQueCreateParams;


/**
 * \brief Avsync Video Queue Object
 *
 * Video Queue handles synchronization of video
 * frames with the system time.
 * The AvsyncLink_VidQueObj stores the state associated
 * with the video queue
 */
typedef struct AvsyncLink_VidQueObj
{
    AvsyncLink_VidQueCreateParams cp;
    /**< Create params associated with this queue */
    Avsync_SynchConfigParams    *cfg;
    /**< Pointer to the AVSYNC configuration for
     *   this video queue.
     */
    Utils_QueHandle        vidFrmQ;
    /**< Handle to the underlying queue handle */
    Avsync_PlayerTimeObj   *playerTime;
    /**< Pointer to the playerTime object associated with
     *   this video queue
     */
    AvsyncLink_DisplayObj  *displayObj;
    /**< Pointer to the display object associated with
     *   this video queue
     */
    Avsync_RefClkObj       *refClk;
    /**< Pointer to the reference clock object associated with
     *   this video queue
     */
    Avsync_VidStats        *stats;
    /**< Pointer to statistics structure */
    UInt32                 state;
    /**< State of the video frame queue */
    UInt32                 playTimerStartTimeout;
    /**< Value of the playback start timeout. On expiry
     * of timeout playback will commence irrespective
     * of video frame received or not
     */
    UInt32                 displayID;
    /**< DisplayID associated with this video queue */
    UInt32                 syncMasterChnum;
    /**< Channel number of synch master which drives reference clock */
#if (AVSYNC_APPLY_PTS_SMOOTHING_FILTER)
    Avsync_SimpleMovingAvgObj ptsDiffAvg;
    /**< Structure used to apply smoothing filter over PTS.
     *   Used only if AVSYNC_APPLY_PTS_SMOOTHING_FILTER define
     *   is enabled
     */
#endif

} AvsyncLink_VidQueObj;



/**
 * \brief Macro used to check if AVSYNC is enabled for a particular video queue
 */
#define AVSYNC_VIDQUE_IS_SYNCH_ENABLED(queObj)           ((((queObj)->cfg) && ((queObj)->cfg->avsyncEnable)) \
                                                          ? TRUE                                             \
                                                          : FALSE)


/**
 * @brief AVSYNC link initialization function invoked at system initialization time of M3VPSS
 */
Int32 AvsyncLink_init();

/**
 * @brief AVSYNC link deinitialization function invoked at system shutdown time of M3VPSS
 */
Int32 AvsyncLink_deInit();

/**
 * @brief Initialize default values for VideoQue Create Params.
 *
 * This API should be invoked before Avsync_vidQueCreate is called
 * so that AvsyncLink_VidQueCreateParams is intiialized correctly
 */
Void Avsync_vidQueCreateParamsInit(AvsyncLink_VidQueCreateParams *cp);


/**
 * @brief Create AVSYNC video queue
 *
 * @param cp      Create Params
 * @param queObj  Queue Handle which will be created
 *
 * @return AVSYNC_S_OK on success
 */
Int Avsync_vidQueCreate(AvsyncLink_VidQueCreateParams *cp,
                        AvsyncLink_VidQueObj *queObj);
/**
 * @brief Delete AVSYNC video queue
 *
 * @param queObj  Queue Handle which will be deleted
 *
 * @return AVSYNC_S_OK on success
 */
Int Avsync_vidQueDelete(AvsyncLink_VidQueObj *queObj);

/**
 * @brief Queue a frame into the the video synch queue
 *
 * @param queObj  Queue Handle
 * @param frame   Frame to be queued
 *
 * @return AVSYNC_S_OK on success
 */
Int Avsync_vidQuePut(AvsyncLink_VidQueObj *queObj,
                     FVID2_Frame *frame);

/**
 * @brief DeQueue a frame into the the video synch queue
 *
 * This function will return a synchronized video
 * frame
 *
 * @param queObj     Queue Handle
 * @param forceGet   Flag indicating AVSYNC should be disregarded
 *                   and a frame should be returned if queue is
 *                   not empty
 * @param framePtr   Pointer to FVID2_Frame pointer.
 *                   If a frame is ready for display it will be
 *                   non-null on return else it will be NULL.
 * @param freeFrameList List of frames which are to be skipped
 *
 * @return AVSYNC_S_OK on success
 */
 Int Avsync_vidQueGet(AvsyncLink_VidQueObj *queObj,
                     Bool  forceGet,
                     FVID2_Frame **framePtr,
                     FVID2_FrameList *freeFrameList);

/**
 * @brief Return number of frame queued
 *
 * @param  queObj     Queue Handle
 * @return Number of frame queued
 */
UInt32 Avsync_vidQueGetQueLength(AvsyncLink_VidQueObj *queObj);

/**
 * @brief Returns TRUE if video queue is empty else FALSE
 *
 * @param  queObj     Queue Handle
 * @return TRUE if queue is empty else FALSE
 */
UInt32 Avsync_vidQueIsEmpty(AvsyncLink_VidQueObj *queObj);

/**
 * @brief Returns the channel number of the synch master
 *
 * The reference clock is driven by the synch master channel.
 * This function returns this channel number
 *
 * @param  syncLinkID   LinkID from which the Avsync_vidQueGet/Avsync_vidQuePut
 *                      will be invoked
 * @return Synch master channel number
 */
UInt32 Avsync_vidQueGetMasterSynchChannel(UInt32  syncLinkID);

/**
 * @brief Returns the displayID associated with the SwMs link
 *
 * @param  syncLinkID   LinkID from which the Avsync_vidQueGet/Avsync_vidQuePut
 *                      will be invoked
 * @return DisplayID associated with the SwMs link
 */
UInt32 Avsync_vidQueGetDisplayID(UInt32  syncLinkID);


/**
 * @brief Flush the video synch queue
 *
 * On invoking flush all frame queued will be returned in the freeFrameList.
 * The last frame in the queue will be set in *framePtr
 *
 * @param  queObj     Queue Handle
 * @param  framePtr   Pointer to Pointer to  FVID_Frame. Will be set to last
 *                    frame in the queue
 * @param  freeFrameList Array of frames which are flushed.
 * @return DisplayID associated with the SwMs link
 */
Int Avsync_vidQueFlush(AvsyncLink_VidQueObj *queObj,
                       FVID2_Frame **framePtr,
                       FVID2_FrameList *freeFrameList);

/**
 * @brief Function invoked from Video Synch ISR indicating VSYNC event
 *
 * @param displayID DisplayID for which VSYNC is occuring.
*/
Void Avsync_vidSynchCallbackFxn(UInt32 displayID);

/**
 * @brief Function to map displayLinkID to display number,
 *
 * DisplayLinks span from SYSTEM_LINK_ID_DISPLAY_FIRST to SYSTEM_LINK_ID_DISPLAY_LAST
 * This needs to be mapped to a display number from 0 - (AVSYNC_MAX_NUM_DISPLAYS - 1)
 * This function does this mapping
 *
 * @param  linkID DisplayLink ID to be mapped
 * @return Corresponding display index
*/
UInt32 Avsync_mapDisplayLinkID2Index(UInt32 linkID);


/**
    \brief Used to log the timestamp of frames captured at capture link

    THis is a API for statistics and debug purposes and is
    invoked inside the Capture link.

    \param chNum         Capture link channel number
    \param ts            Timestamp value that needs to be logged

    \return None
*/
Void AvsyncLink_logCaptureTS(UInt32 chNum, UInt64 ts);

/**
 *   \brief Used to register for callback from AVSYNC when the VideoSYNCH event occurs
 *
 *  The display will invoke Avsync_vidSynchCallbackFxn when the Video Synch event occurs.
 *  Avsync provides an option for any link to register for a callback when this
 *  event occurs
 *
 *  \param displayID     Display number for which VSYNC callback is being registered
 *  \param cbHookObj     Callback info
 *
 *  \return None
*/
Void Avsync_vidSynchRegisterCbHook(UInt32 displayID,
                                   AvsyncLink_VidSyncCbHookObj *cbHookObj);

#endif /* AVSYNC_RTOS_H_ */

/* @} */

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup AVSYNC_LINK_API
    \addtogroup AVSYNC_HLOS_LINK_API    AVSYNC Link API (HLOS)

    The audio video synchronization functionality's high level OS
    interface is exported in this file

    Interface to Avsync module to be included on HLOS side.

    @{
*/

/**
    \file  avsync_hlos.h
    \brief AVSYNC Link API (HLOS)
*/



#ifndef AVSYNC_HLOS_H_
#define AVSYNC_HLOS_H_

#include <osa.h>
#include <osa_que.h>
#include <mcfw/interfaces/ti_media_std.h>
#include <mcfw/interfaces/ti_media_error_def.h>
#include <mcfw/interfaces/link_api/avsync.h>
#include <mcfw/interfaces/link_api/avsync_internal.h>
#include <demos/audio_sample/audio.h>

/**
    \brief Information about Audio Backend required by AVSYNC

    The application passes information related to the
    rendering of audio frames to the
    avsync module when invoking Avsync_audQueGet
*/
typedef struct AvsyncLink_AudioBackEndInfo
{
    UInt64           aSyncSTC;
    /**< Wall Time when the last audio backend interrupt
     *   was received.
     */
    UInt32           audioBackendDelay;
    /**< Audio backend delay in milliseconds which
     * should be accounted for in Avsync calculation
     */
} AvsyncLink_AudioBackEndInfo;


/**
    \brief Avsync Audio Queue Create params

    The application uses an AVsync audio que
    to perform synchronization of audio
    This structure defines the create param
    for the audio queue
*/
typedef struct AvsyncLink_AudQueCreateParams
{
    UInt32                audioDevID;
    /**< Audio device identifier.
     * Valid values: 0 - (AVSYNC_MAX_NUM_AUDIO_PLAYOUT_DEVICES - 1) */
    UInt32                chNum;
    /**< Audio Channel Number. If this audio channel matches the
     * master synch channel then the reference clock will
     * be synchronized to the audio PTS of this stream
     */
    UInt32                maxElements;
    /**< Maximum number of elements that can be queued in this
     * audio queue. Trying to queue greater than this
     * value will result in Avsync_audQuePut failing
     */
} AvsyncLink_AudQueCreateParams;

/**
    \brief Avsync Audio Queue Object

    The Avsync uses a special audio queue
    to perform synchronization of audio.
    This structure defines the audio queue
    object.
*/
typedef struct AvsyncLink_AudQueObj
{
    AvsyncLink_AudQueCreateParams cp;
    /**< Create params associated with this Avsync audio queue */
    OSA_QueHndl            audFrmQ;
    /**< Underlying OSA_que used to maintain the audio frames FIFO */
    Avsync_PlayerTimeObj   *playerTime;
    /**< Pointer to playerTime structure. This will be populated
     *   if this channel is the synch master channel
     */
    Avsync_SynchConfigParams *cfg;
    /**< Pointer to synch config params structure.This will be populated
     *   if this channel is the synch master channel
     */
    Avsync_AudStats        *stats;
    /**< Pointer to the avsync audio statistics */
    Avsync_RefClkObj       *refClk;
    /**< Pointer to the reference clock object */
    UInt32                 state;
    /**< Internal state of the audio queue */
    UInt32                 playTimerStartTimeout;
    /**< Timeout value in msec for playback start.
     *   At start of playback Avsync module will
     *   delay playback start until it receives
     *   the first audio and video frame.
     *   If the playTimerStartTimeout playback will
     *   commence irrespective of first video frame
     *   received
     */
    UInt32                 videoStreamDisplayID;
    /**< Display device ID with which this audio channel
     *   is synchronized
     */
    UInt32                 syncMasterChnum;
    /**< Sync master channel number */
} AvsyncLink_AudQueObj;


/**
    \brief AVsync HLOS link initialization function.

    Invoked at application startup
    \return AVSYNC_S_OK on success
*/
Int32 AvsyncLink_init();

/**
    \brief AVsync HLOS link deinitialization function.

    Invoked at application shutdown
    \return AVSYNC_S_OK on success
*/
Int32 AvsyncLink_deInit();

/**
    \brief AVsync Audio Queue Create function

    \param cp     Create Params of type AvsyncLink_AudQueCreateParams
    \param queObj Pointer to queObj which will be created

    \return AVSYNC_S_OK on success
*/
Int Avsync_audQueCreate(AvsyncLink_AudQueCreateParams *cp,
                        AvsyncLink_AudQueObj *queObj);

/**
    \brief AVsync Audio Queue Delete function

    \return AVSYNC_S_OK on success
*/
Int Avsync_audQueDelete(AvsyncLink_AudQueObj *queObj);

/**
    \brief AVsync Audio Queue Put

    Queue a frame into the avsync audio queue

    \param queObj Handle to the audio queue object
    \param frame  Pointer to the audio frame to be queued

    \return AVSYNC_S_OK on success
*/
Int Avsync_audQuePut(AvsyncLink_AudQueObj *queObj,
                     AudFrm_Buf *frame);

/**
    \brief AVsync Audio Queue Get

    DeQueue a frame from the avsync audio queue

    \param queObj         Handle to the audio queue object
    \param framePtr       Pointer to the audio frame ptr
    \param freeFrameList  List of frames to be freed (Skipped frames)
    \param abeInfo        Info related to Audio Backend (Audio Playout) used for synchronization

    \return AVSYNC_S_OK on success
*/
Int Avsync_audQueGet(AvsyncLink_AudQueObj *queObj,
                     AudFrm_Buf **framePtr,
                     AudFrm_BufList *freeFrameList,
                     AvsyncLink_AudioBackEndInfo *abeInfo);

/**
    \brief API invoked by the application until playback starts.

    At the start of playback, the playback of audio will be
    delayed until the first video frame is ready for playback.
    This is to enable playback to commence in a synchronized
    manner.This API can be invoked by the application which
    will block until audio is ready to start playback
    or timeout occurs.

    \param queObj         Handle to the audio queue object
    \param timeOut        TImeout value in msecs

    \return AVSYNC_S_OK on success
*/
Bool Avsync_audQueWaitForPlayerStart(AvsyncLink_AudQueObj *queObj,
                                     Int32 timeOut);

/**
    \brief Get the current player state for the avsync audio queue

    \param queObj         Handle to the audio queue object

    \return AVSYNC Player state enum Avsync_PlayerTimeState
*/
Avsync_PlayerTimeState Avsync_audQueGetPlayerState(AvsyncLink_AudQueObj *queObj);

/**
    \brief Used to log the timestamp of frames fed to IpcBitsOut_HLOS link.

    THis is a API for statistics and debug purposes and is
    invoked inside the ipcBitsOutHLOS link.

    \param chNum         IpcBitsOutlink channel number
    \param ts            Timestamp value that needs to be logged

    \return None
*/
Void AvsyncLink_logIpcBitsOutTS(UInt32 chNum, UInt64 ts);

#endif /* AVSYNC_HLOS_H_ */

/* @} */

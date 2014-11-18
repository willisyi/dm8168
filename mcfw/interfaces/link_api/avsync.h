/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup AVSYNC_LINK_API AVSYNC Link API

    Avsync link provides audio <-> video and video <-> video synch functionality

    The following functionality are supported by this link
    - Synchronization of system time to audio reference clock
    - Synchronization of system time to video reference clock
    - No clock adjust mode.
    - Trick Play functionality
      - Pause
      - Slow Play
      - Fast Play
      - Step Forward
      - Scan mode playback (I-frame only display)
      - Seek

    @{
*/

/**
    \file  avsync.h
    \brief AVSYNC Link API
*/

#ifndef _AVSYNC_H_
#define _AVSYNC_H_

/* Include's    */
#include <mcfw/interfaces/link_api/system_linkId.h>
#include <mcfw/interfaces/link_api/swMsLink.h>

/* Define's */

/* @{ */
/**
    AVSYNC_LINK_API Error codes
@{
*/
/**
 * \brief AVSYNC Error Code: Success
 */
#define AVSYNC_S_OK                                                         (0)

/**
 * \brief AVSYNC Error Code: Generic failure
 */
#define AVSYNC_E_FAIL                                                       (-1)

/**
 * \brief AVSYNC Error Code: Resource allocation(memory alloc) failure
 */
#define AVSYNC_E_INSUFFICIENTRESOURCES                                      (-2)

/**
 * \brief AVSYNC Error Code: Invalid param passed by application
 */
#define AVSYNC_E_INVALIDPARAMS                                              (-3)

/* @} */

/**
 * \brief Maximum number of independent audio playout devices.
 *
 * Each independent audio playout device can be configured as a reference clock master
 */
#define AVSYNC_MAX_NUM_AUDIO_PLAYOUT_DEVICES                                      (2)

/**
 * \brief Maximum number of independent display devices.
 *
 * Each independent video display device needs to be synchronized
 */
#define AVSYNC_MAX_NUM_DISPLAYS                                                   (SYSTEM_LINK_ID_DISPLAY_COUNT)

/**
 * \brief Maximum number of channels per display device
 *
 * A display device may be fed multiple input channels. Each input channel
 * will be synched to the reference clock when the input channels are
 * composed (Mosaiced).
 */
#define AVSYNC_MAX_CHANNELS_PER_DISPLAY                                           (SYSTEM_SW_MS_MAX_CH_ID)

/**
 * \brief Spl channel id indicating all channels associated with a display device.
 *
 * Avsync link cmds can be applied to each individual channel or can be
 * applied to all channels associated with a particular display device.
 * This define is a spl channelId value indicating cmd applies to all channels
 * associated with a particual display device
*/
#define AVSYNC_ALL_CHANNEL_ID                                                     (AVSYNC_MAX_CHANNELS_PER_DISPLAY + 1)

/**
 * \brief Default value for max video lag in ms.
 *
 * The avsync logic compares the presentation timestamp (PTS) with the current
 * system time.
 * Lag indicates SystemTime - PTS is negative. (Past frame)
 *    -- Max lag is the max threshold value .If lag is within this define it
 *       will be selected for display.
 *       A past frame within AVSYNC_VIDEO_PLAY_MAX_LAG_MS will be selected for
 *       rendering
 *
*/
#define AVSYNC_VIDEO_PLAY_MAX_LAG_MS                                              (45)

/**
 * \brief Default value for max video lead in ms.
 *
 * The avsync logic compares the presentation timestamp (PTS) with the current
 * system time.
 * Lead indicates SystemTime - PTS is positive. (Future frame)
 *    -- Max lead is the max threshold value .If lead is within this define it
 *       will be selected for display.
 *       A future frame within AVSYNC_VIDEO_PLAY_MAX_LEAD_MS will be selected for
 *       rendering
*/
#define AVSYNC_VIDEO_PLAY_MAX_LEAD_MS                                             (0)

/**
 * \brief Default value for max future frame drop threshold in ms.
 *
 * A frame in the future is usually replayed.However if the frame is beyond
 * AVSYNC_VIDEO_FUTURE_FRAME_DROP_THRESHOLD_MS in the future is will be dropped.
*/
#define AVSYNC_VIDEO_FUTURE_FRAME_DROP_THRESHOLD_MS                               (750)

/**
 * \brief Default value for max lead of audioPTS that will result in refclk adjustment
 *
 * When audio is selected as reference clock master, the audio PTS of the
 * frame to be played is compared with the current reference clock time
 * and if the lead is greater than AVSYNC_AUDIO_REFCLKADJUST_MAX_LEAD_MS
 * reference clock will be adjusted to the audioPTS
*/
#define AVSYNC_AUDIO_REFCLKADJUST_MAX_LEAD_MS                                     (45)

/**
 * \brief Default value for max lag of audioPTS that will result in refclk adjustment
 *
 * When audio is selected as reference clock master, the audio PTS of the
 * frame to be played is compared with the current reference clock time
 * and if the lag is greater than AVSYNC_AUDIO_REFCLKADJUST_MAX_LAG_MS
 * reference clock will be adjusted to the audioPTS
*/
#define AVSYNC_AUDIO_REFCLKADJUST_MAX_LAG_MS                                      (45)

/**
 * \brief Default value for max lead of videoPTS that will result in refclk adjustment
 *
 * When video is selected as reference clock master, the video PTS of the
 * frame to be played is compared with the current reference clock time
 * and if the lead is greater than AVSYNC_VIDEO_REFCLKADJUST_MAX_LEAD_MS
 * reference clock will be adjusted to the videoPTS
*/
#define AVSYNC_VIDEO_REFCLKADJUST_MAX_LEAD_MS                                     (120)

/**
 * \brief Default value for max lag of videoPTS that will result in refclk adjustment
 *
 * When video is selected as reference clock master, the video PTS of the
 * frame to be played is compared with the current reference clock time
 * and if the lag is greater than AVSYNC_VIDEO_REFCLKADJUST_MAX_LAG_MS
 * reference clock will be adjusted to the videoPTS
*/
#define AVSYNC_VIDEO_REFCLKADJUST_MAX_LAG_MS                                      (120)

/**
 * \brief Default value for max lead of videoPTS that will result in refclk adjustment
 *
 * When refclk is configured for AVSYNC_REFCLKADJUST_NONE mode, the reference clock
 * once started is freerunning and is never adjusted.However if the difference
 * between the videoPTS and reference clock is greater than AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LEAD_MS
 * then it indicates the timebase of the video stream has changed and the
 * reference clock is adjusted.
*/
#define AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LEAD_MS                                    (3000)

/**
 * \brief Default value for max lag of videoPTS that will result in refclk adjustment
 *
 * When refclk is configured for AVSYNC_REFCLKADJUST_NONE mode, the reference clock
 * once started is freerunning and is never adjusted.However if the difference
 * between the videoPTS and reference clock is less than AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LAG_MS
 * then it indicates the timebase of the video stream has changed and the
 * reference clock is adjusted.
*/
#define AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LAG_MS                                     (3000)

/**
 * \brief Default value for video backend delay in ms
 *
 * Video Synchronization is performned in the compositor component (SwMs).
 * At point of composition the avsync logic needs to account for backed delay
 * from the point of composition to when a frame is actually displayed on screen
 * so that synchronization is correct. This is called video backend delay
 * AVSYNC_VIDEO_BACKEND_DELAY_MS is the default value for video backend delay
*/
#define AVSYNC_VIDEO_BACKEND_DELAY_MS                                             (50)

/**
 * \brief Timescale value for normal 1x playback
 *
 * Timescale value is used to control fast and slow playback.
 * AVSYNC_TIMESCALE_NORMAL_PLAY determines value for 1x playback.
*/
#define AVSYNC_TIMESCALE_NORMAL_PLAY                                              (1000)

/* Control Command's    */

/**
    \ingroup LINK_API_CMD
    \addtogroup AVSYNC_LINK_API_CMD AVSYNC Link Control Commands

    @{
*/


/**
    \brief Link CMD: Initialize the avsync link

    This is the first avsync cmd that should be sent.
    This will allocate avsync data structures and initialize the
    wall time for the system.

    Avsync_InitParams.wallTimeInitTS specifies the initial value of wall time

    \param Avsync_InitParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_INIT                                                      (0xA000)

/**
    \brief Link CMD: Deinitialize the avsync link

    This will free avsync data structures and stop the wall time.
    No further avsync cmds can be sent until a AVSYNC_LINK_CMD_INIT is
    sent again

    \param NONE
*/
#define AVSYNC_LINK_CMD_DEINIT                                                    (0xA001)

/**
    \brief Link CMD: Pause playback

    Avsync_PauseParams.displayID DisplayID to which pause cmd applies
    Avsync_PauseParams.chNum channel number within the displayID to which pause cmd applies.
    If chNum is set to AVSYNC_ALL_CHANNEL_ID it applies to all channels in the
    display device

    \param Avsync_PauseParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_PAUSE                                                     (0xA002)

/**
    \brief Link CMD: Unpause playback

    Avsync_UnPauseParams.displayID DisplayID to which unpause cmd applies
    Avsync_UnPauseParams.chNum channel number within the displayID to which pause cmd applies.
    If chNum is set to AVSYNC_ALL_CHANNEL_ID it applies to all channels in the
    display device

    \param Avsync_UnPauseParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_UNPAUSE                                                   (0xA003)

/**
    \brief Link CMD: Timescale(Fast/Slow) playback

    Avsync_TimeScaleParams.displayID DisplayID to which this cmd applies
    Avsync_TimeScaleParams.chNum Channel number within the displayID to which this cmd applies.
    Avsync_TimeScaleParams.timeScaleX1000 Timescale value to be applied.
    Avsync_TimeScaleParams.displaySeqId   SequenceID that will be associated with frames
                                          received in this state
                                          After this cmd is received any received frame
                                          with associated sequenceID not matching displaySeqId
                                          will not be displayed

    \param Avsync_TimeScaleParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_TIMESCALE                                                 (0xA004)

/**
    \brief Link CMD: Get Avsync shared region obj pointer value.

    This is an avync internal cmd used to get the Avsync Shared Object pointer
    on another core

    Avsync_GetSrObjParams.srObjSrPtr SrPtr value of the AVsync shared object

    \param Avsync_GetSrObjParams *   [OUT]  parameters
*/
#define AVSYNC_LINK_CMD_GETSROBJ                                                  (0xA005)

/**
    \brief Link CMD: Avsync Configuration cmd.

    This is the cmd that configures all avsync parameters for all the channels
    associated with a display device. This cmd must be sent before the
    compositor link (SwMs) associated with the display device is created

    \param AvsyncLink_LinkSynchConfigParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_AVSYNCCFG                                                 (0xA006)

/**
    \brief Link CMD: Get Video Synch channel info for an audio device

    This is an avsync internal cmd used to get the master synch info for a channel
    when the associated audio device id is passed

    \param Avsync_MasterSynchInfo *   [IN/OUT]  parameters
*/
#define AVSYNC_LINK_CMD_GETVIDSYNCHCHINFO                                         (0xA007)

/**
    \brief Link CMD: Step forward by one video frame

    Avsync_StepFwdParams.displayID DisplayID to which this cmd applies
    Avsync_StepFwdParams.chNum channel number within the displayID to which this cmd applies.
    If chNum is set to AVSYNC_ALL_CHANNEL_ID it applies to all channels in the
    display device

    \param Avsync_StepFwdParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_STEP_FWD                                                  (0xA008)

/**
    \brief Link CMD: Set the first video PTS of  a sequence.

    When Avsync_SynchConfigParams.ptsInitMode is set to AVSYNC_PTS_INIT_MODE_APP,
    the application should set the firstVideoPTS of the sequence .
    This cmd enables the application to set the first video PTS of  a sequence
    Avsync_FirstVidPTSParams.displayID DisplayID to which this cmd applies
    Avsync_FirstVidPTSParams.chNum channel number within the displayID to which this cmd applies.
    If chNum is set to AVSYNC_ALL_CHANNEL_ID it applies to all channels in the
    display device
    Avsync_FirstVidPTSParams.firstVidPTS PTS of the first frame in the sequence.

    \param Avsync_FirstVidPTSParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_SET_FIRST_VIDPTS                                          (0xA009)

/**
    \brief Link CMD: Set the first audio PTS of  a sequence.

    When Avsync_SynchConfigParams.ptsInitMode is set to AVSYNC_PTS_INIT_MODE_APP,
    the application should set the firstAudioPTS of the sequence .
    This cmd enables the application to set the first audio PTS of  a sequence
    Avsync_FirstAudPTSParams.displayID DisplayID to which this cmd applies
    Avsync_FirstAudPTSParams.chNum channel number within the displayID to which this cmd applies.
    If chNum is set to AVSYNC_ALL_CHANNEL_ID it applies to all channels in the
    display device
    Avsync_FirstAudPTSParams.firstAudPTS PTS of the first frame in the sequence.

    \param Avsync_FirstAudPTSParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_SET_FIRST_AUDPTS                                          (0xA00A)

/**
    \brief Link CMD: Set the Player state to Reset state.

    In Reset state player time is not initialized and no frames are given for display
    by avsync. This is an internal API and reset is used as an intermediate state
    to perform certain trasitions like seek
    Avsync_ResetPlayerTimerParams.displayID DisplayID to which this cmd applies
    Avsync_ResetPlayerTimerParams.chNum Channel number within the displayID to which this cmd applies.

    \param Avsync_ResetPlayerTimerParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_RESET_PLAYERTIME                                          (0xA00B)

/**
    \brief Link CMD: Configures the video backend delay

    Video Synchronization is performned in the compositor component (SwMs).
    At point of composition the avsync logic needs to account for backed delay
    from the point of composition to when a frame is actually displayed on screen
    so that synchronization is correct. This is called video backend delay
    This API allows application to configure the backend delay value.
    Avsync_VideoBackendDelayParams.displayID DisplayID to which this cmd applies
    Avsync_VideoBackendDelayParams.backendDelayMS Backend delay in ms.

    \param Avsync_VideoBackendDelayParams *   [IN]  parameters
*/
#define AVSYNC_LINK_CMD_SET_VIDEO_BACKEND_DELAY                                   (0xA00C)

/**
    \brief Link CMD: Set the Player state to Play state.

    Setting the player state to play will cause normal playback to resume.
    This cmd will be used when resuming normal playback from reset state
    Avsync_PlayParams.displayID     DisplayID to which this cmd applies
    Avsync_PlayParams.chNum         Channel number within the displayID to which this cmd applies.
    Avsync_PlayParams.displaySeqId  SequenceID that will be associated with frames
                                    received in this state
                                    After this cmd is received any received frame
                                    with associated sequenceID not matching displaySeqId
                                    will not be displayed
*/
#define AVSYNC_LINK_CMD_PLAY                                                      (0xA00D)

/**
    \brief Link CMD: Resume playback at seek point.

    This command is used by the application to resume playback at a different instance
    of time. Application is responsible for feeding the stream close to the new
    seek point. Avsync will resume playback at the application specified PTS.
    Avsync_SeekParams.displayID     DisplayID to which this cmd applies
    Avsync_SeekParams.chNum         Channel number within the displayID to which this cmd applies.
    Avsync_SeekParams.displaySeqId  SequenceID that will be associated with frames
                                    received in this state
                                    After this cmd is received any received frame
                                    with associated sequenceID not matching displaySeqId
                                    will not be displayed
*/
#define AVSYNC_LINK_CMD_SEEK                                                      (0xA00E)

/**
    \brief Link CMD: Set playback state to scan mode.

    In scan mode it is expected that application will feed only I-frames to
    accomplish forward / reverse playback at a high speeds.
    Avsync will display each frame for a configured duration of time.
    Avsync_ScanParams.displayID     DisplayID to which this cmd applies
    Avsync_ScanParams.chNum         Channel number within the displayID to which this cmd applies.
    Avsync_ScanParams.displaySeqId  SequenceID that will be associated with frames
                                    received in this state
                                    After this cmd is received any received frame
                                    with associated sequenceID not matching displaySeqId
                                    will not be displayed
    Avsync_ScanParams.frameDisplayDurationMS  Duration in ms each fram should be displayed.
*/
#define AVSYNC_LINK_CMD_SCAN                                                      (0xA00F)
/* @} */

/**
    \brief Maximum System time value after which wrap around is expected.
*/
#define AVSYNC_MAX_STC_VALUE                                                ((UInt64)(0xFFFFFFFFULL))

/**
    \brief Spl define indicating invalid channel number
*/
#define AVSYNC_INVALID_CHNUM                                                (~(0u))

/**
    \brief Spl define indicating invalid presentation timestamp.
*/
#define AVSYNC_INVALID_PTS                                                  ((UInt64)(~(0ULL)))

/**
    \brief Spl define indicating invalid audio device ID
*/
#define AVSYNC_INVALID_AUDDEVID                                             (~(0u))

/**
    \brief Spl define indicating invalid display device ID
*/
#define AVSYNC_INVALID_DISPLAY_ID                                              (~(0u))


/**
    \brief Types of Reference clock masters.

    Avsync supports adjustment of reference clock by different masters.
    This enum lists the different supported reference clock masters
*/
typedef enum
{
    AVSYNC_REFCLKADJUST_BYAUDIO = 0,
    /**< Audio PTS will be used as master to  adjust reference clock */
    AVSYNC_REFCLKADJUST_BYVIDEO = 1,
    /**< One of the video channel's PTS will be used as master to  adjust reference clock */
    AVSYNC_REFCLKADJUST_NONE    = 2,
    /**< Once initialized reference clock will be free running and not adjusted unless there is
     *   a change in timebase */
    AVSYNC_REFCLKADJUST_LAST = 0x7FFFFFFF
}Avsync_RefClkMasterType;

/**
    \brief Types of playback start mode

    This enum determines the behaviour of AVSYNC when the firstPTS is not initialized.
*/
typedef enum Avsync_PlaybackStartMode
{
    AVSYNC_PLAYBACK_START_MODE_WAITSYNCH,
    /**< The AVSYNC module will internally sleep until the player time is initialized */
    AVSYNC_PLAYBACK_START_MODE_DROPUNSYNCH,
    /**< The AVSYNC module will drop frames until player time is initialized */
    AVSYNC_PLAYBACK_START_MODE_PLAYUNSYNCH,
    /**< The AVSYNC module will play without synch frames until player time is initialized */
    AVSYNC_PLAYBACK_START_MODE_LAST = 0x80000000
} Avsync_PlaybackStartMode;

/**
    \brief Types of Presentation timestamp initialization methods

    This enum determines the manner by which the first PTS of the
    player is initialized.
*/
typedef enum Avsync_PTSInitMode
{
    AVSYNC_PTS_INIT_MODE_AUTO,
    /**< The AVSYNC module will automatically use the PTS of first frame in the
     *   new sequence as the firstPTS to initialize the player timer */
    AVSYNC_PTS_INIT_MODE_APP,
    /**< The application explicitly sets the firstPTS to initialize the player timer */
    AVSYNC_PTS_INIT_MODE_LAST = 0x80000000
} Avsync_PTSInitMode;

/**
    \brief Avsync AVSYNC_LINK_CMD_PAUSE params
*/
typedef struct Avsync_PauseParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

}Avsync_PauseParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_UNPAUSE params
*/
typedef struct Avsync_PauseParams Avsync_UnPauseParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_TIMESCALE params
*/
typedef struct Avsync_TimeScaleParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

    UInt32 timeScaleX1000;
    /**< Timescale for playback.
     *  1000 indicates 1X playback. - Normal playback
     *  2000 indicates 2X playback  - Fast play 2X
     *  0500 indicates 0.5x playback - Slow play 0.5x
     */
    UInt32 displaySeqId;
    /**< Sequence Id which will be associated with all frames
     *  received in play state.
     *  Application should set it to SYSTEM_DISPLAY_SEQID_DEFAULT
     *  if it doesnt want to change displaySeqId
     */
} Avsync_TimeScaleParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_STEPFWD params
*/
typedef struct Avsync_StepFwdParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

} Avsync_StepFwdParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_FIRSTVIDPTS params
*/
typedef struct Avsync_FirstVidPTSParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

    UInt64 firstVidPTS;
    /**< Presentation timestamp of first video frame in sequence */
} Avsync_FirstVidPTSParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_FIRSTAUDPTS params
*/
typedef struct Avsync_FirstAudPTSParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

    UInt64 firstAudPTS;
    /**< Presentation timestamp of first video frame in sequence */
} Avsync_FirstAudPTSParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_RESET params
*/
typedef struct Avsync_ResetPlayerTimerParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */
} Avsync_ResetPlayerTimerParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_PLAY params
*/
typedef struct Avsync_PlayParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

    UInt32 displaySeqId;
    /**< Sequence Id which will be associated with all frames
     *  received in play state.
     *  Application should set it to SYSTEM_DISPLAY_SEQID_DEFAULT
     *  if it doesnt want to change displaySeqId
     */
} Avsync_PlayParams;


/**
    \brief Avsync AVSYNC_LINK_CMD_SCAN params
*/
typedef struct Avsync_ScanParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

    UInt32 frameDisplayDurationMS;
    /**< Time in ms to display each frame. */

    UInt32 displaySeqId;
    /**< Sequence Id which will be associated with all frames
     *  received in play state.
     *  Application should set it to SYSTEM_DISPLAY_SEQID_DEFAULT
     *  if it doesnt want to change displaySeqId
     */
} Avsync_ScanParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_SEEK params
*/
typedef struct Avsync_SeekParams
{
    UInt32 displayLinkID;
    /**< displayLinkId : SYSTEM_LINK_ID_DISPLAY_FIRST - SYSTEM_LINK_ID_DISPLAY_LAST */

    UInt32 chNum;
    /**< chNum: 0 - (AVSYNC_MAX_CHANNELS_PER_DISPLAY - 1) */

    UInt64 seekAudPTS;
    /**< Audio PTS value from whichh playback should begin */

    UInt64 seekVidPTS;
    /**< Video PTS value from whichh playback should begin */

    UInt32 displaySeqId;
    /**< Sequence Id which will be associated with all frames
     *  received in play state.
     *  Application should set it to SYSTEM_DISPLAY_SEQID_DEFAULT
     *  if it doesnt want to change displaySeqId
     */
} Avsync_SeekParams;

/**
    \brief Avsync statistics related to video playback
*/
typedef struct Avsync_VidStats
{
    UInt32 numFramesRendered;
    /**< Number of frames played */
    UInt32 numFramesReplayed;
    /**< Number of frames replayed */
    UInt32 numFramesSkippedLate;
    /**< Number of frames skipped due to frame PTS being the past */
    UInt32 numFramesSkippedEarly;
    /**< Number of frames skipped due to frame PTS being too ahead in the future */
    UInt32 numUnderrun;
    /**< Number of frames played that should have been skipped but still
     *   displayed as it was the only frame available in the queue for
     *   display */
    UInt32 numOverflow;
    /**< Number of frames played that were dropped because avsync queue
     *   capacity exceeded */
} Avsync_VidStats;

/**
    \brief Avsync statistics related to audio playback
*/
typedef struct Avsync_AudStats
{
    UInt32 numFramesPlayed;
    /**< Number of frames played */
    UInt32 numFramesReplayed;
    /**< Number of frames replayed */
    UInt32 numFramesSkippedEarly;
    /**< Number of frames skipped due to frame PTS being too ahead in the future */
    UInt32 numFramesSkippedLate;
    /**< Number of frames skipped due to frame PTS being in the past */
} Avsync_AudStats;

/**
    \brief Avsync statistics related to player timer adjustments
*/
typedef struct Avsync_SystemClockStats
{
    UInt32 numClockAdjustPositive;
    /**< Number of times positive clock adjustment was done */
    UInt32 numClockAdjustNegative;
    /**< Number of times negative clock adjustment was done */
} Avsync_SystemClockStats;

/**
    \brief Avsync AVSYNC_LINK_CMD_INIT params
*/
typedef struct Avsync_InitParams
{
    UInt64 wallTimeInitTS;
    /**< Initial value to which wall timer will be set */
} Avsync_InitParams;


/**
    \brief Avsync AVSYNC_LINK_CMD_GETSROBJ params
*/
typedef struct Avsync_GetSrObjParams
{
    UInt32 srObjSrPtr;
    /**< Avsync Shared Region object pointer */
} Avsync_GetSrObjParams;

/**
    \brief Avsync Video Schedule policy to determine when a frame should be played.

   \verbatim

   --------------------------------------------------------------------------------
   |             |                                |                  |            |
   |  Skip Late  |            PLAY                |   Replay         | Skip Early |
   |             |                                |                  |            |
   |             v                                v                  v            |
   |         playMaxLag                        playMaxLead         maxReplayLead  |
   --------------------------------------------------------------------------------

   The meaning of various parameters of video schedule policy is illustrated
   above.
   The difference between the PTS of the frame and the reference clock is
   determined and the schedule policy is applied based on the range in
   which the PTS difference falls.

   \endverbatim
*/
typedef struct Avsync_vidSchedulePolicy
{
    Int32 playMaxLag;
    /**< Maximum lag allowed from frame to be selected for display */
    Int32 playMaxLead;
    /**< Maximum lead allowed from frame to be selected for display */
    Int32 maxReplayLead;
    /**< Maximum lead allowed from frame to be selected for replay */
    UInt32 doMarginScaling;
    /**< Flag to indicate application wants the play/replay/skip margins
     *   to be scaled (multiplied) by current playback speed
     *   This flag is usually set to FALSE.
     */
} Avsync_vidSchedulePolicy;

/**
    \brief Avsync reference clock adjust policy

    Structure used to configure the reference clock
    adjust policy.
*/
typedef struct Avsync_refClkAdjustPolicy
{
    Avsync_RefClkMasterType  refClkType;
    /**< Refernece clock master (Audio/Video/None) */
    Int32                   clkAdjustLead;
    /**< When refClkType is AVSYNC_REFCLKADJUST_NONE
     *   reference clock will be adjusted when a change
     *   in timebase is deteced.
     *   Max +ve delta which will cause clock adjustment
     *   PTS differnce beyond this indicated
     *   change in timebase
     */
    Int32                   clkAdjustLag;
    /**< When refClkType is AVSYNC_REFCLKADJUST_NONE
     *   reference clock will be adjusted when a change
     *   in timebase is deteced.
     *   Max -ve delta which will cause clock adjustment
     *   PTS differnce beyond this indicated
     *   change in timebase
     */
} Avsync_refClkAdjustPolicy;


/**
    \brief Per channel avsync configuration.
*/
typedef struct Avsync_SynchConfigParams
{
    Avsync_PlaybackStartMode playStartMode;
    /**< Behaviour at sequence start time.*/
    UInt32                   chNum;
    /**< Avsync channel number to which this configuration applies */
    UInt32                   avsyncEnable;
    /**< Enable/Disable avsync for this channel */
    Avsync_vidSchedulePolicy  vidSynchPolicy;
    /**<  Video Synchronization policy to be applied for this channel */
    Avsync_refClkAdjustPolicy clkAdjustPolicy;
    /**<  Reference clock adjust policy to be applied for this channel */
    Avsync_PTSInitMode       ptsInitMode;
    /**<  PTS initialization method */
    UInt32                   audioPresent;
    /**<  Flag indicating whether audio is present for this channel */
    UInt32                   videoPresent;
    /**<  Flag indicating whether video is present for this channel */
    UInt32                   playTimerStartTimeout;
    /**<  If  playStartMode is AVSYNC_PLAYBACK_START_MODE_WAITSYNCH ,
     *     this value in ms determines the max wait before which
     *     player time will be started
     */
    UInt32                   blockOnAudioGet;
    /**< Flag indicating if Avsync_audioGet can block or not */
} Avsync_SynchConfigParams;

/**
    \brief Link level avsync configuration.
*/
typedef struct AvsyncLink_LinkSynchConfigParams
{
    UInt32  numCh;
    /**< Number of channels in the link to be configured */
    UInt32  videoSynchLinkID;
    /**< The linkID of the link which will do Avsync_vidQueGet .
     *   This is usually the Compositor (SwMs) link
     */
    UInt32  displayLinkID;
    /**< The displayID associated with this link */
    UInt32  audioDevId;
    /**< The audio device ID associated with this link */
    UInt32  syncMasterChnum;
    /**< Channel number of the master synch channel when
     *   using AVSYNC_REFCLKADJUST_BYVIDEO mode */
    Avsync_SynchConfigParams queCfg[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    /**< Array of channel specific avsync configuration */
} AvsyncLink_LinkSynchConfigParams;

/**
    \brief Avsync AVSYNC_LINK_CMD_SET_VIDEO_BACKEND_DELAY params
*/
typedef struct Avsync_VideoBackendDelayParams
{
    UInt32 displayLinkID;
    /**< Display ID for which the backend delay value should be applied */
    UInt32 backendDelayMS;
    /**< Backend delay value in ms */
} Avsync_VideoBackendDelayParams;

/* function's */

/**
    \brief Avsync Get the Wall time.

    Walltime is the system time commmon across all cores on the device
    There is a single instance of wall timer that is started at
    AVSYNC_INIT time and runs until AVSYNC_DEINIT is done.
    Walltime is associated with a h/w timer on the playback device.

    \return Current wall time.
*/
UInt64 Avsync_getWallTime();

/**
    \brief Configure link level avsync params

    \return AVSYNC_S_OK on success
*/
Int32 Avsync_configSyncConfigInfo(AvsyncLink_LinkSynchConfigParams *cfg);

/**
    \brief Set Playback speed to perform timescaled playback

    Invokes the AVSYNC_LINK_CMD_TIMESCALE cmd internally

    \return AVSYNC_S_OK on success
*/
Int32 Avsync_setPlaybackSpeed(Avsync_TimeScaleParams *timeScaleParams);

/**
    \brief Pause playback

    Invokes the AVSYNC_LINK_CMD_PAUSE cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_doPause(Avsync_PauseParams *pauseParams);

/**
    \brief UnPause playback

    Invokes the AVSYNC_LINK_CMD_UNPAUSE cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_doUnPause(Avsync_UnPauseParams *unPauseParams);

/**
    \brief Step forward (Advance) by one frame

    Invokes the AVSYNC_LINK_CMD_STEPFWD cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_stepFwd(Avsync_StepFwdParams *stepFwdParams);

/**
    \brief Set the first audio PTS of the sequence

    Invokes the AVSYNC_LINK_CMD_SET_FIRST_AUDPTS cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_setFirstAudPTS(Avsync_FirstAudPTSParams *audPTSParams);

/**
    \brief Set the first video PTS of the sequence

    Invokes the AVSYNC_LINK_CMD_SET_FIRST_VIDPTS cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_setFirstVidPTS(Avsync_FirstVidPTSParams *vidPTSParams);

/**
    \brief Set the player state to RESET

    Invokes the AVSYNC_LINK_CMD_RESET_PLAYERTIME cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_resetPlayerTimer(Avsync_ResetPlayerTimerParams *resetParams);

/**
    \brief Set the video backend delay

    Invokes the AVSYNC_LINK_CMD_SET_VIDEO_BACKEND_DELAY cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_setVideoBackEndDelay(Avsync_VideoBackendDelayParams *delayParams);

/**
    \brief Set the player state to play

    Invokes the AVSYNC_LINK_CMD_PLAY cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_doPlay(Avsync_PlayParams *playParams);

/**
    \brief Set the player state to scan mode

    Invokes the AVSYNC_LINK_CMD_SCAN cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_doScan(Avsync_ScanParams *scanParams);

/**
    \brief Resume playback at seek point

    Invokes the AVSYNC_LINK_CMD_SEEK cmd internally
    \return AVSYNC_S_OK on success
*/
Int32 Avsync_seekPlayback(Avsync_SeekParams *seekParams);

/**
    \brief Print Avsync statistics

*/
Int32 Avsync_printStats();

/**
    \brief Set the initial value of wall time.

    The application set the initial value of the wall time
    before doing Avsync_init().
    The walltimer will start running after Avsync init is done.
*/

Int32 Avsync_setWallTimeBase(UInt64 wallTimeBase);


/**
    \brief Init default params for the channel specific avsync params
*/
static inline void Avsync_SynchConfigParams_Init(Avsync_SynchConfigParams *prms)
{
    prms->audioPresent    = FALSE;
    prms->blockOnAudioGet = FALSE;
    prms->chNum           = AVSYNC_INVALID_CHNUM;
    prms->playStartMode   = AVSYNC_PLAYBACK_START_MODE_WAITSYNCH;
    prms->playTimerStartTimeout = 0;
    prms->ptsInitMode = AVSYNC_PTS_INIT_MODE_AUTO;
    prms->avsyncEnable   = FALSE;
    prms->videoPresent = TRUE;
    prms->vidSynchPolicy.maxReplayLead = AVSYNC_VIDEO_FUTURE_FRAME_DROP_THRESHOLD_MS;
    prms->vidSynchPolicy.playMaxLag    = AVSYNC_VIDEO_PLAY_MAX_LAG_MS;
    prms->vidSynchPolicy.playMaxLead   = AVSYNC_VIDEO_PLAY_MAX_LEAD_MS;
    prms->vidSynchPolicy.doMarginScaling = FALSE;
    prms->clkAdjustPolicy.refClkType    = AVSYNC_REFCLKADJUST_NONE;
    prms->clkAdjustPolicy.clkAdjustLead = AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LEAD_MS;
    prms->clkAdjustPolicy.clkAdjustLag  = AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LAG_MS;

}

/**
    \brief Init default params for the link level avsync params
*/
static inline void AvsyncLink_LinkSynchConfigParams_Init(AvsyncLink_LinkSynchConfigParams *prms)
{
    Int i;

    memset(prms,0,sizeof(*prms));
    prms->audioDevId = AVSYNC_INVALID_AUDDEVID;
    prms->displayLinkID  = AVSYNC_INVALID_DISPLAY_ID;
    prms->numCh      = 0;
    prms->syncMasterChnum = AVSYNC_INVALID_CHNUM;
    prms->videoSynchLinkID = SYSTEM_LINK_ID_INVALID;
    for (i = 0 ; i < AVSYNC_MAX_CHANNELS_PER_DISPLAY; i++)
    {
        Avsync_SynchConfigParams_Init(&prms->queCfg[i]);
    }
}

/**
 * \brief Macro to intialize the playback sequence ID
 */
#define AVSYNC_INIT_SEQID(seqId)   ((seqId) = SYSTEM_DISPLAY_SEQID_DEFAULT)

#endif

/* @} */

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_VDIS_API McFW Video Display (VDIS) API

    @{
*/

/**
    \file ti_vdis.h
    \brief McFW Video Display (VDIS) API
*/

#ifndef __TI_VDIS_H__
#define __TI_VDIS_H__


#ifdef __cplusplus
extern "C" {
#endif
#include "common_def/ti_vdis_common_def.h"
#include "ti_vsys.h"
#include "link_api/avsync.h"
#include "link_api/system_const.h"

/* =============================================================================
 * Defines
 * =============================================================================
 */

/** Maximum Video Display channels */
#define VDIS_CHN_MAX            (32)

/** Invalid channel ID for a mosaic. Used to disable a window on the display */
#define VDIS_CHN_INVALID        (0xFF)

/** \brief Bitmask for HDMI VENC */
#define VDIS_VENC_HDMI          (0x1u)
/** \brief Bitmask for HDCOMP VENC */
#define VDIS_VENC_HDCOMP        (0x2u)
/** \brief Bitmask for DVO2 VENC */
#define VDIS_VENC_DVO2          (0x4u)
/** \brief Bitmask for SD VENC. */
#define VDIS_VENC_SD            (0x8u)

/** \brief Default display sequence id */
#define VDIS_DISPLAY_SEQID_DEFAULT  (SYSTEM_DISPLAY_SEQID_DEFAULT)

/* =============================================================================
 * Enums
 * =============================================================================
 */

/**
    \brief Display Device ID

    Association of display device ID to HDMI or HDDAC is done via VDIS_PARAMS_S
*/
typedef enum
{
    VDIS_DEV_HDMI   = 0,
    /**< Display 0 */

    VDIS_DEV_HDCOMP = 1,
    /**< Display 1 */

    VDIS_DEV_DVO2   = 2,
    /**< Display 2 */

    VDIS_DEV_SD     = 3,
    /**< Display 3 */

} VDIS_DEV;

/**
    \brief Display Type
*/
typedef enum
{
    VDIS_TYPE_SDDAC_COMPOSITE_CVBS  = 0,
    /**< Analog Composite - SDTV */

    VDIS_TYPE_DVO2_BT1120_YUV422 = 1,
    /**< Digital 16-bit embedded sync mode via DVO2 - HDTV */

    VDIS_TYPE_DVO1_BT1120_YUV422 = 2,
    /**< Digital 16-bit embedded sync mode via DVO1 - HDTV */

    VDIS_TYPE_HDDAC_VGA_RGB = 3,
    /**< Analog Component YPbPr - HDTV */

    VDIS_TYPE_HDDAC_COMPONENT_YPBPR = 4,
    /**< Analog Component RGB VESA Standard - HDTV */

    VDIS_TYPE_HDMI_RGB    = 5,
    /**< Digital HDMI Standard (On-Chip HDMI) - HDTV */

} VDIS_TYPE_E;

/**
    \brief TrickPlay Rate
*/
typedef enum
{
    VDIS_AVSYNC_1X = 1000,
    /**< Normal Speed Playback */

    VDIS_AVSYNC_2X = 2000,
    /**< 2X Speed Playback */

    VDIS_AVSYNC_4X = 4000,
    /**< 4X Speed Playback */

    VDIS_AVSYNC_8X = 8000,
    /**< 8X Speed Playback */

    VDIS_AVSYNC_16X = 16000,
    /**< 16X Speed Playback */

    VDIS_AVSYNC_32X = 32000,
    /**< 32X Speed Playback */

    VDIS_AVSYNC_HALFX = 500,
    /**< 1/2X Speed Playback */

    VDIS_AVSYNC_QUARTERX = 250,
    /**< 1/4X Speed Playback */

    VDIS_AVSYNC_IFRAMEONLY = 0,
    /**< 1/4X Speed Playback */


    VDIS_AVSYNC_MAX   = 100000
    /**< Maximum Playback Rate */

}VDIS_AVSYNC;

/**
    \brief Color space conversion mode
*/
typedef enum
{
    VDIS_CSC_MODE_SDTV_VIDEO_R2Y = 0,
    /**< Select coefficient for SDTV Video */

    VDIS_CSC_MODE_SDTV_VIDEO_Y2R,
    /**< Select coefficient for SDTV Video */

    VDIS_CSC_MODE_SDTV_GRAPHICS_R2Y,
    /**< Select coefficient for SDTV Graphics */

    VDIS_CSC_MODE_SDTV_GRAPHICS_Y2R,
    /**< Select coefficient for SDTV Graphics */

    VDIS_CSC_MODE_HDTV_VIDEO_R2Y,
    /**< Select coefficient for HDTV Video */

    VDIS_CSC_MODE_HDTV_VIDEO_Y2R,
    /**< Select coefficient for HDTV Video */

    VDIS_CSC_MODE_HDTV_GRAPHICS_R2Y,
    /**< Select coefficient for HDTV Graphics */

    VDIS_CSC_MODE_HDTV_GRAPHICS_Y2R,
    /**< Select coefficient for HDTV Graphics */

    VDIS_CSC_MODE_MAX,
    /**< Should be the last value of this enumeration.
         Will be used by driver for validating the input parameters. */

    VDIS_CSC_MODE_NONE = 0xFFFFu
    /**< Used when coefficients are provided */

}VDIS_COLOR_SPACE_MODE_E;

/**
 * \brief DVO Format
 */
typedef enum
{
    VDIS_DVOFMT_SINGLECHAN = 0,
    /**< Ouput data format is single channel with embedded sync */
    VDIS_DVOFMT_DOUBLECHAN,
    /**< Output data format is dual channel with embedded sync */
    VDIS_DVOFMT_TRIPLECHAN_EMBSYNC,
    /**< Output data format is tripple channel with embedded sync */
    VDIS_DVOFMT_TRIPLECHAN_DISCSYNC,
    /**< Ouptut data format is triple channel with discrete sync */
    VDIS_DVOFMT_DOUBLECHAN_DISCSYNC,
    /**< Output data format is dual channel with discrete sync */
    VDIS_DVOFMT_MAX
    /**< This should be the last Enum */
} VDIS_DIGITAL_FMT_E;

/**
 * \brief Analog Format
 */
typedef enum
{
    VDIS_A_OUTPUT_COMPOSITE = 0,
    /**< Analog output format composite */
    VDIS_A_OUTPUT_SVIDEO,
    /**< Analog output format svideo */
    VDIS_A_OUTPUT_COMPONENT,
    /**< Analog output format component */
    VDIS_A_OUTPUT_MAX
} VDIS_ANALOG_FMT_E;

/**
 * \brief Signal polarity
 */
typedef enum
{
    VDIS_POLARITY_ACT_HIGH = 0,
    /**< Signal polarity Active high */
    VDIS_POLARITY_ACT_LOW = 1,
    /**< Signal polarity Active low */
    VDIS_POLARITY_MAX = 2,
    /**< Signal polarity Active low */
}VDIS_SIGNAL_POLARITY_E;


/* =============================================================================
 *  Structures
 * =============================================================================
 */

/**
    \brief Screen Buffer Information
*/
typedef struct VDIS_SCREEN_BUF_INFO_S
{
    Void           *bufVirtAddr;
    /**< Buffer user virtual address */

    Void           *bufPhysAddr;
    /**< Buffer system physical address */

    UInt32          pitch[2];
    /**< Buffer pitch, in bytes, for Y and C planes (YUV420SP) or YC plane (YUV422I) */

    UInt32          width;
    /**< Width of data in pixels */

    UInt32          height;
    /**< Height of data in lines */

    VIDEO_FORMATE_E dataFormat;
    /**< YUV Data Format */

} VDIS_SCREEN_BUF_INFO_S;



/* =============================================================================
 * APIs
 * =============================================================================
 */

/**
    \brief Set defaults for VDIS_PARAMS_S

    By default display parameters are setup for 2 HD (Mosaic) + 1 SD (Non-Moisac) Display mode

    \param  pContext    [OUT] VDIS_PARAMS_S filled with default parameters
*/
Void  Vdis_params_init(VDIS_PARAMS_S * pContext);

/**
    \brief Setup Tied device configuration in display sub-system

    MUST be called before calling Vdis_init

    \param   [IN] Device1, Device2, pContext Display configuration

    \return ERROR_NONE on success
*/

Int32 Vdis_tiedVencInit(VDIS_DEV VdDevId1, VDIS_DEV VdDevId2, VDIS_PARAMS_S * pContext);

/**
    \brief Initialize display sub-system

    MUST be called before calling any other VDIS API

    \param  pContext    [IN] Display configuration

    \return ERROR_NONE on success
*/
Int32 Vdis_init(VDIS_PARAMS_S * pContext);

/**
    \brief De-Initialize display sub-system

    MUST be called after Vdis_stop() and Vsys_delete()
*/
Void  Vdis_exit();

/**
    \brief Start display sub-system

    MUST be called after calling Vdis_init() and Vsys_create()

    \return ERROR_NONE on success
*/
Int32 Vdis_start();

/**
    \brief Stop display sub-system

    MUST be called before calling Vsys_delete()

    \return ERROR_NONE on success
*/
Int32 Vdis_stop();

/**
    \brief Set or change mosaic information on a given display

    Use this API to change or switch mosaic on a given display

    \param vdDevId              [IN] Device on which to apply this API
    \param psVdMosaicParam      [IN] Mosaic Parameters

    \return ERROR_NONE on success
*/
Int32 Vdis_setMosaicParams(VDIS_DEV vdDevId, VDIS_MOSAIC_S *psVdMosaicParam);

/**
    \brief Get current mosaic information on a given display

    \param vdDevId              [IN] Device on which to apply this API
    \param psVdMosaicParam      [OUT] Returned current Mosaic Parameters

    \return ERROR_NONE on success
*/
Int32 Vdis_getMosaicParams(VDIS_DEV vdDevId, VDIS_MOSAIC_S *psVdMosaicParam);

/**
    \brief Channel window to channel mapping for current selected mosaic

    \param vdDevId       [IN] Device on which to apply this API
    \param usChnMap      [IN] Window to channel mapping for each window. Set to VDIS_CHN_INVALID is no channel is mapped to a window

    \return ERROR_NONE on success
*/
Int32 Vdis_setMosaicChn(VDIS_DEV vdDevId, VDIS_CHN * usChnMap);

/**
    \brief Disable the window to which the specified channel is mapped

    If the channel is not being shown on the display then this API will have no effect.

    \param vdDevId     [IN] Device on which to apply this API
    \param vdChId      [IN] Channel to disable on the display

    \return ERROR_NONE on success
*/
Int32 Vdis_disableChn(VDIS_DEV vdDevId, VDIS_CHN vdChId);

/**
    \brief Enable the window to which the specified channel is mapped

    If the channel is not being shown on the display then this API will have no effect.

    \param vdDevId     [IN] Device on which to apply this API
    \param vdChId      [IN] Channel to enable on the display

    \return ERROR_NONE on success
*/
Int32 Vdis_enableChn(VDIS_DEV vdDevId, VDIS_CHN vdChId);

/**
 * \brief:
 *      Enable all the display channel
 * \input:
 *      vdDevId         -- Device Id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS        --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdis_enableAllChn(VDIS_DEV vdDevId);
/**
 * \brief:
 *      Diable all the display channel with show blank frame
 * \input:
 *      vdDevId         -- Device Id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS        --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdis_disableAllChn(VDIS_DEV vdDevId);


/**
    \brief Check if there window to which the specified channel is mapped and is enabled

    If the channel is not being shown on the display then this API will return FALSE.

    \param vdDevId     [IN] Device on which to apply this API
    \param vdChId      [IN] Channel to check for enable

    \return TRUE: channel is enabled on the display, FALSE: channel is disable for display
*/
Bool  Vdis_isEnableChn(VDIS_DEV vdDevId, VDIS_CHN vdChId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vdis_setParamDevice(VDIS_DEV VdDevId, VDIS_DEV_PARAM_S *psVdDevParam);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vdis_getParamDevice(VDIS_DEV VdDevId, VDIS_DEV_PARAM_S *psVdDevParam);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vdis_enableDevice(VDIS_DEV vdDevId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vdis_disableDevice(VDIS_DEV vdDevId);


/**
    \brief Send filled video buffers to framework for display

    User calls this API to put full video frames for display

    \param pFrameBufList    [OUT]  List of video frames to be displayed

    \return ERROR_NONE on success
*/
Int32 Vdis_putFullVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList);

/**
    \brief Give displayed video frames back to the application

    Buffers that are were previously put by Vdis_putFullVideoFrames can be
    freed back to the framework by invoking this API.

    \param pFrameBufList [IN]   List of video frames

    \return ERROR_NONE on success
*/
Int32 Vdis_getEmptyVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList, UInt32 timeout);

/**
    \brief Get info on number of channels and each channels property from
           decode framework

    \param channelListInfo [OUT]   Structure pointer that will be populated
                                   with input channel info by this function

    \return ERROR_NONE on success
*/
Int32 Vdis_getChannelInfo(VIDEO_CHANNEL_LIST_INFO_S *channelListInfo);

/**
    \brief Gets display resolution for Venc

    \param devId   deviceId

    \return ERROR_NONE on success
*/
VSYS_VIDEO_STANDARD_E Vdis_getResolution(VDIS_DEV devId);

/**
 * \brief:
 *      Switch the queue from which frame are displayed
 * \input:
 *      vdDevId                -- Device id
 *      queueId                -- QueueID to switch to
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS          --    while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_switchActiveQueue(VDIS_DEV vdDevId, UInt32 queueId);

/**
 * \brief:
 *      Switch the channel Id for SDTV live bypass path
 * \input:
 *      vdDevId                -- Device id
 *      chId                -- chID to switch to
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS          --    while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_switchSDTVChId(VDIS_DEV vdDevId, UInt32 chId);


/**
 * \brief:
 *      Switch the queue from which frame are displayed
 * \input:
 *      vdDevId                -- Device id
 *      channelId              -- channelId to switch to
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS          --    while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_switchActiveChannel(VDIS_DEV vdDevId, UInt32 channelId);

/**

    \brief Sets display resolution at run time

    \param devId   deviceId

    \return ERROR_NONE on success
*/


Int32 Vdis_setResolution(VDIS_DEV devId, UInt32 resolution);

/**
  * \brief:
  *     get channel input information from win ID
  * \input:
  *     vdDevId     -- Mosaic id
  *     winId       -- win ID which we want to get the channel information
  * \output:
  *     psChnlInfo  -- channel input infomation
  * \return:
  *     Always return success
  */

Int32 Vdis_getChnlInfoFromWinId(VDIS_DEV vdDevId, UInt32 winId,WINDOW_S * psChnlInfo);

/**
  * \brief:
  *     set channel crop settings from window ID
  * \input:
  *     vdDevId     -- Mosaic id
  *     winId       -- win ID which we want to get the channel information
  *     cropParam   -- crop settings to apply
  * \return:
  *     Always return success
  */

Int32 Vdis_SetCropParam(VDIS_DEV vdDevId, UInt32 winId,WINDOW_S cropParam);

/**
    \brief Stops display driver

    \param devId   deviceId

    \return ERROR_NONE on success
*/

/* =============================================================================
 * Vdis module APIs
 * =============================================================================
 */
/**
 * \brief:
 *      Re-Initialize Venc parameters to perform display shift adjustments
 *      Call this API when ever want to do diplsy shift adjustments,
 *      perferably along with display resolution chnage
 *      Please note that this API should not be called before the Vsys_create()
 * \input:
 *      VDIS_DEV devId
 * \output:
 *      NA
 * \return
 *      NA
*/
Void Vdis_DisplayShiftAdjust(VDIS_DEV devId);

Int32 Vdis_stopDrv(VDIS_DEV devId);

/**
    \brief Starts display driver

    \param devId   deviceId

    \return ERROR_NONE on success
*/

Int32 Vdis_startDrv(VDIS_DEV devId);

/**
    \brief Playback in a timescaled manner (fast/slow playback)

    This API is used to control playback speed

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID
    \param timeScaleX1000  [IN] playback speed in X1000 factor.
                                For 1x this value should be   1000
                                For 0.5x this value should be 500
                                For 2.0x this value should be 2000
    \param seqId           [IN] Sequence Id which will be associated with all
                                frames received in this playback state.
                                Application should set it to VDIS_DISPLAY_SEQID_DEFAULT
                                if it doesnt want to change displaySeqId
*/
Int32 Vdis_setPlaybackSpeed(VDIS_DEV vdDevId,VDIS_CHN vdispChnId, UInt32 timeScaleX1000,
                            UInt32 seqId);

/**
    \brief Playback in a timescaled manner (fast/slow playback) for all channels

    This API is used to control playback speed

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID
    \param timeScaleX1000  [IN] playback speed in X1000 factor.
                                For 1x this value should be   1000
                                For 0.5x this value should be 500
                                For 2.0x this value should be 2000
    \param seqId           [IN] Sequence Id which will be associated with all
                                frames received in this playback state.
                                Application should set it to VDIS_DISPLAY_SEQID_DEFAULT
                                if it doesnt want to change displaySeqId
*/

Int32 Vdis_mosaicSetPlaybackSpeed(VDIS_DEV vdDevId,UInt32 timeScaleX1000,
                                  UInt32 seqId);

/**
    \brief Pause playback

    This API is used to pause playback

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID

*/
Int32 Vdis_pauseChn(VDIS_DEV vdDevId,VDIS_CHN vdispChnId);

/**
    \brief Pause playback of all channels

    This API is used to pause playback

    \param vdDevId  [IN] display device ID

*/
Int32 Vdis_pauseMosaic(VDIS_DEV vdDevId);

/**
    \brief Unpause playback to resume normal playback

    This API is used to unpause previously paused
    playback

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID

*/
Int32 Vdis_resumeChn(VDIS_DEV vdDevId,VDIS_CHN vdispChnId);

/**
    \brief Unpause playback of all channels to resume normal playback

    This API is used to unpause previously paused
    playback

    \param vdDevId  [IN] display device ID

*/
Int32 Vdis_resumeMosaic(VDIS_DEV vdDevId);

/**
    \brief Step Forward display by one frame

    API is used to step forward by one frame

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID

*/
Int32 Vdis_stepChn(VDIS_DEV vdDevId,VDIS_CHN vdispChnId);

/**
    \brief Step Forward display by one frame for all channels

    API is used to step forward by one frame

    \param vdDevId  [IN] display device ID

*/
Int32 Vdis_stepMosaic(VDIS_DEV vdDevId);

/**
    \brief Resume normal playback 1x

    API is used to resume normal playback after
    previous slow/fast playback

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID

*/
Int32 Vdis_resumeNormalPlayback(VDIS_DEV vdDevId,VDIS_CHN vdispChnId);

/**
    \brief Resume normal playback 1x for all channels

    API is used to resume normal playback after
    previous slow/fast playback

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID

*/
Int32 Vdis_mosaicResumeNormalPlayback(VDIS_DEV vdDevId);


/**
    \brief Flush all buffers in SwMs

    API is used to flush all buffer from SwMs link

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] display channel ID
    \param holdLastFrame  [IN] Flag indicating whther last buffer should be reatained.
                               If TRUE last buffer will not be freed.
                               If FALSE all buffers including last buffer will be freed

*/
Int32 Vdis_flushSwMs(VDIS_DEV vdDevId,VDIS_CHN vdispChnId,UInt32 holdLastFrame);

/**
    \brief Configure the video backend delay

    API is used to set the video backend delay

    \param vdDevId         [IN] display device ID
    \param backendDelayMS  [IN] video backend delay in MS

*/
Int32 Vdis_setVideoBackendDelay(VDIS_DEV vdDevId,
                                UInt32 backendDelayMS);

/**
    \brief Get the Avsync Configuration

    API is used to get the avsync configuration

    \param vdDevId         [IN] display device ID
    \param avsyncConfig    [OUT] Avsync Configuration params

*/
Int32 Vdis_getAvsyncConfig(VDIS_DEV vdDevId,
                          AvsyncLink_LinkSynchConfigParams *avsyncConfig);

/**
    \brief Set the Avsync Configuration

    API is used to set the avsync configuration

    \param vdDevId         [IN] display device ID
    \param avsyncConfig    [IN] Avsync Configuration params

*/
Int32 Vdis_setAvsyncConfig(VDIS_DEV vdDevId,
                          AvsyncLink_LinkSynchConfigParams *avsyncConfig);

/**
    \brief Do player seek to specified time

    API is used to configure playback to resume at specified time.

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
    \param seekAudPTS      [IN] Audio PTS to seek to
    \param seekVidPTS      [IN] Video PTS to seek to
    Application can set either AudPTS/VidPTS to
    AVSYNC_INVALID_PTS (not both) if required.
    \param seqId           [IN] Sequence Id which will be associated with all
                                frames received in this playback state.
                                Application should set it to VDIS_DISPLAY_SEQID_DEFAULT
                                if it doesnt want to change displaySeqId

*/
Int32 Vdis_seek(VDIS_DEV vdDevId,
                VDIS_CHN VdChnId,
                UInt64 seekAudPTS,
                UInt64 seekVidPTS,
                UInt32 seqId);

/**
    \brief Set the start value for WallTime.

    API is set the initial value of wall time.
    This API should be invoked once before Vsys_create
    The Walltimer will be initialized with this value

    \param wallTimeBase    [IN] Initial value of WallTimer
*/
Int32 Vdis_setWallTimeBase(UInt64 wallTimeBase);

/**
    \brief Set the first video PTS of the stream

    API is set the PTS value of the first frame
    in the stream.
    THis API will be used if AVsync is configured
    for app set mode for PTS initialization

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
    \param firstVidPTS    [IN] First video PTS value
*/
Int32 Vdis_setFirstVidPTS(VDIS_DEV vdDevId,
                          VDIS_CHN VdChnId,
                          UInt64 firstVidPTS);

/**
    \brief Set the first audio PTS of the stream

    API is set the PTS value of the first frame
    in the stream.
    THis API will be used if AVsync is configured
    for app set mode for PTS initialization

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
    \param firstAudPTS    [IN] First audio PTS value
*/
Int32 Vdis_setFirstAudPTS(VDIS_DEV vdDevId,
                          VDIS_CHN VdChnId,
                          UInt64 firstAudPTS);

/**
    \brief Reset player time for that particular channel

    THis API will reset player time for a channel.
    Player state should be set to play for playback to
    start again.

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
*/
Int32 Vdis_resetChPlayerTime(VDIS_DEV vdDevId,
                             VDIS_CHN VdChnId);

/**
    \brief Reset player time for all channels

    THis API will reset player time for all channel.
    Player state should be set to play for playback to
    start again.

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
*/

Int32 Vdis_resetMosaicPlayerTime(VDIS_DEV vdDevId);

/**
    \brief Set player state to PLAY

    THis API will set player state for a channel to PLAY state.
    Playback will start once player is in play state.

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
    \param seqId           [IN] Sequence Id which will be associated with all
                                frames received in this playback state.
                                Application should set it to VDIS_DISPLAY_SEQID_DEFAULT
                                if it doesnt want to change displaySeqId

*/
Int32 Vdis_setChPlayerStatePlay(VDIS_DEV vdDevId,
                                VDIS_CHN VdChnId,
                                UInt32 seqId);

/**
    \brief Set player state to PLAY for all channels

    THis API will set player state for all channel to PLAY state.
    Playback will start once player is in play state.

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
    \param seqId           [IN] Sequence Id which will be associated with all
                                frames received in this playback state.
                                Application should set it to VDIS_DISPLAY_SEQID_DEFAULT
                                if it doesnt want to change displaySeqId

*/
Int32 Vdis_setMosaicPlayerStatePlay(VDIS_DEV vdDevId,
                                    UInt32 seqId);


/**
    \brief Start playback in Scan mode

    THis API will set player state for a channel to SCAN state.
    In scan mode application will feed only I-frames and
    each frame will be displayed on screen for specific time
    duration

    \param vdDevId         [IN] display device ID
    \param vdChnId         [IN] display channel ID
    \param frameDisplayDurationMS [IN] Duration in ms each frame should be displayed
    \param seqId           [IN] Sequence Id which will be associated with all
                                frames received in this playback state.
                                Application should set it to VDIS_DISPLAY_SEQID_DEFAULT
                                if it doesnt want to change displaySeqId

*/
Int32 Vdis_scanCh(VDIS_DEV vdDevId,
                  VDIS_CHN VdChnId,
                  UInt32 frameDisplayDurationMS,
                  UInt32 seqId);

/**
    \brief Start playback in Scan mode

    THis API will set player state for a channel to SCAN state.
    In scan mode application will feed only I-frames and
    each frame will be displayed on screen for specific time
    duration

    \param vdDevId         [IN] display device ID
    \param frameDisplayDurationMS [IN] Duration in ms each frame should be displayed
    \param seqId           [IN] Sequence Id which will be associated with all
                                frames received in this playback state.
                                Application should set it to VDIS_DISPLAY_SEQID_DEFAULT
                                if it doesnt want to change displaySeqId

*/
Int32 Vdis_scanMosaic(VDIS_DEV vdDevId,
                      UInt32 frameDisplayDurationMS,
                      UInt32 seqId);

/**
    \brief Print AVSYNC statistics

*/
Int32 Vdis_printAvsyncStatistics();

/**
    \brief Executes sysfs command

    This API will create and execute a sysfs command based on arguments passed
    This API assumes first argument is a string and other arguments followed
    by the string are integers

    \param numAgrs  Number of arguments in sysfs command
    \param ...      Variable arguments

*/
Int32 Vdis_sysfsCmd(UInt32 numArgs, ...);


/**
  * \brief:
  *     Checks the status of grpx blender
    \param grpxId  graphics blender id
    \param buffer
    \param r       return value

  */
Int32 Vdis_isGrpxOn(Int32 grpxId, String buffer, Int32 * r);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __TI_VDIS_H__ */

/* @} */

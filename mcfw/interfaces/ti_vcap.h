/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_VCAP_API McFW Video Capture (VCAP) API

    @{
*/

/**
    \file ti_vcap.h
    \brief McFW Video Capture (VCAP) API
*/

#ifndef __TI_VCAP_H__
#define __TI_VCAP_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "ti_vsys.h"
#include "link_api/algLink.h"
#include "link_api/captureLink.h"
#include "common_def/ti_vcap_common_def.h"

/* =============================================================================
 * Defines
 * =============================================================================
 */

/** Maximum Video Capture streams */
#define VCAP_STRM_MAX       (2)

/** Maximum channels per tvp */
#define VCAP_MAX_CHN_PER_DEVICE  (4)

/* =============================================================================
 * Enums
 * =============================================================================
 */

/**
    \brief Capture Device ID
*/
typedef enum
{
    VCAP_DEV_VIN0A = 0,
    /**< 8-bit VIP0 - PortA */

    VCAP_DEV_VIN0B = 1,
    /**< 8-bit VIP0 - PortB */

    VCAP_DEV_VIN1A = 2,
    /**< 8-bit VIP1 - PortA */

    VCAP_DEV_VIN1B = 3,
    /**< 8-bit VIP1 - PortB */

    VCAP_DEV_VIN0  = VCAP_DEV_VIN0A,
    /**< 16-bit VIP0 - PortA */

    VCAP_DEV_VIN1  = VCAP_DEV_VIN1A,
    /**< 16-bit VIP1 - PortA */

    VCAP_DEV_MAX   = 4
    /**< Maximum Capture Devices */

}VCAP_DEV;

/**
    \brief Capture bus interface mode
*/
typedef enum
{
    VCAP_MODE_BT656_8BIT_YUV422 = 0,
    /**< 8-bit Embedded sync YUV422 mode */

    VCAP_MODE_BT1120_16BIT_YUV422,
    /**< 16-bit Embedded sync YUV422 mode */

    VCAP_MODE_BT1120_24BIT_RGB_24_BIT,
    /**< 24-bit Embedded sync RGB888 mode */

    VCAP_MODE_DIS_SYNC_RGB_24_BIT,
    /**< Discrete sync RGB888 mode */

} VCAP_MODE_E;

/**
    \brief Capture Multi-Channel mode
*/
typedef enum
{
    VCAP_MULTICHN_OPTION_4D1 = 0,
    /**< 4CH D1 Pixel Multiplexed mode */

    VCAP_MULTICHN_OPTION_4HALF_D1,
    /**< 4CH Half-D1 Pixel Multiplexed mode */

    VCAP_MULTICHN_OPTION_4CIF_PLUS_D1,
    /**< 4CH CIF Pixel Multiplexed mode */

    VCAP_MULTICHN_OPTION_SINGLE_CHN
    /**< 1CH Non-Multiplexed mode */

} VCAP_MULTICHN_OPTION_E;

/**
    \brief Capture Param Set/Get ID

    This is a list of all the parameters related toVCAP (capture) block.
    Each paramId needs an associated Create-time or Dynamic Param set
*/
typedef enum
{
    VCAP_CONTRAST,
    /**< Get/Set Contrast for individual capture channel. Configures the video decoder params */

    VCAP_SATURATION,
    /**< Get/Set Saturation for individual capture channel. Configures the video decoder params */

    VCAP_BRIGHTNESS,
    /**< Get/Set Brightness for individual capture channel. Configures the video decoder params */

    VCAP_HUE,
    /**< Get/Set HUE for individual capture channel. Configures the video decoder params */

    VCAP_RESOLUTION,
    /**< Get/Set Resolution for individual capture channel.
            This allows configuring the resolution of each channel (primary or secondary) individually
    */

    VCAP_FORMAT,
    /**< Get/Set Data Format - NOT SUPPORTED */

    VCAP_PORTMODE,
    /**< Get/Set Port Mode - NOT SUPPORTED */

    VCAP_SIGNALTYPE,
    /**< Get/Set Signal Type - NOT SUPPORTED */

    VCAP_OSDWINPRM,
    /**< Get/Set Osd window Parameters
            All the parameters in the AlgLink_OsdChWinParams structure need to be passed while setting windows parameters.
            The command updates all the parameters in the structure.
    */

    VCAP_OSDBLINDWINPRM,
    /**< Get/Set Osd window Parameters
            All the parameters in the AlgLink_OsdChWinParams structure need to be passed while setting windows parameters.
            The command updates all the parameters in the structure.
    */

    VCAP_SCDMODE,
    /**< Get/Set the SCD algorithm operation mode for a channel
            SCD Mode selection allows user to use SCD in Tamper Detection Mode or Motion Detection Mode or both
            Refer to AlgLink_ScdChParams structure
    */

    VCAP_SCDSENSITIVITY,
    /**< Get/Set intrenal threshold for SCD tolerance for triggering alarm
            Sensitivity value ranges from 0 to 6
            Refer to AlgLink_ScdSensitivity
    */

    VCAP_SCDGETALLCHFRAMESTATUS,
    /**< Get SCD status at frame level for all channels

            ONLY in DM810x
    */

    VCAP_IGNORELIGHTSOFF,
    /**< Get/Set ignore sudden lights OFF as scd condition
            Accepts value of TRUE or FALSE
            Refer to AlgLink_ScdChParams structure
    */

    VCAP_IGNORELIGHTSON,
    /**< get/set ignore sudden lights ON as scd condition
            Accepts value of TRUE or FALSE
            Refer to AlgLink_ScdChParams structure
    */

    VCAP_SCDBLOCKCONFIG,
    /**< Get/Set block config of scd channel
            Configure all the parameters for smaller blocks in a video frame.
            Block Configuration is needed for Motion Detection support from SCD algorithm.
            Refer to AlgLink_ScdChblkUpdate
    */

    VCAP_BLINDAREACONFIG,
    /**< Set blind area config of capture channel
            Configure all the parameters for Blind Area i.e. Pricvacy mask area.
    */

    VCAP_ALL

    /**< Get/Set All Params */

} VCAP_PARAMS_E;

/**
    \brief Capture Output Path Set/Get ID
*/
typedef enum
{
    VCAP_PATH_PREVIEW = 0,
    /**< Reference to Data Path for Preview */

    VCAP_PATH_PRIMARY_STREAM,
    /**< Reference to Data Path for Primary Encode Stream */

    VCAP_PATH_SECONDARY_STREAM,
    /**< Reference to Data Path for Secondary Encode Stream */

    VCAP_PATH_ALL
    /**< Maximum of all paths */

} VCAP_PATH_E;

/* =============================================================================
 * Structures
 * =============================================================================
 */

/**
    \brief Capture Device Parameters
*/
typedef struct
{
    Bool                    portEnable;
    /**< Enable/Disable Capture Port */

    VCAP_MODE_E             portMode;
    /**< Capture bus interface mode */

    VCAP_MULTICHN_OPTION_E  dataFormat;
    /**< Capture Multi-channel mode */

    VIDEO_STANDARD_E        signalType;
    /**< Capture video standard or signal type */

} VCAP_DEV_PARAM_S;

/**
    \brief
*/
typedef struct
{
  /**< output path Id */
  VCAP_PATH_E pathId;
  /**< ouput resolution width*/
  UInt32 width;
  /**< ouput resolution height*/
  UInt32 height;
}VCAP_CHN_DYNAMIC_RESOLUTION;

/**
    \brief Capture Channel Specific Dynamic Parameters
*/
typedef struct
{
    Int32  contrast;
    /**< Constrast, 0..255
            Configure the contrast of the capture device. In default mode, contrast for TVP5158 is configured
    */

    Int32  satauration;
    /**< Satauration, 0..255
            Configure the staturation of the capture device. In default mode, saturation for TVP5158 is configured
    */

    Int32  brightness;
    /**< Brightness, 0..255
            Configure the brightness of the capture device. In default mode, brightness for TVP5158 is configured
    */

    Int32  hue;
    /**< HUE, 0..255
            Configure the hue of the capture device. In default mode, hue for TVP5158 is configured
    */

    AlgLink_OsdChWinParams *osdChWinPrm;
   /**< OSD channel configurable params */

    AlgLink_OsdChBlindWinParams *osdChBlindWinPrm;
   /**< OSD channel configurable params */

    AlgLink_ScdChParams scdChPrm;
    /**< SCD channel configurable params */

    AlgLink_ScdChblkUpdate scdChBlkPrm;
    /**< SCD channel block configurable params */

    AlgLink_ScdAllChFrameStatus scdAllChFrameStatus;
    /**< SCD frame level status information for all CHs */

    CaptureLink_BlindInfo captureBlindInfo;

    VCAP_CHN_DYNAMIC_RESOLUTION chDynamicRes;
} VCAP_CHN_DYNAMIC_PARAM_S;

/**
    \brief Capture Channel Specific Static Parameters
*/
typedef struct
{
    VCAP_DEV                 deviceId;
    /**< Device ID to which this channel is mapped */

    Bool                     strmEnable[VCAP_STRM_MAX];
    /**< Enable/Disable Streams in capture, stream 0 should be enabled always */

    VIDEO_FORMATE_E          strmFormat[VCAP_STRM_MAX];
    /**< Data Format, YUV422 or YUV420SP of the capture stream. MUST be VF_YUV422I_YUYV for multi-channel capture */

    WINDOW_S                 strmResolution[VCAP_STRM_MAX];
    /**< Resolution of the stream, stream 0 is always 1:1 resolution, stream 1 can downsized resolution of stream 0 */

    VCAP_CHN_DYNAMIC_PARAM_S dynamicParams;
    /**< Initial values for dynamic parameters of capture channel */

} VCAP_CHN_PARAM_S;


/**
    \brief Capture Sub-system initialization parameters
*/
typedef struct
{
    VCAP_DEV_PARAM_S deviceParams[VCAP_DEV_MAX];
    /**< Device Parameters */

    VCAP_CHN_PARAM_S channelParams[VCAP_CHN_MAX * 3];
    /**< Channel Specific Parameters */

    UInt32 numChn;
    /**< Number of capture channels */

    UInt32 numDevices;
    /**< Number of video decoder devices */

    UInt32 enableConfigExtVideoDecoder;
    /**< When FALSE do not configure TVP5158 from M3 / McFW side */

} VCAP_PARAMS_S;

/**
    \brief Callback for new captured video frames available

    \return NONE
*/
typedef Void (*VCAP_CALLBACK_NEW_DATA)(Ptr cbCtx);

/**
    \brief Capture User Callbacks
*/
typedef struct {

    VCAP_CALLBACK_NEW_DATA  newDataAvailableCb;
    /**< New encoded bitstream available callback */

} VCAP_CALLBACK_S;

/* =============================================================================
 * APIs
 * =============================================================================
 */

/**
    \brief Set defaults for VCAP_PARAMS_S

    By default capture parameters are setup for 16CH D1 capture mode

    \param  pContext    [OUT] VCAP_PARAMS_S filled with default parameters
*/
Void  Vcap_params_init(VCAP_PARAMS_S * pContext);

/**
    \brief Initialize capture sub-system

    MUST be called before calling any other VCAP API

    \param  pContext    [IN] Capture configuration

    \return ERROR_NONE on success
*/
Int32 Vcap_init(VCAP_PARAMS_S * pContext);

/**
    \brief Re-Initialize capture sub-system

    Reuses video decoder handles created in the previous call to Vcap_init

    \param  pContext    [IN] Capture configuration

    \return ERROR_NONE on success
*/
Int32 Vcap_reInit(VCAP_PARAMS_S * pContext);


/**
    \brief De-Initialize capture sub-system

    MUST be called after Vcap_stop() and Vsys_delete()
*/
Void  Vcap_exit();

/**
    \brief Start capture sub-system

    MUST be called after calling Vcap_init() and Vsys_create()

    \return ERROR_NONE on success
*/
Int32 Vcap_start();

/**
    \brief Stop capture sub-system

    MUST be called before calling Vsys_delete()

    \return ERROR_NONE on success
*/
Int32 Vcap_stop();

/**
    \brief Enable a channel in the capture sub-system

    By default all channels and streams are enabled after Vcap_start()

    \param vcChnId  [IN] Capture channel ID
    \param vcStrmId [IN] Capture stream ID

    \return ERROR_NONE on success
*/
Int32 Vcap_enableChn(VCAP_CHN vcChnId, VCAP_STRM vcStrmId);

/**
    \brief Disable a channel in the capture sub-system

    Disabling a channel will make the channel unavailable for encode and display.
    If encode is enabled on that channel then user will stop getting bitstream for that channel.
    If display is enabled on that channel then display will hold on the last frame
    that was shown for that window on the display.
    When a capture channel is disabled, it is recommended to disable the channel on the display as well.

    \param vcChnId  [IN] Capture channel ID
    \param vcStrmId [IN] Capture stream ID

    \return ERROR_NONE on success
*/
Int32 Vcap_disableChn(VCAP_CHN vcChnId, VCAP_STRM vcStrmId);

/**
    \brief Coming soon .. NOT SUPPORTED in this release
*/
Int32 Vcap_setParamChn(VCAP_CHN vcChnId, VCAP_CHN_PARAM_S *psCapChnParam, VCAP_PARAMS_E paramId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_getParamChn(VCAP_CHN vcChnId, VCAP_CHN_PARAM_S *psCapChnParam, VCAP_PARAMS_E paramId);

/**
    \brief Set dynamic parameters for capture channels
*/
Int32 Vcap_setDynamicParamChn(VCAP_CHN vcChnId, VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam, VCAP_PARAMS_E paramId);

/**
    \brief Get dynamic parameters for capture channels
*/
Int32 Vcap_getDynamicParamChn(VCAP_CHN vcChnId, VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam, VCAP_PARAMS_E paramId);

/**
    \brief Set Capture frame rate. This is an additional control in capture side itself; is really useful for secondary stream <CIF>. Stream 0 is D1 & Stream 1 is CIF.
*/
Int32 Vcap_setFrameRate(VCAP_CHN vcChnId, VCAP_STRM vStrmId, Int32 inputFrameRate, Int32 outputFrameRate);

/**
    \brief Skip any specific FID type. This is an additional control in capture side itself; is really useful for secondary stream <CIF>. Stream 0 is D1 & Stream 1 is CIF.
*/
Int32 Vcap_skipFidType(VCAP_CHN vcChnId, Int32 fidType);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_getFrameRate(VCAP_CHN vcChnId, VCAP_STRM vStrmId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_getDynamicParamChn(VCAP_CHN vcChnId, VCAP_CHN_DYNAMIC_PARAM_S *psCapChnParam, VCAP_PARAMS_E paramId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_enableDevice(VCAP_DEV vcDevId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_disableDevice(VCAP_DEV vcDevId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_setParamDevice(VCAP_DEV vcDevId, VCAP_DEV_PARAM_S *psvcDevParam,VCAP_PARAMS_E paramId);

/**
 * \brief:
 *      Get capture channels enabled
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       Number of capture channels
*/
Int32 Vcap_getNumChannels(Void);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_getParamDevice(VCAP_DEV vcDevId, VCAP_DEV_PARAM_S *psvcDevParam,VCAP_PARAMS_E paramId);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_lock2DisplayChn(VCAP_CHN vcChnId,  VDIS_CHN vdChn);

/**
    \brief Coming soon .. NOT SUPPORED in this release
*/
Int32 Vcap_unLock2DisplayChn(VCAP_CHN vcChnId);

/**
    \brief Register user specified callbacks

    \param callback            [IN] User specified callbacks

    \param arg                 [IN] Callback context

    \return ERROR_NONE on success
*/
Int32 Vcap_registerCallback(VCAP_CALLBACK_S * callback, Ptr arg);

/**
    \brief Returns Bits Callback info registered by the application

    \param callback            [OUT] Pointer to User specified callbacks

    \param arg                 [OUT] Pointer to Callback context

    \return ERROR_NONE on success
*/
Int32 Vcap_getBitsCallbackInfo(VCAP_CALLBACK_S ** callback, Ptr *arg);

/**
    \brief Register user specified Bits callbacks

    \param callback            [IN] User specified callbacks

    \param arg                 [IN] Callback context

    \return ERROR_NONE on success
*/
Int32 Vcap_registerBitsCallback(VCAP_CALLBACK_S * callback, Ptr arg);

/**
    \brief Get SCD buffers from ALG framework

    Once user is done with the buffers they should be re-cycled back to the framework
    via Vcap_releaseBitstreamBuffer()

    \param pBitsBufList [OUT]   List of SCD Frame Result Buffers returned by the SCD Link
    \param timeout      [IN]    TIMEOUT_WAIT_FOREVER or TIMEOUT_NO_WAIT or timeout in units of msec

    \return ERROR_NONE on success
*/
Int32 Vcap_getAlgResultBuffer(VALG_FRAMERESULTBUF_LIST_S * pBitsBufList, UInt32 timeout);

/**
    \brief Release SCD buffers to ALG framework

    Buffers returned by Vcap_getAlgResultBuffer() are returned to the framework
    for resue after user is done using the SCD block metadata

    \param pBitsBufList [IN]   List of SCD Buffers to return to the framework

    \return SUCCESS or FAIL
*/
Int32 Vcap_releaseAlgResultBuffer(VALG_FRAMERESULTBUF_LIST_S * pBitsBufList);

/**
    \brief Request filled video buffers from framework

    User calls this API to get full video frames from the framework.
    After getting the video frames, user will
    - consume the video frames
    - and then call Vcap_putEmptyVideoFrames() to free the video frames back to the framework

    \param pFrameBufList    [OUT]  List of video frames returned by the framework
    \param timeout          [IN]   TIMEOUT_WAIT_FOREVER or TIMEOUT_NO_WAIT or timeout in msecs

    \return ERROR_NONE on success
*/
Int32 Vcap_getFullVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList, UInt32 timeout);

/**
    \brief Give consumed video frames back to the application to be freed

    Buffers that are were previously got from Vcap_getFullVideoFrames can be
    freed back to the framework by invoking this API.

    \param pFrameBufList [IN]   List of video frames

    \return ERROR_NONE on success
*/
Int32 Vcap_putEmptyVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList);

/**
    \brief Detects video per channel at the video input.

    \param pFrameBufList [IN]   List of video frames

    \return ERROR_NONE on success
*/

Int32 Vcap_detectVideo();

/**
    \brief Gets status of video source at the video decoder

    \param pStatus  [OUT] Video Source Status of all channels
*/
Int32 Vcap_getVideoSourceStatus(VCAP_VIDEO_SOURCE_STATUS_S *pStatus);

/**
    \brief Sets status of video source at the video decoder

    \param pStatus  [OUT] Video Source Status of all channels
*/
Int32 Vcap_setVideoSourceStatus(VCAP_VIDEO_SOURCE_STATUS_S *pStatus);


/**
    \brief Sets Audio codec parametes
*/
Int32 Vcap_setAudioModeParam(UInt32 numChannels, UInt32 samplingHz, UInt32 audioVolume);

/**
    \brief Configures video decoder either form A8 or from m3
*/
Int32 Vcap_storeVideoDecoderContext(Ptr handle, UInt32 i);

/**
    \brief Gets number of tvp instances
*/

Int32 Vcap_getNumDevices();
/**
    \brief Sets color parameters for give chId
*/

Int32 Vcap_setColor(Int32 contrast,
                    Int32 brightness,
                    Int32 saturation,
                    Int32 hue,
                    Int32 chId);

/**
    \brief Detects the signal status
*/

Bool Vcap_isPalMode();



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


/* @} */

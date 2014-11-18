/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_VENC_API McFW Video Encode (VENC) API

    @{
*/

/**
    \file ti_venc.h
    \brief McFW Video Encode (VENC) API
*/

#ifndef __TI_VENC_H__
#define __TI_VENC_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "ti_vsys.h"

/* =============================================================================
 * Defines
 * =============================================================================
 */

/** Maximum Video Encode channels */
#define VENC_PRIMARY_CHANNELS   (16)

#define VENC_CHN_MAX			(3*VENC_PRIMARY_CHANNELS)    /* If secondary output is enabled, there will be 16+16+16 channels */

/** Maximum Video Encode streams */
#define VENC_STRM_MAX           (3)

/** \brief Max number of ROIs supported in ENC link currently */
#define VENC_CURRENT_MAX_ROI        (4)
#define VENC_DEMO_PRIVACY_MASK_NUM_WINDOWS     (4)
#define VENC_DEMO_PRIVACY_MASK_WIN_WIDTH       (50)
#define VENC_DEMO_PRIVACY_MASK_WIN_HEIGHT      (20)
#define VENC_DEMO_PRIVACY_MASK_WIN0_STARTX     (275)
#define VENC_DEMO_PRIVACY_MASK_WIN0_STARTY     (16)

#define VENC_GENERATE_FILL_PATTERN(Cb,Cr,L)             ((((Cb) & 0xFF) << 0)  | \
                                                          (((Cr) & 0xFF) << 8)  | \
                                                          (((L) & 0xFF) << 16))

#define VENC_FILL_PIXEL_LUMA    (0x10)
#define VENC_FILL_PIXEL_CHROMA  (0x80)


/* =============================================================================
 * Enums
 * =============================================================================
 */

/**
    \brief Encode Param Set/Get ID
*/
typedef enum
{
    VENC_BITRATE,
    /**< Get/Set Bitrate in bits/second */

    VENC_FRAMERATE,
    /**< Get/Set frame-rate */

    VENC_IPRATIO,
    /**< Get/Set Intra-frame interval */

    VENC_RCALG,
    /**< Get/Set Rate Control Algo */

    VENC_QPVAL_I,
    /**< Get/Set QP for I-frame */

    VENC_QPVAL_P,
    /**< Get/Set QP for I-frame */

    VENC_SNAPSHOT,
    /**< Get JPEG snapshot */

    VENC_VBRDURATION,
    /**< Set VBRDuration param of CVBR */

    VENC_VBRSENSITIVITY,
    /**< Set VBRSensitivity param of CVBR */

    VENC_ROI,
    /**< Get/Set ROI parameters */

    VENC_ALL
    /**< Get/Set All parameters */

} VENC_PARAM_E;

/**
    \brief Encoder Rate-control type
*/
typedef enum RC_CTRL_E
{
    VENC_RATE_CTRL_VBR = 0,
    /**< [IH264_RATECONTROL_PRC] Variable Bitrate: For Local Storage*/

    VENC_RATE_CTRL_CBR = 1,
    /**< [IH264_RATECONTROL_PRC_LOW_DELAY]Constant Bitrate: For Video Conferencing*/

} VENC_RATE_CTRL_E;

typedef enum {
    VENC_XDM_DEFAULT = 0,                
    /**< Default setting of encoder.  See
     *   codec specific documentation for its
     *   encoding behaviour.
     */

    VENC_XDM_HIGH_QUALITY = 1,           
    /**< High quality encoding. */

    VENC_XDM_HIGH_SPEED = 2,             
    /**< High speed encoding. */

    VENC_XDM_USER_DEFINED = 3,           
    /**< User defined configuration, using
          *   advanced parameters.
          */

    VENC_XDM_HIGH_SPEED_MED_QUALITY = 4, 
    /**< High speed, medium quality
          *   encoding.
          */

    VENC_XDM_MED_SPEED_MED_QUALITY = 5,  
    /**< Medium speed, medium quality
          *   encoding.
                                     */
    VENC_XDM_MED_SPEED_HIGH_QUALITY = 6, 
    /**< Medium speed, high quality
          *   encoding.
          */

    VENC_XDM_ENCODING_PRESET_MAX  = 7,   
    /**< @todo need to add documentation */

	VENC_XDM_PRESET_DEFAULT = VENC_XDM_MED_SPEED_MED_QUALITY 
    /**< Default setting of
     *   encoder.  See codec specific
     *   documentation for its encoding
     *   behaviour.
     */
} VENC_XDM_EncodingPreset;

/**
  @enum   VENC_IH264ENC_SvcExtensionFlag
  @brief  Define SVC Extension Flag
*/
typedef enum
{
 /*Svc Extension Flag Disabled*/
  VENC_IH264_SVC_EXTENSION_FLAG_DISABLE               = 0 ,
  /*Svc Extension Flag Enabled*/
  VENC_IH264_SVC_EXTENSION_FLAG_ENABLE                = 1 ,
  /*Svc Extension Flag Enabled with EC Flexibility*/
  VENC_IH264_SVC_EXTENSION_FLAG_ENABLE_WITH_EC_FLEXIBILITY = 2

} VENC_IH264ENC_SvcExtensionFlag ;


/**
    \brief Profile Identifier for H.264 Encoder
*/
typedef enum
{
    VENC_CHN_BASELINE_PROFILE = 66,                     
    /**< BaseLine Profile   */

    VENC_CHN_MAIN_PROFILE     = 77,                     
    /**< Main Profile       */

    VENC_CHN_EXTENDED_PROFILE = 88,                     
    /**< Extended Profile   */

    VENC_CHN_HIGH_PROFILE     = 100,                    
    /**< High Profile       */

    VENC_CHN_DEFAULT_PROFILE  = VENC_CHN_HIGH_PROFILE,  
    /**< Default Profile    */

    VENC_CHN_HIGH10_PROFILE   = 110,                    
    /**< High 10 Profile    */

    VENC_CHN_HIGH422_PROFILE  = 122,
    /**< High 4:2:2 Profile */
    
    VENC_CHN_SVC_BASELINE_PROFILE = 83,
    /**< SVC Baseline Profile */

    VENC_CHN_SVC_HIGH_PROFILE      = 86
    /**< SVC High Profile */

} VENC_CHN_H264_Profile_E;


/**
    \brief Indicates number of temporal layers in bitstrea, for H.264 Encoder
*/
typedef enum
{
    VENC_TEMPORAL_LAYERS_1 = 1,
    /**< Only Base Layer */

    VENC_TEMPORAL_LAYERS_2 = 2,
    /**< Base Layer + Temporal Layer */

    VENC_TEMPORAL_LAYERS_3 = 3,
    /**< Base Layer + 2 Temporal Layers */

    VENC_TEMPORAL_LAYERS_4 = 4,
    /**< Base Layer + 3 Temporal Layers */

    VENC_TEMPORAL_LAYERS_MAX = VENC_TEMPORAL_LAYERS_4
    /**< Max number of layers supported */


} VENC_CHN_H264_NumTemporalLayer_E;

/**
  @enum  VENC_IH264ENC_RoiType  
  @brief Defines the different ROI types
*/
typedef enum {
  VENC_IH264_FACE_OBJECT       = 0,
    /**< Face type of ROI object               */
  VENC_IH264_BACKGROUND_OBJECT = 1,
    /**< Background type of ROI object         */
  VENC_IH264_FOREGROUND_OBJECT = 2,
    /**< Foreground type of ROI object         */
  VENC_IH264_DEFAULT_OBJECT    = 3,
    /**< Default type of ROI object            */
  VENC_IH264_PRIVACY_MASK      = 4
    /**< Privacy mask type of ROI object       */
} VENC_IH264ENC_RoiType;



/* =============================================================================
 * Structures
 * =============================================================================
 */

/**
    \brief Callback for new encoded bitstream buffers available

    \return ERROR_NONE on success
*/
typedef Void (*VENC_CALLBACK_NEW_DATA)(Ptr cbCtx);

/**
    \brief Encode Channel Specific ROI Privacy Masking Parameters
*/
typedef struct
{
    Int32 roiNumOfRegion;
    /**< Number of ROI's */

    Int32 roiStartX[VENC_CURRENT_MAX_ROI];
    /**< starting location X coordinate of this region */

    Int32 roiStartY[VENC_CURRENT_MAX_ROI];
    /**< starting location Y coordinate of this region */

    Int32 roiWidth[VENC_CURRENT_MAX_ROI];
    /**< Width of this ROI */

    Int32 roiHeight[VENC_CURRENT_MAX_ROI];
    /**< Height of this ROI */

    Int32 roiType[VENC_CURRENT_MAX_ROI];
    /**< ROI type */

    Int32 roiPriority[VENC_CURRENT_MAX_ROI];
    /**< ROI Priority/Color */

} VENC_CHN_ROI_DYNAMIC_PARAM_S;


/**
    \brief Encode Channel Specific Dynamic Parameters
*/
typedef struct
{
    Int32 frameRate;
    /**< Frame-rate */

    Int32 targetBitRate;
    /**< required bitrate in bits/second */

    Int32 intraFrameInterval;
    /**< I to P ratio or inter-frame interval */

    Int32 inputFrameRate;
    /**< Frame rate of the incoming content */

    Int32 rcAlg;
    /**< Rate-control Algorithm type */

    Int32 qpMin;
    /**< QP Min value */

    Int32 qpMax;
    /**< QP Max value */

    Int32 qpInit;
    /**< QP Init value */

    Int32 vbrDuration;
    /**< VBRDuration for CVBR */

    Int32 vbrSensitivity;
    /**< VBRSensitivity for CVBR */

    VENC_CHN_ROI_DYNAMIC_PARAM_S roiParams;
    /**< ROI Privacy masking parameters */

} VENC_CHN_DYNAMIC_PARAM_S;

/**
    \brief Encode Channel Specific Static Parameters
*/
typedef struct
{
    Int32 videoWidth;
    /**< Read only: Encoded Video Width */

    Int32 videoHeight;
    /**< Read only: Encoded Video Height */

    Int32 enableAnalyticinfo;
    /**< Enable/Disable flag for MV Data generation  */
    
    Int32 enableWaterMarking;
    /**< Enable this option to Insert Water Mark SEI message in the bit stream */

    Int32 encodingPreset;
    /**< Enum identifying predefined encoding presets @sa XDM_EncodingPreset  */

    Int32 maxBitRate;
   /**< Maximum Bit Rate for Encoder*/

    Int32 enableSVCExtensionFlag;
    /**< Enable/Disable flag for SVC extension headers in h.264 Enc  */   

    VENC_RATE_CTRL_E rcType;
    /**< Rate-control type */

    VENC_CHN_H264_NumTemporalLayer_E numTemporalLayer;
     /**< Number of temporal layers the output bitstream should have */
 
    VENC_CHN_DYNAMIC_PARAM_S dynamicParam;
    /**< Initial values for dynamic parameters of encode channel */

} VENC_CHN_PARAMS_S;

/**
    \brief Encode User Callbacks
*/
typedef struct {

    VENC_CALLBACK_NEW_DATA  newDataAvailableCb;
    /**< New encoded bitstream available callback */

} VENC_CALLBACK_S;

/**
    \brief Encode Sub-system initialization parameters
*/
typedef struct {

    VENC_CHN_PARAMS_S encChannelParams[VENC_CHN_MAX];
    /**< Channel Specific Parameters */

    VENC_CHN_H264_Profile_E h264Profile[VENC_CHN_MAX];
    /**< Codec profile */

    UInt32 numPrimaryChn;
    /**< Number of encoder primary channels <D1> */

    UInt32 numSecondaryChn;
    /**< Number of encoder secondary channels <CIF> */

} VENC_PARAMS_S;


/* =============================================================================
 * APIs
 * =============================================================================
 */

/**
    \brief Set defaults for VENC_PARAMS_S

    By default encode parameters are setup for 16CH D1 encode mode

    \param  pContext    [OUT] VENC_PARAMS_S filled with default parameters
*/
Void  Venc_params_init(VENC_PARAMS_S * pContext);

/**
    \brief Initialize encode sub-system

    MUST be called before calling any other VENC API

    \param  pContext    [IN] Encode configuration

    \return ERROR_NONE on success
*/
Int32 Venc_init(VENC_PARAMS_S * pContext);

/**
    \brief De-Initialize encode sub-system

    MUST be called after Venc_stop() and Vsys_delete()
*/
Void  Venc_exit();

/**
    \brief Start encode sub-system

    MUST be called after calling Venc_init() and Vsys_create()

    \return ERROR_NONE on success
*/
Int32 Venc_start();

/**
    \brief Stop encode sub-system

    MUST be called before calling Vsys_delete()

    \return ERROR_NONE on success
*/
Int32 Venc_stop();

/**
    \brief Enable a channel in the encode sub-system

    By default all channels and streams are enabled after Venc_start()

    \param vcChnId  [IN] Capture channel ID

    \return ERROR_NONE on success
*/
Int32 Venc_enableChn(VENC_CHN vencChnId);

/**
    \brief Disable a channel in the encode sub-system

    Disabling a channel will make the channel unavailable for decode and display.
    If display is enabled on that channel then display will hold on the last frame
    that was shown for that window on the display.
 
    \param vcChnId  [IN] Capture channel ID
 
    \return ERROR_NONE on success
*/
Int32 Venc_disableChn(VENC_CHN vencChnId);

/**
    \brief Set Encoder Channel specific dynamic parameters

    The parameters that are applied depend on the value of 'veParamId'

    \param vencChnId            [IN] Channel ID to which this applies
    \param vencStrmID           [IN] Channel ID to which this applies
    \param ptEncDynamicParam    [IN] Values for the various dynamic parameters
    \param veParamId            [IN] Parameter on which to apply the change

    \return ERROR_NONE on success
*/
Int32 Venc_setDynamicParam(VENC_CHN vencChnId, VENC_STRM vencStrmID, VENC_CHN_DYNAMIC_PARAM_S *ptEncDynamicParam, VENC_PARAM_E veParamId);

/**
    \brief Get Encoder Channel specific dynamic parameters

    The parameters that are returned depend on the value of 'veParamId'

    \param vencChnId            [IN] Channel ID to which this applies
    \param vencStrmID           [IN] Channel ID to which this applies
    \param ptEncDynamicParam    [IN] Values Returned for the various dynamic parameters
    \param veParamId            [IN] Parameter on which to get the parameters

    \return ERROR_NONE on success
*/
Int32 Venc_getDynamicParam(VENC_CHN vencChnId, VENC_STRM vencStrmID, VENC_CHN_DYNAMIC_PARAM_S *ptEncDynamicParam, VENC_PARAM_E veParamId);

/**
    \brief Set Encoder Channel specific input frame-rate

    \param vencChnId            [IN] Channel ID to which this applies
    \param veFrameRate         [IN] Value for the input frame-rate to be set

    \return ERROR_NONE on success
*/
Int32 Venc_setInputFrameRate(VENC_CHN vencChnId, Int32 veFrameRate);


/**
    \brief Force I-frame on a given encoder channel and stream

    \param vencChnId            [IN] Channel ID to which this applies
    \param vencStrmID           [IN] Channel ID to which this applies

    \return ERROR_NONE on success
*/
Int32 Venc_forceIDR(VENC_CHN vencChnId, VENC_STRM vencStrmID);

/**
    \brief Dump a jpeg snapshot of a given encoder channel and stream

    \param vencChnId            [IN] Channel ID to which this applies
    \param vencStrmID           [IN] Channel ID to which this applies

    \return ERROR_NONE on success
*/
Int32 Venc_snapshotDump(VENC_CHN vencChnId, VENC_STRM vencStrmID);

/**
    \brief Register user specified callbacks

    \param callback            [IN] User specified callbacks

    \param arg                 [IN] Callback context

    \return ERROR_NONE on success
*/
Int32 Venc_registerCallback(VENC_CALLBACK_S * callback, Ptr arg);

/**
    \brief Returns Callback info registered by the application

    \param callback            [OUT] Pointer to User specified callbacks

    \param arg                 [OUT] Pointer to Callback context

    \return ERROR_NONE on success
*/
Int32 Venc_getCallbackInfo(VENC_CALLBACK_S ** callback, Ptr *arg);

/**
    \brief Get encoded buffers from encode framework

    Once user is done with the buffers they should be re-cycled back to the framework
    via Venc_releaseBitstreamBuffer()

    \param pBitsBufList [OUT]   List of Bistream Buffers returned by the encoder
    \param timeout      [IN]    TIMEOUT_WAIT_FOREVER or TIMEOUT_NO_WAIT or timeout in units of msec

    \return ERROR_NONE on success
*/
Int32 Venc_getBitstreamBuffer(VCODEC_BITSBUF_LIST_S *pBitsBufList, UInt32 timeout);

/**
    \brief Release encoded buffers to encode framework

    Buffers returned by Venc_getBitstreamBuffer() are returned to the framework
    for resue after user is done using the encoded bitstreams

    \param pBitsBufList [IN]   List of Bistream Buffers to return to the framework

    \return SUCCESS or FAIL
*/
Int32 Venc_releaseBitstreamBuffer(VCODEC_BITSBUF_LIST_S *pBitsBufList);

/**
 * \brief:
 *		Get primary channels enabled 
 * \input:
 *		NA
 * \output:
 *		NA
 * \return
*		Number of primary channels
*/
Int32 Venc_getPrimaryChannels();

/**
 * \brief:
 *		Get Secondary channels enabled 
 * \input:
 *		NA
 * \output:
 *		NA
 * \return
*		Number of secondary channels
*/
Int32 Venc_getSecondaryChannels();

/**
 * \brief:
 *      This function gives context info.
 * \input:
 *      VENC_PARAMS_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Void Venc_getContext(VENC_PARAMS_S * contextInfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


/* @} */



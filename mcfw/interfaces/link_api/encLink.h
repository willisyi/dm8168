/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup ENC_LINK_API Video Encoder Link API

    Video Encode Link can be used to take input from a link & after doing
    Video Encode like H264/MJPEG, can output the frames to output queue.

    Along with the above, this link also supports the dynamically updating
    parameters of a particular codec for a desired outcome.

    @{
*/

/**
    \file encLink.h
    \brief Video Encoder Link API
*/

#ifndef _ENC_LINK_H_
#define _ENC_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's  */
#include <mcfw/interfaces/link_api/system.h>

/* Defines */

/* @{ */

/** \brief Number of encoder output queue */
#define ENC_LINK_MAX_OUT_QUE    (1)

/** \brief Max ENC channels per link */
#define ENC_LINK_MAX_CH         (48)

/** \brief Max number of buffer polls in ENC link */
#define ENC_LINK_MAX_BUF_ALLOC_POOLS        (4)

/** \brief Max number of ROIs supported in ENC link currently */
#define ENC_LINK_CURRENT_MAX_ROI        (4)


/**
    \ingroup LINK_API_CMD
    \addtogroup ENC_LINK_API_CMD ENCODE Link Control Commands

    @{
*/

/**
    \brief Link CMD: Configure Video Encoder params
*/
#define ENC_LINK_CMD_PRINT_IVAHD_STATISTICS (0x5001)
/**
    \brief Link CMD: Get Codec Params

    Get ENC Codec Params dynamically

    \param EncLink_GetDynParams * [OUT] Enc Get Codec parameters
*/
#define ENC_LINK_CMD_GET_CODEC_PARAMS       (0x5002)

/**
    \brief Link CMD: Set Bitrate Rate

    Set ENC output bitrate dynamically

    \param EncLink_ChBitRateParams * [IN] Enc BitRate parameter
*/
#define ENC_LINK_CMD_SET_CODEC_BITRATE      (0x5003)

/**
    \brief Link CMD: Set Frame Rate

    Set ENC output frame rate dynamically

    \param EncLink_ChFpsParams * [IN] Enc FPS parameter
*/
#define ENC_LINK_CMD_SET_CODEC_FPS          (0x5004)

/**
    \brief Link CMD: Set Input Frame Rate

    Set ENC Input frame rate dynamically

    \param EncLink_ChInputFpsParam * [IN] Enc Input FPS parameter
*/
#define ENC_LINK_CMD_SET_CODEC_INPUT_FPS    (0x5005)

/**
    \brief Link CMD: Set Intra Frame Interval

    Set ENC Intra frame interval  dynamically

    \param EncLink_ChIntraFrIntParams * [IN] Enc Intra Frame Interval parameter
*/
#define ENC_LINK_CMD_SET_CODEC_INTRAI       (0x5006)
/**
    \brief Link CMD: Set Force I-Frame

    Set ENC to force an I-Frame dynamically

    \param EncLink_ChForceIFrParams * [IN] Enc Force I-Frame parameter
*/
#define ENC_LINK_CMD_SET_CODEC_FORCEI       (0x5007)
/**
    \brief Link CMD: Set Rate Control Algorithm

    Set ENC Rate control algorithm dynamically

    \param EncLink_ChRcAlgParams * [IN] Enc  Rate control algorithm parameter
*/
#define ENC_LINK_CMD_SET_CODEC_RCALGO       (0x5008)
/**
    \brief Link CMD: Set Quantization Parameters for I/IDR frames

    Set ENC Quantization Parameters for I/IDR frames dynamically

    \param EncLink_ChQPParams * [IN] Enc Quantization Parameter for I/IDR frames
*/
#define ENC_LINK_CMD_SET_CODEC_QP_I         (0x5009)
/**
    \brief Link CMD: Set Quantization Parameters for P frames

    Set ENC Quantization Parameters for P frames dynamically

    \param EncLink_ChQPParams * [IN] Enc Quantization Parameter for P frames
*/
#define ENC_LINK_CMD_SET_CODEC_QP_P         (0x500A)
/**
    \brief Link CMD: Set a Flag to Dump a JPEG frame

    Set ENC Flag to Dump a JPEG frame dynamically

    \param EncLink_ChannelInfo * [IN] Enc Flag to Dump a JPEG frame parameter
*/
#define ENC_LINK_CMD_SET_CODEC_SNAPSHOT     (0x500B)
/**
    \brief Link CMD: Set VBR Duration

    Set ENC VBR Duration to switch states dynamically

    \param EncLink_ChCVBRDurationParams * [IN] Enc VBR Duration parameter
*/
#define ENC_LINK_CMD_SET_CODEC_VBRD         (0x500C)
/**
    \brief Link CMD: Set VBR Sensitivity

    Set ENC VBR Sensitivity dynamically

    \param EncLink_ChCVBRSensitivityParams * [IN] Enc Intra Frame Interval parameter
*/
#define ENC_LINK_CMD_SET_CODEC_VBRS         (0x500D)
/**
    \brief Link CMD: Set ROI parameters

    Set ENC ROI dynamically

    \param EncLink_ChROIParams * [IN] Enc ROI parameters
*/
#define ENC_LINK_CMD_SET_CODEC_ROI          (0x500E)

/**
    \brief Link CMD: Enable channel

    \param EncLink_ChannelInfo * [IN] enable channel
*/
#define ENC_LINK_CMD_ENABLE_CHANNEL         (0x500F)

/**
    \brief Link CMD: Disable channel

    \param EncLink_ChannelInfo * [IN] disable channel
*/
#define ENC_LINK_CMD_DISABLE_CHANNEL        (0x5010)
/** \brief Enc command to print channel level statistics */
#define ENC_LINK_CMD_PRINT_STATISTICS       (0x5011)
/** \brief Enc command to print channel buffer statistics */
#define ENC_LINK_CMD_PRINT_BUFFER_STATISTICS       (0x5012)

/* @} */

/* @} */

/* Data structure's */

/**
    \brief Enc link channel dynamic get dynamic parameters

    Defines encoder dynamic parameters that can be probed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_GetDynParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 inputWidth;
    /**< Encoder input width */
    UInt32 inputHeight;
    /**< Encoder input height */
    UInt32 targetBitRate;
    /**< Modified encoder bit rate value in bits/sec format */
    UInt32 targetFps;
    /**< Modified encoder Frames per second (FPS) value in fps x 1000 format */
    UInt32 intraFrameInterval;
    /**< Modified encoder intraFrame rate value */

} EncLink_GetDynParams;

/**
    \brief Enc link channel dynamic set config params

    Defines encoder bitRate parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChBitRateParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 targetBitRate;
    /**< Modified encoder bit rate value in bits/sec format */
} EncLink_ChBitRateParams;

/**
    \brief Enc link channel dynamic set config params

    Defines encoder FPS parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChFpsParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 targetFps;
    /**< Modified encoder Frames per second (FPS) value in fps x 1000 format */
    UInt32 targetBitRate;
    /**< Modified encoder bit rate value in bits/sec format */
} EncLink_ChFpsParams;

/**
    \brief Enc link channel dynamic set config params

    Defines encoder intraFrameInterval parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChIntraFrIntParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 intraFrameInterval;
    /**< Modified encoder intraFrame rate value */
} EncLink_ChIntraFrIntParams;

/**
    \brief Enc link channel dynamic set config params

    Defines encoder RateControl Algorithm parameter that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChRcAlgParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 rcAlg;
    /**< Modified encoder intraFrame rate value */
} EncLink_ChRcAlgParams;

/**
    \brief Enc link channel dynamic set config params

    Defines encoder QP min,max,init parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChQPParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 qpMin;
    /**< Modified encoder qpMin value */
    UInt32 qpMax;
    /**< Modified encoder qpMax value */
    UInt32 qpInit;
    /**< Modified encoder qpInit value */
} EncLink_ChQPParams;

/**
    \brief Enc link channel dynamic set config params

    Defines encoder CVBR duration parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChCVBRDurationParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 vbrDuration;
    /**< Modified encoder VBRDuration value */
} EncLink_ChCVBRDurationParams;

/**
    \brief Enc link channel dynamic set config params

    Defines encoder CVBR sensitivity parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChCVBRSensitivityParams
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 vbrSensitivity;
    /**< Modified encoder VBRSensitivity value */
} EncLink_ChCVBRSensitivityParams;

/**
    \brief Enc link channel ROI set config params

    Defines encoder ROI parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChROIParams {
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 numOfRegion;
    /**< Number of ROIs to be passed to codec */
    UInt32 startX[ENC_LINK_CURRENT_MAX_ROI];
    /**< X co-ordinate of region */
    UInt32 startY[ENC_LINK_CURRENT_MAX_ROI];
    /**< Y co-ordinate of region */
    UInt32 width[ENC_LINK_CURRENT_MAX_ROI];
    /**< width of region */
    UInt32 height[ENC_LINK_CURRENT_MAX_ROI];
    /**< height of region */
    UInt32 type[ENC_LINK_CURRENT_MAX_ROI];
    /**< Type of each ROI */
    UInt32 roiPriority[ENC_LINK_CURRENT_MAX_ROI];
    /**< Priority of each ROI */
} EncLink_ChROIParams;


/**
    \brief Enc link channel dynamic set config params

    Defines encoder Force I-frames/IDR parameters that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChForceIFrParams
{
    UInt32 chId;
    /**< Encoder channel number */
} EncLink_ChForceIFrParams;

/**
    \brief Enc link channel info
*/
typedef struct
{
	UInt32 chId;
    /**< Encoder channel number */
} EncLink_ChannelInfo;

/**
    \brief Enc link channel set input frame-rate

    Defines encoder input FPS param that can be changed dynamically
    on a per channel basis for the encode link
*/
typedef struct EncLink_ChInputFpsParam
{
    UInt32 chId;
    /**< Encoder channel number */
    UInt32 inputFps;
    /**< input Modified encoder Frames per second (FPS) value */
} EncLink_ChInputFpsParam;


/**
    \brief Enc link channel dynamic params

    Defines those parameters that can be changed dynamically on a per channel
    basis for the encode link
*/
typedef struct EncLink_ChDynamicParams
{
    Int32 intraFrameInterval;
    /**< Intra frame interval in frames */

    Int32 targetBitRate;
    /**< Target bitrate of the encoder */

    Int32 interFrameInterval;
    /**< Inter frame interval in frames */

    Int32 mvAccuracy;
    /**< motion vector search accuracy @sa IVIDENC2_MotionVectorAccuracy */

    Int32 inputFrameRate;
    /**< to configure the encode link frame rate control  */

    Int32 targetFrameRate;
    /**< to configure the encode link frame rate control  */

    Int32 rcAlg;
    /**< Rate-control Algorithm type */

    Int32 qpMin;
    /**< QP Min value */

    Int32 qpMax;
    /**< QP Max value */

    Int32 qpInit;
    /**< QP Init value */

    Int32 vbrDuration;
    /**< to configure the encode link cvbr duration  */

    Int32 vbrSensitivity;
    /**< to configure the encode link cvbe sensitivity  */
} EncLink_ChDynamicParams;



typedef struct EncLink_ChCreateParams
{
    UInt32           format;
    /**< Coding type @sa IVIDEO_Format */
    Int32            profile;
    /**< Codec profile */
    Int32            dataLayout;
    /**< Video buffer layout */
    UInt32           fieldMergeEncodeEnable;
    /**< Enable this option to Encode 2 fields in same Encode link call
         * ie. both Top & Bottom field in same input buffer with Top filed
         * followed by bottom field data in field seperated mode */
    Int32 enableAnalyticinfo;
    /**< Enable this option to generate MV Data in the bit stream */
    Int32 enableWaterMarking;
    /**< Enable this option to Insert Water Mark SEI message in the bit stream */
    Int32 maxBitRate;
    /** < Maximum Bit Rate for encode*/
    Int32 encodingPreset;
    /**< Enum identifying predefined encoding presets @sa XDM_EncodingPreset  */
    Int32 rateControlPreset;
    /**<  Enum identifying predefined rate control presets @sa IVIDEO_RateControlPreset  */
    Bool enableHighSpeed;
    /**< Boolean to be set when this channel needs to be encoded in HIGH_SPEED mode. Requires encoding
         * preset to be USER_DEFINED   */
    Bool enableSVCExtensionFlag;
    /**< enable this option to insert svc extension headers into h.264 frames */
    Int32 numTemporalLayer;
    /**< Denotes the number of temporal layers that the encoder output bitstream will have.
         *   Default is the 1 for H.264 encoder link channels and is DONT CARE for both encoders */
    EncLink_ChDynamicParams defaultDynamicParams;
    /**< Create time default dynamic params
       */
} EncLink_ChCreateParams;

/**
    \brief Enc link create parameters
*/
typedef struct
{
    System_LinkInQueParams      inQueParams;
    /**< Input queue information. */
    System_LinkOutQueParams     outQueParams;
    /**< Output queue information. */
    EncLink_ChCreateParams      chCreateParams[ENC_LINK_MAX_CH];
    /**< Encoder link channel create params */
    UInt32                      numBufPerCh[ENC_LINK_MAX_BUF_ALLOC_POOLS];
    /**< Number of buffer to allocate per channel per buffr pool */
} EncLink_CreateParams;

/* function's */

/**
    \brief Enc link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 EncLink_init();

/**
    \brief Enc link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 EncLink_deInit();


/**
    \brief Set defaults for create parameter

    \param pPrm [IN] Default parameters
*/
static inline void EncLink_CreateParams_Init(EncLink_CreateParams *pPrm)
{
    UInt32 i;

    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    pPrm->outQueParams.nextLink = SYSTEM_LINK_ID_INVALID;

    /* when set 0, encoder will take default value based on system
       defined default on BIOS side */
    for (i=0; i<ENC_LINK_MAX_BUF_ALLOC_POOLS; i++)
    {
        pPrm->numBufPerCh[i] = 0;
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/




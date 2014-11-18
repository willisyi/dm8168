/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup SCLR_LINK_API Scalar 5 Link API

    SCLR Link can be used to take input from a link and after Scaling, outputs
    the frames to output queue.

    SCLR has one output
    - Output from either SC5 or SC3 scaler.

    @{
*/

/**
    \file sclrLink.h
    \brief Scalar (SCLR) Link API
*/

#ifndef _SCLR_LINK_H_
#define _SCLR_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/vidframe.h>

/* Define's */

/* @{ */

/** \brief SCLR Scaler output ID */
#define SCLR_LINK_MAX_OUT_QUE  (1)

/** \brief SCLR block SC5 ID */
#define SCLR_LINK_SC5                         (0)

/** \brief SCLR block SC3 ID */
#define SCLR_LINK_SEC0_SC3                    (1)

#define SCLR_LINK_NUM_BUFS_PER_CH_DEFAULT (0)

/* @} */

/**
    \ingroup LINK_API_CMD
    \addtogroup SCLR_LINK_API_CMD SCLR Link Control Commands

    @{
*/

/**
    \brief Link CMD: Get processed data

    \param [OUT] Processed frame data
*/
#define SCLR_LINK_CMD_GET_PROCESSED_DATA      (0x7000)

/**
    \brief Link CMD: Enable channel

    \param SclrLink_ChannelInfo * [IN] enable channel
*/
#define SCLR_LINK_CMD_ENABLE_CHANNEL          (0x7001)

/**
    \brief Link CMD: Disable channel

    \param SclrLink_ChannelInfo * [IN] disable channel
*/
#define SCLR_LINK_CMD_DISABLE_CHANNEL         (0x7002)

/**
    \brief Link CMD: Set Frame Rate

    Set SCLR input and output frame rate dynamically

    \param SclrLink_ChFpsParams * [IN] Sclr FPS parameters
*/
#define SCLR_LINK_CMD_SET_FRAME_RATE          (0x7003)

/**
    \brief Link CMD: Print stats

    Print SCLR Link statistic information
    such as input frame rate, output frame rate etc.

    \param NONE
*/
#define SCLR_LINK_CMD_PRINT_STATISTICS        (0x7004)

/**
    \brief Link CMD: Set resolution

    Set SCLR output resolution dynamically

    \param SclrLink_chDynamicSetOutRes * [IN] Output Resolution parameters
*/
#define SCLR_LINK_CMD_SET_OUTPUTRESOLUTION    (0x7005)

/**
    \brief Link CMD: Get resolution

    Get DEI output resolution dynamically

    \param SclrLink_chDynamicSetOutRes * [OUT] Output Resolution parameters
*/
#define SCLR_LINK_CMD_GET_OUTPUTRESOLUTION    (0x7006)

/**
    \brief Link CMD: Set FID type to be skipped.

    SCLR command to skip one specific FID type

    \param SclrLink_chDynamicSkipFidType * [IN] FID type parameters
*/
#define SCLR_LINK_CMD_SKIP_FID_TYPE           (0x7007)

/* @} */

/* Data structure's */

/**
    \brief SCLR link scale mode
*/
typedef enum
{
    SCLR_SCALE_MODE_ABSOLUTE,
    /**< Absolute scale mode, provide width & height */

    SCLR_SCALE_MODE_RATIO
    /**< Scale based on ratio */
} SCLR_SCALE_MODE;

/**
    \brief SCLR link scale ratio parameters
*/
typedef struct
{
    UInt32  numerator;
    /**< numerator of the ratio */

    UInt32  denominator;
    /**< denominiator of the ratio;
         For a scaling of 1/2, numerator will be 1, denominator will be 2 */
} SclrLink_OutputScaleRatio;

/**
    \brief SCLR link set ouput resolution
*/
typedef struct
{
    UInt32 chId;
    /**< output channel Id */

    UInt32 width;
    /**< ouput width */

    UInt32 height;
    /**< ouput height */

    UInt32 pitch[VIDFRAME_MAX_PLANES];
    /**< ouput pitch */
} SclrLink_chDynamicSetOutRes;

/**
    \brief SCLR link to skip one specific FID type
*/
typedef struct
{
    UInt32 chId;
    /**< output channel Id */

    UInt32 fidType;
    /**< FID type, TOP or BOTTOM */
} SclrLink_chDynamicSkipFidType;

/**
    \brief SCLR link scale factor 
*/
typedef union
{
    struct  
    {
        UInt32 outWidth;
        /**< Required scaled output width */

        UInt32 outHeight;
        /**< Required scaled output height */
    } absoluteResolution;
    /**< Absolute Values of required Output resolution */

    struct  
    {
        SclrLink_OutputScaleRatio    widthRatio;    
        /**< Required scaled output ratio, output width is relative to input width */

        SclrLink_OutputScaleRatio    heightRatio;    
        /**< Required scaled output ratio, output height is relative to input height */
    } ratio;
    /**< In/Out ratio Values for Scaling of outputs */

} SclrLink_OutputScaleFactor;

/**
    \brief Sclr link create parameters
*/
typedef struct
{
    System_LinkInQueParams      inQueParams;
    /**< Input queue information. */

    System_LinkOutQueParams     outQueParams;
    /**< Output queue information. */

    UInt32                      pathId;
    /**<  Scalar Block used for scaling.
          \ref SCLR_LINK_SC5      : Scalar-5 is used
          \ref SCLR_LINK_SEC0_SC3 : Scalar 3 is used */

    UInt32                      tilerEnable;
    /**< tilerEnable = TRUE, enable TILER format for the output
         tilerEnable = FALSE, disable TILER format for the output. */


    UInt32                      scaleMode;
    /**< Scale mode for outputs, see DEI_SCALE_MODE for valid values */

    SclrLink_OutputScaleFactor  outScaleFactor;
    /**< set scale factors for each channel,
         see DeiLink_OutputScaleRatio for valid values */

    UInt32                     enableLineSkipSc;
    /**< enableLineSkipSc = TRUE, alternate line will be skipped while doing scaling
         enableLineSkipSc = FALSE, all the lines will be used to do scaling */

    UInt32                      numBufsPerCh;
    /**< Number of output buffers per channel. Same for all the channels. */

    UInt32                     inputFrameRate;
    /**< Frames per seconds fed to the Scalar Link. 
         inputFrameRate + outputFrameRate should be set to control required fps */
    
    UInt32                     outputFrameRate;
    /**< Frames per seconds at which frames are generated at the output of Scalar Link.
         inputFrameRate + outputFrameRate should be set to control required fps */

    UInt32                      outDataFormat;
    /**< Set data format. Currently YUV422I only is supported. */
} SclrLink_CreateParams;

/**
    \brief SCLR link channel info
*/
typedef struct
{
    UInt32 channelId;
    /**< SCLR channel number */

    UInt32 enable;
    /**< SCLR channel enable or disable flag */
} SclrLink_ChannelInfo;

/**
    \brief SCLR link channel dynamic set config params

    Defines SCLR FPS parameters that can be changed dynamically
    on a per channel basis 
*/
typedef struct SclrLink_ChFpsParams
{
    UInt32 chId;
    /**< SCLR channel number */

    UInt32 inputFrameRate;
    /**< Input frame rate fed to SCLR link*/

    UInt32 outputFrameRate;
    /**< Expected output frame rate */

} SclrLink_ChFpsParams;

/* function's */
/**
    \brief SCLR link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 SclrLink_init();

/**
    \brief Sclr link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 SclrLink_deInit();

/**
    \brief Set defaults for create parameter

    \param pPrm [IN] Default parameters
*/
static inline void SclrLink_CreateParams_Init(SclrLink_CreateParams *pPrm)
{

    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    pPrm->inQueParams.prevLinkQueId = 0;

    pPrm->outQueParams.nextLink = SYSTEM_LINK_ID_INVALID;
    pPrm->outScaleFactor.ratio.widthRatio.numerator = 1;
    pPrm->outScaleFactor.ratio.widthRatio.denominator = 1;
    pPrm->outScaleFactor.ratio.heightRatio.numerator = 1;
    pPrm->outScaleFactor.ratio.heightRatio.denominator = 1;
    pPrm->inputFrameRate = 30;
    pPrm->outputFrameRate = 30;
    pPrm->tilerEnable = FALSE;
    pPrm->scaleMode = SCLR_SCALE_MODE_RATIO;
    pPrm->enableLineSkipSc = FALSE;

    pPrm->numBufsPerCh  = SCLR_LINK_NUM_BUFS_PER_CH_DEFAULT;
    pPrm->outDataFormat = SYSTEM_DF_YUV422I_YUYV;

    pPrm->pathId = SCLR_LINK_SC5;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/


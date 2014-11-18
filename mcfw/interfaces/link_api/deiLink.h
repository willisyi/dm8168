/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup DEI_LINK_API De-interlacer (DEI) Link API

    Dei Link can be used to take input from a link and after doing DEI output
    the frames to output queue.

    DEI can have upto two outputs
    - Output from DEI scaler
    - Output from VIP scaler

    Each can be individually enabled/disabled during link create.

    Each frame output is put in its corresponding output queue.

    @{
*/

/**
    \file deiLink.h
    \brief De-interlacer (DEI) Link API
*/

#ifndef _DEI_LINK_H_
#define _DEI_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/** \brief DEI Scaler output ID */
#define DEI_LINK_OUT_QUE_DEI_SC  (0)

/** \brief VIP Scaler output ID */
#define DEI_LINK_OUT_QUE_VIP_SC  (1)

/** \brief VIP Scaler secondary output ID */
#define DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT  (2)

/** \brief DEI Scaler secondary output ID */
#define DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT  (3)

/** \brief DEI Scaler Tertiary output ID */
#define DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT   (4)

/** \brief Max DEI outputs/output queues */
#define DEI_LINK_MAX_OUT_QUE     (5)

/** \brief Max DEI outputs/output queues */
#define DEI_LINK_MAX_DRIVER_OUT_QUE     (2)

/** \brief Max No. Of channels */
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#define DEI_LINK_MAX_CH                      (16)
#else
#define DEI_LINK_MAX_CH                      (8)
#endif

/** \brief Maximum number of video planes (Y/U/V or R/G/B) */
#define DEI_LINK_MAX_VIDFRAME_PLANES    (3u)

/* @} */

/**
    \ingroup LINK_API_CMD
    \addtogroup DEI_LINK_API_CMD DEI Link Control Commands

    @{
*/

/**
    \brief Link CMD: Get processed data

	\param [OUT] Processed frame data
*/
#define DEI_LINK_CMD_GET_PROCESSED_DATA      (0x3000)

/**
    \brief Link CMD: Enable channel

    \param DeiLink_ChannelInfo * [IN] enable channel
*/
#define DEI_LINK_CMD_ENABLE_CHANNEL          (0x3001)

/**
    \brief Link CMD: Disable channel

    \param DeiLink_ChannelInfo * [IN] disable channel
*/
#define DEI_LINK_CMD_DISABLE_CHANNEL         (0x3002)

/**
    \brief Link CMD: Set Frame Rate

    Set DEI input and output frame rate dynamically

    \param DeiLink_ChFpsParams * [IN] Dei FPS parameters
*/
#define DEI_LINK_CMD_SET_FRAME_RATE          (0x3003)

/**
    \brief Link CMD: Print stats

    Print DEI Link statistic information
    such as input frame rate, output frame rate etc.

    \param NONE
*/
#define DEI_LINK_CMD_PRINT_STATISTICS        (0x3004)

/**
    \brief Link CMD: Set resolution

    Set DEI output resolution dynamically

    \param DeiLink_chDynamicSetOutRes * [IN] Output Resolution parameters
*/
#define DEI_LINK_CMD_SET_OUTPUTRESOLUTION    (0x3005)

/**
    \brief Link CMD: Get resolution

    Get DEI output resolution dynamically

    \param DeiLink_chDynamicSetOutRes * [OUT] Output Resolution parameters

*/
#define DEI_LINK_CMD_GET_OUTPUTRESOLUTION    (0x3006)

/**
    \brief Link CMD: Print buffer status

    Print DEI Link buffer queue status like empty queue count and fill queue count etc.

    \param NONE
*/
#define DEI_LINK_CMD_PRINT_BUFFER_STATISTICS (0x3007)

/**
    \brief Link CMD: Flush previous input fields from DEI

    Flush or release previous input fields

    \param DeiLink_ChFlushParams * [IN] channel id on which flush will happen
*/
#define DEI_LINK_CMD_FLUSH_CHANNEL_INPUT     (0x3008)

/**
    \brief Link CMD: Enable or disable "even odd even" pattern de-interlace

    Only applicable when DEI is ON.

    Only applicable for TI814x and TI810x

    \param DeiLink_ChSetEvenOddEvenPatternDeiParams * [IN] set "Even Odd Even" pattern DEI flag on channel
*/
#define DEI_LINK_CMD_SET_EVEN_ODD_EVEN_PATTERN_DEI (0x3009)

/* @} */

/* Data structure's */

/**
    \brief DEI Link scale mode
*/
typedef enum
{
    DEI_SCALE_MODE_ABSOLUTE,
    /**< Absolute scale mode, provide width & height */

    DEI_SCALE_MODE_RATIO
    /**< Scale based on ratio */

} DEI_SCALE_MODE;

/**
    \brief DEI Link scale ratio parameters
*/
typedef struct
{
	UInt32	numerator;
	/**< numerator of the ratio */

	UInt32	denominator;
	/**< denominator of the ratio */

} DeiLink_OutputScaleRatio;

/**
    \brief DEI Link set ouput resolution
*/
typedef struct
{
    UInt32 queId;
    /**< output queue Id */

    UInt32 chId;
    /**< output channel Id */

    UInt32 width;
    /**< ouput width */

    UInt32 height;
    /**< ouput height */

    UInt32 pitch[DEI_LINK_MAX_VIDFRAME_PLANES];
    /**< ouput pitch */

} DeiLink_chDynamicSetOutRes;

/**
    \brief DEI Link scale factor
*/
typedef struct
{
    UInt32  scaleMode;
    /**< Scale mode for outputs, see DEI_SCALE_MODE for valid values */

    /**< Scale factor for outputs */
    union
    {
        struct
        {
            UInt32 outWidth;
            /**< Required scaled output width */

            UInt32 outHeight;
            /**< Required scaled output height */

        } absoluteResolution;
        struct
        {
            DeiLink_OutputScaleRatio    widthRatio;
            /**< Required scaled output ratio, output width is relative to input width */

            DeiLink_OutputScaleRatio    heightRatio;
            /**< Required scaled output ratio, output height is relative to input height */

        } ratio;
    };
} DeiLink_OutputScaleFactor;

/**
    \brief DEI Link create parameters
*/
typedef struct
{
    System_LinkInQueParams     inQueParams;
    /**< Input queue information. */

    System_LinkOutQueParams    outQueParams[DEI_LINK_MAX_OUT_QUE];
    /**< Output queue information. */

    UInt32                     enableOut[DEI_LINK_MAX_OUT_QUE];
    /**< enableOut[x] = TRUE, enable the corresponding output
         enableOut[x] = FALSE, disable the corresponding output. */

    UInt32                     tilerEnable[DEI_LINK_MAX_OUT_QUE];
    /**< tilerEnable[x] = TRUE, enable TILER format for the corresponding output
         tilerEnable[x] = FALSE, disable TILER format for the corresponding output. */

    UInt32                     comprEnable;
    /**< enable compression for writing back the DEI context to memory.
         ALWAYS keep it as FALSE */

    UInt32                     setVipScYuv422Format;
    /**< setVipScYuv422Format = TRUE, set data format of VIP-SC to YUV422I
         setVipScYuv422Format = FALSE, set data format of VIP-SC to YUV420SP */

    DeiLink_OutputScaleFactor  outScaleFactor[DEI_LINK_MAX_OUT_QUE][DEI_LINK_MAX_CH];
    /**< set scale factors for each channel,
         see DeiLink_OutputScaleRatio for valid values */

    UInt32                     enableDeiForceBypass;
    /**< enableDeiForceBypass = TRUE, de-interlace module will be bypassed
         and data will NOT be de-interlaced
         enableDeiForceBypass = FALSE, data will be de-interlaced */

    UInt32                     enableLineSkipSc;
    /**< enableLineSkipSc = TRUE, alternate line will be skipped while doing scaling
         enableLineSkipSc = FALSE, all the lines will be used to do scaling */

    UInt32                     inputFrameRate[DEI_LINK_MAX_OUT_QUE];
    /**< applicable for progressive input OR when enableDeiForceBypass is TRUE
         inputFrameRate + outputFrameRate should be set to control required fps */

    UInt32                     outputFrameRate[DEI_LINK_MAX_OUT_QUE];
    /**< applicable for progressive input OR when enableDeiForceBypass is TRUE
         inputFrameRate + outputFrameRate should be set to control required fps */

    UInt32                     inputDeiFrameRate;
    /**< applicable for de-interlace module of DEI link
         the inputDeiFrameRate + outputDeiFrameRate will do fps control first
         the output of de-interlace module will go to scalar module of DEI link
         then, the inputDeiFrameRate + outputDeiFrameRate will be used to fps control */

    UInt32                     outputDeiFrameRate;
    /**< applicable for de-interlace module of DEI link
         the inputDeiFrameRate + outputDeiFrameRate will do fps control first
         the output of de-interlace module will go to scalar module of DEI link
         then, the inputDeiFrameRate + outputDeiFrameRate will be used to fps control */

    UInt32                     numBufsPerCh[DEI_LINK_MAX_OUT_QUE];
    /**< buffer number of each channel,
         queue based parameter, all the channels in one queue have the same value */

    Bool                       generateBlankOut[DEI_LINK_MAX_OUT_QUE];
    /**< generateBlankOut[xx] = TURE, all output frames of the queue will dropped
         generateBlankOut[xx] = FALSE, all output frames of the queue will be sent to next links */

} DeiLink_CreateParams;

/**
    \brief DEI Link channel info
*/
typedef struct
{
	UInt32 channelId;
	/**< Dei channel number */

	UInt32 streamId;
	/**< Dei steamId, maps to Queue No */

	UInt32 enable;
	/**< DEI channel enable or disable flag */

} DeiLink_ChannelInfo;

/**
    \brief DEI Link channel dynamic set config params

    Defines Dei FPS parameters that can be changed dynamically
    on a per channel basis
*/
typedef struct DeiLink_ChFpsParams
{
    UInt32 chId;
    /**< Dei channel number */

	UInt32 streamId;
    /**< Dei steamId, maps to Queue No */

    UInt32 inputFrameRate;
    /**< input frame rate - 60 or 50 fps if interlaced */

    UInt32 outputFrameRate;
    /**< Expected output frame rate */

} DeiLink_ChFpsParams;

/**
    \brief DEI Link channel flush params

    Flush or release previous input fields.
    Only applicable when DEI is ON.

    Only applicable for TI814x and TI810x
*/
typedef struct DeiLink_ChFlushParams
{
    UInt32 chId;
    /**< Dei channel number */

} DeiLink_ChFlushParams;

/**
    \brief DEI Link set Even Odd Even pattern de-interlace

    Only applicable when DEI is ON.

    Only applicable for TI814x and TI810x
*/
typedef struct DeiLink_ChSetEvenOddEvenDeiPatternParams
{
    UInt32 chId;
    /**< Dei channel number */

    Bool   enable;
    /**< Set "Even Odd Even" pattern DEI flag to TRUE or FALSE */

} DeiLink_ChSetEvenOddEvenPatternDeiParams;

/* function's */

/**
    \brief DEI Link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 DeiLink_init();

/**
    \brief DEI Link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 DeiLink_deInit();

/**
    \brief Set defaults for create parameter

    \param pPrm [IN] Default parameters
*/
static inline void DeiLink_CreateParams_Init(DeiLink_CreateParams *pPrm)
{
    UInt32 outId, chId;

    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    pPrm->inQueParams.prevLinkQueId = 0;

    for(outId=0; outId<DEI_LINK_MAX_OUT_QUE; outId++)
    {
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        pPrm->numBufsPerCh[outId] = 4;
#else
        pPrm->numBufsPerCh[outId] = 7;
#endif
        pPrm->outQueParams[outId].nextLink = SYSTEM_LINK_ID_INVALID;
        pPrm->enableOut[outId] = FALSE;
        for(chId=0; chId<DEI_LINK_MAX_CH; chId++)
        {
            pPrm->outScaleFactor[outId][chId].scaleMode = DEI_SCALE_MODE_RATIO;
            pPrm->outScaleFactor[outId][chId].ratio.widthRatio.numerator = 1;
            pPrm->outScaleFactor[outId][chId].ratio.widthRatio.denominator = 1;
            pPrm->outScaleFactor[outId][chId].ratio.heightRatio.numerator = 1;
            pPrm->outScaleFactor[outId][chId].ratio.heightRatio.denominator = 1;
        }
        pPrm->generateBlankOut[outId]   = FALSE;
        pPrm->tilerEnable[outId] = FALSE;
    }

    pPrm->comprEnable = FALSE;

    pPrm->setVipScYuv422Format = FALSE;
    pPrm->inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 60;
    pPrm->outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 30;
    pPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;
    pPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;
    pPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = 30;
    pPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = 30;

    pPrm->enableDeiForceBypass = FALSE;
    pPrm->enableLineSkipSc = FALSE;

    pPrm->inputDeiFrameRate  = 30;
    pPrm->outputDeiFrameRate = 30;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/


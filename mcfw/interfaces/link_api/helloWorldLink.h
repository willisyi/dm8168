/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup HELLOWORLD_LINK_API Scalar 5 Link API

    HELLOWORLD Link can be used to take input from a link and after Scaling output
    the frames to output queue.

    HELLOWORLD has one output
    - Output from SC5 scaler


    @{
*/

/**
    \file sclrLink.h
    \brief Scaler  (HELLOWORLD) Link API
*/

#ifndef _HELLOWORLD_LINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#define _HELLOWORLD_LINK_H_

#include <mcfw/interfaces/link_api/system.h>

#define HELLOWORLD_LINK_MAX_CH  (1)
#define HELLOWORLD_LINK_MAX_OUT_QUE (1)

/** \HELLOWORLD command to get processed data */
#define HELLOWORLD_LINK_CMD_GET_PROCESSED_DATA      (0x7000)
/** \HELLOWORLD command to enable channel */
#define HELLOWORLD_LINK_CMD_ENABLE_CHANNEL          (0x7001)
/** \HELLOWORLD command to enable channel */
#define HELLOWORLD_LINK_CMD_DISABLE_CHANNEL         (0x7002)
/** \HELLOWORLD command to set Frame Rate */
#define HELLOWORLD_LINK_CMD_SET_FRAME_RATE          (0x7003)
/** \HELLOWORLD command to print stats */
#define HELLOWORLD_LINK_CMD_PRINT_STATISTICS        (0x7004)

#define HELLOWORLD_LINK_NUM_BUFS_PER_CH_DEFAULT (0)


/**
    \brief HelloWorld link create parameters
*/
typedef struct
{
    /**< System configuration ID. */
    System_LinkInQueParams      inQueParams;
    /**< Input queue information. */
    System_LinkOutQueParams     outQueParams;
    /**< Output queue information. */

    UInt32 maxWidth;
    /**< Set the maximum width (in pixels) of video frame that scd will process */

    UInt32 maxHeight;
    /**< Set the maximum height (in pixels) of video frame that scd will process */

    UInt32 maxStride;
    /**< Set the maximum video frame pitch/stride of the images in the video buffer*/

    UInt32 maxChannels;
    /**< Set the maximum number of channels for which the algorithm instance need to be created */

	UInt32                      createOutBuf1;

	UInt32                      numBufsPerCh;
    /**< Number of output buffers per channel in capture */
} HelloWorldLink_CreateParams;

/**
    \brief HELLOWORLD link channel info
*/
typedef struct
{
	UInt32 channelId;
	UInt32 enable;
} HelloWorldLink_ChannelInfo;

/**
    \brief HELLOWORLD link channel dynamic set config params

    Defines HELLOWORLD FPS parameters that can be changed dynamically
    on a per channel basis 
*/
typedef struct HelloWorldLink_ChFpsParams
{
    UInt32 chId;
    /**< HelloWorld channel number */
    UInt32 inputFrameRate;
    /**< input frame rate - 60 or 50 fps if interlaced */
    UInt32 outputFrameRate;
    /**< Expected output frame rate */
} HelloWorldLink_ChFpsParams;

/**
    \brief HELLOWORLD link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 HelloWorldLink_init();

/**
    \brief HelloWorld link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 HelloWorldLink_deInit();

static inline void HelloWorldLink_CreateParams_Init(HelloWorldLink_CreateParams *pPrm)
{
    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    pPrm->inQueParams.prevLinkQueId = 0;

    pPrm->outQueParams.nextLink = SYSTEM_LINK_ID_INVALID;
    pPrm->maxWidth  = 360;
    pPrm->maxHeight = 240;
	pPrm->maxStride = 360;
	pPrm->maxChannels = 1;
    pPrm->numBufsPerCh  = HELLOWORLD_LINK_NUM_BUFS_PER_CH_DEFAULT;
	pPrm->createOutBuf1 = 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

/*@}*/


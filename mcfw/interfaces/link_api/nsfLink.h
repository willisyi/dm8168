/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup NSF_LINK_API Noise Filter (NSF / NF) Link API

    @{
*/

/**
    \file nsfLink.h
    \brief Noise Filter (NSF / NF) Link API
*/

#ifndef _NSF_LINK_H_
#define _NSF_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */
#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/** \brief Max NSF outputs/output queues */
#define NSF_LINK_MAX_OUT_QUE (2)

/** \brief Indicates number of output buffers to be set to default
     value by the nsf link */
#define NSF_LINK_NUM_BUFS_PER_CH_DEFAULT (0)

/* @} */


/**
    \ingroup LINK_API_CMD
    \addtogroup NSF_LINK_API_CMD NSF Link Control Commands

    @{
*/

/** \brief NSF command to set Frame Rate */
#define NSF_LINK_CMD_SET_FRAME_RATE          (0x8001)

/**
    \brief Link CMD: Print stats

    Print NSF Link statistic information
    such as input frame rate, output frame rate etc.

    \param NONE
*/
#define NSF_LINK_CMD_PRINT_STATISTICS        (0x8002)

/**
    \brief Link CMD: Print buffer status

    Print NSF Link buffer queue status like empty queue count and fill queue count etc.

    \param NONE
*/
#define NSF_LINK_CMD_PRINT_BUFFER_STATISTICS        (0x8003)

/* @} */

/* Data structure's */


typedef struct
{
    System_LinkInQueParams      inQueParams;

    /* Since NSF link can have 2 output queues, incoming channels will
     * be splitted in half automatically if user enables both the queues.
     * Channels {0 to (incomingChannels/2 - 1)} will goto output queue 0 and
     * channels {incomingChannels/2 to incomingChannels} will goto output queue 1.
     * If only 1 output queue is enabled, incoming channels will not be
     * splitted and sent only to output queue 0.
     * For e.g.:
     * Incoming channels = 16, numOutQue = 1 -> outQueue0 = 16, outQueue1 = 0
     * Incoming channels = 16, numOutQue = 2 -> outQueue0 = 8, outQueue1 = 8
     * Incoming channels = 8, numOutQue = 1 -> outQueue0 = 8, outQueue1 = 0
     * Incoming channels = 8, numOutQue = 2 -> outQueue0 = 4, outQueue1 = 4
     */
    UInt32                      numOutQue;
    /**< Nummber of Output queue. */


    System_LinkOutQueParams     outQueParams[NSF_LINK_MAX_OUT_QUE];
    /**< Output queue information. */

    UInt32                      bypassNsf; 
    /**< Bypass Noise filter and does Chroma downsampling.
         applied for all Chs, do only Chroma DS */

    UInt32                      tilerEnable;
    /**< tilerEnable = TRUE, enable TILER format for the output
         tilerEnable = FALSE, disable TILER format for the output. */

    UInt32                      numBufsPerCh;
    /**< Number of output buffers per channel */
    
    UInt32                     inputFrameRate;
    /**< Frames per seconds fed to the NSF Link. 
         inputFrameRate + outputFrameRate should be set to control required fps */

    UInt32                     outputFrameRate;
    /**< Frames per seconds at which frames are generated at the output of NSF Link.
         inputFrameRate + outputFrameRate should be set to control required fps */

} NsfLink_CreateParams;

/**
    \brief NSF link channel dynamic set config params

    Defines NSF Link FPS parameters that can be changed dynamically
    on a per channel basis
*/
typedef struct NsfLink_ChFpsParams
{
    UInt32 chId;
    /**< NSF channel number */

    UInt32 inputFrameRate;
    /**< Input frame rate */

    UInt32 outputFrameRate;
    /**< Expected output frame rate */
} NsfLink_ChFpsParams;

/* function's */
/**
    \brief NSF Link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 NsfLink_init();
/**
    \brief NSF Link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 NsfLink_deInit();

/**
    \brief Set defaults for create parameter

    \param pPrm [IN] Default parameters
*/
static inline void NsfLink_CreateParams_Init(NsfLink_CreateParams *pPrm)
{
    memset(pPrm, 0, sizeof(*pPrm));
    
    pPrm->bypassNsf       = TRUE;
    pPrm->tilerEnable     = FALSE;
    pPrm->numBufsPerCh    = NSF_LINK_NUM_BUFS_PER_CH_DEFAULT;
    pPrm->inputFrameRate  = 30;
    pPrm->outputFrameRate = 30;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/* @} */


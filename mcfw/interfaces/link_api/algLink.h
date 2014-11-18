/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup ALG_LINK_API ALG Link API

    ALG Link can be used to apply some algorithms on frame data of a channel

    Currently below algorithms are supported in ALG Link
    - OSD - On-screen display
    - SCD - Scene change detect

    The same interface is used for different algorithms on different platforms.
    Below table shows the platforms and algorithms supported.

    <table>
        <tr>
            <td>

            </td>
            <td>
                OSD
            </td>
            <td>
                SCD - Tamper Detect (TD)
            </td>
            <td>
                SCD - Live Motion Detect (LMD)
            </td>
        </tr>
        <tr>
            <td>
                DM816x
            </td>
            <td>
                YES - DSP based
            </td>
            <td>
                YES - DSP based
            </td>
            <td>
                YES - DSP based
            </td>
        </tr>
        <tr>
            <td>
                DM814x
            </td>
            <td>
                YES - DSP based
            </td>
            <td>
                YES - DSP based
            </td>
            <td>
                YES - DSP based
            </td>
        </tr>
        <tr>
            <td>
                DM810x
            </td>
            <td>
                YES - SIMCOP based
            </td>
            <td>
                YES - SIMCOP based
            </td>
            <td>
                NO
            </td>
        </tr>
    </table>

    @{
*/

/**
    \file algLink.h
    \brief ALG Link API
*/

#ifndef _ALG_LINK_H_
#define _ALG_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/osdLink.h>
#include <mcfw/interfaces/link_api/scdLink.h>

/* Define's */

/* @{ */

/**
    \brief Max ALG outputs/output queues

    Currently ALG link has two outputs
    - \ref ALG_LINK_SCD_OUT_QUE
    - \ref ALG_LINK_FRAMES_OUT_QUE
*/
#define ALG_LINK_MAX_OUT_QUE     (2)

/**
    \brief SCD meta data output queue

    - ONLY SUPPORTED in DM816x, DM814x
    - This used for LMD (Live motion detect)
*/
#define ALG_LINK_SCD_OUT_QUE     (0)

/**
    \brief the next link to which the frame is forwarded

    - ONLY SUPPORTED in DM810x
    - This is used to forward frames to next link in the chain
    - In DM816x, DM814x, the processed frame is sent back to the previous link which
        is IPC Frames In link (on DSP).
          - IPC Frames In Link in turn sends it to IPC Frames Out link (on M3)
          - IPC Frames Out link (on M3) will forward the frame to the next link i nthe chain
          - Hence ALG_LINK_FRAMES_OUT_QUE is not needed/supported in DM816x, DM814x
*/
#define ALG_LINK_FRAMES_OUT_QUE  (1)

/* @} */

/* Control Commands */

/**
    \ingroup LINK_API_CMD
    \addtogroup ALG_LINK_API_CMD ALG Link Control Commands

    @{
*/

/* @} */

/* Data structure's */

/**
    \brief ALG Link - create parameters
*/
typedef struct
{
    UInt32   enableOSDAlg;
    /**< TRUE: enable OSD Algorithm, FALSE: Disable it */

    UInt32   enableSCDAlg;
    /**< TRUE: enable SCD Algorithm, FALSE: Disable it */

    System_LinkInQueParams  inQueParams;
    /**< Input queue information */

    System_LinkOutQueParams outQueParams[ALG_LINK_MAX_OUT_QUE];
    /**< Output queue information

       See \ref ALG_LINK_SCD_OUT_QUE and \ref ALG_LINK_FRAMES_OUT_QUE for more details
     */

    AlgLink_OsdChCreateParams  osdChCreateParams[ALG_LINK_OSD_MAX_CH];
    /**< Create time OSD params for all channels. */

    AlgLink_ScdCreateParams    scdCreateParams;
    /**< Create time SCD params for all channels. */

} AlgLink_CreateParams;


/* function's */

/**
    \brief Alg link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 AlgLink_init();

/**
    \brief Alg link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 AlgLink_deInit();


/**
    \brief Set defaults for create parameter

    \param pPrm [OUT] Default parameters
*/
static inline void AlgLink_CreateParams_Init(AlgLink_CreateParams *pPrm)
{
    Int32					numBlksInFrame, x, y, i, numHorzBlks, numVertBlks, chId;

    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    pPrm->outQueParams[0].nextLink  = SYSTEM_LINK_ID_INVALID;
    pPrm->outQueParams[1].nextLink  = SYSTEM_LINK_ID_INVALID;

    pPrm->enableOSDAlg = pPrm->enableSCDAlg = FALSE;

    pPrm->scdCreateParams.maxWidth				= 352;
    pPrm->scdCreateParams.maxHeight				= 288;
    pPrm->scdCreateParams.maxStride				= 704;
    pPrm->scdCreateParams.numValidChForSCD      = 0;
    pPrm->scdCreateParams.numBufPerCh           = 3;

    pPrm->scdCreateParams.numSecs2WaitB4Init	= 3;
    pPrm->scdCreateParams.numSecs2WaitB4FrmAlert= 1;
    pPrm->scdCreateParams.numSecs2WaitAfterFrmAlert  = 2;
    pPrm->scdCreateParams.numSecs2Wait2MarkStableFrame = 10;

    pPrm->scdCreateParams.thresold2WaitB4FrmAlert    = ALG_LINK_TIME_TO_WAIT_BEFORE_TAMPER_ALERT_THRESOLD;
    pPrm->scdCreateParams.thresold2WaitAfterFrmAlert = ALG_LINK_TIME_TO_WAIT_AFTER_TAMPER_ALERT_THRESOLD;

    pPrm->scdCreateParams.inputFrameRate        = 30;
    pPrm->scdCreateParams.outputFrameRate       = 5;

    pPrm->scdCreateParams.enableMotionNotify    = TRUE;
    pPrm->scdCreateParams.enableTamperNotify    = TRUE;

    numHorzBlks		= pPrm->scdCreateParams.maxWidth / 32;
    numVertBlks		= pPrm->scdCreateParams.maxHeight / 12;
    numBlksInFrame	= numHorzBlks * numVertBlks;

    for(chId = 0; chId<ALG_LINK_SCD_MAX_CH; chId++)
    {
        pPrm->scdCreateParams.chDefaultParams[chId].chId = chId;
        pPrm->scdCreateParams.chDefaultParams[chId].mode				= ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
        pPrm->scdCreateParams.chDefaultParams[chId].frmIgnoreLightsON	= FALSE;
        pPrm->scdCreateParams.chDefaultParams[chId].frmIgnoreLightsOFF	= FALSE;
        pPrm->scdCreateParams.chDefaultParams[chId].frmSensitivity		= ALG_LINK_SCD_SENSITIVITY_MID;
        pPrm->scdCreateParams.chDefaultParams[chId].frmEdgeThreshold	= 100;
        pPrm->scdCreateParams.chDefaultParams[chId].blkNumBlksInFrame	= numBlksInFrame;

        /* Configure array to monitor scene changes in all frame blocks, i.e., motion detection.
         Each block is fixed to be 32x12 in size */

        i = 0;
        for(y = 0; y < numVertBlks; y++)
        {
            for(x = 0; x < numHorzBlks; x++)
            {
                pPrm->scdCreateParams.chDefaultParams[chId].blkConfig[i].sensitivity = ALG_LINK_SCD_SENSITIVITY_MID;
                pPrm->scdCreateParams.chDefaultParams[chId].blkConfig[i].monitored	 = 1;
                i++;
            }
        }
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/


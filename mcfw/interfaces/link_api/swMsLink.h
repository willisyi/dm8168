/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup SWMS_LINK_API Software Mosaic Link API

    SWMS Link can be used to create the moasic layout, the ouput of SWMS will
    be fed to disply link, the basic functionalities of SWMS are
    - Mosic Layout generation
    - Layout change support
    - Channel to Window map chnage support
    - One SWMS instance per Display is only supported
    - etc

    @{
*/

/**
    \file swMsLink.h
    \brief Software Mosaic Link API
*/

#ifndef _SYSTEM_SW_MS_H_
#define _SYSTEM_SW_MS_H_

/* include files */
#include <mcfw/interfaces/link_api/system.h>

/* Define's */
/* @{ */

/** \brief Max number of SWMS instances supported */
#define SYSTEM_SW_MS_MAX_INST                (4)
/** \brief Max number of windows per mosaic layout */
#define SYSTEM_SW_MS_MAX_WIN                 (36)
/** \brief Number of input channels per SWMS Link/Instance */
#define SYSTEM_SW_MS_MAX_CH_ID               (64)
/** \brief Max number of input channels per SWMS Link/Instance */
#define SYSTEM_SW_MS_ALL_CH_ID               (SYSTEM_SW_MS_MAX_CH_ID + 1)

/** \brief Number of channels per scalar instance for 816x build */
#ifdef TI_816X_BUILD
/** \brief Max Number of channels per scalar instance */
    #define SYSTEM_SW_MS_MAX_WIN_PER_SC      (18)
#endif

/** \brief Number of channels per scalar instance for non 816x build */
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
/** \brief Max Number of channels per scalar instance */
    #define SYSTEM_SW_MS_MAX_WIN_PER_SC      (16)
#endif

/** \brief Channel map can be set as Ivalid channel ID if that particular
     window is not mapped with any valid input channles */
#define SYSTEM_SW_MS_INVALID_ID              (0xFF)
/** \brief Max number od different moasic layouts supported */
#define SYSTEM_SW_MS_MAX_LAYOUTS             (20)
/** \brief Input queue length can be set to this value only if when
     NO input frame drop logic is required in SWMS link even if there
     is buffer accumulation happens at the input side of SWMS */
#define SYSTEM_SW_MS_INVALID_INPUT_QUE_LEN   (0xFF)
/** \brief If Input queue length is set to this value SWMS link
     internally identifies the default Input queue length value */
#define SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN   (0)

/* SWMS link: supported scalar instance */

/**
    SWMS Link: one SWMS can support multiple scalar instances,
    Supported scalar instance are

    @{
*/
/** \brief DEI HQ DEI path Scalar */
#define SYSTEM_SW_MS_SC_INST_DEIHQ_SC        (1)
/** \brief DEI MQ DEI path Scalar */
#define SYSTEM_SW_MS_SC_INST_DEI_SC          (2)
/** \brief SE0 WB VIP0 path Scalar */
#define SYSTEM_SW_MS_SC_INST_VIP0_SC         (3)
/** \brief SE0 WB VIP1 path Scalar */
#define SYSTEM_SW_MS_SC_INST_VIP1_SC         (4)
/** \brief WB path SC5 Scalar */
#define SYSTEM_SW_MS_SC_INST_SC5             (5)
/** \brief DEI MQ DEI path Scalar with DEIH in bypass mode */
#define SYSTEM_SW_MS_SC_INST_DEIHQ_SC_NO_DEI (6)
/** \brief DEI MQ DEI path Scalar with DEI in bypass mode */
#define SYSTEM_SW_MS_SC_INST_DEI_SC_NO_DEI   (7)

/* @} */

/* Control Command's    */
/**
    \ingroup LINK_API_CMD
    \addtogroup SWMS_LINK_API_CMD SWMS Link Control Commands

    @{
*/
/**
    \brief SWMS cmd to Switch layout from current one to a new layout
    \param SwMsLink_LayoutParams * [IN] Command parameters
*/
#define SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT            (0x8000)

/**
    \brief SWMS cmd to Switch layout from current one to a new layout
    \param SwMsLink_LayoutParams * [IN] Command parameters
*/
#define SYSTEM_SW_MS_LINK_CMD_GET_LAYOUT_PARAMS        (0x8001)

/**
    \brief Print detaild IVA-HD statistics
     This is meant to be used by developer for internal debugging purposes
    \param NONE
*/
#define SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS         (0x8002)

/**
    \brief SWMS cmd to Switch layout from current one to a new layout
    \param SwMsLink_LayoutParams * [IN] Command parameters
*/
#define SYSTEM_SW_MS_LINK_CMD_GET_INPUT_CHNL_INFO      (0x8003)

/**
    \brief SWMS cmd to Switch layout from current one to a new layout
    \param SwMsLink_LayoutParams * [IN] Command parameters
*/
#define SYSTEM_SW_MS_LINK_CMD_SET_CROP_PARAM           (0x8004)

/**
    \brief Print detaild IVA-HD statistics
     This is meant to be used by developer for internal debugging purposes
    \param NONE
*/
#define SYSTEM_SW_MS_LINK_CMD_PRINT_BUFFER_STATISTICS  (0x8005)

/**
    \brief SWMS cmd to Switch layout from current one to a new layout
    \param SwMsLink_LayoutParams * [IN] Command parameters
*/
#define SYSTEM_SW_MS_LINK_CMD_FLUSH_BUFFERS            (0x8006)
/* @} */

/**
 *  \brief SWMS layout window info
 *
 *  Defines the structure for each single window parameters of the layout
 */
typedef struct SwMsLink_LayoutWinInfo
{
    UInt32 channelNum;
    /**< channel associated with this window */
    UInt32 bufAddrOffset;
    /**< buffer offset for the window start location,
         NOT TO BE SET BY USER, used for Link internals  */
    UInt32 startX;
    /**< window start-X offset */
    UInt32 startY;
    /**< window start-Y offset */
    UInt32 width;
    /**< window width */
    UInt32 height;
    /**< window height */
    UInt32 bypass;
    /**< TRUE/FALSE - Flag for Low Cost Scaling/DEI enable or disable */
} SwMsLink_LayoutWinInfo;

/**
 *  \brief SWMS window channel params
 *
 *  Defines the structure for the crop parameters of the window input channel
 */
typedef struct SwMsLink_WinInfo
{
    UInt32  winId;
    /**< window ID */
    UInt32  startX;
    /**< Video start-X value */
    UInt32  startY;
    /**< Video start-Y value */
    UInt32  width;
    /**< Video width */
    UInt32  height;
    /**< Video height */
} SwMsLink_WinInfo;

/**
 *  \brief SWMS link layout params
 *
 *  Defines the structure to specify the details of a mosaic layout
 *  Make sure windows are specified in the order of overlap from
 *  background to foreground
 */
typedef struct SwMsLink_LayoutPrm
{
    UInt32 onlyCh2WinMapChanged;
    /**< FALSE: Layout is also changed
         TRUE: Layout is not changed only Channels mapped to windows changed
     */

    UInt32 numWin;
    /**< Display Layout Number of Windows */

    SwMsLink_LayoutWinInfo winInfo[SYSTEM_SW_MS_MAX_WIN];
    /**< Display Layout Individual window coordinates
         see SwMsLink_LayoutWinInfo for details */

    UInt32  ch2WinMap[SYSTEM_SW_MS_MAX_CH_ID];
    /**< Display Layout Channel to Window  Mapping - NOT TO BE SET BY USER */

    UInt32 outputFPS;
    /**< Rate at which output frames should be generated,
         should typically be equal to the display rate
         Example, for 60fps display, outputFPS should be 60
    */
} SwMsLink_LayoutPrm;

/**
*   \brief SWMS link create params
*
*   Defines those parameters that can be configured/set
*   during the SWMS link create time
*/
typedef struct SwMsLink_CreateParams
{
    UInt32      numSwMsInst;
    /**< number of scaler instance in one sw mosaic */

    UInt32      swMsInstId[SYSTEM_SW_MS_MAX_INST];
    /**< scaler ID of each scaler instance */

    UInt32      swMsInstStartWin[SYSTEM_SW_MS_MAX_INST];
    /**< start win ID for each scaler instance */

    System_LinkInQueParams  inQueParams;
    /**< input queue information */

    System_LinkOutQueParams outQueParams;
    /**< output queue information */

    SwMsLink_LayoutPrm    layoutPrm;
    /**< Layout specific params, see SwMsLink_LayoutPrm for details */

    UInt32 maxOutRes;
    /**< output resolution SYSTEM_RES_xxx - decides SWMS buffer params */

    UInt32 initOutRes;
    /**< init time output resolution - decides ouput queue params */

    UInt32 lineSkipMode;
    /**< Enable line skip while scaling, supported value: TRUE/FALSE */

    UInt32 enableLayoutGridDraw;
    /**< Enable/Disable Drawing of Layout specific Grids: TRUE/FALSE */

    UInt32 maxInputQueLen;
    /**< Maximum number of frames that can be queued at input of
     *   sw mosaic for each active window.
     *   If queue length exceeds this value, frames will be dropped */

    UInt32 numOutBuf;
    /**<
        When set to 0 system default is used else
        user specified number of buffers is allocated */

    UInt32 enableOuputDup;
    /**<
        Flag to enable the SWMS ouput dup. This can be enabled only if the
        SWMS layout FPS is <= 30 and the displaylink FPS is 60
        This dup feature will be turned-off automatically in SWMS link if the
        layout FPS is > 30 */

    UInt32 enableProcessTieWithDisplay;
    /**<
        Flag to enable the SWMS doScaling process tie-up with dsiplay
        interrupt. Otherwise scaling is triggered by SWMS internal clock object
    */
} SwMsLink_CreateParams;

/**
 *  \brief SWMS link buffer flush parameters
 *
 *  Defines the structure to flush a channel of the SWMS link
 */
typedef struct SwMsLink_FlushParams
{
    UInt32  chNum;
    /**< Channel number */
    UInt32  holdLastFrame;
    /**< Flag to hold the last frame or not */
} SwMsLink_FlushParams;

/**
*   \brief SWMS link register and init

*   - Creates link task
*   - Registers as a link with the system API

*   \return FVID2_SOK on success
*/
Int32 SwMsLink_init();

/**
*   \brief SWMS link de-register and de-init

*   - Deletes link task
*   - De-registers as a link with the system API

*   \return FVID2_SOK on success
*/
Int32 SwMsLink_deInit();

/**
*   \brief Set defaults values for the SWMS link & channel parameters
*
*   \param pPrm [OUT] Default information
*/
static inline void SwMsLink_CreateParams_Init(SwMsLink_CreateParams *pPrm)
{
    memset(pPrm, 0, sizeof(*pPrm));
    pPrm->enableOuputDup = FALSE;
    pPrm->enableProcessTieWithDisplay = FALSE;
    pPrm->initOutRes = SYSTEM_STD_INVALID;
}

#endif

/* @} */

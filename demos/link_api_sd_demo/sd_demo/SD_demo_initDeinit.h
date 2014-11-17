/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#ifndef _SD_DEMO_INITDEINIT_H_
#define _SD_DEMO_INITDEINIT_H_



#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include "ti_vdis_common_def.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>


/** \brief Bitmask for HDMI VENC */
#define DIS_VENC_HDMI          (0x1u)
/** \brief Bitmask for HDCOMP VENC */
#define DIS_VENC_HDCOMP        (0x2u)
/** \brief Bitmask for DVO2 VENC */
#define DIS_VENC_DVO2          (0x4u)
/** \brief Bitmask for SD VENC. */
#define DIS_VENC_SD            (0x8u)


/**
    \brief Display Device ID

*/
typedef enum
{
    DIS_DEV_HDMI   = 0,
    /**< Display 0 */

    DIS_DEV_HDCOMP = 1,
    /**< Display 1 */

    DIS_DEV_DVO2   = 2,
    /**< Display 2 */

    DIS_DEV_SD     = 3,
    /**< Display 3 */

} DIS_DEV;


/**
    \brief Display Type
*/
typedef enum
{
    DIS_TYPE_SDDAC_COMPOSITE_CVBS  = 0,
    /**< Analog Composite - SDTV */

    DIS_TYPE_DVO2_BT1120_YUV422 = 1,
    /**< Digital 16-bit embedded sync mode via DVO2 - HDTV */

    DIS_TYPE_DVO1_BT1120_YUV422 = 2,
    /**< Digital 16-bit embedded sync mode via DVO1 - HDTV */

    DIS_TYPE_HDDAC_VGA_RGB = 3,
    /**< Analog Component YPbPr - HDTV */

    DIS_TYPE_HDDAC_COMPONENT_YPBPR = 4,
    /**< Analog Component RGB VESA Standard - HDTV */

    DIS_TYPE_HDMI_RGB    = 5,
    /**< Digital HDMI Standard (On-Chip HDMI) - HDTV */

} DIS_TYPE_E;

/**
    \brief TrickPlay Rate
*/
typedef enum
{
    DIS_AVSYNC_1X = 1,
    /**< Normal Speed Playback */

    DIS_AVSYNC_2X = 2,
    /**< 2X Speed Playback */

    DIS_AVSYNC_4X = 4,
    /**< 4X Speed Playback */

    DIS_AVSYNC_8X = 8,
    /**< 8X Speed Playback */

    DIS_AVSYNC_16X = 16,
    /**< 16X Speed Playback */

    DIS_AVSYNC_32X = 32,
    /**< 32X Speed Playback */

    DIS_AVSYNC_HALFX = 100*(1/2),
    /**< 1/2X Speed Playback */

    DIS_AVSYNC_QUARTERX = 100*(1/4),
    /**< 1/4X Speed Playback */

    DIS_AVSYNC_IFRAMEONLY = 0,
    /**< 1/4X Speed Playback */


    DIS_AVSYNC_MAX   = 100
    /**< Maximum Playback Rate */

}DIS_AVSYNC;

/**
    \brief Color space conversion mode
*/
typedef enum
{
    DIS_CSC_MODE_SDTV_VIDEO_R2Y = 0,
    /**< Select coefficient for SDTV Video */

    DIS_CSC_MODE_SDTV_VIDEO_Y2R,
    /**< Select coefficient for SDTV Video */

    DIS_CSC_MODE_SDTV_GRAPHICS_R2Y,
    /**< Select coefficient for SDTV Graphics */

    DIS_CSC_MODE_SDTV_GRAPHICS_Y2R,
    /**< Select coefficient for SDTV Graphics */

    DIS_CSC_MODE_HDTV_VIDEO_R2Y,
    /**< Select coefficient for HDTV Video */

    DIS_CSC_MODE_HDTV_VIDEO_Y2R,
    /**< Select coefficient for HDTV Video */

    DIS_CSC_MODE_HDTV_GRAPHICS_R2Y,
    /**< Select coefficient for HDTV Graphics */

    DIS_CSC_MODE_HDTV_GRAPHICS_Y2R,
    /**< Select coefficient for HDTV Graphics */

    DIS_CSC_MODE_MAX,
    /**< Should be the last value of this enumeration.
         Will be used by driver for validating the input parameters. */

    DIS_CSC_MODE_NONE = 0xFFFFu
    /**< Used when coefficients are provided */

}DIS_COLOR_SPACE_MODE_E;

/**
 * \brief DVO Format
 */
typedef enum
{
    DIS_DVOFMT_SINGLECHAN = 0,
    /**< Ouput data format is single channel with embedded sync */
    DIS_DVOFMT_DOUBLECHAN,
    /**< Output data format is dual channel with embedded sync */
    DIS_DVOFMT_TRIPLECHAN_EMBSYNC,
    /**< Output data format is tripple channel with embedded sync */
    DIS_DVOFMT_TRIPLECHAN_DISCSYNC,
    /**< Ouptut data format is triple channel with discrete sync */
    DIS_DVOFMT_DOUBLECHAN_DISCSYNC,
    /**< Output data format is dual channel with discrete sync */
    DIS_DVOFMT_MAX
    /**< This should be the last Enum */
} DIS_DIGITAL_FMT_E;

/**
 * \brief Analog Format
 */
typedef enum
{
    DIS_A_OUTPUT_COMPOSITE = 0,
    /**< Analog output format composite */
    DIS_A_OUTPUT_SVIDEO,
    /**< Analog output format svideo */
    DIS_A_OUTPUT_COMPONENT,
    /**< Analog output format component */
    DIS_A_OUTPUT_MAX
} DIS_ANALOG_FMT_E;

/**
 * \brief Signal polarity
 */
typedef enum
{
    DIS_POLARITY_ACT_HIGH = 0,
    /**< Signal polarity Active high */
    DIS_POLARITY_ACT_LOW = 1,
    /**< Signal polarity Active low */
    DIS_POLARITY_MAX = 2,
    /**< Signal polarity Active low */
}DIS_SIGNAL_POLARITY_E;

Void SD_Demo_setDefaultCfg(SD_Demo_Ctrl * sd_Demo_ctrl);

Int32 SD_Demo_vDisParams_init(VDIS_PARAMS_S * pContext);
Int32 SD_Demo_displayInit(VDIS_PARAMS_S * pContext);
Int32 SD_Demo_displayCtrlInit();
Int32 SD_Demo_displayCtrlDeInit();

Int32 SD_Demo_videoDecoderInit();
Int32 SD_Demo_configVideoDecoder(Device_VideoDecoderVideoModeParams *modeParams, UInt32 numDevices);

#endif

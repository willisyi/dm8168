/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 * \ingroup DEVICE_DRV_DEVICE_API
 * \defgroup DEVICE_DRV_DEVICE_VID_ENC_API External Video Encoder API
 *
 *  This modules define to configure and control external video encoders like
 *  HDMI transmitters. Typically the external video encoders
 *  interface to the host via one of the DVO ports. I2C is used to send
 *  control commands to the video encoder.
 *
 *  User application controls and configures the video encoder
 *  and based on this in turn configures the DVO ports.
 *
 *  The following video decoder's are currently supported
 *  - SII9022a - HDMI HD transmitter
 *
 *  The API interface used in the FVID2 interface (See also \ref DEVICE_DRV_API)
 *
 *  The following FVID2 APIs are supported by video encoder device drivers,
 *
 *

 - <b> Creating the driver </b> - DEVICE_create()
     <table border="1">
      <tr>
        <th>Parameter</th>
        <th>Value</th>
      </tr>
      <tr>
        <td>drvId</td>
        <td>
        \ref DEVICE_DEVICE_VID_ENC_SII9022A_DRV <br>
        </td>
      </tr>
      <tr>
        <td>instanceId</td>
        <td> Set to 0
        </td>
      </tr>
      <tr>
        <td>createArgs</td>
        <td>
        Device_VideoEncoderCreateParams *
        </td>
      </tr>
      <tr>
        <td>createStatusArgs</td>
        <td>
        Device_VideoEncoderCreateStatus *
        </td>
      </tr>
      <tr>
        <td>cbParams</td>
        <td>
        NOT USED, Set to NULL.
        </td>
      </tr>
    </table>
    \ref DEVICE_Handle returned by DEVICE_create() is used in subsequent FVID2 APIs

  - <b> Deleting the driver </b> - DEVICE_delete()
    <table border="1">
      <tr>
        <th>Parameter</th>
        <th>Value</th>
      </tr>
      <tr>
        <td>deleteArgs</td>
        <td>NOT USED, set to NULL</td>
      </tr>
    </table>

 - <b> Starting the driver </b> - DEVICE_start()
    <table border="1">
      <tr>
        <th>Parameter</th>
        <th>Value</th>
      </tr>
      <tr>
        <td>cmdArgs</td>
        <td>NOT USED, set to NULL</td>
      </tr>
    </table>

 - <b> Stopping the driver </b> - DEVICE_stop()
    <table border="1">
      <tr>
        <th>Parameter</th>
        <th>Value</th>
      </tr>
      <tr>
        <td>cmdArgs</td>
        <td>NOT USED, set to NULL</td>
      </tr>
    </table>

 - <b> Controlling the driver </b> - DEVICE_control() <br>
 See \ref DEVICE_DRV_IOCTL_VID_ENC for the list of IOCTLs supported by the driver. <br>
 All supported video encoders implement these IOCTLs. <br> <br>
   - SII9022A supports further additional specific IOCTLs (See \ref DEVICE_DRV_DEVICE_VID_ENC_SII9022A_API)

 *
 * @{
*/

/**
 *  \file vps_videoEncoder.h
 *
 *  \brief External Video Encoder API
*/

#ifndef _DEVICE_VIDEO_ENCODER_H_
#define _DEVICE_VIDEO_ENCODER_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <device.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


#define DEVICE_VID_ENC_IOCTL_BASE          (DEVICE_CMD_MAX)


/**
  * \addtogroup DEVICE_DRV_IOCTL_VID_ENC
  * @{
*/

/**
  * \brief Get Chip ID
  *
  * This IOCTL can be used to get video  encoder chip information
  * like chip number, revision, firmware/patch revision
  *
  *
  * \param cmdArgs       [IN/OUT]  Device_VideoEncoderChipId *
  * \param cmdArgsStatus [OUT] NULL
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_ENCODER_GET_CHIP_ID       \
            (DEVICE_VID_ENC_IOCTL_BASE + 0x00)

/**
  * \brief Configure HDMI
  *
  * This IOCTL can be used to configure HDMI for mode.
  *
  *
  * \param cmdArgs       [IN]  Device_VideoEncoderConfigParams *
  * \param cmdArgsStatus [OUT] NULL
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_ENCODER_SET_MODE       \
            (DEVICE_VID_ENC_IOCTL_BASE + 0x01)

/* @} */

/**
 * \brief Enum defining ID of the standard Modes.
 *
 *  Standard timinig parameters
 *  will be used if the standard mode id is used for configuring mode
 *  in the hdmi.
 */
typedef enum
{
    DEVICE_VIDEO_ENCODER_MODE_NTSC = 0,
    /**< Mode Id for NTSC */
    DEVICE_VIDEO_ENCODER_MODE_PAL,
    /**< Mode Id for PAL */
    DEVICE_VIDEO_ENCODER_MODE_1080P_60,
    /**< Mode Id for 1080p at 60fps mode */
    DEVICE_VIDEO_ENCODER_MODE_720P_60,
    /**< Mode Id for 720p at 60fps mode */
    DEVICE_VIDEO_ENCODER_MODE_1080I_60,
    /**< Mode Id for 1080I at 60fps mode */
    DEVICE_VIDEO_ENCODER_MODE_1080P_30,
    /**< Mode Id for 1080P at 30fps mode */
    DEVICE_VIDEO_ENCODER_MAX_MODE
    /**< This should be the last mode id */
} Device_VideoEncoderOutputModeId;

/**
 * \brief Enum defining external or embedded sync mode.
 */
typedef enum
{
    DEVICE_VIDEO_ENCODER_EXTERNAL_SYNC,
    /**< HDMI in external sync mode i.e. H-sync and V-sync are external */
    DEVICE_VIDEO_ENCODER_EMBEDDED_SYNC,
    /**< Embedded sync mode */
    DEVICE_VIDEO_ENCODER_MAX_SYNC
    /**< This should be the last mode id */
} Device_VideoEncoderSyncMode;

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */


/**
 * \brief Structure for getting HDMI chip identification Id
 */
typedef struct
{
    UInt32                  chipId;
    /**< Chip ID, value is device specific */

    UInt32                  chipRevision;
    /**< Chip revision, value is device specific  */

    UInt32                  firmwareVersion;
    /**< Chip internal patch/firmware revision, value is device specific */

} Device_VideoEncoderChipId;


/**
  * \brief Arguments for DEVICE_create()
*/
typedef struct
{

    UInt32                          deviceI2cInstId;
    /**< I2C device instance ID to use 0 or 1 */
    UInt32                          deviceI2cAddr;
    /**< I2C device address for each device */
    UInt32                          inpClk;
    /**< input clock*/
    UInt32                          hdmiHotPlugGpioIntrLine;
    /**< HDMI hot plug GPIO interrupt line no */
    UInt32                          syncMode;
    /**< Sync Mode. See #Device_VideoEncoderSyncMode for valid values */
    UInt32                          clkEdge;
    /**< Specifies the clock edge to be used to latch data on.
         FALSE spacifies to latch on falling edge, raising edge otherwise */
} Device_VideoEncoderCreateParams;

/**
  * \brief Status of DEVICE_create()
*/
typedef struct
{

    Int32   retVal;
    /**< DEVICE_SOK on success, else failure */
} Device_VideoEncoderCreateStatus;


/**
  * \brief configuration paramters for HDMI
*/
typedef struct
{
    Device_VideoEncoderOutputModeId  ouputMode;
    /**< output mode of hdmi */

    Device_VideoEncoderSyncMode      syncMode;
    /**< Select either embedded or external sync */

} Device_VideoEncoderConfigParams;

#endif

/*@}*/


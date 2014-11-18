/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 * \ingroup DEVICE_DRV_DEVICE_API
 * \defgroup DEVICE_DRV_DEVICE_VID_DEC_API External Video Decoder API
 *
 *  This modules define API to capture video data using external video
 *  decoders like TVP5158, TVP7002. Typically the external video decoders
 *  interface to the host via one of the VIP ports. I2C is used to send
 *  control commands to the video decoder.
 *
 *  User application controls and configures the video decoder
 *  and based on this in turn configures the VIP port.
 *
 *  The following video decoder's are currently supported
 *  - TVP5158 - Multi-CH NTSC/PAL video decoder
 *  - TVP7002 - Component HD video decoder
 *  - SII9135 - HDMI HD video decoder
 *
 *  The API interface used in the FVID2 interface (See also \ref DEVICE_DRV_API)
 *
 *  The following FVID2 APIs are supported by video decoder device drivers,
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
        \ref DEVICE_VID_DEC_TVP5158_DRV <br>
        \ref DEVICE_VID_DEC_TVP7002_DRV <br>
        \ref DEVICE_VID_DEC_SII9135_DRV <br>
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
        Device_VideoDecoderCreateParams *
        </td>
      </tr>
      <tr>
        <td>createStatusArgs</td>
        <td>
        Device_VideoDecoderCreateStatus *
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
 See \ref DEVICE_DRV_IOCTL_VID_DEC for the list of IOCTLs supported by the driver. <br>
 All supported video decoders implement these IOCTLs. <br> <br>
   - TVP5158 supports further additional specific IOCTLs (See \ref DEVICE_DRV_DEVICE_VID_DEC_TVP5158_API)
   - SII9153 and TVP7002 do not support any additional specific IOCTLs.

Typicall API calling sequence for video decoder in context of video capture application would be like below
- DEVICE_create() - create the video decoder device handle
- \ref IOCTL_DEVICE_VIDEO_DECODER_GET_CHIP_ID - check if device is active
- \ref IOCTL_DEVICE_VIDEO_DECODER_RESET  - reset it
- \ref IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_MODE - setup the device for a particular mode
- \ref IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS - check if video source is detected
- Create capture driver based on video decoder setup and video decoder status.
- Start capture driver
- \ref DEVICE_start() - enable video decoder to start data output to VIP capture port
- Do video capture using capture driver.
- Stop capture driver
- \ref DEVICE_stop() - disable video decoder device
- \ref DEVICE_delete() - delete video decoder device
- Delete the capture driver

 *
 * @{
*/

/**
 *  \file vps_videoDecoder.h
 *
 *  \brief External Video Decoder API
*/


#ifndef _DEVICE_VIDEO_DECODER_H_
#define _DEVICE_VIDEO_DECODER_H_


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

// #include <ti/psp/devices/vps_device.h>
// #include <ti/psp/vps/vps_capture.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


#define DEVICE_VID_DEC_IOCTL_BASE                   (DEVICE_CMD_MAX)


/** \brief Max devices that can be associated with a single VIP port  */
#define DEVICE_VIDEO_DECODER_DEV_PER_PORT_MAX        (2)

/** \brief Value to use, when it is needed to apply device specific default
  *        setting value
*/
#define DEVICE_VIDEO_DECODER_DEFAULT                 ((UInt32)-1)

/** \brief Value to use, when it is needed to NOT change a device specific
  *        setting value
*/
#define DEVICE_VIDEO_DECODER_NO_CHANGE               ((UInt32)-2)

/** \brief Value to use, when there no GPIO is used for device reset
*/
#define DEVICE_VIDEO_DECODER_GPIO_NONE               ((UInt32)-1)


/**
  * \addtogroup DEVICE_DRV_IOCTL_VID_DEC
  * @{
*/

/**
  * \brief Get Chip ID
  *
  * This IOCTL can be used to get video decoder chip information
  * like chip number, revision, firmware/patch revision
  *
  *
  * \param cmdArgs       [IN]  Device_VideoDecoderChipIdParams *
  * \param cmdArgsStatus [OUT] Device_VideoDecoderChipIdStatus *
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_DECODER_GET_CHIP_ID       \
            (DEVICE_VID_DEC_IOCTL_BASE + 0x00)


/**
  * \brief Reset video decoder chip
  *
  * This IOCTL can be used to reset video decoder chip to power ON reset state
  *
  *
  * \param cmdArgs       [IN]   NULL
  * \param cmdArgsStatus [OUT]  NULL
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_DECODER_RESET             \
            (DEVICE_VID_DEC_IOCTL_BASE + 0x01)

/**
  * \brief Configure video decoder video mode
  *
  * This IOCTL can be used to configure video decoder in a specific mode
  * like YUV output, 16-bit output, input standard selection, auto detection
  * of input standard and so on
  *
  * When standard is set as AUTO-DETECT, this API will wait internally until
  * video is detected (upto a timeout limit set by user).
  *
  * Once the API returns IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS can be used
  * to get the detected video standard information
  *
  * \param cmdArgs       [IN]  Device_VideoDecoderVideoModeParams *
  * \param cmdArgsStatus [OUT] NULL
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_MODE    \
            (DEVICE_VID_DEC_IOCTL_BASE + 0x02)

/**
  * \brief Get video status
  *
  * This IOCTL can be used to detect current video status like video standard
  * widthxheight, interlaced or progressive video etc
  *
  * Note, this API will not wait until video is detected. User can call this
  * API repeatedly if they want to wait until video is detected
  *
  * This API can be called independently for every channel in case of
  * multi-channel video decoders
  *
  * This API can be called only after calling
  * IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_MODE with appropriate parameters.
  *
  * \param cmdArgs       [IN]   Device_VideoDecoderVideoStatusParams *
  * \param cmdArgsStatus [OUT]  Device_VideoDecoderVideoStatus *
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS  \
            (DEVICE_VID_DEC_IOCTL_BASE + 0x03)

/**
  * \brief Set video color processing related parameters
  *
  * This IOCTL can be called separetly for each channel.
  *
  * \param cmdArgs       [IN]   Device_VideoDecoderColorParams *
  * \param cmdArgsStatus [OUT]  NULL
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_COLOR   \
            (DEVICE_VID_DEC_IOCTL_BASE + 0x04)

/**
  * \brief Low level I2C register write
  *
  * \param cmdArgs       [IN]   Device_VideoDecoderRegRdWrParams *
  * \param cmdArgsStatus [OUT]  NULL
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_DECODER_REG_WRITE         \
            (DEVICE_VID_DEC_IOCTL_BASE + 0x08)

/**
  * \brief Low level I2C register read
  *
  * \param cmdArgs       [IN]   Device_VideoDecoderRegRdWrParams *
  * \param cmdArgsStatus [OUT]  NULL
  *
  * \return FVID_SOK on success, else failure
  *
*/
#define IOCTL_DEVICE_VIDEO_DECODER_REG_READ          \
            (DEVICE_VID_DEC_IOCTL_BASE + 0x09)

/* @} */

/**
 *  \brief Video system
*/
typedef enum
{
  DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_NTSC = 0,
  /**< Interlaced, NTSC system, 720x240 per field */

  DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_PAL,
  /**< Interlaced, PAL system, 720x288 per field */

  DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT,
  /**< Auto-detect NTSC or PAL system */

  DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_NONE,
  /**< Neither NTSC nor PAL system */

  DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_MAX
  /**< Max video system */

} Device_VideoDecoderVideoSystem;

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
/**
  * \brief Arguments for DEVICE_create()
*/
typedef struct
{

  UInt32  deviceI2cInstId;
  /**< I2C device instance ID to use 0 or 1 */

  UInt32  numDevicesAtPort;
  /**< Number of devices connected to a video port  */

  UInt32  deviceI2cAddr[DEVICE_VIDEO_DECODER_DEV_PER_PORT_MAX];
  /**< I2C device address for each device */

  UInt32  deviceResetGpio[DEVICE_VIDEO_DECODER_DEV_PER_PORT_MAX];
  /**< GPIO number that is used to reset the device
   *
   *   In case HW does not have any such GPIO then set this to
   *   DEVICE_VIDEO_DECODER_GPIO_NONE
   */


} Device_VideoDecoderCreateParams;

/**
  * \brief Status of DEVICE_create()
*/
typedef struct
{

  Int32   retVal;
  /**< DEVICE_SOK on success, else failure */

} Device_VideoDecoderCreateStatus;

/**
 * \brief Arguments for \ref IOCTL_DEVICE_VIDEO_DECODER_GET_CHIP_ID
*/
typedef struct
{

  UInt32 deviceNum;
  /**< Device number for which chip ID is required
   *
   *   Device number will be 0 in most cases, unless multiple devices
   *   are cascaded together like in TVP5158 in which case device num
   *   can be from 0 .. Device_VideoDecoderCreateParams.numDevicesAtPort-1
   */

} Device_VideoDecoderChipIdParams;

/**
 * \brief Status for \ref IOCTL_DEVICE_VIDEO_DECODER_GET_CHIP_ID
*/
typedef struct
{

  UInt32 chipId;
  /**< Chip ID, value is device specific  */

  UInt32 chipRevision;
  /**< Chip revision, value is device specific  */

  UInt32 firmwareVersion;
  /**< Chip internal patch/firmware revision, value is device specific  */

} Device_VideoDecoderChipIdStatus;

/**
  * \brief Arguments for \ref IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_MODE
*/
typedef struct
{

  UInt32 videoIfMode;
  /**< 8 or 16 or 24-bit video interface mode.

     For valid values see #Device_CaptVideoIfMode
   */

  UInt32 videoDataFormat;
  /**< RGB or YUV data format. valid values are given below \n
      DEVICE_DF_YUV422P, ( 'P' is not relavent for input data format) \n
      DEVICE_DF_YUV444P, ( 'P' is not relavent for input data format) \n
      DEVICE_DF_RGB24_888.

      For valid values see #DEVICE_DataFormat.
   */

  UInt32 standard;
  /**< Video decoder video standard
    *
    *  When set to DEVICE_STD_AUTO_DETECT the video resolution
    *  will be auto-detected but user will still need to provide other
    *  parameters like videoIfMode, videoDataFormat, videoCaptureMode
    *
    *  For valid values see #DEVICE_Standard
    */

  UInt32 videoCaptureMode;
  /**< Multiplexing mode, line mux or pixel mux or split line mode

       For valid values see #Device_CaptVideoCaptureMode
  */

  UInt32 videoSystem;
  /**< Video system, NTSC or PAL or NONE or Auto-detect
    *
    *  When set to DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT the video
    *  system NTSC or PAL will be auto-detected but user will
    *  still need to provide other parameters like videoIfMode,
    *  videoDataFormat, videoCaptureMode, standard
    *
    *  For valid values see #Device_VideoDecoderVideoSystem
    */

  Int32 videoAutoDetectTimeout;
  /**< Auto-detect timeout, when auto-detecting standard in OS ticks
   *
   * Valid only when
   *
   * Device_VideoDecoderVideoModeParams.standard
   *   = DEVICE_STD_AUTO_DETECT
   *
   * OR
   *
   * Device_VideoDecoderVideoModeParams.videoSystem
   *   = DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT
   */

   UInt32 videoCropEnable;
    /**< Applicable only for D1/CIF capture input.
     *   Crops 720 to 704 and 360 to 352 at video decoder.
     *   Only applied if video decoder supports this mode.
     */
} Device_VideoDecoderVideoModeParams;

/**
 * \brief Arguments for \ref IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_COLOR
*/
typedef struct
{

  UInt32 channelNum;
  /**< Channel number for which these parameters are to be applied
   *
   *   Channel number will be 0 in most cases, unless the device is capable of
   *   multi-channel capture like in TVP5158 in which case channel number
   *   can be from 0 .. Device specific max channels-1
   */

  UInt32 videoBrightness;
  /**< Video brightness, value is device specific
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_DEFAULT when device specific
   * default needs to be applied in current IOCTL call.
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_NO_CHANGE when last applied
   * value needs to be retained in current IOCTL call.
   *
   */

  UInt32 videoContrast;
  /**< Video contrast, value is device specific
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_DEFAULT when device specific
   * default needs to be applied in current IOCTL call.
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_NO_CHANGE when last applied
   * value needs to be retained in current IOCTL call.
   *
   */

  UInt32 videoSaturation;
  /**< Video saturation, value is device specific
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_DEFAULT when device specific
   * default needs to be applied in current IOCTL call.
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_NO_CHANGE when last applied
   * value needs to be retained in current IOCTL call.
   *
   */

  UInt32 videoSharpness;
  /**< Video sharpness, value is device specific
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_DEFAULT when device specific
   * default needs to be applied in current IOCTL call.
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_NO_CHANGE when last applied
   * value needs to be retained in current IOCTL call.
   *
   */

  UInt32 videoHue;
  /**< Video hue, value is device specific
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_DEFAULT when device specific
   * default needs to be applied in current IOCTL call.
   *
   * This field can be set to \ref DEVICE_VIDEO_DECODER_NO_CHANGE when last applied
   * value needs to be retained in current IOCTL call.
   *
   */

} Device_VideoDecoderColorParams;

/**
  * \brief Arguments for \ref IOCTL_DEVICE_VIDEO_DECODER_REG_WRITE
  *        and \ref IOCTL_DEVICE_VIDEO_DECODER_REG_READ
*/
typedef struct
{

  UInt32  deviceNum;
  /**< Device from which to do I2C read/write
   *
   *   Device number will be 0 in most cases, unless multiple devices
   *   are cascaded together like in TVP5158 in which case device num
   *   can be from 0 .. Device_VideoDecoderCreateParams.numDevicesAtPort-1
   */

  UInt32  numRegs;
  /**< Number of registers to read */

  UInt8  *regAddr;
  /**< Array of register addresses */

  UInt8  *regValue8;
  /**< Array of register value to write or read back */

} Device_VideoDecoderRegRdWrParams;



#endif  /*  _DEVICE_VIDEO_DECODER_H_  */

/*@}*/



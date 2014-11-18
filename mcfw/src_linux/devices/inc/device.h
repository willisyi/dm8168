/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 * \defgroup DEVICE_DRV_DEVICE_API External Device Interface API
 *
 *  This module defines APIs for external video devices like video
 *  encoders, video decoders, video filters
 *
 *  Typically I2C is used to send control commands to these external devices.
 *  The external device drivers make use of I2C wrapper APIs defined in this module.
 *  The I2C wrapper APIs in turn use the I2C driver to do the actual I2C transfer.
 *
 * @{
*/

/**
 *  \file vps_device.h
 *
 *  \brief External Video Device API
*/

#ifndef _DEVICE_VIDEO_DEVICE_H_
#define _DEVICE_VIDEO_DEVICE_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/**
 *  \name External Video Device Driver ID
 *
 *  Used as drvId when calling FVID2_create()
 *
 */

/* @{ */

#define DEVICE_CMD_BASE   (0x0)
#define DEVICE_CMD_START  (DEVICE_CMD_BASE + 0x1)
#define DEVICE_CMD_STOP   (DEVICE_CMD_BASE + 0x2)
#define DEVICE_CMD_MAX    (DEVICE_CMD_BASE + 0x3)


#define DEVICE_VID_DEC_DRV_BASE            (0x00000400u)

#define DEVICE_VID_ENC_DRV_BASE            (0x00000800u)



/** \brief TVP5158 video decoder driver ID used at the time of FVID2_create() */
#define DEVICE_VID_DEC_TVP5158_DRV    (DEVICE_VID_DEC_DRV_BASE + 0x0000u)

/** \brief TVP7002 video decoder driver ID used at the time of FVID2_create() */
#define DEVICE_VID_DEC_TVP7002_DRV    (DEVICE_VID_DEC_DRV_BASE + 0x0001u)

/** \brief HDMI SII9135 video decoder driver ID used at the time of FVID2_create() */
#define DEVICE_VID_DEC_SII9135_DRV    (DEVICE_VID_DEC_DRV_BASE + 0x0002u)

/** \brief HDMI SII233A video decoder driver ID used at the time of FVID2_create() */
#define DEVICE_VID_DEC_SII9233A_DRV   (DEVICE_VID_DEC_DRV_BASE + 0x0003u)

/** \brief HDMI SII9022a video encoder driver ID used at the time of FVID2_create() */
#define DEVICE_VID_ENC_SII9022A_DRV   (DEVICE_VID_ENC_DRV_BASE + 0x0000u)

/** \brief HDMI SII9134 video encoder driver ID used at the time of FVID2_create() */
#define DEVICE_VID_ENC_SII9134_DRV    (DEVICE_VID_ENC_DRV_BASE + 0x0001u)

#define DEVICE_CA_BOARD_A1_IO_EXP_I2C_ADDR (0x21u)

/* @} */

/**
 *  \name I2C instance ID
 *
 *  Used with I2C APIs
 */

/* @{ */

/** \brief I2C instance 0 */
#define DEVICE_I2C_INST_ID_0   (0)

/** \brief I2C instance 1 */
#define DEVICE_I2C_INST_ID_1   (1)

/** \brief I2C instance 2 */
#define DEVICE_I2C_INST_ID_2   (2)

/** \brief I2C instance 3 */
#define DEVICE_I2C_INST_ID_3   (3)

/** \brief I2C instance not used

    Used as value forDevice_InitParams.i2cClkKHz[n]
    when it is not needed to init i2c instance 'n'
*/
#define DEVICE_I2C_INST_NOT_USED   (0xFFFF)

/* @} */

/**
 *  \name Max limits
 */

/* @{ */

/** \brief Max I2C instance's */
#define DEVICE_I2C_INST_ID_MAX (4)

/** \brief Max I2C instance's */
#define DEVICE_CAPT_INST_MAX  (4)

/** \brief Max handles per external device driver  */
#define DEVICE_MAX_HANDLES  (4)

/* @} */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/**
  \brief External video device sub-system init parameters
*/
typedef struct
{
    Ptr     i2cRegs[DEVICE_I2C_INST_ID_MAX];
    /**< I2C peripheral base address */

    UInt32  i2cIntNum[DEVICE_I2C_INST_ID_MAX];
    /**< I2C Interrupt number */

    UInt32  i2cClkKHz[DEVICE_I2C_INST_ID_MAX];
    /**< I2C bus clock in KHz

      Set to DEVICE_I2C_INST_NOT_USED in case
      I2C instance init is not needed
    */

    UInt32  isI2cInitReq;
    /**< Indicates whether I2C initialization is required */
}Device_InitParams;

/**
 * \brief Video capture operation mode
*/
typedef enum
{
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_SINGLE_CH_NON_MUX_EMBEDDED_SYNC = 0,
    /**< Single Channel non multiplexed mode */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_LINE_MUX_EMBEDDED_SYNC,
    /**< Multi-channel line-multiplexed mode */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_PIXEL_MUX_EMBEDDED_SYNC,
    /**< Multi-channel pixel muxed */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_SINGLE_CH_NON_MUX_DISCRETE_SYNC_HSYNC_VBLK,
    /**< Single Channel non multiplexed discrete sync mode with HSYNC and
        VBLK as control signals. */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_SINGLE_CH_NON_MUX_DISCRETE_SYNC_HSYNC_VSYNC,
    /**< Single Channel non multiplexed discrete sync mode with HSYNC and
        VSYNC as control signals. */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_SINGLE_CH_NON_MUX_DISCRETE_SYNC_ACTVID_VBLK,
    /**< Single Channel non multiplexed discrete sync mode with ACTVID and
        VBLK as control signals. */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_SINGLE_CH_NON_MUX_DISCRETE_SYNC_ACTVID_VSYNC,
    /**< Single Channel non multiplexed discrete sync mode with ACTVID and
        VBLK as control signals. */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_LINE_MUX_SPLIT_LINE_EMBEDDED_SYNC,
    /**< Multi-channel line-multiplexed mode - split line mode */
    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MAX
    /**< Maximum modes */
}Device_CaptVideoCaptureMode;

/**
 * \brief Video interface mode
 */
typedef enum
{
    DEVICE_CAPT_VIDEO_IF_MODE_8BIT = 0,
    /**< Embedded sync mode:  8bit - BT656 standard  */
    DEVICE_CAPT_VIDEO_IF_MODE_16BIT,
    /**< Embedded sync mode:  16bit - BT1120 standard  */
    DEVICE_CAPT_VIDEO_IF_MODE_24BIT,
    /**< Embedded sync mode:  24bit */
    DEVICE_CAPT_VIDEO_IF_MODE_MAX
    /**< Maximum modes */
} Vps_CaptVideoIfMode;



/* ========================================================================== */
/*                         Functions                                          */
/* ========================================================================== */

/**
  \brief Retrieves i2c address for the decoder

  \param vidDecId   [IN] decoder id
  \param vipInstId  [IN] instance id

  \return FVID2_SOK slave device found, else slave device not found
*/
UInt32 Device_getVidDecI2cAddr(UInt32 vidDecId, UInt32 vipInstId);

/* ------To be implemented ------ */

/**
  \brief Enable/disable debug prints from I2C driver

  Debug prints disabled by default

  \param enable   [IN] TRUE: enable, FALSE: disable

  \return FVID2_SOK on success else failure
*/
Int32 Device_deviceI2cDebugEnable(UInt32 enable);

/**
  \brief Probes an I2C bus for all video devices.

  Probes an I2C bus for all possibly connected slaves to it.
  Prints the detected slave address on the console.

  \param i2cInstId  [IN] \ref DEVICE_I2C_INST_ID_0 or \ref DEVICE_I2C_INST_ID_1

  \return FVID2_SOK on success else failure
*/
Int32 Device_deviceI2cProbeAll(UInt16 i2cInstId);

/**
  \brief Probes an I2C bus for a specific device slave address

  \param i2cInstId  [IN] \ref DEVICE_I2C_INST_ID_0 or \ref DEVICE_I2C_INST_ID_1
  \param slaveAddr  [IN] Slave I2C address

  \return FVID2_SOK slave device found, else slave device not found
*/
Int32 Device_deviceI2cProbeDevice(UInt16 i2cInstId, UInt8 slaveAddr);


#endif /*  _DEVICE_VIDEO_DEVICE_H_  */

/*@}*/

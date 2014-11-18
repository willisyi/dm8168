/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 *  \file vpsdrv_hdmi9022aPriv.h
 *
 *  \brief HDMI9022a driver
 *  This file implements functionality for the HDMI.
 *
 */

#ifndef _DEVICE_9022A_PRIV_H_
#define _DEVICE_9022A_PRIV_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <device_videoEncoder.h>
#include <device_sii9022a.h>

#include <osa_sem.h>
#include <osa_i2c.h>




/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/** \brief SII9022A handle state - not used. */
#define VPS_SII9022A_OBJ_STATE_UNUSED   (0)

/** \brief SII9022A handle state -  used */
#define VPS_SII9022A_OBJ_STATE_USED     (1)

/** \brief SII9022A device state -  initilized succefully */
#define VPS_SII9022A_NOT_INITED         (0)

/** \brief SII9022A device state -  initilized succefully */
#define VPS_SII9022A_INITED             (1)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/**
 * \brief HDMI Object.
 */
typedef struct
{
    UInt32 instId;
    /* instance id */
    Device_VideoEncoderCreateParams createArgs;
    /* create time arguments */
    Device_HdmiChipId  hdmiChipid;
    /* HDMI Chip id */
    UInt32  hpd;
    /* HDMI Hot plug detect */
    UInt32 syncCfgReg;
    /**< Sync Configuration Register */
    UInt32 syncPolarityReg;
    UInt32 isRgbOutput;
    UInt32 inBusCfg;
} Device_Sii9022aObj;

typedef Device_Sii9022aObj * Device_Sii9022aHandle;

/*
  Overall HDMI driver object
*/
typedef struct
{
    OSA_I2cHndl i2cHandle;
    /** < i2c handle for i2c read/write*/
    Device_Sii9022aHandle sii9022aHandle[DEVICE_MAX_HANDLES];
    /** < HDMI handle objects */
    Device_SiI9022aPrms prms;
    /**< 9022A params */
} Device_Sii9022aCommonObj;


Device_Sii9022aCommonObj gDevice_sii9022aCommonObj;

#endif


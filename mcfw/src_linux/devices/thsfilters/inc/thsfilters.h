/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
*  \file vpsdrv_ths.h
*
*  \brief .
*
*/

#ifndef _VIDEO_DEVICE_THS_H_
#define _VIDEO_DEVICE_THS_H_

#include <device_thsfilters.h>


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* TODO: Get the I2C address of PCF8575 from platform file */
#define DEVICE_PCF8575_I2C_ADDR            (0x20)


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

Int32 Device_thsFiltersInit();

Int32 Device_ths7375SetParams(Device_ThsFilterCtrl ctrl);

Int32 Device_ths7360SetSdParams(Device_ThsFilterCtrl ctrl);

Int32 Device_ths7360SetSfParams(Device_Ths7360SfCtrl ctrl);

Int32 Device_thsFiltersDeInit();

#endif

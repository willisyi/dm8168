/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 *  \file vpsdrv_ths.c
 *
 *  \brief I2C driver
 *  This file implements functionality for the THS7375 and THS8360.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */


#include "ti_media_std.h"
#include <device_videoDecoder.h>
#include <device_thsfilters.h>
#include <thsfilters.h>
#include <thsfilters_priv.h>


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Maximum ports supported by the PCF8575 */
#define DEVICE_PCF8575_MAX_PORTS           (2u)

/* Macros for accessing for ports */
#define DEVICE_PCF8575_PORT0               (0u)
#define DEVICE_PCF8575_PORT1               (1u)

#ifdef TI_816X_BUILD
/* Macros for PCF8575 Pins */
#define DEVICE_PCF8575_PIN0                (0x1)
#define DEVICE_PCF8575_PIN1                (0x2)
#define DEVICE_PCF8575_PIN2                (0x4)
#define DEVICE_PCF8575_PIN3                (0x8)
#define DEVICE_PCF8575_PIN4                (0x10)
#define DEVICE_PCF8575_PIN5                (0x20)
#define DEVICE_PCF8575_PIN6                (0x40)
#define DEVICE_PCF8575_PIN7                (0x80)

#define DEVICE_PCF8575_PIN10               (0x1)
#define DEVICE_PCF8575_PIN11               (0x2)
#else

/* Macros for PCF8575 Pins */
#define DEVICE_PCF8575_PIN0                (0x10)
#define DEVICE_PCF8575_PIN1                (0x20)
#define DEVICE_PCF8575_PIN2                (0x4)
#define DEVICE_PCF8575_PIN3                (0x8)
#define DEVICE_PCF8575_PIN4                (0x2)
#define DEVICE_PCF8575_PIN5                (0x1)
#define DEVICE_PCF8575_PIN6                (0x40)
#define DEVICE_PCF8575_PIN7                (0x80)

#define DEVICE_PCF8575_PIN10               (0x1)
#define DEVICE_PCF8575_PIN11               (0x2)
#endif

#define DEVICE_THS7375_MASK                (DEVICE_PCF8575_PIN10 | DEVICE_PCF8575_PIN11)

#define DEVICE_THS7360_SD_MASK             (DEVICE_PCF8575_PIN2 | DEVICE_PCF8575_PIN5)

#define DEVICE_THS7360_SF_MASK             (DEVICE_PCF8575_PIN0 |                    \
                                         DEVICE_PCF8575_PIN1 |                    \
                                         DEVICE_PCF8575_PIN3 |                    \
                                         DEVICE_PCF8575_PIN4)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Static array to keep track of each pin value for each port */

#if defined(TI_816X_BUILD) 
	static UInt8 DevicePcf8575Port[DEVICE_PCF8575_MAX_PORTS] = {0x0u, 0x0u};
#endif

#if defined(TI_814X_BUILD) 
	static UInt8 DevicePcf8575Port[DEVICE_PCF8575_MAX_PORTS] = {0x0u, 0x0u};
#endif

#if defined(TI_8107_BUILD) 
	static UInt8 DevicePcf8575Port[DEVICE_PCF8575_MAX_PORTS] = {0xFFu, 0xFFu};
#endif

/* Semaphore to protect port array */
// static Semaphore_Handle DevicePcfPortLock;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

/* \brief Function to initialize THS7375 and THS7360. It also creates
          semaphore to protect pcf8575 port array */
Int32 Device_thsFiltersInit()
{
    Int32 retVal = 0;

    retVal = OSA_i2cOpen(&(gDevice_ThsCommonObj.i2cHandle), I2C_DEFAULT_INST_ID);

	OSA_i2cRawRead8(&gDevice_ThsCommonObj.i2cHandle,
                    DEVICE_PCF8575_I2C_ADDR,
                    DevicePcf8575Port,
                    DEVICE_PCF8575_MAX_PORTS);

    return (retVal);
}



/* \brief Function to deinitialize THS7375 and THS7360 */
Int32 Device_thsFiltersDeInit()
{

    OSA_i2cClose(&(gDevice_ThsCommonObj.i2cHandle));

    return (0);
}



/* \brief Function to set THS7375 parameters through PCF8575 I2C expander */
Int32 Device_ths7375SetParams(Device_ThsFilterCtrl ctrl)
{
    Int32 retVal;

    /* Clear out THS7375 */
    DevicePcf8575Port[DEVICE_PCF8575_PORT1] &= ~DEVICE_THS7375_MASK;

    DevicePcf8575Port[DEVICE_PCF8575_PORT1] |= (ctrl & DEVICE_THS7375_MASK);

    retVal = OSA_i2cRawWrite8(&gDevice_ThsCommonObj.i2cHandle,
                              DEVICE_PCF8575_I2C_ADDR,
                              DevicePcf8575Port,
                              DEVICE_PCF8575_MAX_PORTS);
    return (retVal);
}



/* \brief Function to set THS7360 SD parameters through PCF8575 I2C
          expander. This settings affects the Composite and
          SVideo outputs only. */
Int32 Device_ths7360SetSdParams(Device_ThsFilterCtrl ctrl)
{
    Int32 retVal = 0;
    UInt8 val;

    DevicePcf8575Port[DEVICE_PCF8575_PORT0] &= ~(DEVICE_THS7360_SD_MASK);
    switch (ctrl)
    {
        case DEVICE_THSFILTER_ENABLE_MODULE:
            val = ~(DEVICE_THS7360_SD_MASK);
            break;
        case DEVICE_THSFILTER_BYPASS_MODULE:
            val = DEVICE_PCF8575_PIN2;
            break;
        case DEVICE_THSFILTER_DISABLE_MODULE:
            val = DEVICE_THS7360_SD_MASK;
            break;
        default:
            retVal = -1;
    }

    if (retVal >= 0)
    {
        DevicePcf8575Port[DEVICE_PCF8575_PORT0] |= (val & DEVICE_THS7360_SD_MASK);
        retVal = OSA_i2cRawWrite8(&gDevice_ThsCommonObj.i2cHandle,
                                  DEVICE_PCF8575_I2C_ADDR,
                                  DevicePcf8575Port,
                                  DEVICE_PCF8575_MAX_PORTS);
    }

    return (retVal);
}



/* \brief Function to set THS7360 Sf (Selectable Filter) parameters
          through PCF8575 I2C expander. This settings affects
          component outputs only. This is used to select filter to
          the SD/ED/HD/True HD modes. */
Int32 Device_ths7360SetSfParams(Device_Ths7360SfCtrl ctrl)
{
    Int32 retVal = 0;
    UInt8 val;

    DevicePcf8575Port[DEVICE_PCF8575_PORT0] &= ~(DEVICE_THS7360_SF_MASK);
    switch(ctrl)
    {
        case DEVICE_THS7360_DISABLE_SF:
            val = DEVICE_PCF8575_PIN4;
            break;
        case DEVICE_THS7360_BYPASS_SF:
            val = DEVICE_PCF8575_PIN3;
            break;
        case DEVICE_THS7360_SF_SD_MODE:
            val = ~(DEVICE_THS7360_SF_MASK);
            break;
        case DEVICE_THS7360_SF_ED_MODE:
            val = DEVICE_PCF8575_PIN0;
            break;
        case DEVICE_THS7360_SF_HD_MODE:
            val = DEVICE_PCF8575_PIN1;
            break;
        case DEVICE_THS7360_SF_TRUE_HD_MODE:
            val = DEVICE_PCF8575_PIN0|DEVICE_PCF8575_PIN1;
            break;
        default:
            retVal = -1;
    }

    if (retVal >= 0)
    {
        DevicePcf8575Port[DEVICE_PCF8575_PORT0] |= (val & DEVICE_THS7360_SF_MASK);

        retVal = OSA_i2cRawWrite8(&gDevice_ThsCommonObj.i2cHandle,
                                  DEVICE_PCF8575_I2C_ADDR,
                                  DevicePcf8575Port,
                                  DEVICE_PCF8575_MAX_PORTS);
    }

    return (retVal);
}



/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
*  \file vpsdrv_hdmi9022a.h
*
*  \brief .
*
*/

#ifndef _DEVICE_SII9022A_H_
#define _DEVICE_SII9022A_H_


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

// #include <ti/psp/devices/vps_videoEncoder.h>
// #include <ti/psp/vps/drivers/fvid2_drvMgr.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* None */



/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

Int32 Device_sii9022aInit(void);
Int32 Device_sii9022aDeInit(void);

Device_Sii9022aHandle Device_sii9022aCreate (UInt32 drvId,
                                               UInt32 instanceId,
                                               Ptr createArgs,
                                               Ptr createStatusArgs);
                                               //,
                                               //const FVID2_DrvCbParams * fdmCbParams);

Int32 Device_sii9022aDelete(Device_Sii9022aHandle handle, Ptr deleteArgs);

Int32 Device_sii9022aControl (Device_Sii9022aHandle handle,
                              UInt32 cmd,
                              Ptr cmdArgs,
                              Ptr cmdStatusArgs);

#endif

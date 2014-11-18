/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _UTILS_DSP_H_
#define _UTILS_DSP_H_

#include <ti/xdais/xdas.h>
#include <ti/xdais/dm/xdm.h>
#include <mcfw/src_bios6/utils/utils_trace.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/sysbios/knl/Task.h>
/* =============================================================================
 * All success and failure codes for the module
 * ============================================================================= */
Int Utils_dspInit();

Int Utils_dspDeInit();

#endif //_UTILS_DSP_H_



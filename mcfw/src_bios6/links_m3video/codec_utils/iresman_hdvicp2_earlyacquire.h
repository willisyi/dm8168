/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IRESMAN_HDVICP2_EARLY_ACQUIRE_H_
#define _IRESMAN_HDVICP2_EARLY_ACQUIRE_H_

#include <ti/xdais/xdas.h>
#include <ti/xdais/ialg.h>
#include <ti/xdais/ires.h>

IRES_Status IRESMAN_TiledMemoryForceDisableTileAlloc_Register(IALG_Handle algHandle);
IRES_Status IRESMAN_TiledMemoryForceDisableTileAlloc_UnRegister(IALG_Handle algHandle);
Int32 Utils_encdec_checkResourceAvail(IALG_Handle alg, IRES_Fxns * resFxns,
                                      FVID2_Format *pFormat, UInt32 numFrames,
                                      IRES_ResourceDescriptor resDesc[]);

#endif                                                     /* _IRESMAN_HDVICP2_EARLY_ACQUIRE_H_
                                                            */

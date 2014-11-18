/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _IRESMAN_TILEDMEMORY_H_
#define _IRESMAN_TILEDMEMORY_H_

#include <ti/xdais/xdas.h>
#include <ti/xdais/ialg.h>
#include <ti/xdais/ires.h>

IRES_Status IRESMAN_TiledMemoryResourceRegister();
IRES_Status IRESMAN_TiledMemoryResourceUnregister();
IRES_Status IRESMAN_TILEDMEMORY_checkResourceAvail(IALG_Handle alg, UInt32 *size,
                                            IRES_Fxns * resFxns,
                                            IRES_ResourceDescriptor resDesc[]);

#endif                                                     /* _IRESMAN_TILEDMEMORY_H_ 
                                                            */

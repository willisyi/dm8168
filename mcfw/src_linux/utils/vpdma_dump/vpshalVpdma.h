/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 * \file vpshalVpdma.h
 *
 * \brief VPS VPDMA HAL default configuration file
 * This file contains default configuration for VPDMA.
 *
 */

#ifndef _VPSSHALVPDMA_H
#define _VPSSHALVPDMA_H

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#ifdef TI_816X_BUILD
#include "vpshalVpdmaDefaultsTI816x.h"
#endif

#ifdef TI_814X_BUILD
#include "vpshalVpdmaDefaultsTI814x.h"
#endif

#ifdef TI_8107_BUILD
#include "vpshalVpdmaDefaultsTI813x.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None*/

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _VPSHALVPDMA_H */

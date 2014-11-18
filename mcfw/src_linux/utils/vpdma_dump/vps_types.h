/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

 /*********************************************************************
* file: vps_types.h
*
*********************************************************************/

#ifndef _VPS_TYPES_H_
#define _VPS_TYPES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osa.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Platform - TI_816X_BUILD, TI_814X_BUILD, TI_8107_BUILD, TI_811X_BUILD*/
//#define TI_816X_BUILD

#define Vps_printf  printf


#define Task_sleep          OSA_waitMsecs
#define Clock_getTicks      OSA_getCurTimeInMsecs

/*
 * =========== Error codes returned by VPS functions =============
 */
#define VPS_SOK                         (0)
#define VPS_EFAIL                       (-1)
#define VPS_EBADARGS                    (-2)
#define VPS_EINVALID_PARAMS             (-3)
#define VPS_EDEVICE_INUSE               (-4)
#define VPS_ETIMEOUT                    (-5)
#define VPS_EALLOC                      (-6)
#define VPS_EOUT_OF_RANGE               (-7)
#define VPS_EAGAIN                      (-8)
#define VPS_EUNSUPPORTED_CMD            (-9)
#define VPS_ENO_MORE_BUFFERS            (-10)
#define VPS_EUNSUPPORTED_OPS            (-11)
#define VPS_EDRIVER_INUSE               (-12)

/**
 *  \brief Field type.
 */
typedef enum
{
    FVID2_FID_TOP = 0,
    /**< Top field. */
    FVID2_FID_BOTTOM,
    /**< Bottom field. */
    FVID2_FID_FRAME,
    /**< Frame mode - Contains both the fields or a progressive frame. */
    FVID2_FID_MAX
    /**< Should be the last value of this enumeration.
         Will be used by driver for validating the input parameters. */
} FVID2_Fid;

/** \brief Log enable for all HAL modules. */
#define VpsHalTrace                     (GT_INFO | GT_TraceState_Enable)
/** \brief Log enable for VPDMA HAL debug modules. */
#define VpsHalVpdmaDebugTrace           (GT_DEBUG | GT_TraceState_Enable)

#ifdef __cplusplus
}
#endif

#endif

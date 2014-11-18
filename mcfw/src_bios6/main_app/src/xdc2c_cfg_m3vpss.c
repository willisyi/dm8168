/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 *  \file xdc2c_cfg_m3vpss.c
 *  \brief Translates from XDC configuration values to plain C global variables
 *
 *  The values for these are set by xdc configuration script.
 *  To decouple dependence on XDC for the libraries like tiler allocator,
 *  this file translates from XDC configuration values to plain C global variables
*/
#include <xdc/std.h>
#include <xdc/cfg/global.h>

/* The following global variables are referenced by tiler allocator.
 */
const UInt32 UTILS_TILER_CNT_8BIT_SIZE  = XDC_CFG_TILER_MEM_8BIT_SIZE;
const UInt32 UTILS_TILER_CNT_16BIT_SIZE = XDC_CFG_TILER_MEM_16BIT_SIZE;
const UInt32 UTILS_TILER_CNT_32BIT_SIZE = XDC_CFG_TILER_MEM_32BIT_SIZE;
UInt8 *gUtils_tilerPhysMem = ((UInt8 *)XDC_CFG_TILER_MEM_BASE_ADDR);

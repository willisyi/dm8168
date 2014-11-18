/*
 *  Copyright 2008
 *  Texas Instruments Incorporated
 *
 *  All rights reserved.  Property of Texas Instruments Incorporated
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */

#ifndef _SCD_TI_
#define _SCD_TI_

#ifdef __cplusplus
extern "C" {
#endif

#include <xdc/std.h>
#include <ti/xdais/dm/ividanalytics.h>
#include <ti/xdais/ires.h>

///////////////////////////////////////////////////////////////////////////////
// We export global variables that represent the single instance (of the 
//	respective type) running on DSP
//
// SCD_TI is an object instance of type IVIDANALYTICS_Fxns, a xDAIS type reserved 
// for video analytics algorithms
//
// SCD_TI_IRES is an object instance of type IRES, a xDAIS interface used to describe 
// an algorithm's resource requirements. In this case, SCD_TI_IRES is the corresponding
// object used to specify SCD_TI's DMA resource interface
//
// These variable declarations were created in SCD_TI_iagl.c and SCD_TI_ires.c,
// respectively.
//
// DJM notes 2011-11-02

extern IVIDANALYTICS_Fxns	SCD_TI;
//extern IRES_Fxns SCD_TI_IRES;

#ifdef __cplusplus
}
#endif

#endif


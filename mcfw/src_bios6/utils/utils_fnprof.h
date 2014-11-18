/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file utils_fnprof.h
    \brief Function level profiling utility fuinction interface file
*/


#ifndef UTILS_FNPROF_H_
#define UTILS_FNPROF_H_
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <xdc/std.h>

Void FNPROF_STG1_enableProfile();
Void FNPROF_STG2_setFxnCallCntThreshold(UInt32 fxnCallCntThreshold);
Void FNPROF_STG2_enableProfile();
Void FNPROF_printProfileInfo();
Void FNPROF_disableProfiling();
Void FNPROF_hookOverheadCalibrateFxn();


#endif /* UTILS_FNPROF_H_ */

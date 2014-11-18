/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file utils_dmtimer.h
    \brief Dual mode timer interface functions
*/


#ifndef UTILS_DMTIMER_H_
#define UTILS_DMTIMER_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <xdc/runtime/Types.h>

Void   Utils_dmTimerInit();
Void   Utils_dmTimerDeInit();
Void   Utils_dmTimerGetFreq(UInt32 dmTimerId, Types_FreqHz *freq);
UInt32 Utils_dmTimerGetCurTimeInMsec(UInt32 dmTimerId);
UInt32 Utils_dmTimerGetCount(UInt32 dmTimerId);


#endif /* UTILS_DMTIMER_H_ */

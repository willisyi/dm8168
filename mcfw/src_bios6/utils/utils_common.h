/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _UTILS_COMMON_H_
#define _UTILS_COMMON_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <xdc/std.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

#include <ti/psp/vps/vps.h>
#include <mcfw/src_bios6/utils/utils_trace.h>

int Utils_remoteSendChar(char ch);
int Utils_getChar(char *pChar, UInt32 timeout);
int Utils_getString(char *pChar, UInt32 timeout);

#endif                                                     /* ifndef
                                                            * _UTILS_TRACE_H_
                                                            */

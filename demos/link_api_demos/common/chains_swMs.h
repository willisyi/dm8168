/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _CHAINS_SW_MS_H_
#define _CHAINS_SW_MS_H_


#include <demos/link_api_demos/common/chains.h>

#define CHAINS_SW_MS_MAX_DISPLAYS    (3)


Int32 Chains_swMsSwitchLayout(
            UInt32 swMsLinkId[CHAINS_SW_MS_MAX_DISPLAYS],
            SwMsLink_CreateParams swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS],
            Bool switchLayout,
            Bool switchCh,
            UInt32 numDisplay);

Void Chains_swMsGenerateLayoutParams(UInt32 devId, UInt32 layoutId, 
            SwMsLink_CreateParams * swMsLayoutParams);

#endif


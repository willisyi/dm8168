/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SD_Demo_SW_MS_H_
#define _SD_Demo_SW_MS_H_


#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>

#define SD_DEMO_SW_MS_MAX_DISPLAYS    (3)

Int32 SD_Demo_swMsSwitchLayout(
            UInt32 swMsLinkId[SD_DEMO_SW_MS_MAX_DISPLAYS],
            SwMsLink_CreateParams swMsPrm[SD_DEMO_SW_MS_MAX_DISPLAYS],
            Bool switchLayout,
            Bool switchCh,
            UInt32 numDisplay);

Void SD_Demo_swMsGenerateLayoutParams(UInt32 devId, UInt32 layoutId, 
            SwMsLink_CreateParams * swMsLayoutParams);

#endif


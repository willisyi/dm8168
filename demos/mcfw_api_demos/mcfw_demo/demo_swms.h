/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DEMO_SW_MS_H_
#define _DEMO_SW_MS_H_

#include <demos/mcfw_api_demos/mcfw_demo/demo.h>

#define DEMO_LAYOUT_MODE_4CH_4CH    0
#define DEMO_LAYOUT_MODE_4CH        1
#define DEMO_LAYOUT_MODE_1CH        2
#define DEMO_LAYOUT_MODE_9CH        3
#define DEMO_LAYOUT_MODE_6CH        4
#define DEMO_LAYOUT_MODE_16CH       5
#define DEMO_LAYOUT_MODE_7CH_1CH    6
#define DEMO_LAYOUT_MODE_2CH_PIP    7
#define DEMO_LAYOUT_MODE_20CH_4X5   8
#define DEMO_LAYOUT_MODE_25CH_5X5   9
#define DEMO_LAYOUT_MODE_30CH_5X6   10
#define DEMO_LAYOUT_MODE_36CH_6X6   11
#define DEMO_LAYOUT_MAX             12


Void Demo_swMsGenerateLayout(VDIS_DEV devId, UInt32 startChId, UInt32 maxChns, UInt32 layoutId,
     VDIS_MOSAIC_S * vdMosaicParam, Bool forceLowCostScaling, UInt32 demoType, UInt32 resolution);
Void Demo_swMs_PrintLayoutParams(VDIS_MOSAIC_S * vdMosaicParam);
Void Demo_swMsSetOutputFPS(VDIS_MOSAIC_S * vdMosaicParam, UInt32 outputFPS);
Int32 Demo_swMsGetOutputFPS(VDIS_MOSAIC_S * vdMosaicParam);
Int32 Demo_swMsGetOutSize(UInt32 outRes, UInt32 * width, UInt32 * height);


#endif


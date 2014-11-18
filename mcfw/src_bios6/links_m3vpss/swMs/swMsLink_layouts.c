/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "swMsLink_priv.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>



Int32 SwMsLink_updateLayoutParams(SwMsLink_LayoutPrm * layoutParams,  UInt32 outPitch)
{
    SwMsLink_LayoutWinInfo *winInfo;
    UInt16 winId, chId;

    for(chId=0;chId<SYSTEM_SW_MS_MAX_CH_ID;chId++)
    {
        layoutParams->ch2WinMap[chId] = SYSTEM_SW_MS_INVALID_ID;
    }

    for (winId = 0; winId < layoutParams->numWin; winId++)
    {
        winInfo = &(layoutParams->winInfo[winId]);

        /* Align window width and height  */
        winInfo->width = SystemUtils_align(winInfo->width,(SYSTEM_BUFFER_ALIGNMENT/2));
        winInfo->height = SystemUtils_align(winInfo->height,2);

        /* assuming YUV422I data */
        winInfo->bufAddrOffset = winInfo->startY * outPitch + winInfo->startX * 2;
        winInfo->bufAddrOffset = SystemUtils_align(winInfo->bufAddrOffset,SYSTEM_BUFFER_ALIGNMENT);

        if(winInfo->channelNum >= SYSTEM_SW_MS_MAX_CH_ID)
        {
            /* if channel is out of bounds, then remove the channel mapping to
                        this window
            */
            winInfo->channelNum = SYSTEM_SW_MS_INVALID_ID;
        }
        if(winInfo->channelNum != SYSTEM_SW_MS_INVALID_ID)
        {
            /* valid channel */

            if(layoutParams->ch2WinMap[winInfo->channelNum]==SYSTEM_SW_MS_INVALID_ID)
            {
                /* Channel not mapped to any window */
                layoutParams->ch2WinMap[winInfo->channelNum] = winId;
            }
            else
            {
                /* Channel is already mapped to a window, so this is a repeat channel
                    For repeated channels we will remove the channel mapping to
                    this window
                */
                winInfo->channelNum = SYSTEM_SW_MS_INVALID_ID;
            }
        }
    }

    return 0;
}



/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo_swMs.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>


#define SD_DEMO_MAX_LAYOUTS      (8)

Int32 SD_Demo_swMsGetOutSize(UInt32 outRes, UInt32 * width, UInt32 * height)
{
    switch (outRes)
    {
        case VSYS_STD_MAX:
            *width = 1920;
            *height = 1200;
            break;

        case VSYS_STD_720P_60:
            *width = 1280;
            *height = 720;
            break;
        case VSYS_STD_XGA_60:
            *width = 1024;
            *height = 768;
            break;
        default:
        case VSYS_STD_1080I_60:
        case VSYS_STD_1080P_60:
        case VSYS_STD_1080P_30:
            *width = 1920;
            *height = 1080;
            break;

        case VSYS_STD_NTSC:
            *width = 720;
            *height = 480;
            break;

        case VSYS_STD_PAL:
            *width = 720;
            *height = 576;
            break;

    }
    return 0;
}

Void SD_Demo_swMsGenerateLayoutParams(UInt32 devId, UInt32 layoutId,
                                     SwMsLink_CreateParams *swMsCreateArgs)
{
    SwMsLink_LayoutPrm *layoutInfo;
    SwMsLink_LayoutWinInfo *winInfo;
    UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
    UInt32 SwMsOutputFps;

    SD_Demo_swMsGetOutSize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

    widthAlign = 8;
    heightAlign = 1;

    if(devId>1)
        devId = 0;

    layoutInfo = &swMsCreateArgs->layoutPrm;
    /* store the SwMs output fps locally */
    SwMsOutputFps = layoutInfo->outputFPS;
    /* init to known default */
    memset(layoutInfo, 0, sizeof(*layoutInfo));

    layoutInfo->onlyCh2WinMapChanged = FALSE;
    /* restore the value OR
       Modify with new value if required */
    layoutInfo->outputFPS = SwMsOutputFps;

    if(layoutId > SD_DEMO_MAX_LAYOUTS)
        layoutId = 0;

    if(layoutId == 0)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_2X2_PLUS_4CH\n", devId);

        layoutInfo->numWin = 8;

        for(row=0; row<2; row++)
        {
            for(col=0; col<2; col++)
            {
                winId = row*2+col;

                winInfo = &layoutInfo->winInfo[winId];

                winInfo->width  = SystemUtils_align((outWidth*2)/5, widthAlign);
                winInfo->height = SystemUtils_align(outHeight/2, heightAlign);
                winInfo->startX = winInfo->width*col;
                winInfo->startY = winInfo->height*row;
                winInfo->bypass = TRUE;
                winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
            }
        }

        for(row=0; row<4; row++)
        {
            winId = 4 + row;

            winInfo = &layoutInfo->winInfo[winId];

            winInfo->width  = layoutInfo->winInfo[0].width/2;
            winInfo->height = layoutInfo->winInfo[0].height/2;
            winInfo->startX = layoutInfo->winInfo[0].width*2;
            winInfo->startY = winInfo->height*row;
            winInfo->bypass = TRUE;
            winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
        }
    }

    if(layoutId == 1)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_2X2\n", devId);

        layoutInfo->numWin    = 4;

        for(row=0; row<2; row++)
        {
            for(col=0; col<2; col++)
            {
                winId = row*2+col;

                winInfo = &layoutInfo->winInfo[winId];

                winInfo->width  = SystemUtils_align(outWidth/2, widthAlign);
                winInfo->height = SystemUtils_align(outHeight/2, heightAlign);
                winInfo->startX = winInfo->width*col;
                winInfo->startY = winInfo->height*row;
                winInfo->bypass = TRUE;
                winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
            }
        }
    }

    if(layoutId == 2)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_1x1\n", devId);

        layoutInfo->numWin    = 1;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->width  = outWidth;
        winInfo->height = outHeight;
        winInfo->bypass = TRUE;
        winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
    }

    if(layoutId == 3)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_1x1_PLUS_2PIP\n", devId);

        layoutInfo->numWin = 3;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->width  = outWidth;
        winInfo->height = outHeight;
        winInfo->bypass = TRUE;
        winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;

        for(col=0; col<2; col++)
        {
            winId = 1 + col;

            winInfo = &layoutInfo->winInfo[winId];

            winInfo->width  = SystemUtils_align(outWidth/4, widthAlign);
            winInfo->height = SystemUtils_align(outHeight/4, heightAlign);

            if(col==0)
            {
                winInfo->startX = SystemUtils_align(outWidth/20, widthAlign);
            }
            else
            {
                winInfo->startX = SystemUtils_align(
                    outWidth - winInfo->width - outWidth/20,
                    widthAlign);
            }

            winInfo->startY = SystemUtils_align(
                            outHeight - winInfo->height - outHeight/20,
                            heightAlign
                            );
            winInfo->bypass = TRUE;
            winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
        }
    }

    if(layoutId == 4)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_3X3\n", devId);

        layoutInfo->numWin    = 9;

        for(row=0; row<3; row++)
        {
            for(col=0; col<3; col++)
            {
                winId = row*3+col;

                winInfo = &layoutInfo->winInfo[winId];

                winInfo->width  = SystemUtils_align(outWidth/3, widthAlign);
                winInfo->height = SystemUtils_align(outHeight/3, heightAlign);
                winInfo->startX = winInfo->width*col;
                winInfo->startY = winInfo->height*row;
                winInfo->bypass = TRUE;
                winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
            }
        }

    }

    if(layoutId == 5)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_4X4\n", devId);

        layoutInfo->numWin    = 16;

        for(row=0; row<4; row++)
        {
            for(col=0; col<4; col++)
            {
                winId = row*4+col;

                winInfo = &layoutInfo->winInfo[winId];

                winInfo->width  = SystemUtils_align(outWidth/4, widthAlign);
                winInfo->height = SystemUtils_align(outHeight/4, heightAlign);
                winInfo->startX = winInfo->width*col;
                winInfo->startY = winInfo->height*row;
                winInfo->bypass = TRUE;
                winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
            }
        }
    }

    if(layoutId == 6)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_5CH_PLUS_1CH\n", devId);

        layoutInfo->numWin = 6;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->width  = SystemUtils_align((outWidth*2)/3, widthAlign);
        winInfo->height = SystemUtils_align((outHeight*2)/3, heightAlign);
        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->bypass = TRUE;
        winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;

        for(row=0; row<2; row++)
        {
            winId = 1 + row;

            winInfo = &layoutInfo->winInfo[winId];

            winInfo->width  = SystemUtils_align(layoutInfo->winInfo[0].width/2, widthAlign);
            winInfo->height = SystemUtils_align(layoutInfo->winInfo[0].height/2, heightAlign);
            winInfo->startX = layoutInfo->winInfo[0].width;
            winInfo->startY = winInfo->height*row;
            winInfo->bypass = TRUE;
            winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;

        }

        for(col=0; col<3; col++)
        {
            winId = 3 + col;

            winInfo = &layoutInfo->winInfo[winId];

            winInfo->width  = SystemUtils_align(layoutInfo->winInfo[0].width/2, widthAlign);
            winInfo->height = SystemUtils_align(layoutInfo->winInfo[0].height/2, heightAlign);
            winInfo->startX = winInfo->width*col;
            winInfo->startY = layoutInfo->winInfo[0].height;
            winInfo->bypass = TRUE;
            winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
        }

    }

    if(layoutId == 7)
    {
        printf("Display %d: Layout: SD_DEMO_LAYOUT_7CH_PLUS_1CH\n", devId);
        layoutInfo->numWin = 8;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->width  = SystemUtils_align((outWidth)/4, widthAlign)*3;
        winInfo->height = SystemUtils_align((outHeight)/4, heightAlign)*3;
        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->bypass = TRUE;
        winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;

        for(row=0; row<3; row++)
        {
            winId = 1 + row;

            winInfo = &layoutInfo->winInfo[winId];

            winInfo->width  = SystemUtils_align(
                                outWidth - layoutInfo->winInfo[0].width,
                                widthAlign);
            winInfo->height = SystemUtils_align(
                                layoutInfo->winInfo[0].height / 3,
                                heightAlign);
            winInfo->startX = layoutInfo->winInfo[0].width;
            winInfo->startY = winInfo->height*row;
            winInfo->bypass = TRUE;
            winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;

        }

        for(col=0; col<4; col++)
        {
            winId = 4 + col;

            winInfo = &layoutInfo->winInfo[winId];

            winInfo->width  = SystemUtils_align(outWidth / 4, widthAlign);
            winInfo->height = SystemUtils_align(
                                outHeight - layoutInfo->winInfo[0].height,
                                heightAlign);
            winInfo->startX = winInfo->width*col;
            winInfo->startY = layoutInfo->winInfo[0].height;
            winInfo->bypass = TRUE;
            winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
        }
    }
}

Int32 SD_Demo_swMsSwitchLayout(
            UInt32 swMsLinkId[SD_DEMO_SW_MS_MAX_DISPLAYS],
            SwMsLink_CreateParams swMsPrm[SD_DEMO_SW_MS_MAX_DISPLAYS],
            Bool switchLayout,
            Bool switchCh,
            UInt32 numDisplay)
{
    UInt32 i, winId;
    SwMsLink_LayoutPrm *layoutInfo;

    static UInt32 curLayoutId = 0;

    if(!switchLayout && !switchCh)
        return OSA_SOK;

    if(switchLayout == TRUE)
    {
        curLayoutId++;
        if(curLayoutId >= SD_DEMO_MAX_LAYOUTS)
            curLayoutId = 0;

        for(i=0; i<numDisplay; i++)
        {
           /* If switching mosaic create entire Mosaic Layout */
           SD_Demo_swMsGenerateLayoutParams(i, curLayoutId, &swMsPrm[i]);

           System_linkControl(swMsLinkId[i], SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, &swMsPrm[i].layoutPrm, sizeof(swMsPrm[i].layoutPrm), TRUE);
        }
    }
    if(switchCh == TRUE)
    {
        for(i=0; i<numDisplay; i++)
        {
            layoutInfo = &swMsPrm[i].layoutPrm;

            /* For switching channel get current mosaic Layout */
            System_linkControl(swMsLinkId[i], SYSTEM_SW_MS_LINK_CMD_GET_LAYOUT_PARAMS, layoutInfo, sizeof(*layoutInfo), TRUE);

            printf("Display %d: Channels: ", i);

            /* Populate the channel parameter for each window */
            for(winId=0; winId < layoutInfo->numWin; winId++)
            {
                layoutInfo->winInfo[winId].channelNum
                        =
                            (layoutInfo->winInfo[winId].channelNum+1)%SYSTEM_SW_MS_MAX_CH_ID;

                printf(" %d ",layoutInfo->winInfo[winId].channelNum);
            }

            printf("\n");

            layoutInfo->onlyCh2WinMapChanged = TRUE;

            /* Submit the same Layout with different window mapping */
            System_linkControl(swMsLinkId[i], SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, layoutInfo, sizeof(*layoutInfo), TRUE);
        }

    }

    return OSA_SOK;
}


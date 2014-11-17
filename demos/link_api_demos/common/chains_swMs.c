/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains_swMs.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>


#define CHAINS_MAX_LAYOUTS      (8)

Int32 Chains_swMsGetOutSize(UInt32 outRes, UInt32 * width, UInt32 * height)
{
    Vsys_getResSize(outRes, width, height);
    return 0;
}

Void Chains_swMsSetLayoutParams(UInt32 layoutId, SwMsLink_CreateParams *swMsCreateArgs)
{
    SwMsLink_LayoutPrm *layoutInfo;
    SwMsLink_LayoutWinInfo *winInfo;
    UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
    UInt32 SwMsOutputFps;
    UInt32 ch;

    Chains_swMsGetOutSize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

    widthAlign = 8;
    heightAlign = 1;

    layoutInfo = &swMsCreateArgs->layoutPrm;
    /* store the SwMs output fps locally */
    SwMsOutputFps = layoutInfo->outputFPS;
    /* init to known default */
    memset(layoutInfo, 0, sizeof(*layoutInfo));

    layoutInfo->onlyCh2WinMapChanged = FALSE;
    /* restore the value OR
       Modify with new value if required */
    layoutInfo->outputFPS = SwMsOutputFps;

    layoutId = layoutId % 6;

    if(layoutId == 0)
    {
        layoutInfo->numWin  = 1;

        winId               = 0;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->startX     = 0;
        winInfo->startY     = 0;
        winInfo->width      = outWidth;
        winInfo->height     = outHeight;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = 0;
    }

    if(layoutId == 1)
    {
        layoutInfo->numWin  = 1;

        winId               = 0;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->startX     = 0;
        winInfo->startY     = 0;
        winInfo->width      = outWidth;
        winInfo->height     = outHeight;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = 1;
    }

    if(layoutId == 2)
    {
        layoutInfo->numWin  = 2;

        winId               = 0;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = outWidth;
        winInfo->height     = outHeight;
        winInfo->startX     = 0;
        winInfo->startY     = 0;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = 0;

        winId               = 1;
        ch                  = 3;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = gChains_ctrl.channelConf[ch].width;
        winInfo->height     = gChains_ctrl.channelConf[ch].height;
        winInfo->startX     = outWidth-winInfo->width-64;
        winInfo->startY     = outHeight-winInfo->height-64;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = ch;
	}

    if(layoutId == 3)
    {
        layoutInfo->numWin  = 2;

        winId               = 0;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = outWidth;
        winInfo->height     = outHeight;
        winInfo->startX     = 0;
        winInfo->startY     = 0;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = 0;

        winId               = 1;
        ch                  = 2;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = gChains_ctrl.channelConf[ch].width;
        winInfo->height     = gChains_ctrl.channelConf[ch].height;
        winInfo->startX     = outWidth-winInfo->width-64;
        winInfo->startY     = outHeight-winInfo->height-64;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = ch;
	}

    if(layoutId == 4)
    {
        layoutInfo->numWin  = 2;

        winId               = 0;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = outWidth;
        winInfo->height     = outHeight;
        winInfo->startX     = 0;
        winInfo->startY     = 0;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = 1;

        winId               = 1;
        ch                  = 2;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = gChains_ctrl.channelConf[ch].width;
        winInfo->height     = gChains_ctrl.channelConf[ch].height;
        winInfo->startX     = outWidth-winInfo->width-64;
        winInfo->startY     = outHeight-winInfo->height-64;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = ch;
	}

    if(layoutId == 5)
    {
        layoutInfo->numWin  = 2;

        winId               = 0;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = outWidth;
        winInfo->height     = outHeight;
        winInfo->startX     = 0;
        winInfo->startY     = 0;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = 1;

        winId               = 1;
        ch                  = 3;
        winInfo             = &layoutInfo->winInfo[winId];
        winInfo->width      = gChains_ctrl.channelConf[ch].width;
        winInfo->height     = gChains_ctrl.channelConf[ch].height;
        winInfo->startX     = outWidth-winInfo->width-64;
        winInfo->startY     = outHeight-winInfo->height-64;
        winInfo->bypass     = TRUE;
        winInfo->channelNum = ch;
	}
}

Void Chains_swMsGenerateLayoutParams(UInt32 devId, UInt32 layoutId,
                                     SwMsLink_CreateParams *swMsCreateArgs)
{
    SwMsLink_LayoutPrm *layoutInfo;
    SwMsLink_LayoutWinInfo *winInfo;
    UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
    UInt32 SwMsOutputFps;
    UInt32 ch;

    Chains_swMsGetOutSize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

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

    if(layoutId >= CHAINS_MAX_LAYOUTS)
        layoutId = 0;

    if(layoutId == 0)
    {
        printf("Display %d: Layout: CHAINS_LAYOUT_2X2_PLUS_4CH\n", devId);

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
                winInfo->channelNum = winId;
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
            winInfo->channelNum = winId;
        }
    }

    if(layoutId == 1)
    {
        printf("Display %d: Layout: CHAINS_LAYOUT_2X2\n", devId);

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
                if(winId == 0)
                	winInfo->channelNum = 0;
                if(winId == 1)
                	winInfo->channelNum = 1;
                if(winId == 2)
                	winInfo->channelNum = 4;
                if(winId == 3)
                	winInfo->channelNum = 5;
            }
        }
    }

    if(layoutId == 2)
    {
        printf("Display %d: Layout: CHAINS_LAYOUT_1x1\n", devId);

        layoutInfo->numWin    = 1;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->width  = outWidth;
        winInfo->height = outHeight;
        winInfo->bypass = TRUE;
        if(devId == 0)
        {
        	winInfo->channelNum = 0;
        }

        if(devId == 1)
        {
        	winInfo->channelNum = 1;
        }

    }

    if(layoutId == 3)
    {
        printf("Display %d: Layout: CHAINS_LAYOUT_1x1_PLUS_2PIP\n", devId);

        layoutInfo->numWin = 3;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->width  = outWidth;
        winInfo->height = outHeight;
        winInfo->bypass = TRUE;
        winInfo->channelNum = winId;

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
            //winInfo->channelNum = devId*SYSTEM_SW_MS_MAX_WIN + winId;
            winInfo->channelNum =winId;
        }
    }

    if(layoutId == 4)
    {
        printf("Display %d: Layout: CHAINS_LAYOUT_3X3\n", devId);

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
                winInfo->channelNum = winId;
              }
        }

    }

    if(layoutId == 5)
    {
        printf("Display %d: Layout: CHAINS_LAYOUT_5CH_PLUS_1CH\n", devId);

        layoutInfo->numWin = 6;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->width  = SystemUtils_align((outWidth*2)/3, widthAlign);
        winInfo->height = SystemUtils_align((outHeight*2)/3, heightAlign);
        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->bypass = TRUE;
        winInfo->channelNum = 0;

        for(row=0; row<2; row++)
        {
            winId = 1 + row;

            winInfo = &layoutInfo->winInfo[winId];

            winInfo->width  = SystemUtils_align(layoutInfo->winInfo[0].width/2, widthAlign);
            winInfo->height = SystemUtils_align(layoutInfo->winInfo[0].height/2, heightAlign);
            winInfo->startX = layoutInfo->winInfo[0].width;
            winInfo->startY = winInfo->height*row;
            winInfo->bypass = TRUE;
            winInfo->channelNum = winId;

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
            winInfo->channelNum = winId;
        }

    }

    if(layoutId == 6)
    {
        printf("Display %d: Layout: CHAINS_LAYOUT_7CH_PLUS_1CH\n", devId);
        layoutInfo->numWin = 8;

        winId = 0;

        winInfo = &layoutInfo->winInfo[winId];

        winInfo->width  = SystemUtils_align((outWidth)/4, widthAlign)*3;
        winInfo->height = SystemUtils_align((outHeight)/4, heightAlign)*3;
        winInfo->startX = 0;
        winInfo->startY = 0;
        winInfo->bypass = TRUE;
        winInfo->channelNum = 0;

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
            winInfo->channelNum = winId;

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
            winInfo->channelNum = winId;
        }
    }

    if(layoutId == 7)
      {
          printf("Display %d: Layout: CHAINS_LAYOUT_1x1_PLUS_1PIP\n", devId);

          layoutInfo->numWin  = 2;

          winId               = 0;
          winInfo             = &layoutInfo->winInfo[winId];
          winInfo->width      = outWidth;
          winInfo->height     = outHeight;
          winInfo->startX     = 0;
          winInfo->startY     = 0;
          winInfo->bypass     = FALSE;
          winInfo->channelNum =  0;

          winId               = 1;
          winInfo             = &layoutInfo->winInfo[winId];
          winInfo->width      = SystemUtils_align(360, widthAlign);
          winInfo->height     = SystemUtils_align(202, heightAlign);
          winInfo->startX     = outWidth-winInfo->width - 60;
          winInfo->startY     = outHeight-winInfo->height - 40;
          winInfo->bypass     = FALSE;
          winInfo->channelNum = 3;
  	}

      if(layoutId == 8)
      {
          printf("Display %d: Layout: CHAINS_LAYOUT_1x1_PLUS_1PIPuuu\n", devId);

          layoutInfo->numWin  = 2;

          winId               = 0;
          winInfo             = &layoutInfo->winInfo[winId];
          winInfo->width      = outWidth;
          winInfo->height     = outHeight;
          winInfo->startX     = 0;
          winInfo->startY     = 0;
          winInfo->bypass     = FALSE;
          winInfo->channelNum =  1;

          winId               = 1;
          winInfo             = &layoutInfo->winInfo[winId];
          winInfo->width      = SystemUtils_align(360, widthAlign);
          winInfo->height     = SystemUtils_align(202, heightAlign);
          winInfo->startX     = outWidth-winInfo->width - 60;
          winInfo->startY     = outHeight-winInfo->height - 40;
          winInfo->bypass     = FALSE;
          winInfo->channelNum = 2;
  	}
}

Int32 Chains_swMsSwitchLayout(
            UInt32 swMsLinkId[CHAINS_SW_MS_MAX_DISPLAYS],
            SwMsLink_CreateParams swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS],
            Bool switchLayout,
            Bool switchCh,
            UInt32 numDisplay)
{
    UInt32 i, winId;
    SwMsLink_LayoutPrm *layoutInfo;
    SwMsLink_LayoutPrm *layoutInfo0;
    SwMsLink_LayoutPrm *layoutInfo1;

    static UInt32 curLayoutId = 0;

    if(!switchLayout && !switchCh)
        return OSA_SOK;

    if(switchLayout == TRUE)
    {
    	if(curLayoutId >= CHAINS_MAX_LAYOUTS)
    	    curLayoutId = 0;

    	   Chains_swMsGenerateLayoutParams(0,curLayoutId, &swMsPrm[0]);
    	   Chains_swMsGenerateLayoutParams(1,curLayoutId, &swMsPrm[1]);
    	   System_linkControl(swMsLinkId[0], SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, &swMsPrm[0].layoutPrm, sizeof(swMsPrm[0].layoutPrm), TRUE);
    	   System_linkControl(swMsLinkId[1], SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, &swMsPrm[1].layoutPrm, sizeof(swMsPrm[1].layoutPrm), TRUE);
    	   curLayoutId++;
    }

    if(switchCh == TRUE)
    {
    	if(0 == numDisplay  || 2 == numDisplay)
    	{
    			System_linkControl(swMsLinkId[0], SYSTEM_SW_MS_LINK_CMD_GET_LAYOUT_PARAMS, layoutInfo0, sizeof(*layoutInfo0), TRUE);
    			layoutInfo0 = &swMsPrm[0].layoutPrm;
    			if(layoutInfo0->numWin == 2)
    			{
    				if(layoutInfo0->winInfo[0].channelNum == 0 && layoutInfo0->winInfo[1].channelNum == 3)
    				{
    					layoutInfo0->winInfo[0].channelNum  = 1;
    					layoutInfo0->winInfo[1].channelNum  = 2;
    				}
    				else if(layoutInfo0->winInfo[0].channelNum == 1 && layoutInfo0->winInfo[1].channelNum == 2)
    				{
    					layoutInfo0->winInfo[0].channelNum  = 0;
    					layoutInfo0->winInfo[1].channelNum  = 3;
    				}

    				layoutInfo0->onlyCh2WinMapChanged = TRUE;
    				System_linkControl(swMsLinkId[0], SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, layoutInfo0, sizeof(*layoutInfo0), TRUE);
    			}

    	}
    	if(1 == numDisplay || 2 == numDisplay)
    	{
    		System_linkControl(swMsLinkId[1], SYSTEM_SW_MS_LINK_CMD_GET_LAYOUT_PARAMS, layoutInfo1, sizeof(*layoutInfo1), TRUE);
    		layoutInfo1 = &swMsPrm[1].layoutPrm;

    		if(layoutInfo1->numWin == 2)
    		{
    			if(layoutInfo1->winInfo[0].channelNum == 1 && layoutInfo1->winInfo[1].channelNum == 2)
    			{
    				layoutInfo1->winInfo[0].channelNum  = 0;
    				layoutInfo1->winInfo[1].channelNum  = 3;
    			}
    			else if(layoutInfo1->winInfo[0].channelNum == 0 && layoutInfo1->winInfo[1].channelNum == 3)
    			{
    				layoutInfo1->winInfo[0].channelNum  = 1;
    				layoutInfo1->winInfo[1].channelNum  = 2;
    			}

    			layoutInfo1->onlyCh2WinMapChanged = TRUE;
    			System_linkControl(swMsLinkId[1], SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, layoutInfo1, sizeof(*layoutInfo1), TRUE);
    		}

    	}

    }

    return OSA_SOK;
}

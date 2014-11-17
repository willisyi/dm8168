
#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>

CaptureLink_BlindInfo g_blindAreaChParam[CAPTURE_LINK_MAX_CH_PER_OUT_QUE];

Int32 SD_Demo_blindAreaInit(UInt32 numCh)
{
    int chId, winId;

    for(chId = 0; chId < numCh; chId++)
    {
        CaptureLink_BlindInfo * blindInfo;
        blindInfo = &g_blindAreaChParam[chId];

        if(chId < 8)
        {
          blindInfo->queId = 0;
          blindInfo->channelId = chId;
        }
        else
        {
          blindInfo->queId = 1;
          blindInfo->channelId = chId - 8;
        }
        blindInfo->numBlindArea = SD_DEMO_BLIND_AREA_NUM_WINDOWS;
        for(winId=0; winId < blindInfo->numBlindArea; winId++)
        {
            blindInfo->win[winId].enableWin = TRUE;
            blindInfo->win[winId].fillColorYUYV= 0x80108010;
            blindInfo->win[winId].startX = SD_DEMO_BLIND_AREA_WIN0_STARTX;
            blindInfo->win[winId].startY = SD_DEMO_BLIND_AREA_WIN0_STARTY + SD_DEMO_BLIND_AREA_WIN0_STARTY * winId +( SD_DEMO_BLIND_AREA_WIN_HEIGHT * winId);
            blindInfo->win[winId].width  = SD_DEMO_BLIND_AREA_WIN_WIDTH;
            blindInfo->win[winId].height = SD_DEMO_BLIND_AREA_WIN_HEIGHT;
        }

#if 0
        for(winId=0; winId < blindInfo->numBlindArea; winId++)
        {
        printf("chanId %d, startX %d startY %d \n",params.captureBlindInfo.channelId,
                                                   params.captureBlindInfo.win[winId].startX,
                                                   params.captureBlindInfo.win[winId].startY);
        }
#endif
//        Vcap_setDynamicParamChn(chId, &params, VCAP_BLINDAREACONFIG);

           System_linkControl(
                                        SYSTEM_LINK_ID_CAPTURE,
                                        CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA,
                                        blindInfo,
                                        sizeof(CaptureLink_BlindInfo),
                                        TRUE
                                        );

    }
    return 0;
}

Int32 SD_Demo_blindAreaUpdate(UInt32 numCh)
{

    UInt32 winId, startX,startY,enableWin,chId;
    CaptureLink_BlindInfo * blindInfo;

    printf("\n In the current demo UI option input is assume to be 704x240. User can change it if needed.\n");
    printf("\n User is required to give chanId, MaskWindowId and pixel coordinate. \n");
    printf("\n From the given pixel coordinate pixel windows of size 50x20 is masked/unMasked\n");


    chId = SD_Demo_getChId("CAPTURE", numCh);

    blindInfo = &g_blindAreaChParam[chId];

    winId = SD_Demo_getIntValue("Mask window number to be updated", 0, g_blindAreaChParam[chId].numBlindArea-1, 0);
    enableWin = SD_Demo_getIntValue("Mask window disable/enable", 0, 1, 0);

    blindInfo->win[winId].enableWin = enableWin;
    if(enableWin == TRUE)
    {
        blindInfo->win[winId].fillColorYUYV= 0x80108010;
        startX = SD_Demo_getIntValue("Mask window StartX", 0, 704, 0);;
        startY = SD_Demo_getIntValue("Mask window StartY", 0, 240, 0);;

        if(startX > 604)
           startX = 604;
        if(startY > 180)
           startY = 180;
        blindInfo->win[winId].startX = startX;
        blindInfo->win[winId].startY = startY;

        blindInfo->win[winId].width  = 50;
        blindInfo->win[winId].height = 20;
    }

     System_linkControl(
                        SYSTEM_LINK_ID_CAPTURE,
                        CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA,
                        blindInfo,
                        sizeof(CaptureLink_BlindInfo),
                        TRUE
                        );

   return 0;
}
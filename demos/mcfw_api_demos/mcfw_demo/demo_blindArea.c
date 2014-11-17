
#include <demo.h>

CaptureLink_BlindInfo g_blindAreaChParam[CAPTURE_LINK_MAX_CH_PER_OUT_QUE];

Int32 Demo_blindAreaInit(UInt32 numCh, UInt32 demoId)
{
    int chId, winId, numChanPerQueue = numCh;

    for(chId = 0; chId < numCh; chId++)
    {
        VCAP_CHN_DYNAMIC_PARAM_S params = { 0 };
        CaptureLink_BlindInfo * blindInfo;
        blindInfo = &g_blindAreaChParam[chId];

        numChanPerQueue = numCh;

       if((demoId == VSYS_USECASE_MULTICHN_HD_VCAP_VENC) || (demoId == VSYS_USECASE_MULTICHN_VCAP_VENC))
            numChanPerQueue = numCh/2;

        if(chId < numChanPerQueue)
        {
          blindInfo->queId     = 0;
          blindInfo->channelId = chId;
        }
        else
        {
          blindInfo->queId     = 1;
          blindInfo->channelId = chId - numChanPerQueue;
        }

        blindInfo->numBlindArea = DEMO_BLIND_AREA_NUM_WINDOWS;
        for(winId=0; winId < blindInfo->numBlindArea; winId++)
        {
            blindInfo->win[winId].enableWin = FALSE;
            blindInfo->win[winId].fillColorYUYV= 0x80108010;
            blindInfo->win[winId].startX = DEMO_BLIND_AREA_WIN0_STARTX;
            blindInfo->win[winId].startY = DEMO_BLIND_AREA_WIN0_STARTY + DEMO_BLIND_AREA_WIN0_STARTY * winId +( DEMO_BLIND_AREA_WIN_HEIGHT * winId);
            blindInfo->win[winId].width  = DEMO_BLIND_AREA_WIN_WIDTH;
            blindInfo->win[winId].height = DEMO_BLIND_AREA_WIN_HEIGHT;
        }
        memcpy(&params.captureBlindInfo,blindInfo,sizeof(CaptureLink_BlindInfo));
#if 0
        for(winId=0; winId < blindInfo->numBlindArea; winId++)
        {
            printf("ChanId %d, StartX %d StartY %d Width %d Height %d\n",
                                                   params.captureBlindInfo.channelId,
                                                   params.captureBlindInfo.win[winId].startX,
                                                   params.captureBlindInfo.win[winId].startY,
                                                   params.captureBlindInfo.win[winId].width,
                                                   params.captureBlindInfo.win[winId].height);
        }
#endif
        Vcap_setDynamicParamChn(chId, &params, VCAP_BLINDAREACONFIG);
    }
    return 0;
}

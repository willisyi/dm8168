/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>

/*
                      Capture (YUV422I) 16CH D1 60fps
                          |
                          |
                        NSF (YUV420SP)
                          |
                          |
                      SW Mosaic
                      (DEIH YUV422I)
                          |
                          |
                          |
                    Off-Chip HDMI
                      1080p60
*/


Void Chains_multiChCaptureNsfDei(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
    static SwMsLink_CreateParams       swMsPrm;
    DisplayLink_CreateParams    displayPrm;

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;

    UInt32 captureId, nsfId, swMsId, displayId;
    UInt32 vipInstId;

    Bool switchCh;
    Bool switchLayout;
    Bool enableNsf;

    char ch;

    captureId   = SYSTEM_LINK_ID_CAPTURE;
    nsfId       = SYSTEM_LINK_ID_NSF_0;

    swMsId      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    swMsPrm.numSwMsInst = 1;
    swMsPrm.swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEI_SC;

    displayId   = SYSTEM_LINK_ID_DISPLAY_1;

    CaptureLink_CreateParams_Init(&capturePrm);
    SwMsLink_CreateParams_Init(&swMsPrm);

    capturePrm.numVipInst = 4;

    enableNsf = chainsCfg->enableNsfLink;

    if(enableNsf)
        capturePrm.outQueParams[0].nextLink = nsfId;
    else
        capturePrm.outQueParams[0].nextLink = swMsId;

    capturePrm.tilerEnable = FALSE;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = (SYSTEM_CAPTURE_INST_VIP0_PORTA+vipInstId)%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_TVP5158_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = SYSTEM_STD_MUX_4CH_D1;
        pCaptureInstPrm->numOutput          = 1;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = 0;
        pCaptureOutPrm->scOutHeight         = 0;
        pCaptureOutPrm->outQueId            = 0;
    }

    nsfPrm.bypassNsf                = chainsCfg->bypassNsf;
    nsfPrm.tilerEnable              = FALSE;
    nsfPrm.inQueParams.prevLinkId   = captureId;
    nsfPrm.inQueParams.prevLinkQueId= 0;
    nsfPrm.numOutQue                = 1;
    nsfPrm.outQueParams[0].nextLink = swMsId;

    if(enableNsf)
        swMsPrm.inQueParams.prevLinkId = nsfId;
    else
        swMsPrm.inQueParams.prevLinkId = captureId;

    swMsPrm.inQueParams.prevLinkQueId = 0;
    swMsPrm.outQueParams.nextLink     = displayId;
    swMsPrm.maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
    swMsPrm.maxOutRes                 = chainsCfg->displayRes[SYSTEM_DC_VENC_HDMI];
    swMsPrm.lineSkipMode              = TRUE;
    swMsPrm.layoutPrm.outputFPS       = 60;


    Chains_swMsGenerateLayoutParams(0, 0, &swMsPrm);

    CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);
    displayPrm.inQueParams[0].prevLinkId    = swMsId;
    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.displayRes                = swMsPrm.maxOutRes;

    Chains_displayCtrlInit(chainsCfg->displayRes);

    System_linkCreate (captureId, &capturePrm, sizeof(capturePrm));
    System_linkControl(captureId, CAPTURE_LINK_CMD_CONFIGURE_VIP_DECODERS, NULL, 0, TRUE);

    if(enableNsf)
        System_linkCreate(nsfId     , &nsfPrm, sizeof(nsfPrm));

    System_linkCreate(swMsId   , &swMsPrm, sizeof(swMsPrm));
    System_linkCreate(displayId, &displayPrm, sizeof(displayPrm));

    Chains_memPrintHeapStatus();

    {
        System_linkStart(displayId);
        if(enableNsf)
            System_linkStart(nsfId    );
        System_linkStart(swMsId   );

        /* Start taking CPU load just before starting of links */
        Chains_prfLoadCalcEnable(TRUE, FALSE, FALSE);

        System_linkStart(captureId);

        while(1)
        {
            switchLayout = FALSE;
            switchCh     = FALSE;

            ch = Chains_menuRunTime();

            if(ch=='0')
                break;
            if(ch=='p')
                System_linkControl(captureId, CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
            if(ch=='s')
               switchLayout = TRUE;
            if(ch=='c')
               switchCh = TRUE;

            Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 1);
        }

        System_linkStop(captureId);
        if(enableNsf)
            System_linkStop(nsfId    );
        System_linkStop(swMsId    );
        System_linkStop(displayId);
    }

    System_linkDelete(captureId);
    if(enableNsf)
        System_linkDelete(nsfId    );
    System_linkDelete(swMsId   );
    System_linkDelete(displayId);

    Chains_displayCtrlDeInit();

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    Chains_prfLoadCalcEnable(FALSE, TRUE, FALSE);
}


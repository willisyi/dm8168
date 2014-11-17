/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <demos/link_api_demos/common/chains_ipcBuf.h>
#include <demos/link_api_demos/common/chains_ipcFrames.h>


Void Chains_singleChCapNsfSwMsDis(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams capturePrm;
    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;
    NsfLink_CreateParams     nsfPrm;
    SwMsLink_CreateParams    swMsPrm;
    DisplayLink_CreateParams displayPrm;

    UInt32 vipInstId;
    UInt32 displayId, captureId;
    UInt32 nsfId, swMsId;

    char ch;

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams, capturePrm);
    CHAINS_INIT_STRUCT(NsfLink_CreateParams, nsfPrm);
    CHAINS_INIT_STRUCT(SwMsLink_CreateParams, swMsPrm);
    CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);

    captureId = SYSTEM_LINK_ID_CAPTURE;
    nsfId     = SYSTEM_LINK_ID_NSF_0;
    swMsId    = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    displayId = SYSTEM_LINK_ID_DISPLAY_1;

    capturePrm.numVipInst               = 1;
    capturePrm.tilerEnable              = FALSE;
    capturePrm.numBufsPerCh             = CAPTURE_LINK_NUM_BUFS_PER_CH_DEFAULT;
    capturePrm.numExtraBufs             = 0;
    capturePrm.maxBlindAreasPerCh       = 0;
    capturePrm.isPalMode                = FALSE;
    capturePrm.enableSdCrop             = FALSE;
    capturePrm.doCropInCapture          = FALSE;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = (SYSTEM_CAPTURE_INST_VIP0_PORTA+vipInstId*2)%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_SII9233A_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = chainsCfg->displayRes[0];
        pCaptureInstPrm->numOutput          = 1;
        pCaptureInstPrm->numChPerOutput     = 1;
        pCaptureInstPrm->frameCaptureMode   = FALSE;
        pCaptureInstPrm->fieldsMerged       = FALSE;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = chainsCfg->channelConf[0].width;
        pCaptureOutPrm->scOutHeight         = chainsCfg->channelConf[0].height;
        pCaptureOutPrm->outQueId            = 0;
    }
    capturePrm.outQueParams[0].nextLink = nsfId;

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    chainsCfg->displayRes[0] = capturePrm.vipInst[0].standard;
    Vsys_getResSize(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].width, &chainsCfg->channelConf[0].height);
    Vsys_getResRate(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].frameRate);

    nsfPrm.inQueParams.prevLinkId    = captureId;
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.bypassNsf                 = FALSE;
    nsfPrm.tilerEnable               = TRUE;
    nsfPrm.numOutQue                 = 1;
    nsfPrm.numBufsPerCh              = 10;
    nsfPrm.inputFrameRate            = chainsCfg->channelConf[0].frameRate;
    nsfPrm.outputFrameRate           = chainsCfg->channelConf[0].frameRate;
    nsfPrm.outQueParams[0].nextLink  = swMsId;

    swMsPrm.inQueParams.prevLinkId    = nsfId;
    swMsPrm.outQueParams.nextLink     = displayId;
    swMsPrm.numSwMsInst               = 1;
    swMsPrm.swMsInstId[0]             = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm.inQueParams.prevLinkQueId = 0;
    swMsPrm.lineSkipMode              = FALSE;
    swMsPrm.enableLayoutGridDraw      = FALSE;
    swMsPrm.layoutPrm.outputFPS       = chainsCfg->channelConf[0].frameRate;
    swMsPrm.maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
    swMsPrm.numOutBuf                 = 0;
    swMsPrm.maxOutRes                 = chainsCfg->displayRes[0];

    Chains_swMsGenerateLayoutParams(0, 2, &swMsPrm);

    displayPrm.inQueParams[0].prevLinkId    = swMsId;
    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.displayRes                   = chainsCfg->displayRes[0];
    displayPrm.numInputQueues               = 1;
    displayPrm.activeQueue                  = 0;
    displayPrm.forceFieldSeparatedInputMode = FALSE;

    Chains_displayCtrlInit(chainsCfg->displayRes);

    System_linkCreate(nsfId , &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(swMsId, &swMsPrm, sizeof(swMsPrm));
    System_linkCreate(displayId, &displayPrm, sizeof(displayPrm));

    Chains_memPrintHeapStatus();

    {
        System_linkStart(displayId);
        System_linkStart(swMsId);
        System_linkStart(nsfId);
        System_linkStart(captureId);

        while(1)
        {
            ch = Chains_menuRunTime();
            if(ch=='0')
                break;
            if(ch=='v')
                System_linkControl(captureId, CAPTURE_LINK_CMD_FORCE_RESET, NULL, 0, TRUE);
            if(ch=='p') {
                System_linkControl(captureId, CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
                System_linkControl(displayId, DISPLAY_LINK_CMD_PRINT_STATISTICS, NULL, 0, TRUE);
            }
        }

        System_linkStop(captureId);
        System_linkStop(nsfId);
        System_linkStop(swMsId);
        System_linkStop(displayId);
    }

    System_linkDelete(captureId);
    System_linkDelete(nsfId);
    System_linkDelete(swMsId);
    System_linkDelete(displayId);

    Chains_displayCtrlDeInit();
}

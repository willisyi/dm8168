/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <demos/link_api_demos/common/chains_ipcBuf.h>
#include <demos/link_api_demos/common/chains_ipcFrames.h>


Void Chains_doubleChCapDis(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams  capturePrm;
    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;
    DisplayLink_CreateParams  displayPrm[CHAINS_SW_MS_MAX_DISPLAYS];

    UInt32 captureId, vipInstId;
    UInt32 displayId[CHAINS_SW_MS_MAX_DISPLAYS];

    Int32 i;
    char ch;

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams,capturePrm);
    for (i=0; i<CHAINS_SW_MS_MAX_DISPLAYS; i++)
        CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);

    captureId    = SYSTEM_LINK_ID_CAPTURE;
    displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI

    capturePrm.numVipInst               = 2;
    capturePrm.tilerEnable              = FALSE;
    capturePrm.numBufsPerCh             = 16;
    capturePrm.numExtraBufs             = 0;
    capturePrm.maxBlindAreasPerCh       = 0;
    capturePrm.isPalMode                = FALSE;
    capturePrm.enableSdCrop             = FALSE;
    capturePrm.doCropInCapture          = FALSE;

    for(i=0; i<capturePrm.numVipInst; i++)
    {
        vipInstId = SYSTEM_CAPTURE_INST_VIP0_PORTA + 2*i;
        pCaptureInstPrm                     = &capturePrm.vipInst[i];
        pCaptureInstPrm->vipInstId          = vipInstId%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_SII9233A_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = chainsCfg->displayRes[i];
        pCaptureInstPrm->numOutput          = 1;
        pCaptureInstPrm->numChPerOutput     = 1;
        pCaptureInstPrm->frameCaptureMode   = FALSE;
        pCaptureInstPrm->fieldsMerged       = FALSE;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = chainsCfg->channelConf[i].width;
        pCaptureOutPrm->scOutHeight         = chainsCfg->channelConf[i].height;
        pCaptureOutPrm->outQueId            = i;
    }
    capturePrm.outQueParams[0].nextLink     = displayId[0];
    capturePrm.outQueParams[1].nextLink     = displayId[1];

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    chainsCfg->displayRes[0] = capturePrm.vipInst[0].standard;
    chainsCfg->displayRes[1] = capturePrm.vipInst[1].standard;

    for (i=0; i<2; i++) {
        displayPrm[i].displayRes                   = chainsCfg->displayRes[i];
        displayPrm[i].numInputQueues               = 1;
        displayPrm[i].activeQueue                  = 0;
        displayPrm[i].inQueParams[0].prevLinkId    = captureId;
        displayPrm[i].inQueParams[0].prevLinkQueId = i;
        displayPrm[i].forceFieldSeparatedInputMode = FALSE;
    }

    Chains_displayCtrlInit(chainsCfg->displayRes);
    System_linkCreate(displayId[0], &displayPrm[0], sizeof(displayPrm[0]));
    System_linkCreate(displayId[1], &displayPrm[1], sizeof(displayPrm[1]));

    Chains_memPrintHeapStatus();

    {
        System_linkStart(displayId[0]);
        System_linkStart(displayId[1]);
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
            }
        }

        System_linkStop(captureId);
        System_linkStop(displayId[0]);
        System_linkStop(displayId[1]);
    }

    System_linkDelete(captureId);
    System_linkDelete(displayId[0]);
    System_linkDelete(displayId[1]);

    Chains_displayCtrlDeInit();
}

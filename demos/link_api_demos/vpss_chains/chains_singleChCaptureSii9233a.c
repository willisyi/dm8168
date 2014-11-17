/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <demos/link_api_demos/common/chains_ipcBuf.h>
#include <demos/link_api_demos/common/chains_ipcFrames.h>


Void Chains_singleChCaptureSii9233a(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams capturePrm;
    DisplayLink_CreateParams displayPrm;
    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssToHostPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInHostPrm;
    IpcFramesOutLinkHLOS_CreateParams  ipcFramesOutHostPrm;
    IpcFramesInLinkRTOS_CreateParams   ipcFramesInVpssFromHostPrm;
    System_LinkInfo framesProducerLinkInfo;

    UInt32 displayId, captureId;
    UInt32 vipInstId;
    Uint32 ipcFramesOutVpssToHost, ipcFramesInHost;
    Uint32 ipcFramesOutHost, ipcFramesInVpssFromHost;

    char ch;
    Bool enableVidFrameExport = FALSE;
    Bool enableGrpx = FALSE;

    if (enableVidFrameExport) {
        Chains_ipcFramesInit();
    }

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams,capturePrm);
    CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);
    CHAINS_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssToHostPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInHostPrm);
    CHAINS_INIT_STRUCT(IpcFramesOutLinkHLOS_CreateParams ,ipcFramesOutHostPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  ,ipcFramesInVpssFromHostPrm);

    captureId = SYSTEM_LINK_ID_CAPTURE;
    displayId = SYSTEM_LINK_ID_DISPLAY_1;

    ipcFramesOutVpssToHost  = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
    ipcFramesInHost         = SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0;
    ipcFramesOutHost        = SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0;
    ipcFramesInVpssFromHost = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_0;

    capturePrm.numVipInst               = 1;
    capturePrm.tilerEnable              = FALSE;
    capturePrm.numBufsPerCh             = 16;
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
        pCaptureInstPrm->standard           = chainsCfg->displayRes[vipInstId];
        pCaptureInstPrm->numOutput          = 1;
        pCaptureInstPrm->numChPerOutput     = 1;
        pCaptureInstPrm->frameCaptureMode   = FALSE;
        pCaptureInstPrm->fieldsMerged       = FALSE;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = chainsCfg->channelConf[vipInstId].width;
        pCaptureOutPrm->scOutHeight         = chainsCfg->channelConf[vipInstId].height;
        pCaptureOutPrm->outQueId            = 0;
    }

    if (enableVidFrameExport) {
        capturePrm.outQueParams[0].nextLink = ipcFramesOutVpssToHost;
    }
    else {
        capturePrm.outQueParams[0].nextLink = displayId;
    }

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    chainsCfg->displayRes[0] = capturePrm.vipInst[0].standard;
    Vsys_getResSize(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].width, &chainsCfg->channelConf[0].height);
    Vsys_getResRate(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].frameRate);

    if (enableVidFrameExport) {
        ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkId = captureId;

        ipcFramesOutVpssToHostPrm.baseCreateParams.noNotifyMode = FALSE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssToHostPrm.baseCreateParams.outQueParams[0].nextLink = ipcFramesInHost;
        ipcFramesOutVpssToHostPrm.baseCreateParams.inputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesOutVpssToHostPrm.baseCreateParams.outputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesOutVpssToHostPrm.baseCreateParams.notifyProcessLink = FALSE;

        ipcFramesInHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesInHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesInHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkId = ipcFramesOutVpssToHost;
        ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInHostPrm.baseCreateParams.outQueParams[0].nextLink = SYSTEM_LINK_ID_INVALID;
        ipcFramesInHostPrm.exportOnlyPhyAddr = FALSE;
        ipcFramesInHostPrm.baseCreateParams.inputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesInHostPrm.baseCreateParams.outputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesInHostPrm.baseCreateParams.notifyProcessLink = FALSE;
        Chains_ipcFramesInSetCbInfo(&ipcFramesInHostPrm);

        ipcFramesOutHostPrm.baseCreateParams.noNotifyMode   = TRUE;
        ipcFramesOutHostPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesOutHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutHostPrm.baseCreateParams.outQueParams[0].nextLink = ipcFramesInVpssFromHost;
        ipcFramesOutHostPrm.baseCreateParams.inputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesOutHostPrm.baseCreateParams.outputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesOutHostPrm.baseCreateParams.notifyProcessLink = FALSE;

        ipcFramesInVpssFromHostPrm.baseCreateParams.noNotifyMode = FALSE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkId = ipcFramesOutHost;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInVpssFromHostPrm.baseCreateParams.outQueParams[0].nextLink = displayId;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesInVpssFromHostPrm.baseCreateParams.outputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyProcessLink = FALSE;
    }

    if (enableVidFrameExport) {
        displayPrm.inQueParams[0].prevLinkId    = ipcFramesInVpssFromHost;
    }
    else {
        displayPrm.inQueParams[0].prevLinkId    = captureId;
    }

    displayPrm.displayRes                   = chainsCfg->displayRes[0];
    displayPrm.numInputQueues               = 1;
    displayPrm.activeQueue                  = 0;
    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.forceFieldSeparatedInputMode = FALSE;

    Chains_displayCtrlInit(chainsCfg->displayRes);

    if(enableVidFrameExport)
    {
        System_linkCreate(ipcFramesOutVpssToHost     , &ipcFramesOutVpssToHostPrm    , sizeof(ipcFramesOutVpssToHostPrm));
        System_linkCreate(ipcFramesInHost     , &ipcFramesInHostPrm    , sizeof(ipcFramesInHostPrm));
        System_linkGetInfo(ipcFramesInHost,&framesProducerLinkInfo);
        OSA_assert(framesProducerLinkInfo.numQue == 1);
        ipcFramesOutHostPrm.inQueInfo = framesProducerLinkInfo.queInfo[0];
        System_linkCreate(ipcFramesOutHost     , &ipcFramesOutHostPrm    , sizeof(ipcFramesOutHostPrm));
        System_linkCreate(ipcFramesInVpssFromHost     , &ipcFramesInVpssFromHostPrm    , sizeof(ipcFramesInVpssFromHostPrm));
    }

    System_linkCreate(displayId, &displayPrm, sizeof(displayPrm));

    Chains_memPrintHeapStatus();

    {
        if(enableGrpx) {
            grpx_init(GRPX_FORMAT_ARGB8888);
        }

        System_linkStart(displayId);
        if(enableVidFrameExport)
        {
            System_linkStart(ipcFramesInVpssFromHost);
            System_linkStart(ipcFramesOutHost);
            System_linkStart(ipcFramesInHost);
            System_linkStart(ipcFramesOutVpssToHost);
        }
        System_linkStart(captureId);

        if(enableGrpx)
        {
            grpx_fb_draw_demo();
        }

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
        if(enableVidFrameExport)
        {
            Chains_ipcFramesStop();
            System_linkStop(ipcFramesOutVpssToHost);
            System_linkStop(ipcFramesInHost);
            System_linkStop(ipcFramesOutHost);
            System_linkStop(ipcFramesInVpssFromHost);
        }
        System_linkStop(displayId);
        if(enableGrpx)
        {
            grpx_exit();
        }
    }

    System_linkDelete(captureId);
    if(enableVidFrameExport)
    {
        System_linkDelete(ipcFramesOutVpssToHost);
        System_linkDelete(ipcFramesInHost);
        System_linkDelete(ipcFramesOutHost);
        System_linkDelete(ipcFramesInVpssFromHost);
    }
    System_linkDelete(displayId);

    Chains_displayCtrlDeInit();
    if (enableVidFrameExport) {
        Chains_ipcFramesExit();
    }
}

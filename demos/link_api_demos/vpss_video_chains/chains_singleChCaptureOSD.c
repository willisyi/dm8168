/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <demos/link_api_demos/common/chains_ipcBuf.h>
#include <demos/link_api_demos/common/chains_ipcFrames.h>
#include <mcfw/interfaces/link_api/system_common.h>

Void Chains_singleChCaptureOSD(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams capturePrm;
    DisplayLink_CreateParams displayPrm;
    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInDspPrm;
    //HelloWorldLink_CreateParams  helloPrm;
   AlgLink_CreateParams        osdPrm;
    Ptr osdWinSrAddr[ALG_LINK_OSD_MAX_CH][CHAINS_OSD_NUM_WINDOWS];
    Ptr osdWinBuPtr[ALG_LINK_OSD_MAX_CH][CHAINS_OSD_NUM_WINDOWS];
    UInt32 displayId, captureId;
    UInt32 vipInstId;
    UInt32 ipcFramesOutVpssId, ipcFramesInDspId;
  //   UInt32 helloId;  

    char ch;
    Bool enableVidFrameExport = TRUE;
    Bool enableGrpx = FALSE;
    UInt32 osdId;
    Bool enableOsdAlgLink=TRUE;
	UInt32 i,j;
    if (enableVidFrameExport) {
        Chains_ipcFramesInit();
    }

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams,capturePrm);
    CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);
    CHAINS_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInDspPrm);
   // CHAINS_INIT_STRUCT(HelloWorldLink_CreateParams  , helloPrm);    
    CHAINS_INIT_STRUCT(AlgLink_CreateParams,osdPrm);
    captureId = SYSTEM_LINK_ID_CAPTURE;
    displayId = SYSTEM_LINK_ID_DISPLAY_1;

    ipcFramesOutVpssId  = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
    ipcFramesInDspId         = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
    //helloId         = SYSTEM_DSP_LINK_ID_HELLOWORLD_0;    
   osdId     = SYSTEM_LINK_ID_ALG_0;
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
        capturePrm.outQueParams[0].nextLink = ipcFramesOutVpssId;
    }
    else {
        capturePrm.outQueParams[0].nextLink = displayId;
    }

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    chainsCfg->displayRes[0] = capturePrm.vipInst[0].standard;
    Vsys_getResSize(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].width, &chainsCfg->channelConf[0].height);
    Vsys_getResRate(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].frameRate);

    if (enableVidFrameExport) {
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkId = captureId;
        ipcFramesOutVpssPrm.baseCreateParams.noNotifyMode = FALSE;
        ipcFramesOutVpssPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesOutVpssPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm.baseCreateParams.outQueParams[0].nextLink = displayId;
        ipcFramesOutVpssPrm.baseCreateParams.inputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesOutVpssPrm.baseCreateParams.outputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesOutVpssPrm.baseCreateParams.processLink = ipcFramesInDspId;
        ipcFramesOutVpssPrm.baseCreateParams.notifyProcessLink = FALSE;
        ipcFramesInDspPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesInDspPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkId = ipcFramesOutVpssId;
        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm.baseCreateParams.outQueParams[0].nextLink = osdId;
        ipcFramesInDspPrm.exportOnlyPhyAddr = FALSE;
        ipcFramesInDspPrm.baseCreateParams.inputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesInDspPrm.baseCreateParams.outputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesInDspPrm.baseCreateParams.notifyProcessLink = FALSE;
        Chains_ipcFramesInSetCbInfo(&ipcFramesInDspPrm);
/*
        helloPrm.inQueParams.prevLinkId=ipcFramesInDspId;
        helloPrm.inQueParams.prevLinkQueId = 0;
        helloPrm.outQueParams.nextLink=SYSTEM_LINK_ID_INVALID;
        helloPrm.createOutBuf1 = 1;
        helloPrm.numBufsPerCh=8;
*/
        osdPrm.inQueParams.prevLinkId=ipcFramesInDspId;
        osdPrm.inQueParams.prevLinkQueId = 0;
        osdPrm.enableOSDAlg=TRUE;
	 osdPrm.enableSCDAlg=FALSE;
    }
if(enableOsdAlgLink)
    {
        for(i = 0; i < ALG_LINK_OSD_MAX_CH; i++)
        {
            AlgLink_OsdChWinParams * chWinPrm = &osdPrm.osdChCreateParams[i].chDefaultParams;

	     /* set osd window max width and height */
            osdPrm.osdChCreateParams[i].maxWidth = CHAINS_OSD_WIN_MAX_WIDTH;
            osdPrm.osdChCreateParams[i].maxHeight = CHAINS_OSD_WIN_MAX_HEIGHT;
            chWinPrm->numWindows = CHAINS_OSD_NUM_WINDOWS;

            /* set osd window params. In this demo # of windows set to 2 */
            chWinPrm->winPrm[0].startX             = CHAINS_OSD_WIN0_STARTX;
            chWinPrm->winPrm[0].startY             = CHAINS_OSD_WIN0_STARTY;

/*            chWinPrm->winPrm[1].startX             = CHAINS_OSD_WIN1_STARTX;
            chWinPrm->winPrm[1].startY             = CHAINS_OSD_WIN1_STARTY;  */
            /* set osd window params */
		for(j=0; j < chWinPrm->numWindows; j++)
            {
                Int8 fileName[CHAINS_OSD_MAX_FILE_NAME_SIZE] = CHAINS_OSD_WIN0_FILE_NAME;
                Chains_createBuf(&chWinPrm->winPrm[j].addr[0][0],
                                 &osdWinBuPtr[i][j], &osdWinSrAddr[i][j], CHAINS_OSD_WIN_WIDTH *
                                 CHAINS_OSD_WIN_HEIGHT*2, SYSTEM_IPC_SR_NON_CACHED_DEFAULT);
		Chains_fillBuf(osdWinBuPtr[i][j],
                               fileName,
                               CHAINS_OSD_WIN_WIDTH * CHAINS_OSD_WIN_HEIGHT * 2);
		
                chWinPrm->winPrm[j].format             = SYSTEM_DF_YUV422I_YUYV;
                chWinPrm->winPrm[j].width              = CHAINS_OSD_WIN_WIDTH;
                chWinPrm->winPrm[j].height             = CHAINS_OSD_WIN_HEIGHT;
                chWinPrm->winPrm[j].lineOffset         = CHAINS_OSD_WIN_PITCH;
                chWinPrm->winPrm[j].globalAlpha        = CHAINS_OSD_GLOBAL_ALPHA;
                chWinPrm->winPrm[j].transperencyEnable = CHAINS_OSD_TRANSPARENCY;
                chWinPrm->winPrm[j].enableWin          = CHAINS_OSD_ENABLE_WIN;
            }
        }
    }
    if (enableVidFrameExport) {
        displayPrm.inQueParams[0].prevLinkId    = ipcFramesOutVpssId;
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
        System_linkCreate(ipcFramesOutVpssId     , &ipcFramesOutVpssPrm    , sizeof(ipcFramesOutVpssPrm));
        System_linkCreate(ipcFramesInDspId     , &ipcFramesInDspPrm    , sizeof(ipcFramesInDspPrm));
//System_linkCreate(helloId,&helloPrm,sizeof(helloPrm));
System_linkCreate(osdId,&osdPrm,sizeof(osdPrm));
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
        System_linkStart(osdId);
            //System_linkStart(helloId);
            System_linkStart(ipcFramesInDspId);
            System_linkStart(ipcFramesOutVpssId);
        }
        System_linkStart(captureId);

        if(enableGrpx)
        {
            grpx_fb_draw_demo();
        }

        sleep(100);

        System_linkStop(captureId);
        if(enableVidFrameExport)
        {
            Chains_ipcFramesStop();
            System_linkStop(ipcFramesOutVpssId);
            System_linkStop(ipcFramesInDspId);
//System_linkStop(helloId);
System_linkStop(osdId);
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
        System_linkDelete(ipcFramesOutVpssId);
        System_linkDelete(ipcFramesInDspId);
       // System_linkDelete(helloId);
        System_linkDelete(osdId);
	   for(i = 0; i < ALG_LINK_OSD_MAX_CH; i++)
        {
            AlgLink_OsdChWinParams * chWinPrm = &osdPrm.osdChCreateParams[i].chDefaultParams;

            chWinPrm->numWindows = CHAINS_OSD_NUM_WINDOWS;

            for(j=0; j < chWinPrm->numWindows; j++)
            {
                Chains_deleteBuf(osdWinBuPtr[i][j], CHAINS_OSD_WIN_WIDTH *
                                 CHAINS_OSD_WIN_HEIGHT*2, SYSTEM_IPC_SR_NON_CACHED_DEFAULT);
            }
        }
    }
    System_linkDelete(displayId);

    Chains_displayCtrlDeInit();
    if (enableVidFrameExport) {
        Chains_ipcFramesExit();
    }
}

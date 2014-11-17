/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <mcfw/interfaces/ti_venc.h>
#include <demos/link_api_demos/common/demo_text.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#define O_RDONLY    00000000  

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 10,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0 , 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 0,
        .EncChList = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
        .DecNumCh  = 12,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0 , 0, 0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 6,
        .EncChList = {10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .DecNumCh  = 4,
        .DecChList = {12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

Void Chains_singleChCapEncSend(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams    capturePrm;
    CaptureLink_VipInstParams   *pCaptureInstPrm;
    CaptureLink_OutParams       *pCaptureOutPrm;
    DupLink_CreateParams        dupPrm;
    DisplayLink_CreateParams    displayPrm;
    NsfLink_CreateParams        nsfPrm;
    EncLink_CreateParams        encPrm;
    IpcLink_CreateParams        ipcOutVpssPrm;
    IpcLink_CreateParams        ipcInVideoPrm;
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams    ipcBitsInHostPrm;
    //------------------------------Sue------------------------------------
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInDspPrm;
    //---------------------------------------------------------------------	
    UInt32 captureId;
    UInt32 vipInstId;
    UInt32 dupId;
    UInt32 displayId;
    UInt32 nsfId;
    UInt32 encId;
    UInt32 ipcOutVpssId;
    UInt32 ipcInVideoId;
    UInt32 ipcBitsOutVideoId, ipcBitsInHostId;

    UInt32 i = 0;
    //******dyx20131108******
    int count1=0;//是否需要键入开始消息
    int count2=0;//是否需要键入停止消息
    char kb_input[2];//用于接收键盘键入
     //------------------------------Sue------------------------------------
    UInt32 ipcFramesOutVpssId, ipcFramesInDspId;
   
    Bool enableOsdAlgLink=gChains_ctrl.channelConf[0].enableOsd;
	
      //---------------------------------------------------------------------

    UInt32  ch;
    AudioOn=AUDIO_ON;
    Chains_ipcBitsInit();
    //------------------------------Sue------------------------------------
    if (enableOsdAlgLink) 
    {
        Chains_ipcFramesInit();
    }
    //---------------------------------------------------------------------
    CHAINS_INIT_STRUCT(CaptureLink_CreateParams, capturePrm);
    CHAINS_INIT_STRUCT(DisplayLink_CreateParams, displayPrm);
    CHAINS_INIT_STRUCT(NsfLink_CreateParams, nsfPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams, encPrm);
    //------------------------------Sue------------------------------------
    CHAINS_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInDspPrm);
    CHAINS_INIT_STRUCT(AlgLink_CreateParams,DSPLinkPrm);
    //---------------------------------------------------------------------
    captureId = SYSTEM_LINK_ID_CAPTURE;
    dupId     = SYSTEM_VPSS_LINK_ID_DUP_0;
    //displayId = SYSTEM_LINK_ID_DISPLAY_1;
    displayId = SYSTEM_LINK_ID_DISPLAY_0;
    nsfId     = SYSTEM_LINK_ID_NSF_0;
    encId     = SYSTEM_LINK_ID_VENC_0;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;

    ipcBitsOutVideoId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInHostId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
   //------------------------------Sue------------------------------------
    ipcFramesOutVpssId  = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
    ipcFramesInDspId         = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
    osdId     = SYSTEM_LINK_ID_ALG_0;
   //---------------------------------------------------------------------
    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );
   

    capturePrm.numVipInst               = 1;
    capturePrm.tilerEnable              = FALSE;
    capturePrm.numBufsPerCh             = 16;//CAPTURE_LINK_NUM_BUFS_PER_CH_DEFAULT;
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
        if (Chains_IsInterlaced(chainsCfg->displayRes[0])) {
            pCaptureInstPrm->frameCaptureMode = TRUE;
            pCaptureInstPrm->fieldsMerged   = TRUE;
        }

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = chainsCfg->channelConf[vipInstId].width;
        pCaptureOutPrm->scOutHeight         = chainsCfg->channelConf[vipInstId].height;
        pCaptureOutPrm->outQueId            = 0;
    }
    //------------------------------Sue------------------------------------
    if (enableOsdAlgLink) 
    {
        capturePrm.outQueParams[0].nextLink=ipcFramesOutVpssId;
	  
    }
    else
    {
	 capturePrm.outQueParams[0].nextLink = dupId;
    }
    //---------------------------------------------------------------------
    //capturePrm.outQueParams[0].nextLink = dupId;

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    chainsCfg->displayRes[0] = capturePrm.vipInst[0].standard;
    Vsys_getResSize(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].width, &chainsCfg->channelConf[0].height);
    Vsys_getResRate(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].frameRate);
   //------------------------------Sue------------------------------------
     if (enableOsdAlgLink) {
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkId = captureId;
        ipcFramesOutVpssPrm.baseCreateParams.noNotifyMode = FALSE;
        ipcFramesOutVpssPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesOutVpssPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm.baseCreateParams.outQueParams[0].nextLink = dupId;
        ipcFramesOutVpssPrm.baseCreateParams.inputFrameRate = chainsCfg->channelConf[0].frameRate;
        ipcFramesOutVpssPrm.baseCreateParams.outputFrameRate =chainsCfg->channelConf[0].frameRate;// 
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

        DSPLinkPrm.inQueParams.prevLinkId=ipcFramesInDspId;
        DSPLinkPrm.inQueParams.prevLinkQueId = 0;
        DSPLinkPrm.enableOSDAlg=TRUE;
	 DSPLinkPrm.enableSCDAlg=FALSE;
	
    }
    if(enableOsdAlgLink)
    {
       DSPLinkPrm.osdChCreateParams[0].maxWidth = CHAINS_OSD_WIN_MAX_WIDTH;
       DSPLinkPrm.osdChCreateParams[0].maxHeight = CHAINS_OSD_WIN_MAX_HEIGHT;
       DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[0].startX=CHAINS_OSD_WIN0_STARTX;//
	DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[0].startY=CHAINS_OSD_WIN0_STARTY;//
       DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[1].startX=CHAINS_OSD_WIN0_STARTX;
       DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[1].startY=CHAINS_OSD_WIN0_STARTY+CHAINS_OSD_WIN_HEIGHT+CHAINS_OSD_WIN0_STARTY;
       DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[2].startX=CHAINS_OSD_WIN0_STARTX;
	DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[2].startY=CHAINS_OSD_WIN0_STARTY+CHAINS_OSD_WIN_HEIGHT+DEMO_OSD_WIN_HEIGHT+CHAINS_OSD_WIN0_STARTY;
       DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[3].startX=gChains_ctrl.channelConf[0].OSDStartX;
       DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[3].startY=gChains_ctrl.channelConf[0].OSDStartY;

	DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[0].enableWin=DEMO_OSD_ENABLE_WIN;
	DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[1].enableWin=DEMO_OSD_ENABLE_WIN;
	DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[2].enableWin=DEMO_OSD_ENABLE_WIN;//
	DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[3].enableWin=DEMO_OSD_ENABLE_WIN;
       strcpy(NameExample,"1080P60高清视频");
	OSDPrmCofig();
    }
    if (enableOsdAlgLink) 
    {
        dupPrm.inQueParams.prevLinkId    = ipcFramesOutVpssId;
    }
    else 
    {
        dupPrm.inQueParams.prevLinkId       = captureId;
    }
   //---------------------------------------------------------------------
    //dupPrm.inQueParams.prevLinkId       = captureId;
    dupPrm.inQueParams.prevLinkQueId    = 0;
    dupPrm.numOutQue                    = 2;
    dupPrm.outQueParams[0].nextLink     = nsfId;
    dupPrm.outQueParams[1].nextLink     = displayId;
    dupPrm.notifyNextLink               = TRUE;

    displayPrm.inQueParams[0].prevLinkId    = dupId;
    displayPrm.inQueParams[0].prevLinkQueId = 1;
    displayPrm.displayRes                   = chainsCfg->displayRes[0];

    nsfPrm.inQueParams.prevLinkId    = dupId;
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.bypassNsf                 = FALSE;
    nsfPrm.tilerEnable               = TRUE;
    nsfPrm.numOutQue                 = 1;
    nsfPrm.outQueParams[0].nextLink  = ipcOutVpssId;
    nsfPrm.inputFrameRate            = chainsCfg->channelConf[0].frameRate;
    nsfPrm.outputFrameRate           = chainsCfg->channelConf[0].frameRate;
    nsfPrm.numBufsPerCh              = 8;

    ipcOutVpssPrm.inQueParams.prevLinkId    = nsfId;
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.numChPerOutQue[0]         = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink  = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = TRUE;
    ipcOutVpssPrm.notifyPrevLink            = FALSE;
    ipcOutVpssPrm.noNotifyMode              = FALSE;
    ipcOutVpssPrm.inputFrameRate            = chainsCfg->channelConf[0].frameRate;
    ipcOutVpssPrm.outputFrameRate           = chainsCfg->channelConf[0].frameRate;
    ipcOutVpssPrm.equallyDivideChAcrossOutQues = FALSE;
    ipcOutVpssPrm.numChPerOutQue[0]         = 0;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.numChPerOutQue[0]         = 1;
    ipcInVideoPrm.outQueParams[0].nextLink  = encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = FALSE;
    ipcInVideoPrm.inputFrameRate            = chainsCfg->channelConf[0].frameRate;
    ipcInVideoPrm.outputFrameRate           = chainsCfg->channelConf[0].frameRate;
    ipcInVideoPrm.equallyDivideChAcrossOutQues = FALSE;
    ipcInVideoPrm.numChPerOutQue[0]         = 0;

    for (i=0; i<1; i++) {
        encPrm.chCreateParams[i].format                             = chainsCfg->channelConf[i].encFormat;
        encPrm.chCreateParams[i].profile                            = chainsCfg->channelConf[i].encProfile;
        encPrm.chCreateParams[i].dataLayout                         = IVIDEO_FIELD_SEPARATED;
        encPrm.chCreateParams[i].fieldMergeEncodeEnable             = FALSE;
        encPrm.chCreateParams[i].enableAnalyticinfo                 = FALSE;
        encPrm.chCreateParams[i].enableWaterMarking                 = FALSE;
        encPrm.chCreateParams[i].maxBitRate                         = -1;
        encPrm.chCreateParams[i].encodingPreset                     = XDM_USER_DEFINED;
        encPrm.chCreateParams[i].rateControlPreset                  = IVIDEO_USER_DEFINED;
        encPrm.chCreateParams[i].enableHighSpeed                    = FALSE;
        encPrm.chCreateParams[i].enableSVCExtensionFlag             = FALSE;
        encPrm.chCreateParams[i].numTemporalLayer                   = 0;
        encPrm.chCreateParams[i].defaultDynamicParams.intraFrameInterval = chainsCfg->channelConf[i].intraFrameInterval;
        encPrm.chCreateParams[i].defaultDynamicParams.interFrameInterval = 1;
        encPrm.chCreateParams[i].defaultDynamicParams.inputFrameRate     = chainsCfg->channelConf[i].frameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate    = chainsCfg->channelConf[i].encFrameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetBitRate      = chainsCfg->channelConf[i].bitRate;
        encPrm.chCreateParams[i].defaultDynamicParams.mvAccuracy         = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
        encPrm.chCreateParams[i].defaultDynamicParams.rcAlg              = chainsCfg->channelConf[i].rateCtrl;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMin              = 0;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMax              = 51;
        encPrm.chCreateParams[i].defaultDynamicParams.qpInit             = -1;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrDuration        = 8;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrSensitivity     = 0;
    }
    encPrm.inQueParams.prevLinkId   = ipcInVideoId;
    encPrm.inQueParams.prevLinkQueId= 0;
    encPrm.outQueParams.nextLink    = ipcBitsOutVideoId;
    encPrm.numBufPerCh[0]           = 8;

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId      = encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                   = 1;
    ipcBitsOutVideoPrm.baseCreateParams.numChPerOutQue[0]           = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink    = ipcBitsInHostId;
    ipcBitsOutVideoPrm.baseCreateParams.inputFrameRate              = chainsCfg->channelConf[0].encFrameRate;
    ipcBitsOutVideoPrm.baseCreateParams.outputFrameRate             = chainsCfg->channelConf[0].encFrameRate;
    Chains_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm,
                                               FALSE);

    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId        = ipcBitsOutVideoId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId     = 0;
    ipcBitsInHostPrm.baseCreateParams.numOutQue                     = 1;
    ipcBitsInHostPrm.baseCreateParams.numChPerOutQue[0]             = 1;
    ipcBitsInHostPrm.baseCreateParams.inputFrameRate                = chainsCfg->channelConf[0].encFrameRate;
    ipcBitsInHostPrm.baseCreateParams.outputFrameRate               = chainsCfg->channelConf[0].encFrameRate;
    Chains_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);

    Chains_displayCtrlInit(chainsCfg->displayRes);
   //------------------------------Sue------------------------------------
   
    if(enableOsdAlgLink)
    {
        System_linkCreate(ipcFramesOutVpssId     , &ipcFramesOutVpssPrm    , sizeof(ipcFramesOutVpssPrm));
        System_linkCreate(ipcFramesInDspId     , &ipcFramesInDspPrm    , sizeof(ipcFramesInDspPrm));
        System_linkCreate(osdId,&DSPLinkPrm,sizeof(DSPLinkPrm));
    }

    //---------------------------------------------------------------------
    System_linkCreate(dupId, &dupPrm, sizeof(dupPrm));
    System_linkCreate(displayId, &displayPrm, sizeof(displayPrm));
    System_linkCreate(nsfId , &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );
    System_linkCreate(encId, &encPrm, sizeof(encPrm));
    System_linkCreate(ipcBitsOutVideoId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(ipcBitsInHostId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));
    



    Chains_memPrintHeapStatus();

    {
        System_linkStart(ipcBitsInHostId);
        System_linkStart(ipcBitsOutVideoId);
        System_linkStart(encId);
        System_linkStart(ipcInVideoId);
        System_linkStart(ipcOutVpssId);
        System_linkStart(nsfId);
        System_linkStart(displayId);
        System_linkStart(dupId);
        //------------------------------Sue------------------------------------
        if(enableOsdAlgLink)
        {
            System_linkStart(osdId);
            System_linkStart(ipcFramesInDspId);
            System_linkStart(ipcFramesOutVpssId);
			
        }
        //---------------------------------------------------------------------
        System_linkStart(captureId);

//******dyx20131108******
        while(1)
        {
            ch = Chains_menuRunTime();
            if(ch=='0')
                break;
            if(ch=='p')
                System_linkControl(captureId, CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
	   //  if(ch=='b')
	   //  if(ch=='f')
	     if(ch=='a')
		 AudioOn=AUDIO_OFF;
	     if(ch=='s')
		 Chains_ipcBitsLocSt();
	     if(ch=='e')
		 Chains_ipcBitsLocStStop();
	     if(ch=='o')
	     	{
	     		DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[3].startX=200;
			DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[3].startY=200;
			//DSPLinkPrm.osdChCreateParams[0].chDefaultParams.winPrm[0].enableWin=0;
			gChains_ctrl.channelConf[0].OSDfont=32;
			//OSD_free();
		    strcpy(NameExample,"南京邮电大学NUPT");
			OSDPrmCofig();
		//	Osdsocket();
	     	}
	    
	     	

        } 


//此次添加代码在需要的时停止程序
/*
		while(1)
		{
		;
		}
		*/
//sleep(30);

AudioOn=AUDIO_OFF;
        System_linkStop(captureId);
        //------------------------------Sue------------------------------------
        if(enableOsdAlgLink)
        {
            Chains_ipcFramesStop();
            System_linkStop(ipcFramesOutVpssId);
            System_linkStop(ipcFramesInDspId);
            System_linkStop(osdId);
        }
        //---------------------------------------------------------------------

        System_linkStop(dupId);
        System_linkStop(displayId);
        System_linkStop(nsfId);
        System_linkStop(ipcOutVpssId);
        System_linkStop(ipcInVideoId);
        System_linkStop(encId);
        System_linkStop(ipcBitsOutVideoId);
        System_linkStop(ipcBitsInHostId);
        Chains_ipcBitsStop();
		/*
	if(gChains_ctrl.channelConf[0].audioEnable)
	{
	    Audio_captureStop ();
	}*/
    }
    System_linkDelete(captureId);
    //------------------------------Sue------------------------------------
	if(enableOsdAlgLink)
    {
        System_linkDelete(ipcFramesOutVpssId);
        System_linkDelete(ipcFramesInDspId);
        System_linkDelete(osdId);
	 OSD_free();
           
    }
    //---------------------------------------------------------------------
    System_linkDelete(dupId);
    System_linkDelete(displayId);
    System_linkDelete(nsfId);
    System_linkDelete(ipcOutVpssId);
    System_linkDelete(ipcInVideoId);
    System_linkDelete(encId);
    System_linkDelete(ipcBitsOutVideoId);
    System_linkDelete(ipcBitsInHostId);
    Chains_ipcBitsExit();
    //------------------------------Sue------------------------------------
    if (enableOsdAlgLink) 
    {
        Chains_ipcFramesExit();
    }
    //---------------------------------------------------------------------
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <mcfw/interfaces/ti_venc.h>
#include <mcfw/interfaces/ti_vsys.h>//for printf statistic
#include <mcfw/interfaces/link_api/system_common.h>
#include "chains_scd_bits_wr.h"		//for scd by guo
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
static UInt8 SCDChannelMonitor[16] = {0, 1, 2, 3};
void Chains_ScdPrint(UInt32 scdId)
{
	    AlgLink_ScdAllChFrameStatus scdAllChFrameStatus;
    		/**< SCD frame level status information for all CHs */
            int status = System_linkControl(
                                        scdId,
                                        ALG_LINK_SCD_CMD_GET_ALL_CHANNEL_FRAME_STATUS,
                                        &(scdAllChFrameStatus),
                                        sizeof(scdAllChFrameStatus),
                                        TRUE
                                        );
	int i;
	for(i=0;i<scdAllChFrameStatus.numCh;i++)
	{
	 	 printf("SCD status numCh:%d, chId %d , frmResult = %d\r\n",i,scdAllChFrameStatus.chanFrameResult[i].chId,
		 	scdAllChFrameStatus.chanFrameResult[i].frmResult);
	}
		
}

void Chains_OSDConfig(AlgLink_CreateParams *DSPLinkPrm)
{
   Ptr osdWinSrAddr[ALG_LINK_OSD_MAX_CH][CHAINS_OSD_NUM_WINDOWS];
    Ptr osdWinBuPtr[ALG_LINK_OSD_MAX_CH][CHAINS_OSD_NUM_WINDOWS];
	int i,j;
           for(i = 0; i < ALG_LINK_OSD_MAX_CH; i++)
        {
	 AlgLink_OsdChWinParams * chWinPrm = &DSPLinkPrm->osdChCreateParams[i].chDefaultParams;

	     /* set osd window max width and height */
            DSPLinkPrm->osdChCreateParams[i].maxWidth = CHAINS_OSD_WIN_MAX_WIDTH;
            DSPLinkPrm->osdChCreateParams[i].maxHeight = CHAINS_OSD_WIN_MAX_HEIGHT;
            //chWinPrm->numWindows = CHAINS_OSD_NUM_WINDOWS;
		chWinPrm->numWindows = 1;
            /* set osd window params. In this demo # of windows set to 2 */
            chWinPrm->winPrm[0].startX             = CHAINS_OSD_WIN0_STARTX;
            chWinPrm->winPrm[0].startY             = CHAINS_OSD_WIN0_STARTY;

/*            chWinPrm->winPrm[1].startX             = CHAINS_OSD_WIN1_STARTX;
            chWinPrm->winPrm[1].startY             = CHAINS_OSD_WIN1_STARTY;  */
            /* set osd window params */
		int j;
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

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 1,
        .EncChList = {0},
        .DecNumCh  = 0,
        .DecChList = {0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 1,
        .EncChList = {1},
        .DecNumCh  = 0,
        .DecChList = {0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 2,
        .EncChList = {2,3},
        .DecNumCh  = 0,
        .DecChList = {0},
    },
};

Void Chains_doubleChCapScEncSend(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams            capturePrm;
    CaptureLink_VipInstParams           *pCaptureInstPrm;
    CaptureLink_OutParams               *pCaptureOutPrm;
    MergeLink_CreateParams              mergePrm;   
    DupLink_CreateParams                dupPrm;
    NsfLink_CreateParams       		 nsfPrm;
    DupLink_CreateParams                dup2Prm;			//for two display
    SwMsLink_CreateParams               swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    DisplayLink_CreateParams            displayPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    IpcLink_CreateParams                ipcOutVpssPrm;
    IpcLink_CreateParams                ipcInVideoPrm;
    EncLink_CreateParams                encPrm;
    IpcBitsOutLinkRTOS_CreateParams     ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams      ipcBitsInHostPrm;

    UInt32 captureId, vipInstId;
    UInt32 mergeId;
    UInt32 dupId;
    UInt32 nsfId;
    UInt32 dup2Id;
    UInt32 swMsId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 displayId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 ipcOutVpssId;
    UInt32 ipcInVideoId;
    UInt32 encId;
    UInt32 ipcBitsOutVideoId;
    UInt32 ipcBitsInHostId;

    Bool enableOsdAlgLink=1;
    UInt32 enableDisplay = FALSE;
////////////////////////////OSD///////////////////////////////////
    AlgLink_CreateParams        	    DSPLinkPrm;//guo changed to global
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInDspPrm;
    UInt32 osdId;
    UInt32 ipcFramesOutVpssId, ipcFramesInDspId;	
    UInt8 osdFormat[ALG_LINK_OSD_MAX_CH];
    //enableOsdAlgLink=gChains_ctrl.channelConf[0].enableOsd;
    //gChains_ctrl.enableOsdAlgLink =TRUE;
	//	    gChains_ctrl.channelConf[0].enableOsd=TRUE;
    memset(osdFormat,  SYSTEM_DF_YUV422I_YUYV, ALG_LINK_OSD_MAX_CH);
    if (enableOsdAlgLink) 
    {
        Chains_ipcFramesInit();//This is likely no use.
    }
    CHAINS_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInDspPrm);
    CHAINS_INIT_STRUCT(AlgLink_CreateParams,DSPLinkPrm);
    ipcFramesOutVpssId  = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
    ipcFramesInDspId      = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
    osdId     = SYSTEM_LINK_ID_ALG_0;
//////////////////guo2014/11/21--added for scd////////////////////////
	//Scd CreateParams is contined in AlgLink_CreateParmas
	NsfLink_CreateParams       		 nsfPrm2;//SCD  only supports YUV420
	SclrLink_CreateParams           sclrPrm;
	AlgLink_CreateParams		scdPrm;
	IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm2;	//outVpss
	IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm2;		//Into Dsp
	IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm2;
	IpcBitsInLinkHLOS_CreateParams  ipcBitsInHostPrm2;

	UInt32 nsfId2;
	UInt32 scdId;
	UInt32 sclrId;
	UInt32 ipcFramesOutVpssId2;
	UInt32 ipcFramesInDspId2;
	UInt32 ipcBitsOutDspId2;
	UInt32 ipcBitsInHostId2;
	
	Bool enableScdAlgLink = 1;

	nsfId2 = SYSTEM_LINK_ID_NSF_1;
	scdId = SYSTEM_LINK_ID_ALG_1;
	sclrId = SYSTEM_LINK_ID_SCLR_INST_0;
	ipcFramesOutVpssId2 =  SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1; 
	ipcFramesInDspId2 = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_1;
	ipcBitsOutDspId2 = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
	ipcBitsInHostId2 = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
	CHAINS_INIT_STRUCT(NsfLink_CreateParams, nsfPrm2);
	CHAINS_INIT_STRUCT(SclrLink_CreateParams,sclrPrm);
	CHAINS_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm2);
   	CHAINS_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams,ipcFramesInDspPrm2);
	CHAINS_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutDspPrm2);
	CHAINS_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm2);
	CHAINS_INIT_STRUCT(AlgLink_CreateParams,scdPrm);


//	Scd_bitsWriteCreate(0,ipcBitsInHostId2);	//Init scd's ipcBitsInHost by:guo8113,this is ok, temporarily disabled!
	
//----------------end scd-----------------------------
    Chains_ipcBitsInit();		

    UInt32 i;
    char   ch;
   
    CHAINS_INIT_STRUCT(CaptureLink_CreateParams,capturePrm);
    for (i=0; i<CHAINS_SW_MS_MAX_DISPLAYS; i++) {
        CHAINS_INIT_STRUCT(SwMsLink_CreateParams,swMsPrm[i]);
        CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
    }
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(NsfLink_CreateParams, nsfPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams,encPrm);
    CHAINS_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);

    captureId     = SYSTEM_LINK_ID_CAPTURE;
    mergeId       = SYSTEM_VPSS_LINK_ID_MERGE_0;
    dupId         = SYSTEM_VPSS_LINK_ID_DUP_0;
    nsfId     = SYSTEM_LINK_ID_NSF_0;
    dup2Id        = SYSTEM_VPSS_LINK_ID_DUP_1;
    swMsId[0]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    swMsId[1]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    displayId[0]  = SYSTEM_LINK_ID_DISPLAY_0;
    displayId[1]  = SYSTEM_LINK_ID_DISPLAY_1;

    encId         = SYSTEM_LINK_ID_VENC_0;
    ipcOutVpssId  = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId  = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcBitsOutVideoId   = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInHostId     = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

    System_linkControl(       
		SYSTEM_LINK_ID_M3VPSS,
		SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES, 
		NULL,        0,        TRUE        );

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

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
        pCaptureInstPrm->numOutput          = 2;
        pCaptureInstPrm->numChPerOutput     = 1;
        pCaptureInstPrm->frameCaptureMode   = FALSE;
        pCaptureInstPrm->fieldsMerged       = FALSE;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;//nsf can change 422 to 420,because enc require 420
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = gChains_ctrl.channelConf[i].width;
        pCaptureOutPrm->scOutHeight         = gChains_ctrl.channelConf[i].height;
        pCaptureOutPrm->outQueId            = 0;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[1];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = gChains_ctrl.channelConf[2+i].width;
        pCaptureOutPrm->scOutHeight         = gChains_ctrl.channelConf[2+i].height;
        pCaptureOutPrm->outQueId            = 1;
    }
    capturePrm.outQueParams[0].nextLink     = mergeId;
    capturePrm.outQueParams[1].nextLink     = mergeId;

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    for (i=0; i<gChains_ctrl.channelNum; i++) {
        chainsCfg->displayRes[i] = capturePrm.vipInst[i].standard;
        Vsys_getResSize(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].width, &chainsCfg->channelConf[i].height);
        Vsys_getResRate(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].frameRate);
    }

    mergePrm.numInQue                       = 2;
    mergePrm.inQueParams[0].prevLinkId      = captureId;
    mergePrm.inQueParams[0].prevLinkQueId   = 0;
    mergePrm.inQueParams[1].prevLinkId      = captureId;
    mergePrm.inQueParams[1].prevLinkQueId   = 1;
//guo---------------------------------------------------------
     if (enableOsdAlgLink) 
    {
        mergePrm.outQueParams.nextLink=ipcFramesOutVpssId;  
	 ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkId = mergeId;
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
        Chains_ipcFramesInSetCbInfo(&ipcFramesInDspPrm);	//This is likely no use.

	 DSPLinkPrm.inQueParams.prevLinkId=ipcFramesInDspId;
        DSPLinkPrm.inQueParams.prevLinkQueId = 0;
        DSPLinkPrm.enableOSDAlg=TRUE;
	 DSPLinkPrm.enableSCDAlg=FALSE;
//#define LOGOONLY  //this is reserved for original logo only osd ,for case you will test the logo.
#ifdef LOGOONLY

        Chains_OSDConfig(DSPLinkPrm);//origin logo osd is OK
 #else

 	Demo_osdInit(4, osdFormat);
       for(i=0;i< gChains_ctrl.channelNum; i++)
       	{
		       DSPLinkPrm.osdChCreateParams[i].maxWidth = CHAINS_OSD_WIN_MAX_WIDTH;
       			DSPLinkPrm.osdChCreateParams[i].maxHeight = CHAINS_OSD_WIN_MAX_HEIGHT;
			memcpy(&(DSPLinkPrm.osdChCreateParams[i].chDefaultParams),
				&g_osdChParam[i],sizeof(AlgLink_OsdChWinParams));
       	}
 #endif
    }
    else
    {
	 mergePrm.outQueParams.nextLink = dupId;
    }
//------------------------------------------------------------
    mergePrm.notifyNextLink                 = TRUE;
    if (enableOsdAlgLink) 
    {
        dupPrm.inQueParams.prevLinkId    = ipcFramesOutVpssId;
    }
    else 
    {
        dupPrm.inQueParams.prevLinkId       = mergeId;
    }

    dupPrm.inQueParams.prevLinkQueId        = 0;
    dupPrm.numOutQue                        = 2;
//guo scd--------
    if(enableScdAlgLink)
    {
    	dupPrm.numOutQue	=	3;	//dup's third OutQue
	dupPrm.outQueParams[2].nextLink = sclrId;
	

    	sclrPrm.inQueParams.prevLinkId             = dupId;
    	sclrPrm.inQueParams.prevLinkQueId          = 2;
    	sclrPrm.outQueParams.nextLink              = nsfId2;

  	sclrPrm.tilerEnable                        = FALSE;
    	sclrPrm.enableLineSkipSc                   = TRUE;//FALSE;
    	sclrPrm.inputFrameRate                     = 60;
    	sclrPrm.outputFrameRate                    = 60;
    	sclrPrm.scaleMode                          = DEI_SCALE_MODE_ABSOLUTE;
	sclrPrm.outScaleFactor.absoluteResolution.outWidth = 352;
	sclrPrm.outScaleFactor.absoluteResolution.outHeight = 240;


   	nsfPrm2.inQueParams.prevLinkId    = sclrId;
    	nsfPrm2.inQueParams.prevLinkQueId = 0;
    	nsfPrm2.bypassNsf                 = TRUE;
    	nsfPrm2.tilerEnable               = FALSE;
    	nsfPrm2.numOutQue                 = 1;
    	nsfPrm2.outQueParams[0].nextLink  = ipcFramesOutVpssId2;
    	nsfPrm2.inputFrameRate            = 60;
    	nsfPrm2.outputFrameRate           = 60;
    	nsfPrm2.numBufsPerCh              = 8;

      	ipcFramesOutVpssPrm2.baseCreateParams.inQueParams.prevLinkId = nsfId2;
        ipcFramesOutVpssPrm2.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm2.baseCreateParams.notifyPrevLink = TRUE;

        ipcFramesOutVpssPrm2.baseCreateParams.numOutQue = 1;
        ipcFramesOutVpssPrm2.baseCreateParams.outQueParams[0].nextLink = ipcFramesInDspId2;
        ipcFramesOutVpssPrm2.baseCreateParams.notifyNextLink = TRUE;
	ipcFramesOutVpssPrm2.baseCreateParams.inputFrameRate = 60;
	ipcFramesOutVpssPrm2.baseCreateParams.outputFrameRate = 2;


        ipcFramesInDspPrm2.baseCreateParams.inQueParams.prevLinkId = ipcFramesOutVpssId2;
        ipcFramesInDspPrm2.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm2.baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm2.baseCreateParams.outQueParams[0].nextLink = scdId;
        ipcFramesInDspPrm2.baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesInDspPrm2.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm2.baseCreateParams.noNotifyMode   = FALSE;


	Int32    numBlksInFrame;
       Int32    numHorzBlks, numVertBlks, chIdx;
       Uint32  x, y, i;
	scdPrm.inQueParams.prevLinkId = ipcFramesInDspId2;
	scdPrm.inQueParams.prevLinkQueId =0;
        scdPrm.enableSCDAlg              = TRUE;
	scdPrm.enableOSDAlg	= FALSE;
        scdPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = ipcBitsOutDspId2;
        scdPrm.scdCreateParams.numBufPerCh            = 3;

        scdPrm.scdCreateParams.maxWidth               = 352;
           scdPrm.scdCreateParams.maxHeight           = 240;//288

        scdPrm.scdCreateParams.maxStride              = 352;
        scdPrm.scdCreateParams.numValidChForSCD       = 4;	//16

        scdPrm.scdCreateParams.numSecs2WaitB4Init     = 3;
        scdPrm.scdCreateParams.numSecs2WaitB4FrmAlert = 1;
        scdPrm.scdCreateParams.inputFrameRate         = 2;
        scdPrm.scdCreateParams.outputFrameRate        = 2;

        scdPrm.scdCreateParams.numSecs2WaitAfterFrmAlert    = 2;
        scdPrm.scdCreateParams.numSecs2Wait2MarkStableFrame = 5;

        scdPrm.scdCreateParams.enableMotionNotify        = FALSE;
        scdPrm.scdCreateParams.enableTamperNotify        = FALSE;

        /* Should be applied on CIF channels whose ch numbers are 4~7 */
        //scdPrm.scdCreateParams.startChNoForSCD = 16;
        // Configure array to monitor scene changes in all frame blocks, i.e., motion detection.
        // Each block is fixed to be 32x10 in size,
        numHorzBlks     = scdPrm.scdCreateParams.maxWidth / 32;
        if(scdPrm.scdCreateParams.maxHeight == 240)
           numVertBlks    = scdPrm.scdCreateParams.maxHeight / 10;
        else   /* For 288 Block height becomes 12 */
           numVertBlks    = scdPrm.scdCreateParams.maxHeight / 12;

        numBlksInFrame  = numHorzBlks * numVertBlks;

        for(chIdx = 0; chIdx < scdPrm.scdCreateParams.numValidChForSCD; chIdx++)
        {
           AlgLink_ScdChParams * chPrm = &scdPrm.scdCreateParams.chDefaultParams[chIdx];

           chPrm->blkNumBlksInFrame  = numBlksInFrame;
           chPrm->chId               = SCDChannelMonitor[chIdx];
           chPrm->mode               = ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
           chPrm->frmIgnoreLightsON  = FALSE;
           chPrm->frmIgnoreLightsOFF = FALSE;
           chPrm->frmSensitivity     = ALG_LINK_SCD_SENSITIVITY_HIGH;//ALG_LINK_SCD_SENSITIVITY_VERYHIGH;
           chPrm->frmEdgeThreshold   = 100;

           i = 0;
           for(y = 0; y < numVertBlks; y++)
           {
             for(x = 0; x < numHorzBlks; x++)
             {
               chPrm->blkConfig[i].sensitivity = ALG_LINK_SCD_SENSITIVITY_LOW;
               chPrm->blkConfig[i].monitored     = 0;
               i++;
             }
           }
        }

        ipcBitsOutDspPrm2.baseCreateParams.inQueParams.prevLinkId    = scdId;
        ipcBitsOutDspPrm2.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcBitsOutDspPrm2.baseCreateParams.numOutQue                 = 1;
	 ipcBitsOutDspPrm2.baseCreateParams.noNotifyMode	= FALSE;
	  ipcBitsOutDspPrm2.baseCreateParams.notifyNextLink              = TRUE;
        ipcBitsOutDspPrm2.baseCreateParams.outQueParams[0].nextLink  =  ipcBitsInHostId2;
        
        ipcBitsOutDspPrm2.baseCreateParams.notifyNextLink              = TRUE;
        ipcBitsOutDspPrm2.baseCreateParams.noNotifyMode                = FALSE;

        ipcBitsInHostPrm2.baseCreateParams.inQueParams.prevLinkId = ipcBitsOutDspId2;
        ipcBitsInHostPrm2.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcBitsInHostPrm2.baseCreateParams.numOutQue                 = 1;
        ipcBitsInHostPrm2.baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;
  
	Chains_ipcBitsInitCreateParams_BitsInHLOSVcap(&ipcBitsInHostPrm2);
        ipcBitsInHostPrm2.baseCreateParams.notifyPrevLink         = TRUE;
        ipcBitsInHostPrm2.baseCreateParams.noNotifyMode              = FALSE;		
    }
    else
    {
        scdPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;//nullId;
    }
	  
    dupPrm.outQueParams[0].nextLink = nsfId;
    dupPrm.outQueParams[1].nextLink = dup2Id;//to display
    dupPrm.notifyNextLink                    = TRUE;


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
    ipcOutVpssPrm.numChPerOutQue[0]         = 0;
    ipcOutVpssPrm.equallyDivideChAcrossOutQues = FALSE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.numChPerOutQue[0]         = 1;
    ipcInVideoPrm.outQueParams[0].nextLink  = encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = FALSE;
    ipcInVideoPrm.numChPerOutQue[0]         = 0;
    ipcInVideoPrm.equallyDivideChAcrossOutQues = FALSE;

    for (i=0; i<gChains_ctrl.channelNum; i++) {
        encPrm.chCreateParams[i].format                             = chainsCfg->channelConf[i].encFormat;
        encPrm.chCreateParams[i].profile                            = chainsCfg->channelConf[i].encProfile;
        encPrm.chCreateParams[i].dataLayout                         = IVIDEO_FIELD_SEPARATED;
        encPrm.chCreateParams[i].fieldMergeEncodeEnable             = FALSE;
        encPrm.chCreateParams[i].enableAnalyticinfo                 = FALSE;
        encPrm.chCreateParams[i].enableWaterMarking                 = FALSE;
        encPrm.chCreateParams[i].encodingPreset                     = XDM_USER_DEFINED;
        encPrm.chCreateParams[i].rateControlPreset                  = IVIDEO_USER_DEFINED;
        encPrm.chCreateParams[i].enableHighSpeed                    = FALSE;
        encPrm.chCreateParams[i].defaultDynamicParams.interFrameInterval  = 1;
        encPrm.chCreateParams[i].defaultDynamicParams.intraFrameInterval  = chainsCfg->channelConf[i].intraFrameInterval;
        encPrm.chCreateParams[i].defaultDynamicParams.inputFrameRate      = chainsCfg->channelConf[i].frameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate     = chainsCfg->channelConf[i].encFrameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetBitRate       = chainsCfg->channelConf[i].bitRate;
        encPrm.chCreateParams[i].defaultDynamicParams.mvAccuracy          = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
        encPrm.chCreateParams[i].defaultDynamicParams.rcAlg               = chainsCfg->channelConf[i].rateCtrl;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMin               = 0;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMax               = 51;
        encPrm.chCreateParams[i].defaultDynamicParams.qpInit              = -1;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrDuration         = 8;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrSensitivity      = 0;
    }
    encPrm.inQueParams.prevLinkId   = ipcInVideoId;
    encPrm.inQueParams.prevLinkQueId= 0;
    encPrm.outQueParams.nextLink    = ipcBitsOutVideoId;
    encPrm.numBufPerCh[0]           = 0;

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId      = encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                   = 1;
    ipcBitsOutVideoPrm.baseCreateParams.numChPerOutQue[0]           = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink    = ipcBitsInHostId;
    ipcBitsOutVideoPrm.baseCreateParams.notifyNextLink              = FALSE;
    ipcBitsOutVideoPrm.baseCreateParams.notifyPrevLink              = FALSE;
    ipcBitsOutVideoPrm.baseCreateParams.noNotifyMode                = TRUE;
    Chains_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm,FALSE);

    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId        = ipcBitsOutVideoId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId     = 0;
    ipcBitsInHostPrm.baseCreateParams.numOutQue                     = 1;
    ipcBitsInHostPrm.baseCreateParams.numChPerOutQue[0]             = 1;
    ipcBitsInHostPrm.baseCreateParams.notifyNextLink                = FALSE;
    ipcBitsInHostPrm.baseCreateParams.notifyPrevLink                = FALSE;
    ipcBitsInHostPrm.baseCreateParams.noNotifyMode                  = TRUE;
    Chains_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);

    Chains_displayCtrlInit(chainsCfg->displayRes);
    System_linkCreate(mergeId, &mergePrm, sizeof(mergePrm));
   

    dup2Prm.inQueParams.prevLinkId           = dupId;
    dup2Prm.inQueParams.prevLinkQueId        = 1;
    dup2Prm.numOutQue                        = 2;
    dup2Prm.outQueParams[0].nextLink         = swMsId[0];
    dup2Prm.outQueParams[1].nextLink         = swMsId[1];

    dup2Prm.notifyNextLink                   = TRUE;

    for (i=0; i<2; i++) {
		
        swMsPrm[i].numSwMsInst               = 1;
        swMsPrm[i].swMsInstId[0]             = SYSTEM_SW_MS_SC_INST_SC5;
        swMsPrm[i].inQueParams.prevLinkId    = dup2Id;
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = displayId[i];
        swMsPrm[i].lineSkipMode              = FALSE;
        swMsPrm[i].enableLayoutGridDraw      = FALSE;
        swMsPrm[i].layoutPrm.outputFPS       = chainsCfg->channelConf[i].frameRate;
        swMsPrm[i].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes                 = chainsCfg->displayRes[i];
        swMsPrm[i].numOutBuf                 = 0;
        Chains_swMsSetLayoutParams(i, &swMsPrm[i]);

        displayPrm[i].numInputQueues                = 1;
        displayPrm[i].activeQueue                   = 0;
       displayPrm[i].inQueParams[0].prevLinkId     = swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId  = 0;
        displayPrm[i].displayRes                    = chainsCfg->displayRes[i];
        displayPrm[i].forceFieldSeparatedInputMode  = FALSE;
    }

 	if(enableOsdAlgLink)
    {
        System_linkCreate(ipcFramesOutVpssId     , &ipcFramesOutVpssPrm    , sizeof(ipcFramesOutVpssPrm));
        System_linkCreate(ipcFramesInDspId     , &ipcFramesInDspPrm    , sizeof(ipcFramesInDspPrm));
        System_linkCreate(osdId,&DSPLinkPrm,sizeof(DSPLinkPrm));
    }


    System_linkCreate(dupId, &dupPrm, sizeof(dupPrm));
    System_linkCreate(nsfId , &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(ipcOutVpssId, &ipcOutVpssPrm, sizeof(ipcOutVpssPrm));
    System_linkCreate(ipcInVideoId, &ipcInVideoPrm, sizeof(ipcInVideoPrm));
    System_linkCreate(encId, &encPrm, sizeof(encPrm));
    System_linkCreate(ipcBitsOutVideoId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(ipcBitsInHostId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));

		//guo
	if(enableScdAlgLink)
	{
		System_linkCreate(sclrId, &sclrPrm    , sizeof(sclrPrm ));
		System_linkCreate(nsfId2,&nsfPrm2,sizeof(nsfPrm2));
		System_linkCreate(ipcFramesOutVpssId2     , &ipcFramesOutVpssPrm2    , sizeof(ipcFramesOutVpssPrm2));
		System_linkCreate(ipcFramesInDspId2     , &ipcFramesInDspPrm2    , sizeof(ipcFramesInDspPrm2));
		System_linkCreate(scdId,&scdPrm,sizeof(scdPrm));
		System_linkCreate(ipcBitsOutDspId2, &ipcBitsOutDspPrm2, sizeof(ipcBitsOutDspPrm2));
   		System_linkCreate(ipcBitsInHostId2, &ipcBitsInHostPrm2, sizeof(ipcBitsInHostPrm2));
	
	}

    System_linkCreate(dup2Id, &dup2Prm, sizeof(dup2Prm));
    for (i=0; i<2; i++) {
       System_linkCreate(swMsId[i], &swMsPrm[i], sizeof(swMsPrm[i]));
       System_linkCreate(displayId[i], &displayPrm[i], sizeof(displayPrm[i]));
    }

    //Chains_memPrintHeapStatus();

   
    System_linkStart(encId);
    for (i=0; i<2; i++) {
          System_linkStart(displayId[i]);
          System_linkStart(swMsId[i]);
    }

  	grpx_init(GRPX_FORMAT_RGB888);
	
     System_linkStart(dup2Id);
     System_linkStart(nsfId);
     System_linkStart(dupId);
//guo scd
	System_linkStart(ipcBitsInHostId2);
	System_linkStart(ipcBitsOutDspId2);
	System_linkStart(scdId);
	System_linkStart(ipcFramesInDspId2);
	System_linkStart(ipcFramesOutVpssId2);
	System_linkStart(nsfId2);
	System_linkStart(sclrId);
//guo OSD
	if(enableOsdAlgLink)
        {
            System_linkStart(osdId);
            System_linkStart(ipcFramesInDspId);
            System_linkStart(ipcFramesOutVpssId);		
        }
	
        System_linkStart(mergeId);
        System_linkStart(captureId);
 	//grpx_fb_draw_demo();
	Chains_ipcBitsLocSt();	//start RTP (or Local Storage)	
	while(1)//RUN SYSTEM SERVER
	{
		//Chains_ScdPrint(scdId);//This is only for temp test!!!
		sleep(2);
		Chains_swMsSwitchLayout(&swMsId, &swMsPrm, TRUE, FALSE, 2);
		Chains_ScdPrint(scdId);//This is only for temp test!!!
		sleep(1);
		Chains_swMsSwitchLayout(&swMsId, &swMsPrm, FALSE, TRUE, 2);
	}	
//This is for user input args ,when runs system server this is not supported
#if 0
        while(1)
        {
            printf("My menu:Input L to storage the video and E to stop\n"
				"Input P for statistic\n");
			
            ch = Chains_menuRunTime();
            if(ch=='0')

                break;
            if(ch=='v')
                System_linkControl(captureId, CAPTURE_LINK_CMD_FORCE_RESET, NULL, 0, TRUE);
            if(ch=='p')
            {
                System_linkControl(captureId, CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
		  System_linkControl(captureId,SYSTEM_COMMON_CMD_PRINT_STATUS,NULL,0,FALSE);
		  System_linkControl(encId,SYSTEM_COMMON_CMD_PRINT_STATUS,NULL,0,FALSE);
		  System_linkControl(osdId,SYSTEM_COMMON_CMD_PRINT_STATUS,NULL,0,FALSE);
            }
	   if(ch=='P')
	   {
	   	Vsys_printDetailedStatistics();
		continue;
	   }
	   if(ch == 'd')//draw
	   	{
	   		 grpx_draw_box(500,500,500,500);//在HDMI2上画了出来
	   		 grpx_fb_draw_grid(300,300,200,100,11);
	   		 continue;
	   	}
	   if(ch == 'x')
	   	{
	   	grpx_draw_box_exit(500,500,500,500);
		grpx_fb_draw_grid_exit(300,300,200,100,11);
	 	grpx_exit();
		continue;
	   	}

          {
            	 Bool switchCh = FALSE;
            	 Bool switchLayout = FALSE;
		if(ch=='L')
		{
			Chains_ipcBitsLocSt();
			continue;
		}
		if(ch=='E')
		{
			Chains_ipcBitsLocStStop();
			continue;
		}
            	 if(ch=='s')
            	 {
            		 switchLayout = TRUE;
            		 Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 2);
            		printf("switch layout\n");
			continue;

            	 }

            	 if(ch == '1')
            	 {
            		 switchCh = TRUE;
            		 Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 0);
            		 continue;
            	 }
            	 if(ch == '2')
            	 {
            		 switchCh = TRUE;
            		 Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 1);
            		 continue;
            	 }
		
            }
		Chains_ScdPrint(scdId);//This is only for temp test!!!
        }
#endif

        System_linkStop(captureId);
        System_linkStop(mergeId);
//guo----------------------
	if(enableScdAlgLink)
	{
		Scd_bitsWriteStop();
		System_linkStop(sclrId);
		System_linkStop(nsfId2);	
		System_linkStop(ipcFramesOutVpssId2);
		System_linkStop(ipcFramesInDspId2);
		System_linkStop(scdId);
		System_linkStop(ipcBitsOutDspId2);
		System_linkStop(ipcBitsInHostId2);	
	}
	if(enableOsdAlgLink)
        {
            Chains_ipcFramesStop();
            System_linkStop(ipcFramesOutVpssId);
            System_linkStop(ipcFramesInDspId);
            System_linkStop(osdId);
        }
//-------------------------------
        System_linkStop(dupId);
        System_linkStop(nsfId);
        System_linkStop(dup2Id);
        for (i=0; i<2; i++) {
            System_linkStop(swMsId[i]);
            System_linkStop(displayId[i]);
        }
        System_linkStop(encId);
        Chains_ipcBitsStop();
        System_linkStop(ipcBitsOutVideoId);
        System_linkStop(ipcBitsInHostId);
    

    System_linkDelete(captureId);
    System_linkDelete(mergeId);
//guo-------------
	if(enableScdAlgLink)
	{
		Scd_bitsWriteDelete();
		System_linkDelete(sclrId);
		System_linkDelete(nsfId2);	
		System_linkDelete(ipcFramesOutVpssId2);
		System_linkDelete(ipcFramesInDspId2);
		System_linkDelete(scdId);
		System_linkDelete(ipcBitsOutDspId2);
		System_linkDelete(ipcBitsInHostId2);	
	}
    if(enableOsdAlgLink)
    {
        System_linkDelete(ipcFramesOutVpssId);
        System_linkDelete(ipcFramesInDspId);
        System_linkDelete(osdId);         
    }
//-----------------
    System_linkDelete(dupId);
    System_linkDelete(nsfId);
    System_linkDelete(dup2Id);
    for (i=0; i<2; i++) {
        System_linkDelete(swMsId[i]);
        System_linkDelete(displayId[i]);
    }
    System_linkDelete(ipcOutVpssId);
    System_linkDelete(ipcInVideoId);
    System_linkDelete(encId);
    System_linkDelete(ipcBitsOutVideoId);
    System_linkDelete(ipcBitsInHostId);
    Chains_ipcBitsExit();

    Chains_displayCtrlDeInit();
}




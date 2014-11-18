/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "multich_common.h"
/*

		          2D1 Encode <OR> 1D1+4CIF Encode


				       Capture (YUV422I)
					0-3 |           |4-7
						|           |
					  ------        |
				      Select        |
					  ------        |
					 0-1|           |0-3
						|           |
					-------       -------
					  DEI           DEI
                     VIP-SC       VIP-SC
					-------       -------
						|           |
						|           |
						|           |
                    --------     -------
                      DUP0         DUP1
                    --------     -------
				 0,1|  0,1 |  0-3 |    | 0-3
                    |      |      |    -------------SWMS1 [4CIF Mosaic]---NSF
                    |      |      |                                        |    
                    |      |      |                                        |
                    |      |      |                                        |
                    |       ----------------------------------------       |
                    |             |                                |       |
                    |             |                               ------------
                    |             |                                VENC_MERGE --->IPCM3OUT(VPSS)---IPCM3IN(VID)----ENC-----IPCBITS_RTOSOUT(VID)----IPCBITS_HLOSIN(HOST)-----FILEOUT
                    |             |                               ------------
                    |             |
                    |             |
                    |             |
                    |             |
                    |             |
                   ------------------
				        VDIS_MERGE
				   ------------------
                          |
                          |
						SWMS
                          |
						  |
				   DISPLAY <SDTV>  -- <CH0,CH1 for 2D1 encode; CH0-CH4 for 1D1+4CIF>


  */


#define VCAP_OUT0_LINK_IDX (0)
#define VCAP_OUT1_LINK_IDX (1)
#define NUM_DUP_LINK           (2)

#define VENC_MERGE_LINK_IDX    (0)
#define VDIS_MERGE_LINK_IDX    (1)
#define NUM_MERGE_LINK         (2)

#define MAX_DEC_OUT_FRAMES_PER_CH   (5)

#define     NUM_CAPTURE_DEVICES          2


typedef struct {

    /* capture subsystem link ID's - other link ID's are in gVcapModuleContext */
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 selectId;

    /* encode, deccode, display subsystem link ID's are in gVxxxModuleContext */

} MultiCh_CustomVcapVencVdecVdisObj;

MultiCh_CustomVcapVencVdecVdisObj gMultiCh_customVcapVencVdecVdisObj;


static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 8,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0 , 0, 0},

        .DecNumCh  = 8,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0 , 0, 0},
    },
    .ivaMap[1] =  {  .EncNumCh  = 0,  .DecNumCh  = 0,  },
    .ivaMap[2] =  {  .EncNumCh  = 0,  .DecNumCh  = 0,  },
};


Int32 MultiCh_createCustomVcap(
            System_LinkInQueParams *vdecOutQue,
            System_LinkInQueParams *vencInQue,
            System_LinkInQueParams *vdisInQue,
            UInt32 numVipInst,
            UInt32 tilerEnable
        );

Int32 MultiCh_createCustomVdis(System_LinkInQueParams *vdisInQue);

static
Void MultiCh_setCustomAvsyncVidQuePrm(Avsync_SynchConfigParams *queCfg,
                                            Int chnum,
                                            UInt32 avsStartChNum,
                                            UInt32 avsEndChNum);

static
Void MultiCh_setCustomAvsyncPrm(AvsyncLink_LinkSynchConfigParams *avsyncPrm,
                                  UInt32 prevLinkID,
                                  UInt32 prevLinkQueId,
                                    UInt32 swMSId);

/*
    Create use-case

    Separate create function for each subsystem.
    Create gets called only if the subsystem is enabled.
*/
Void MultiCh_createCustomVcapVencVdecVdis()
{
    Int32 i;
    Bool tilerEnable;
    UInt32 vdecNextLinkId;
    UInt32 numVipInst;

    System_LinkInQueParams   vdecOutQue;
    System_LinkInQueParams   vencInQue;
    System_LinkInQueParams   vdisInQue;


    tilerEnable = FALSE;

    /* init gMultiCh_customVcapVencVdecVdisObj to known default state */
    for(i=0; i<NUM_MERGE_LINK; i++)
    {
        gMultiCh_customVcapVencVdecVdisObj.mergeId[i] = SYSTEM_LINK_ID_INVALID;
    }

    for(i=0; i<NUM_DUP_LINK; i++)
    {
        gMultiCh_customVcapVencVdecVdisObj.dupId[i] = SYSTEM_LINK_ID_INVALID;
    }

    gVcapModuleContext.capSwMsId = SYSTEM_LINK_ID_INVALID;

    gMultiCh_customVcapVencVdecVdisObj.mergeId[VDIS_MERGE_LINK_IDX]  = SYSTEM_VPSS_LINK_ID_MERGE_1;

    System_LinkInQueParams_Init(&vdecOutQue);
    System_LinkInQueParams_Init(&vencInQue);
    System_LinkInQueParams_Init(&vdisInQue);

    /* setup enc/dec channel map */
    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );


    if(gVsysModuleContext.vsysConfig.enableDecode)
    {
        /* create decode subsystem */

        if(gVsysModuleContext.vsysConfig.enableCapture)
            vdecNextLinkId = gMultiCh_customVcapVencVdecVdisObj.mergeId[VDIS_MERGE_LINK_IDX];
        else
            vdecNextLinkId = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;

        Vdec_create(
            &vdecOutQue, /* output by this function */
            vdecNextLinkId,
            tilerEnable,
            MAX_DEC_OUT_FRAMES_PER_CH
            );
    }
    else
    {
        /* if decode is not enabled then capture MUST be enabled */
        gVsysModuleContext.vsysConfig.enableCapture = TRUE;
    }

    if(gVsysModuleContext.vsysConfig.enableCapture)
    {
        if(gVsysModuleContext.vsysConfig.enableSecondaryOut)
            numVipInst = 2;
        else
            numVipInst = 1;

        /* create capture subsystem */
        MultiCh_createCustomVcap(
            &vdecOutQue, /* input to this function */
            &vencInQue,  /* output by this function */
            &vdisInQue,  /* output by this function */
            numVipInst,
            FALSE
            );
    }
    else
    {
        /* if capture is not enabled then display input = decode output */
        vdisInQue = vdecOutQue;
    }

    /* create display subsystem based on output of capture and decode subsystem */
    MultiCh_createCustomVdis(
        &vdisInQue  /* input to this function */
        );

    /* if capture is enabled then encode is also enabled */
    if(gVsysModuleContext.vsysConfig.enableCapture)
    {
        /* create encode subsystem based on output of capture subsystem */
        Venc_create(
            &vencInQue  /* input to this function */
            );
    }

    /* print memory consumed status */
    MultiCh_memPrintHeapStatus();
    if(gVsysModuleContext.vsysConfig.enableDecode)
    {
        MultiCh_setDec2DispMap(VDIS_DEV_SD,gVdecModuleContext.vdecConfig.numChn,0,0);
    }
}


Void MultiCh_setCustomVcapSwMs(SwMsLink_CreateParams *swMsCreateArgs, Bool forceLowCostScaling)
{
    SwMsLink_LayoutPrm *layoutInfo;
    SwMsLink_LayoutWinInfo *winInfo;
    UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
    UInt32 rowMax, colMax;

    if (swMsCreateArgs->maxOutRes == VSYS_STD_PAL)
    {
        outWidth = 720;
        outHeight = 576;
    }
    else
    {
        outWidth = 720;
        outHeight = 480;
    }

    widthAlign = 8;
    heightAlign = 1;

    layoutInfo = &swMsCreateArgs->layoutPrm;

    /* init to known default */
    memset(layoutInfo, 0, sizeof(*layoutInfo));

    rowMax = 2;
    colMax = 2;

    {
      layoutInfo->onlyCh2WinMapChanged = FALSE;
      layoutInfo->outputFPS = 30;
      layoutInfo->numWin = 4;
      for(row=0; row<rowMax; row++)
      {
          for(col=0; col<colMax; col++)
          {
              winId = row*colMax+col;

              winInfo = &layoutInfo->winInfo[winId];

              winInfo->width  = SystemUtils_align(outWidth/colMax, widthAlign);
              winInfo->height = SystemUtils_align(outHeight/rowMax, heightAlign);
              winInfo->startX = winInfo->width*col;
              winInfo->startY = winInfo->height*row;
              if (forceLowCostScaling == TRUE)
                winInfo->bypass = TRUE;
              else
                winInfo->bypass = FALSE;
              winInfo->channelNum = winId;
          }
      }
    }
}


Int32 MultiCh_createCustomVcap(
            System_LinkInQueParams *vdecOutQue,
            System_LinkInQueParams *vencInQue,
            System_LinkInQueParams *vdisInQue,
            UInt32 numVipInst,
            UInt32 tilerEnable
        )
{
    CaptureLink_CreateParams    capturePrm;
    DeiLink_CreateParams        deiPrm[NUM_DUP_LINK];
    DupLink_CreateParams        dupPrm[NUM_DUP_LINK];
    SwMsLink_CreateParams       swMsPrm;
    NsfLink_CreateParams        nsfPrm;
    MergeLink_CreateParams      mergePrm[NUM_MERGE_LINK];
    AvsyncLink_LinkSynchConfigParams avsyncPrms;

    CaptureLink_VipInstParams  *pCaptureInstPrm;
    CaptureLink_OutParams      *pCaptureOutPrm;
    SelectLink_CreateParams    selectPrm;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];
    UInt32 i, vipInstId;

    gVcapModuleContext.captureId                                     = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.deiId[VCAP_OUT0_LINK_IDX] = SYSTEM_LINK_ID_DEI_0; // use the same DEI
    gVcapModuleContext.deiId[VCAP_OUT1_LINK_IDX] = SYSTEM_LINK_ID_DEI_1; // use the same DEI

    gMultiCh_customVcapVencVdecVdisObj.dupId[VCAP_OUT0_LINK_IDX] = SYSTEM_VPSS_LINK_ID_DUP_0;
    gMultiCh_customVcapVencVdecVdisObj.dupId[VCAP_OUT1_LINK_IDX] = SYSTEM_VPSS_LINK_ID_DUP_1;

    gVcapModuleContext.nsfId[0]  = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.capSwMsId = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;

    gMultiCh_customVcapVencVdecVdisObj.mergeId[VENC_MERGE_LINK_IDX]  = SYSTEM_VPSS_LINK_ID_MERGE_0;
    gMultiCh_customVcapVencVdecVdisObj.selectId = SYSTEM_VPSS_LINK_ID_SELECT_0;

    vencInQue->prevLinkId    = gMultiCh_customVcapVencVdecVdisObj.mergeId[VENC_MERGE_LINK_IDX];
    vencInQue->prevLinkQueId = 0;

    vdisInQue->prevLinkId    = gMultiCh_customVcapVencVdecVdisObj.mergeId[VDIS_MERGE_LINK_IDX];
    vdisInQue->prevLinkQueId = 0;

    MULTICH_INIT_STRUCT(CaptureLink_CreateParams, capturePrm);

    if(numVipInst>2)
        numVipInst = 2;
    capturePrm.numVipInst    = numVipInst;
    capturePrm.tilerEnable   = FALSE;
    capturePrm.enableSdCrop  = FALSE;
    capturePrm.numBufsPerCh  = 8;

    MULTICH_INIT_STRUCT(SelectLink_CreateParams, selectPrm);
    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        MULTICH_INIT_STRUCT(DeiLink_CreateParams, deiPrm[vipInstId]);

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
        pCaptureOutPrm->outQueId            = vipInstId;

        if (vipInstId == 0)
        {
            capturePrm.outQueParams[vipInstId].nextLink = gMultiCh_customVcapVencVdecVdisObj.selectId;
            deiPrm[vipInstId].inQueParams.prevLinkId    = gMultiCh_customVcapVencVdecVdisObj.selectId;
            deiPrm[vipInstId].inQueParams.prevLinkQueId = 0;

            selectPrm.numOutQue = 1;
            selectPrm.outQueParams[0].nextLink = gVcapModuleContext.deiId[vipInstId];
            selectPrm.inQueParams.prevLinkId = gVcapModuleContext.captureId;
            selectPrm.inQueParams.prevLinkQueId = vipInstId;
            selectPrm.outQueChInfo[0].outQueId = 0;
            /*
                     *  2 encode usecases are differentiated by numVipInst. 
                     *  If vipInst = 1, 2 D1 capture, so select ch0 & 1.
                     *  If vipInst = 2, 1 D1 of vip0 & 4 D1<CIF> of vip1 will be used. 
                     *  So select ch0 from vip0 & ch0-3 from vip1.
                     */
            if (numVipInst > 1)
            {
                selectPrm.outQueChInfo[0].numOutCh = 1;  
                selectPrm.outQueChInfo[0].inChNum[0] = 0;
            }
            else
            {
                selectPrm.outQueChInfo[0].numOutCh = 2;  
                selectPrm.outQueChInfo[0].inChNum[0] = 0;
                selectPrm.outQueChInfo[0].inChNum[1] = 1;
            }
        }
        else
        {
            capturePrm.outQueParams[vipInstId].nextLink = gVcapModuleContext.deiId[vipInstId];
            deiPrm[vipInstId].inQueParams.prevLinkId    = gVcapModuleContext.captureId;
            deiPrm[vipInstId].inQueParams.prevLinkQueId = vipInstId;
        }

        deiPrm[vipInstId].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink= gMultiCh_customVcapVencVdecVdisObj.dupId[vipInstId];
        deiPrm[vipInstId].enableOut[DEI_LINK_OUT_QUE_VIP_SC]            = TRUE;
        deiPrm[vipInstId].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = tilerEnable;
        deiPrm[vipInstId].inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]       = 60;
        deiPrm[vipInstId].outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]      = 30;
        deiPrm[vipInstId].enableOut[DEI_LINK_OUT_QUE_DEI_SC]                = FALSE;
        deiPrm[vipInstId].enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]  = FALSE;
        /* rest all DEI params are default */

        dupPrm[vipInstId].inQueParams.prevLinkId         = gVcapModuleContext.deiId[vipInstId];
        dupPrm[vipInstId].inQueParams.prevLinkQueId      = DEI_LINK_OUT_QUE_VIP_SC;
        dupPrm[vipInstId].numOutQue                      = 2;
        if(vipInstId==0)
            dupPrm[vipInstId].outQueParams[0].nextLink       = gMultiCh_customVcapVencVdecVdisObj.mergeId[VENC_MERGE_LINK_IDX];
        else
            dupPrm[vipInstId].outQueParams[0].nextLink       = gVcapModuleContext.capSwMsId;
        dupPrm[vipInstId].outQueParams[1].nextLink       = gMultiCh_customVcapVencVdecVdisObj.mergeId[VDIS_MERGE_LINK_IDX];
        dupPrm[vipInstId].notifyNextLink                 = TRUE;
    }

    for(i = 0; i < NUM_CAPTURE_DEVICES; i++)
    {
        vidDecVideoModeArgs[i].videoIfMode        = DEVICE_CAPT_VIDEO_IF_MODE_8BIT;
        vidDecVideoModeArgs[i].videoDataFormat    = SYSTEM_DF_YUV422P;
        vidDecVideoModeArgs[i].standard           = SYSTEM_STD_MUX_4CH_D1;
        vidDecVideoModeArgs[i].videoCaptureMode   =
                    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_PIXEL_MUX_EMBEDDED_SYNC;
        vidDecVideoModeArgs[i].videoSystem        =
                                      DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT;
        vidDecVideoModeArgs[i].videoCropEnable    = FALSE;
        vidDecVideoModeArgs[i].videoAutoDetectTimeout = -1;
    }

    Vcap_configVideoDecoder(vidDecVideoModeArgs, numVipInst);

    SwMsLink_CreateParams_Init(&swMsPrm);

    swMsPrm.numSwMsInst          = 1;
    swMsPrm.swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm.swMsInstStartWin[0]  = 0;

    swMsPrm.inQueParams.prevLinkId     = gMultiCh_customVcapVencVdecVdisObj.dupId[1];
    swMsPrm.inQueParams.prevLinkQueId  = 0;
    swMsPrm.outQueParams.nextLink      = gVcapModuleContext.nsfId[0];
    swMsPrm.maxOutRes                  = VSYS_STD_PAL;
    swMsPrm.initOutRes                 = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
    swMsPrm.lineSkipMode               = FALSE;
    swMsPrm.maxInputQueLen             = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;

    swMsPrm.enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

    MultiCh_setCustomVcapSwMs(&swMsPrm, FALSE);

    NsfLink_CreateParams_Init(&nsfPrm);
    nsfPrm.bypassNsf                 = TRUE;
    nsfPrm.tilerEnable               = tilerEnable;
    nsfPrm.inQueParams.prevLinkId    = gVcapModuleContext.capSwMsId;
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.numOutQue                 = 1;
    nsfPrm.outQueParams[0].nextLink  = gMultiCh_customVcapVencVdecVdisObj.mergeId[VENC_MERGE_LINK_IDX];

    mergePrm[VENC_MERGE_LINK_IDX].numInQue                     = capturePrm.numVipInst;
    mergePrm[VENC_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = gMultiCh_customVcapVencVdecVdisObj.dupId[0];
    mergePrm[VENC_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = 0;
    mergePrm[VENC_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = gVcapModuleContext.nsfId[0];
    mergePrm[VENC_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = 0;
    mergePrm[VENC_MERGE_LINK_IDX].outQueParams.nextLink        = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    mergePrm[VENC_MERGE_LINK_IDX].notifyNextLink               = TRUE;

    mergePrm[VDIS_MERGE_LINK_IDX].numInQue                     = capturePrm.numVipInst;

    if(gVsysModuleContext.vsysConfig.enableDecode)
        mergePrm[VDIS_MERGE_LINK_IDX].numInQue++;   /* + 1 for decode CHs */

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        mergePrm[VDIS_MERGE_LINK_IDX].inQueParams[vipInstId].prevLinkId    = gMultiCh_customVcapVencVdecVdisObj.dupId[vipInstId];
        mergePrm[VDIS_MERGE_LINK_IDX].inQueParams[vipInstId].prevLinkQueId = 1;
    }
    mergePrm[VDIS_MERGE_LINK_IDX].inQueParams[vipInstId].prevLinkId    = vdecOutQue->prevLinkId;
    mergePrm[VDIS_MERGE_LINK_IDX].inQueParams[vipInstId].prevLinkQueId = vdecOutQue->prevLinkQueId;
    mergePrm[VDIS_MERGE_LINK_IDX].outQueParams.nextLink        = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    mergePrm[VDIS_MERGE_LINK_IDX].notifyNextLink               = TRUE;

#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif
    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));

    System_linkCreate (gMultiCh_customVcapVencVdecVdisObj.selectId, &selectPrm, sizeof(selectPrm));

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        System_linkCreate(
            gVcapModuleContext.deiId[vipInstId],
            &deiPrm[vipInstId],
            sizeof(deiPrm[vipInstId])
            );

        System_linkCreate(
            gMultiCh_customVcapVencVdecVdisObj.dupId[vipInstId],
            &dupPrm[vipInstId],
            sizeof(dupPrm[vipInstId])
            );
    }

    if(capturePrm.numVipInst>1)
    {
        /* Set dummy avsync params & disable AVSync as this swMS Out is not going to display */
        MultiCh_setCustomAvsyncPrm(&avsyncPrms, 
                            swMsPrm.inQueParams.prevLinkId,
                            swMsPrm.inQueParams.prevLinkQueId,
                            gVcapModuleContext.capSwMsId);
        System_linkCreate(gVcapModuleContext.capSwMsId, &swMsPrm, sizeof(swMsPrm));
        System_linkCreate(gVcapModuleContext.nsfId[0] , &nsfPrm, sizeof(nsfPrm));
    }

    for(i=0; i<NUM_MERGE_LINK; i++)
    {
        System_linkCreate(
            gMultiCh_customVcapVencVdecVdisObj.mergeId[i],
            &mergePrm[i],
            sizeof(mergePrm[i])
          );
    }

#if     0
    /* disable channels that are not required */
    {
        DeiLink_ChannelInfo deiChDisable;

        deiChDisable.channelId  = 2;
        deiChDisable.streamId   = DEI_LINK_OUT_QUE_VIP_SC;
        deiChDisable.enable     = FALSE;

        System_linkControl(
            gVcapModuleContext.deiId[0],
            DEI_LINK_CMD_DISABLE_CHANNEL,
            &deiChDisable,
            sizeof(deiChDisable),
            TRUE
            );

        deiChDisable.channelId  = 3;

        System_linkControl(
            gVcapModuleContext.deiId[0],
            DEI_LINK_CMD_DISABLE_CHANNEL,
            &deiChDisable,
            sizeof(deiChDisable),
            TRUE
            );

        if(vipInstId>1)
        {
            deiChDisable.channelId  = 1;

            System_linkControl(
                gVcapModuleContext.deiId[0],
                DEI_LINK_CMD_DISABLE_CHANNEL,
                &deiChDisable,
                sizeof(deiChDisable),
                TRUE
            );
        }
    }
#endif

    return 0;
}

Int32 MultiCh_createCustomVdis(System_LinkInQueParams *vdisInQue)
{
    SwMsLink_CreateParams       swMsPrm;
    DisplayLink_CreateParams    displayPrm;

    UInt32 displayIdx;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    displayIdx = 1; // SDTV on Centaurus
#else
    displayIdx = 2; // SDTV on Netra
#endif

    gVdisModuleContext.swMsId[displayIdx]    = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.displayId[displayIdx] = SYSTEM_LINK_ID_DISPLAY_2; // SDTV

    MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm);

    swMsPrm.numSwMsInst          = 1;
    swMsPrm.swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm.swMsInstStartWin[0]  = 0;

    swMsPrm.inQueParams.prevLinkId     = vdisInQue->prevLinkId;
    swMsPrm.inQueParams.prevLinkQueId  = vdisInQue->prevLinkQueId;
    swMsPrm.outQueParams.nextLink      = gVdisModuleContext.displayId[displayIdx];

    swMsPrm.maxOutRes                  = VSYS_STD_PAL;
    swMsPrm.initOutRes                 = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
    swMsPrm.lineSkipMode               = FALSE;
    swMsPrm.maxInputQueLen             = SYSTEM_SW_MS_INVALID_INPUT_QUE_LEN;

    swMsPrm.enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

    MultiCh_swMsGetDefaultLayoutPrm(SYSTEM_DC_VENC_SD, &swMsPrm, FALSE);
    MULTICH_INIT_STRUCT(DisplayLink_CreateParams ,displayPrm);
    displayPrm.inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[displayIdx];
    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.displayRes  = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;


    System_linkCreate(gVdisModuleContext.swMsId[displayIdx]   , &swMsPrm   , sizeof(swMsPrm)   );
    System_linkCreate(gVdisModuleContext.displayId[displayIdx], &displayPrm, sizeof(displayPrm));

    return 0;
}

/*
    Delete use-case

    No separate delete function for each subsystem, since the overall
    delete sequence is very short

    While deleting, the McFW APIs knows whats created.
    So delete is called for each sub-system.
    If the sub-system was not created to being with then the
    delete will have no effect.
*/
Void MultiCh_deleteCustomVcapVencVdecVdis()
{
    Int32 i;

    /* delete individual subsystem  */
    Vcap_delete();
    Vdec_delete();
    Venc_delete();
    Vdis_delete();

    /* delete additional use-case specific link's   */
    for(i=0; i<NUM_MERGE_LINK; i++)
    {
        if(gMultiCh_customVcapVencVdecVdisObj.mergeId[i]
                !=
            SYSTEM_LINK_ID_INVALID
            )
        {
            System_linkDelete(gMultiCh_customVcapVencVdecVdisObj.mergeId[i]);
        }
    }

    for(i=0; i<NUM_DUP_LINK; i++)
    {
        if(gMultiCh_customVcapVencVdecVdisObj.dupId[i]
                !=
            SYSTEM_LINK_ID_INVALID
            )
        {
            System_linkDelete(gMultiCh_customVcapVencVdecVdisObj.dupId[i]);
        }
    }
    System_linkDelete(gMultiCh_customVcapVencVdecVdisObj.selectId);

    /* Print slave processor load info */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

}

/** These are dummy settings as swMS used in vcap here is tied to avsync; so set dummy params & disable avsync */
static
Void MultiCh_setCustomAvsyncVidQuePrm(Avsync_SynchConfigParams *queCfg,
                                            Int chnum,
                                            UInt32 avsStartChNum,
                                            UInt32 avsEndChNum)
{
    queCfg->chNum = chnum;
    queCfg->audioPresent = FALSE;
    queCfg->avsyncEnable = FALSE;
    queCfg->clkAdjustPolicy.refClkType = AVSYNC_REFCLKADJUST_BYVIDEO;
    queCfg->playTimerStartTimeout = 0;
    queCfg->playStartMode = AVSYNC_PLAYBACK_START_MODE_WAITSYNCH;
    queCfg->clkAdjustPolicy.clkAdjustLead = AVSYNC_VIDEO_REFCLKADJUST_MAX_LEAD_MS;
    queCfg->clkAdjustPolicy.clkAdjustLag = AVSYNC_VIDEO_REFCLKADJUST_MAX_LAG_MS;
}

static
Void MultiCh_setCustomAvsyncPrm(AvsyncLink_LinkSynchConfigParams *avsyncPrm,
                                  UInt32 prevLinkID,
                                  UInt32 prevLinkQueId,
                                    UInt32 swMSId)
{
    System_LinkInfo                   swmsInLinkInfo;
    Int i;
    Int32 status;

    Vdis_getAvsyncConfig(VDIS_DEV_SD,avsyncPrm);
    avsyncPrm->displayLinkID        = Vdis_getDisplayId(VDIS_DEV_SD);
    avsyncPrm->videoSynchLinkID = swMSId;
    System_linkGetInfo(prevLinkID,&swmsInLinkInfo);
    OSA_assert(swmsInLinkInfo.numQue > prevLinkQueId);

    avsyncPrm->numCh            = swmsInLinkInfo.queInfo[prevLinkQueId].numCh;
    avsyncPrm->syncMasterChnum = 0;
    for (i = 0; i < avsyncPrm->numCh;i++)
    {
        MultiCh_setCustomAvsyncVidQuePrm(&avsyncPrm->queCfg[i],
                                             i,
                                             0,
                                            (0 + (avsyncPrm->numCh - 1)));
    }
    Vdis_setAvsyncConfig(VDIS_DEV_SD,avsyncPrm);

    status = Avsync_configSyncConfigInfo(avsyncPrm);
    OSA_assert(status == 0);

}



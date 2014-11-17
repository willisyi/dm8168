/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <demos/link_api_demos/common/chains_ipcBuf.h>
#include <demos/link_api_demos/common/chains_ipcFrames.h>
#include <demos/graphic/graphic.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>



/**
                        Capture (YUV422I) 16CH D1 60fps
                          |
                          |
                         NSF (YUV420SP) ------------------+
                          |                               |
                          |                               |
                        DEIH (VIP-SC YUV420 )     DEI (VIP-SC YUV420 )
                          |                               |
                          |+------------------------------+
                        MERGE1
                          |
                          |
                         DUP1
                          ||-------IPCM3OUT(VPS)--IPCM3IN(VID)--ENC
                          |                                     |
                          |+-------IPCM3IN(VPS)--IPCM3OUT(VID)--DEC
                        MERGE2
                          |
                          |
                         DUP2
                         |||
         +---------------+|+------------+
         |                |             |
         |                |             |
      SW Mosaic       SW Mosaic         |
      (SC5 YUV422I)  (SC5 YUV422I)      |
         |                |             |
 GRPX0   |       GRPX1,2  |             |
    |    |           |    |             |
    On-Chip HDMI    Off-Chip HDMI  SDTV (NTSC)
      1080p60         1080p60        480i60
*/

/* To enable or disable graphics in the application */
#define ENABLE_GRPX 1

/* To select if FBDEV interface is used for Graphics */
#define USE_FBDEV   0

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
        .EncNumCh  = 16,
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


Void chains_multiChDucatiSystemUseCaseSwMsTriDisplay1(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
    AlgLink_CreateParams        osdPrm;
    DeiLink_CreateParams        deiPrm[2];
    MergeLink_CreateParams      merge1Prm;
    MergeLink_CreateParams      merge2Prm;
    DupLink_CreateParams        dup1Prm;
    DupLink_CreateParams        dup2Prm;
    static SwMsLink_CreateParams       swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    DisplayLink_CreateParams    displayPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    IpcLink_CreateParams        ipcOutVpssPrm;
    IpcLink_CreateParams        ipcInVpssPrm;
    IpcLink_CreateParams        ipcOutVideoPrm;
    IpcLink_CreateParams        ipcInVideoPrm;
    EncLink_CreateParams        encPrm;
    DecLink_CreateParams        decPrm;
    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams    ipcBitsInHostPrm;
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssToHostPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInHostPrm;
    IpcFramesOutLinkHLOS_CreateParams  ipcFramesOutHostPrm;
    IpcFramesInLinkRTOS_CreateParams   ipcFramesInVpssFromHostPrm;

    System_LinkInfo bitsProducerLinkInfo;
    System_LinkInfo framesProducerLinkInfo;

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;

    UInt32 captureId;
    UInt32 osdId;
    UInt32 nsfId;
    UInt32 deiId[2];
    UInt32 swMsId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 displayId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 grpxId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 nullId;
    UInt32 merge1Id, merge2Id, dup1Id, dup2Id;
    UInt32 deiOutQue;
    UInt32 encId, decId;
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    UInt32 ipcBitsOutVideoId, ipcBitsInHostId;
    UInt32 ipcBitsInVideoId, ipcBitsOutHostId;
    Uint32 ipcFramesOutVpssId, ipcFramesInDspId;
    Uint32 ipcFramesOutVpssToHost, ipcFramesInHost;
    Uint32 ipcFramesOutHost, ipcFramesInVpssFromHost;

    UInt32 vipInstId;
    UInt32 i, j;
    UInt32 displayRes[SYSTEM_DC_MAX_VENC];
    UInt32 enableGrpx, enableNsfLink;
    Bool enableOsdAlgLink;
    Bool enableVidFrameExport;
    UInt32 numSubChains;

    Ptr osdWinSrAddr[ALG_LINK_OSD_MAX_CH][CHAINS_OSD_NUM_WINDOWS];
    Ptr osdWinBuPtr[ALG_LINK_OSD_MAX_CH][CHAINS_OSD_NUM_WINDOWS];

    Bool switchCh;
    Bool switchLayout;
    Bool enableSdtv;
    char ch;
#ifdef  SYSTEM_ENABLE_AUDIO
    Bool    audioCaptureActive = FALSE;
    Bool    audioPlaybackActive = FALSE;
    Int8    audioCaptureChNum = 0;
    Int8    audioPlaybackChNum = 0;
    Bool    audioPathSetFlag = FALSE;
#endif

    Chains_ipcBitsInit();
    Chains_ipcFramesInit();

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams, capturePrm);
    CHAINS_INIT_STRUCT(DeiLink_CreateParams, deiPrm[0]);
    CHAINS_INIT_STRUCT(DeiLink_CreateParams, deiPrm[1]);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    CHAINS_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm);
    CHAINS_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssToHostPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInHostPrm);
    CHAINS_INIT_STRUCT(IpcFramesOutLinkHLOS_CreateParams ,ipcFramesOutHostPrm);
    CHAINS_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  ,ipcFramesInVpssFromHostPrm);
    CHAINS_INIT_STRUCT(DecLink_CreateParams, decPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams, encPrm);

    for (i = 0; i < CHAINS_SW_MS_MAX_DISPLAYS; i++)
    {
        CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
        CHAINS_INIT_STRUCT(SwMsLink_CreateParams, swMsPrm[i]);
    }

    captureId    = SYSTEM_LINK_ID_CAPTURE;
    osdId        = SYSTEM_LINK_ID_ALG_0;
    nsfId        = SYSTEM_LINK_ID_NSF_0;
    deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    merge1Id      = SYSTEM_VPSS_LINK_ID_MERGE_0;
    merge2Id      = SYSTEM_VPSS_LINK_ID_MERGE_1;
    dup1Id        = SYSTEM_VPSS_LINK_ID_DUP_0;
    dup2Id        = SYSTEM_VPSS_LINK_ID_DUP_1;

    swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    swMsPrm[1].numSwMsInst = 1;
    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;

    displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI
    displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; // OFF CHIP HDMI
    grpxId[0]    = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]    = SYSTEM_LINK_ID_GRPX_1;
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
    grpxId[2]    = SYSTEM_LINK_ID_GRPX_2;
#endif
    nullId       = SYSTEM_VPSS_LINK_ID_NULL_0;

    ipcFramesOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
    ipcFramesInDspId   = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    encId        = SYSTEM_LINK_ID_VENC_0;
    decId        = SYSTEM_LINK_ID_VDEC_0;

    ipcBitsOutVideoId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInHostId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    ipcBitsOutHostId  = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInVideoId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

    ipcFramesOutVpssToHost  = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
    ipcFramesInHost         = SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0;
    ipcFramesOutHost        = SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0;
    ipcFramesInVpssFromHost = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_0;

    chainsCfg->enableNsfLink = FALSE;//TRUE;
    chainsCfg->enableOsdAlgLink = FALSE;
    chainsCfg->enableVidFrameExport = TRUE;
    enableGrpx               = ENABLE_GRPX;
    enableNsfLink            = chainsCfg->enableNsfLink;
    enableOsdAlgLink            = chainsCfg->enableOsdAlgLink;
    enableVidFrameExport     = chainsCfg->enableVidFrameExport;

    memcpy(displayRes,chainsCfg->displayRes,sizeof(displayRes));
    numSubChains             = 2;
    deiOutQue                = DEI_LINK_OUT_QUE_VIP_SC;
    enableSdtv               = TRUE;

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    CaptureLink_CreateParams_Init(&capturePrm);
    capturePrm.enableSdCrop = FALSE;

    capturePrm.numVipInst    = 2*numSubChains;

    /* If OsdAlgLink is enabled, make sure to enable NsfLink as ipcFramesIn and */
    /* out is not supported to operate in multiple out queues required to    */
    /* connect then to DEI.                                                  */
    if(enableOsdAlgLink)
    {
        capturePrm.outQueParams[0].nextLink = ipcFramesOutVpssId;

        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkId = captureId;
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm.baseCreateParams.notifyPrevLink = TRUE;

        if(enableNsfLink)
        {
            ipcFramesOutVpssPrm.baseCreateParams.numOutQue = 1;
            ipcFramesOutVpssPrm.baseCreateParams.outQueParams[0].nextLink = nsfId;
        }
        else
        {
            ipcFramesOutVpssPrm.baseCreateParams.numOutQue = numSubChains;
            ipcFramesOutVpssPrm.baseCreateParams.outQueParams[0].nextLink = deiId[0];
            ipcFramesOutVpssPrm.baseCreateParams.outQueParams[1].nextLink = deiId[1];
        }

        ipcFramesOutVpssPrm.baseCreateParams.notifyNextLink = TRUE;

        ipcFramesOutVpssPrm.baseCreateParams.processLink = ipcFramesInDspId;
        ipcFramesOutVpssPrm.baseCreateParams.notifyProcessLink = FALSE;
        ipcFramesOutVpssPrm.baseCreateParams.noNotifyMode = TRUE;

        //prevLink->processLink->nextLink
        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkId = ipcFramesOutVpssId;
        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm.baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm.baseCreateParams.outQueParams[0].nextLink = osdId;
        ipcFramesInDspPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInDspPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm.baseCreateParams.noNotifyMode              = TRUE;

        osdPrm.inQueParams.prevLinkId = ipcFramesInDspId;
        osdPrm.inQueParams.prevLinkQueId = 0;
    }
    else if(enableNsfLink) {
      capturePrm.outQueParams[0].nextLink = nsfId;
    } else{
      capturePrm.outQueParams[0].nextLink = deiId[0];
      capturePrm.outQueParams[1].nextLink = deiId[1];
    }
    capturePrm.tilerEnable              = FALSE;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = (SYSTEM_CAPTURE_INST_VIP0_PORTA+
                                              vipInstId)%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_TVP5158_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = SYSTEM_STD_MUX_4CH_D1;
        pCaptureInstPrm->numOutput          = 1;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = 0;
        pCaptureOutPrm->scOutHeight         = 0;
        if ((vipInstId >= numSubChains) && !(enableNsfLink || enableOsdAlgLink))
          pCaptureOutPrm->outQueId          = 1;
        else
          pCaptureOutPrm->outQueId          = 0;
    }

    if(enableOsdAlgLink)
    {
        nsfPrm.inQueParams.prevLinkId    = ipcFramesOutVpssId;
        chainsCfg->bypassNsf             = TRUE;
    }
    else
    {
        chainsCfg->bypassNsf             = TRUE;
        nsfPrm.inQueParams.prevLinkId    = captureId;
    }

    nsfPrm.bypassNsf                 = chainsCfg->bypassNsf;
    nsfPrm.tilerEnable               = FALSE;
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.numOutQue                 = numSubChains;
    nsfPrm.outQueParams[0].nextLink  = deiId[0];
    nsfPrm.outQueParams[1].nextLink  = deiId[1];

    for(i=0; i<numSubChains; i++)
    {
        DeiLink_CreateParams_Init(&deiPrm[i]);

        if(enableOsdAlgLink)
        {
            deiPrm[i].inQueParams.prevLinkId = ipcFramesOutVpssId;
        }
        else if(enableNsfLink)
             {
                 deiPrm[i].inQueParams.prevLinkId                      = nsfId;
             }
             else
             {
                 deiPrm[i].inQueParams.prevLinkId                      = captureId;
             }
        deiPrm[i].inQueParams.prevLinkQueId                     = i;
        deiPrm[i].outQueParams[deiOutQue].nextLink              = merge1Id;
        deiPrm[i].outQueParams[deiOutQue^1].nextLink            = nullId;
        deiPrm[i].enableOut[deiOutQue]                          = TRUE;
        deiPrm[i].enableOut[deiOutQue^1]                        = FALSE;
        deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = TRUE;
        deiPrm[i].comprEnable                                   = FALSE;
        deiPrm[i].setVipScYuv422Format                          = FALSE;

        /* DEI Path Scalar ratio is 1:2, D1 => CIF */
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode    = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator    = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator  = 2;

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 2;

        /* VIP Scalar ratio is 1:1 */
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode    = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator    = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator  = 1;

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator = 1;

        {
            int deiQId,deiChId;
            for (deiQId = 0 ; deiQId < DEI_LINK_MAX_OUT_QUE; deiQId++)
            {
                for (deiChId = 1; deiChId < DEI_LINK_MAX_CH;deiChId++)
                {
                    deiPrm[i].outScaleFactor[deiQId][deiChId] =
                      deiPrm[i].outScaleFactor[deiQId][0];
                }
            }
        }
        merge1Prm.numInQue                     = numSubChains;
        merge1Prm.inQueParams[i].prevLinkId    = deiId[i];
        merge1Prm.inQueParams[i].prevLinkQueId = deiOutQue;
        merge1Prm.outQueParams.nextLink        = dup1Id;
        merge1Prm.notifyNextLink               = TRUE;

        dup1Prm.inQueParams.prevLinkId         = merge1Id;
        dup1Prm.inQueParams.prevLinkQueId      = 0;
        dup1Prm.numOutQue                      = numSubChains;
        if (i == 0)
          dup1Prm.outQueParams[i].nextLink     = merge2Id;
        else
          dup1Prm.outQueParams[i].nextLink     = ipcOutVpssId;
        dup1Prm.notifyNextLink                 = TRUE;
    }

    ipcOutVpssPrm.inQueParams.prevLinkId    = dup1Id;
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 1;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = TRUE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.outQueParams[0].nextLink     = encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    for (i=0; i<16; i++) {
        encPrm.chCreateParams[i].format     = IVIDEO_H264HP;
        encPrm.chCreateParams[i].profile    = IH264_HIGH_PROFILE;
        encPrm.chCreateParams[i].dataLayout = IVIDEO_FIELD_SEPARATED;
        encPrm.chCreateParams[i].fieldMergeEncodeEnable  = FALSE;
        encPrm.chCreateParams[i].defaultDynamicParams.intraFrameInterval = 30;
        encPrm.chCreateParams[i].encodingPreset = XDM_DEFAULT;
        encPrm.chCreateParams[i].enableAnalyticinfo = 0;
        encPrm.chCreateParams[i].enableWaterMarking = 0;
        encPrm.chCreateParams[i].rateControlPreset =
                                 IVIDEO_STORAGE;
        encPrm.chCreateParams[i].defaultDynamicParams.inputFrameRate = 30;
        encPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
                                 (2 * 1000 * 1000);
        encPrm.chCreateParams[i].defaultDynamicParams.interFrameInterval = 1;
        encPrm.chCreateParams[i].defaultDynamicParams.mvAccuracy =
                                 IVIDENC2_MOTIONVECTOR_QUARTERPEL;
    }
    encPrm.inQueParams.prevLinkId   = ipcInVideoId;
    encPrm.inQueParams.prevLinkQueId= 0;
    encPrm.outQueParams.nextLink = ipcBitsOutVideoId;

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId = encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink = ipcBitsInHostId;
    Chains_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm,
                                               TRUE);

    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId = ipcBitsOutVideoId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    Chains_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);

    Chains_displayCtrlInit(displayRes);

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

    System_linkCreate (captureId, &capturePrm, sizeof(capturePrm));
    System_linkControl(captureId, CAPTURE_LINK_CMD_CONFIGURE_VIP_DECODERS, NULL, 0, TRUE);

    if(enableOsdAlgLink)
    {
        System_linkCreate(ipcFramesOutVpssId, &ipcFramesOutVpssPrm, sizeof(ipcFramesOutVpssPrm));
        System_linkCreate(ipcFramesInDspId, &ipcFramesInDspPrm, sizeof(ipcFramesInDspPrm));
        System_linkCreate(osdId , &osdPrm, sizeof(osdPrm));
    }

    if(enableNsfLink)
       System_linkCreate(nsfId , &nsfPrm, sizeof(nsfPrm));

    for(i=0; i<numSubChains; i++)
        System_linkCreate(deiId[i]  , &deiPrm[i], sizeof(deiPrm[i]));

    System_linkCreate(merge1Id   , &merge1Prm  , sizeof(merge1Prm));
    System_linkCreate(dup1Id     , &dup1Prm    , sizeof(dup1Prm));

    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );

    System_linkCreate(encId, &encPrm, sizeof(encPrm));

    System_linkCreate(ipcBitsOutVideoId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(ipcBitsInHostId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));


    System_linkGetInfo(ipcBitsInHostId,&bitsProducerLinkInfo);
    OSA_assert(bitsProducerLinkInfo.numQue = 1);
    ipcBitsOutHostPrm.baseCreateParams.numOutQue = 1;
    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = ipcBitsInVideoId;
    Chains_ipcBitsInitCreateParams_BitsOutHLOS(&ipcBitsOutHostPrm,
                                               &bitsProducerLinkInfo.queInfo[0]);
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId = ipcBitsOutHostId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue                = 1;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink = decId;
    Chains_ipcBitsInitCreateParams_BitsInRTOS(&ipcBitsInVideoPrm,
                                              TRUE);

    for (i=0; i<16; i++) {
        decPrm.chCreateParams[i].format          = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile         = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].targetMaxWidth  = 720;
        decPrm.chCreateParams[i].targetMaxHeight = 576;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = FALSE;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = 30;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
                                 (2 * 1000 * 1000);
    }
    decPrm.inQueParams.prevLinkId = ipcBitsInVideoId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcOutVideoId;
    decPrm.tilerEnable = TRUE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                    = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink     = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = FALSE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = TRUE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink     = merge2Id;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = TRUE;

    merge2Prm.numInQue                     = numSubChains;
    merge2Prm.inQueParams[0].prevLinkId    = dup1Id;
    merge2Prm.inQueParams[0].prevLinkQueId = 0;
    merge2Prm.inQueParams[1].prevLinkId    = ipcInVpssId;
    merge2Prm.inQueParams[1].prevLinkQueId = 0;
    merge2Prm.outQueParams.nextLink        = dup2Id;
    merge2Prm.notifyNextLink               = TRUE;

    dup2Prm.inQueParams.prevLinkId         = merge2Id;
    dup2Prm.inQueParams.prevLinkQueId      = 0;
    dup2Prm.numOutQue                      = numSubChains;
    dup2Prm.outQueParams[0].nextLink       = swMsId[0];
    dup2Prm.outQueParams[1].nextLink       = swMsId[1];
    dup2Prm.notifyNextLink                 = TRUE;

    for(i=0; i<numSubChains; i++)
    {
        swMsPrm[i].inQueParams.prevLinkId    = dup2Id;
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        if ((i == 0) && (enableVidFrameExport))
        {
            swMsPrm[i].outQueParams.nextLink     = ipcFramesOutVpssToHost;
            /* Set ipcFramesOutVpss link info */
            ipcFramesOutVpssToHostPrm.baseCreateParams.noNotifyMode = TRUE;
            ipcFramesOutVpssToHostPrm.baseCreateParams.notifyNextLink = FALSE;
            ipcFramesOutVpssToHostPrm.baseCreateParams.notifyPrevLink = TRUE;
            ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkId = swMsId[i];
            ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
            ipcFramesOutVpssToHostPrm.baseCreateParams.outQueParams[0].nextLink = ipcFramesInHost;

            ipcFramesInHostPrm.baseCreateParams.noNotifyMode = TRUE;
            ipcFramesInHostPrm.baseCreateParams.notifyNextLink = FALSE;
            ipcFramesInHostPrm.baseCreateParams.notifyPrevLink = FALSE;
            ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkId = ipcFramesOutVpssToHost;
            ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
            ipcFramesInHostPrm.baseCreateParams.outQueParams[0].nextLink = SYSTEM_LINK_ID_INVALID;
            ipcFramesInHostPrm.exportOnlyPhyAddr = TRUE;
            Chains_ipcFramesInSetCbInfo(&ipcFramesInHostPrm);

            ipcFramesOutHostPrm.baseCreateParams.noNotifyMode = TRUE;
            ipcFramesOutHostPrm.baseCreateParams.notifyNextLink = FALSE;
            ipcFramesOutHostPrm.baseCreateParams.notifyPrevLink = FALSE;
            ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
            ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
            ipcFramesOutHostPrm.baseCreateParams.outQueParams[0].nextLink = ipcFramesInVpssFromHost;

            ipcFramesInVpssFromHostPrm.baseCreateParams.noNotifyMode = TRUE;
            ipcFramesInVpssFromHostPrm.baseCreateParams.notifyNextLink = TRUE;
            ipcFramesInVpssFromHostPrm.baseCreateParams.notifyPrevLink = FALSE;
            ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkId = ipcFramesOutHost;
            ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
            ipcFramesInVpssFromHostPrm.baseCreateParams.outQueParams[0].nextLink = displayId[i];
        }
        else
        {
            swMsPrm[i].outQueParams.nextLink     = displayId[i];
        }
        swMsPrm[i].lineSkipMode              = FALSE;
        swMsPrm[i].layoutPrm.outputFPS       = 30;
        swMsPrm[i].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes                 = displayRes[i];

        Chains_swMsGenerateLayoutParams(i, 0, &swMsPrm[i]);
        if ((i == 0) && (enableVidFrameExport))
        {
            displayPrm[i].inQueParams[0].prevLinkId    = ipcFramesInVpssFromHost;
        }
        else
        {
            displayPrm[i].inQueParams[0].prevLinkId    = swMsId[i];
        }
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                = swMsPrm[i].maxOutRes;
    }

    if(enableSdtv)
    {
        dup2Prm.numOutQue                      = 3;
        dup2Prm.outQueParams[2].nextLink       = displayId[2];
    }

    displayPrm[2].inQueParams[0].prevLinkId    = dup2Id;
    displayPrm[2].inQueParams[0].prevLinkQueId = 2;
    displayPrm[2].displayRes                = VSYS_STD_NTSC;

    System_linkCreate(ipcBitsOutHostId,&ipcBitsOutHostPrm,sizeof(ipcBitsOutHostPrm));
    System_linkCreate(ipcBitsInVideoId,&ipcBitsInVideoPrm,sizeof(ipcBitsInVideoPrm));
    System_linkCreate(decId, &decPrm, sizeof(decPrm));

    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );

    System_linkCreate(merge2Id   , &merge2Prm  , sizeof(merge2Prm));
    System_linkCreate(dup2Id     , &dup2Prm    , sizeof(dup2Prm));

    for(i=0; i<numSubChains; i++)
        System_linkCreate(swMsId[i]  , &swMsPrm[i], sizeof(swMsPrm[i]));

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
    for(i=0; i<numSubChains; i++)
        System_linkCreate(displayId[i], &displayPrm[i], sizeof(displayPrm[i]));

    if(enableSdtv)
    {
        System_linkCreate(displayId[2], &displayPrm[2], sizeof(displayPrm[2]));
    }

#ifdef  SYSTEM_ENABLE_AUDIO
    Audio_captureCreate();
    Audio_playCreate();
    audioPathSetFlag = FALSE;
    audioCaptureActive = FALSE;
    audioPlaybackActive = FALSE;
#endif

    Chains_memPrintHeapStatus();
    //Utils_encdecHdvicpPrfInit();

    {
        if(enableGrpx)
        {
#if USE_FBDEV
            grpx_init(GRPX_FORMAT_RGB888);
#else
            Chains_grpxEnable(grpxId[0], TRUE);
            Chains_grpxEnable(grpxId[1], TRUE);
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
            Chains_grpxEnable(grpxId[2], TRUE);
#endif
#endif
        }
        for(i=0; i<numSubChains; i++)
            System_linkStart(displayId[i]);

        if(enableSdtv)
        {
            System_linkStart(displayId[2]);
        }

        if(enableVidFrameExport)
        {
            System_linkStart(ipcFramesOutVpssToHost);
            System_linkStart(ipcFramesInHost);
            System_linkStart(ipcFramesOutHost);
            System_linkStart(ipcFramesInVpssFromHost);
        }

        for(i=0; i<numSubChains; i++)
            System_linkStart(swMsId[i] );

        System_linkStart(decId);
        System_linkStart(encId);

        for(i=0; i<numSubChains; i++)
            System_linkStart(deiId[i] );

        if(enableNsfLink)
           System_linkStart(nsfId    );

        if(enableOsdAlgLink)
        {
            System_linkStart(ipcFramesOutVpssId);
            System_linkStart(ipcFramesInDspId);
            System_linkStart(osdId);
        }

        System_linkStart(captureId);

        //OSA_waitMsecs(10000);
        System_linkStart(ipcBitsOutHostId);

        /* Start taking CPU load just before starting of links */
        Chains_prfLoadCalcEnable(TRUE, FALSE, FALSE);



        if(enableGrpx)
        {
#if USE_FBDEV
            grpx_draw();
#endif
        }

        while(1)
        {
#ifdef  SYSTEM_ENABLE_AUDIO
INPUT:
#endif
            ch = Chains_menuRunTime();

            switchLayout = FALSE;
            switchCh = FALSE;

#ifdef  SYSTEM_ENABLE_AUDIO
            if(ch=='a')
            {
                while (1)
                {
                    ch = Chains_audioMenuRunTime();
                    if (ch == 'f')
                    {
                        char inputStr[256];

                        if (audioPathSetFlag == TRUE)
                        {
                            printf ("Audio storage path set already, setting to a new path............\n");
                        }
                        Chains_AudioStorageInputRunTime(inputStr);
                        if (Audio_setStoragePath(inputStr) == AUDIO_STATUS_OK)
                        {
                            audioPathSetFlag = TRUE;
                        }
                        else
                        {
                            audioPathSetFlag = FALSE;
                        }
                    }

                    if (audioPathSetFlag == TRUE)
                    {
                        if (ch == 's')
                        {
                            audioCaptureChNum = Chains_AudioCaptureInputRunTime(audioCaptureActive, AUDIO_MAX_CHANNELS);
                            audioCaptureChNum--;    /* Audio ch num is 0 ~ 3 */
                            if (audioCaptureActive == TRUE)
                            {
                                Audio_captureStop();
                            }
                            if (audioPlaybackActive == TRUE && audioCaptureChNum == audioPlaybackChNum)
                            {
                                printf ("\r\n Playback active on same channel... stopping playback...");
                                Audio_playStop();
                                audioPlaybackActive = FALSE;
                            }
                            Audio_captureStart(audioCaptureChNum);
                            audioCaptureActive = TRUE;
                        }

                        if (ch == 't')
                        {
                            Audio_captureStop();
                            printf ("\r\n Audio Capture stopped....\n");
                            audioCaptureActive = FALSE;
                        }

                        if(ch=='p')
                        {
                            audioPlaybackChNum = Chains_AudioPlaybackInputRunTime(audioPlaybackActive, AUDIO_MAX_CHANNELS);
                            audioPlaybackChNum--;   /* Audio ch num is 0 ~ 3 */
                            if (audioPlaybackActive == TRUE)
                            {
                                Audio_playStop();
                            }
                            if (audioCaptureActive == TRUE && audioCaptureChNum == audioPlaybackChNum)
                            {
                                printf ("\r\n Capture active on same channel... stopping capture...");
                                Audio_captureStop();
                                audioCaptureActive = FALSE;
                            }
                            Audio_playStart(audioPlaybackChNum,0);
                            audioPlaybackActive = TRUE;
                        }

                        if (ch == 'b')
                        {
                            Audio_playStop();
                            printf ("\r\n Audio Playback stopped....\n");
                            audioPlaybackActive = FALSE;
                        }

                        if (ch == 'd')
                        {
                            Audio_capturePrintStats();
                            Audio_playPrintStats();
                        }
                    }
                    else
                    {
                        printf ("\n\n Invalid OPERATION !!!!! - Set Valid Storage Path to enable capture / playback...\n");
                    }
                    if(ch=='0')
                        break;
                }
                goto INPUT;
            }
#endif
            if(ch=='0')
                break;
            if(ch=='p')
                System_linkControl(captureId,
                       CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
            if(ch=='s')
               switchLayout = TRUE;
            if(ch=='c')
               switchCh = TRUE;
            if(ch=='b')
            {
                EncLink_ChBitRateParams params = { 0 };
                params.chId = Chains_ChanInputRunTime();
                if(params.chId != 128 )
                {
                    printf("\r\n Channel Selected: %d", params.chId);

                    /* New bitrate value */
                    params.targetBitRate = Chains_BitRateInputRunTime();
                    if(params.targetBitRate != 0)
                        System_linkControl(encId, ENC_LINK_CMD_SET_CODEC_BITRATE,
                                           &params, sizeof(params), TRUE);
                }
            }
            if(ch=='f')
            {
                EncLink_ChFpsParams params = { 0 };
                params.chId = Chains_ChanInputRunTime();
                if(params.chId != 128 )
                {
                    printf("\r\n Channel Selected: %d", params.chId);
                    /* New fps vaule in fps x 1000 formate */
                    params.targetFps = (1000 * Chains_FrameRateInputRunTime());
                    if(params.targetFps != 0)
                        System_linkControl(encId, ENC_LINK_CMD_SET_CODEC_FPS,
                                          &params, sizeof(params), TRUE);
                }
            }
            if(ch=='r')
            {
                EncLink_ChIntraFrIntParams params = { 0 };
                params.chId = Chains_ChanInputRunTime();
                if(params.chId != 128 )
                {
                    printf("\r\n Channel Selected: %d", params.chId);
                    /* new GOP value */
                    params.intraFrameInterval = Chains_IntraFrRateInputRunTime();
                    if(params.intraFrameInterval != 0)
                        System_linkControl(encId, ENC_LINK_CMD_SET_CODEC_INTRAI,
                                   &params, sizeof(params), TRUE);
                }
            }
            if(ch=='t')
            {
                EncLink_ChForceIFrParams params = { 0 };
                params.chId = Chains_ChanInputRunTime();
                if(params.chId != 128 )
                {
                    printf("\r\nForce IDR on Channel: %d", params.chId);
                    System_linkControl(encId, ENC_LINK_CMD_SET_CODEC_FORCEI,
                                       &params, sizeof(params), TRUE);
                }
            }
            if(ch=='g')
            {
                EncLink_GetDynParams params = { 0 };
                params.chId = Chains_ChanInputRunTime();
                if(params.chId != 128 )
                {
                    printf("\r\nGet Dynamic params of Channel: %d", params.chId);
                    System_linkControl(encId, ENC_LINK_CMD_GET_CODEC_PARAMS,
                                       &params, sizeof(params), TRUE);
                    printf("\r\n Height: %d\n Width: %d\n Framerate: %d\n",
                    params.inputHeight/**< Input frame height. */,
                    params.inputWidth/**< Input frame width. */,
                    params.targetFps);

                    printf(" Bitrate: %d\n Intra-Framerate: %d \n",
                    params.targetBitRate,
                    params.intraFrameInterval);
                }
            }
            if(ch=='i')
            {
                System_linkControl(encId,
                       ENC_LINK_CMD_PRINT_IVAHD_STATISTICS, NULL, 0, TRUE);
            }
            if(ch=='m')
            {
                printf(" STATISTICS for SwMs with linkId: %d \n", swMsId[0]);
                System_linkControl(swMsId[0],
                       SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS, NULL, 0, TRUE);
                printf(" STATISTICS for SwMs with linkId: %d \n", swMsId[1]);
                System_linkControl(swMsId[1],
                       SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS, NULL, 0, TRUE);
            }
            Chains_swMsSwitchLayout(swMsId, swMsPrm, switchLayout, switchCh, 2);
        }

        System_linkStop(captureId);

        if(enableOsdAlgLink)
        {
            System_linkStop(ipcFramesOutVpssId);
            System_linkStop(ipcFramesInDspId);
            System_linkStop(osdId);
        }

        if(enableNsfLink)
           System_linkStop(nsfId    );

        for(i=0; i<numSubChains; i++)
            System_linkStop(deiId[i] );

        System_linkStop(encId);
        Chains_ipcBitsStop();
        System_linkStop(ipcBitsOutHostId);
        System_linkStop(ipcBitsInVideoId);
        System_linkStop(decId);

        for(i=0; i<numSubChains; i++)
            System_linkStop(swMsId[i] );

        if(enableVidFrameExport)
        {
            System_linkStop(ipcFramesOutVpssToHost);
            System_linkStop(ipcFramesInHost);
            System_linkStop(ipcFramesOutHost);
            System_linkStop(ipcFramesInVpssFromHost);
        }

        for(i=0; i<numSubChains; i++)
            System_linkStop(displayId[i]);

        if(enableSdtv)
        {
            System_linkStop(displayId[2]);
        }

#ifdef  SYSTEM_ENABLE_AUDIO
            if (audioCaptureActive == TRUE)
            {
                Audio_captureStop();
            }
            if (audioPlaybackActive == TRUE)
            {
                Audio_playStop();
            }
#endif

        if(enableGrpx)
        {
#if USE_FBDEV
            grpx_exit();
#else
            Chains_grpxEnable(grpxId[0], FALSE);
            Chains_grpxEnable(grpxId[1], FALSE);
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
            Chains_grpxEnable(grpxId[2], FALSE);
#endif
#endif
        }
    }

    System_linkDelete(captureId);

    if(enableOsdAlgLink)
    {
        System_linkDelete(ipcFramesOutVpssId);
        System_linkDelete(ipcFramesInDspId);
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

    if(enableNsfLink)
       System_linkDelete(nsfId    );

    for(i=0; i<numSubChains; i++)
        System_linkDelete(deiId[i] );

    System_linkDelete(merge1Id);
    System_linkDelete(dup1Id);

    System_linkDelete(ipcOutVpssId );
    System_linkDelete(ipcInVideoId );

    System_linkDelete(encId);
    System_linkDelete(ipcBitsOutVideoId);
    System_linkDelete(ipcBitsInHostId);
    System_linkDelete(ipcBitsOutHostId);
    System_linkDelete(ipcBitsInVideoId);
    System_linkDelete(decId);

    System_linkDelete(ipcOutVideoId);
    System_linkDelete(ipcInVpssId  );

    System_linkDelete(merge2Id);
    System_linkDelete(dup2Id);

    for(i=0; i<numSubChains; i++)
        System_linkDelete(swMsId[i] );

    if(enableVidFrameExport)
    {
        System_linkDelete(ipcFramesOutVpssToHost);
        System_linkDelete(ipcFramesInHost);
        System_linkDelete(ipcFramesOutHost);
        System_linkDelete(ipcFramesInVpssFromHost);
    }

    for(i=0; i<numSubChains; i++)
        System_linkDelete(displayId[i]);

    if(enableSdtv)
    {
        System_linkDelete(displayId[2]);
    }
#ifdef  SYSTEM_ENABLE_AUDIO
     Audio_captureDelete();
     Audio_playDelete();
#endif

    Chains_displayCtrlDeInit();

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    Chains_prfLoadCalcEnable(FALSE, TRUE, FALSE);
    //Utils_encdecHdvicpPrfPrint();

    Chains_ipcBitsExit();
    Chains_ipcFramesExit();

}


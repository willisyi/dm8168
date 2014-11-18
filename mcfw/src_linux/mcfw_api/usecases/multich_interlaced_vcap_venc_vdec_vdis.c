/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
                        Capture (YUV422I) 16CH D1 60fps
                          |
                          |
       ----------------dup0--------------
       |          |                      |
       |    NSF (16 D1 YUV420SP)     SCLRVIP(16 CIF)
       |          |                      |
       |          |                      |
       |          ------    -------------
       |                |  |
       |                |  |
       |                MERGE0 16D1 + 16CIF
       |                   |
       |                   |
       |                   |           /-IPCFRAMEINDSPIN----+
       |               IPCFRAMEOUT(VPS)                     |
       |                   |           \SYSTEM_LINK_ID_OSD_0
       |                   |               DSP(SWOSD - YUV420SP)
       |                   |
       |              IPCM3OUT(VPS)-- IPCM3IN(VID)--ENC--------|
       |                                                       |
       -------------------| |----IPCM3IN(VPS)--IPCM3OUT(VID)--DEC
                          | |
                        MERGE1
                          |
                         dup1
                         ||
         +---------------+|
         |                |
         |                |
      SW Mosaic       SW Mosaic
      (DEIH YUV422I)  (DEI YUV422I)
         |                |
 GRPX0   |       GRPX1,2  |
    |    |           |    |
    On-Chip HDMI    Off-Chip HDMI
      1080p60         1080p60
*/


#include "multich_common.h"
#include "multich_ipcbits.h"


#define     NUM_CAPTURE_DEVICES          (4)

/* =============================================================================
 * Externs
 * =============================================================================
 */


/* =============================================================================
 * Use case code
 * =============================================================================
 */
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

Void MultiCh_createInterlacedVcapVencVdecVdis()
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
    DupLink_CreateParams        dup1Prm;
    DupLink_CreateParams        dup0Prm;
    static SwMsLink_CreateParams       swMsPrm[VDIS_DEV_MAX];
    DisplayLink_CreateParams    displayPrm[VDIS_DEV_MAX];
    IpcLink_CreateParams        ipcOutVpssPrm;
    IpcLink_CreateParams        ipcInVpssPrm;
    IpcLink_CreateParams        ipcOutVideoPrm;
    IpcLink_CreateParams        ipcInVideoPrm;
    EncLink_CreateParams        encPrm;
    DecLink_CreateParams        decPrm;
    MergeLink_CreateParams      merge0Prm;
    MergeLink_CreateParams      merge1Prm;
    SclrLink_CreateParams       sclrPrm;

    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm;
    IpcFramesOutLinkRTOS_CreateParams ipcFramesOutVpssPrm;
    AlgLink_CreateParams              dspAlgPrm;


    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams    ipcBitsInHostPrm;
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    System_LinkInfo                   bitsProducerLinkInfo;

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

    UInt32 dup1Id,dup0Id, merge1Id,merge0Id;
    UInt32 grpxId[VDIS_DEV_MAX];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    /**********debug use only******************************/
    Bool   enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    UInt32 vipInstId;
    UInt32 i;

    UInt32 numDisplay;

    Bool enabledup1;

    printf("\n\nMCFW Interlaced demo\n\n");

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(DecLink_CreateParams, decPrm);
    MULTICH_INIT_STRUCT(EncLink_CreateParams, encPrm);
    MULTICH_INIT_STRUCT(SclrLink_CreateParams, sclrPrm);

    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm);
    MULTICH_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm);

    for (i = 0; i < VDIS_DEV_MAX; i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams  ,
                            displayPrm[i]);

        MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[i]);
    }
    MultiCh_detectBoard();

    System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES,
        NULL,
        0,
        TRUE
        );

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;



    if(enableOsdAlgLink)
    {
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_ALG_0;
        gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
        gVcapModuleContext.ipcFramesInDspId[0] = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
    }

    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]    = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]    = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    gVdisModuleContext.swMsId[2]    = SYSTEM_LINK_ID_SW_MS_MULTI_INST_2;



    swMsPrm[0].numSwMsInst          = 1;
    swMsPrm[0].swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_DEIHQ_SC;

    swMsPrm[1].numSwMsInst          = 1;
    swMsPrm[1].swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_DEI_SC;

    swMsPrm[2].numSwMsInst          = 1;
    swMsPrm[2].swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_SC5;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI
    gVdisModuleContext.displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; // SDTV

    dup0Id       = SYSTEM_VPSS_LINK_ID_DUP_0;
    dup1Id       = SYSTEM_VPSS_LINK_ID_DUP_1;
    merge0Id     = SYSTEM_VPSS_LINK_ID_MERGE_0;
    merge1Id     = SYSTEM_VPSS_LINK_ID_MERGE_1;

    gVcapModuleContext.sclrId[0]       = SYSTEM_LINK_ID_SCLR_INST_0;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    grpxId[0]    = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]    = SYSTEM_LINK_ID_GRPX_1;
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
    grpxId[2]    = SYSTEM_LINK_ID_GRPX_2;
#endif

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId   = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

    numDisplay     = gVsysModuleContext.vsysConfig.numDisplays;

    enabledup1 = FALSE;
    if(numDisplay>1)
        enabledup1 = TRUE;

    CaptureLink_CreateParams_Init(&capturePrm);

    capturePrm.numVipInst    = 4;
    capturePrm.outQueParams[0].nextLink = dup0Id;
    capturePrm.tilerEnable              = FALSE;
    capturePrm.numBufsPerCh             = 12;

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
        pCaptureOutPrm->outQueId            = 0;
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

    Vcap_configVideoDecoder(vidDecVideoModeArgs, NUM_CAPTURE_DEVICES);



    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        dup0Prm.numOutQue = 3;
    }
    else
    {
        dup0Prm.numOutQue = 2;
    }

    dup0Prm.notifyNextLink = TRUE;
    dup0Prm.outQueParams[0].nextLink = merge1Id;
    dup0Prm.outQueParams[1].nextLink = gVcapModuleContext.nsfId[0];
    dup0Prm.outQueParams[2].nextLink = gVcapModuleContext.sclrId[0];
    dup0Prm.inQueParams.prevLinkId    = gVcapModuleContext.captureId;
    dup0Prm.inQueParams.prevLinkQueId = 0;

    NsfLink_CreateParams_Init(&nsfPrm);
    nsfPrm.bypassNsf                 = TRUE;
    nsfPrm.tilerEnable               = FALSE;
    nsfPrm.inQueParams.prevLinkId    = dup0Id;
    nsfPrm.inQueParams.prevLinkQueId = 1;
    nsfPrm.numOutQue                 = 1;
    nsfPrm.outQueParams[0].nextLink  = merge0Id;

    SclrLink_CreateParams_Init(&sclrPrm);
    sclrPrm.inQueParams.prevLinkId             = dup0Id;
    sclrPrm.inQueParams.prevLinkQueId          = 2;
    sclrPrm.outQueParams.nextLink              = merge0Id;
    sclrPrm.tilerEnable                        = FALSE;
    sclrPrm.enableLineSkipSc                   = FALSE;
    sclrPrm.inputFrameRate                     = 30;
    sclrPrm.outputFrameRate                    = 15;
    sclrPrm.scaleMode                          = DEI_SCALE_MODE_RATIO;
    sclrPrm.outScaleFactor.ratio.widthRatio.numerator = 1;
    sclrPrm.outScaleFactor.ratio.widthRatio.denominator = 2;
    sclrPrm.outScaleFactor.ratio.heightRatio.numerator = 1;
    sclrPrm.outScaleFactor.ratio.heightRatio.denominator = 2;
    sclrPrm.outDataFormat = VF_YUV420SP_UV;
    sclrPrm.pathId = SCLR_LINK_SEC0_SC3;


    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        merge0Prm.numInQue                     = 2;
    }
    else
    {
        merge0Prm.numInQue                     = 1;
    }

    merge0Prm.inQueParams[0].prevLinkId    = gVcapModuleContext.nsfId[0];
    merge0Prm.inQueParams[0].prevLinkQueId = 0;
    merge0Prm.inQueParams[1].prevLinkId    = gVcapModuleContext.sclrId[0];
    merge0Prm.inQueParams[1].prevLinkQueId = 0;
    merge0Prm.notifyNextLink               = TRUE;
    merge0Prm.outQueParams.nextLink        = ipcOutVpssId;



    ipcOutVpssPrm.inQueParams.prevLinkId    = merge0Id;
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = TRUE;



    if(enableOsdAlgLink)
    {
        merge0Prm.outQueParams.nextLink        = gVcapModuleContext.ipcFramesOutVpssId[0];
        ipcOutVpssPrm.inQueParams.prevLinkId    = gVcapModuleContext.ipcFramesOutVpssId[0];

        /* Redirect to DSP for OSD / SCD */
        ipcFramesOutVpssPrm .baseCreateParams.inQueParams.prevLinkId   = merge0Id;
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm.baseCreateParams.outQueParams[0].nextLink  = ipcOutVpssId;
        ipcFramesOutVpssPrm.baseCreateParams.processLink               = gVcapModuleContext.ipcFramesInDspId[0];
        ipcFramesOutVpssPrm.baseCreateParams.notifyPrevLink            = TRUE;
        ipcFramesOutVpssPrm.baseCreateParams.notifyNextLink            = TRUE;
        ipcFramesOutVpssPrm.baseCreateParams.notifyProcessLink         = TRUE;
        ipcFramesOutVpssPrm.baseCreateParams.noNotifyMode              = FALSE;
        ipcFramesOutVpssPrm.baseCreateParams.numOutQue                 = 1;

        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkId      = gVcapModuleContext.ipcFramesOutVpssId[0];
        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
        ipcFramesInDspPrm.baseCreateParams.outQueParams[0].nextLink    = gVcapModuleContext.dspAlgId[0];
        ipcFramesInDspPrm.baseCreateParams.notifyPrevLink              = TRUE;
        ipcFramesInDspPrm.baseCreateParams.notifyNextLink              = TRUE;
        ipcFramesInDspPrm.baseCreateParams.noNotifyMode                = FALSE;
        ipcFramesInDspPrm.baseCreateParams.numOutQue                   = 1;

        dspAlgPrm.enableOSDAlg = TRUE;
        dspAlgPrm.enableSCDAlg = FALSE;
        dspAlgPrm.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
        dspAlgPrm.inQueParams.prevLinkQueId = 0;
    }


    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                    = 1;
    ipcInVideoPrm.outQueParams[0].nextLink     = gVencModuleContext.encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    {
        EncLink_ChCreateParams *pLinkChPrm;
        EncLink_ChDynamicParams *pLinkDynPrm;
        VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
        VENC_CHN_PARAMS_S *pChPrm;

        /* Primary Stream Params - D1 */
        for (i=0; i<gVencModuleContext.vencConfig.numPrimaryChn; i++)
        {
            pLinkChPrm  = &encPrm.chCreateParams[i];
            pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

            pChPrm      = &gVencModuleContext.vencConfig.encChannelParams[i];
            pDynPrm     = &pChPrm->dynamicParam;

            pLinkChPrm->format                  = IVIDEO_H264HP;
            pLinkChPrm->profile                 = gVencModuleContext.vencConfig.h264Profile[i];
            pLinkChPrm->dataLayout              = IVIDEO_FIELD_SEPARATED;
            pLinkChPrm->fieldMergeEncodeEnable  = TRUE;
            pLinkChPrm->enableAnalyticinfo      = pChPrm->enableAnalyticinfo;
            pLinkChPrm->enableWaterMarking      = pChPrm->enableWaterMarking;
            pLinkChPrm->maxBitRate              = pChPrm->maxBitRate;
            pLinkChPrm->encodingPreset          = pChPrm->encodingPreset;
            pLinkChPrm->rateControlPreset       = pChPrm->rcType;
            pLinkChPrm->enableSVCExtensionFlag  = pChPrm->enableSVCExtensionFlag;
            pLinkChPrm->numTemporalLayer        = pChPrm->numTemporalLayer;

            pLinkDynPrm->intraFrameInterval     = pDynPrm->intraFrameInterval;
            pLinkDynPrm->targetBitRate          = pDynPrm->targetBitRate;
            pLinkDynPrm->interFrameInterval     = 1;
            pLinkDynPrm->mvAccuracy             = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
            pLinkDynPrm->inputFrameRate         = pDynPrm->inputFrameRate;
            pLinkDynPrm->rcAlg                  = pDynPrm->rcAlg;
            pLinkDynPrm->qpMin                  = pDynPrm->qpMin;
            pLinkDynPrm->qpMax                  = pDynPrm->qpMax;
            pLinkDynPrm->qpInit                 = pDynPrm->qpInit;
            pLinkDynPrm->vbrDuration            = pDynPrm->vbrDuration;
            pLinkDynPrm->vbrSensitivity         = pDynPrm->vbrSensitivity;
        }
        if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
        {
            for (i=gVencModuleContext.vencConfig.numPrimaryChn;
                  i<(gVencModuleContext.vencConfig.numPrimaryChn
                           + gVencModuleContext.vencConfig.numSecondaryChn);
                    i++)
            {
                pLinkChPrm  = &encPrm.chCreateParams[i];
                pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

                pChPrm      = &gVencModuleContext.vencConfig.encChannelParams[i];
                pDynPrm     = &pChPrm->dynamicParam;

                pLinkChPrm->format                  = IVIDEO_H264HP;
                pLinkChPrm->profile                 = gVencModuleContext.vencConfig.h264Profile[i];
                pLinkChPrm->dataLayout              = IVIDEO_FIELD_SEPARATED;
                pLinkChPrm->fieldMergeEncodeEnable  = FALSE;
                pLinkChPrm->enableAnalyticinfo      = pChPrm->enableAnalyticinfo;
                pLinkChPrm->enableWaterMarking      = pChPrm->enableWaterMarking;
                pLinkChPrm->maxBitRate              = pChPrm->maxBitRate;
                pLinkChPrm->encodingPreset          = pChPrm->encodingPreset;
                pLinkChPrm->rateControlPreset       = pChPrm->rcType;
                pLinkChPrm->enableSVCExtensionFlag  = pChPrm->enableSVCExtensionFlag;
                pLinkChPrm->numTemporalLayer        = pChPrm->numTemporalLayer;

                pLinkDynPrm->intraFrameInterval     = pDynPrm->intraFrameInterval;
                pLinkDynPrm->targetBitRate          = pDynPrm->targetBitRate;
                pLinkDynPrm->interFrameInterval     = 1;
                pLinkDynPrm->mvAccuracy             = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
                pLinkDynPrm->inputFrameRate         = pDynPrm->inputFrameRate;
                pLinkDynPrm->rcAlg                  = pDynPrm->rcAlg;
                pLinkDynPrm->qpMin                  = pDynPrm->qpMin;
                pLinkDynPrm->qpMax                  = pDynPrm->qpMax;
                pLinkDynPrm->qpInit                 = pDynPrm->qpInit;
                pLinkDynPrm->vbrDuration            = pDynPrm->vbrDuration;
                pLinkDynPrm->vbrSensitivity         = pDynPrm->vbrSensitivity;
            }
        }

        encPrm.inQueParams.prevLinkId   = ipcInVideoId;
        encPrm.inQueParams.prevLinkQueId= 0;
        encPrm.outQueParams.nextLink = gVencModuleContext.ipcBitsOutRTOSId;
    }


    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId      = gVencModuleContext.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink       = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm, TRUE);

    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.ipcBitsOutRTOSId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);

#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif

    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));

    if(enableOsdAlgLink)
    {
        int chId;

        for(chId = 0; chId < ALG_LINK_OSD_MAX_CH; chId++)
        {
            AlgLink_OsdChWinParams * chWinPrm = &dspAlgPrm.osdChCreateParams[chId].chDefaultParams;

            /* set osd window max width and height */
            dspAlgPrm.osdChCreateParams[chId].maxWidth  = EXAMPLE_OSD_WIN_MAX_WIDTH;
            dspAlgPrm.osdChCreateParams[chId].maxHeight = EXAMPLE_OSD_WIN_MAX_HEIGHT;

            chWinPrm->chId = chId;
            chWinPrm->numWindows = 0;
        }
    }

    System_linkCreate(dup0Id , &dup0Prm, sizeof(dup0Prm));
    System_linkCreate(gVcapModuleContext.nsfId[0] , &nsfPrm, sizeof(nsfPrm));



    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        System_linkCreate(gVcapModuleContext.sclrId[0], &sclrPrm, sizeof(sclrPrm));
    }

    System_linkCreate(merge0Id, &merge0Prm, sizeof(merge0Prm));

    if(enableOsdAlgLink)
    {
        System_linkCreate(gVcapModuleContext.ipcFramesOutVpssId[0], &ipcFramesOutVpssPrm, sizeof(ipcFramesOutVpssPrm));
        System_linkCreate(gVcapModuleContext.ipcFramesInDspId[0], &ipcFramesInDspPrm, sizeof(ipcFramesInDspPrm));
        System_linkCreate(gVcapModuleContext.dspAlgId[0], &dspAlgPrm, sizeof(dspAlgPrm));
    }

    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );

    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );
    System_linkCreate(gVencModuleContext.encId, &encPrm, sizeof(encPrm));


    System_linkCreate(gVencModuleContext.ipcBitsOutRTOSId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(gVencModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));

    System_linkGetInfo(gVencModuleContext.ipcBitsInHLOSId,&bitsProducerLinkInfo);
    OSA_assert(bitsProducerLinkInfo.numQue = 1);
    ipcBitsOutHostPrm.baseCreateParams.numOutQue = 1;
    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.ipcBitsInRTOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutHLOS(&ipcBitsOutHostPrm, &bitsProducerLinkInfo.queInfo[0]);

    if(gVdecModuleContext.vdecConfig.forceUseDecChannelParams)
    {
        /* use channel info provided by user instead of from encoder */
        UInt32 chId;
        System_LinkChInfo *pChInfo;

        ipcBitsOutHostPrm.inQueInfo.numCh = gVdecModuleContext.vdecConfig.numChn;
        for(chId=0; chId<ipcBitsOutHostPrm.inQueInfo.numCh; chId++)
        {
            pChInfo = &ipcBitsOutHostPrm.inQueInfo.chInfo[chId];

            pChInfo->bufType        = 0; // NOT USED
            pChInfo->codingformat   = 0; // NOT USED
            pChInfo->dataFormat     = 0; // NOT USED
            pChInfo->memType        = 0; // NOT USED
            pChInfo->startX         = 0; // NOT USED
            pChInfo->startY         = 0; // NOT USED
            pChInfo->width          = gVdecModuleContext.vdecConfig.decChannelParams[chId].maxVideoWidth;
            pChInfo->height         = gVdecModuleContext.vdecConfig.decChannelParams[chId].maxVideoHeight;
            pChInfo->pitch[0]       = 0; // NOT USED
            pChInfo->pitch[1]       = 0; // NOT USED
            pChInfo->pitch[2]       = 0; // NOT USED
            pChInfo->scanFormat     = SYSTEM_SF_PROGRESSIVE;
        }
    }

    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsOutHLOSId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId    = 0;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue = 1;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink        = gVdecModuleContext.decId;
    MultiCh_ipcBitsInitCreateParams_BitsInRTOS(&ipcBitsInVideoPrm, TRUE);

    for (i=0; i<gVdecModuleContext.vdecConfig.numChn; i++) {
        decPrm.chCreateParams[i].format          = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile         = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].targetMaxWidth  = ipcBitsOutHostPrm.inQueInfo.chInfo[i].width;
        decPrm.chCreateParams[i].targetMaxHeight = ipcBitsOutHostPrm.inQueInfo.chInfo[i].height;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = TRUE;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = 30;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate = (2 * 1000 * 1000);
    }




    decPrm.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsInRTOSId;

    decPrm.inQueParams.prevLinkQueId    = 0;
    decPrm.outQueParams.nextLink     = ipcOutVideoId;
    decPrm.tilerEnable = TRUE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink     = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = FALSE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = TRUE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue    = 1;
    ipcInVpssPrm.outQueParams[0].nextLink     = merge1Id ;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = TRUE;

    merge1Prm.numInQue                     = 2;
    merge1Prm.inQueParams[0].prevLinkId    = dup0Id;
    merge1Prm.inQueParams[0].prevLinkQueId = 0;
    merge1Prm.inQueParams[1].prevLinkId    = ipcInVpssId;
    merge1Prm.inQueParams[1].prevLinkQueId = 0;
    merge1Prm.notifyNextLink               = TRUE;

    if(enabledup1)
       merge1Prm.outQueParams.nextLink        = dup1Id;
    else
       merge1Prm.outQueParams.nextLink        = gVdisModuleContext.swMsId[0];

    dup1Prm.inQueParams.prevLinkId      = merge1Id ;
    dup1Prm.inQueParams.prevLinkQueId   = 0;
    dup1Prm.numOutQue                   = numDisplay;
    dup1Prm.notifyNextLink              = TRUE;


    for(i=0; i<numDisplay; i++)
    {
        if(enabledup1)
            swMsPrm[i].inQueParams.prevLinkId = dup1Id;
        else
            swMsPrm[i].inQueParams.prevLinkId = merge1Id ;

        swMsPrm[i].inQueParams.prevLinkQueId    = i;
        swMsPrm[i].outQueParams.nextLink        = gVdisModuleContext.displayId[i];
        swMsPrm[i].lineSkipMode = TRUE;
        swMsPrm[i].maxInputQueLen               = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes                    = VSYS_STD_1080P_60;
        swMsPrm[i].initOutRes                   = gVdisModuleContext.vdisConfig.deviceParams[i].resolution;

        /* SDTV is @ 30fps */
        if(i==2) {
            swMsPrm[i].lineSkipMode = TRUE;
            swMsPrm[i].maxOutRes    = VSYS_STD_PAL;
	        swMsPrm[i].initOutRes   = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
        }

        swMsPrm[i].enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(i, &swMsPrm[i], TRUE);

        dup1Prm.outQueParams[i].nextLink = gVdisModuleContext.swMsId[i];

        displayPrm[i].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        if (i==2)
        {
            displayPrm[i].numInputQueues = 2;
            displayPrm[i].inQueParams[1].prevLinkId    = dup0Id;
            displayPrm[i].inQueParams[1].prevLinkQueId = 2;
            displayPrm[i].activeQueue = 0;
        }
        displayPrm[i].displayRes   = swMsPrm[i].initOutRes;
        if (i == 2)
            displayPrm[i].displayRes = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
    }

    System_linkCreate(gVdecModuleContext.ipcBitsOutHLOSId,&ipcBitsOutHostPrm,sizeof(ipcBitsOutHostPrm));
    System_linkCreate(gVdecModuleContext.ipcBitsInRTOSId,&ipcBitsInVideoPrm,sizeof(ipcBitsInVideoPrm));

    System_linkCreate(gVdecModuleContext.decId, &decPrm, sizeof(decPrm));
    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(merge1Id    , &merge1Prm  , sizeof(merge1Prm));

    if(enabledup1)
        System_linkCreate(dup1Id     , &dup1Prm, sizeof(dup1Prm));

    for(i=0; i<numDisplay; i++)
    {
        System_linkCreate(gVdisModuleContext.swMsId[i], &swMsPrm[i], sizeof(swMsPrm[i]));
        System_linkCreate(gVdisModuleContext.displayId[i], &displayPrm[i], sizeof(displayPrm[i]));
    }
    {
        MergeLink_InLinkChInfo inChInfo;

        MergeLink_InLinkChInfo_Init(&inChInfo);
        inChInfo.inLinkID = ipcInVpssId;
        System_linkControl(merge1Id,
                           MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO,
                           &inChInfo,
                           sizeof(inChInfo),
                           TRUE);
        OSA_assert(inChInfo.numCh == gVdecModuleContext.vdecConfig.numChn);
        MultiCh_setDec2DispMap(VDIS_DEV_HDMI,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
        MultiCh_setDec2DispMap(VDIS_DEV_HDCOMP,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
   }


}


Void MultiCh_deleteInterlacedVcapVencVdecVdis()
{
    UInt32 i;
    UInt32 numDisplay   = gVsysModuleContext.vsysConfig.numDisplays;;
    UInt32 grpxId[VDIS_DEV_MAX];

    UInt32 dup1Id, dup0Id, merge0Id,merge1Id;
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    dup0Id       = SYSTEM_VPSS_LINK_ID_DUP_0;
    dup1Id       = SYSTEM_VPSS_LINK_ID_DUP_1;
    merge0Id     = SYSTEM_VPSS_LINK_ID_MERGE_0;
    merge1Id     = SYSTEM_VPSS_LINK_ID_MERGE_1;
    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    grpxId[0]    = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]    = SYSTEM_LINK_ID_GRPX_1;
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
    grpxId[2]    = SYSTEM_LINK_ID_GRPX_2;
#endif

    System_linkDelete(gVcapModuleContext.captureId);

    System_linkDelete(dup0Id);

    if(gVsysModuleContext.vsysConfig.enableNsf)
       System_linkDelete(gVcapModuleContext.nsfId[0]);

    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        System_linkDelete(gVcapModuleContext.sclrId[0]);
    }

    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        System_linkDelete(merge0Id);
    }

    System_linkDelete(ipcOutVpssId );
    if(gVsysModuleContext.vsysConfig.enableOsd)
    {
        System_linkDelete(gVcapModuleContext.ipcFramesOutVpssId[0]);
        System_linkDelete(gVcapModuleContext.ipcFramesInDspId[0]);
        System_linkDelete(gVcapModuleContext.dspAlgId[0]);
    }
    System_linkDelete(ipcInVideoId );

    System_linkDelete(gVencModuleContext.encId);
    System_linkDelete(gVencModuleContext.ipcBitsOutRTOSId);
    System_linkDelete(gVencModuleContext.ipcBitsInHLOSId);
    System_linkDelete(gVdecModuleContext.ipcBitsOutHLOSId);
    System_linkDelete(gVdecModuleContext.ipcBitsInRTOSId);
    System_linkDelete(gVdecModuleContext.decId);

    System_linkDelete(ipcOutVideoId);
    System_linkDelete(ipcInVpssId  );
    System_linkDelete(merge1Id);


    System_linkDelete(dup1Id);

    for(i=0; i<numDisplay; i++)
    {
        System_linkDelete(gVdisModuleContext.swMsId[i] );
        System_linkDelete(gVdisModuleContext.displayId[i]);
    }

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

}

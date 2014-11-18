/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
                        Capture (YUV422I) 8CH CIF 60fps
                          |
                          |
                        DEI (VIP-SC YUV420 )
                            |           |
                |     NSF
                |       |
                |       ||-------IPCM3OUT(VPS)--IPCM3IN(VID)--ENC
                |                                            |
                |   |+-----------IPCM3IN(VPS)--IPCM3OUT(VID)--DEC
                        MERGE1
                            |
              SW Mosaic
              (SC5 YUV422I)
                    |
             GRPX0   |
                       |
            On-Chip HDMI
              1080p60
*/

#include "mcfw/src_linux/mcfw_api/usecases/multich_common.h"
#include "mcfw/src_linux/mcfw_api/usecases/multich_ipcbits.h"


#define     NUM_CAPTURE_DEVICES          (2)


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
        .EncNumCh  = 8,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0 , 0, 0},
        .DecNumCh  = 8,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0 , 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 0,
        .EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 0,
        .EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

Void MultiCh_create_8CifProgressiveVcapVencVdecVdis()
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
    DeiLink_CreateParams        deiPrm[1];
    DupLink_CreateParams        dup1Prm;
    MergeLink_CreateParams      merge1Prm;
    static SwMsLink_CreateParams       swMsPrm[2];
    DisplayLink_CreateParams    displayPrm[3];
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
    System_LinkInfo bitsProducerLinkInfo;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;
    UInt32 dup1Id,merge1Id,nullId;
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    UInt32 vipInstId;
    UInt32 i, chId;
    UInt32 numSubChains;

    printf("\nDB_DEBUG:%s %s() %d\n",__FILE__,__FUNCTION__,__LINE__);
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
    MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[0]);
    MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[1]);

    for (i = 0; i < 3; i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams ,displayPrm[i]);
    }
    printf("\n********* Entered usecase 8CH <814x> Enc/Dec \n\n");

    printf("\nDB_DEBUG:%s %s() %d\n",__FILE__,__FUNCTION__,__LINE__);
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

    printf("\nDB_DEBUG:%s %s() %d\n",__FILE__,__FUNCTION__,__LINE__);
    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_0;
    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[1].numSwMsInst = 1;
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI
    gVdisModuleContext.displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; // OFF CHIP HDMI
    merge1Id      = SYSTEM_VPSS_LINK_ID_MERGE_0;
    dup1Id      = SYSTEM_VPSS_LINK_ID_DUP_0;
    nullId      = SYSTEM_VPSS_LINK_ID_NULL_0;


    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    gVencModuleContext.ipcBitsOutRTOSId     = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    gVencModuleContext.ipcBitsInHLOSId       = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.ipcBitsOutHLOSId     = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId       = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;


    capturePrm.numVipInst    = 2;       // 8 channel
    capturePrm.outQueParams[0].nextLink = gVcapModuleContext.deiId[0];      // DEI support both 420SP & 422I
    capturePrm.tilerEnable              = FALSE;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = (SYSTEM_CAPTURE_INST_VIP0_PORTA+
                                              vipInstId)%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_TVP5158_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = SYSTEM_STD_MUX_4CH_HALF_D1;
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

    DeiLink_CreateParams_Init(&deiPrm[0]);
    deiPrm[0].inQueParams.prevLinkId                        = gVcapModuleContext.captureId;
    deiPrm[0].inQueParams.prevLinkQueId                     = 0;
    deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink  = merge1Id;
    deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink  = gVcapModuleContext.nsfId[0];
    deiPrm[0].enableOut[DEI_LINK_OUT_QUE_VIP_SC]              = TRUE;
    deiPrm[0].enableOut[DEI_LINK_OUT_QUE_DEI_SC]              = TRUE;
    deiPrm[0].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = TRUE;
    deiPrm[0].comprEnable                                   = FALSE;
    deiPrm[0].setVipScYuv422Format                          = FALSE;

#ifdef      DEI_SCALING_BASED_ON_RATIO
    /* Set Output Scaling at DEI to ratio based */
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    /* DEI Path Scalar ratio is 1:2, D1 => CIF */
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator    = 1;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator  = 2;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator   = 1;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator         = 2;
    for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
        deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    /* VIP Scalar ratio is 1:1 */
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator    = 1;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator  = 1;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator   = 1;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator         = 1;
    for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
        deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId] = deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];
#else
    /* Set Output Scaling at DEI to ratio based */
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_ABSOLUTE;

    /* DEI Path Scalar <for primary out> is set to D1 */
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].absoluteResolution.outWidth   = 352;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].absoluteResolution.outHeight  = 288;
    for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
        deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_ABSOLUTE;
    /* DEI Path Scalar <for secondary out> is set to CIF */
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outWidth   = 720;
    deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outHeight  = 480;
    for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
        deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId] = deiPrm[0].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];
#endif

    NsfLink_CreateParams_Init(&nsfPrm);
    nsfPrm.bypassNsf                 = TRUE;
    nsfPrm.tilerEnable               = FALSE;
    nsfPrm.inQueParams.prevLinkId    = gVcapModuleContext.deiId[0];
    nsfPrm.inQueParams.prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    nsfPrm.numOutQue                 = 1;
    nsfPrm.outQueParams[0].nextLink  = ipcOutVpssId;

    ipcOutVpssPrm.inQueParams.prevLinkId    = gVcapModuleContext.nsfId[0];
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = TRUE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.outQueParams[0].nextLink     = gVencModuleContext.encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    encPrm.numBufPerCh[0] = 4; //CIF
    {
        EncLink_ChCreateParams *pLinkChPrm;
        EncLink_ChDynamicParams *pLinkDynPrm;
        VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
        VENC_CHN_PARAMS_S *pChPrm;

        /* Primary Stream Params - D1 */
        for (i=0; i<8; i++)
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

        encPrm.inQueParams.prevLinkId    = ipcInVideoId;
        encPrm.inQueParams.prevLinkQueId = 0;
        encPrm.outQueParams.nextLink     = gVencModuleContext.ipcBitsOutRTOSId;
    }


    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm,
                                               TRUE);

    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.ipcBitsOutRTOSId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);


#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif
    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));
    System_linkCreate(gVcapModuleContext.deiId[0]  , &deiPrm[0], sizeof(deiPrm[0]));
    System_linkCreate(gVcapModuleContext.nsfId[0] , &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );

    System_linkCreate(gVencModuleContext.encId, &encPrm, sizeof(encPrm));

    System_linkCreate(gVencModuleContext.ipcBitsOutRTOSId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(gVencModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));


    System_linkGetInfo(gVencModuleContext.ipcBitsInHLOSId,&bitsProducerLinkInfo);
    OSA_assert(bitsProducerLinkInfo.numQue = 1);
    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.ipcBitsInRTOSId;
    printf ("\n\n========bitsProducerLinkInfo============\n");
    printf ("numQ %d, numCh %d\n",
                    bitsProducerLinkInfo.numQue,
                    bitsProducerLinkInfo.queInfo[0].numCh);
    {
        int i;
        for (i=0; i<bitsProducerLinkInfo.queInfo[0].numCh; i++)
        {
            printf ("Ch [%d] Width %d, Height %d\n",
                i,
                bitsProducerLinkInfo.queInfo[0].chInfo[i].width,
                bitsProducerLinkInfo.queInfo[0].chInfo[i].height
                );
        }
    }
    printf ("\n====================\n\n");

    printf ("Reducing bitsProducerLinkInfo.numCh to 8\n");
    bitsProducerLinkInfo.queInfo[0].numCh = 8;

    MultiCh_ipcBitsInitCreateParams_BitsOutHLOS(&ipcBitsOutHostPrm,
                                               &bitsProducerLinkInfo.queInfo[0]);
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
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId = gVdecModuleContext.ipcBitsOutHLOSId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.decId;
    MultiCh_ipcBitsInitCreateParams_BitsInRTOS(&ipcBitsInVideoPrm,
                                              TRUE);
    for (i=0; i<8; i++) {
        decPrm.chCreateParams[i].format          = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile         = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = FALSE;
        decPrm.chCreateParams[i].targetMaxWidth  =
            gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoWidth;
        decPrm.chCreateParams[i].targetMaxHeight =
            gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoHeight;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate =
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.frameRate;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.targetBitRate;
    }
    decPrm.inQueParams.prevLinkId = gVdecModuleContext.ipcBitsInRTOSId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcOutVideoId;
    decPrm.tilerEnable = TRUE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.outQueParams[0].nextLink     = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = FALSE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = TRUE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.outQueParams[0].nextLink     = merge1Id;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = TRUE;

    merge1Prm.numInQue                     = 2;
    merge1Prm.inQueParams[0].prevLinkId    = gVcapModuleContext.deiId[0];
    merge1Prm.inQueParams[0].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    merge1Prm.inQueParams[1].prevLinkId    = ipcInVpssId;
    merge1Prm.inQueParams[1].prevLinkQueId = 0;
    merge1Prm.outQueParams.nextLink        = dup1Id;
    merge1Prm.notifyNextLink               = TRUE;

    dup1Prm.inQueParams.prevLinkId         = merge1Id;;
    dup1Prm.inQueParams.prevLinkQueId      = 0;
    dup1Prm.numOutQue                      = 2;
    dup1Prm.outQueParams[0].nextLink       = gVdisModuleContext.swMsId[0];
    dup1Prm.outQueParams[1].nextLink       = gVdisModuleContext.swMsId[1];
    dup1Prm.notifyNextLink                 = TRUE;

//  swMsPrm[0].inQueParams.prevLinkId    = dup1Id;
//  swMsPrm[0].inQueParams.prevLinkQueId = 0;   //DEI_LINK_OUT_QUE_VIP_SC;
//  swMsPrm[1].inQueParams.prevLinkId    = dup1Id;
//  swMsPrm[1].inQueParams.prevLinkQueId = 1;

    numSubChains = 2;
    for(i=0; i<numSubChains; i++)
    {
        swMsPrm[i].inQueParams.prevLinkId    = dup1Id;
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = gVdisModuleContext.displayId[i];
        swMsPrm[i].maxOutRes                 = VSYS_STD_1080P_60;
        swMsPrm[i].initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[i].resolution;
        swMsPrm[i].lineSkipMode = FALSE;

        swMsPrm[i].enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(i, &swMsPrm[i], FALSE);

        displayPrm[i].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                = swMsPrm[i].initOutRes;
    }
    displayPrm[2].inQueParams[0].prevLinkId    = dup1Id;
    displayPrm[2].inQueParams[0].prevLinkQueId = 1;
    displayPrm[2].displayRes            = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;


    System_linkCreate(gVdecModuleContext.ipcBitsOutHLOSId,&ipcBitsOutHostPrm,sizeof(ipcBitsOutHostPrm));
    System_linkCreate(gVdecModuleContext.ipcBitsInRTOSId,&ipcBitsInVideoPrm,sizeof(ipcBitsInVideoPrm));
    System_linkCreate(gVdecModuleContext.decId, &decPrm, sizeof(decPrm));
    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(merge1Id   , &merge1Prm  , sizeof(merge1Prm));
    System_linkCreate(dup1Id     , &dup1Prm    , sizeof(dup1Prm));
    for(i=0; i<2; i++)
        System_linkCreate(gVdisModuleContext.swMsId[i]  , &swMsPrm[i], sizeof(swMsPrm[i]));
    for(i=0; i<3; i++)
        System_linkCreate(gVdisModuleContext.displayId[i], &displayPrm[i], sizeof(displayPrm[i]));

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

//    MultiCh_memPrintHeapStatus();
    //Utils_encdecHdvicpPrfInit();
}

Void MultiCh_delete_8CifProgressiveVcapVencVdecVdis()
{
    UInt32 dup1Id,merge1Id;
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    int i;
    dup1Id      = SYSTEM_VPSS_LINK_ID_DUP_0;
    merge1Id    = SYSTEM_VPSS_LINK_ID_MERGE_0;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    System_linkDelete(gVcapModuleContext.captureId);

    System_linkDelete(gVcapModuleContext.deiId[0] );
    System_linkDelete(gVcapModuleContext.nsfId[0]);
    System_linkDelete(ipcOutVpssId );
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
    for(i=0; i<2; i++)
        System_linkDelete(gVdisModuleContext.swMsId[i] );

    for(i=0; i<3; i++)
        System_linkDelete(gVdisModuleContext.displayId[i]);


    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);
    //Utils_encdecHdvicpPrfPrint();


}


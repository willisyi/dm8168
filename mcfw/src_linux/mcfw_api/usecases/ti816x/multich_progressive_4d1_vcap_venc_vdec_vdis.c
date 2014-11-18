/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/* This usecase assumes that all 3 outputs - D1 <h.264> + CIF <h.264> + D1 <MJPEG> are enabled */
/*------------------------------ D1 <h.264> + CIF <h.264> + D1 <MJPEG> -------------------------------------


                  Capture (YUV422I) 4CH D1 60fps
                              |
                              |
                              |
                              |
                              DEI
                              |+------------------------------------------------------------------------
                              |                                            |                           |
                       (DEI-SC YUV422I)                              (VIP-SC YUV420 )            (VIP-SC1 YUV420 )
                              D1                                        D1 |                        CIF|
                              |                                            |                           |
                              |                                            |                           |
                              |                                            |                           |
                        -----------------                                  |                           |
                         D1_DUP_LINK_IDX                                   |                           |
                        -----------------                                  |                           |
                              |     |                                      |                           |
                              |     |                                      |                           |
                              |     |                                      |                           |
                              |    NSF                                     |                           |
                              |  (YUV420)                                  |                           |
                              |     |                                      |                           |
                              |     |                                      |          |-----------------
                              |     |-----------------------------         |          |
                              |                                  |         |          |
                              |                      <D1-MJPEG> 2|         |0         | 1
                              |                                  |         |<D1-H.264>| <CIF-H.264>
                              |                                  |         |          |
                              |                                  |         |          |
                              |                            ---------------------------------
                              |                                   D1_CIF_MERGE_LINK_IDX
                              |                            ---------------------------------
                              |                                            |
                              |                                            |
                              |                                            |
                              |                                     FRAMESOUT(VPSS)---------------<<<processLink>>>----FramesInDSP--------ALG_LINK <OSD, SCD Algs>
                              |                                            |
                              |                                            |
                              |                                            |
                              |                                            |
                              |                                       IPCM3OUT(VPSS)------IPCM3IN(VID)----ENC-----IPCBITS_RTOSOUT(VID)-----IPCBITS_HLOSIN(HOST)-----FILEOUT
                              |                                                                                                                                  |
                              |                                                                                                                                  |
                              |                                                                                                                                  |
                              |                                                                                                                                  |
                              |+--------------------------IPCM3IN(VPSS)---------IPCM3OUT(VID)---------DEC---------------IPCBITS_RTOSIN(VID)-----------IPCBITS_HLOSOUT(HOST)
                              |
                              |
                      LIVE_DECODE_MERGE_LINK_IDX
                              |
                              |
                              |
                        LIVE_DECODE_DUP_LINK_IDX
                             |||
                             |||
                             |||
             +---------------+|+----------------+
             |                                  |
             |                                  |
          SW Mosaic 1                       SW Mosaic 0
         (SC2 YUV422I)                     (SC5 YUV422I)
             |                                  |
             |                                  |
             |                                  |
             |                                  |
             |                                  |
             |                                  |
       -------------                       -------------
          DISPLAY 1                           DISPLAY 0
       -------------                       -------------   <tied>
          <SDTV>                          <On-Chip HDMI> --------- <Off-Chip HDMI>
         PAL/NTSC                            1080p60                 1080p60


*/


#include "mcfw/src_linux/mcfw_api/usecases/multich_common.h"
#include "mcfw/src_linux/mcfw_api/usecases/multich_ipcbits.h"

// Keeping TILER disabled for 4D1 usecase
#define TILER_ENABLE    TRUE


/* =============================================================================
 * Externs
 * =============================================================================
 */

static UInt8 SCDChannelMonitor[4] = {4, 5, 6, 7};

/* =============================================================================
 * Use case code
 * =============================================================================
 */
static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 16,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 , 14, 15},
        .DecNumCh  = 16,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 , 14, 15},
    },

};

#define     ADD_NSF_AFTER_CAPTURE


#define     NUM_MERGE_LINK                          2
#define     D1_CIF_MERGE_LINK_IDX                  0
#define     LIVE_DECODE_MERGE_LINK_IDX             1

#define     NUM_DUP_LINK                            2
#define     D1_DUP_LINK_IDX                         0
#define     LIVE_DECODE_DUP_LINK_IDX                1

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD) || defined(DDR_MEM_256M)
#define     NUM_CAPTURE_BUFFERS          8
#define     NUM_NSF_BUFFERS              4
#define     NUM_ENCODE_D1_BUFFERS       4
#define     NUM_ENCODE_CIF_BUFFERS       4
#define     NUM_DECODE_BUFFERS           4
#define     NUM_IPC_OUT_BUFFERS          4
#define     NUM_SWMS_MAX_BUFFERS         8
#else
#define     NUM_CAPTURE_BUFFERS          8
#define     NUM_NSF_BUFFERS              6
#define     NUM_ENCODE_D1_BUFFERS       6
#define     NUM_ENCODE_CIF_BUFFERS       6
#define     NUM_DECODE_BUFFERS           6
#define     NUM_IPC_OUT_BUFFERS          6
#define     NUM_SWMS_MAX_BUFFERS         8
#endif

#define     NUM_CAPTURE_DEVICES          1


/* This usecase assumes CIF <secondary out>, MJPEG are enabled */
Void MultiCh_createProgressive4D1VcapVencVdecVdis()
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
    DeiLink_CreateParams        deiPrm;
    MergeLink_CreateParams      mergePrm[NUM_MERGE_LINK];
    DupLink_CreateParams        dupPrm[NUM_DUP_LINK];
    static SwMsLink_CreateParams       swMsPrm[VDIS_DEV_MAX];
    DisplayLink_CreateParams    displayPrm[VDIS_DEV_MAX];
    IpcLink_CreateParams        ipcOutVpssPrm;
    IpcLink_CreateParams        ipcInVpssPrm;
    IpcLink_CreateParams        ipcOutVideoPrm;
    IpcLink_CreateParams        ipcInVideoPrm;
    EncLink_CreateParams        encPrm;
    DecLink_CreateParams        decPrm;
    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams    ipcBitsInHostPrm[2];

    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    CaptureLink_VipInstParams         *pCaptureInstPrm;
    CaptureLink_OutParams             *pCaptureOutPrm;
    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm;
    IpcFramesOutLinkRTOS_CreateParams ipcFramesOutVpssPrm;
    AlgLink_CreateParams              dspAlgPrm;

    System_LinkInfo                   bitsProducerLinkInfo;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm;
    UInt32 ipcBitsOutDSPId;

    UInt32 vipInstId;
    UInt32 i, j;
    Bool   enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool   enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;
#ifdef     ADD_NSF_AFTER_CAPTURE
    NsfLink_CreateParams        nsfPrm2;
#endif

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutDspPrm);

    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm[0]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm[1]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(DecLink_CreateParams, decPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm);
    MULTICH_INIT_STRUCT(EncLink_CreateParams, encPrm);
    MULTICH_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm);
    for (i = 0; i < VDIS_DEV_MAX;i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams,
                            displayPrm[i]);
        MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[i]);
    }

    MULTICH_INIT_STRUCT(NsfLink_CreateParams, nsfPrm);
    MULTICH_INIT_STRUCT(DeiLink_CreateParams, deiPrm);

    printf("\n********* Entered usecase 4CH <814x> Enc/Dec OSD %s SCD %s \n\n",
            enableOsdAlgLink == TRUE ? "Enabled" : "Disabled",
            enableScdAlgLink == TRUE ? "Enabled" : "Disabled"
            );

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

    vipInstId = 0;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;

    if(enableOsdAlgLink || enableScdAlgLink)
    {
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_ALG_0  ;
        gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
        gVcapModuleContext.ipcFramesInDspId[0] = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
        if(enableScdAlgLink)
        {
          ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
          gVcapModuleContext.ipcBitsInHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
        }
    }

    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
#ifdef     ADD_NSF_AFTER_CAPTURE
    gVcapModuleContext.nsfId[1]     =SYSTEM_LINK_ID_NSF_1;
#endif
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_0;
    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;

    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[1].numSwMsInst = 1;

    #if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    /* use AUX scaler (SC2), since SC1 is used for DEI */
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEI_SC_NO_DEI;
    #else
    /* use MAIN scaler (SC1), since SC2 is used for DEI */
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEIHQ_SC_NO_DEI;
    #endif

    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; /* ON AND OFF CHIP HDMI */
    gVdisModuleContext.displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; /* SDTV */

    mergeId[LIVE_DECODE_MERGE_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[D1_CIF_MERGE_LINK_IDX]          = SYSTEM_VPSS_LINK_ID_MERGE_1;

    dupId[D1_DUP_LINK_IDX]              = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_DUP_1;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    if(enableScdAlgLink)
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
    else
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

    CaptureLink_CreateParams_Init(&capturePrm);
#ifdef     ADD_NSF_AFTER_CAPTURE
    capturePrm.outQueParams[0].nextLink   = gVcapModuleContext.nsfId[1];
#else
    capturePrm.outQueParams[0].nextLink   = gVcapModuleContext.deiId[0];
#endif
    capturePrm.numVipInst                 = 1;
    capturePrm.tilerEnable                = FALSE;
    capturePrm.numBufsPerCh               = NUM_CAPTURE_BUFFERS;

    pCaptureInstPrm                     = &capturePrm.vipInst[0];
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


#ifdef     ADD_NSF_AFTER_CAPTURE
    nsfPrm2.bypassNsf                          = TRUE;
    nsfPrm2.tilerEnable                          = FALSE;
    nsfPrm2.inQueParams.prevLinkId          = gVcapModuleContext.captureId;
    nsfPrm2.inQueParams.prevLinkQueId     = 0;
    nsfPrm2.numOutQue                        = 1;
    nsfPrm2.outQueParams[0].nextLink      = gVcapModuleContext.deiId[0];
    nsfPrm2.numBufsPerCh                     = NUM_NSF_BUFFERS;
    nsfPrm2.inputFrameRate                    = 30;
    nsfPrm2.outputFrameRate                  = 30;

    deiPrm.inQueParams.prevLinkId = gVcapModuleContext.nsfId[1];
#else
    deiPrm.inQueParams.prevLinkId = gVcapModuleContext.captureId;
#endif
    deiPrm.inQueParams.prevLinkQueId  = 0;

    /* Set Output Scaling at DEI based on ratio */
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 1;
    for (i=1; i < DEI_LINK_MAX_CH; i++)
        deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][i] = deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator = 1;
    for (i=1; i < DEI_LINK_MAX_CH; i++)
        deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][i] = deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];

    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.numerator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.denominator = 2;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.numerator = 1;
    deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.denominator = 2;
    for (i=1; i < DEI_LINK_MAX_CH; i++)
        deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][i] = deiPrm.outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0];

    deiPrm.enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
    deiPrm.enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = TRUE;
    deiPrm.enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]          = TRUE;

    deiPrm.outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink                   = dupId[D1_DUP_LINK_IDX];
    deiPrm.outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink                   = mergeId[D1_CIF_MERGE_LINK_IDX];
    deiPrm.outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink     = mergeId[D1_CIF_MERGE_LINK_IDX];

    deiPrm.tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = TILER_ENABLE;
    deiPrm.comprEnable                                   = FALSE;
    deiPrm.setVipScYuv422Format                          = FALSE;

    dupPrm[D1_DUP_LINK_IDX].inQueParams.prevLinkId         = gVcapModuleContext.deiId[0];
    dupPrm[D1_DUP_LINK_IDX].inQueParams.prevLinkQueId      = DEI_LINK_OUT_QUE_DEI_SC;
    dupPrm[D1_DUP_LINK_IDX].numOutQue                      = 2;
    dupPrm[D1_DUP_LINK_IDX].outQueParams[0].nextLink       = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    dupPrm[D1_DUP_LINK_IDX].outQueParams[1].nextLink       = gVcapModuleContext.nsfId[0];
    dupPrm[D1_DUP_LINK_IDX].notifyNextLink                 = TRUE;

    nsfPrm.bypassNsf                        = TRUE;
    nsfPrm.tilerEnable                      = TILER_ENABLE;
    nsfPrm.inQueParams.prevLinkId           = dupId[D1_DUP_LINK_IDX];
    nsfPrm.inQueParams.prevLinkQueId        = 1;
    nsfPrm.numOutQue                        = 1;
    nsfPrm.outQueParams[0].nextLink         = mergeId[D1_CIF_MERGE_LINK_IDX];
    nsfPrm.numBufsPerCh                     = NUM_NSF_BUFFERS;
    nsfPrm.inputFrameRate = 30;
    nsfPrm.outputFrameRate = 1;

    /* Merge Q0 - D1 <from VIP-SC>, Q1 - CIF <from VIP-SC_SECONDARY>, Q2 -D1 for MJPEG <from DEI-SC>  */
    mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[0].prevLinkId       = gVcapModuleContext.deiId[0];
    mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId    = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[1].prevLinkId       = gVcapModuleContext.deiId[0];
    mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId    = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[2].prevLinkId       = gVcapModuleContext.nsfId[0];
    mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId    = 0;

    mergePrm[D1_CIF_MERGE_LINK_IDX].notifyNextLink                  = TRUE;
    mergePrm[D1_CIF_MERGE_LINK_IDX].numInQue = 3;

    if (enableOsdAlgLink || enableScdAlgLink)
    {
        mergePrm[D1_CIF_MERGE_LINK_IDX].outQueParams.nextLink  = gVcapModuleContext.ipcFramesOutVpssId[0];
        ipcOutVpssPrm.inQueParams.prevLinkId                   = gVcapModuleContext.ipcFramesOutVpssId[0];

        /* Redirect to DSP for OSD / SCD */
        ipcFramesOutVpssPrm .baseCreateParams.inQueParams.prevLinkId   = mergeId[D1_CIF_MERGE_LINK_IDX];
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

        dspAlgPrm.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
        dspAlgPrm.inQueParams.prevLinkQueId = 0;
    }
    else
    {
        mergePrm[D1_CIF_MERGE_LINK_IDX].outQueParams.nextLink  = ipcOutVpssId;
        ipcOutVpssPrm.inQueParams.prevLinkId                   = mergeId[D1_CIF_MERGE_LINK_IDX];
    }

    ipcOutVpssPrm.inQueParams.prevLinkQueId    = 0;
    ipcOutVpssPrm.numOutQue = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink               = TRUE;
    ipcOutVpssPrm.notifyPrevLink               = TRUE;
    ipcOutVpssPrm.noNotifyMode                 = FALSE;

    ipcInVideoPrm.inQueParams.prevLinkId       = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId    = 0;
    ipcInVideoPrm.numOutQue                    = 1;
    ipcInVideoPrm.outQueParams[0].nextLink     = gVencModuleContext.encId;
    ipcInVideoPrm.notifyNextLink               = TRUE;
    ipcInVideoPrm.notifyPrevLink               = TRUE;
    ipcInVideoPrm.noNotifyMode                 = FALSE;

    {
        EncLink_ChCreateParams *pLinkChPrm;
        EncLink_ChDynamicParams *pLinkDynPrm;
        VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
        VENC_CHN_PARAMS_S *pChPrm;

        EncLink_CreateParams_Init(&encPrm);

        encPrm.numBufPerCh[0] = NUM_ENCODE_D1_BUFFERS;
        encPrm.numBufPerCh[1] = NUM_ENCODE_CIF_BUFFERS;

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
        /* Secondary Out <CIF> Params */
        for (i=gVencModuleContext.vencConfig.numPrimaryChn, j=VENC_PRIMARY_CHANNELS;
              i<(gVencModuleContext.vencConfig.numPrimaryChn
                       + gVencModuleContext.vencConfig.numSecondaryChn);
                i++, j++)
        {
            pLinkChPrm  = &encPrm.chCreateParams[i];
            pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

            pChPrm      = &gVencModuleContext.vencConfig.encChannelParams[j];
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
            pLinkDynPrm->qpMin                  = pDynPrm->qpMin;
            pLinkDynPrm->qpMax                  = pDynPrm->qpMax;
            pLinkDynPrm->qpInit                 = pDynPrm->qpInit;
            pLinkDynPrm->vbrDuration            = pDynPrm->vbrDuration;
            pLinkDynPrm->vbrSensitivity         = pDynPrm->vbrSensitivity;
        }

        /* MJPEG  Params */
        for (i=gVencModuleContext.vencConfig.numPrimaryChn + gVencModuleContext.vencConfig.numSecondaryChn;
                  i<(VENC_CHN_MAX); i++)
        {
             pLinkChPrm  = &encPrm.chCreateParams[i];
             pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

             pChPrm      = &gVencModuleContext.vencConfig.encChannelParams[i];
             pDynPrm     = &pChPrm->dynamicParam;

             pLinkChPrm->format                 = IVIDEO_MJPEG;
             pLinkChPrm->profile                = 0;
             pLinkChPrm->dataLayout             = IVIDEO_FIELD_SEPARATED;
             pLinkChPrm->fieldMergeEncodeEnable = FALSE;
             pLinkChPrm->enableAnalyticinfo     = 0;
             pLinkChPrm->enableWaterMarking     = 0;
             pLinkChPrm->maxBitRate             = 0;
             pLinkChPrm->encodingPreset         = 0;
             pLinkChPrm->rateControlPreset      = 0;
             pLinkChPrm->enableSVCExtensionFlag = 0;
             pLinkChPrm->numTemporalLayer       = 0;

             pLinkDynPrm->intraFrameInterval    = 0;
             pLinkDynPrm->targetBitRate         = 100*1000;
             pLinkDynPrm->interFrameInterval    = 0;
             pLinkDynPrm->mvAccuracy            = 0;
             pLinkDynPrm->inputFrameRate        = pDynPrm->inputFrameRate;
             pLinkDynPrm->qpMin                 = 0;
             pLinkDynPrm->qpMax                 = 0;
             pLinkDynPrm->qpInit                = -1;
             pLinkDynPrm->vbrDuration           = 0;
             pLinkDynPrm->vbrSensitivity        = 0;
        }

        encPrm.inQueParams.prevLinkId    = ipcInVideoId;
        encPrm.inQueParams.prevLinkQueId = 0;
        encPrm.outQueParams.nextLink     = gVencModuleContext.ipcBitsOutRTOSId;
    }

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm,
                                               TRUE);

    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.ipcBitsOutRTOSId;
    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm[0]);

    if(enableOsdAlgLink)
    {
        int chId;
        dspAlgPrm.enableOSDAlg = TRUE;

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

    if (enableScdAlgLink)
    {
        Int32   numBlksInFrame;
        Int32   numHorzBlks, numVertBlks, chIdx;
        Uint32  x, y, i;
        //AlgLink_ScdblkChngConfig  blkConfig[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];

        dspAlgPrm.enableSCDAlg              = TRUE;
        dspAlgPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = ipcBitsOutDSPId;

        dspAlgPrm.scdCreateParams.maxWidth               = 352;
        if(Vcap_isPalMode())
           dspAlgPrm.scdCreateParams.maxHeight              = 288;
        else
           dspAlgPrm.scdCreateParams.maxHeight              = 240;
        dspAlgPrm.scdCreateParams.maxStride              = 352;
        dspAlgPrm.scdCreateParams.numValidChForSCD       = 4;
        dspAlgPrm.scdCreateParams.numSecs2WaitB4Init     = 3;
        dspAlgPrm.scdCreateParams.numSecs2WaitB4FrmAlert = 1;
        dspAlgPrm.scdCreateParams.inputFrameRate         = 30;
        dspAlgPrm.scdCreateParams.outputFrameRate        = 5;
        dspAlgPrm.scdCreateParams.numSecs2WaitAfterFrmAlert = 2;

        /* Should be applied on CIF channels whose ch numbers are 4~7 */
        //dspAlgPrm.scdCreateParams.startChNoForSCD = 4;
       // Configure array to monitor scene changes in all frame blocks, i.e., motion detection.
       // Each block is fixed to be 32x10 in size,
        numHorzBlks    = dspAlgPrm.scdCreateParams.maxWidth / 32;

        if(dspAlgPrm.scdCreateParams.maxHeight == 240)
           numVertBlks    = dspAlgPrm.scdCreateParams.maxHeight / 10;
        else   /* For 288 Block height becomes 12 */
           numVertBlks    = dspAlgPrm.scdCreateParams.maxHeight / 12;

        numBlksInFrame = numHorzBlks * numVertBlks;
/*
   i = 0;
   for(y = 0; y < numVertBlks; y++)
   {
     for(x = 0; x < numHorzBlks; x++)
     {
       blkConfig[i].sensitivity = ALG_LINK_SCD_SENSITIVITY_MID;
       blkConfig[i].monitored    = 1;
       i++;
     }
   }
*/
        for(chIdx = 0; chIdx < dspAlgPrm.scdCreateParams.numValidChForSCD; chIdx++)
        {
           AlgLink_ScdChParams * chPrm = &dspAlgPrm.scdCreateParams.chDefaultParams[chIdx];

           chPrm->blkNumBlksInFrame = numBlksInFrame;
           chPrm->chId               = SCDChannelMonitor[chIdx];
           chPrm->mode               = ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
           chPrm->frmIgnoreLightsON = FALSE;
           chPrm->frmIgnoreLightsOFF    = FALSE;
           chPrm->frmSensitivity     = ALG_LINK_SCD_SENSITIVITY_HIGH;
                chPrm->frmEdgeThreshold   = 100;
     //     chPrm->blkConfig            = NULL; //blkConfig;
           i = 0;
           for(y = 0; y < numVertBlks; y++)
           {
             for(x = 0; x < numHorzBlks; x++)
             {
               chPrm->blkConfig[i].sensitivity = ALG_LINK_SCD_SENSITIVITY_MID;
               chPrm->blkConfig[i].monitored     = 0;
               i++;
             }
           }
        }

        ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkId    = gVcapModuleContext.dspAlgId[0];
        ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcBitsOutDspPrm.baseCreateParams.numOutQue                 = 1;
        ipcBitsOutDspPrm.baseCreateParams.outQueParams[0].nextLink  = gVcapModuleContext.ipcBitsInHLOSId;
        MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutDspPrm,
                                                   TRUE);
        ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkId = ipcBitsOutDSPId;
        ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        MultiCh_ipcBitsInitCreateParams_BitsInHLOSVcap(&ipcBitsInHostPrm[1]);
    }
    else
    {
        dspAlgPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;
    }
#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif
#if 0
    {
        CaptureLink_BlindInfo blindInfo[2];

        blindInfo[0].streamId = 0;
        blindInfo[0].channelId = 1;
        blindInfo[0].fillColorYUYV= 0x80108010;
        blindInfo[0].numBlindArea = 4;
        blindInfo[0].win[0].startX = 30;
        blindInfo[0].win[0].startY = 10;
        blindInfo[0].win[0].width = 30;
        blindInfo[0].win[0].height = 50;

        blindInfo[0].win[1].startX = 70;
        blindInfo[0].win[1].startY = 60;
        blindInfo[0].win[1].width = 30;
        blindInfo[0].win[1].height = 50;
        blindInfo[0].win[2].startX = 110;
        blindInfo[0].win[2].startY = 110;
        blindInfo[0].win[2].width = 30;
        blindInfo[0].win[2].height = 50;
        blindInfo[0].win[3].startX = 150;
        blindInfo[0].win[3].startY = 160;
        blindInfo[0].win[3].width = 30;
        blindInfo[0].win[3].height = 50;

        blindInfo[1] = blindInfo[0];

        blindInfo[1].channelId = 15;

        capturePrm.blindInfo[0] = blindInfo[0];
        capturePrm.blindInfo[1] = blindInfo[1];
    }
#endif
    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));

#ifdef     ADD_NSF_AFTER_CAPTURE
    System_linkCreate(gVcapModuleContext.nsfId[1], &nsfPrm2, sizeof(nsfPrm2));
#endif
    System_linkCreate(gVcapModuleContext.deiId[0], &deiPrm, sizeof(deiPrm));

    System_linkCreate(dupId[D1_DUP_LINK_IDX], &dupPrm[D1_DUP_LINK_IDX], sizeof(dupPrm[D1_DUP_LINK_IDX]));
    System_linkCreate(gVcapModuleContext.nsfId[0] , &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(mergeId[D1_CIF_MERGE_LINK_IDX], &mergePrm[D1_CIF_MERGE_LINK_IDX], sizeof(mergePrm[D1_CIF_MERGE_LINK_IDX]));

    if(enableOsdAlgLink || enableScdAlgLink)
    {
        System_linkCreate(gVcapModuleContext.ipcFramesOutVpssId[0], &ipcFramesOutVpssPrm, sizeof(ipcFramesOutVpssPrm));
        System_linkCreate(gVcapModuleContext.ipcFramesInDspId[0], &ipcFramesInDspPrm, sizeof(ipcFramesInDspPrm));
        System_linkCreate(gVcapModuleContext.dspAlgId[0] , &dspAlgPrm, sizeof(dspAlgPrm));
        if(enableScdAlgLink)
        {
          System_linkCreate(ipcBitsOutDSPId, &ipcBitsOutDspPrm, sizeof(ipcBitsOutDspPrm));
          System_linkCreate(gVcapModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm[1], sizeof(ipcBitsInHostPrm[1]));
        }
    }
    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );

    System_linkCreate(gVencModuleContext.encId, &encPrm, sizeof(encPrm));

    System_linkCreate(gVencModuleContext.ipcBitsOutRTOSId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(gVencModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm[0], sizeof(ipcBitsInHostPrm[0]));

    System_linkGetInfo(gVencModuleContext.ipcBitsInHLOSId,&bitsProducerLinkInfo);
    OSA_assert(bitsProducerLinkInfo.numQue == 1);
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

    if (bitsProducerLinkInfo.queInfo[0].numCh > gVencModuleContext.vencConfig.numPrimaryChn)
        bitsProducerLinkInfo.queInfo[0].numCh = gVencModuleContext.vencConfig.numPrimaryChn;

    printf ("Reducing bitsProducerLinkInfo.numCh to %d\n", bitsProducerLinkInfo.queInfo[0].numCh);

    MultiCh_ipcBitsInitCreateParams_BitsOutHLOS(&ipcBitsOutHostPrm,
                                               &bitsProducerLinkInfo.queInfo[0]);
    ipcBitsOutHostPrm.numBufPerCh[0]  = NUM_IPC_OUT_BUFFERS;
    if(gVdecModuleContext.vdecConfig.forceUseDecChannelParams)
    {
        /* use channel info provided by user instead of from encoder */
        UInt32 chId;
        System_LinkChInfo *pChInfo;

        ipcBitsOutHostPrm.inQueInfo.numCh = gVdecModuleContext.vdecConfig.numChn;

        for(chId=0; chId<ipcBitsOutHostPrm.inQueInfo.numCh; chId++)
        {
            pChInfo = &ipcBitsOutHostPrm.inQueInfo.chInfo[chId];

            /* Not Used - Start */
            pChInfo->bufType        = 0;
            pChInfo->codingformat   = 0;
            pChInfo->dataFormat     = 0;
            pChInfo->memType        = 0;
            pChInfo->startX         = 0;
            pChInfo->startY         = 0;
            pChInfo->pitch[0]       = 0;
            pChInfo->pitch[1]       = 0;
            pChInfo->pitch[2]       = 0;
            /* Not Used - End */

            pChInfo->width          = gVdecModuleContext.vdecConfig.decChannelParams[chId].maxVideoWidth;
            pChInfo->height         = gVdecModuleContext.vdecConfig.decChannelParams[chId].maxVideoHeight;
            pChInfo->scanFormat     = SYSTEM_SF_PROGRESSIVE;
        }
    }
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsOutHLOSId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId    = 0;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue                    = 1;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink     = gVdecModuleContext.decId;
    MultiCh_ipcBitsInitCreateParams_BitsInRTOS(&ipcBitsInVideoPrm, TRUE);

    for (i=0; i<gVdecModuleContext.vdecConfig.numChn; i++) {
        decPrm.chCreateParams[i].format                 = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile                = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = FALSE;
        decPrm.chCreateParams[i].targetMaxWidth         = ipcBitsOutHostPrm.inQueInfo.chInfo[i].width;
        decPrm.chCreateParams[i].targetMaxHeight        = ipcBitsOutHostPrm.inQueInfo.chInfo[i].height;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.frameRate;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate   = gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.targetBitRate;
        /* Max ref frames is only 2 as this is closed loop decoder */
        decPrm.chCreateParams[i].dpbBufSizeInFrames                   = 2;
        decPrm.chCreateParams[i].numBufPerCh = NUM_DECODE_BUFFERS;
    }
    decPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsInRTOSId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink  = ipcOutVideoId;
    decPrm.tilerEnable = TILER_ENABLE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                 = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink  = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = TRUE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = FALSE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink  = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = TRUE;
    ipcInVpssPrm.noNotifyMode              = FALSE;

    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                     = 2;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = dupId[D1_DUP_LINK_IDX];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = 0;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = ipcInVpssId;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = 0;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].outQueParams.nextLink        = dupId[LIVE_DECODE_DUP_LINK_IDX];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].notifyNextLink               = TRUE;

    dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkId         = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].numOutQue                      = 2;
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[0].nextLink       = gVdisModuleContext.swMsId[0];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[1].nextLink       = gVdisModuleContext.swMsId[1];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].notifyNextLink                 = TRUE;

    for(i=0; i<2; i++)
    {
        swMsPrm[i].inQueParams.prevLinkId    = dupId[LIVE_DECODE_DUP_LINK_IDX];
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        if (i == 1)
        {
            swMsPrm[i].outQueParams.nextLink     = gVdisModuleContext.displayId[2];
        }
        else
        {
            swMsPrm[i].outQueParams.nextLink     = gVdisModuleContext.displayId[i];
        }
        swMsPrm[i].maxInputQueLen            = 4; // KC: changed to 4 to make the display smooth
        swMsPrm[i].maxOutRes                 = VSYS_STD_1080P_60;
        swMsPrm[i].initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[i].resolution;
        swMsPrm[i].numOutBuf                 = 8;
        if (i == 1)
		{
            swMsPrm[i].maxOutRes  = VSYS_STD_PAL;
            swMsPrm[i].initOutRes = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
		}

#ifdef  SYSTEM_USE_TILER
        swMsPrm[i].lineSkipMode = FALSE; // Double pitch not possible in tiler mode; so Line skip not possible
#else
        swMsPrm[i].lineSkipMode = TRUE; // Set to TRUE for Enable low cost scaling
#endif
        swMsPrm[i].enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(i, &swMsPrm[i], TRUE);

        displayPrm[i].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                = swMsPrm[i].initOutRes;
        if (i == 1)
        {
            displayPrm[i].displayRes            = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
            displayPrm[i].forceFieldSeparatedInputMode = TRUE;
        }
    }

    System_linkCreate(gVdecModuleContext.ipcBitsOutHLOSId,&ipcBitsOutHostPrm,sizeof(ipcBitsOutHostPrm));
    System_linkCreate(gVdecModuleContext.ipcBitsInRTOSId,&ipcBitsInVideoPrm,sizeof(ipcBitsInVideoPrm));
    System_linkCreate(gVdecModuleContext.decId, &decPrm, sizeof(decPrm));

    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(mergeId[LIVE_DECODE_MERGE_LINK_IDX], &mergePrm[LIVE_DECODE_MERGE_LINK_IDX], sizeof(mergePrm[LIVE_DECODE_MERGE_LINK_IDX]));
    System_linkCreate(dupId[LIVE_DECODE_DUP_LINK_IDX], &dupPrm[LIVE_DECODE_DUP_LINK_IDX], sizeof(dupPrm[LIVE_DECODE_DUP_LINK_IDX]));

    for(i=0; i<2; i++)
        System_linkCreate(gVdisModuleContext.swMsId[i]  , &swMsPrm[i], sizeof(swMsPrm[i]));

    for(i=0; i<2; i++)  /* Both tied VENCs HDMI and DVO2 are handled by single link instance */
    {
        if (i == 1)
        {
            System_linkCreate(gVdisModuleContext.displayId[2],
            &displayPrm[i], sizeof(displayPrm[i]));
        }
        else
        {
            System_linkCreate(gVdisModuleContext.displayId[i],
            &displayPrm[i], sizeof(displayPrm[i]));
        }
    }
    {
        MergeLink_InLinkChInfo inChInfo;

        MergeLink_InLinkChInfo_Init(&inChInfo);
        inChInfo.inLinkID = ipcInVpssId;
        System_linkControl(mergeId[LIVE_DECODE_MERGE_LINK_IDX],
                           MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO,
                           &inChInfo,
                           sizeof(inChInfo),
                           TRUE);
        OSA_assert(inChInfo.numCh == gVdecModuleContext.vdecConfig.numChn);
        MultiCh_setDec2DispMap(VDIS_DEV_HDMI,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
        MultiCh_setDec2DispMap(VDIS_DEV_SD,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
   }

}

Void MultiCh_deleteProgressive4D1VcapVencVdecVdis()
{
    UInt32 i;
    Bool enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;

    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    UInt32 ipcBitsOutDSPId;

    mergeId[LIVE_DECODE_MERGE_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[D1_CIF_MERGE_LINK_IDX]          = SYSTEM_VPSS_LINK_ID_MERGE_1;

    dupId[D1_DUP_LINK_IDX]              = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_DUP_1;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;

    gVdecModuleContext.ipcBitsOutHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

    System_linkDelete(gVcapModuleContext.captureId);
#ifdef     ADD_NSF_AFTER_CAPTURE
    System_linkDelete(gVcapModuleContext.nsfId[1]);
#endif
    System_linkDelete(gVcapModuleContext.deiId[0]);
    System_linkDelete(gVcapModuleContext.nsfId[0]);

    if(enableOsdAlgLink || enableScdAlgLink)
    {
        System_linkDelete(gVcapModuleContext.ipcFramesOutVpssId[0]);
        System_linkDelete(gVcapModuleContext.ipcFramesInDspId[0]);
        System_linkDelete(gVcapModuleContext.dspAlgId[0]);

        if(enableScdAlgLink)
        {
            System_linkDelete(ipcBitsOutDSPId);
            System_linkDelete(gVcapModuleContext.ipcBitsInHLOSId);
        }
    }
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

    for(i=0; i<2; i++)
        System_linkDelete(gVdisModuleContext.swMsId[i] );

    for(i=0; i<2; i++)
    {
        if (i == 1)
        {
            System_linkDelete(gVdisModuleContext.displayId[2]);
        }
        else
        {
            System_linkDelete(gVdisModuleContext.displayId[i]);
        }
    }


    for(i=0; i<NUM_DUP_LINK; i++)
        System_linkDelete(dupId[i]);

    for(i=0; i<NUM_MERGE_LINK; i++)
    {
        if (mergeId[i] != SYSTEM_LINK_ID_INVALID )
           System_linkDelete(mergeId[i]);
    }

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

}





/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/*
                      Capture (YUV422I) 16CH D1 60fps
                                  | |
                                  | DUP1------------------------------------------------------------------+
                                  | |                                                                     |
                               DUP0-]------------------------------------------------------------------+  |
                                  | |                                                                  |  |
                            8CH D1| |8CH D1                                                           MERGE2
                                  | +-----------------------------|                                      |
       +-----------+----------- DEIH                 +----------- DEI ------------+                     SC5
    (DEI-SC)   (VIP-SC 420)  (VIP-SC 420)         (DEI-SC)   (VIP-SC 420 )  (VIP-SC 420)                 |
       |           |              |                  |            |              |                       |
       |           |              |  +---------------]------------+              |                      NSF
       |           |        8CH D1|  | 8CH D1        |                           |                       |
       |           |              |  |               |                           |                       |
       |           |              |  |               |  8CH MJPEG  8CH MJPEG     |                       |
       |           +--------------]--]---------------]----------+ +--------------+                       |
 8CH D1|  8CH D1                  |  +---------------]-------+  | |                                      |
       | +------------------------]------------------+       |  | |                                      |
       | |                        |                          |  | |                16CH CIF              |
      MERGE1                      +--------------------------MERGE0--------------------------------------+
        |                                                        |
        |                                                        |
        |16CH D1 Preview                           IPC Frames Out0 (M3)--<<process link>>--IPC Frames In(DSP)--ALGLINK
        |                                                        |
        |                                                        |
        |                           +----------------------------+
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
  IPC Frames Out1 (M3)          IPC OUT (M3)
        |                           |
  IPC Frames IN   (A8)          IPC IN  (M3)
        |                           |
  IPC Frames OUT  (A8)          Encode (16CH D1 + 16CH CIF + 16CH MJPEG)
        |                           |
  IPC Frames IN1  (M3)          IPC BITS OUT (M3)
        |                           |
    SW Mosaic                   IPC BITS IN  (A8)
    (SC5 422)
        |
        |
    On-Chip HDMI
     1080p60
*/

#include "multich_common.h"
#include "mcfw/interfaces/link_api/system_tiler.h"
#define DEI_VIP_SC_MERGE_LINK_IDX   (0)
#define DEI_SC_D1_MERGE_LINK_IDX    (1)
#define CAP_SC5_MERGE_LINK_IDX      (2)
#define NUM_MERGE_LINK              (3)

#define CAP_DUP_LINK_IDX_0          (0)
#define CAP_DUP_LINK_IDX_1          (1)
#define NUM_DUP_LINK                (2)

#define     NUM_CAPTURE_DEVICES     (4)

#define     NUM_DEI_LINK            (2)

typedef struct {

    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 ipcOutVpssId;
    UInt32 ipcInVideoId;

} MultiCh_VcapVencVdisObj;

MultiCh_VcapVencVdisObj gMultiCh_VcapVencVdisObj;
static UInt8 SCDChannelMonitor[16] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
/* =============================================================================
 * Use case code
 * =============================================================================
 */
static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 24,
        .EncChList = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 24,
        .EncChList = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47},
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

Void MultiCh_createVcapVencVdis()
{
    CaptureLink_CreateParams        capturePrm;
    DeiLink_CreateParams            deiPrm[NUM_DEI_LINK];
    MergeLink_CreateParams          mergePrm[NUM_MERGE_LINK];
    DupLink_CreateParams            dupPrm[NUM_DUP_LINK];
    NsfLink_CreateParams            nsfPrm;
    SclrLink_CreateParams           sclrPrm;
    SwMsLink_CreateParams           swMsPrm;
    DisplayLink_CreateParams        displayPrm;
    IpcLink_CreateParams            ipcOutVpssPrm;
    IpcLink_CreateParams            ipcInVideoPrm;
    EncLink_CreateParams            encPrm;
    IpcBitsOutLinkRTOS_CreateParams ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams  ipcBitsInHostPrm[2];

    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssToHostPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInHostPrm;
    IpcFramesOutLinkHLOS_CreateParams  ipcFramesOutHostPrm;
    IpcFramesInLinkRTOS_CreateParams   ipcFramesInVpssFromHostPrm;

    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm;
    IpcFramesOutLinkRTOS_CreateParams ipcFramesOutVpssPrm;
    AlgLink_CreateParams              dspAlgPrm;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

    Bool                               enableVideoFramesExport = TRUE;//FALSE;

    Bool   enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool   osdFormat = 0; // 0 for 420SP and 1 for 422i
    Bool   enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;

    System_LinkInfo framesProducerLinkInfo;

    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm;
    UInt32 ipcBitsOutDSPId;

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;

    UInt32 vipInstId;
    UInt32 i,j;
    Bool enableTiler;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams ,ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams ,ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm[0]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm[1]);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssToHostPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInHostPrm);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkHLOS_CreateParams ,ipcFramesOutHostPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  ,ipcFramesInVpssFromHostPrm);
    MULTICH_INIT_STRUCT(EncLink_CreateParams, encPrm);
    MULTICH_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm);
    MULTICH_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm);

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

    /* Disable tiler allocator for this usecase
     * for that tiler memory can be reused for
     * non-tiled allocation
     */
    SystemTiler_disableAllocator();

    enableTiler                     = FALSE;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.sclrId[0]       = SYSTEM_LINK_ID_SCLR_INST_0;
    if(enableOsdAlgLink || enableScdAlgLink)
    {
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_ALG_0  ;
        gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
        gVcapModuleContext.ipcFramesInDspId[0]   = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
        if(enableScdAlgLink)
        {
          ipcBitsOutDSPId                    = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
          gVcapModuleContext.ipcBitsInHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
        }
    }

    gVdisModuleContext.swMsId[0]    = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;

    swMsPrm.numSwMsInst             = 1;
    swMsPrm.swMsInstId[0]           = SYSTEM_SW_MS_SC_INST_SC5;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI

    gVencModuleContext.encId            = SYSTEM_LINK_ID_VENC_0;
    if(enableScdAlgLink)
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
    else
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

    gVencModuleContext.ipcBitsOutRTOSId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;

    gMultiCh_VcapVencVdisObj.mergeId[DEI_VIP_SC_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_0;
    gMultiCh_VcapVencVdisObj.mergeId[DEI_SC_D1_MERGE_LINK_IDX]  = SYSTEM_VPSS_LINK_ID_MERGE_1;
    gMultiCh_VcapVencVdisObj.mergeId[CAP_SC5_MERGE_LINK_IDX]    = SYSTEM_VPSS_LINK_ID_MERGE_2;
    gMultiCh_VcapVencVdisObj.dupId[CAP_DUP_LINK_IDX_0]          = SYSTEM_VPSS_LINK_ID_DUP_0;
    gMultiCh_VcapVencVdisObj.dupId[CAP_DUP_LINK_IDX_1]          = SYSTEM_VPSS_LINK_ID_DUP_1;
    gMultiCh_VcapVencVdisObj.ipcOutVpssId                       = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    gMultiCh_VcapVencVdisObj.ipcInVideoId                       = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;

    gVcapModuleContext.ipcFramesOutVpssToHostId  = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
    gVcapModuleContext.ipcFramesInHostId         = SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0;
    gVdisModuleContext.ipcFramesOutHostId        = SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0;
    gVdisModuleContext.ipcFramesInVpssFromHostId = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_0;


    CaptureLink_CreateParams_Init(&capturePrm);

    capturePrm.numVipInst               = 4;
    capturePrm.outQueParams[0].nextLink = gMultiCh_VcapVencVdisObj.dupId[CAP_DUP_LINK_IDX_0];
    capturePrm.outQueParams[1].nextLink = gMultiCh_VcapVencVdisObj.dupId[CAP_DUP_LINK_IDX_1];
    capturePrm.tilerEnable              = FALSE;
    capturePrm.maxBlindAreasPerCh       = 4;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = SYSTEM_CAPTURE_INST_VIP0_PORTA+vipInstId;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_TVP5158_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = SYSTEM_STD_MUX_4CH_D1;
        pCaptureInstPrm->numOutput          = 1;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = 0;
        pCaptureOutPrm->scOutHeight         = 0;
        if ((vipInstId >= 2) && !(enableOsdAlgLink & osdFormat))
          pCaptureOutPrm->outQueId          = 1;
        else
          pCaptureOutPrm->outQueId          = 0;
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


    dupPrm[CAP_DUP_LINK_IDX_0].inQueParams.prevLinkId    = gVcapModuleContext.captureId;
    dupPrm[CAP_DUP_LINK_IDX_0].inQueParams.prevLinkQueId = 0;
    dupPrm[CAP_DUP_LINK_IDX_0].notifyNextLink            = TRUE;
    dupPrm[CAP_DUP_LINK_IDX_0].numOutQue                 = 2;
    dupPrm[CAP_DUP_LINK_IDX_0].outQueParams[0].nextLink  = gVcapModuleContext.deiId[0];
    dupPrm[CAP_DUP_LINK_IDX_0].outQueParams[1].nextLink  = gMultiCh_VcapVencVdisObj.mergeId[CAP_SC5_MERGE_LINK_IDX];

    dupPrm[CAP_DUP_LINK_IDX_1].inQueParams.prevLinkId    = gVcapModuleContext.captureId;
    dupPrm[CAP_DUP_LINK_IDX_1].inQueParams.prevLinkQueId = 1;
    dupPrm[CAP_DUP_LINK_IDX_1].notifyNextLink            = TRUE;
    dupPrm[CAP_DUP_LINK_IDX_1].numOutQue                 = 2;
    dupPrm[CAP_DUP_LINK_IDX_1].outQueParams[0].nextLink  = gVcapModuleContext.deiId[1];
    dupPrm[CAP_DUP_LINK_IDX_1].outQueParams[1].nextLink  = gMultiCh_VcapVencVdisObj.mergeId[CAP_SC5_MERGE_LINK_IDX];

    mergePrm[CAP_SC5_MERGE_LINK_IDX].numInQue                     = 2;
    mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = gMultiCh_VcapVencVdisObj.dupId[CAP_DUP_LINK_IDX_0];
    mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = 1;
    mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = gMultiCh_VcapVencVdisObj.dupId[CAP_DUP_LINK_IDX_1];
    mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = 1;
    mergePrm[CAP_SC5_MERGE_LINK_IDX].notifyNextLink               = TRUE;
    mergePrm[CAP_SC5_MERGE_LINK_IDX].outQueParams.nextLink        = gVcapModuleContext.sclrId[0];

    for(i=0; i<NUM_DEI_LINK; i++)
    {
        Int32 chId;

        DeiLink_CreateParams_Init(&deiPrm[i]);

        deiPrm[i].inQueParams.prevLinkId                        = gMultiCh_VcapVencVdisObj.dupId[i];
        deiPrm[i].inQueParams.prevLinkQueId                     = 0;
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink               = gMultiCh_VcapVencVdisObj.mergeId[DEI_SC_D1_MERGE_LINK_IDX];
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink = gMultiCh_VcapVencVdisObj.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink               = gMultiCh_VcapVencVdisObj.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_DEI_SC]               = TRUE;
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = TRUE;
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC]               = TRUE;
        deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = enableTiler;
        deiPrm[i].comprEnable                                   = FALSE;
        deiPrm[i].setVipScYuv422Format                          = FALSE;
        deiPrm[i].enableDeiForceBypass                          = FALSE;
        deiPrm[i].enableLineSkipSc                              = FALSE;

        deiPrm[i].inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]  = 30;
        deiPrm[i].outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = 1;

        /* Set Output Scaling at DEI based on ratio */
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.denominator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.denominator = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0];

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].numInQue                     = NUM_DEI_LINK;
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = gVcapModuleContext.deiId[i];
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    }

    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].numInQue                     = 5;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = gVcapModuleContext.deiId[0];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = gVcapModuleContext.deiId[1];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[2].prevLinkId    = gVcapModuleContext.nsfId[0];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId = 0;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[3].prevLinkId    = gVcapModuleContext.deiId[0];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[3].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[4].prevLinkId    = gVcapModuleContext.deiId[1];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[4].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].notifyNextLink               = TRUE;

    if (enableOsdAlgLink || enableScdAlgLink)
    {
        mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].outQueParams.nextLink = gVcapModuleContext.ipcFramesOutVpssId[0];
    }
    else
    {
        mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].outQueParams.nextLink = gMultiCh_VcapVencVdisObj.ipcOutVpssId;
    }

    if (enableOsdAlgLink || enableScdAlgLink)
    {
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkId = gMultiCh_VcapVencVdisObj.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
        ipcFramesOutVpssPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm.baseCreateParams.notifyPrevLink = TRUE;

        ipcFramesOutVpssPrm.baseCreateParams.numOutQue = 1;
        ipcFramesOutVpssPrm.baseCreateParams.outQueParams[0].nextLink = gMultiCh_VcapVencVdisObj.ipcOutVpssId;
        ipcFramesOutVpssPrm.baseCreateParams.notifyNextLink = TRUE;

        ipcFramesOutVpssPrm.baseCreateParams.processLink = gVcapModuleContext.ipcFramesInDspId[0];
        ipcFramesOutVpssPrm.baseCreateParams.notifyProcessLink = TRUE;
        ipcFramesOutVpssPrm.baseCreateParams.noNotifyMode = FALSE;

        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
        ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm.baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm.baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[0];
        ipcFramesInDspPrm.baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesInDspPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm.baseCreateParams.noNotifyMode   = FALSE;

        dspAlgPrm.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
        dspAlgPrm.inQueParams.prevLinkQueId = 0;
    }

    SclrLink_CreateParams_Init(&sclrPrm);
    sclrPrm.inQueParams.prevLinkId             = gMultiCh_VcapVencVdisObj.mergeId[CAP_SC5_MERGE_LINK_IDX];
    sclrPrm.inQueParams.prevLinkQueId          = 0;
    sclrPrm.outQueParams.nextLink              = gVcapModuleContext.nsfId[0];
    sclrPrm.tilerEnable                        = FALSE;
    sclrPrm.enableLineSkipSc                   = TRUE;//FALSE;
    sclrPrm.inputFrameRate                     = 60;
    sclrPrm.outputFrameRate                    = 30;
    sclrPrm.scaleMode                          = DEI_SCALE_MODE_RATIO;
    sclrPrm.outScaleFactor.ratio.widthRatio.numerator = 1;
    sclrPrm.outScaleFactor.ratio.widthRatio.denominator = 2;
    sclrPrm.outScaleFactor.ratio.heightRatio.numerator = 1;
    sclrPrm.outScaleFactor.ratio.heightRatio.denominator = 1;

    NsfLink_CreateParams_Init(&nsfPrm);
    nsfPrm.bypassNsf = TRUE;
    nsfPrm.inputFrameRate  = 30;
    nsfPrm.outputFrameRate = 30;
    nsfPrm.tilerEnable     = FALSE;
    nsfPrm.inQueParams.prevLinkId    = gVcapModuleContext.sclrId[0];
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.numOutQue = 1;
    nsfPrm.outQueParams[0].nextLink  = gMultiCh_VcapVencVdisObj.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
    nsfPrm.numBufsPerCh = 6;

    if (enableVideoFramesExport)
    {
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].outQueParams.nextLink = gVcapModuleContext.ipcFramesOutVpssToHostId; // for display
    }
    else
    {
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].outQueParams.nextLink = gVdisModuleContext.swMsId[0];
    }
    mergePrm[DEI_SC_D1_MERGE_LINK_IDX].notifyNextLink        = TRUE;

    if (enableVideoFramesExport)
    {
        /* Set ipcFramesOutVpss link info */
        ipcFramesOutVpssToHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkId = gMultiCh_VcapVencVdisObj.mergeId[DEI_SC_D1_MERGE_LINK_IDX];
        ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssToHostPrm.baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.ipcFramesInHostId;

        ipcFramesInHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesInHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesInHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssToHostId;
        ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInHostPrm.baseCreateParams.outQueParams[0].nextLink = SYSTEM_LINK_ID_INVALID;
        ipcFramesInHostPrm.exportOnlyPhyAddr = TRUE;

        ipcFramesInHostPrm.cbCtx = &gVcapModuleContext;
        ipcFramesInHostPrm.cbFxn = Vcap_ipcFramesInCbFxn;

        ipcFramesOutHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesOutHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesOutHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdisModuleContext.ipcFramesInVpssFromHostId;

        ipcFramesInVpssFromHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkId = gVdisModuleContext.ipcFramesOutHostId;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInVpssFromHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdisModuleContext.swMsId[0];
        swMsPrm.inQueParams.prevLinkId    = gVdisModuleContext.ipcFramesInVpssFromHostId;
    }
    else
    {
        swMsPrm.inQueParams.prevLinkId    = gMultiCh_VcapVencVdisObj.mergeId[DEI_SC_D1_MERGE_LINK_IDX];
    }

    swMsPrm.inQueParams.prevLinkQueId = 0;
    swMsPrm.outQueParams.nextLink     = gVdisModuleContext.displayId[0];
    swMsPrm.maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
    swMsPrm.maxOutRes                 = VSYS_STD_1080P_60;
    swMsPrm.initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_HDMI].resolution;
    swMsPrm.lineSkipMode              = TRUE;
    swMsPrm.enableOuputDup = TRUE;
    swMsPrm.enableProcessTieWithDisplay = TRUE;

    swMsPrm.enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

    MultiCh_swMsGetDefaultLayoutPrm(0, &swMsPrm, FALSE);
    displayPrm.inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[0];

    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.displayRes                   = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_HDMI].resolution;

    if (enableOsdAlgLink || enableScdAlgLink)
    {
        ipcOutVpssPrm.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
    }
    else
    {
        ipcOutVpssPrm.inQueParams.prevLinkId = gMultiCh_VcapVencVdisObj.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
    }
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink  = gMultiCh_VcapVencVdisObj.ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = TRUE;

    ipcInVideoPrm.inQueParams.prevLinkId    = gMultiCh_VcapVencVdisObj.ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.outQueParams[0].nextLink     = gVencModuleContext.encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId    = gVencModuleContext.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink   = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm, TRUE);

    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkId    = gVencModuleContext.ipcBitsOutRTOSId;
    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInHostPrm[0].baseCreateParams.numOutQue                 = 1;
    ipcBitsInHostPrm[0].baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm[0]);

    encPrm.numBufPerCh[0] = 6; //D1
    encPrm.numBufPerCh[1] = 4; //CIF
    /* available buffers per channel with CIF and MJPEG encoder support is less*/
    if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
    {
        encPrm.numBufPerCh[0] = 10;
        encPrm.numBufPerCh[1] = 8;
        encPrm.numBufPerCh[2] = 6;
        encPrm.numBufPerCh[3] = 6;
    }

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

        if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
        {
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
        }

        /* MJPEG  Params */
        if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
        {
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
                pLinkDynPrm->inputFrameRate        = 1;//pDynPrm->inputFrameRate;
                pLinkDynPrm->qpMin                 = 0;
                pLinkDynPrm->qpMax                 = 0;
                pLinkDynPrm->qpInit                = -1;
                pLinkDynPrm->vbrDuration           = 0;
                pLinkDynPrm->vbrSensitivity        = 0;
            }
        }

        encPrm.inQueParams.prevLinkId   = gMultiCh_VcapVencVdisObj.ipcInVideoId;
        encPrm.inQueParams.prevLinkQueId= 0;
        encPrm.outQueParams.nextLink    = gVencModuleContext.ipcBitsOutRTOSId;
    }

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
       Int32    numBlksInFrame;
       Int32    numHorzBlks, numVertBlks, chIdx;
       Uint32  x, y, i;

        dspAlgPrm.enableSCDAlg              = TRUE;
        dspAlgPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = ipcBitsOutDSPId;
        dspAlgPrm.scdCreateParams.numBufPerCh            = 3;


        dspAlgPrm.scdCreateParams.maxWidth               = 352;
        if(Vcap_isPalMode())
           dspAlgPrm.scdCreateParams.maxHeight           = 288;
        else
           dspAlgPrm.scdCreateParams.maxHeight           = 240;

        dspAlgPrm.scdCreateParams.maxStride              = 352;
        dspAlgPrm.scdCreateParams.numValidChForSCD       = 16;

        dspAlgPrm.scdCreateParams.numSecs2WaitB4Init     = 3;
        dspAlgPrm.scdCreateParams.numSecs2WaitB4FrmAlert = 1;
        dspAlgPrm.scdCreateParams.inputFrameRate         = 30;
        dspAlgPrm.scdCreateParams.outputFrameRate        = 2;

        dspAlgPrm.scdCreateParams.numSecs2WaitAfterFrmAlert    = 2;
        dspAlgPrm.scdCreateParams.numSecs2Wait2MarkStableFrame = 5;

        dspAlgPrm.scdCreateParams.enableMotionNotify        = FALSE;
        dspAlgPrm.scdCreateParams.enableTamperNotify        = FALSE;

        /* Should be applied on CIF channels whose ch numbers are 4~7 */
        //dspAlgPrm.scdCreateParams.startChNoForSCD = 16;
        // Configure array to monitor scene changes in all frame blocks, i.e., motion detection.
        // Each block is fixed to be 32x10 in size,
        numHorzBlks     = dspAlgPrm.scdCreateParams.maxWidth / 32;
        if(dspAlgPrm.scdCreateParams.maxHeight == 240)
           numVertBlks    = dspAlgPrm.scdCreateParams.maxHeight / 10;
        else   /* For 288 Block height becomes 12 */
           numVertBlks    = dspAlgPrm.scdCreateParams.maxHeight / 12;

        numBlksInFrame  = numHorzBlks * numVertBlks;

        for(chIdx = 0; chIdx < dspAlgPrm.scdCreateParams.numValidChForSCD; chIdx++)
        {
           AlgLink_ScdChParams * chPrm = &dspAlgPrm.scdCreateParams.chDefaultParams[chIdx];

           chPrm->blkNumBlksInFrame  = numBlksInFrame;
           chPrm->chId               = SCDChannelMonitor[chIdx];
           chPrm->mode               = ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
           chPrm->frmIgnoreLightsON  = FALSE;
           chPrm->frmIgnoreLightsOFF = FALSE;
           chPrm->frmSensitivity     = ALG_LINK_SCD_SENSITIVITY_VERYLOW;//ALG_LINK_SCD_SENSITIVITY_VERYHIGH;
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

        ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkId    = gVcapModuleContext.dspAlgId[0];
        ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcBitsOutDspPrm.baseCreateParams.numOutQue                 = 1;
        ipcBitsOutDspPrm.baseCreateParams.outQueParams[0].nextLink  = gVcapModuleContext.ipcBitsInHLOSId;
        MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutDspPrm,
                                                   TRUE);
        ipcBitsOutDspPrm.baseCreateParams.notifyNextLink              = TRUE;
        ipcBitsOutDspPrm.baseCreateParams.noNotifyMode                = FALSE;

        ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkId = ipcBitsOutDSPId;
        ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcBitsInHostPrm[1].baseCreateParams.numOutQue                 = 1;
        ipcBitsInHostPrm[1].baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;
        MultiCh_ipcBitsInitCreateParams_BitsInHLOSVcap(&ipcBitsInHostPrm[1]);
        ipcBitsInHostPrm[1].baseCreateParams.notifyPrevLink         = TRUE;
        ipcBitsInHostPrm[1].baseCreateParams.noNotifyMode              = FALSE;

    }
    else
    {
        dspAlgPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;//nullId;
    }

#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif
    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));
    for (i=0; i<NUM_DUP_LINK; i++)
        System_linkCreate(gMultiCh_VcapVencVdisObj.dupId[i], &dupPrm[i], sizeof(dupPrm[i]));

    System_linkCreate(  gMultiCh_VcapVencVdisObj.mergeId [CAP_SC5_MERGE_LINK_IDX],
                            &mergePrm [CAP_SC5_MERGE_LINK_IDX],
                            sizeof(mergePrm [CAP_SC5_MERGE_LINK_IDX])
                        );

    System_linkCreate(gVcapModuleContext.sclrId[0], &sclrPrm, sizeof(sclrPrm));

    System_linkCreate(gVcapModuleContext.nsfId[0], &nsfPrm, sizeof(nsfPrm));

    for(i=0; i<NUM_DEI_LINK; i++)
        System_linkCreate(gVcapModuleContext.deiId[i]  , &deiPrm[i], sizeof(deiPrm[i]));

    System_linkCreate(  gMultiCh_VcapVencVdisObj.mergeId [DEI_SC_D1_MERGE_LINK_IDX],
                            &mergePrm [DEI_SC_D1_MERGE_LINK_IDX],
                            sizeof(mergePrm [DEI_SC_D1_MERGE_LINK_IDX])
                        );

    System_linkCreate(  gMultiCh_VcapVencVdisObj.mergeId [DEI_VIP_SC_MERGE_LINK_IDX],
                        &mergePrm [DEI_VIP_SC_MERGE_LINK_IDX],
                        sizeof(mergePrm [DEI_VIP_SC_MERGE_LINK_IDX])
                    );

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

    System_linkCreate(gMultiCh_VcapVencVdisObj.ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(gMultiCh_VcapVencVdisObj.ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );

    System_linkCreate(gVencModuleContext.encId, &encPrm, sizeof(encPrm));
    System_linkCreate(gVencModuleContext.ipcBitsOutRTOSId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(gVencModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm[0], sizeof(ipcBitsInHostPrm[0]));

    if(enableVideoFramesExport)
    {
        System_linkCreate(gVcapModuleContext.ipcFramesOutVpssToHostId     , &ipcFramesOutVpssToHostPrm    , sizeof(ipcFramesOutVpssToHostPrm));
        System_linkCreate(gVcapModuleContext.ipcFramesInHostId     , &ipcFramesInHostPrm    , sizeof(ipcFramesInHostPrm));
        System_linkGetInfo(gVcapModuleContext.ipcFramesInHostId,&framesProducerLinkInfo);
        OSA_assert(framesProducerLinkInfo.numQue == 1);
        ipcFramesOutHostPrm.inQueInfo = framesProducerLinkInfo.queInfo[0];
        System_linkCreate(gVdisModuleContext.ipcFramesOutHostId     , &ipcFramesOutHostPrm    , sizeof(ipcFramesOutHostPrm));
        System_linkCreate(gVdisModuleContext.ipcFramesInVpssFromHostId     , &ipcFramesInVpssFromHostPrm    , sizeof(ipcFramesInVpssFromHostPrm));
    }

    System_linkCreate(gVdisModuleContext.swMsId[0]  , &swMsPrm, sizeof(swMsPrm));
    System_linkCreate(gVdisModuleContext.displayId[0], &displayPrm, sizeof(displayPrm));

    MultiCh_memPrintHeapStatus();

}


Void MultiCh_deleteVcapVencVdis()
{
    UInt32 i;
    UInt32 ipcBitsOutDSPId;

    Bool enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;
    Bool enableVideoFramesExport = TRUE;//FALSE;

    /* delete can be done in any order */
    ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;


    Vcap_delete();
    Venc_delete();
    Vdis_delete();

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

    for(i=0; i<NUM_MERGE_LINK; i++)
    {
        System_linkDelete(gMultiCh_VcapVencVdisObj.mergeId[i]);
    }

    for (i=0; i<NUM_DUP_LINK; i++)
    {
        System_linkDelete(gMultiCh_VcapVencVdisObj.dupId[i]);
    }

    System_linkDelete(gVcapModuleContext.nsfId[0]);

    System_linkDelete(gVcapModuleContext.sclrId[0]);

    System_linkDelete(gMultiCh_VcapVencVdisObj.ipcOutVpssId );
    System_linkDelete(gMultiCh_VcapVencVdisObj.ipcInVideoId );

    if(enableVideoFramesExport)
    {
        System_linkDelete(gVcapModuleContext.ipcFramesOutVpssToHostId);
        System_linkDelete(gVcapModuleContext.ipcFramesInHostId);
        System_linkDelete(gVdisModuleContext.ipcFramesOutHostId);
        System_linkDelete(gVdisModuleContext.ipcFramesInVpssFromHostId);
    }
    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

    /* Reenable tiler allocator taht was disabled by this usecase
     * at delete time.
     */
    SystemTiler_enableAllocator();

}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/*
                                          Capture (4CH 1080p30 422)
                                           ******************
                                                |         |
                                               DUP0      DUP1
                                               | | |     | | |
                                               | | |     | | +---------------------------+
                                               | | |  +--+ |                             |
                                               | | +--[----[--------------------------+  |
                                               | |    |    +--+                       |  |
                                               | +----[----+  |                      MERGE1
                                               |      |    |  |                        |
                                               | +----+  MERGE0                        |
                                               | |          |                          |
                                               | |          +----- SC5 ------+         |
                                               | |             (4CH D1 422)  |        SC5
                                           +---+ +--------+                  |     (4CH 422) (MJPEG)
                                           |              |                  |         |
                                         DEIH (BP-Mode)  DEI (BP-Mode)       |         |
                                          | |            | |                 |  +------+
                              2CH 1080p30 | |            | | 2CH 1080p30     |  |
                                          | |            | |                 |  |
                               +----------+ +-----+  +---+ +---+             |  |
                               |                  |  |            |          |  |
                               |          +-------[--+            |         MERGE2
                               |          |       |               |          |  |
                               |   (DEI-SC2 422)  +--+            |          |  |
                               |          |          |            |          |  |
                               |          |          |       (VIP-SC4 420)   |  |
                          (DEI-SC1 422)   |   (VIP-SC3 420)       |          NSF
                               |          |          |            |           |(4CH D1 420) + 4Ch MJEPG
                               +--+   +---+          +----+       |           |
                                  |   |                   |       |           |
                                  MERGE3                 |       +--------+  |
                                    |                     +--------------+ |  |
                                   DUP2------------------+               | |  |
                                    |                    |               | |  |
    +--<<<processLink>>>--- IPC Frames Out (M3)          |               | |  |
    |                               |                    |               MERGE4
    |                       IPC Frames IN (A8)           |                 |
  FramesInDSP                       |                    |                 |
    |                       IPC Frames Out (A8)          |              IPC OUT(M3)----<<<processLink>>>---FramesInDSP--+
 ALG LINK                           |                    |                 |                                            |
 <OSD SCD Algs>             IPC Frames IN (M3)           |              IPC IN(M3)                                      |
                                    |              On-Chip HDMI            |                                            |
                              OFF-Chip HDMI          1080p60      Encode (4CH 1080p30 + 4CH D1)                      ALG_LINK
                                 1080p60          (1-Ch 1-Window)          |                                       <OSD, SCD Algs>
                             (1-Ch 1-Window)                          IPC Bits OUT (M3)
                                                                           |
                                                                      IPC Bits IN (A8)

(BP-Mode) --> Bypass Mode
*/

#include "multich_common.h"

#include "mcfw/interfaces/link_api/system_tiler.h"


#define     NUM_CAPTURE_DEVICES     (4)

/* =============================================================================
 * Externs
 * =============================================================================
 */

static UInt8 SCDChannelMonitor[4] = {4, 5, 6, 7};

//#define TWOOSD_INSTANCE

typedef struct {

    UInt32 mergeId[5];
    UInt32 dupId[3];
    UInt32 ipcOutVpssId;
    UInt32 ipcInVideoId;
    UInt32 ipcFrameOutVpssId[2];
    UInt32 ipcFramesInDspId[2];
} MultiChHd_VcapVencVdisObj;

MultiChHd_VcapVencVdisObj gMultiChHd_VcapVencVdisObj;

/* =============================================================================
 * Use case code
 * =============================================================================
 */

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 2,
        .EncChList = {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 2,
        .EncChList = {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 8,
        .EncChList = {4, 5, 6, 7, 8, 9, 10, 11, 0, 0, 0, 0, 0, 0 , 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

Void MultiChHd_createVcapVencVdis()
{
    CaptureLink_CreateParams        capturePrm;
    DeiLink_CreateParams            deiPrm[MAX_DEI_LINK];
    MergeLink_CreateParams          mergePrm[5];
    DupLink_CreateParams            dupPrm[3];
    SclrLink_CreateParams           sclrPrm[2];
    DisplayLink_CreateParams        displayPrm[2];
    NsfLink_CreateParams            nsfPrm[2];
    IpcLink_CreateParams            ipcOutVpssPrm;
    IpcLink_CreateParams            ipcInVideoPrm;
    EncLink_CreateParams            encPrm;
    IpcBitsOutLinkRTOS_CreateParams ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams  ipcBitsInHostPrm[2];
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssToHostPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInHostPrm;
    IpcFramesOutLinkHLOS_CreateParams  ipcFramesOutHostPrm;
    IpcFramesInLinkRTOS_CreateParams   ipcFramesInVpssFromHostPrm;

    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm[2];
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm[2];
    AlgLink_CreateParams                dspAlgPrm[2];

    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm;
    UInt32 ipcBitsOutDSPId;

    System_LinkInfo framesProducerLinkInfo;

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;
    UInt32 vipInstId, i, j;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

    UInt32 numSubChains;
    Bool   enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool   enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;
#ifdef TWOOSD_INSTANCE
    Bool   enableOsdAlgLinkA8Path = TRUE;
    Bool   enableScdAlgLinkA8Path = TRUE;
#else
    Bool   enableOsdAlgLinkA8Path = FALSE;
    Bool   enableScdAlgLinkA8Path = FALSE;
#endif
    Bool   enableTiler;
    Uint32 noOfDSPAlgo = 2;


    if(enableOsdAlgLink == FALSE)
        enableOsdAlgLinkA8Path = FALSE;

    if(enableScdAlgLink == FALSE)
        enableScdAlgLinkA8Path = FALSE;


    if((enableOsdAlgLinkA8Path == FALSE) && (enableScdAlgLinkA8Path == FALSE))
        noOfDSPAlgo = 1;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams           ,ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams           ,ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams ,ipcBitsInHostPrm[0]);

    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssToHostPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInHostPrm);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkHLOS_CreateParams ,ipcFramesOutHostPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  ,ipcFramesInVpssFromHostPrm);

    if(enableOsdAlgLink || enableScdAlgLink)
    {
        for (i = 0; i < noOfDSPAlgo;i++)
        {
           MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm[i]);
           MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm[i]);
           MULTICH_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm[i]);
        }
        MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams ,ipcBitsInHostPrm[1]);
    }


    MULTICH_INIT_STRUCT(EncLink_CreateParams, encPrm);
    for (i = 0; i < 2;i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams ,displayPrm[i]);
    }

    MultiCh_detectBoard();

    System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES,
        NULL,
        0,
        TRUE
        );
    /* Disable tiler allocator for this usecase
     * for that tiler memory can be reused for
     * non-tiled allocation
     */
    SystemTiler_disableAllocator();



    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    enableTiler                     = FALSE;
    numSubChains                    = 2;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    gVcapModuleContext.sclrId[0]       = SYSTEM_LINK_ID_SCLR_INST_0;
    gVcapModuleContext.sclrId[1]       = SYSTEM_LINK_ID_SCLR_INST_1;

    gVcapModuleContext.nsfId[0]        = SYSTEM_LINK_ID_NSF_0;

    gVdisModuleContext.swMsId[0]    = SYSTEM_LINK_ID_INVALID;


    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; /* OFF CHIP HDMI */

    gVencModuleContext.encId            = SYSTEM_LINK_ID_VENC_0;
    if(enableScdAlgLink)
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
    else
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

    gVencModuleContext.ipcBitsOutRTOSId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;

    gMultiChHd_VcapVencVdisObj.mergeId[0]        = SYSTEM_VPSS_LINK_ID_MERGE_0;
    gMultiChHd_VcapVencVdisObj.mergeId[1]        = SYSTEM_VPSS_LINK_ID_MERGE_1;
    gMultiChHd_VcapVencVdisObj.mergeId[2]        = SYSTEM_VPSS_LINK_ID_MERGE_2;
    gMultiChHd_VcapVencVdisObj.mergeId[3]        = SYSTEM_VPSS_LINK_ID_MERGE_3;
    gMultiChHd_VcapVencVdisObj.mergeId[4]        = SYSTEM_VPSS_LINK_ID_MERGE_4;

    gMultiChHd_VcapVencVdisObj.dupId[0]          = SYSTEM_VPSS_LINK_ID_DUP_0;
    gMultiChHd_VcapVencVdisObj.dupId[1]          = SYSTEM_VPSS_LINK_ID_DUP_1;
    gMultiChHd_VcapVencVdisObj.dupId[2]          = SYSTEM_VPSS_LINK_ID_DUP_2;

    if(enableOsdAlgLink || enableScdAlgLink)
    {
        gVcapModuleContext.dspAlgId[0]                  = SYSTEM_LINK_ID_ALG_0;
        gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[0] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
        gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[0]  = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
        if(enableScdAlgLink)
        {
          ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
          gVcapModuleContext.ipcBitsInHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
        }
    }
    if(enableOsdAlgLinkA8Path || enableScdAlgLinkA8Path)
    {
        gVcapModuleContext.dspAlgId[1]                  = SYSTEM_LINK_ID_ALG_1;
        gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[1] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
        gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[1]  = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_1;
        gVcapModuleContext.ipcFramesOutVpssToHostId     = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_2;
    }
    else
    {
        gVcapModuleContext.ipcFramesOutVpssToHostId  = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
    }
    gMultiChHd_VcapVencVdisObj.ipcOutVpssId      = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    gMultiChHd_VcapVencVdisObj.ipcInVideoId      = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;


    gVcapModuleContext.ipcFramesInHostId         = SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0;

    gVdisModuleContext.ipcFramesOutHostId        = SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0;
    gVdisModuleContext.ipcFramesInVpssFromHostId = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_0;

    CaptureLink_CreateParams_Init(&capturePrm);

    capturePrm.numVipInst               = 2*numSubChains; /* Need to change based on actual HD video decoder */
    capturePrm.outQueParams[0].nextLink = gMultiChHd_VcapVencVdisObj.dupId[0];
    capturePrm.outQueParams[1].nextLink = gMultiChHd_VcapVencVdisObj.dupId[1];
    capturePrm.tilerEnable              = FALSE;
    capturePrm.fakeHdMode               = TRUE;
    capturePrm.enableSdCrop             = FALSE;
    capturePrm.doCropInCapture          = FALSE;
    capturePrm.maxBlindAreasPerCh       = 4;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = (SYSTEM_CAPTURE_INST_VIP0_PORTA+vipInstId)%SYSTEM_CAPTURE_INST_MAX; /* Need to change based on actual HD video decoder */
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_TVP5158_DRV; /* Need to change based on actual HD video decoder */
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = SYSTEM_STD_D1; /* Need to change based on actual HD video decoder */
        pCaptureInstPrm->numOutput          = 1; /* Need to change based on actual HD video decoder */

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;//SYSTEM_DF_YUV420SP_UV;
        pCaptureOutPrm->scEnable            = FALSE; /* Need to change based on actual HD video decoder */
        pCaptureOutPrm->scOutWidth          = 704; /* Need to change based on actual HD video decoder */
        pCaptureOutPrm->scOutHeight         = 240; /* Need to change based on actual HD video decoder */
        pCaptureOutPrm->outQueId            = 0;

        if (vipInstId >= numSubChains)
          pCaptureOutPrm->outQueId          = 1;
        else
          pCaptureOutPrm->outQueId          = 0;
    }

    for(i = 0; i < NUM_CAPTURE_DEVICES; i++)
    {
        vidDecVideoModeArgs[i].videoIfMode        = DEVICE_CAPT_VIDEO_IF_MODE_8BIT;
        vidDecVideoModeArgs[i].videoDataFormat    = SYSTEM_DF_YUV422P;
        vidDecVideoModeArgs[i].standard           = SYSTEM_STD_D1;
        vidDecVideoModeArgs[i].videoCaptureMode   =
                    DEVICE_CAPT_VIDEO_CAPTURE_MODE_SINGLE_CH_NON_MUX_EMBEDDED_SYNC;
        vidDecVideoModeArgs[i].videoSystem        =
                                      DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT;
        vidDecVideoModeArgs[i].videoCropEnable    = FALSE;
        vidDecVideoModeArgs[i].videoAutoDetectTimeout = -1;
    }

    Vcap_configVideoDecoder(vidDecVideoModeArgs, NUM_CAPTURE_DEVICES);

    dupPrm[0].inQueParams.prevLinkId        = gVcapModuleContext.captureId;
    dupPrm[0].inQueParams.prevLinkQueId     = 0;
    dupPrm[0].numOutQue                     = 3;
    dupPrm[0].outQueParams[0].nextLink      = gVcapModuleContext.deiId[0];
    dupPrm[0].outQueParams[1].nextLink      = gMultiChHd_VcapVencVdisObj.mergeId[0];
    dupPrm[0].outQueParams[2].nextLink      = gMultiChHd_VcapVencVdisObj.mergeId[1];
    dupPrm[0].notifyNextLink                = TRUE;

    dupPrm[1].inQueParams.prevLinkId        = gVcapModuleContext.captureId;
    dupPrm[1].inQueParams.prevLinkQueId     = 1;
    dupPrm[1].numOutQue                     = 3;
    dupPrm[1].outQueParams[0].nextLink      = gVcapModuleContext.deiId[1];
    dupPrm[1].outQueParams[1].nextLink      = gMultiChHd_VcapVencVdisObj.mergeId[0];
    dupPrm[1].outQueParams[2].nextLink      = gMultiChHd_VcapVencVdisObj.mergeId[1];
    dupPrm[1].notifyNextLink                = TRUE;

    mergePrm[0].numInQue                     = 2;
    mergePrm[0].inQueParams[0].prevLinkId    = gMultiChHd_VcapVencVdisObj.dupId[0];
    mergePrm[0].inQueParams[0].prevLinkQueId = 1;
    mergePrm[0].inQueParams[1].prevLinkId    = gMultiChHd_VcapVencVdisObj.dupId[1];
    mergePrm[0].inQueParams[1].prevLinkQueId = 1;
    mergePrm[0].outQueParams.nextLink        = gVcapModuleContext.sclrId[0];
    mergePrm[0].notifyNextLink               = TRUE;

    mergePrm[1].numInQue                     = 2;
    mergePrm[1].inQueParams[0].prevLinkId    = gMultiChHd_VcapVencVdisObj.dupId[0];
    mergePrm[1].inQueParams[0].prevLinkQueId = 2;
    mergePrm[1].inQueParams[1].prevLinkId    = gMultiChHd_VcapVencVdisObj.dupId[1];
    mergePrm[1].inQueParams[1].prevLinkQueId = 2;
    mergePrm[1].outQueParams.nextLink        = gVcapModuleContext.sclrId[1];
    mergePrm[1].notifyNextLink               = TRUE;

    for(i=0; i<numSubChains; i++)
    {
        Int32 chId;

        DeiLink_CreateParams_Init(&deiPrm[i]);

        deiPrm[i].inQueParams.prevLinkId                           = gMultiChHd_VcapVencVdisObj.dupId[i];
        deiPrm[i].inQueParams.prevLinkQueId                        = 0;
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_DEI_SC]               = TRUE;
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC]               = TRUE;
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = FALSE;
        deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]             = enableTiler;

        /* Set non default DEI Frame Rate */
        deiPrm[i].inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC]       = 30;
        deiPrm[i].outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC]      = 30;

        /* Set non default DEI Frame Rate */
        deiPrm[i].inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]       = 30;
        deiPrm[i].outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]      = 30;

        deiPrm[i].comprEnable                                   = FALSE;
        deiPrm[i].setVipScYuv422Format                          = FALSE;
        deiPrm[i].enableDeiForceBypass                          = TRUE;
        deiPrm[i].enableLineSkipSc                              = FALSE;

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode  = DEI_SCALE_MODE_RATIO;
        /* DEI Path Scalar ratio is 1:2, D1 => CIF */
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator = 1;

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode  = DEI_SCALE_MODE_RATIO;
        /* DEI Path Scalar ratio is 1:2, D1 => CIF */
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator = 1;

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];

        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink= gMultiChHd_VcapVencVdisObj.mergeId[3];
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink= gMultiChHd_VcapVencVdisObj.mergeId[4];
    }

    mergePrm[3].numInQue                     = numSubChains;
    mergePrm[3].inQueParams[0].prevLinkId    = gVcapModuleContext.deiId[0];
    mergePrm[3].inQueParams[0].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    mergePrm[3].inQueParams[1].prevLinkId    = gVcapModuleContext.deiId[1];
    mergePrm[3].inQueParams[1].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    if(enableOsdAlgLinkA8Path || enableScdAlgLinkA8Path)
    {
         mergePrm[3].outQueParams.nextLink        = gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[1];
    }
    else
    {
         mergePrm[3].outQueParams.nextLink        = gMultiChHd_VcapVencVdisObj.dupId[2];
    }

    mergePrm[3].notifyNextLink               = TRUE;

    mergePrm[4].numInQue                     = 3;
    mergePrm[4].inQueParams[0].prevLinkId    = gVcapModuleContext.deiId[0];
    mergePrm[4].inQueParams[0].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[4].inQueParams[1].prevLinkId    = gVcapModuleContext.deiId[1];
    mergePrm[4].inQueParams[1].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[4].inQueParams[2].prevLinkId    = gVcapModuleContext.nsfId[0];
    mergePrm[4].inQueParams[2].prevLinkQueId = 0;
    mergePrm[4].notifyNextLink               = TRUE;

    if(enableOsdAlgLink || enableScdAlgLink)
    {
        mergePrm[4].outQueParams.nextLink        = gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[0];
        ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkId = gMultiChHd_VcapVencVdisObj.mergeId[4];
        ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyPrevLink = TRUE;

        ipcFramesOutVpssPrm[0].baseCreateParams.numOutQue = 1;
        ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[0].nextLink = gMultiChHd_VcapVencVdisObj.ipcOutVpssId;
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesOutVpssPrm[0].baseCreateParams.noNotifyMode = FALSE;

        ipcFramesOutVpssPrm[0].baseCreateParams.processLink = gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[0];
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyProcessLink = TRUE;

        ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkId = gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[0];
        ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm[0].baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[0];
        ipcFramesInDspPrm[0].baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesInDspPrm[0].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm[0].baseCreateParams.noNotifyMode   = FALSE;

        dspAlgPrm[0].inQueParams.prevLinkId = gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[0];
        dspAlgPrm[0].inQueParams.prevLinkQueId = 0;

    }
    else
    {
        mergePrm[4].outQueParams.nextLink        = gMultiChHd_VcapVencVdisObj.ipcOutVpssId;
    }

    SclrLink_CreateParams_Init(&sclrPrm[0]);
    sclrPrm[0].inQueParams.prevLinkId                        = gMultiChHd_VcapVencVdisObj.mergeId[0];
    sclrPrm[0].inQueParams.prevLinkQueId                     = 0;
    sclrPrm[0].outQueParams.nextLink                         = gMultiChHd_VcapVencVdisObj.mergeId[2];
    sclrPrm[0].enableLineSkipSc                              = TRUE;//FALSE;
    sclrPrm[0].inputFrameRate                                = 30;
    sclrPrm[0].outputFrameRate                               = 30;
    sclrPrm[0].numBufsPerCh                                  = 3;
#if 0
    sclrPrm[0].scaleMode                                     = SCLR_SCALE_MODE_RATIO;

    sclrPrm[0].outScaleFactor.ratio.widthRatio.numerator    = 1;
    sclrPrm[0].outScaleFactor.ratio.widthRatio.denominator  = 2;

    sclrPrm[0].outScaleFactor.ratio.heightRatio.numerator   = 1;
    sclrPrm[0].outScaleFactor.ratio.heightRatio.denominator = 2;
#endif
#if 1
    /* Set Output Scaling at DEI to absolute values */
    sclrPrm[0].scaleMode  = DEI_SCALE_MODE_ABSOLUTE;

    /* DEI Path Scalar <for secondary out> is set to CIF */
    sclrPrm[0].outScaleFactor.absoluteResolution.outWidth   = 704;
    sclrPrm[0].outScaleFactor.absoluteResolution.outHeight  = 480;
#endif

    SclrLink_CreateParams_Init(&sclrPrm[1]);
    sclrPrm[1].inQueParams.prevLinkId                        = gMultiChHd_VcapVencVdisObj.mergeId[1];
    sclrPrm[1].inQueParams.prevLinkQueId                     = 0;
    sclrPrm[1].outQueParams.nextLink                         = gMultiChHd_VcapVencVdisObj.mergeId[2];
    sclrPrm[1].enableLineSkipSc                              = FALSE;
    sclrPrm[1].inputFrameRate                                = 30;
    sclrPrm[1].outputFrameRate                               = 30;
    sclrPrm[1].numBufsPerCh                                  = 3;
#if 0
    sclrPrm[1].scaleMode                                     = SCLR_SCALE_MODE_RATIO;

    sclrPrm[1].outScaleFactor.ratio.widthRatio.numerator    = 1;
    sclrPrm[1].outScaleFactor.ratio.widthRatio.denominator  = 2;

    sclrPrm[1].outScaleFactor.ratio.heightRatio.numerator   = 1;
    sclrPrm[1].outScaleFactor.ratio.heightRatio.denominator = 2;
#endif
#if 1
    /* Set Output Scaling at DEI to absolute values */
    sclrPrm[1].scaleMode  = DEI_SCALE_MODE_ABSOLUTE;

    /* DEI Path Scalar <for secondary out> is set to CIF */
    sclrPrm[1].outScaleFactor.absoluteResolution.outWidth   = 704;
    sclrPrm[1].outScaleFactor.absoluteResolution.outHeight  = 480;
#endif

    mergePrm[2].numInQue                     = 2;
    mergePrm[2].inQueParams[0].prevLinkId    = gVcapModuleContext.sclrId[0];
    mergePrm[2].inQueParams[0].prevLinkQueId = 0;
    mergePrm[2].inQueParams[1].prevLinkId    = gVcapModuleContext.sclrId[1];
    mergePrm[2].inQueParams[1].prevLinkQueId = 0;
    mergePrm[2].outQueParams.nextLink        = gVcapModuleContext.nsfId[0];
    mergePrm[2].notifyNextLink               = TRUE;

    NsfLink_CreateParams_Init(&nsfPrm[0]);
    nsfPrm[0].bypassNsf                        = TRUE;
    nsfPrm[0].tilerEnable                      = enableTiler;
    nsfPrm[0].inQueParams.prevLinkId           = gMultiChHd_VcapVencVdisObj.mergeId[2];
    nsfPrm[0].inQueParams.prevLinkQueId        = 0;
    nsfPrm[0].numOutQue                        = 1;
    nsfPrm[0].numBufsPerCh                     = 4;
    nsfPrm[0].inputFrameRate                   = 30;
    nsfPrm[0].outputFrameRate                  = 30;
    nsfPrm[0].outQueParams[0].nextLink         = gMultiChHd_VcapVencVdisObj.mergeId[4];

    if(enableOsdAlgLinkA8Path || enableScdAlgLinkA8Path)
    {
        ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkId = gMultiChHd_VcapVencVdisObj.mergeId[3];
        ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm[1].baseCreateParams.notifyPrevLink = TRUE;

        ipcFramesOutVpssPrm[1].baseCreateParams.numOutQue = 1;
        ipcFramesOutVpssPrm[1].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.ipcFramesOutVpssToHostId; // A8
        ipcFramesOutVpssPrm[1].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesOutVpssPrm[1].baseCreateParams.noNotifyMode = FALSE;

        ipcFramesOutVpssPrm[1].baseCreateParams.processLink = gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[1];
        ipcFramesOutVpssPrm[1].baseCreateParams.notifyProcessLink = TRUE;

        ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkId = gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[1];
        ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm[1].baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm[1].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[1];
        ipcFramesInDspPrm[1].baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesInDspPrm[1].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm[1].baseCreateParams.noNotifyMode   = FALSE;

        dspAlgPrm[1].inQueParams.prevLinkId = gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[1];
        dspAlgPrm[1].inQueParams.prevLinkQueId = 0;


        dupPrm[2].inQueParams.prevLinkId        = gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[1];
    }
    else
    {
        dupPrm[2].inQueParams.prevLinkId        = gMultiChHd_VcapVencVdisObj.mergeId[3];
    }


    dupPrm[2].inQueParams.prevLinkQueId     = 0;
    dupPrm[2].numOutQue                     = 2; /* display and encode and substream scaling */
    dupPrm[2].outQueParams[0].nextLink      = gVcapModuleContext.ipcFramesOutVpssToHostId; // A8
    dupPrm[2].outQueParams[1].nextLink      = gVdisModuleContext.displayId[0]; // for display
    dupPrm[2].notifyNextLink                = TRUE;



    /* Set ipcFramesOutVpss link info */
    ipcFramesOutVpssToHostPrm.baseCreateParams.noNotifyMode = TRUE;
    ipcFramesOutVpssToHostPrm.baseCreateParams.notifyNextLink = FALSE;
    ipcFramesOutVpssToHostPrm.baseCreateParams.notifyPrevLink = TRUE;
    ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkId = gMultiChHd_VcapVencVdisObj.dupId[2];
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
    ipcFramesInVpssFromHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdisModuleContext.displayId[1];

    displayPrm[0].inQueParams[0].prevLinkId        = gVdisModuleContext.ipcFramesInVpssFromHostId;
    displayPrm[0].inQueParams[0].prevLinkQueId     = 0;

    displayPrm[0].displayRes                    = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_HDMI].resolution;//displayRes[SYSTEM_DC_VENC_HDMI];

    displayPrm[1].inQueParams[0].prevLinkId        = gMultiChHd_VcapVencVdisObj.dupId[2];
    displayPrm[1].inQueParams[0].prevLinkQueId     = 1;
    displayPrm[1].displayRes                    = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_HDCOMP].resolution;//displayRes[SYSTEM_DC_VENC_HDCOMP];


    if(enableOsdAlgLink || enableScdAlgLink)
    {
        ipcOutVpssPrm.inQueParams.prevLinkId    = gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[0];
    }
    else
    {
        ipcOutVpssPrm.inQueParams.prevLinkId    = gMultiChHd_VcapVencVdisObj.mergeId[4];
    }

    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.outQueParams[0].nextLink  = gMultiChHd_VcapVencVdisObj.ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = TRUE;//FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = FALSE;

    ipcInVideoPrm.inQueParams.prevLinkId    = gMultiChHd_VcapVencVdisObj.ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.outQueParams[0].nextLink  = gVencModuleContext.encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId    = gVencModuleContext.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink   = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm, TRUE);

    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkId    = gVencModuleContext.ipcBitsOutRTOSId;
    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInHostPrm[0].baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm[0]);

    encPrm.numBufPerCh[0] = 6; //D1
    encPrm.numBufPerCh[1] = 4; //CIF
    /* available buffers per channel with CIF and MJPEG encoder support is less*/
    if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
    {
        encPrm.numBufPerCh[0] = 3;
        encPrm.numBufPerCh[1] = 3;
        encPrm.numBufPerCh[2] = 3;
        encPrm.numBufPerCh[3] = 3;
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
                      i<(3*gVencModuleContext.vencConfig.numPrimaryChn); i++)
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

        encPrm.inQueParams.prevLinkId   = gMultiChHd_VcapVencVdisObj.ipcInVideoId;
        encPrm.inQueParams.prevLinkQueId= 0;
        encPrm.outQueParams.nextLink    = gVencModuleContext.ipcBitsOutRTOSId;
    }




    if(enableOsdAlgLink)
    {
        int chId;
        dspAlgPrm[0].enableOSDAlg = FALSE;
        dspAlgPrm[1].enableOSDAlg = FALSE;
        for(i = 0; i< noOfDSPAlgo; i++)
        {
            dspAlgPrm[i].enableOSDAlg = TRUE;

            for(chId = 0; chId < ALG_LINK_OSD_MAX_CH; chId++)
            {
                AlgLink_OsdChWinParams * chWinPrm = &dspAlgPrm[i].osdChCreateParams[chId].chDefaultParams;

                /* set osd window max width and height */
                dspAlgPrm[i].osdChCreateParams[chId].maxWidth  = EXAMPLE_OSD_WIN_MAX_WIDTH;
                dspAlgPrm[i].osdChCreateParams[chId].maxHeight = EXAMPLE_OSD_WIN_MAX_HEIGHT;

                chWinPrm->chId = chId;
                chWinPrm->numWindows = 0;

                }
            }
        }

    if (enableScdAlgLink)
    {
       Int32    numBlksInFrame;
       Int32    numHorzBlks, numVertBlks, chIdx;
       Uint32  x, y, i;
   //AlgLink_ScdblkChngConfig   blkConfig[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];
       dspAlgPrm[0].enableSCDAlg = FALSE;
       dspAlgPrm[1].enableSCDAlg = FALSE;
       dspAlgPrm[0].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = ipcBitsOutDSPId;
       dspAlgPrm[1].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;

       for(i = 0; i< 1; i++)
       {
         dspAlgPrm[i].enableSCDAlg = TRUE;
         dspAlgPrm[i].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = ipcBitsOutDSPId;

         dspAlgPrm[i].scdCreateParams.maxWidth = 352;
        if(Vcap_isPalMode())
           dspAlgPrm[i].scdCreateParams.maxHeight              = 288;
        else
           dspAlgPrm[i].scdCreateParams.maxHeight              = 240;

         dspAlgPrm[i].scdCreateParams.maxStride = 352;
         dspAlgPrm[i].scdCreateParams.numValidChForSCD = 4;
         dspAlgPrm[i].scdCreateParams.numSecs2WaitB4Init    = 3;
         dspAlgPrm[i].scdCreateParams.numSecs2WaitB4FrmAlert= 1;
         dspAlgPrm[i].scdCreateParams.inputFrameRate = 30;
         dspAlgPrm[i].scdCreateParams.outputFrameRate = 5;
         dspAlgPrm[i].scdCreateParams.numSecs2WaitAfterFrmAlert = 2;
            /* Should be applied on CIF channels whose ch numbers are 4~7 */
            //dspAlgPrm.scdCreateParams.startChNoForSCD = 4;
           // Configure array to monitor scene changes in all frame blocks, i.e., motion detection.
           // Each block is fixed to be 32x10 in size,
         numHorzBlks        = dspAlgPrm[i].scdCreateParams.maxWidth / 32;
        if(dspAlgPrm[i].scdCreateParams.maxHeight == 240)
           numVertBlks    = dspAlgPrm[i].scdCreateParams.maxHeight / 10;
        else   /* For 288 Block height becomes 12 */
           numVertBlks    = dspAlgPrm[i].scdCreateParams.maxHeight / 12;

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
         for(chIdx = 0; chIdx < dspAlgPrm[i].scdCreateParams.numValidChForSCD; chIdx++)
         {
             AlgLink_ScdChParams * chPrm = &dspAlgPrm[i].scdCreateParams.chDefaultParams[chIdx];

             chPrm->blkNumBlksInFrame   = numBlksInFrame;
             chPrm->chId               = SCDChannelMonitor[chIdx];
             chPrm->mode               = ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
             chPrm->frmIgnoreLightsON   = FALSE;
             chPrm->frmIgnoreLightsOFF  = FALSE;
             chPrm->frmSensitivity     = ALG_LINK_SCD_SENSITIVITY_HIGH;//ALG_LINK_SCD_SENSITIVITY_MID;
                  chPrm->frmEdgeThreshold   = 100;
       //     chPrm->blkConfig          = NULL; //blkConfig;
             i = 0;
             for(y = 0; y < numVertBlks; y++)
             {
                 for(x = 0; x < numHorzBlks; x++)
                 {
                     chPrm->blkConfig[i].sensitivity = ALG_LINK_SCD_SENSITIVITY_MID;
                     chPrm->blkConfig[i].monitored   = 0;
                     i++;
                 }
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
        dspAlgPrm[0].enableSCDAlg = FALSE;
        dspAlgPrm[1].enableSCDAlg = FALSE;

        dspAlgPrm[0].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;
        dspAlgPrm[1].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;
    }

#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif

    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));
    for(i=0;i<2;i++)
       System_linkCreate(gMultiChHd_VcapVencVdisObj.dupId[i], &dupPrm[i], sizeof(dupPrm[i]));

    for(i=0;i<2;i++)
       System_linkCreate(gMultiChHd_VcapVencVdisObj.mergeId[i], &mergePrm[i],sizeof(mergePrm[i]));

    for(i=0;i<2;i++)
        System_linkCreate(gVcapModuleContext.deiId[i], &deiPrm[i], sizeof(deiPrm[i]));

    for(i=0;i<2;i++)
    {
        System_linkCreate(gVcapModuleContext.sclrId[i], &sclrPrm[i], sizeof(sclrPrm[i]));

    }
    for(i=2;i<3;i++)
       System_linkCreate(gMultiChHd_VcapVencVdisObj.mergeId[i], &mergePrm[i],sizeof(mergePrm[i]));

    System_linkCreate(gVcapModuleContext.nsfId[0] , &nsfPrm[0], sizeof(nsfPrm[0]));
    for(i=3;i<5;i++)
       System_linkCreate(gMultiChHd_VcapVencVdisObj.mergeId[i], &mergePrm[i],sizeof(mergePrm[i]));

    if(enableOsdAlgLink || enableScdAlgLink)
    {
        for(i=0;i<noOfDSPAlgo;i++)
        {

          System_linkCreate(gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[i], &ipcFramesOutVpssPrm[i], sizeof(ipcFramesOutVpssPrm[i]));
          System_linkCreate(gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[i], &ipcFramesInDspPrm[i], sizeof(ipcFramesInDspPrm[i]));
          System_linkCreate(gVcapModuleContext.dspAlgId[i] , &dspAlgPrm[i], sizeof(dspAlgPrm[i]));
       }
        if(enableScdAlgLink)
        {
          System_linkCreate(ipcBitsOutDSPId, &ipcBitsOutDspPrm, sizeof(ipcBitsOutDspPrm));
          System_linkCreate(gVcapModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm[1], sizeof(ipcBitsInHostPrm[1]));
        }
    }

    for(i=2;i<3;i++)
       System_linkCreate(gMultiChHd_VcapVencVdisObj.dupId[i], &dupPrm[i], sizeof(dupPrm[i]));

    System_linkCreate(gMultiChHd_VcapVencVdisObj.ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(gMultiChHd_VcapVencVdisObj.ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );

    System_linkCreate(gVencModuleContext.encId, &encPrm, sizeof(encPrm));
    System_linkCreate(gVencModuleContext.ipcBitsOutRTOSId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(gVencModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm[0], sizeof(ipcBitsInHostPrm[0]));
    System_linkCreate(gVcapModuleContext.ipcFramesOutVpssToHostId     , &ipcFramesOutVpssToHostPrm    , sizeof(ipcFramesOutVpssToHostPrm));
    System_linkCreate(gVcapModuleContext.ipcFramesInHostId     , &ipcFramesInHostPrm    , sizeof(ipcFramesInHostPrm));

    System_linkGetInfo(gVcapModuleContext.ipcFramesInHostId,&framesProducerLinkInfo);
    OSA_assert(framesProducerLinkInfo.numQue == 1);
    ipcFramesOutHostPrm.inQueInfo = framesProducerLinkInfo.queInfo[0];
    System_linkCreate(gVdisModuleContext.ipcFramesOutHostId     , &ipcFramesOutHostPrm    , sizeof(ipcFramesOutHostPrm));
    System_linkCreate(gVdisModuleContext.ipcFramesInVpssFromHostId     , &ipcFramesInVpssFromHostPrm    , sizeof(ipcFramesInVpssFromHostPrm));

    for(i=0;i<2;i++)
    System_linkCreate(gVdisModuleContext.displayId[i], &displayPrm[i], sizeof(displayPrm[i]));

    MultiCh_memPrintHeapStatus();

}


Void MultiChHd_deleteVcapVencVdis()
{
    UInt32 i;
    Bool enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;
#ifdef TWOOSD_INSTANCE
    Bool   enableOsdAlgLinkA8Path = TRUE;
    Bool   enableScdAlgLinkA8Path = TRUE;
#else
    Bool   enableOsdAlgLinkA8Path = FALSE;
    Bool   enableScdAlgLinkA8Path = FALSE;
#endif
    UInt32 noOfDSPAlgo = 2;
    UInt32 ipcBitsOutDSPId;

    if(enableOsdAlgLink == FALSE)
        enableOsdAlgLinkA8Path = FALSE;

    if(enableScdAlgLink == FALSE)
        enableScdAlgLinkA8Path = FALSE;

    if((enableOsdAlgLinkA8Path == FALSE) && (enableScdAlgLinkA8Path == FALSE))
        noOfDSPAlgo = 1;

    /* delete can be done in any order */

    /* delete can be done in any order */
    ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;

    Vcap_delete();
    Venc_delete();
    Vdis_delete();

    for(i=0;i<5;i++)
        System_linkDelete(gMultiChHd_VcapVencVdisObj.mergeId[i]);

    for(i=0;i<3;i++)
    System_linkDelete(gMultiChHd_VcapVencVdisObj.dupId[i]);


    if(enableOsdAlgLink || enableScdAlgLink)
    {
        for(i=0;i<noOfDSPAlgo;i++)
        {
           System_linkDelete(gMultiChHd_VcapVencVdisObj.ipcFrameOutVpssId[i] );
           System_linkDelete(gMultiChHd_VcapVencVdisObj.ipcFramesInDspId[i]);
        }

        if(enableScdAlgLink)
        {
            System_linkDelete(ipcBitsOutDSPId);
            System_linkDelete(gVcapModuleContext.ipcBitsInHLOSId);
        }
    }
    System_linkDelete(gMultiChHd_VcapVencVdisObj.ipcOutVpssId );
    System_linkDelete(gMultiChHd_VcapVencVdisObj.ipcInVideoId );
    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

    /* Reenable tiler allocator taht was disabled by this usecase
     * at delete time.
     */
    SystemTiler_enableAllocator();

}

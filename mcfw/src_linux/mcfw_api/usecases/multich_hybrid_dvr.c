/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


#include "mcfw/src_linux/mcfw_api/usecases/multich_common.h"
#include "mcfw/src_linux/mcfw_api/usecases/multich_ipcbits.h"
#include "mcfw/src_linux/mcfw_api/ti_vdis_priv.h"
#include "mcfw/interfaces/link_api/system_tiler.h"
#include "mcfw/interfaces/link_api/avsync_hlos.h"

/* =============================================================================
 * Externs
 * =============================================================================
 */

static UInt8 SCDChannelMonitor[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
/* =============================================================================
 * Use case code
 * =============================================================================
 */

/**
 * ------------------------------ DUAL OUT <D1 + CIF> -------------------------------------

                               +-----------------------------------+
                               | Capture (YUV422I) 16CH D1 60fps   |
                               +-----------------------------------+
                                               |
                                               |
                                               |
                                     +------------------+                        +----------------+  +---------------+
                                     |IPC_FRM_OUT_VPSS_0|<--<<<PROCESS_LINK>>>-->|IPC_FRM_IN_DSP_0|->|ALG_LINK_0(OSD)|
                                     +------------------+                        +----------------+  +---------------+
                                          +       +
                                          |       |
 +------------------------------------+   |Q0     |Q1  +------------------------------------+
 |                                    |   |       |    |                                    |
 |      DEI  (VIP-SC YUV420 )         |   |       |    |      DEIH (VIP-SC YUV420 )         |
 |           (DEI-SC YUV422I )        |<--+       +--->|           (DEI-SC YUV422I )        |
 |                                    |                |                                    |
 +------------------------------------+                +------------------------------------+
    |             |           |                           |             |           |
    |             |           |                           |             |           |
    |             |           |                           |             |           |
  VIP_SC       VIP_SC       DEI_SC                      VIP_SC       VIP_SC       DEI_SC
  YUV420       YUV420       YUV422                      YUV420       YUV420       YUV422
 PRIMARY      SECONDARY       +                        PRIMARY      SECONDARY       +
    +             +           |                           +             +           |
    |             |           |                           |             |           |
    |             |           |                           +-------------)-----------)--+
    |             |       Q0  |   +---------------------------+   Q1    |           |  |
    |             +-----------}-->| DEI_SC_CIF_MERGE_LINK_IDX |<--------+           |  |
    |                         |   +---------------------------+                     |  |
    |                         |                +                                    |  |
    |                    +----+                |                                    |  |
    |                    |                     |                                    |  |
    |                    |                     v                                    |  |
    |                    |    +-----------------------------------------+           |  |
    |                    |    |            CIF_DUP_LINK_IDX             |           |  |
    |                    |    +-----------------------------------------+           |  |
    |                    |      +                 +                   +             |  |
    |                    |      +                 |                   |             |  |
    |                    |      |                 |                   |             |  |
    |                    |      |                 |            +------}-------------+  |
    |                    |      |                 |            |      |                |
    +--------------------}------|-----------------)------------)------)----------+Q0   |Q1
                         |      |                 |            |      |          v     v
                         |      |                 |            |      |    +----------------------+
                         |      |                 |            |      +--->| D1_CIF_MERGE_LINK_IDX|
                         +------|----+            |            |       Q2  +----------------------+
                                |    |            |            |                       |
                                |    |            |            |                +-------------+     +------------+   +----+      +--------------------+   +--------------------+
                                |    |            |            |                |IPCM3OUT(VPS)|+--> |IPCM3IN(VID)|+--|ENC |+---> |IPCBITS_RTOSOUT(VID)|+->|IPCBITS_HLOSIN(HOST)|  FILEOUT
                                |    |Q0          |Q3          |Q1              +-------------+     +------------+   +----+      +--------------------+   +--------------------+
                                |    v            v            v
                                |   +---------------------------+               +------------+     +-------------+   +----+      +-------------------+    +---------------------+
                                |   |LIVE_DECODE_MERGE_LINK_IDX | <------------+|IPCM3IN(VPS)|<---+|IPCM3OUT(VID)|<--|DEC |<----+|IPCBITS_RTOSIN(VID)|<--+|IPCBITS_HLOSOUT(HOST)|
                                |   +---------------------------+ Q2            +------------+     +-------------+   +----+      +-------------------+    +---------------------+
                                |                 +
                                |                 |
                                |                 v
                                |   +---------------------------+
                                |   |  LIVE_DECODE_DUP_LINK_IDX |
                                |   +---------------------------+
                                |       +                 +
                                |       |                 |
                                |       v                 v
                                | +-------------+  +-------------+             +------------------+  +----------------+  +---------------+  +--------------------+   +--------------------+
                                | | SW Mosaic   |  | SW Mosaic   |       +---->|IPC_FRM_OUT_VPSS_1|->|IPC_FRM_IN_DSP_1|->|ALG_LINK_1(SCD)|->|IPCBITS_RTOSOUT(DSP)|+->|IPCBITS_HLOSIN1HOST)|  FILEOUT
                                | |(SC5 YUV422I)|  |(SC5 YUV422I)|       |     +------------------+  +----------------+  +---------------+  +--------------------+   +--------------------+
                                | +-------------+  +-------------+       |
                                |     GRPX0             GRPX1            |
                                |   On-Chip HDMI        SDTV             |
                                |      1080p60         480i60            |
                                |                                        |
                                |                                        |
                                |                                        |
                                +----------------------------------------+
*/

#define     NUM_CAPTURE_DEVICES          4
#define     NUM_SD_DECODE_CHANNELS      (16)
#define     NUM_HD_DECODE_CHANNELS      (4)
#define     SD_DECODE_CHANNEL_WIDTH     (720)
#define     SD_DECODE_CHANNEL_HEIGHT     (576)
#define     HD_DECODE_CHANNEL_WIDTH     (1920)
#define     HD_DECODE_CHANNEL_HEIGHT     (1080)

#define     MULTICH_HYBRID_DVR_USECASE_MAX_NUM_LINKS       (64)

#define     MULTICH_DSP_ALG_LINK_OSD_IDX  (0)
#define     MULTICH_DSP_ALG_LINK_SCD_IDX  (1)

#define     OSD_VID_PLANE_FMT_420         (0)
#define     OSD_VID_PLANE_FMT_422         (1)

#define     NUM_BUFS_PER_CH_CAPTURE              (6)
#define     NUM_BUFS_PER_CH_DEI_DEIQ             (4)
#define     NUM_BUFS_PER_CH_DEI_VIP_SC_PRIQ      (4)
#define     NUM_BUFS_PER_CH_DEI_VIP_SC_SECQ      (3)
#define     NUM_BUFS_PER_CH_DEC_SD               (4)
#define     NUM_BUFS_PER_CH_DEC_HD               (3)
#define     NUM_BUFS_PER_CH_SWMS_HD              (4)
#define     NUM_BUFS_PER_CH_SWMS_SD              (4)
#define     NUM_BUFS_PER_CH_ENC_PRI              (4)
#define     NUM_BUFS_PER_CH_ENC_SEC              (3)
#define     NUM_BUFS_PER_CH_BITSOUT_SD           (4)
#define     NUM_BUFS_PER_CH_BITSOUT_HD           (3)



#define     ENC_LINK_SECONDARY_STREAM_POOL_ID    (0)
#define     ENC_LINK_PRIMARY_STREAM_POOL_ID      (1)

#define     IPCBITSOUT_LINK_SD_STREAM_POOL_ID    (0)
#define     IPCBITSOUT_LINK_HD_STREAM_POOL_ID    (1)

#define     TILER_ENABLE_ENCODE                  (TRUE)
#define     TILER_ENABLE_DECODE                  (FALSE)

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
        .EncNumCh  = 18,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 24, 25, 26, 27,28,29, 30, 31},
        .DecNumCh  = 1,
        .DecChList = {15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 16,
        .EncChList = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47},
        .DecNumCh  = 17,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 , 11, 12,16,17,18,19},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 14,
        .EncChList = {10,11,12,13, 14, 15,16, 17, 18, 19, 20, 21, 22, 23},
        .DecNumCh  = 2,
        .DecChList = {13, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

/** Merge Link Info */
#define     NUM_MERGE_LINK                         (3)

/**  DEI SC Merge
 *   DEI0 ---DEI_VIP_SC_SEC_Q-->Q0--|
 *                                  |-DEI_SC_CIF_MERGE_LINK_IDX
 *   DEI1 ---DEI_VIP_SC_SEC_Q-->Q1--|
 */
#define     DEI_SC_CIF_MERGE_LINK_IDX               0
#define     DEI_SC_CIF_MERGE_LINK_NUM_INQUE        (2)
#define     DEI_SC_CIF_MERGE_LINK_DEI0_QIDX        (0)
#define     DEI_SC_CIF_MERGE_LINK_DEI1_QIDX        (1)

/**  D1 CIF Merge
 *   DEI0 ---DEI_VIP_SC_PRI_Q-->Q0--|
 *                                  |
 *   DEI1 ---DEI_VIP_SC_PRI_Q-->Q1--|-DEI_SC_CIF_MERGE_LINK_IDX
 *                                  |
 *   MERGED_CIF_DUP ----->Q0--->Q2--|
 */
#define     D1_CIF_MERGE_LINK_IDX                  1
#define     D1_CIF_MERGE_LINK_NUM_INQUE           (3)
#define     D1_CIF_MERGE_LINK_DEI0_QIDX           (0)
#define     D1_CIF_MERGE_LINK_DEI1_QIDX           (1)
#define     D1_CIF_MERGE_LINK_CIFDUP_QIDX         (2)

/**  Live Decode Merge
 *   DEI0 ---DEI_DEI_SC_Q-->Q0--|
 *                              |
 *   DEI1 ---DEI_DEI_SC_Q-->Q1--|
 *                              |-LIVE_DECODE_MERGE_LINK_IDX
 *   VDEC ----->Q0--------->Q2--|
 *                              |
 *   CIF_DUP ----->Q1------>Q3--|
 */
#define     LIVE_DECODE_MERGE_LINK_IDX             2
#define     LIVE_DECODE_MERGE_LINK_NUM_INQUE      (4)
#define     LIVE_DECODE_MERGE_LINK_VDEC_QIDX      (0)
#define     LIVE_DECODE_MERGE_LINK_DEI0_QIDX      (1)
#define     LIVE_DECODE_MERGE_LINK_DEI1_QIDX      (2)
#define     LIVE_DECODE_MERGE_LINK_CIFDUP_QIDX    (3)

#define     NUM_DUP_LINK                           2
#define     LIVE_DECODE_DUP_LINK_IDX               0
#define     CIF_DUP_LINK_IDX                       1

typedef struct MultichHybridDVR_Context
{
    UInt32 ipcBitsOutDSPId;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 grpxId[VDIS_DEV_MAX];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    UInt32 createdLinkCount;
    UInt32 createdLinks[MULTICH_HYBRID_DVR_USECASE_MAX_NUM_LINKS];
    CaptureLink_CreateParams    capturePrm;
    DeiLink_CreateParams        deiPrm[MAX_DEI_LINK];
    MergeLink_CreateParams      mergePrm[NUM_MERGE_LINK];
    DupLink_CreateParams        dupPrm[NUM_DUP_LINK];
    SwMsLink_CreateParams       swMsPrm[VDIS_DEV_MAX];
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
    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm[2];
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm[2];
    AlgLink_CreateParams                dspAlgPrm[2];
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm;
    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];
    AvsyncLink_LinkSynchConfigParams       avsyncCfg[2];
    UInt32  captureFps;
}  MultichHybridDVR_Context;

MultichHybridDVR_Context gHybridDVRUsecaseContext =
{
    .createdLinkCount           = 0
};


static Void multich_hybriddvr_register_created_link(MultichHybridDVR_Context *pContext,
                                                    UInt32 linkID)
{
    OSA_assert(pContext->createdLinkCount < OSA_ARRAYSIZE(pContext->createdLinks));
    pContext->createdLinks[pContext->createdLinkCount] = linkID;
    pContext->createdLinkCount++;
}

#define MULTICH_HYBRIDDVR_CREATE_LINK(linkID,createPrm,createPrmSize)           \
    do                                                                          \
    {                                                                           \
        System_linkCreate(linkID,createPrm,createPrmSize);                      \
        multich_hybriddvr_register_created_link(&gHybridDVRUsecaseContext,      \
                                                linkID);                        \
    } while (0)

static
Void multich_hybriddvr_reset_link_prms()
{
    int i;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridDVRUsecaseContext.ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridDVRUsecaseContext.ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridDVRUsecaseContext.ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridDVRUsecaseContext.ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,gHybridDVRUsecaseContext.ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,gHybridDVRUsecaseContext.ipcBitsInHostPrm[0]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,gHybridDVRUsecaseContext.ipcBitsInHostPrm[1]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,gHybridDVRUsecaseContext.ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,gHybridDVRUsecaseContext.ipcFramesInDspPrm[0]);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,gHybridDVRUsecaseContext.ipcFramesInDspPrm[1]);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0]);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1]);
    IpcBitsOutLinkHLOS_CreateParams_Init(&gHybridDVRUsecaseContext.ipcBitsOutHostPrm);
    DecLink_CreateParams_Init(&gHybridDVRUsecaseContext.decPrm);
    EncLink_CreateParams_Init(&gHybridDVRUsecaseContext.encPrm);
    CaptureLink_CreateParams_Init(&gHybridDVRUsecaseContext.capturePrm);
    for (i = 0; i < OSA_ARRAYSIZE(gHybridDVRUsecaseContext.displayPrm);i++)
    {
        DisplayLink_CreateParams_Init(&gHybridDVRUsecaseContext.displayPrm[i]);
    }
    for (i = 0; i < OSA_ARRAYSIZE(&gHybridDVRUsecaseContext.dspAlgPrm);i++)
    {
        AlgLink_CreateParams_Init(&gHybridDVRUsecaseContext.dspAlgPrm[i]);
    }
    for (i = 0; i < OSA_ARRAYSIZE(&gHybridDVRUsecaseContext.swMsPrm);i++)
    {
        SwMsLink_CreateParams_Init(&gHybridDVRUsecaseContext.swMsPrm[i]);
    }

     for (i = 0; i < OSA_ARRAYSIZE(&gHybridDVRUsecaseContext.deiPrm);i++)
     {
         DeiLink_CreateParams_Init(&gHybridDVRUsecaseContext.deiPrm[i]);
     }
     for (i = 0; i < OSA_ARRAYSIZE(&gHybridDVRUsecaseContext.avsyncCfg);i++)
     {
         AvsyncLink_LinkSynchConfigParams_Init(&gHybridDVRUsecaseContext.avsyncCfg[i]);
     }
}

static
Void multich_hybriddvr_set_capture_prm(CaptureLink_CreateParams *capturePrm,
                                       UInt32 numSubChains,
                                       Bool singleOutQ)
{
    CaptureLink_VipInstParams         *pCaptureInstPrm;
    CaptureLink_OutParams             *pCaptureOutPrm;
    UInt32 vipInstId;

    /* This is for TVP5158 Audio Channels - Change it to 16 if there are 16 audio channels connected in cascade */
    capturePrm->numVipInst                 = 2*numSubChains;

    capturePrm->tilerEnable                = FALSE;
    capturePrm->numBufsPerCh               = NUM_BUFS_PER_CH_CAPTURE;
    capturePrm->numExtraBufs               = 0;
#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm->isPalMode = Vcap_isPalMode();
#endif

    for(vipInstId=0; vipInstId<capturePrm->numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm->vipInst[vipInstId];
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
        if ((vipInstId >= numSubChains) && !singleOutQ)
          pCaptureOutPrm->outQueId          = 1;
        else
          pCaptureOutPrm->outQueId          = 0;

    }
}

static
Void multich_hybriddvr_configure_extvideodecoder_prm(UInt32 numCaptureDevices)
{
    int i;

    for(i = 0; i < numCaptureDevices; i++)
    {
        gHybridDVRUsecaseContext.vidDecVideoModeArgs[i].videoIfMode        = DEVICE_CAPT_VIDEO_IF_MODE_8BIT;
        gHybridDVRUsecaseContext.vidDecVideoModeArgs[i].videoDataFormat    = SYSTEM_DF_YUV422P;
        gHybridDVRUsecaseContext.vidDecVideoModeArgs[i].standard           = SYSTEM_STD_MUX_4CH_D1;
        gHybridDVRUsecaseContext.vidDecVideoModeArgs[i].videoCaptureMode   =
                    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_PIXEL_MUX_EMBEDDED_SYNC;
        gHybridDVRUsecaseContext.vidDecVideoModeArgs[i].videoSystem        =
                                      DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT;
        gHybridDVRUsecaseContext.vidDecVideoModeArgs[i].videoCropEnable    = FALSE;
        gHybridDVRUsecaseContext.vidDecVideoModeArgs[i].videoAutoDetectTimeout = -1;
    }
    Vcap_configVideoDecoder(&gHybridDVRUsecaseContext.vidDecVideoModeArgs[0],
                            NUM_CAPTURE_DEVICES);
}


static
Void multich_hybriddvr_set_dei_prm(DeiLink_CreateParams *deiPrm,
                                   UInt32 numSubChains)
{
    int deiChIdx;

    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC] = NUM_BUFS_PER_CH_DEI_DEIQ;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC] = NUM_BUFS_PER_CH_DEI_VIP_SC_PRIQ;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = NUM_BUFS_PER_CH_DEI_VIP_SC_SECQ;
    /* Set Output Scaling at DEI based on ratio */
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 1;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][deiChIdx] =
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];
    }

    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.denominator = 2;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.denominator = 2;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][deiChIdx] =
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0];
    }

    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator = 1;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][deiChIdx] =
            deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];
    }

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]          = TRUE;
    /* If osd is not enabled then by default enable the tiler mode */
    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = TILER_ENABLE_ENCODE;
    deiPrm->comprEnable                                   = FALSE;
    deiPrm->setVipScYuv422Format                          = FALSE;
}

static
Void multich_hybriddvr_set_enclink_prm(EncLink_CreateParams *encPrm)
{
    int i,j;
    EncLink_ChCreateParams *pLinkChPrm;
    EncLink_ChDynamicParams *pLinkDynPrm;
    VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
    VENC_CHN_PARAMS_S *pChPrm;

    encPrm->numBufPerCh[ENC_LINK_SECONDARY_STREAM_POOL_ID] = NUM_BUFS_PER_CH_ENC_SEC;
    encPrm->numBufPerCh[ENC_LINK_SECONDARY_STREAM_POOL_ID] = NUM_BUFS_PER_CH_ENC_PRI;
    /* Primary Stream Params - D1 */
    for (i=0; i < gVencModuleContext.vencConfig.numPrimaryChn; i++)
    {
        pLinkChPrm  = &encPrm->chCreateParams[i];
        pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

        pChPrm      = &gVencModuleContext.vencConfig.encChannelParams[i];
        pDynPrm     = &pChPrm->dynamicParam;

        pLinkChPrm->format                  = IVIDEO_H264HP;
        pLinkChPrm->profile                 = gVencModuleContext.vencConfig.h264Profile[i];
        pLinkChPrm->dataLayout              = IVIDEO_FIELD_SEPARATED;
        pLinkChPrm->fieldMergeEncodeEnable  = FALSE;
        pLinkChPrm->enableAnalyticinfo      = pChPrm->enableAnalyticinfo;
        pLinkChPrm->maxBitRate              = pChPrm->maxBitRate;
        pLinkChPrm->encodingPreset          = pChPrm->encodingPreset;
        pLinkChPrm->rateControlPreset       = pChPrm->rcType;
        pLinkChPrm->enableHighSpeed         = FALSE;
        pLinkChPrm->numTemporalLayer        = pChPrm->numTemporalLayer;
        pLinkChPrm->enableSVCExtensionFlag  = pChPrm->enableSVCExtensionFlag;

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
    for (i =  gVencModuleContext.vencConfig.numPrimaryChn,
         j =  VENC_PRIMARY_CHANNELS;
         i < (gVencModuleContext.vencConfig.numPrimaryChn
              + gVencModuleContext.vencConfig.numSecondaryChn);
         i++, j++)
    {
        pLinkChPrm  = &encPrm->chCreateParams[i];
        pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

        pChPrm      = &gVencModuleContext.vencConfig.encChannelParams[j];
        pDynPrm     = &pChPrm->dynamicParam;

        pLinkChPrm->format                  = IVIDEO_H264HP;
        pLinkChPrm->profile                 = gVencModuleContext.vencConfig.h264Profile[i];
        pLinkChPrm->dataLayout              = IVIDEO_FIELD_SEPARATED;
        pLinkChPrm->fieldMergeEncodeEnable  = FALSE;
        pLinkChPrm->enableAnalyticinfo      = pChPrm->enableAnalyticinfo;
        pLinkChPrm->maxBitRate              = pChPrm->maxBitRate;
        pLinkChPrm->encodingPreset          = pChPrm->encodingPreset;
        pLinkChPrm->rateControlPreset       = pChPrm->rcType;
        pLinkChPrm->enableHighSpeed         = TRUE;
        pLinkChPrm->numTemporalLayer        = pChPrm->numTemporalLayer;
        pLinkChPrm->enableSVCExtensionFlag  = pChPrm->enableSVCExtensionFlag;


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

static
Void multich_hybriddvr_set_swms_prm(SwMsLink_CreateParams *swMsPrm,
                                    UInt32 swMsIdx)
{
    UInt32 devId;

    swMsPrm->numSwMsInst = 1;
    swMsPrm->swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm->maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN + 6;
    swMsPrm->numOutBuf = NUM_BUFS_PER_CH_SWMS_HD;
    if (swMsIdx == 1)
    {
        swMsPrm->maxOutRes  = VSYS_STD_PAL;
	    swMsPrm->initOutRes = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
        swMsPrm->numOutBuf  = NUM_BUFS_PER_CH_SWMS_SD;
        devId               = VDIS_DEV_SD;
    }
    else
    {
        swMsPrm->maxOutRes  = VSYS_STD_1080P_60;
	    swMsPrm->initOutRes = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_HDMI].resolution;
        swMsPrm->numOutBuf  = NUM_BUFS_PER_CH_SWMS_HD;
        devId               = VDIS_DEV_HDMI;
    }
    swMsPrm->lineSkipMode = FALSE;
    swMsPrm->enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

    MultiCh_swMsGetDefaultLayoutPrm(devId, swMsPrm, FALSE);
}

static
Void mulich_hybriddvr_set_avsync_vidque_prm(Avsync_SynchConfigParams *queCfg,
                                            Int chnum,
                                            UInt32 avsStartChNum,
                                            UInt32 avsEndChNum)
{
    queCfg->chNum = chnum;
    queCfg->audioPresent = FALSE;
    if ((queCfg->chNum >= avsStartChNum)
        &&
        (queCfg->chNum <= avsEndChNum)
        &&
        (gVsysModuleContext.vsysConfig.enableAVsync))
    {
        queCfg->avsyncEnable = FALSE;
    }
    else
    {
        queCfg->avsyncEnable = FALSE;
    }
    queCfg->clkAdjustPolicy.refClkType = AVSYNC_REFCLKADJUST_NONE;
    queCfg->playTimerStartTimeout = 0;
    queCfg->playStartMode = AVSYNC_PLAYBACK_START_MODE_WAITSYNCH;
    queCfg->ptsInitMode   = AVSYNC_PTS_INIT_MODE_APP;
    queCfg->clkAdjustPolicy.clkAdjustLead = AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LEAD_MS;
    queCfg->clkAdjustPolicy.clkAdjustLag = AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LAG_MS;
    queCfg->vidSynchPolicy.playMaxLag    = 200;
}

static
Void mulich_hybriddvr_set_avsync_prm(AvsyncLink_LinkSynchConfigParams *avsyncPrm,
                                     UInt32 swMsIdx,
                                     UInt32 prevLinkID,
                                     UInt32 prevLinkQueId)
{
    System_LinkInfo                   swmsInLinkInfo;
    Int i;
    Int32 status;

    if (0 == swMsIdx)
    {
        Vdis_getAvsyncConfig(VDIS_DEV_HDMI,avsyncPrm);
        avsyncPrm->displayLinkID        = Vdis_getDisplayId(VDIS_DEV_HDMI);
    }
    else
    {
        Vdis_getAvsyncConfig(VDIS_DEV_SD,avsyncPrm);
        avsyncPrm->displayLinkID        = Vdis_getDisplayId(VDIS_DEV_SD);
    }
    avsyncPrm->videoSynchLinkID = gVdisModuleContext.swMsId[swMsIdx];
    System_linkGetInfo(prevLinkID,&swmsInLinkInfo);
    OSA_assert(swmsInLinkInfo.numQue > prevLinkQueId);

    avsyncPrm->numCh            = swmsInLinkInfo.queInfo[prevLinkQueId].numCh;
    avsyncPrm->syncMasterChnum = AVSYNC_INVALID_CHNUM;
    for (i = 0; i < avsyncPrm->numCh;i++)
    {
        mulich_hybriddvr_set_avsync_vidque_prm(&avsyncPrm->queCfg[i],
                                               i,
                                               0,
                                               (0 + (gVdecModuleContext.vdecConfig.numChn - 1)));
    }
    if (0 == swMsIdx)
    {
        Vdis_setAvsyncConfig(VDIS_DEV_HDMI,avsyncPrm);
    }
    else
    {
        Vdis_setAvsyncConfig(VDIS_DEV_SD,avsyncPrm);
    }

    status = Avsync_configSyncConfigInfo(avsyncPrm);
    OSA_assert(status == 0);

}

static
Void multich_hybriddvr_set_osd_prm(AlgLink_CreateParams *dspAlgPrm)
{
    int chId;

    dspAlgPrm->enableOSDAlg = TRUE;
    dspAlgPrm->enableSCDAlg = FALSE;
    dspAlgPrm->outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink = SYSTEM_LINK_ID_INVALID;


    for(chId = 0; chId < ALG_LINK_OSD_MAX_CH; chId++)
    {
        AlgLink_OsdChWinParams * chWinPrm =
          &dspAlgPrm->osdChCreateParams[chId].chDefaultParams;
        /* set osd window max width and height */
        dspAlgPrm->osdChCreateParams[chId].maxWidth  = EXAMPLE_OSD_WIN_MAX_WIDTH;
        dspAlgPrm->osdChCreateParams[chId].maxHeight = EXAMPLE_OSD_WIN_MAX_HEIGHT;

        chWinPrm->chId = chId;
        chWinPrm->numWindows = 0;
    }
}

static
Void multich_hybriddvr_set_scd_prm(AlgLink_CreateParams *dspAlgPrm)
{
    Int32   numBlksInFrame;
    Int32   numHorzBlks, numVertBlks, chIdx;
    UInt32  x, y, i;

    //AlgLink_ScdblkChngConfig  blkConfig[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];
    dspAlgPrm->enableOSDAlg = FALSE;
    dspAlgPrm->enableSCDAlg = TRUE;
    dspAlgPrm->scdCreateParams.maxWidth               = 352;
    if(Vcap_isPalMode())
       dspAlgPrm->scdCreateParams.maxHeight              = 288;
    else
       dspAlgPrm->scdCreateParams.maxHeight              = 240;
    dspAlgPrm->scdCreateParams.maxStride              = 352;
    dspAlgPrm->scdCreateParams.numValidChForSCD       = 16;

    dspAlgPrm->scdCreateParams.numSecs2WaitB4Init     = 3;
    dspAlgPrm->scdCreateParams.numSecs2WaitB4FrmAlert = 1;
    dspAlgPrm->scdCreateParams.inputFrameRate         = 2;
    dspAlgPrm->scdCreateParams.outputFrameRate        = 2;

    /* Should be applied on CIF channels whose ch numbers are 4~7 */
    //dspAlgPrm[0].scdCreateParams.startChNoForSCD = 4;
   // Configure array to monitor scene changes in all frame blocks, i.e., motion detection.
   // Each block is fixed to be 32x10 in size when height is 240,
   // Each block is fixed to be 32x11 in size when height is 288
    numHorzBlks    = dspAlgPrm->scdCreateParams.maxWidth / 32;
    if(dspAlgPrm->scdCreateParams.maxHeight == 240)
       numVertBlks    = dspAlgPrm->scdCreateParams.maxHeight / 10;
    else   /* For 288 Block height becomes 12 */
       numVertBlks    = dspAlgPrm->scdCreateParams.maxHeight / 12;

    numBlksInFrame = numHorzBlks * numVertBlks;

    for(chIdx = 0; chIdx < dspAlgPrm->scdCreateParams.numValidChForSCD; chIdx++)
    {
       AlgLink_ScdChParams * chPrm = &dspAlgPrm->scdCreateParams.chDefaultParams[chIdx];

       chPrm->blkNumBlksInFrame = numBlksInFrame;
       chPrm->chId               = SCDChannelMonitor[chIdx];
       chPrm->mode               = ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
       chPrm->frmIgnoreLightsON = FALSE;
       chPrm->frmIgnoreLightsOFF    = FALSE;
       chPrm->frmSensitivity     = ALG_LINK_SCD_SENSITIVITY_HIGH;//ALG_LINK_SCD_SENSITIVITY_MID;
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
}

static
Void multich_hybriddvr_set_declink_prms(DecLink_CreateParams *decPrm)
{
    int i;


    gVdecModuleContext.vdecConfig.numChn = (NUM_SD_DECODE_CHANNELS + NUM_HD_DECODE_CHANNELS);
    for (i=0; i<gVdecModuleContext.vdecConfig.numChn; i++)
    {
        decPrm->chCreateParams[i].format                 = IVIDEO_H264HP;
        decPrm->chCreateParams[i].profile                = IH264VDEC_PROFILE_ANY;
        decPrm->chCreateParams[i].fieldMergeDecodeEnable = FALSE;
        if (i < NUM_SD_DECODE_CHANNELS)
        {
            decPrm->chCreateParams[i].targetMaxWidth         = SD_DECODE_CHANNEL_WIDTH;
            decPrm->chCreateParams[i].targetMaxHeight        = SD_DECODE_CHANNEL_HEIGHT;
            decPrm->chCreateParams[i].numBufPerCh            = NUM_BUFS_PER_CH_DEC_SD;
        }
        else
        {
            decPrm->chCreateParams[i].targetMaxWidth         = HD_DECODE_CHANNEL_WIDTH;
            decPrm->chCreateParams[i].targetMaxHeight        = HD_DECODE_CHANNEL_HEIGHT;
            decPrm->chCreateParams[i].numBufPerCh            = NUM_BUFS_PER_CH_DEC_HD;
        }
        decPrm->chCreateParams[i].defaultDynamicParams.targetFrameRate = gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.frameRate;
        decPrm->chCreateParams[i].defaultDynamicParams.targetBitRate   = gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.targetBitRate;
    }
    decPrm->tilerEnable = TILER_ENABLE_DECODE;
}

static
Void multich_hybriddvr_set_ipcbitsout_hlos_prms(IpcBitsOutLinkHLOS_CreateParams * ipcBitsOutHostPrm)
{
    int i;

    for (i = 0;
         i < (NUM_SD_DECODE_CHANNELS + NUM_HD_DECODE_CHANNELS);
         i++)
    {
        System_LinkChInfo *pChInfo;

        pChInfo = &ipcBitsOutHostPrm->inQueInfo.chInfo[i];

        pChInfo->bufType        = 0; // NOT USED
        pChInfo->codingformat   = 0; // NOT USED
        pChInfo->dataFormat     = 0; // NOT USED
        pChInfo->memType        = 0; // NOT USED
        pChInfo->startX         = 0; // NOT USED
        pChInfo->startY         = 0; // NOT USED
        if (i < NUM_SD_DECODE_CHANNELS)
        {
            pChInfo->width          = SD_DECODE_CHANNEL_WIDTH;
            pChInfo->height         = SD_DECODE_CHANNEL_HEIGHT;
        }
        else
        {
            pChInfo->width          = HD_DECODE_CHANNEL_WIDTH;
            pChInfo->height         = HD_DECODE_CHANNEL_HEIGHT;
        }
        pChInfo->pitch[0]       = 0; // NOT USED
        pChInfo->pitch[1]       = 0; // NOT USED
        pChInfo->pitch[2]       = 0; // NOT USED
        pChInfo->scanFormat     = SYSTEM_SF_PROGRESSIVE;
    }
    ipcBitsOutHostPrm->baseCreateParams.noNotifyMode = FALSE;
    ipcBitsOutHostPrm->baseCreateParams.notifyNextLink = TRUE;
    ipcBitsOutHostPrm->baseCreateParams.numOutQue = 1;
    ipcBitsOutHostPrm->numBufPerCh[IPCBITSOUT_LINK_SD_STREAM_POOL_ID] =
        NUM_BUFS_PER_CH_BITSOUT_SD;
    ipcBitsOutHostPrm->numBufPerCh[IPCBITSOUT_LINK_HD_STREAM_POOL_ID] =
        NUM_BUFS_PER_CH_BITSOUT_HD;
    ipcBitsOutHostPrm->inQueInfo.numCh =
        (NUM_SD_DECODE_CHANNELS + NUM_HD_DECODE_CHANNELS);
}

static
Void multich_hybriddvr_set_display_prms(DisplayLink_CreateParams *displayPrm,
                                        UInt32 maxOutRes)
{
    displayPrm->displayRes = maxOutRes;
}

static
Void multich_hybriddvr_set_link_ids()
{
    Bool   enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool   enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;
    int    i;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;

    if(enableOsdAlgLink)
    {
        gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
        gVcapModuleContext.ipcFramesInDspId[0] = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_ALG_0;
    }
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    for (i = 0; i < NUM_MERGE_LINK;i++)
    {
        gHybridDVRUsecaseContext.mergeId[i] = SYSTEM_VPSS_LINK_ID_MERGE_0 + i;
    }
    for (i = 0; i < NUM_DUP_LINK;i++)
    {
        gHybridDVRUsecaseContext.dupId[i] = SYSTEM_VPSS_LINK_ID_DUP_0 + i;
    }

    if(enableScdAlgLink)
    {
        gVcapModuleContext.ipcFramesOutVpssId[1] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
        gVcapModuleContext.ipcFramesInDspId[1]   = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_1;
        gVcapModuleContext.dspAlgId[1]           = SYSTEM_LINK_ID_ALG_1;
        gHybridDVRUsecaseContext.ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
        gVcapModuleContext.ipcBitsInHLOSId       = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    }
    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;

    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)] = SYSTEM_LINK_ID_DISPLAY_0; /* ON CHIP HDMI */
    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_SD)] = SYSTEM_LINK_ID_DISPLAY_2; /* SD HDMI */

    gHybridDVRUsecaseContext.grpxId[0]                       = SYSTEM_LINK_ID_GRPX_0;
    gHybridDVRUsecaseContext.grpxId[1]                       = SYSTEM_LINK_ID_GRPX_1;

    gHybridDVRUsecaseContext.ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    gHybridDVRUsecaseContext.ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    gHybridDVRUsecaseContext.ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    gHybridDVRUsecaseContext.ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    if(enableScdAlgLink)
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
    else
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;
}

static
Void multich_hybriddvr_reset_link_ids()
{
    Bool   enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool   enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;
    int    i;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_INVALID;

    if(enableOsdAlgLink)
    {
        gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_LINK_ID_INVALID;
        gVcapModuleContext.ipcFramesInDspId[0] = SYSTEM_LINK_ID_INVALID;
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_INVALID;
    }
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_INVALID;
    for (i = 0; i < NUM_MERGE_LINK;i++)
    {
        gHybridDVRUsecaseContext.mergeId[i] = SYSTEM_LINK_ID_INVALID;
    }
    for (i = 0; i < NUM_DUP_LINK;i++)
    {
        gHybridDVRUsecaseContext.dupId[i] = SYSTEM_LINK_ID_INVALID;
    }

    if(enableScdAlgLink)
    {
        gVcapModuleContext.ipcFramesOutVpssId[1] = SYSTEM_LINK_ID_INVALID;
        gVcapModuleContext.ipcFramesInDspId[1]   = SYSTEM_LINK_ID_INVALID;
        gVcapModuleContext.dspAlgId[1]           = SYSTEM_LINK_ID_INVALID;
        gHybridDVRUsecaseContext.ipcBitsOutDSPId = SYSTEM_LINK_ID_INVALID;
        gVcapModuleContext.ipcBitsInHLOSId       = SYSTEM_LINK_ID_INVALID;
    }
    gVencModuleContext.encId        = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_INVALID;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_INVALID;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_INVALID;

    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)] = SYSTEM_LINK_ID_INVALID; /* ON CHIP HDMI */
    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_SD)] = SYSTEM_LINK_ID_INVALID; /* SD HDMI */

    gHybridDVRUsecaseContext.grpxId[0]                       = SYSTEM_LINK_ID_INVALID;
    gHybridDVRUsecaseContext.grpxId[1]                       = SYSTEM_LINK_ID_INVALID;

    gHybridDVRUsecaseContext.ipcOutVpssId = SYSTEM_LINK_ID_INVALID;
    gHybridDVRUsecaseContext.ipcInVideoId = SYSTEM_LINK_ID_INVALID;
    gHybridDVRUsecaseContext.ipcOutVideoId= SYSTEM_LINK_ID_INVALID;
    gHybridDVRUsecaseContext.ipcInVpssId  = SYSTEM_LINK_ID_INVALID;

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_LINK_ID_INVALID;
    if(enableScdAlgLink)
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_LINK_ID_INVALID;
    else
       gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_LINK_ID_INVALID;
}

static
Void multich_hybriddvr_set_capture_fps(UInt32 *captureFps)
{
    Bool isPal = Vcap_isPalMode();

    if (isPal)
    {
        *captureFps = 50;
    }
    else
    {
        *captureFps = 60;
    }
}

#define MULTICH_HYBRIDDVR_GET_CAPTURE_FIELDS_PER_SEC()           (gHybridDVRUsecaseContext.captureFps)
#define MULTICH_HYBRIDDVR_GET_CAPTURE_FRAMES_PER_SEC()           (gHybridDVRUsecaseContext.captureFps/2)


static
Void multich_hybriddvr_set_links_framerate()
{
    Int32 status;
    DeiLink_ChFpsParams params;
    UInt32 chId;

    for (chId = 0; chId < gVcapModuleContext.vcapConfig.numChn;chId++)
    {
        /* Capture -> Dei */
        params.chId = chId;
        params.inputFrameRate = MULTICH_HYBRIDDVR_GET_CAPTURE_FIELDS_PER_SEC();

        /* Stream 0 -DEI_SC_OUT is inputfps/2 */
        params.streamId = 0;
        params.outputFrameRate = MULTICH_HYBRIDDVR_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = 1;
        params.inputFrameRate  = MULTICH_HYBRIDDVR_GET_CAPTURE_FRAMES_PER_SEC();
        params.outputFrameRate = MULTICH_HYBRIDDVR_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);

        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = 2;
        params.inputFrameRate  = MULTICH_HYBRIDDVR_GET_CAPTURE_FRAMES_PER_SEC();
        params.outputFrameRate = MULTICH_HYBRIDDVR_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
    }
}

static
Void multich_hybriddvr_connect_links(Bool   enableScdAlgLink)
{
     multich_hybriddvr_configure_extvideodecoder_prm(NUM_CAPTURE_DEVICES);
      /**Capture Link**/
     multich_hybriddvr_set_capture_prm(&gHybridDVRUsecaseContext.capturePrm,2,TRUE);
     /* Capture ---Q0---> IpcFramesOut */
     gHybridDVRUsecaseContext.capturePrm.outQueParams[0].nextLink   = gVcapModuleContext.ipcFramesOutVpssId[0];
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.captureId;
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.notifyPrevLink = TRUE;
     MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.captureId,&gHybridDVRUsecaseContext.capturePrm,sizeof(gHybridDVRUsecaseContext.capturePrm));

     /**After Capture is created set capture fps */
     multich_hybriddvr_set_capture_fps(&gHybridDVRUsecaseContext.captureFps);

     /**IpcFramesOut Link**/
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.numOutQue = 2;
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.notifyNextLink = TRUE;
     /* IpcFramesOut ---Q0---> DEI[0] */
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.deiId[0];
     gHybridDVRUsecaseContext.deiPrm[0].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
     gHybridDVRUsecaseContext.deiPrm[0].inQueParams.prevLinkQueId  = 0;
     /* IpcFramesOut ---Q1---> DEI[1] */
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[1].nextLink = gVcapModuleContext.deiId[1];
     gHybridDVRUsecaseContext.deiPrm[1].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
     gHybridDVRUsecaseContext.deiPrm[1].inQueParams.prevLinkQueId  = 1;

     /* IpcFramesOutVpss ---ProcessLink --- IpcFramesInDsp */
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.processLink = gVcapModuleContext.ipcFramesInDspId[0];
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.notifyProcessLink = TRUE;
     gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.noNotifyMode = FALSE;
     gHybridDVRUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
     gHybridDVRUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
     MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.ipcFramesOutVpssId[0],&gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0],sizeof(gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[0]));


     /**IpcFramesOut Link**/
     /* IpcFramesInDsp ---Q0--- dspAlg0(OSD) */
     gHybridDVRUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.numOutQue   = 1;
     gHybridDVRUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[0];
     gHybridDVRUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.notifyPrevLink = TRUE;
     gHybridDVRUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.notifyNextLink = TRUE;
     gHybridDVRUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.noNotifyMode   = FALSE;
     gHybridDVRUsecaseContext.dspAlgPrm[0].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
     gHybridDVRUsecaseContext.dspAlgPrm[0].inQueParams.prevLinkQueId = 0;
     MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.ipcFramesInDspId[0],&gHybridDVRUsecaseContext.ipcFramesInDspPrm[0],sizeof(gHybridDVRUsecaseContext.ipcFramesInDspPrm[0]));
     multich_hybriddvr_set_osd_prm(&gHybridDVRUsecaseContext.dspAlgPrm[0]);
     MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.dspAlgId[0],&gHybridDVRUsecaseContext.dspAlgPrm[0],sizeof(gHybridDVRUsecaseContext.dspAlgPrm[0]));

     /**Dei0 Link**/
     multich_hybriddvr_set_dei_prm(&gHybridDVRUsecaseContext.deiPrm[0],2);
     /* DEI(0) ---DEI_LINK_OUT_QUE_DEI_SC--- mergeId[LIVE_DECODE_MERGE_LINK_IDX] */
     gHybridDVRUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            =
      gHybridDVRUsecaseContext.mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                      = LIVE_DECODE_MERGE_LINK_NUM_INQUE;
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_DEI0_QIDX].prevLinkId     = gVcapModuleContext.deiId[0];
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_DEI0_QIDX].prevLinkQueId  = DEI_LINK_OUT_QUE_DEI_SC;

     /* DEI(0) ---DEI_LINK_OUT_QUE_VIP_SC--- mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX] */
    gHybridDVRUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink            =
      gHybridDVRUsecaseContext.mergeId[D1_CIF_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].numInQue              = D1_CIF_MERGE_LINK_NUM_INQUE;
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[D1_CIF_MERGE_LINK_DEI0_QIDX].prevLinkId    = gVcapModuleContext.deiId[0];
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[D1_CIF_MERGE_LINK_DEI0_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;

     /* DEI(0) ---DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT--- mergeId[DEI_SC_CIF_MERGE_LINK_IDX] */
    gHybridDVRUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink   =
      gHybridDVRUsecaseContext.mergeId[DEI_SC_CIF_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].numInQue                      = DEI_SC_CIF_MERGE_LINK_NUM_INQUE;
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].inQueParams[DEI_SC_CIF_MERGE_LINK_DEI0_QIDX].prevLinkId     = gVcapModuleContext.deiId[0];
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].inQueParams[DEI_SC_CIF_MERGE_LINK_DEI0_QIDX].prevLinkQueId  = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.deiId[0],&gHybridDVRUsecaseContext.deiPrm[0],sizeof(gHybridDVRUsecaseContext.deiPrm[0]));

     /**Dei1 Link**/
     multich_hybriddvr_set_dei_prm(&gHybridDVRUsecaseContext.deiPrm[1],2);
     /* DEI(1) ---DEI_LINK_OUT_QUE_DEI_SC--- mergeId[LIVE_DECODE_MERGE_LINK_IDX] */
     gHybridDVRUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            =
      gHybridDVRUsecaseContext.mergeId[LIVE_DECODE_MERGE_LINK_IDX];
     gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_DEI1_QIDX].prevLinkId     = gVcapModuleContext.deiId[1];
     gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_DEI1_QIDX].prevLinkQueId  = DEI_LINK_OUT_QUE_DEI_SC;

     /* DEI(1) ---DEI_LINK_OUT_QUE_VIP_SC--- mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX] */
    gHybridDVRUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink            =
      gHybridDVRUsecaseContext.mergeId[D1_CIF_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[D1_CIF_MERGE_LINK_DEI1_QIDX].prevLinkId    = gVcapModuleContext.deiId[1];
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[D1_CIF_MERGE_LINK_DEI1_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;

     /* DEI(1) ---DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT--- mergeId[DEI_SC_CIF_MERGE_LINK_IDX] */
    gHybridDVRUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink   =
      gHybridDVRUsecaseContext.mergeId[DEI_SC_CIF_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].inQueParams[DEI_SC_CIF_MERGE_LINK_DEI1_QIDX].prevLinkId     = gVcapModuleContext.deiId[1];
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].inQueParams[DEI_SC_CIF_MERGE_LINK_DEI1_QIDX].prevLinkQueId  = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.deiId[1],&gHybridDVRUsecaseContext.deiPrm[1],sizeof(gHybridDVRUsecaseContext.deiPrm[1]));

    /** mergeId[DEI_SC_CIF_MERGE_LINK_IDX] **/
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].numInQue                      = DEI_SC_CIF_MERGE_LINK_NUM_INQUE;
    /* mergeId[DEI_SC_CIF_MERGE_LINK_IDX] ---Q0---> dupId[CIF_DUP_LINK_IDX] */
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].outQueParams.nextLink    = gHybridDVRUsecaseContext.dupId[CIF_DUP_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].notifyNextLink           = TRUE;
    gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].inQueParams.prevLinkId         = gHybridDVRUsecaseContext.mergeId[DEI_SC_CIF_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.mergeId[DEI_SC_CIF_MERGE_LINK_IDX],
                                  &gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX],
                                  sizeof(gHybridDVRUsecaseContext.mergePrm[DEI_SC_CIF_MERGE_LINK_IDX]));

    /**dupId[CIF_DUP_LINK_IDX] */
    if(enableScdAlgLink)
    {
        gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].numOutQue = 3;
    }
    else
    {
        gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].numOutQue = 2;
    }
    /* dupId[CIF_DUP_LINK_IDX] ---Q0---> mergeId[D1_CIF_MERGE_LINK_IDX] */
    gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].outQueParams[0].nextLink = gHybridDVRUsecaseContext.mergeId[D1_CIF_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].numInQue          = D1_CIF_MERGE_LINK_NUM_INQUE;
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[D1_CIF_MERGE_LINK_CIFDUP_QIDX].prevLinkId       = gHybridDVRUsecaseContext.dupId[CIF_DUP_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[D1_CIF_MERGE_LINK_CIFDUP_QIDX].prevLinkQueId    = 0;
    /* dupId[CIF_DUP_LINK_IDX] ---Q1---> mergeId[LIVE_DECODE_MERGE_LINK_IDX] */
    gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].outQueParams[1].nextLink = gHybridDVRUsecaseContext.mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_CIFDUP_QIDX].prevLinkId    = gHybridDVRUsecaseContext.dupId[CIF_DUP_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_CIFDUP_QIDX].prevLinkQueId = 1;
    gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].notifyNextLink = TRUE;

    if(enableScdAlgLink)
    {
        /* dupId[CIF_DUP_LINK_IDX] ---Q2---> mergeId[LIVE_DECODE_MERGE_LINK_IDX] */
        gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX].outQueParams[2].nextLink  = gVcapModuleContext.ipcFramesOutVpssId[1];
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkId = gHybridDVRUsecaseContext.dupId[CIF_DUP_LINK_IDX];
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 2;
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.notifyPrevLink = TRUE;
    }
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.dupId[CIF_DUP_LINK_IDX],
                                  &gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX],
                                  sizeof(gHybridDVRUsecaseContext.dupPrm[CIF_DUP_LINK_IDX]));

    if(enableScdAlgLink)
    {
        /* ipcFramesOutVpssId[1] ---Q2---> ipcFramesInDspId[1] */
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.inputFrameRate = MULTICH_HYBRIDDVR_GET_CAPTURE_FRAMES_PER_SEC();
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.outputFrameRate = 2;
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.numOutQue = 1;
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.ipcFramesInDspId[1];
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.notifyNextLink = TRUE;
        gHybridDVRUsecaseContext.ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[1];
        gHybridDVRUsecaseContext.ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        gHybridDVRUsecaseContext.ipcFramesInDspPrm[1].baseCreateParams.notifyPrevLink = TRUE;
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.processLink = SYSTEM_LINK_ID_INVALID;
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.notifyProcessLink = FALSE;
        gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1].baseCreateParams.noNotifyMode = FALSE;
        MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.ipcFramesOutVpssId[1],
                                      &gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1],
                                      sizeof(gHybridDVRUsecaseContext.ipcFramesOutVpssPrm[1]));

        gHybridDVRUsecaseContext.ipcFramesInDspPrm[1].baseCreateParams.numOutQue   = 1;
        gHybridDVRUsecaseContext.ipcFramesInDspPrm[1].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[MULTICH_DSP_ALG_LINK_SCD_IDX];
        gHybridDVRUsecaseContext.ipcFramesInDspPrm[1].baseCreateParams.notifyNextLink = TRUE;
        gHybridDVRUsecaseContext.ipcFramesInDspPrm[1].baseCreateParams.noNotifyMode   = FALSE;
        gHybridDVRUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[1];
        gHybridDVRUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX].inQueParams.prevLinkQueId = 0;
        MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.ipcFramesInDspId[1],
                                      &gHybridDVRUsecaseContext.ipcFramesInDspPrm[1],
                                      sizeof(gHybridDVRUsecaseContext.ipcFramesInDspPrm[1]));

        /* dspAlgId[1] ---Q0---> ipcBitsOutDsp */
        multich_hybriddvr_set_scd_prm(&gHybridDVRUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX]);
        gHybridDVRUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = gHybridDVRUsecaseContext.ipcBitsOutDSPId;
        gHybridDVRUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkId    = gVcapModuleContext.dspAlgId[1];
        gHybridDVRUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.dspAlgId[1],
                                      &gHybridDVRUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX],
                                      sizeof(gHybridDVRUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX]));

        /* ipcBitsOutDsp ---Q0---> ipcBitsInHlos */
        gHybridDVRUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.numOutQue                 = 1;
        gHybridDVRUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.outQueParams[0].nextLink  = gVcapModuleContext.ipcBitsInHLOSId;
        MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&gHybridDVRUsecaseContext.ipcBitsOutDspPrm,
                                                    TRUE);
        gHybridDVRUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkId = gHybridDVRUsecaseContext.ipcBitsOutDSPId;
        gHybridDVRUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.ipcBitsOutDSPId,
                                      &gHybridDVRUsecaseContext.ipcBitsOutDspPrm,
                                      sizeof(gHybridDVRUsecaseContext.ipcBitsOutDspPrm));

        gHybridDVRUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.numOutQue                 = 1;
        gHybridDVRUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;
        MultiCh_ipcBitsInitCreateParams_BitsInHLOSVcap(&gHybridDVRUsecaseContext.ipcBitsInHostPrm[1]);
        MULTICH_HYBRIDDVR_CREATE_LINK(gVcapModuleContext.ipcBitsInHLOSId,
                                      &gHybridDVRUsecaseContext.ipcBitsInHostPrm[1],
                                      sizeof(gHybridDVRUsecaseContext.ipcBitsInHostPrm[1]));
    }

    /* mergeId[D1_CIF_MERGE_LINK_IDX] ---Q0---> ipcOutVpssId */
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].outQueParams.nextLink           = gHybridDVRUsecaseContext.ipcOutVpssId;
    gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].notifyNextLink                  = TRUE;
    gHybridDVRUsecaseContext.ipcOutVpssPrm.inQueParams.prevLinkId    = gHybridDVRUsecaseContext.mergeId[D1_CIF_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;

     gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX].numInQue = D1_CIF_MERGE_LINK_NUM_INQUE;
     MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.mergeId[D1_CIF_MERGE_LINK_IDX],
                                   &gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX],
                                   sizeof(gHybridDVRUsecaseContext.mergePrm[D1_CIF_MERGE_LINK_IDX]));

    /* ipcOutVpssIdisOutVpssId ---Q0---> ipcInVideoId */
    gHybridDVRUsecaseContext.ipcOutVpssPrm.outQueParams[0].nextLink  = gHybridDVRUsecaseContext.ipcInVideoId;
    gHybridDVRUsecaseContext.ipcOutVpssPrm.notifyNextLink            = FALSE;
    gHybridDVRUsecaseContext.ipcOutVpssPrm.notifyPrevLink            = TRUE;
    gHybridDVRUsecaseContext.ipcOutVpssPrm.noNotifyMode              = TRUE;
    gHybridDVRUsecaseContext.ipcInVideoPrm.inQueParams.prevLinkId    = gHybridDVRUsecaseContext.ipcOutVpssId;
    gHybridDVRUsecaseContext.ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.ipcOutVpssId,
                                  &gHybridDVRUsecaseContext.ipcOutVpssPrm,
                                  sizeof(gHybridDVRUsecaseContext.ipcOutVpssPrm));


    /* ipcInVideoId ---Q0---> encId */
    gHybridDVRUsecaseContext.ipcInVideoPrm.numOutQue                 = 1;
    gHybridDVRUsecaseContext.ipcInVideoPrm.outQueParams[0].nextLink  = gVencModuleContext.encId;
    gHybridDVRUsecaseContext.ipcInVideoPrm.notifyNextLink            = TRUE;
    gHybridDVRUsecaseContext.ipcInVideoPrm.notifyPrevLink            = FALSE;
    gHybridDVRUsecaseContext.ipcInVideoPrm.noNotifyMode              = TRUE;
    gHybridDVRUsecaseContext.encPrm.inQueParams.prevLinkId    = gHybridDVRUsecaseContext.ipcInVideoId;
    gHybridDVRUsecaseContext.encPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.ipcInVideoId,
                                  &gHybridDVRUsecaseContext.ipcInVideoPrm,
                                  sizeof(gHybridDVRUsecaseContext.ipcInVideoPrm));


    /* encId ---Q0---> ipcBitsOutRTOSId */
    multich_hybriddvr_set_enclink_prm(&gHybridDVRUsecaseContext.encPrm);
    gHybridDVRUsecaseContext.encPrm.outQueParams.nextLink     = gVencModuleContext.ipcBitsOutRTOSId;
    gHybridDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.encId;
    gHybridDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gVencModuleContext.encId,
                                  &gHybridDVRUsecaseContext.encPrm,
                                  sizeof(gHybridDVRUsecaseContext.encPrm));

    /* ipcBitsOutVideoId ---Q0---> ipcBitsInHostId */
    gHybridDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    gHybridDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&gHybridDVRUsecaseContext.ipcBitsOutVideoPrm,
                                               TRUE);
    gHybridDVRUsecaseContext.ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.ipcBitsOutRTOSId;
    gHybridDVRUsecaseContext.ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gVencModuleContext.ipcBitsOutRTOSId,
                                  &gHybridDVRUsecaseContext.ipcBitsOutVideoPrm,
                                  sizeof(gHybridDVRUsecaseContext.ipcBitsOutVideoPrm));
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&gHybridDVRUsecaseContext.ipcBitsInHostPrm[0]);
    MULTICH_HYBRIDDVR_CREATE_LINK(gVencModuleContext.ipcBitsInHLOSId,
                                  &gHybridDVRUsecaseContext.ipcBitsInHostPrm[0],
                                  sizeof(gHybridDVRUsecaseContext.ipcBitsInHostPrm[0]));


    /* ipcBitsOutHostId ---Q0---> ipcBitsInRtosId */
    multich_hybriddvr_set_ipcbitsout_hlos_prms(&gHybridDVRUsecaseContext.ipcBitsOutHostPrm);
    gHybridDVRUsecaseContext.ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.ipcBitsInRTOSId;
    gHybridDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsOutHLOSId;
    gHybridDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId    = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gVdecModuleContext.ipcBitsOutHLOSId,
                                  &gHybridDVRUsecaseContext.ipcBitsOutHostPrm,
                                  sizeof(gHybridDVRUsecaseContext.ipcBitsOutHostPrm));

    /* ipcBitsInRtosId ---Q0---> decId */
    gHybridDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.numOutQue                    = 1;
    gHybridDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink     = gVdecModuleContext.decId;
    MultiCh_ipcBitsInitCreateParams_BitsInRTOS(&gHybridDVRUsecaseContext.ipcBitsInVideoPrm, TRUE);
    gHybridDVRUsecaseContext.decPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsInRTOSId;
    gHybridDVRUsecaseContext.decPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gVdecModuleContext.ipcBitsInRTOSId,
                                  &gHybridDVRUsecaseContext.ipcBitsInVideoPrm,
                                  sizeof(gHybridDVRUsecaseContext.ipcBitsInVideoPrm));

    /* decId---Q0--->ipcOutVideoId*/
    multich_hybriddvr_set_declink_prms(&gHybridDVRUsecaseContext.decPrm);
    gHybridDVRUsecaseContext.decPrm.outQueParams.nextLink  = gHybridDVRUsecaseContext.ipcOutVideoId;
    gHybridDVRUsecaseContext.ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    gHybridDVRUsecaseContext.ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gVdecModuleContext.decId,
                                  &gHybridDVRUsecaseContext.decPrm,
                                  sizeof(gHybridDVRUsecaseContext.decPrm));

    /*ipcOutVideoId---Q0-->ipcInVpssId*/
    gHybridDVRUsecaseContext.ipcOutVideoPrm.numOutQue                 = 1;
    gHybridDVRUsecaseContext.ipcOutVideoPrm.outQueParams[0].nextLink  = gHybridDVRUsecaseContext.ipcInVpssId;
    gHybridDVRUsecaseContext.ipcOutVideoPrm.notifyNextLink            = FALSE;
    gHybridDVRUsecaseContext.ipcOutVideoPrm.notifyPrevLink            = TRUE;
    gHybridDVRUsecaseContext.ipcOutVideoPrm.noNotifyMode              = TRUE;
    gHybridDVRUsecaseContext.ipcInVpssPrm.inQueParams.prevLinkId    = gHybridDVRUsecaseContext.ipcOutVideoId;
    gHybridDVRUsecaseContext.ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.ipcOutVideoId,
                                  &gHybridDVRUsecaseContext.ipcOutVideoPrm,
                                  sizeof(gHybridDVRUsecaseContext.ipcOutVideoPrm));

    /*ipcInVpssId---Q0--> mergeId[LIVE_DECODE_MERGE_LINK_IDX] */
    gHybridDVRUsecaseContext.ipcInVpssPrm.numOutQue                 = 1;
    gHybridDVRUsecaseContext.ipcInVpssPrm.outQueParams[0].nextLink  = gHybridDVRUsecaseContext.mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.ipcInVpssPrm.notifyNextLink            = TRUE;
    gHybridDVRUsecaseContext.ipcInVpssPrm.notifyPrevLink            = FALSE;
    gHybridDVRUsecaseContext.ipcInVpssPrm.noNotifyMode              = TRUE;
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue  = LIVE_DECODE_MERGE_LINK_NUM_INQUE;
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_VDEC_QIDX].prevLinkId    = gHybridDVRUsecaseContext.ipcInVpssId;
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[LIVE_DECODE_MERGE_LINK_VDEC_QIDX].prevLinkQueId = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.ipcInVpssId,
                                  &gHybridDVRUsecaseContext.ipcInVpssPrm,
                                  sizeof(gHybridDVRUsecaseContext.ipcInVpssPrm));

    /*mergeId[LIVE_DECODE_MERGE_LINK_IDX]---Q0--> dupId[LIVE_DECODE_DUP_LINK_IDX] */
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                 = LIVE_DECODE_MERGE_LINK_NUM_INQUE;
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].outQueParams.nextLink    = gHybridDVRUsecaseContext.dupId[LIVE_DECODE_DUP_LINK_IDX];
    gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX].notifyNextLink           = TRUE;
    gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkId       = gHybridDVRUsecaseContext.mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.mergeId[LIVE_DECODE_MERGE_LINK_IDX],
                                  &gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX],
                                  sizeof(gHybridDVRUsecaseContext.mergePrm[LIVE_DECODE_MERGE_LINK_IDX]));

    /*dupId[LIVE_DECODE_DUP_LINK_IDX]---Q0--> swMsId[0] */
    gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX].numOutQue                      = 2;
    gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[0].nextLink       = gVdisModuleContext.swMsId[0];
    gHybridDVRUsecaseContext.swMsPrm[0].inQueParams.prevLinkId    = gHybridDVRUsecaseContext.dupId[LIVE_DECODE_DUP_LINK_IDX];
    gHybridDVRUsecaseContext.swMsPrm[0].inQueParams.prevLinkQueId = 0;

    /*dupId[LIVE_DECODE_DUP_LINK_IDX]---Q1--> swMsId[1] */
    gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX].numOutQue                      = 2;
    gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[1].nextLink       = gVdisModuleContext.swMsId[1];
    gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX].notifyNextLink                 = TRUE;
    gHybridDVRUsecaseContext.swMsPrm[1].inQueParams.prevLinkId    = gHybridDVRUsecaseContext.dupId[LIVE_DECODE_DUP_LINK_IDX];
    gHybridDVRUsecaseContext.swMsPrm[1].inQueParams.prevLinkQueId = 1;
    MULTICH_HYBRIDDVR_CREATE_LINK(gHybridDVRUsecaseContext.dupId[LIVE_DECODE_DUP_LINK_IDX],
                                  &gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX],
                                  sizeof(gHybridDVRUsecaseContext.dupPrm[LIVE_DECODE_DUP_LINK_IDX]));

    /* Avsync configuration for SwMs[0] */
    mulich_hybriddvr_set_avsync_prm(&gHybridDVRUsecaseContext.avsyncCfg[0],
                                    0,
                                    gHybridDVRUsecaseContext.dupId[LIVE_DECODE_DUP_LINK_IDX],
                                    0);
    /*swMsId[0]---Q0--> displayId[VDIS_DEV_HDMI] */
    gHybridDVRUsecaseContext.swMsPrm[0].outQueParams.nextLink     = Vdis_getDisplayId(VDIS_DEV_HDMI);
    gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)].numInputQueues = 1;
    gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[0];
    gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)].inQueParams[0].prevLinkQueId = 0;
    multich_hybriddvr_set_swms_prm(&gHybridDVRUsecaseContext.swMsPrm[0],
                                    0);
    MULTICH_HYBRIDDVR_CREATE_LINK(gVdisModuleContext.swMsId[0],
                                 &gHybridDVRUsecaseContext.swMsPrm[0],
                                 sizeof(gHybridDVRUsecaseContext.swMsPrm[0]));

    /* Avsync configuration for SwMs[1] */
    mulich_hybriddvr_set_avsync_prm(&gHybridDVRUsecaseContext.avsyncCfg[1],
                                    1,
                                    gHybridDVRUsecaseContext.dupId[LIVE_DECODE_DUP_LINK_IDX],
                                    1);
    /*swMsId[1]---Q1--> displayId[VDIS_DEV_SD] */
    gHybridDVRUsecaseContext.swMsPrm[1].outQueParams.nextLink     = Vdis_getDisplayId(VDIS_DEV_SD);
    gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_SD)].numInputQueues = 1;
    gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_SD)].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[1];
    gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_SD)].inQueParams[0].prevLinkQueId = 0;
    multich_hybriddvr_set_swms_prm(&gHybridDVRUsecaseContext.swMsPrm[1],
                                    1);
    MULTICH_HYBRIDDVR_CREATE_LINK(gVdisModuleContext.swMsId[1],
                                  &gHybridDVRUsecaseContext.swMsPrm[1],
                                  sizeof(gHybridDVRUsecaseContext.swMsPrm[1]));

    multich_hybriddvr_set_display_prms(&gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)],
                                        gHybridDVRUsecaseContext.swMsPrm[0].initOutRes);
    MULTICH_HYBRIDDVR_CREATE_LINK(Vdis_getDisplayId(VDIS_DEV_HDMI),
                                  &gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)],
                                  sizeof(gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)]));
    multich_hybriddvr_set_display_prms(&gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_SD)],
                                        gHybridDVRUsecaseContext.swMsPrm[1].initOutRes);
    MULTICH_HYBRIDDVR_CREATE_LINK(Vdis_getDisplayId(VDIS_DEV_SD),
                                  &gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_SD)],
                                  sizeof(gHybridDVRUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_SD)]));
}

static
void multich_hybriddvr_set_dec2disp_chmap()
{
    MergeLink_InLinkChInfo inChInfo;

    MergeLink_InLinkChInfo_Init(&inChInfo);
    inChInfo.inLinkID = gHybridDVRUsecaseContext.ipcInVpssId;
    System_linkControl(gHybridDVRUsecaseContext.mergeId[LIVE_DECODE_MERGE_LINK_IDX],
                       MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO,
                       &inChInfo,
                       sizeof(inChInfo),
                       TRUE);
    OSA_assert(inChInfo.numCh == gVdecModuleContext.vdecConfig.numChn);

    MultiCh_setDec2DispMap(VDIS_DEV_HDMI,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
    MultiCh_setDec2DispMap(VDIS_DEV_SD,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
}

Void MultiCh_createHybridDVR()
{

    multich_hybriddvr_reset_link_prms();
    multich_hybriddvr_set_link_ids();
    printf("\n********* Entered usecase HybridDVR <816x> Cap/Enc/Dec/Dis \n\n");

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

    if ((FALSE == TILER_ENABLE_ENCODE) && (FALSE == TILER_ENABLE_DECODE))
    {
        SystemTiler_disableAllocator();
    }
    multich_hybriddvr_connect_links(gVsysModuleContext.vsysConfig.enableScd);
    multich_hybriddvr_set_links_framerate();
    multich_hybriddvr_set_dec2disp_chmap();
}

Void MultiCh_deleteHybridDVR()
{
    UInt32 i;

    for (i = 0; i < gHybridDVRUsecaseContext.createdLinkCount; i++)
    {
        System_linkDelete (gHybridDVRUsecaseContext.createdLinks[i]);
    }
    gHybridDVRUsecaseContext.createdLinkCount = 0;
    multich_hybriddvr_reset_link_ids();

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

    if ((FALSE == TILER_ENABLE_ENCODE) && (FALSE == TILER_ENABLE_DECODE))
    {
        SystemTiler_enableAllocator();
    }
}



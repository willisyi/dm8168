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

       +----------------------+       +----------------------+      +----------------------+
       |   TVP5158 1 4CH D1   |       |   TVP5158 2 4CH D1   |      |   TVP5158 3 1CH D1   |
       +----------------------+       +----------------------+      +----------------------+
                 V                              +                               V
 +------------------------------------+         |          +------------------------------------+
 |                                    |         |          |                                    |
 |      DEI  (VIP-SC YUV420 )         |         |          |      DEI  (VIP-SC YUV420 )         |
 |           (DEI-SC YUV422I )        |         +--------->|           (DEI-SC YUV422I )        |
 |                                    |                    |                                    |
 +------------------------------------+                    +------------------------------------+
    +        +        +       +      +                        +        +        +       +      +
    |        |        |       |      |                        |        |        |       |      |
    |        |        |       |      |                        |        |        |       |      |
    |        v        v       v      v                        |        v        v       v      v
  DEI_SC   DEI_SC   DEI_SC  VIP_SC VIP_SC                   DEI_SC   DEI_SC   DEI_SC  VIP_SC VIP_SC
  YUV422   YUV422   YUV422  YUV420 YUV420                   YUV422   YUV422   YUV422  YUV420 YUV420
 PRIMARY    SEC      TER     PRI    SEC                    PRIMARY    SEC      TER     PRI    SEC
   30       15       15      30     30                       30       15       15      30     30
  fps      fps      fps     fps    fps                      fps      fps      fps     fps    fps
    +        +       +       +      +                         +        +       +       +      +
    |        |       v       |      |                         |        |       v       |      |
    |        +---------------)------)-----------+-------------)--------+-------+       |      |
    |                +-------)------)-----------)-------------+                        |      |
    |                |       |      |           |                                      |      |
    |    9chYUV422   |       |      |           |18chYUV422                            |      |
    |      30 fps    |       |      |           |   15fps                              |      |
    |                |       |      |           |                                      |      |
    v                v       |      |           v                                      |      |
 +--------------------+      |      |   +--------------------+                         |      |
 | DSP_ALG_MERGE_LINK |      |      |   | NSF_PRE_MERGE_LINK |                         |      |
 +--------------------+      |      |   +--------------------+                         |      |
          +                  |      |           +                                      |      |
          |                  |      +-----------|--------------------------------+     |      |
          v                  |                  |                                |     |      |
     +------------+          +------------------|------------------------+       |     |      |
     |   DUP      |+--------+                   | 18chYUV422             |       |     |      |
     +------------+         |                   v  15fps                 v       v     v      v
          +                 |           +---------------------+     +------------------------------+
          v                 |           |      NSF LINK       |+--->|    PRE_ENCODE_MERGE_LINK     |36ch (18 MJPEG+9 H264 + 9 H264)
  +------------------+      |           +---------------------+     +------------------------------+
  |IPC_FRM_OUT_VPSS_1|      |                                                          +
  +------------------+      |                                                   +-------------+
          +                 |                                                   |IPCM3OUT(VPS)|
          v                 |                                                   +-------------+
   +----------------+       |                                                          +
   |IPC_FRM_IN_DSP_1|       |                                                          v
   +----------------+       |                                                   +------------+     +----+      +--------------------+   +--------------------+
          v                 |                                                   |IPCM3IN(VID)| +-->|ENC |+---> |IPCBITS_RTOSOUT(VID)|+->|IPCBITS_HLOSIN(HOST)|  FILEOUT
   +---------------+        |                                                   +------------+     +----+      +--------------------+   +--------------------+
   |ALG_LINK_1(SCD)|        |
   +---------------+        |                                +------------+     +-------------+   +----+      +-------------------+    +---------------------+
          +                 |                                |IPCM3IN(VPS)|<---+|IPCM3OUT(VID)|<--|DEC |<----+|IPCBITS_RTOSIN(VID)|<--+|IPCBITS_HLOSOUT(HOST)|
          v                 |                                +------------+     +-------------+   +----+      +-------------------+    +---------------------+
  +--------------------+    |       +-------------------+          +
  |IPCBITS_RTOSOUT(DSP)|    |       |                   |          |
  +--------------------+    +------>|  PRE_SW_MS_MERGE  |<---------+
          +                         +-------------------+
          v                                   +
  +--------------------+                      |
  |IPCBITS_HLOSIN1HOST)|                      v
  +--------------------+               +-------------+
                                       | SW Mosaic   |
        FILEOUT                        |(SC5 YUV422I)|
                                       +-------------+
                                           GRPX0
                                         On-Chip HDMI
                                            1080p60

*/

#define     NUM_CAPTURE_DEVICES          3
#define     NUM_SD_DECODE_CHANNELS      (1)
#define     NUM_HD_DECODE_CHANNELS      (0)
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
#define     NUM_BUFS_PER_CH_DEI_DEI_SC_SECQ      (3)
#define     NUM_BUFS_PER_CH_DEI_DEI_SC_TERQ      (3)
#define     NUM_BUFS_PER_CH_DEC_SD               (4)
#define     NUM_BUFS_PER_CH_DEC_HD               (3)
#define     NUM_BUFS_PER_CH_SWMS_HD              (4)
#define     NUM_BUFS_PER_CH_SWMS_SD              (4)
#define     NUM_BUFS_PER_CH_ENC_PRI              (4)
#define     NUM_BUFS_PER_CH_ENC_SEC              (3)
#define     NUM_BUFS_PER_CH_BITSOUT_SD           (4)
#define     NUM_BUFS_PER_CH_BITSOUT_HD           (3)
#define     NUM_BUFS_PER_CH_NSF                  (4)



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
        .EncNumCh  = 16,
        .EncChList = {0, 3, 6, 9, 12, 15, 18, 21, 24,27,30,33,36,39,42,45},
        .DecNumCh  = 1,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 16,
        .EncChList = {1, 4, 7, 10, 13, 16, 19, 22, 25,28,31,34,37,40,43,46},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 16,
        .EncChList = {2, 5, 8, 11, 14, 17, 20, 23, 26,29,32,35,38,41,44,47},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

/** Merge Link Info */
#define     NUM_MERGE_LINK                         (4)

/**  DEI out mjpeg  Merge
 *   DEI0 ---DEI_DEI_SC_SEC_Q-->Q0---|
 *                                   |
 *   DEI0 ---DEI_DEI_TER_SEC_Q-->Q1--|
 *                                   |-DEI_OUT_MJPEG_MERGE_LINK_IDX
 *   DEI1 ---DEI_DEI_SC_SEC_Q-->Q2---|
 *                                   |
 *   DEI1 ---DEI_DEI_TER_SEC_Q-->Q3--|
 */
#define     DEI_OUT_MJPEG_MERGE_LINK_IDX            0
#define     DEI_OUT_MJPEG_MERGE_LINK_NUM_INQUE     (4)
#define     DEI_OUT_MJPEG_MERGE_LINK_DEI0_SC_QIDX  (0)
#define     DEI_OUT_MJPEG_MERGE_LINK_DEI0_TER_QIDX (1)
#define     DEI_OUT_MJPEG_MERGE_LINK_DEI1_SC_QIDX  (2)
#define     DEI_OUT_MJPEG_MERGE_LINK_DEI1_TER_QIDX (3)

/**  Pre Encode Merge
 *   DEI0 ------------------DEI_VIP_SC_PRI_Q-->Q0--|
 *                                                 |
 *   DEI0 ------------------DEI_VIP_SC_SEC_Q-->Q1--|
 *                                                 |
 *   NSF -------------------Q0---------------->Q2--|
 *                                                 |
 *   DEI1 ------------------DEI_VIP_SC_PRI_Q-->Q3--|
 *                                                 |
 *   DEI1 ------------------DEI_VIP_SC_SEC_Q-->Q4--|
 *                                                 |
 */
#define     PRE_ENCODE_MERGE_LINK_IDX                 1
#define     PRE_ENCODE_MERGE_LINK_NUM_INQUE          (5)
#define     PRE_ENCODE_MERGE_LINK_DEI0_VIP_PRI_QIDX  (0)
#define     PRE_ENCODE_MERGE_LINK_DEI0_VIP_SEC_QIDX  (1)
#define     PRE_ENCODE_MERGE_LINK_DEI1_VIP_PRI_QIDX  (2)
#define     PRE_ENCODE_MERGE_LINK_DEI1_VIP_SEC_QIDX  (3)
#define     PRE_ENCODE_MERGE_LINK_NSF_QIDX           (4)

/**  Pre DSP alg merge
 *   DEI0 ---DEI_DEI_SC_Q-->Q0--|
 *                              |-PRE_DSPALG_MERGE_LINK_IDX
 *   DEI1 ---DEI_DEI_SC_Q-->Q1--|
 */
#define     PRE_DSPALG_MERGE_LINK_IDX              2
#define     PRE_DSPALG_MERGE_LINK_NUM_INQUE       (2)
#define     PRE_DSPALG_MERGE_LINK_DEI0_QIDX       (0)
#define     PRE_DSPALG_MERGE_LINK_DEI1_QIDX       (1)

/**  Pre SWMS merge
 *   DEI_SC_OUT_DUP ---Q0--|
 *                         |-PRE_SWMS_MERGE_LINK_IDX
 *   DEC --------------Q0--|
 */
#define     PRE_SWMS_MERGE_LINK_IDX                   3
#define     PRE_SWMS_MERGE_LINK_NUM_INQUE            (2)
#define     PRE_SWMS_MERGE_LINK_DEI_SC_OUT_DUP_QIDX  (0)
#define     PRE_SWMS_MERGE_LINK_DEC_OUT_QIDX         (1)

#define     NUM_DUP_LINK                           2
#define     CAPTURE_DUP_LINK_IDX                   0
#define     DEI_SC_PRI_OUT_DUP_LINK_IDX            1



typedef struct MultichHybridEnc_Context
{
    UInt32 ipcBitsOutDSPId;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    UInt32 createdLinkCount;
    UInt32 createdLinks[MULTICH_HYBRID_DVR_USECASE_MAX_NUM_LINKS];
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
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
}  MultichHybridEnc_Context;

MultichHybridEnc_Context gHybridEncUsecaseContext =
{
    .createdLinkCount           = 0
};


static Void multich_hybridenc_register_created_link(MultichHybridEnc_Context *pContext,
                                                    UInt32 linkID)
{
    OSA_assert(pContext->createdLinkCount < OSA_ARRAYSIZE(pContext->createdLinks));
    pContext->createdLinks[pContext->createdLinkCount] = linkID;
    pContext->createdLinkCount++;
}

#define MULTICH_HYBRIDENC_GET_CAPTURE_FIELDS_PER_SEC()           (gHybridEncUsecaseContext.captureFps)
#define MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC()           (gHybridEncUsecaseContext.captureFps/2)

#define MULTICH_HYBRIDENC_CREATE_LINK(linkID,createPrm,createPrmSize)           \
    do                                                                          \
    {                                                                           \
        System_linkCreate(linkID,createPrm,createPrmSize);                      \
        multich_hybridenc_register_created_link(&gHybridEncUsecaseContext,      \
                                                linkID);                        \
    } while (0)

static
Void multich_hybridenc_reset_link_prms()
{
    int i;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridEncUsecaseContext.ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridEncUsecaseContext.ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridEncUsecaseContext.ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gHybridEncUsecaseContext.ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,gHybridEncUsecaseContext.ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,gHybridEncUsecaseContext.ipcBitsInHostPrm[0]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,gHybridEncUsecaseContext.ipcBitsInHostPrm[1]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,gHybridEncUsecaseContext.ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,gHybridEncUsecaseContext.ipcFramesInDspPrm[0]);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,gHybridEncUsecaseContext.ipcFramesInDspPrm[1]);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0]);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,gHybridEncUsecaseContext.ipcFramesOutVpssPrm[1]);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,   gHybridEncUsecaseContext.ipcBitsOutDspPrm);
    IpcBitsOutLinkHLOS_CreateParams_Init(&gHybridEncUsecaseContext.ipcBitsOutHostPrm);
    DecLink_CreateParams_Init(&gHybridEncUsecaseContext.decPrm);
    EncLink_CreateParams_Init(&gHybridEncUsecaseContext.encPrm);
    MULTICH_INIT_STRUCT(NsfLink_CreateParams,gHybridEncUsecaseContext.nsfPrm);
    CaptureLink_CreateParams_Init(&gHybridEncUsecaseContext.capturePrm);
    for (i = 0; i < OSA_ARRAYSIZE(gHybridEncUsecaseContext.displayPrm);i++)
    {
        DisplayLink_CreateParams_Init(&gHybridEncUsecaseContext.displayPrm[i]);
    }
    for (i = 0; i < OSA_ARRAYSIZE(&gHybridEncUsecaseContext.dspAlgPrm);i++)
    {
        AlgLink_CreateParams_Init(&gHybridEncUsecaseContext.dspAlgPrm[i]);
    }
    for (i = 0; i < OSA_ARRAYSIZE(&gHybridEncUsecaseContext.swMsPrm);i++)
    {
        SwMsLink_CreateParams_Init(&gHybridEncUsecaseContext.swMsPrm[i]);
    }

     for (i = 0; i < OSA_ARRAYSIZE(&gHybridEncUsecaseContext.deiPrm);i++)
     {
         DeiLink_CreateParams_Init(&gHybridEncUsecaseContext.deiPrm[i]);
     }
     for (i = 0; i < OSA_ARRAYSIZE(&gHybridEncUsecaseContext.avsyncCfg);i++)
     {
         AvsyncLink_LinkSynchConfigParams_Init(&gHybridEncUsecaseContext.avsyncCfg[i]);
     }
}

static
Void multich_hybridenc_set_capture_prm(CaptureLink_CreateParams *capturePrm)
{
    CaptureLink_VipInstParams         *pCaptureInstPrm;
    CaptureLink_OutParams             *pCaptureOutPrm;
    UInt32 vipInstId;

    /* This is for TVP5158 Audio Channels - Change it to 16 if there are 16 audio channels connected in cascade */
    capturePrm->numVipInst                 = NUM_CAPTURE_DEVICES;

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
        if (2 == vipInstId)
        {
            pCaptureInstPrm->numChPerOutput = 1;
        }
        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = 0;
        pCaptureOutPrm->scOutHeight         = 0;
        if (0 == vipInstId)
        {
            pCaptureOutPrm->outQueId          = 0;
        }
        else
        {
            pCaptureOutPrm->outQueId          = 1;
        }
    }
}

static
Void multich_hybridenc_configure_extvideodecoder_prm(UInt32 numCaptureDevices)
{
    int i;

    for(i = 0; i < numCaptureDevices; i++)
    {
        gHybridEncUsecaseContext.vidDecVideoModeArgs[i].videoIfMode        = DEVICE_CAPT_VIDEO_IF_MODE_8BIT;
        gHybridEncUsecaseContext.vidDecVideoModeArgs[i].videoDataFormat    = SYSTEM_DF_YUV422P;
        gHybridEncUsecaseContext.vidDecVideoModeArgs[i].standard           = SYSTEM_STD_MUX_4CH_D1;
        gHybridEncUsecaseContext.vidDecVideoModeArgs[i].videoCaptureMode   =
                    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_PIXEL_MUX_EMBEDDED_SYNC;
        gHybridEncUsecaseContext.vidDecVideoModeArgs[i].videoSystem        =
                                      DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT;
        gHybridEncUsecaseContext.vidDecVideoModeArgs[i].videoCropEnable    = FALSE;
        gHybridEncUsecaseContext.vidDecVideoModeArgs[i].videoAutoDetectTimeout = -1;
    }
    Vcap_configVideoDecoder(&gHybridEncUsecaseContext.vidDecVideoModeArgs[0],
                            NUM_CAPTURE_DEVICES);
}


static
Void multich_hybridenc_set_dei_prm(DeiLink_CreateParams *deiPrm)
{
    int deiChIdx;

    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC] = NUM_BUFS_PER_CH_DEI_DEIQ;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC] = NUM_BUFS_PER_CH_DEI_VIP_SC_PRIQ;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = NUM_BUFS_PER_CH_DEI_VIP_SC_SECQ;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT] = NUM_BUFS_PER_CH_DEI_DEI_SC_SECQ;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT] = NUM_BUFS_PER_CH_DEI_DEI_SC_TERQ;

    /* Set Output Scaling at DEI based on ratio */
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator = 2;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 2;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][deiChIdx] =
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];
    }

    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.denominator = 4;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.denominator = 4;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][deiChIdx] =
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0];
    }

    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator = 2;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator = 2;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][deiChIdx] =
            deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];
    }

    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT][0].ratio.heightRatio.denominator = 3;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT][0].ratio.widthRatio.denominator = 3;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT][deiChIdx] =
            deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT][0];
    }


    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT][0].ratio.heightRatio.denominator = 5;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT][0].ratio.widthRatio.denominator = 5;
    for (deiChIdx = 1; deiChIdx < DEI_LINK_MAX_CH; deiChIdx++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT][deiChIdx] =
            deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT][0];
    }

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]          = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT]          = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT]           = TRUE;
    /* If osd is not enabled then by default enable the tiler mode */
    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]  = TILER_ENABLE_ENCODE;
    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]  = TILER_ENABLE_ENCODE;
    deiPrm->comprEnable                                   = FALSE;
    deiPrm->setVipScYuv422Format                          = FALSE;
}

static
Void multich_hybridenc_set_enclink_prm(EncLink_CreateParams *encPrm)
{
    int i,j;
    EncLink_ChCreateParams *pLinkChPrm;
    EncLink_ChDynamicParams *pLinkDynPrm;
    VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
    VENC_CHN_PARAMS_S *pChPrm;

    encPrm->numBufPerCh[ENC_LINK_SECONDARY_STREAM_POOL_ID] = NUM_BUFS_PER_CH_ENC_SEC;
    encPrm->numBufPerCh[ENC_LINK_PRIMARY_STREAM_POOL_ID] = NUM_BUFS_PER_CH_ENC_PRI;
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
    for (i=gVencModuleContext.vencConfig.numPrimaryChn + gVencModuleContext.vencConfig.numSecondaryChn;
              i<(VENC_CHN_MAX); i++)
    {
        pLinkChPrm  = &(encPrm->chCreateParams[i]);
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


}

static
Void multich_hybridenc_set_ipcbitsout_hlos_prms(IpcBitsOutLinkHLOS_CreateParams * ipcBitsOutHostPrm)
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
Void multich_hybridenc_set_declink_prms(DecLink_CreateParams *decPrm)
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
Void multich_hybridenc_set_swms_prm(SwMsLink_CreateParams *swMsPrm)
{
    UInt32 devId;

    swMsPrm->numSwMsInst = 1;
    swMsPrm->swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm->maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN + 6;
    swMsPrm->numOutBuf = NUM_BUFS_PER_CH_SWMS_HD;
    swMsPrm->maxOutRes             = VSYS_STD_1080P_60;
    swMsPrm->initOutRes            = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_HDMI].resolution;
    swMsPrm->numOutBuf = NUM_BUFS_PER_CH_SWMS_HD;
    devId              = VDIS_DEV_HDMI;
    swMsPrm->lineSkipMode = FALSE;
    swMsPrm->enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

    MultiCh_swMsGetDefaultLayoutPrm(devId, swMsPrm, FALSE);
}

static
Void mulich_hybridenc_set_avsync_vidque_prm(Avsync_SynchConfigParams *queCfg,
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
Void mulich_hybridenc_set_avsync_prm(AvsyncLink_LinkSynchConfigParams *avsyncPrm,
                                     UInt32 prevLinkID,
                                     UInt32 prevLinkQueId)
{
    System_LinkInfo                   swmsInLinkInfo;
    Int i;
    Int32 status;

    Vdis_getAvsyncConfig(VDIS_DEV_SD,avsyncPrm);
    avsyncPrm->displayLinkID        = Vdis_getDisplayId(VDIS_DEV_SD);
    avsyncPrm->videoSynchLinkID = gVdisModuleContext.swMsId[0];
    System_linkGetInfo(prevLinkID,&swmsInLinkInfo);
    OSA_assert(swmsInLinkInfo.numQue > prevLinkQueId);

    avsyncPrm->numCh            = swmsInLinkInfo.queInfo[prevLinkQueId].numCh;
    avsyncPrm->syncMasterChnum = AVSYNC_INVALID_CHNUM;
    for (i = 0; i < avsyncPrm->numCh;i++)
    {
        mulich_hybridenc_set_avsync_vidque_prm(&avsyncPrm->queCfg[i],
                                               i,
                                               0,
                                               (0 + (gVdecModuleContext.vdecConfig.numChn - 1)));
    }
    Vdis_setAvsyncConfig(VDIS_DEV_SD,avsyncPrm);
    status = Avsync_configSyncConfigInfo(avsyncPrm);
    OSA_assert(status == 0);

}


static
Void multich_hybridenc_set_scd_prm(AlgLink_CreateParams *dspAlgPrm,UInt32 prevLinkID)
{
    Int32   numBlksInFrame;
    Int32   numHorzBlks, numVertBlks, chIdx;
    UInt32  x, y, i;
    System_LinkInfo   ipcFramesInLinkInfo;
    UInt32 numCh;

    System_linkGetInfo(prevLinkID,&ipcFramesInLinkInfo);
    OSA_assert(ipcFramesInLinkInfo.numQue == 1);

    numCh            = ipcFramesInLinkInfo.queInfo[0].numCh;

    dspAlgPrm->enableOSDAlg = FALSE;
    dspAlgPrm->enableSCDAlg = FALSE;
    dspAlgPrm->scdCreateParams.maxWidth               = 352;
    if(Vcap_isPalMode())
       dspAlgPrm->scdCreateParams.maxHeight              = 288;
    else
       dspAlgPrm->scdCreateParams.maxHeight              = 240;
    dspAlgPrm->scdCreateParams.maxStride              = 352;
    dspAlgPrm->scdCreateParams.numValidChForSCD       = numCh;

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
Void multich_hybridenc_set_nsf_prm(NsfLink_CreateParams *nsfPrm)
{
    nsfPrm->bypassNsf                        = TRUE;
    nsfPrm->tilerEnable                      = TILER_ENABLE_ENCODE;
    nsfPrm->numOutQue                        = 1;
    nsfPrm->numBufsPerCh                     = NUM_BUFS_PER_CH_NSF;
    nsfPrm->inputFrameRate                   = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();;
    nsfPrm->outputFrameRate                  = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();;
}



static
Void multich_hybridenc_set_display_prms(DisplayLink_CreateParams *displayPrm,
                                        UInt32 maxOutRes)
{
    displayPrm->displayRes = maxOutRes;
}

static
Void multich_hybridenc_set_link_ids()
{
    int    i;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;

    gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
    gVcapModuleContext.ipcFramesInDspId[0] = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
    gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_ALG_0;
    gVcapModuleContext.dspAlgId[1] = SYSTEM_LINK_ID_ALG_1;
    gHybridEncUsecaseContext.ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
    gVcapModuleContext.ipcBitsInHLOSId       = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
    for (i = 0; i < NUM_MERGE_LINK;i++)
    {
        gHybridEncUsecaseContext.mergeId[i] = SYSTEM_VPSS_LINK_ID_MERGE_0 + i;
    }
    for (i = 0; i < NUM_DUP_LINK;i++)
    {
        gHybridEncUsecaseContext.dupId[i] = SYSTEM_VPSS_LINK_ID_DUP_0 + i;
    }

    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;

    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)] = SYSTEM_LINK_ID_DISPLAY_0; /* HD HDMI */
    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_SD)] = SYSTEM_LINK_ID_DISPLAY_2; /* SD HDMI */


    gHybridEncUsecaseContext.ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    gHybridEncUsecaseContext.ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    gHybridEncUsecaseContext.ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    gHybridEncUsecaseContext.ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

}

static
Void multich_hybridenc_reset_link_ids()
{
    int    i;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_INVALID;

    gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcFramesInDspId[0] = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.dspAlgId[1] = SYSTEM_LINK_ID_INVALID;
    gHybridEncUsecaseContext.ipcBitsOutDSPId = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcBitsInHLOSId       = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
    for (i = 0; i < NUM_MERGE_LINK;i++)
    {
        gHybridEncUsecaseContext.mergeId[i] = SYSTEM_LINK_ID_INVALID;
    }
    for (i = 0; i < NUM_DUP_LINK;i++)
    {
        gHybridEncUsecaseContext.dupId[i] = SYSTEM_LINK_ID_INVALID;
    }

    gVencModuleContext.encId        = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_INVALID;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_INVALID;

    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)] = SYSTEM_LINK_ID_INVALID; /* HD HDMI */
    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_SD)] = SYSTEM_LINK_ID_INVALID; /* SD HDMI */

    gHybridEncUsecaseContext.ipcOutVpssId = SYSTEM_LINK_ID_INVALID;
    gHybridEncUsecaseContext.ipcInVideoId = SYSTEM_LINK_ID_INVALID;
    gHybridEncUsecaseContext.ipcOutVideoId= SYSTEM_LINK_ID_INVALID;
    gHybridEncUsecaseContext.ipcInVpssId  = SYSTEM_LINK_ID_INVALID;

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_LINK_ID_INVALID;
    gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_LINK_ID_INVALID;
}

static
Void multich_hybridenc_set_capture_fps(UInt32 *captureFps)
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


static
Void multich_hybridenc_set_links_framerate()
{
    Int32 status;
    DeiLink_ChFpsParams params;
    UInt32 chId;

    for (chId = 0; chId < gVcapModuleContext.vcapConfig.numChn;chId++)
    {
        /* Capture -> Dei */
        params.chId = chId;
        params.inputFrameRate = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();

        /* Stream 0 -DEI_SC_OUT is inputfps/2 */
        params.streamId = DEI_LINK_OUT_QUE_DEI_SC;
        params.outputFrameRate = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = DEI_LINK_OUT_QUE_VIP_SC;
        params.inputFrameRate  = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        params.outputFrameRate = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);

        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
        params.inputFrameRate  = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        params.outputFrameRate = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);

        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT;
        params.inputFrameRate  = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        params.outputFrameRate = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);

        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT;
        params.inputFrameRate  = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        params.outputFrameRate = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);


    }
}

static
Void multich_hybridenc_connect_links()
{
    multich_hybridenc_configure_extvideodecoder_prm(NUM_CAPTURE_DEVICES);
     /**Capture Link**/
    multich_hybridenc_set_capture_prm(&gHybridEncUsecaseContext.capturePrm);

    gHybridEncUsecaseContext.capturePrm.outQueParams[0].nextLink   = gVcapModuleContext.deiId[0];
    gHybridEncUsecaseContext.deiPrm[0].inQueParams.prevLinkId = gVcapModuleContext.captureId;
    gHybridEncUsecaseContext.deiPrm[0].inQueParams.prevLinkQueId  = 0;
    gHybridEncUsecaseContext.capturePrm.outQueParams[1].nextLink   = gVcapModuleContext.deiId[1];
    gHybridEncUsecaseContext.deiPrm[1].inQueParams.prevLinkId = gVcapModuleContext.captureId;
    gHybridEncUsecaseContext.deiPrm[1].inQueParams.prevLinkQueId  = 1;

    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.captureId,&gHybridEncUsecaseContext.capturePrm,sizeof(gHybridEncUsecaseContext.capturePrm));

    /**After Capture is created set capture fps */
    multich_hybridenc_set_capture_fps(&gHybridEncUsecaseContext.captureFps);

    /**Dei0 Link**/
    multich_hybridenc_set_dei_prm(&gHybridEncUsecaseContext.deiPrm[0]);
    gHybridEncUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink = gHybridEncUsecaseContext.mergeId[PRE_DSPALG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX].inQueParams[PRE_DSPALG_MERGE_LINK_DEI0_QIDX].prevLinkId = gVcapModuleContext.deiId[0];
    gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX].inQueParams[PRE_DSPALG_MERGE_LINK_DEI0_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    gHybridEncUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT].nextLink = gHybridEncUsecaseContext.mergeId[DEI_OUT_MJPEG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI0_SC_QIDX].prevLinkId = gVcapModuleContext.deiId[0];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI0_SC_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT;
    gHybridEncUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT].nextLink = gHybridEncUsecaseContext.mergeId[DEI_OUT_MJPEG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI0_TER_QIDX].prevLinkId = gVcapModuleContext.deiId[0];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI0_TER_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT;
    gHybridEncUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = gHybridEncUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI0_VIP_PRI_QIDX].prevLinkId = gVcapModuleContext.deiId[0];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI0_VIP_PRI_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    gHybridEncUsecaseContext.deiPrm[0].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink = gHybridEncUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI0_VIP_SEC_QIDX].prevLinkId = gVcapModuleContext.deiId[0];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI0_VIP_SEC_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.deiId[0],&gHybridEncUsecaseContext.deiPrm[0],sizeof(gHybridEncUsecaseContext.deiPrm[0]));

    /**Dei1 Link**/
    multich_hybridenc_set_dei_prm(&gHybridEncUsecaseContext.deiPrm[1]);
    gHybridEncUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink = gHybridEncUsecaseContext.mergeId[PRE_DSPALG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX].inQueParams[PRE_DSPALG_MERGE_LINK_DEI1_QIDX].prevLinkId = gVcapModuleContext.deiId[1];
    gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX].inQueParams[PRE_DSPALG_MERGE_LINK_DEI1_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;

    gHybridEncUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT].nextLink = gHybridEncUsecaseContext.mergeId[DEI_OUT_MJPEG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI1_SC_QIDX].prevLinkId = gVcapModuleContext.deiId[1];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI1_SC_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC_SECONDARY_OUT;

    gHybridEncUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT].nextLink = gHybridEncUsecaseContext.mergeId[DEI_OUT_MJPEG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI1_TER_QIDX].prevLinkId = gVcapModuleContext.deiId[1];
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].inQueParams[DEI_OUT_MJPEG_MERGE_LINK_DEI1_TER_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC_TERTIARY_OUT;

    gHybridEncUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = gHybridEncUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI1_VIP_PRI_QIDX].prevLinkId = gVcapModuleContext.deiId[1];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI1_VIP_PRI_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;

    gHybridEncUsecaseContext.deiPrm[1].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink = gHybridEncUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI1_VIP_SEC_QIDX].prevLinkId = gVcapModuleContext.deiId[1];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_DEI1_VIP_SEC_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.deiId[1],&gHybridEncUsecaseContext.deiPrm[1],sizeof(gHybridEncUsecaseContext.deiPrm[1]));

    gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX].notifyNextLink = TRUE;
    gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX].numInQue       = PRE_DSPALG_MERGE_LINK_NUM_INQUE;
    gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX].outQueParams.nextLink = gHybridEncUsecaseContext.dupId[DEI_SC_PRI_OUT_DUP_LINK_IDX];
    gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX].inQueParams.prevLinkId = gHybridEncUsecaseContext.mergeId[PRE_DSPALG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX].inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.mergeId[PRE_DSPALG_MERGE_LINK_IDX],&gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX],sizeof(gHybridEncUsecaseContext.mergePrm[PRE_DSPALG_MERGE_LINK_IDX]));

    gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX].notifyNextLink = TRUE;
    gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX].numOutQue      = 2;
    gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX].outQueParams[0].nextLink = gVcapModuleContext.ipcFramesOutVpssId[0];
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkId = gHybridEncUsecaseContext.dupId[DEI_SC_PRI_OUT_DUP_LINK_IDX];
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX].outQueParams[1].nextLink = gHybridEncUsecaseContext.mergeId[PRE_SWMS_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX].inQueParams[PRE_SWMS_MERGE_LINK_DEI_SC_OUT_DUP_QIDX].prevLinkId = gHybridEncUsecaseContext.dupId[DEI_SC_PRI_OUT_DUP_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX].inQueParams[PRE_SWMS_MERGE_LINK_DEI_SC_OUT_DUP_QIDX].prevLinkQueId = 1;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.dupId[DEI_SC_PRI_OUT_DUP_LINK_IDX],&gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX],sizeof(gHybridEncUsecaseContext.dupPrm[DEI_SC_PRI_OUT_DUP_LINK_IDX]));

    /* ipcFramesOutVpssId[0] ---Q0---> ipcFramesInDspId[0] */
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.inputFrameRate = MULTICH_HYBRIDENC_GET_CAPTURE_FRAMES_PER_SEC();
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.outputFrameRate = 2;
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.numOutQue = 1;
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.ipcFramesInDspId[0];
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.notifyNextLink = TRUE;
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.notifyPrevLink = TRUE;
    gHybridEncUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
    gHybridEncUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    gHybridEncUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.notifyPrevLink = TRUE;
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.processLink = SYSTEM_LINK_ID_INVALID;
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.notifyProcessLink = FALSE;
    gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0].baseCreateParams.noNotifyMode = FALSE;
    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.ipcFramesOutVpssId[0],
                                  &gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0],
                                  sizeof(gHybridEncUsecaseContext.ipcFramesOutVpssPrm[0]));

    gHybridEncUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.numOutQue   = 1;
    gHybridEncUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[MULTICH_DSP_ALG_LINK_SCD_IDX];
    gHybridEncUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.notifyNextLink = TRUE;
    gHybridEncUsecaseContext.ipcFramesInDspPrm[0].baseCreateParams.noNotifyMode   = FALSE;
    gHybridEncUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
    gHybridEncUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX].inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.ipcFramesInDspId[0],
                                  &gHybridEncUsecaseContext.ipcFramesInDspPrm[0],
                                  sizeof(gHybridEncUsecaseContext.ipcFramesInDspPrm[0]));

    /* dspAlgId[1] ---Q0---> ipcBitsOutDsp */
    multich_hybridenc_set_scd_prm(&gHybridEncUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX],
                                  gVcapModuleContext.ipcFramesInDspId[0]);
    gHybridEncUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = gHybridEncUsecaseContext.ipcBitsOutDSPId;
    gHybridEncUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkId    = gVcapModuleContext.dspAlgId[MULTICH_DSP_ALG_LINK_SCD_IDX];
    gHybridEncUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.dspAlgId[MULTICH_DSP_ALG_LINK_SCD_IDX],
                                  &gHybridEncUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX],
                                  sizeof(gHybridEncUsecaseContext.dspAlgPrm[MULTICH_DSP_ALG_LINK_SCD_IDX]));

    /* ipcBitsOutDsp ---Q0---> ipcBitsInHlos */
    gHybridEncUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.numOutQue                 = 1;
    gHybridEncUsecaseContext.ipcBitsOutDspPrm.baseCreateParams.outQueParams[0].nextLink  = gVcapModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&gHybridEncUsecaseContext.ipcBitsOutDspPrm,
                                                TRUE);
    gHybridEncUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkId = gHybridEncUsecaseContext.ipcBitsOutDSPId;
    gHybridEncUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.ipcBitsOutDSPId,
                                  &gHybridEncUsecaseContext.ipcBitsOutDspPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcBitsOutDspPrm));

    gHybridEncUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.numOutQue                 = 1;
    gHybridEncUsecaseContext.ipcBitsInHostPrm[1].baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOSVcap(&gHybridEncUsecaseContext.ipcBitsInHostPrm[1]);
    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.ipcBitsInHLOSId,
                                  &gHybridEncUsecaseContext.ipcBitsInHostPrm[1],
                                  sizeof(gHybridEncUsecaseContext.ipcBitsInHostPrm[1]));

    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].notifyNextLink = TRUE;
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].numInQue       = DEI_OUT_MJPEG_MERGE_LINK_NUM_INQUE;
    gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX].outQueParams.nextLink = gVcapModuleContext.nsfId[0];
    gHybridEncUsecaseContext.nsfPrm.inQueParams.prevLinkId = gHybridEncUsecaseContext.mergeId[DEI_OUT_MJPEG_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.nsfPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.mergeId[DEI_OUT_MJPEG_MERGE_LINK_IDX],
                                  &gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX],
                                  sizeof(gHybridEncUsecaseContext.mergePrm[DEI_OUT_MJPEG_MERGE_LINK_IDX]));

    multich_hybridenc_set_nsf_prm(&gHybridEncUsecaseContext.nsfPrm);
    gHybridEncUsecaseContext.nsfPrm.outQueParams[0].nextLink = gHybridEncUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_NSF_QIDX].prevLinkId = gVcapModuleContext.nsfId[0];
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_NSF_QIDX].prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVcapModuleContext.nsfId[0],
                                  &gHybridEncUsecaseContext.nsfPrm,
                                  sizeof(gHybridEncUsecaseContext.nsfPrm));

    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].notifyNextLink = TRUE;
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].numInQue       = PRE_ENCODE_MERGE_LINK_NUM_INQUE;
    gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].outQueParams.nextLink = gHybridEncUsecaseContext.ipcOutVpssId;
    gHybridEncUsecaseContext.ipcOutVpssPrm.inQueParams.prevLinkId     = gHybridEncUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.ipcOutVpssPrm.inQueParams.prevLinkQueId  = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX],
                                  &gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX],
                                  sizeof(gHybridEncUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX]));

    /* ipcOutVpssIdisOutVpssId ---Q0---> ipcInVideoId */
    gHybridEncUsecaseContext.ipcOutVpssPrm.outQueParams[0].nextLink  = gHybridEncUsecaseContext.ipcInVideoId;
    gHybridEncUsecaseContext.ipcOutVpssPrm.notifyNextLink            = FALSE;
    gHybridEncUsecaseContext.ipcOutVpssPrm.notifyPrevLink            = TRUE;
    gHybridEncUsecaseContext.ipcOutVpssPrm.noNotifyMode              = TRUE;
    gHybridEncUsecaseContext.ipcInVideoPrm.inQueParams.prevLinkId    = gHybridEncUsecaseContext.ipcOutVpssId;
    gHybridEncUsecaseContext.ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.ipcOutVpssId,
                                  &gHybridEncUsecaseContext.ipcOutVpssPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcOutVpssPrm));


    /* ipcInVideoId ---Q0---> encId */
    gHybridEncUsecaseContext.ipcInVideoPrm.numOutQue                 = 1;
    gHybridEncUsecaseContext.ipcInVideoPrm.outQueParams[0].nextLink  = gVencModuleContext.encId;
    gHybridEncUsecaseContext.ipcInVideoPrm.notifyNextLink            = TRUE;
    gHybridEncUsecaseContext.ipcInVideoPrm.notifyPrevLink            = FALSE;
    gHybridEncUsecaseContext.ipcInVideoPrm.noNotifyMode              = TRUE;
    gHybridEncUsecaseContext.encPrm.inQueParams.prevLinkId    = gHybridEncUsecaseContext.ipcInVideoId;
    gHybridEncUsecaseContext.encPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.ipcInVideoId,
                                  &gHybridEncUsecaseContext.ipcInVideoPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcInVideoPrm));


    /* encId ---Q0---> ipcBitsOutRTOSId */
    multich_hybridenc_set_enclink_prm(&gHybridEncUsecaseContext.encPrm);
    gHybridEncUsecaseContext.encPrm.outQueParams.nextLink     = gVencModuleContext.ipcBitsOutRTOSId;
    gHybridEncUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.encId;
    gHybridEncUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVencModuleContext.encId,
                                  &gHybridEncUsecaseContext.encPrm,
                                  sizeof(gHybridEncUsecaseContext.encPrm));

    /* ipcBitsOutVideoId ---Q0---> ipcBitsInHostId */
    gHybridEncUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    gHybridEncUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&gHybridEncUsecaseContext.ipcBitsOutVideoPrm,
                                               TRUE);
    gHybridEncUsecaseContext.ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.ipcBitsOutRTOSId;
    gHybridEncUsecaseContext.ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVencModuleContext.ipcBitsOutRTOSId,
                                  &gHybridEncUsecaseContext.ipcBitsOutVideoPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcBitsOutVideoPrm));
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&gHybridEncUsecaseContext.ipcBitsInHostPrm[0]);
    MULTICH_HYBRIDENC_CREATE_LINK(gVencModuleContext.ipcBitsInHLOSId,
                                  &gHybridEncUsecaseContext.ipcBitsInHostPrm[0],
                                  sizeof(gHybridEncUsecaseContext.ipcBitsInHostPrm[0]));

    /* ipcBitsOutHostId ---Q0---> ipcBitsInRtosId */
    multich_hybridenc_set_ipcbitsout_hlos_prms(&gHybridEncUsecaseContext.ipcBitsOutHostPrm);
    gHybridEncUsecaseContext.ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.ipcBitsInRTOSId;
    gHybridEncUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsOutHLOSId;
    gHybridEncUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId    = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVdecModuleContext.ipcBitsOutHLOSId,
                                  &gHybridEncUsecaseContext.ipcBitsOutHostPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcBitsOutHostPrm));

    /* ipcBitsInRtosId ---Q0---> decId */
    gHybridEncUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.numOutQue                    = 1;
    gHybridEncUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink     = gVdecModuleContext.decId;
    MultiCh_ipcBitsInitCreateParams_BitsInRTOS(&gHybridEncUsecaseContext.ipcBitsInVideoPrm, TRUE);
    gHybridEncUsecaseContext.decPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsInRTOSId;
    gHybridEncUsecaseContext.decPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVdecModuleContext.ipcBitsInRTOSId,
                                  &gHybridEncUsecaseContext.ipcBitsInVideoPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcBitsInVideoPrm));

    /* decId---Q0--->ipcOutVideoId*/
    multich_hybridenc_set_declink_prms(&gHybridEncUsecaseContext.decPrm);
    gHybridEncUsecaseContext.decPrm.outQueParams.nextLink  = gHybridEncUsecaseContext.ipcOutVideoId;
    gHybridEncUsecaseContext.ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    gHybridEncUsecaseContext.ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gVdecModuleContext.decId,
                                  &gHybridEncUsecaseContext.decPrm,
                                  sizeof(gHybridEncUsecaseContext.decPrm));

    /*ipcOutVideoId---Q0-->ipcInVpssId*/
    gHybridEncUsecaseContext.ipcOutVideoPrm.numOutQue                 = 1;
    gHybridEncUsecaseContext.ipcOutVideoPrm.outQueParams[0].nextLink  = gHybridEncUsecaseContext.ipcInVpssId;
    gHybridEncUsecaseContext.ipcOutVideoPrm.notifyNextLink            = FALSE;
    gHybridEncUsecaseContext.ipcOutVideoPrm.notifyPrevLink            = TRUE;
    gHybridEncUsecaseContext.ipcOutVideoPrm.noNotifyMode              = TRUE;
    gHybridEncUsecaseContext.ipcInVpssPrm.inQueParams.prevLinkId    = gHybridEncUsecaseContext.ipcOutVideoId;
    gHybridEncUsecaseContext.ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.ipcOutVideoId,
                                  &gHybridEncUsecaseContext.ipcOutVideoPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcOutVideoPrm));

    /*ipcInVpssId---Q0--> mergeId[LIVE_DECODE_MERGE_LINK_IDX] */
    gHybridEncUsecaseContext.ipcInVpssPrm.numOutQue                 = 1;
    gHybridEncUsecaseContext.ipcInVpssPrm.outQueParams[0].nextLink  = gHybridEncUsecaseContext.mergeId[PRE_SWMS_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX].inQueParams[PRE_SWMS_MERGE_LINK_DEC_OUT_QIDX].prevLinkId = gHybridEncUsecaseContext.ipcInVpssId;
    gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX].inQueParams[PRE_SWMS_MERGE_LINK_DEC_OUT_QIDX].prevLinkQueId = 0;
    gHybridEncUsecaseContext.ipcInVpssPrm.notifyNextLink            = TRUE;
    gHybridEncUsecaseContext.ipcInVpssPrm.notifyPrevLink            = FALSE;
    gHybridEncUsecaseContext.ipcInVpssPrm.noNotifyMode              = TRUE;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.ipcInVpssId,
                                  &gHybridEncUsecaseContext.ipcInVpssPrm,
                                  sizeof(gHybridEncUsecaseContext.ipcInVpssPrm));

    gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX].numInQue = PRE_SWMS_MERGE_LINK_NUM_INQUE;
    gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX].notifyNextLink = TRUE;
    gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX].outQueParams.nextLink = gVdisModuleContext.swMsId[0];
    gHybridEncUsecaseContext.swMsPrm[0].inQueParams.prevLinkId = gHybridEncUsecaseContext.mergeId[PRE_SWMS_MERGE_LINK_IDX];
    gHybridEncUsecaseContext.swMsPrm[0].inQueParams.prevLinkQueId = 0;
    MULTICH_HYBRIDENC_CREATE_LINK(gHybridEncUsecaseContext.mergeId[PRE_SWMS_MERGE_LINK_IDX],
                                  &gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX],
                                  sizeof(gHybridEncUsecaseContext.mergePrm[PRE_SWMS_MERGE_LINK_IDX]));
    /* Avsync configuration for SwMs[0] */
    mulich_hybridenc_set_avsync_prm(&gHybridEncUsecaseContext.avsyncCfg[0],
                                    gHybridEncUsecaseContext.mergeId[PRE_SWMS_MERGE_LINK_IDX],
                                    0);
    /*swMsId[0]---Q0--> displayId[VDIS_DEV_HDMI] */
    gHybridEncUsecaseContext.swMsPrm[0].outQueParams.nextLink     = Vdis_getDisplayId(VDIS_DEV_HDMI);
    gHybridEncUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)].numInputQueues = 1;
    gHybridEncUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[0];
    gHybridEncUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)].inQueParams[0].prevLinkQueId = 0;
    multich_hybridenc_set_swms_prm(&gHybridEncUsecaseContext.swMsPrm[0]);
    MULTICH_HYBRIDENC_CREATE_LINK(gVdisModuleContext.swMsId[0],
                                 &gHybridEncUsecaseContext.swMsPrm[0],
                                 sizeof(gHybridEncUsecaseContext.swMsPrm[0]));

    multich_hybridenc_set_display_prms(&gHybridEncUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)],
                                        gHybridEncUsecaseContext.swMsPrm[0].initOutRes);
    MULTICH_HYBRIDENC_CREATE_LINK(Vdis_getDisplayId(VDIS_DEV_HDMI),
                                  &gHybridEncUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)],
                                  sizeof(gHybridEncUsecaseContext.displayPrm[Vdis_getDisplayContextIndex(VDIS_DEV_HDMI)]));
}

static
void multich_hybridenc_set_dec2disp_chmap()
{
    MergeLink_InLinkChInfo inChInfo;

    MergeLink_InLinkChInfo_Init(&inChInfo);
    inChInfo.inLinkID = gHybridEncUsecaseContext.ipcInVpssId;
    System_linkControl(gHybridEncUsecaseContext.mergeId[PRE_SWMS_MERGE_LINK_IDX],
                       MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO,
                       &inChInfo,
                       sizeof(inChInfo),
                       TRUE);
    OSA_assert(inChInfo.numCh == gVdecModuleContext.vdecConfig.numChn);

    MultiCh_setDec2DispMap(VDIS_DEV_HDMI,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
}

Void MultiCh_createHybridEnc()
{
    memset(&gHybridEncUsecaseContext,0,sizeof(gHybridEncUsecaseContext));
    multich_hybridenc_reset_link_prms();
    multich_hybridenc_set_link_ids();
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
    multich_hybridenc_connect_links(gVsysModuleContext.vsysConfig.enableScd);
    multich_hybridenc_set_links_framerate();
    multich_hybridenc_set_dec2disp_chmap();
}

Void MultiCh_deleteHybridEnc()
{
    UInt32 i;

    for (i = 0; i < gHybridEncUsecaseContext.createdLinkCount; i++)
    {
        System_linkDelete (gHybridEncUsecaseContext.createdLinks[i]);
    }
    gHybridEncUsecaseContext.createdLinkCount = 0;
    multich_hybridenc_reset_link_ids();

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

    if ((FALSE == TILER_ENABLE_ENCODE) && (FALSE == TILER_ENABLE_DECODE))
    {
        SystemTiler_enableAllocator();
    }
}



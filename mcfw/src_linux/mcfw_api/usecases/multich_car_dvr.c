/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file  multich_car_dvr.c
    \brief Usecase file for Car DVR usecase
*/

/* =============================================================================
 * Use case code
 * =============================================================================
 */

/** --------------------------- Car DVR usecase -----------------------------------
   +--------+              +--------+
   |        |              |        |
   |        |              |        |
   | BitsOut|              |Capture |
   |  HLOS  |              |        |
   |        |              |        |
   +---+----+              + ------ +
       |                    +      +Ch (1 - 4)
       |              4 chD1|      |
       |                    v      v
       |                +-------+ +-------+
       |                |SELECT | | Dei   |
       |                |       | |YUV420 |
       |                +-------+ +-------+
       |                    +      +
       |              Ch0   |      |
       |                    |      |
       |                +-------+ ++--------+
       v Ch0            |       | |         |
   +--------+           |DEI    | |Composite|
   |        |           |YUV420 | |  SwMs   |
   | Decode |           +-------+ +---------+
   |        |           Ch0 |     +Ch0 YUV422
   +--------+               |     |
        +                   |     |
        |                   |     v
        |                   |    +---------+
        |                   |    |         |
        v                   |    |   NSF   |
   +---------+              |    +---------+
   |         |            Q1|     + ch0 YUV420
   | SWMS    |              |     | Q0
   | Playback|              v     v
   +---------+              +--------+
        |                   | Merge  |
        |                   +--------+
        |                       |
        v                       |                                        Ch0,1
   +---------+                  |                                +-------+      +------+     +--------+
   |         |                  |                          Ch0,1 |       | Q0   |      |     |        |
   |   NSF   |                  |                                |       |+---> |      |     |        |
   +---------+                  |Ch 0,1<----<<<PROCESS_LINK>>>-->|Dup    |      |Merge |+--->| Encode |+------>
        |                       v Q1          IPC_FRAMES_OUT     |       |      |      |     |        |       Ch0
        |        Q0       +--------+                             |       |+---> |      |Ch0  |        |         1
       +----------------->| Merge  |                             |       | Q1   |      |  1  +--------+         3
                   Ch0    +--------+                             +-------+      +------+  2    Disable Ch2      4
                               +                                          Ch0,1    ^      3
                               |Ch0,1,2                                            |Q2    4
                           +--------+                                              |
                           | SELECT |                                              |
                           +--------+                                              |
                               v Ch(0/1/2)                                         | IPC_IN_M3
                          +--------+                                               |
                          |        |                                               |
                          |        |                                               |
                          |  SWOSD |                                               |
                          |        |                                               |
                          +--------+                                               |
                               +                                                   |
                               |                                                   |
                               |Ch(0/1/2)                                          |
                               v                                                   |
                         +--------+                                                |
                         |        |        Ch(0/1/2)                               |
                         |  Dup   |+-----------------------------------------------+
                         |        |     IPC_OUT_M3
                         +--------+
                              +
                              |
                              |Ch(0/1/2)
                              v
                         +--------+
                         |  SD    |
                         | Display|(Configure Display to select 0/1/2)
                         |        |
                         +--------+
*/

#include "mcfw/src_linux/mcfw_api/usecases/multich_common.h"
#include "mcfw/src_linux/mcfw_api/usecases/multich_ipcbits.h"

#include "mcfw/src_linux/mcfw_api/ti_vdis_priv.h"

#define TILER_ENABLE    FALSE


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
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 , 14, 15},
        .DecNumCh  = 16,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 , 14, 15},
    },

};

#define     MULTICH_CAR_DVR_USECASE_MAX_NUM_LINKS       (64)
#define     MULTICH_CAR_DVR_USECASE_NUM_DISPLAY_CH      (3)

#define     NUM_CAPTURE_DEVICES                         (2)

/** Merge Link Info */
#define     NUM_MERGE_LINK                              (4)
/**  DEI CIFCOMPOSITE Merge
 *   NSF0 ---DEI_DEI_SC_Q------->Q0--|
 *                                   |-DEI_SC_CIF_MERGE_LINK_IDX
 *   DEI0 ---DEI_VIP_SC_SEC_Q--->Q1--|
 */
#define     D1_CIFCOMPOSITE_MERGE_LINK_IDX              (0)

#define     D1_CIFCOMPOSITE_MERGE_LINK_NUM_INQUE        (2)
#define     D1_CIFCOMPOSITE_MERGE_LINK_NSF_QIDX         (0)
#define     D1_CIFCOMPOSITE_MERGE_LINK_DEI0_QIDX        (1)


/**  LIVE_PLAYBACK_MERGE
 *   IPCFRAMESOUT_ENCODE_FRAMESOUT_LINK_IDX ---(Q0)------->Q0--|
 *                                                             |-DEI_SC_CIF_MERGE_LINK_IDX
 *   IPC_IN_M3 --------------------------------(Q1)------->Q1--|
 */
#define     LIVE_PLAYBACK_MERGE_LINK_IDX                (1)
#define     LIVE_PLAYBACK_MERGE_LINK_NUM_INQUE          (2)
#define     LIVE_PLAYBACK_MERGE_LINK_LIVECH_QIDX        (1)
#define     LIVE_PLAYBACK_MERGE_LINK_PLAYBACKCH_QIDX    (0)


/**  PRE_ENCODE_MERGE
 *   IPC_FRAMES_IN -------------------------------(Q0)------->Q0--|
 *                                                                |
                                                                  |-PRE_ENCODE_MERGE_LINK_IDX
 *                                                                |
 *   IPC_IN_M3 -----------------------------------(Q0)------->Q1--|
 */
#define     PRE_ENCODE_MERGE_LINK_IDX                   (2)
#define     PRE_ENCODE_MERGE_LINK_NUM_INQUE             (2)
#define     PRE_ENCODE_MERGE_LINK_IPCFRAMESIN_QIDX      (0)
#define     PRE_ENCODE_MERGE_LINK_IPCINM3_QIDX          (1)


/**  FG_BG_ENCODE_DUP_MERGE
 *   FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX ---(Q0)------->Q0--|
 *                                                                |
 *   FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX----(Q1)------->Q1--|-PRE_ENCODE_MERGE_LINK_IDX
 */
#define     FG_BG_ENCODE_DUP_MERGE_LINK_IDX             (3)
#define     FG_BG_ENCODE_DUP_MERGE_LINK_NUM_INQUE       (2)
#define     FG_BG_ENCODE_DUP_MERGE_LINK_FGBGDUPQ0_QIDX (0)
#define     FG_BG_ENCODE_DUP_MERGE_LINK_FGBGDUPQ1_QIDX (1)


#define     NUM_DUP_LINK                                (2)
#define     FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX   (0)
#define     DISPLAY_ENCODE_DUP_LINK_IDX                 (1)

#define     NUM_SELECT_LINK                             (2)
#define     CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX        (0)
#define     DISPLAY_SELECT_LINK_IDX                     (1)

#define     NUM_DEI_LINK                                (2)
#define     D1_DEI_LINK_IDX                             (0)
#define     CIF_DEI_LINK_IDX                            (1)

#define     NUM_VPSS_FRAMES_OUT_LINK                    (2)
#define     ENCODE_FRAMESOUT_LINK_IDX                   (0)
#define     OSD_FRAMESOUT_LINK_IDX                      (1)


#define     NUM_CAPTURE_BUFFERS                         (8)
#define     NUM_NSF_BUFFERS                             (6)
#define     NUM_ENCODE_D1_BUFFERS                       (6)
#define     NUM_ENCODE_CIF_BUFFERS                      (6)
#define     NUM_DECODE_BUFFERS                          (6)
#define     NUM_IPC_OUT_BUFFERS                         (6)
#define     NUM_SWMS_MAX_BUFFERS                        (8)

#define     BACKGROUND_CHANNEL_ENCODE_FPS               (5)

typedef struct MultichCarDVR_Context
{
    UInt32 ipcFramesInVideoLiveEncode;
    UInt32 createdLinks[MULTICH_CAR_DVR_USECASE_MAX_NUM_LINKS];
    UInt32 createdLinkCount;
    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];
    AvsyncLink_LinkSynchConfigParams avsyncPrms[2];
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm[2];
    DeiLink_CreateParams        deiPrm[NUM_DEI_LINK];
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
    IpcBitsInLinkHLOS_CreateParams    ipcBitsInHostPrm;
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm;
    IpcFramesOutLinkRTOS_CreateParams ipcFramesOutVpssPrm[NUM_VPSS_FRAMES_OUT_LINK];
    IpcFramesInLinkRTOS_CreateParams  ipcFramesInVideoLiveEncodePrm;
    AlgLink_CreateParams              dspAlgPrm;
    System_LinkInfo                   bitsProducerLinkInfo;
    SelectLink_CreateParams           selectPrm[NUM_SELECT_LINK];
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 ipcOutVpssId;
    UInt32 ipcInVpssId;
    UInt32 ipcOutVideoId;
    UInt32 ipcInVideoId;
    UInt32  captureFps;
    UInt32  selectLinkId[NUM_SELECT_LINK];
}  MultichCarDVR_Context;

MultichCarDVR_Context gCarDVRUsecaseContext =
{
    .ipcFramesInVideoLiveEncode =  SYSTEM_LINK_ID_INVALID,
    .createdLinkCount           = 0
};

static Void multich_cardvr_register_created_link(MultichCarDVR_Context *pContext,
                                                 UInt32 linkID)
{
    OSA_assert(pContext->createdLinkCount < OSA_ARRAYSIZE(pContext->createdLinks));
    pContext->createdLinks[pContext->createdLinkCount] = linkID;
    pContext->createdLinkCount++;
}

#define MULTICH_CARDVR_CREATE_LINK(linkID,createPrm,createPrmSize)              \
    do                                                                          \
    {                                                                           \
        System_linkCreate(linkID,createPrm,createPrmSize);                      \
        multich_cardvr_register_created_link(&gCarDVRUsecaseContext,            \
                                             linkID);                           \
    } while (0)


static
Void mulich_cardvr_set_avsync_vidque_prm(Avsync_SynchConfigParams *queCfg,
                                            Int chnum,
                                            UInt32 avsStartChNum,
                                            UInt32 avsEndChNum,
                                            Bool avsyncEnable)
{
    queCfg->chNum = chnum;
    queCfg->audioPresent = FALSE;
    if ((queCfg->chNum >= avsStartChNum)
        &&
        (queCfg->chNum <= avsEndChNum)
        &&
        (gVsysModuleContext.vsysConfig.enableAVsync)
        &&
        (avsyncEnable))
    {
        queCfg->avsyncEnable = TRUE;
    }
    else
    {
        queCfg->avsyncEnable = FALSE;
    }
    queCfg->clkAdjustPolicy.refClkType = AVSYNC_REFCLKADJUST_BYVIDEO;
    queCfg->playTimerStartTimeout = 0;
    queCfg->playStartMode = AVSYNC_PLAYBACK_START_MODE_WAITSYNCH;
    queCfg->clkAdjustPolicy.clkAdjustLead = AVSYNC_VIDEO_REFCLKADJUST_MAX_LEAD_MS;
    queCfg->clkAdjustPolicy.clkAdjustLag = AVSYNC_VIDEO_REFCLKADJUST_MAX_LAG_MS;
    queCfg->ptsInitMode   = AVSYNC_PTS_INIT_MODE_AUTO;
}

static
Void mulich_cardvr_set_avsync_prm(AvsyncLink_LinkSynchConfigParams *avsyncPrm,
                                  UInt32 prevLinkID,
                                  UInt32 prevLinkQueId,
                                  UInt32 swMsId)
{
    System_LinkInfo                   swmsInLinkInfo;
    Int i;
    Int32 status;
    Bool avsyncEnable;

    if (0 == swMsId)
        avsyncEnable  = FALSE;
    else
        avsyncEnable  = TRUE;

    Vdis_getAvsyncConfig(VDIS_DEV_SD,avsyncPrm);
    if (0 == swMsId)
        avsyncPrm->displayLinkID        = SYSTEM_LINK_ID_INVALID;
    else
        avsyncPrm->displayLinkID        = Vdis_getDisplayId(VDIS_DEV_SD);
    avsyncPrm->videoSynchLinkID = gVdisModuleContext.swMsId[swMsId];
    System_linkGetInfo(prevLinkID,&swmsInLinkInfo);
    OSA_assert(swmsInLinkInfo.numQue > prevLinkQueId);

    avsyncPrm->numCh            = swmsInLinkInfo.queInfo[prevLinkQueId].numCh;
    avsyncPrm->syncMasterChnum = 0;
    for (i = 0; i < avsyncPrm->numCh;i++)
    {
        mulich_cardvr_set_avsync_vidque_prm(&avsyncPrm->queCfg[i],
                                             i,
                                             0,
                                            (0 + (avsyncPrm->numCh - 1)),
                                            avsyncEnable);
    }
    Vdis_setAvsyncConfig(VDIS_DEV_SD,avsyncPrm);

    status = Avsync_configSyncConfigInfo(avsyncPrm);
    OSA_assert(status == 0);

}

static
Void multich_cardvr_configure_extvideodecoder_prm(UInt32 numCaptureDevices)
{
    int i;

    for(i = 0; i < numCaptureDevices; i++)
    {
        gCarDVRUsecaseContext.vidDecVideoModeArgs[i].videoIfMode        = DEVICE_CAPT_VIDEO_IF_MODE_8BIT;
        gCarDVRUsecaseContext.vidDecVideoModeArgs[i].videoDataFormat    = SYSTEM_DF_YUV422P;
        if (i == 0)
        {
            gCarDVRUsecaseContext.vidDecVideoModeArgs[i].standard           = SYSTEM_STD_MUX_4CH_D1;
        }
        else
        {
            gCarDVRUsecaseContext.vidDecVideoModeArgs[i].standard           = SYSTEM_STD_MUX_4CH_D1;
        }
        gCarDVRUsecaseContext.vidDecVideoModeArgs[i].videoCaptureMode   =
                    DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_PIXEL_MUX_EMBEDDED_SYNC;
        gCarDVRUsecaseContext.vidDecVideoModeArgs[i].videoSystem        =
                                      DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT;
        gCarDVRUsecaseContext.vidDecVideoModeArgs[i].videoCropEnable    = FALSE;
        gCarDVRUsecaseContext.vidDecVideoModeArgs[i].videoAutoDetectTimeout = -1;
    }
    Vcap_configVideoDecoder(&gCarDVRUsecaseContext.vidDecVideoModeArgs[0],
                            NUM_CAPTURE_DEVICES);
}

static
Void multich_cardvr_set_link_ids()
{
    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.deiId[D1_DEI_LINK_IDX]     = SYSTEM_LINK_ID_DEI_0;
    gVcapModuleContext.deiId[CIF_DEI_LINK_IDX]     = SYSTEM_LINK_ID_DEI_1;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.nsfId[1]     = SYSTEM_LINK_ID_NSF_1;

    gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_ALG_0  ;
    gVcapModuleContext.ipcFramesOutVpssId[ENCODE_FRAMESOUT_LINK_IDX] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
    gVcapModuleContext.ipcFramesOutVpssId[OSD_FRAMESOUT_LINK_IDX] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncode = SYSTEM_VIDEO_LINK_ID_IPC_FRAMES_IN_0;
    gVcapModuleContext.ipcFramesInDspId[0]   = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;


    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_SD)] = SYSTEM_LINK_ID_DISPLAY_2; /* SDTV */

    gCarDVRUsecaseContext.mergeId[D1_CIFCOMPOSITE_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_0;
    gCarDVRUsecaseContext.mergeId[LIVE_PLAYBACK_MERGE_LINK_IDX]   = SYSTEM_VPSS_LINK_ID_MERGE_1;
    gCarDVRUsecaseContext.mergeId[FG_BG_ENCODE_DUP_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_2;
    gCarDVRUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX]   = SYSTEM_VIDEO_LINK_ID_MERGE_0;

    gCarDVRUsecaseContext.dupId[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX] = SYSTEM_VPSS_LINK_ID_DUP_0;
    gCarDVRUsecaseContext.dupId[DISPLAY_ENCODE_DUP_LINK_IDX]               = SYSTEM_VPSS_LINK_ID_DUP_1;

    gCarDVRUsecaseContext.ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    gCarDVRUsecaseContext.ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    gCarDVRUsecaseContext.ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    gCarDVRUsecaseContext.ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

    gCarDVRUsecaseContext.selectLinkId[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX] = SYSTEM_VPSS_LINK_ID_SELECT_0;
    gCarDVRUsecaseContext.selectLinkId[DISPLAY_SELECT_LINK_IDX] = SYSTEM_VPSS_LINK_ID_SELECT_1;

}

static
Void multich_cardvr_reset_link_ids()
{
    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.deiId[D1_DEI_LINK_IDX]     = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.deiId[CIF_DEI_LINK_IDX]     = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.nsfId[1]     = SYSTEM_LINK_ID_INVALID;

    gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_INVALID  ;
    gVcapModuleContext.ipcFramesOutVpssId[ENCODE_FRAMESOUT_LINK_IDX] = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcFramesOutVpssId[OSD_FRAMESOUT_LINK_IDX] = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncode = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcFramesInDspId[0]   = SYSTEM_LINK_ID_INVALID;


    gVencModuleContext.encId        = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_INVALID;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_INVALID;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_INVALID;
    gVdisModuleContext.displayId[Vdis_getDisplayContextIndex(VDIS_DEV_SD)] = SYSTEM_LINK_ID_INVALID; /* SDTV */

    gCarDVRUsecaseContext.mergeId[D1_CIFCOMPOSITE_MERGE_LINK_IDX] = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.mergeId[LIVE_PLAYBACK_MERGE_LINK_IDX]   = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX]   = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.mergeId[FG_BG_ENCODE_DUP_MERGE_LINK_IDX] = SYSTEM_LINK_ID_INVALID;

    gCarDVRUsecaseContext.dupId[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX] = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.dupId[DISPLAY_ENCODE_DUP_LINK_IDX]               = SYSTEM_LINK_ID_INVALID;

    gCarDVRUsecaseContext.ipcOutVpssId = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.ipcInVideoId = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.ipcOutVideoId= SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.ipcInVpssId  = SYSTEM_LINK_ID_INVALID;

    gVencModuleContext.ipcBitsOutRTOSId  = SYSTEM_LINK_ID_INVALID;
    gVencModuleContext.ipcBitsInHLOSId   = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsOutHLOSId  = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_LINK_ID_INVALID;

    gCarDVRUsecaseContext.selectLinkId[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX] = SYSTEM_LINK_ID_INVALID;
    gCarDVRUsecaseContext.selectLinkId[DISPLAY_SELECT_LINK_IDX] = SYSTEM_LINK_ID_INVALID;

}

static
Void multich_cardvr_set_capture_fps(UInt32 *captureFps)
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

#define MULTICH_CARDVR_GET_CAPTURE_FIELDS_PER_SEC()           (gCarDVRUsecaseContext.captureFps)
#define MULTICH_CARDVR_GET_CAPTURE_FRAMES_PER_SEC()           (gCarDVRUsecaseContext.captureFps/2)


static
UInt32 multich_cardvr_enclink_mapch2fps(UInt32 chNum)
{
    UInt32 fps = 0;

    switch (chNum)
    {
        case 0:
        case 1:
        case 4:
            fps = MULTICH_CARDVR_GET_CAPTURE_FRAMES_PER_SEC();
            break;
        case 2:
            fps = 0;
            break;
        case 3:
            fps = BACKGROUND_CHANNEL_ENCODE_FPS;
            break;
        default:
            fps = 0;
            break;
    }
    return (fps * 1000);
}

static
Void multich_cardvr_set_links_framerate()
{
    Int32 status;
    DeiLink_ChFpsParams params;
    UInt32 chId;

    for (chId = 0; chId < gVcapModuleContext.vcapConfig.numChn;chId++)
    {
        /* Capture -> Dei */
        params.chId = chId;
        params.inputFrameRate = MULTICH_CARDVR_GET_CAPTURE_FIELDS_PER_SEC();

        /* Stream 0 -DEI_SC_OUT is inputfps/2 */
        params.streamId = 0;
        params.outputFrameRate = MULTICH_CARDVR_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = 1;
        params.outputFrameRate = MULTICH_CARDVR_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);

        /* Stream 1 -VIP_SC_OUT_PRIMARY is inputfps/2 */
        params.streamId = 2;
        params.outputFrameRate = MULTICH_CARDVR_GET_CAPTURE_FRAMES_PER_SEC();
        status = System_linkControl(gVcapModuleContext.deiId[0], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
        status = System_linkControl(gVcapModuleContext.deiId[1], DEI_LINK_CMD_SET_FRAME_RATE,
                                    &params, sizeof(params), TRUE);
    }
    for (chId=0; chId < gVencModuleContext.vencConfig.numPrimaryChn +
                        gVencModuleContext.vencConfig.numSecondaryChn; chId++)
    {
        UInt32 fps;

        fps = multich_cardvr_enclink_mapch2fps(chId);
        if (0 == fps)
        {
            EncLink_ChannelInfo chDisablePrm;
            chDisablePrm.chId = chId;
            status = System_linkControl(gVencModuleContext.encId, ENC_LINK_CMD_DISABLE_CHANNEL,
                                    &chDisablePrm, sizeof(chDisablePrm), TRUE);
        }
        else
        {
            EncLink_ChFpsParams fpsPrm;

            fpsPrm.chId = chId;
            fpsPrm.targetFps = fps;
            fpsPrm.targetBitRate = 0;
            status = System_linkControl(gVencModuleContext.encId, ENC_LINK_CMD_SET_CODEC_FPS,
                                        &fpsPrm, sizeof(fpsPrm), TRUE);
        }
    }
}

static
Void multich_cardvr_reset_link_prms()
{
    int i;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gCarDVRUsecaseContext.ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gCarDVRUsecaseContext.ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gCarDVRUsecaseContext.ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,gCarDVRUsecaseContext.ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,gCarDVRUsecaseContext.ipcBitsOutHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,gCarDVRUsecaseContext.ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,gCarDVRUsecaseContext.ipcBitsInHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,gCarDVRUsecaseContext.ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(DecLink_CreateParams, gCarDVRUsecaseContext.decPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,gCarDVRUsecaseContext.ipcFramesInDspPrm);
    for (i = 0; i < NUM_VPSS_FRAMES_OUT_LINK; i++)
    {
        MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,
                            gCarDVRUsecaseContext.ipcFramesOutVpssPrm[i]);
    }
    MULTICH_INIT_STRUCT(EncLink_CreateParams, gCarDVRUsecaseContext.encPrm);
    MULTICH_INIT_STRUCT(AlgLink_CreateParams, gCarDVRUsecaseContext.dspAlgPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams , gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm);
    for (i = 0; i < VDIS_DEV_MAX;i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams,
                            gCarDVRUsecaseContext.displayPrm[i]);
        MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,gCarDVRUsecaseContext.swMsPrm[i]);
    }

    MULTICH_INIT_STRUCT(NsfLink_CreateParams, gCarDVRUsecaseContext.nsfPrm[0]);
    MULTICH_INIT_STRUCT(NsfLink_CreateParams, gCarDVRUsecaseContext.nsfPrm[1]);
    for (i = 0; i < NUM_DEI_LINK; i++)
    {
        MULTICH_INIT_STRUCT(DeiLink_CreateParams, gCarDVRUsecaseContext.deiPrm[i]);
    }
    MULTICH_INIT_STRUCT(AvsyncLink_LinkSynchConfigParams,gCarDVRUsecaseContext.avsyncPrms[0]);
    MULTICH_INIT_STRUCT(AvsyncLink_LinkSynchConfigParams,gCarDVRUsecaseContext.avsyncPrms[1]);
    CaptureLink_CreateParams_Init(&gCarDVRUsecaseContext.capturePrm);
    EncLink_CreateParams_Init(&gCarDVRUsecaseContext.encPrm);
    for (i = 0; i < NUM_SELECT_LINK;i++)
    {
        MULTICH_INIT_STRUCT(SelectLink_CreateParams, gCarDVRUsecaseContext.selectPrm[i]);
    }
}

static
Void multich_cardvr_set_capture_prm(CaptureLink_CreateParams *capturePrm)
{
    UInt32 vipInstId;
    CaptureLink_VipInstParams         *pCaptureInstPrm;
    CaptureLink_OutParams             *pCaptureOutPrm;

    vipInstId = 0;

    /* This is for TVP5158 Audio Channels - Change it to 16 if there are 16 audio channels connected in cascade */
    capturePrm->numVipInst                 = 2;
    capturePrm->tilerEnable                = FALSE;
    capturePrm->numBufsPerCh               = NUM_CAPTURE_BUFFERS;
    capturePrm->enableSdCrop               = FALSE;
#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm->isPalMode                  = Vcap_isPalMode();
#endif

    pCaptureInstPrm                     = &capturePrm->vipInst[0];
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

    vipInstId = 1;
    pCaptureInstPrm                     = &capturePrm->vipInst[1];
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
    pCaptureOutPrm->outQueId            = 1;
}

static
Void multich_cardvr_set_dei_prm(DeiLink_CreateParams *deiPrm,UInt32 outQueIdx)
{
    int i;

    /* Set Output Scaling at DEI based on ratio */
    deiPrm->outScaleFactor[outQueIdx][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[outQueIdx][0].ratio.widthRatio.numerator = 1;
    deiPrm->outScaleFactor[outQueIdx][0].ratio.widthRatio.denominator = 1;
    deiPrm->outScaleFactor[outQueIdx][0].ratio.heightRatio.numerator = 1;
    deiPrm->outScaleFactor[outQueIdx][0].ratio.heightRatio.denominator = 1;
    for (i = 1; i < DEI_LINK_MAX_CH; i++)
    {
        deiPrm->outScaleFactor[outQueIdx][i] =
                     deiPrm->outScaleFactor[outQueIdx][0];
    }
    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = FALSE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = FALSE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]          = FALSE;
    deiPrm->enableOut[outQueIdx]                                      = TRUE;
    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = TILER_ENABLE;
    deiPrm->comprEnable                                   = FALSE;
    deiPrm->setVipScYuv422Format                          = FALSE;
}


static
Void multich_cardvr_set_swms_singlewin_layoutprm(UInt32 devId, SwMsLink_CreateParams *swMsCreateArgs)
{
    SwMsLink_LayoutPrm *layoutInfo;
    SwMsLink_LayoutWinInfo *winInfo;
    UInt32 outWidth, outHeight, winId, widthAlign, heightAlign;

    MultiCh_swMsGetOutSize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);
    widthAlign = 8;
    heightAlign = 1;

    if(devId>=VDIS_DEV_MAX)
        devId = VDIS_DEV_HDMI;

    layoutInfo = &swMsCreateArgs->layoutPrm;

    /* init to known default */
    memset(layoutInfo, 0, sizeof(*layoutInfo));

    layoutInfo->onlyCh2WinMapChanged = FALSE;
    layoutInfo->outputFPS = 30;
    layoutInfo->numWin = 1;
    winId = 0;

    winInfo = &layoutInfo->winInfo[winId];

    winInfo->width  = SystemUtils_floor(outWidth, widthAlign);
    winInfo->height = SystemUtils_floor(outHeight, heightAlign);
    winInfo->startX = 0;
    winInfo->startY = 0;
    winInfo->bypass = FALSE;
    winInfo->channelNum = 0 + winId;
}

static
Void multich_cardvr_set_swms_prm(SwMsLink_CreateParams *swMsPrm,UInt32 swMsId)
{
    swMsPrm->numSwMsInst = 1;
    /* use AUX scaler (SC2), since SC1 is used for DEI */
    swMsPrm->swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm->maxInputQueLen            = 4;
    swMsPrm->maxOutRes                 = VSYS_STD_PAL;
    swMsPrm->initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
    swMsPrm->numOutBuf                 = NUM_SWMS_MAX_BUFFERS;
    swMsPrm->lineSkipMode = FALSE; // Set to TRUE for Enable low cost scaling
    swMsPrm->enableLayoutGridDraw = FALSE;

    if (0 == swMsId)
    {
        MultiCh_swMsGetDefaultLayoutPrm(VDIS_DEV_SD, swMsPrm, FALSE);
    }
    else
    {
        multich_cardvr_set_swms_singlewin_layoutprm(VDIS_DEV_SD,swMsPrm);
    }
    swMsPrm->layoutPrm.outputFPS = MULTICH_CARDVR_GET_CAPTURE_FRAMES_PER_SEC();
    /* Compensate for 10% timerPeriod compensation in SwMs */
    //swMsPrm->layoutPrm.outputFPS -= MULTICH_CARDVR_GET_CAPTURE_FRAMES_PER_SEC() / 10;
}


static
Void multich_cardvr_set_nsf_prm(NsfLink_CreateParams *nsfPrm)
{
    nsfPrm->bypassNsf                        = TRUE;
    nsfPrm->tilerEnable                      = TILER_ENABLE;
    nsfPrm->numOutQue                        = 1;
    nsfPrm->numBufsPerCh                     = NUM_NSF_BUFFERS;
    nsfPrm->inputFrameRate                   = 30;
    nsfPrm->outputFrameRate                  = 30;
}


static
Void multich_cardvr_set_declink_prms(DecLink_CreateParams *decPrm)
{
    int i;

    for (i=0; i<gVdecModuleContext.vdecConfig.numChn; i++)
    {
        decPrm->chCreateParams[i].format                 = IVIDEO_H264HP;
        decPrm->chCreateParams[i].profile                = IH264VDEC_PROFILE_ANY;
        decPrm->chCreateParams[i].fieldMergeDecodeEnable = FALSE;
        decPrm->chCreateParams[i].targetMaxWidth         = gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoWidth;
        decPrm->chCreateParams[i].targetMaxHeight        = gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoHeight;
        decPrm->chCreateParams[i].defaultDynamicParams.targetFrameRate = gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.frameRate;
        decPrm->chCreateParams[i].defaultDynamicParams.targetBitRate   = gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.targetBitRate;
        decPrm->chCreateParams[i].numBufPerCh = NUM_DECODE_BUFFERS;
        decPrm->chCreateParams[i].displayDelay = 2;
    }
    decPrm->tilerEnable = TILER_ENABLE;
}


static
Void multich_cardvr_set_ipcbitsout_hlos_prms(IpcBitsOutLinkHLOS_CreateParams * ipcBitsOutHostPrm)
{
    int i;

    for (i = 0;
         i < (gVdecModuleContext.vdecConfig.numChn);
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
        pChInfo->width          = gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoWidth;
        pChInfo->height         = gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoHeight;
        pChInfo->pitch[0]       = 0; // NOT USED
        pChInfo->pitch[1]       = 0; // NOT USED
        pChInfo->pitch[2]       = 0; // NOT USED
        pChInfo->scanFormat     = SYSTEM_SF_PROGRESSIVE;
    }
    ipcBitsOutHostPrm->baseCreateParams.noNotifyMode = FALSE;
    ipcBitsOutHostPrm->baseCreateParams.notifyNextLink = TRUE;
    ipcBitsOutHostPrm->baseCreateParams.numOutQue = 1;
    ipcBitsOutHostPrm->numBufPerCh[0] =  NUM_IPC_OUT_BUFFERS;
    ipcBitsOutHostPrm->inQueInfo.numCh =
        (gVdecModuleContext.vdecConfig.numChn);
}

Void multich_cardvr_set_osd_prm(AlgLink_CreateParams *dspAlgPrm)
{

    int chId;

    dspAlgPrm->enableOSDAlg = TRUE;
    for(chId = 0; chId < ALG_LINK_OSD_MAX_CH; chId++)
    {
        AlgLink_OsdChWinParams * chWinPrm = &dspAlgPrm->osdChCreateParams[chId].chDefaultParams;

        /* set osd window max width and height */
        dspAlgPrm->osdChCreateParams[chId].maxWidth  = EXAMPLE_OSD_WIN_MAX_WIDTH;
        dspAlgPrm->osdChCreateParams[chId].maxHeight = EXAMPLE_OSD_WIN_MAX_HEIGHT;

        chWinPrm->chId = chId;
        chWinPrm->numWindows = 0;
    }
}

static
Void multich_cardvr_set_enclink_prm(EncLink_CreateParams *encPrm)
{

    EncLink_ChCreateParams *pLinkChPrm;
    EncLink_ChDynamicParams *pLinkDynPrm;
    VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
    VENC_CHN_PARAMS_S *pChPrm;
    int i;

    encPrm->numBufPerCh[0] = NUM_ENCODE_D1_BUFFERS;
    encPrm->numBufPerCh[1] = NUM_ENCODE_CIF_BUFFERS;

    /* Primary Stream Params - D1 */
    for (i=0; i<gVencModuleContext.vencConfig.numPrimaryChn +
                gVencModuleContext.vencConfig.numSecondaryChn; i++)
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
}


static
Void multich_cardvr_set_display_prms(DisplayLink_CreateParams *displayPrm)
{
    displayPrm->displayRes = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
}

static
Void multich_cardvr_connect_links()
{
    multich_cardvr_configure_extvideodecoder_prm(NUM_CAPTURE_DEVICES);

    /**Capture Link**/
    multich_cardvr_set_capture_prm(&gCarDVRUsecaseContext.capturePrm);
    /* Capture --> Q0 --> SELECT_CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX */
    gCarDVRUsecaseContext.capturePrm.outQueParams[0].nextLink   = gCarDVRUsecaseContext.selectLinkId[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX];
    gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX].inQueParams.prevLinkId = gVcapModuleContext.captureId;
    gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX].inQueParams.prevLinkQueId = 0;

    /* Capture --> Q1 --> CIF_DEI_LINK_IDX */
    gCarDVRUsecaseContext.capturePrm.outQueParams[1].nextLink   = gVcapModuleContext.deiId[CIF_DEI_LINK_IDX];
    gCarDVRUsecaseContext.deiPrm[CIF_DEI_LINK_IDX].inQueParams.prevLinkId = gVcapModuleContext.captureId;
    gCarDVRUsecaseContext.deiPrm[CIF_DEI_LINK_IDX].inQueParams.prevLinkQueId  = 1;
    MULTICH_CARDVR_CREATE_LINK (gVcapModuleContext.captureId, &gCarDVRUsecaseContext.capturePrm,
                                sizeof(gCarDVRUsecaseContext.capturePrm));
    /**After Capture is created set capture fps */
    multich_cardvr_set_capture_fps(&gCarDVRUsecaseContext.captureFps);

    /* SELECT_CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX --Q0--> D1_DEI_LINK_IDX */
    gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX].numOutQue = 1;
    gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX].outQueParams[0].nextLink   = gVcapModuleContext.deiId[D1_DEI_LINK_IDX];
    gCarDVRUsecaseContext.deiPrm[D1_DEI_LINK_IDX].inQueParams.prevLinkId = gCarDVRUsecaseContext.selectLinkId[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX];
    gCarDVRUsecaseContext.deiPrm[D1_DEI_LINK_IDX].inQueParams.prevLinkQueId  = 0;
    gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX].outQueChInfo[0].numOutCh = 1;
    gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX].outQueChInfo[0].outQueId = 0;
    gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX].outQueChInfo[0].inChNum[0] = 0;
    MULTICH_CARDVR_CREATE_LINK (gCarDVRUsecaseContext.selectLinkId[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX],
                                &gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX],
                                sizeof(gCarDVRUsecaseContext.selectPrm[CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX]));

    /* D1_DEI_LINK_IDX --DEI_LINK_OUT_QUE_VIP_SC--> D1_CIFCOMPOSITE_MERGE_LINK_IDX */
    multich_cardvr_set_dei_prm(&gCarDVRUsecaseContext.deiPrm[D1_DEI_LINK_IDX],
                               DEI_LINK_OUT_QUE_VIP_SC);
    gCarDVRUsecaseContext.deiPrm[D1_DEI_LINK_IDX].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink  = gCarDVRUsecaseContext.mergeId[D1_CIFCOMPOSITE_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX].inQueParams[D1_CIFCOMPOSITE_MERGE_LINK_DEI0_QIDX].prevLinkId = gVcapModuleContext.deiId[D1_DEI_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX].inQueParams[D1_CIFCOMPOSITE_MERGE_LINK_DEI0_QIDX].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.deiId[D1_DEI_LINK_IDX], &gCarDVRUsecaseContext.deiPrm[D1_DEI_LINK_IDX], sizeof(gCarDVRUsecaseContext.deiPrm[D1_DEI_LINK_IDX]));

    multich_cardvr_set_dei_prm(&gCarDVRUsecaseContext.deiPrm[CIF_DEI_LINK_IDX],
                               DEI_LINK_OUT_QUE_VIP_SC);
    gCarDVRUsecaseContext.deiPrm[CIF_DEI_LINK_IDX].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink            = gVdisModuleContext.swMsId[0];
    gCarDVRUsecaseContext.swMsPrm[0].inQueParams.prevLinkId = gVcapModuleContext.deiId[CIF_DEI_LINK_IDX];
    gCarDVRUsecaseContext.swMsPrm[0].inQueParams.prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.deiId[CIF_DEI_LINK_IDX], &gCarDVRUsecaseContext.deiPrm[CIF_DEI_LINK_IDX], sizeof(gCarDVRUsecaseContext.deiPrm[CIF_DEI_LINK_IDX]));

/*
 *  Disable avsync configuration for capture compositor component
    mulich_cardvr_set_avsync_prm(&gCarDVRUsecaseContext.avsyncPrms[0],
                                 gCarDVRUsecaseContext.swMsPrm[0].inQueParams.prevLinkId,
                                 gCarDVRUsecaseContext.swMsPrm[0].inQueParams.prevLinkQueId,
                                 0);
*/

    multich_cardvr_set_swms_prm(&gCarDVRUsecaseContext.swMsPrm[0],0);
    gCarDVRUsecaseContext.swMsPrm[0].outQueParams.nextLink     = gVcapModuleContext.nsfId[0];
    gCarDVRUsecaseContext.nsfPrm[0].inQueParams.prevLinkId = gVdisModuleContext.swMsId[0];
    gCarDVRUsecaseContext.nsfPrm[0].inQueParams.prevLinkQueId =0;
    MULTICH_CARDVR_CREATE_LINK(gVdisModuleContext.swMsId[0], &gCarDVRUsecaseContext.swMsPrm[0], sizeof(gCarDVRUsecaseContext.swMsPrm[0]));


    multich_cardvr_set_nsf_prm(&gCarDVRUsecaseContext.nsfPrm[0]);
    gCarDVRUsecaseContext.nsfPrm[0].outQueParams[0].nextLink  = gCarDVRUsecaseContext.mergeId[D1_CIFCOMPOSITE_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX].inQueParams[D1_CIFCOMPOSITE_MERGE_LINK_NSF_QIDX].prevLinkId = gVcapModuleContext.nsfId[0];
    gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX].inQueParams[D1_CIFCOMPOSITE_MERGE_LINK_NSF_QIDX].prevLinkQueId = 0;
    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.nsfId[0] , &(gCarDVRUsecaseContext.nsfPrm[0]), sizeof(gCarDVRUsecaseContext.nsfPrm[0]));

    gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX].notifyNextLink                  = TRUE;
    gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX].numInQue = D1_CIFCOMPOSITE_MERGE_LINK_NUM_INQUE;
    gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX].outQueParams.nextLink  = gCarDVRUsecaseContext.dupId[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX];
    gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX].inQueParams.prevLinkId = gCarDVRUsecaseContext.mergeId[D1_CIFCOMPOSITE_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX].inQueParams.prevLinkQueId = 0;
    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.mergeId[D1_CIFCOMPOSITE_MERGE_LINK_IDX], &gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX],
                               sizeof(gCarDVRUsecaseContext.mergePrm[D1_CIFCOMPOSITE_MERGE_LINK_IDX]));

    gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX].numOutQue = 2;
    gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX].notifyNextLink = TRUE;
    gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX].outQueParams[0].nextLink = gCarDVRUsecaseContext.mergeId[FG_BG_ENCODE_DUP_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX].inQueParams[FG_BG_ENCODE_DUP_MERGE_LINK_FGBGDUPQ0_QIDX].prevLinkId = gCarDVRUsecaseContext.dupId[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX].inQueParams[FG_BG_ENCODE_DUP_MERGE_LINK_FGBGDUPQ0_QIDX].prevLinkQueId = 0;
    gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX].outQueParams[1].nextLink = gCarDVRUsecaseContext.mergeId[FG_BG_ENCODE_DUP_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX].inQueParams[FG_BG_ENCODE_DUP_MERGE_LINK_FGBGDUPQ1_QIDX].prevLinkId = gCarDVRUsecaseContext.dupId[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX].inQueParams[FG_BG_ENCODE_DUP_MERGE_LINK_FGBGDUPQ1_QIDX].prevLinkQueId = 1;
    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.dupId[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX],
                      &gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX],
                      sizeof(gCarDVRUsecaseContext.dupPrm[FOREGROUND_BACKGROUND_ENCODE_DUP_LINK_IDX]));

    gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX].numInQue = FG_BG_ENCODE_DUP_MERGE_LINK_NUM_INQUE;
    gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX].notifyNextLink = TRUE;
    gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX].outQueParams.nextLink = gVcapModuleContext.ipcFramesOutVpssId[ENCODE_FRAMESOUT_LINK_IDX];
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.inQueParams.prevLinkId =
                         gCarDVRUsecaseContext.mergeId[FG_BG_ENCODE_DUP_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.inQueParams.prevLinkQueId = 0;
    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.mergeId[FG_BG_ENCODE_DUP_MERGE_LINK_IDX],
                      &gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX],
                      sizeof(gCarDVRUsecaseContext.mergePrm[FG_BG_ENCODE_DUP_MERGE_LINK_IDX]));

    /* Redirect to VIDEO-M3 for Capture channel encode */
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.outQueParams[0].nextLink  = gCarDVRUsecaseContext.mergeId[LIVE_PLAYBACK_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX].inQueParams[LIVE_PLAYBACK_MERGE_LINK_LIVECH_QIDX].prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[ENCODE_FRAMESOUT_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX].inQueParams[LIVE_PLAYBACK_MERGE_LINK_LIVECH_QIDX].prevLinkQueId = 0;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.processLink               = gCarDVRUsecaseContext.ipcFramesInVideoLiveEncode;
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm.baseCreateParams.inQueParams.prevLinkId      = gVcapModuleContext.ipcFramesOutVpssId[ENCODE_FRAMESOUT_LINK_IDX];
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.notifyPrevLink            = TRUE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.notifyNextLink            = TRUE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.notifyProcessLink         = TRUE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.noNotifyMode              = FALSE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX].baseCreateParams.numOutQue                 = 1;

    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.ipcFramesOutVpssId[ENCODE_FRAMESOUT_LINK_IDX], &gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX],
                      sizeof(gCarDVRUsecaseContext.ipcFramesOutVpssPrm[ENCODE_FRAMESOUT_LINK_IDX]));

    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm.baseCreateParams.outQueParams[0].nextLink    = gCarDVRUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_IPCFRAMESIN_QIDX].prevLinkId = gCarDVRUsecaseContext.ipcFramesInVideoLiveEncode;
    gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_IPCFRAMESIN_QIDX].prevLinkQueId = 0;
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm.baseCreateParams.notifyPrevLink              = TRUE;
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm.baseCreateParams.notifyNextLink              = TRUE;
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm.baseCreateParams.noNotifyMode                = FALSE;
    gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm.baseCreateParams.numOutQue                   = 1;

    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.ipcFramesInVideoLiveEncode, &gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm,
                      sizeof(gCarDVRUsecaseContext.ipcFramesInVideoLiveEncodePrm));

    multich_cardvr_set_ipcbitsout_hlos_prms(&gCarDVRUsecaseContext.ipcBitsOutHostPrm);
    gCarDVRUsecaseContext.ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.ipcBitsInRTOSId;
    gCarDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsOutHLOSId;
    gCarDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId    = 0;

    MULTICH_CARDVR_CREATE_LINK(gVdecModuleContext.ipcBitsOutHLOSId,
                      &gCarDVRUsecaseContext.ipcBitsOutHostPrm,
                      sizeof(gCarDVRUsecaseContext.ipcBitsOutHostPrm));

    gCarDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.numOutQue                    = 1;
    gCarDVRUsecaseContext.ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink     = gVdecModuleContext.decId;
    gCarDVRUsecaseContext.decPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsInRTOSId;
    gCarDVRUsecaseContext.decPrm.inQueParams.prevLinkQueId = 0;
    MultiCh_ipcBitsInitCreateParams_BitsInRTOS(&gCarDVRUsecaseContext.ipcBitsInVideoPrm, TRUE);

    MULTICH_CARDVR_CREATE_LINK(gVdecModuleContext.ipcBitsInRTOSId,
                      &gCarDVRUsecaseContext.ipcBitsInVideoPrm,
                      sizeof(gCarDVRUsecaseContext.ipcBitsInVideoPrm));
    multich_cardvr_set_declink_prms(&gCarDVRUsecaseContext.decPrm);
    gCarDVRUsecaseContext.decPrm.outQueParams.nextLink  = gCarDVRUsecaseContext.ipcOutVideoId;
    gCarDVRUsecaseContext.ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    gCarDVRUsecaseContext.ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;

    MULTICH_CARDVR_CREATE_LINK(gVdecModuleContext.decId,
                      &gCarDVRUsecaseContext.decPrm,
                      sizeof(gCarDVRUsecaseContext.decPrm));

    gCarDVRUsecaseContext.ipcOutVideoPrm.numOutQue                 = 1;
    gCarDVRUsecaseContext.ipcOutVideoPrm.outQueParams[0].nextLink  = gCarDVRUsecaseContext.ipcInVpssId;
    gCarDVRUsecaseContext.ipcInVpssPrm.inQueParams.prevLinkId    = gCarDVRUsecaseContext.ipcOutVideoId;
    gCarDVRUsecaseContext.ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    gCarDVRUsecaseContext.ipcOutVideoPrm.notifyNextLink            = TRUE;
    gCarDVRUsecaseContext.ipcOutVideoPrm.notifyPrevLink            = TRUE;
    gCarDVRUsecaseContext.ipcOutVideoPrm.noNotifyMode              = FALSE;


    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.ipcOutVideoId,
                      &gCarDVRUsecaseContext.ipcOutVideoPrm,
                      sizeof(gCarDVRUsecaseContext.ipcOutVideoPrm));


    gCarDVRUsecaseContext.ipcInVpssPrm.numOutQue                 = 1;
    gCarDVRUsecaseContext.ipcInVpssPrm.outQueParams[0].nextLink  = gVdisModuleContext.swMsId[1];
    gCarDVRUsecaseContext.swMsPrm[1].inQueParams.prevLinkId      = gCarDVRUsecaseContext.ipcInVpssId;
    gCarDVRUsecaseContext.swMsPrm[1].inQueParams.prevLinkQueId   = 0;
    gCarDVRUsecaseContext.ipcInVpssPrm.notifyNextLink            = TRUE;
    gCarDVRUsecaseContext.ipcInVpssPrm.notifyPrevLink            = TRUE;
    gCarDVRUsecaseContext.ipcInVpssPrm.noNotifyMode              = FALSE;

    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.ipcInVpssId,
                      &gCarDVRUsecaseContext.ipcInVpssPrm,
                      sizeof(gCarDVRUsecaseContext.ipcInVpssPrm));

    mulich_cardvr_set_avsync_prm(&gCarDVRUsecaseContext.avsyncPrms[1],
                                 gCarDVRUsecaseContext.swMsPrm[1].inQueParams.prevLinkId,
                                 gCarDVRUsecaseContext.swMsPrm[1].inQueParams.prevLinkQueId,
                                 1);

    multich_cardvr_set_swms_prm(&gCarDVRUsecaseContext.swMsPrm[1],1);
    gCarDVRUsecaseContext.swMsPrm[1].outQueParams.nextLink     = gVcapModuleContext.nsfId[1];
    gCarDVRUsecaseContext.nsfPrm[1].inQueParams.prevLinkId     = gVdisModuleContext.swMsId[1];
    gCarDVRUsecaseContext.nsfPrm[1].inQueParams.prevLinkQueId  = 0;
    MULTICH_CARDVR_CREATE_LINK(gVdisModuleContext.swMsId[1],
                      &gCarDVRUsecaseContext.swMsPrm[1],
                      sizeof(gCarDVRUsecaseContext.swMsPrm[1]));

    multich_cardvr_set_nsf_prm(&gCarDVRUsecaseContext.nsfPrm[1]);
    gCarDVRUsecaseContext.nsfPrm[1].outQueParams[0].nextLink = gCarDVRUsecaseContext.mergeId[LIVE_PLAYBACK_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX].inQueParams[LIVE_PLAYBACK_MERGE_LINK_PLAYBACKCH_QIDX].prevLinkId = gVcapModuleContext.nsfId[1];
    gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX].inQueParams[LIVE_PLAYBACK_MERGE_LINK_PLAYBACKCH_QIDX].prevLinkQueId = 0;
    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.nsfId[1],
                      &gCarDVRUsecaseContext.nsfPrm[1],
                      sizeof(gCarDVRUsecaseContext.nsfPrm[1]));

    gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX].notifyNextLink                  = TRUE;
    gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX].numInQue = LIVE_PLAYBACK_MERGE_LINK_NUM_INQUE;
    gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX].outQueParams.nextLink  = gCarDVRUsecaseContext.selectLinkId[DISPLAY_SELECT_LINK_IDX];
    gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX].inQueParams.prevLinkId = gCarDVRUsecaseContext.mergeId[LIVE_PLAYBACK_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX].inQueParams.prevLinkQueId = 0;

    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.mergeId[LIVE_PLAYBACK_MERGE_LINK_IDX],
                      &gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX],
                      sizeof(gCarDVRUsecaseContext.mergePrm[LIVE_PLAYBACK_MERGE_LINK_IDX]));

    /* SELECT_CAPTURE_D1_SINGLE_CH_SELECT_LINK_IDX --Q0--> D1_DEI_LINK_IDX */
    gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX].numOutQue = 1;
    gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX].outQueParams[0].nextLink   = gVcapModuleContext.ipcFramesOutVpssId[OSD_FRAMESOUT_LINK_IDX];
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.inQueParams.prevLinkId   = gCarDVRUsecaseContext.selectLinkId[DISPLAY_SELECT_LINK_IDX];
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.inQueParams.prevLinkQueId = 0;
    gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX].outQueChInfo[0].numOutCh = 1;
    gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX].outQueChInfo[0].outQueId = 0;
    gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX].outQueChInfo[0].inChNum[0] = 0;
    MULTICH_CARDVR_CREATE_LINK (gCarDVRUsecaseContext.selectLinkId[DISPLAY_SELECT_LINK_IDX],
                                &gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX],
                                sizeof(gCarDVRUsecaseContext.selectPrm[DISPLAY_SELECT_LINK_IDX]));

    /* Redirect to DSP for OSD */
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.outQueParams[0].nextLink  = gCarDVRUsecaseContext.dupId[DISPLAY_ENCODE_DUP_LINK_IDX];
    gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[OSD_FRAMESOUT_LINK_IDX];
    gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.processLink               = gVcapModuleContext.ipcFramesInDspId[0];
    gCarDVRUsecaseContext.ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkId      = gVcapModuleContext.ipcFramesOutVpssId[OSD_FRAMESOUT_LINK_IDX];
    gCarDVRUsecaseContext.ipcFramesInDspPrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.notifyPrevLink            = TRUE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.notifyNextLink            = TRUE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.notifyProcessLink         = TRUE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.noNotifyMode              = FALSE;
    gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX].baseCreateParams.numOutQue                 = 1;

    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.ipcFramesOutVpssId[OSD_FRAMESOUT_LINK_IDX],
                      &gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX],
                      sizeof(gCarDVRUsecaseContext.ipcFramesOutVpssPrm[OSD_FRAMESOUT_LINK_IDX]));

    gCarDVRUsecaseContext.ipcFramesInDspPrm.baseCreateParams.outQueParams[0].nextLink    = gVcapModuleContext.dspAlgId[0];
    gCarDVRUsecaseContext.dspAlgPrm.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
    gCarDVRUsecaseContext.dspAlgPrm.inQueParams.prevLinkQueId = 0;
    gCarDVRUsecaseContext.ipcFramesInDspPrm.baseCreateParams.notifyPrevLink              = TRUE;
    gCarDVRUsecaseContext.ipcFramesInDspPrm.baseCreateParams.notifyNextLink              = TRUE;
    gCarDVRUsecaseContext.ipcFramesInDspPrm.baseCreateParams.noNotifyMode                = FALSE;
    gCarDVRUsecaseContext.ipcFramesInDspPrm.baseCreateParams.numOutQue                   = 1;

    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.ipcFramesInDspId[0],
                      &gCarDVRUsecaseContext.ipcFramesInDspPrm,
                      sizeof(gCarDVRUsecaseContext.ipcFramesInDspPrm));

    multich_cardvr_set_osd_prm(&gCarDVRUsecaseContext.dspAlgPrm);
    MULTICH_CARDVR_CREATE_LINK(gVcapModuleContext.dspAlgId[0],
                      &gCarDVRUsecaseContext.dspAlgPrm,
                      sizeof(gCarDVRUsecaseContext.dspAlgPrm));

    gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX].numOutQue                      = 2;
    gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX].outQueParams[0].nextLink       = Vdis_getDisplayId(VDIS_DEV_SD);
    gCarDVRUsecaseContext.displayPrm[VDIS_DEV_SD].inQueParams[0].prevLinkId    = gCarDVRUsecaseContext.dupId[DISPLAY_ENCODE_DUP_LINK_IDX];
    gCarDVRUsecaseContext.displayPrm[VDIS_DEV_SD].inQueParams[0].prevLinkQueId = 0;
    gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX].outQueParams[1].nextLink       = gCarDVRUsecaseContext.ipcOutVpssId;
    gCarDVRUsecaseContext.ipcOutVpssPrm.inQueParams.prevLinkId       = gCarDVRUsecaseContext.dupId[DISPLAY_ENCODE_DUP_LINK_IDX];
    gCarDVRUsecaseContext.ipcOutVpssPrm.inQueParams.prevLinkQueId    = 1;
    gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX].notifyNextLink                 = TRUE;

    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.dupId[DISPLAY_ENCODE_DUP_LINK_IDX],
                      &gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX],
                      sizeof(gCarDVRUsecaseContext.dupPrm[DISPLAY_ENCODE_DUP_LINK_IDX]));

    gCarDVRUsecaseContext.ipcOutVpssPrm.numOutQue                    = 1;
    gCarDVRUsecaseContext.ipcOutVpssPrm.outQueParams[0].nextLink     = gCarDVRUsecaseContext.ipcInVideoId;
    gCarDVRUsecaseContext.ipcInVideoPrm.inQueParams.prevLinkId       = gCarDVRUsecaseContext.ipcOutVpssId;
    gCarDVRUsecaseContext.ipcInVideoPrm.inQueParams.prevLinkQueId    = 0;
    gCarDVRUsecaseContext.ipcOutVpssPrm.notifyNextLink               = TRUE;
    gCarDVRUsecaseContext.ipcOutVpssPrm.notifyPrevLink               = TRUE;
    gCarDVRUsecaseContext.ipcOutVpssPrm.noNotifyMode                 = FALSE;

    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.ipcOutVpssId,
                      &gCarDVRUsecaseContext.ipcOutVpssPrm,
                      sizeof(gCarDVRUsecaseContext.ipcOutVpssPrm));

    gCarDVRUsecaseContext.ipcInVideoPrm.numOutQue                    = 1;
    gCarDVRUsecaseContext.ipcInVideoPrm.outQueParams[0].nextLink     = gCarDVRUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_IPCINM3_QIDX].prevLinkId = gCarDVRUsecaseContext.ipcInVideoId;
    gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].inQueParams[PRE_ENCODE_MERGE_LINK_IPCINM3_QIDX].prevLinkQueId = 0;
    gCarDVRUsecaseContext.ipcInVideoPrm.notifyNextLink               = TRUE;
    gCarDVRUsecaseContext.ipcInVideoPrm.notifyPrevLink               = TRUE;
    gCarDVRUsecaseContext.ipcInVideoPrm.noNotifyMode                 = FALSE;

    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.ipcInVideoId,
                      &gCarDVRUsecaseContext.ipcInVideoPrm,
                      sizeof(gCarDVRUsecaseContext.ipcInVideoPrm));

    gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].notifyNextLink                  = TRUE;
    gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].numInQue = PRE_ENCODE_MERGE_LINK_NUM_INQUE;
    gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX].outQueParams.nextLink  = gVencModuleContext.encId;
    gCarDVRUsecaseContext.encPrm.inQueParams.prevLinkId    = gCarDVRUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX];
    gCarDVRUsecaseContext.encPrm.inQueParams.prevLinkQueId = 0;

    MULTICH_CARDVR_CREATE_LINK(gCarDVRUsecaseContext.mergeId[PRE_ENCODE_MERGE_LINK_IDX],
                      &gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX],
                      sizeof(gCarDVRUsecaseContext.mergePrm[PRE_ENCODE_MERGE_LINK_IDX]));

    multich_cardvr_set_enclink_prm(&gCarDVRUsecaseContext.encPrm);
    gCarDVRUsecaseContext.encPrm.outQueParams.nextLink     = gVencModuleContext.ipcBitsOutRTOSId;
    gCarDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.encId;
    gCarDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;

    MULTICH_CARDVR_CREATE_LINK(gVencModuleContext.encId,
                      &gCarDVRUsecaseContext.encPrm,
                      sizeof(gCarDVRUsecaseContext.encPrm));


    gCarDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    gCarDVRUsecaseContext.ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&gCarDVRUsecaseContext.ipcBitsOutVideoPrm,
                                               TRUE);
    MULTICH_CARDVR_CREATE_LINK(gVencModuleContext.ipcBitsOutRTOSId,
                      &gCarDVRUsecaseContext.ipcBitsOutVideoPrm,
                      sizeof(gCarDVRUsecaseContext.ipcBitsOutVideoPrm));

    gCarDVRUsecaseContext.ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId = gVencModuleContext.ipcBitsOutRTOSId;
    gCarDVRUsecaseContext.ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&gCarDVRUsecaseContext.ipcBitsInHostPrm);

    MULTICH_CARDVR_CREATE_LINK(gVencModuleContext.ipcBitsInHLOSId,
                      &gCarDVRUsecaseContext.ipcBitsInHostPrm,
                      sizeof(gCarDVRUsecaseContext.ipcBitsInHostPrm));

    multich_cardvr_set_display_prms(&gCarDVRUsecaseContext.displayPrm[VDIS_DEV_SD]);

    MULTICH_CARDVR_CREATE_LINK(Vdis_getDisplayId(VDIS_DEV_SD),
                      &gCarDVRUsecaseContext.displayPrm[VDIS_DEV_SD],
                      sizeof(gCarDVRUsecaseContext.displayPrm[VDIS_DEV_SD]));




}


static
void multich_cardvr_set_dec2disp_chmap()
{
    MergeLink_InLinkChInfo inChInfo;

    MergeLink_InLinkChInfo_Init(&inChInfo);
    inChInfo.inLinkID = gVcapModuleContext.nsfId[1];
    System_linkControl(gCarDVRUsecaseContext.mergeId[LIVE_PLAYBACK_MERGE_LINK_IDX],
                       MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO,
                       &inChInfo,
                       sizeof(inChInfo),
                       TRUE);
    OSA_assert(inChInfo.numCh == gVdecModuleContext.vdecConfig.numChn);

    MultiCh_setDec2DispMap(VDIS_DEV_SD,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
}

Void MultiCh_createCarDVR()
{
    printf("\n********* Entered usecase D1 + 4 CIF CarDVR usecase \n\n");

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

    multich_cardvr_reset_link_prms();
    multich_cardvr_set_link_ids();

    multich_cardvr_connect_links();
    multich_cardvr_set_links_framerate();
    multich_cardvr_set_dec2disp_chmap();
}

Void MultiCh_deleteCarDVR()
{
    UInt32 i;

    for (i = 0; i < gCarDVRUsecaseContext.createdLinkCount; i++)
    {
        System_linkDelete (gCarDVRUsecaseContext.createdLinks[i]);
    }
    gCarDVRUsecaseContext.createdLinkCount = 0;
    multich_cardvr_reset_link_ids();

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);
}

Void MultiCh_carDVRSwitchDisplayCh(Int displayChId)
{
    SelectLink_OutQueChInfo selectPrm;

    if (displayChId < MULTICH_CAR_DVR_USECASE_NUM_DISPLAY_CH)
    {
        selectPrm.outQueId = 0;
        selectPrm.numOutCh = 1;
        selectPrm.inChNum[0]  = displayChId;
        System_linkControl(
            gCarDVRUsecaseContext.selectLinkId[DISPLAY_SELECT_LINK_IDX],
            SELECT_LINK_CMD_SET_OUT_QUE_CH_INFO,
            &selectPrm,
            sizeof(selectPrm),
            TRUE);
    }
}


/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


#include "mcfw/src_linux/mcfw_api/usecases/multich_common.h"
#include "mcfw/src_linux/mcfw_api/usecases/multich_ipcbits.h"
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
                        +----------------------+
                        |   Capture (YUV422I)  |
                        |                      |
                        |   16 CH D1 60fps     |
                        +----------+-----------+
                                   |
                                   |
                                   v
                        +----------------------+
                        | CAPTURE_DISPLAY_DUP  |
                        +--+------------+------+
       +-------------------+            |
       |                                |
       |                                v            <<Process
       |                           +--------------+    Link>>    +---------------+      +---------+
       |                           +IPC Frames Out+------------->|IPC Frames In  +----->| AlgLink |
       |                           |    (M3)      <--------------+    (DSP)      <------+  (SWOSD)|
       |                           +---+-------+--+              +---------------+      +---------+
       |                               |       |
       |                               |       |
       |                               |       |8CH D1
       |                         8CH D1|       |YUV422
       |                         YUV422|       |
       |                               |       +---------------------+
       |                               |                             |
       |                               |                             |
       |                               v                             v
       |                         +-------------+              +--------------+
       |                         |   DEIH      |              |    DEI       |
       |                         |             |              |              |
       |                         |DEI_SC VIP_SC|              |DEI_SC  VIP_SC|
       |                         +-------------+              +--------------+
       |                      8CH D1|    |  |8CH D1          8CH D1|   |  |8CH D1
       |                      YUV422|    |  |YUV420          YUV422|   |  |YUV420
       |                            |    |  |                      |   |  +---------------+
       |                            |    |  +----------------------|---|-------+          |
       |                            |    |                         |   |       |          |
       |                            |    |8CH CIF               8CH|CIF|       |          |
       |                  +---------+    |YUV420                YUV|420|      +v----------v----+
       |                  |              |                         |   |      |MERGE_VIP_SC_D1 |
       |                  |     +----------------------------------+   |      |                |
       |                  |     |        |                             |      +---------+------+
       |                  |     |        +---------+    +--------------+                |16CH D1
       |                  |     |                  |    |                               v
       |                  |     |                  |    |                      +---------------+
       |                  |     |                  |    |                      |   DUP_D1_LINK |
       |                  |     |                  v    v                      |               |
       |                  |     |            +----------------+                +-------+-----+-+
       |                  |     |            |MERGE_VIP_SC_   |                 16CH D1|     |16CH D1
       |                  |     |            |SECONDARY_OUT   |                  YUV420|     |YUV420
       |                  |     |            +-------+--------+                  PRI   |     |MJPEG
       |                  |     |                    |                                 |     |
       |                  |     |                    |16CH CIF                         |     |
       |                  |     |                    |YUV420        +--------------+   |     |
       |                  |     |                    v              |    16CH CIF  |   |     |
       |                  |     |            +-----------------+    |    YUV420    |   |     |
       |                  |     |            |  CIF DUP LINK   |    |              v   v     v
       |                  |     |            |                 |    |             +----------------+
       |                  |     |            +--+----+------+--+    |             | MERGE_D1_CIF   |
       |                  |     |               |    |      |       |             |                |
       |                  |     |               |    |      +-------+             +---------+------+
       |                  |     |    +----------+    |                                      |            48CH YUV420
       |            8CH   | 8CH |    |               |                                      +-------------------+
       +            D1    |  D1 |    |16CH           v                                                          |
       v                  |     |    |CIF       +-------------+       +------------+        +----------+        |
  +----------+            v     v    v          |IPC FramesOut+------>|IPC FramesIn+------->| AlgLink  |        |
  |SDTV Disp |         +--------------+         |   (M3)      <------+|   (DSP)    |<------+|  (SCD)   |        |
  |          |         |              |<---+    +-------------+       +------------+        +----------+        |
  +----------+         | LIVE_DECODE  | 16CH                                                                    |
                       |  MERGE       | D1 |                                                                    |
                       |              |Play|     +------------+          +--------+         +--------+          |
                       +-----+--------+    |     |            |          |IPCBits |         |IPCBits |          |
                             |             +-----+ DecLink    |<---------|Out (A8)|         |In(A8)  |          |
                             |                   +------------+  IPCBits +--------+         +--------+          v
                             |48CH                               In(M3)                         ^        +------------+
                             |                                                           IPCBits|        |  IPCM3OUT  |
                             |                                                           Out(M3)|        +------+-----+
                             v                                                              +---+-----+         |
                       +--------------+                                                     |EncLink  |         |
                       | LIVE_DECODE  |                                                     +---------+         v
                       |  DUP         |                                                          ^       +------------+
                       +-+----------+-+                                                          |       |  IPCM3IN   |
                         |          |                                                            |       +------+-----+
                         |          |                                                            |              |
                         v          v                                                            +--------------+
                  +---------+    +-----------+
                  |SWMS0    |    |SWMS1      |
                  |         |    |           |
                  +----+----+    +----+------+
                       |              |
                       v              v
                   On-chip          Off-chip  HDDAC
                   HDMI             HDMI     + (VGA)
                   (HDMI0)          (HDMI1)

*/

//#define USE_DEI_SC_FOR_SECONDARY
#undef USE_DEI_SC_FOR_SECONDARY

/* Added separate path for SCD as SCD should get frame without any external content e.g. OSD */
//#undef USE_SCLR_FOR_SCD
#define USE_SCLR_FOR_SCD

#define     NUM_CAPTURE_DEVICES          4


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
        .EncChList = {0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45 },
        .DecNumCh  = 6,
        .DecChList = {0, 3, 6, 9, 12, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 16,
        .EncChList = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46 },
        .DecNumCh  = 5,
        .DecChList = {1, 4, 7, 10, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 16,
        .EncChList = {2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47 },
        .DecNumCh  = 5,
        .DecChList = {2, 5, 8, 11, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
};


#define     NUM_MERGE_LINK                          4
#define     DEI_VIP_SC_D1_MERGE_LINK_IDX           0
#define     DEI_SC_CIF_MERGE_LINK_IDX              1
#define     D1_CIF_MERGE_LINK_IDX                  2
#define     LIVE_DECODE_MERGE_LINK_IDX             3

#define     NUM_DUP_LINK                            4
#define     D1_DUP_LINK_IDX                         0
#define     LIVE_DECODE_DUP_LINK_IDX               1
#define     CIF_DUP_LINK_IDX                       2
#define     CAPTURE_DISPLAY_DUP_LINK_IDX           3

#define     NUM_DEI_CHAN                           8
static
Void mulich_progressive_set_avsync_vidque_prm(Avsync_SynchConfigParams *queCfg,
                                            Int chnum,
                                            UInt32 avsStartChNum,
                                            UInt32 avsEndChNum,
                                            VDIS_DEV vdDevId)
{
    queCfg->chNum = chnum;
    queCfg->audioPresent = FALSE;
    if ((queCfg->chNum >= avsStartChNum)
        &&
        (queCfg->chNum <= avsEndChNum)
        &&
        (gVsysModuleContext.vsysConfig.enableAVsync)
        &&
        (VDIS_DEV_SD != vdDevId))
    {
        queCfg->avsyncEnable = FALSE;
    }
    else
    {
        queCfg->avsyncEnable = FALSE;
    }
    queCfg->clkAdjustPolicy.refClkType = AVSYNC_REFCLKADJUST_NONE;
    queCfg->playTimerStartTimeout = 0;
    queCfg->playStartMode =     AVSYNC_PLAYBACK_START_MODE_WAITSYNCH;
    queCfg->ptsInitMode   = AVSYNC_PTS_INIT_MODE_APP;
}

static
Void mulich_progressive_set_avsync_prm(AvsyncLink_LinkSynchConfigParams *avsyncPrm,
                                     UInt32 swMsIdx,
                                     VDIS_DEV vdDevId)
{
    Int i;
    Int32 status;

    Vdis_getAvsyncConfig(vdDevId,avsyncPrm);
    avsyncPrm->displayLinkID        = Vdis_getDisplayId(vdDevId);
    avsyncPrm->videoSynchLinkID = gVdisModuleContext.swMsId[swMsIdx];
    avsyncPrm->numCh            = gVdecModuleContext.vdecConfig.numChn;
    avsyncPrm->syncMasterChnum =  AVSYNC_INVALID_CHNUM;
    for (i = 0; i < avsyncPrm->numCh;i++)
    {
        mulich_progressive_set_avsync_vidque_prm(&avsyncPrm->queCfg[i],
                                               i+16,
                                               16,
                                               (16 + gVdecModuleContext.vdecConfig.numChn - 1),
                                               vdDevId);
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


Void MultiCh_createProgressiveVcapVencVdecVdis()
{
    CaptureLink_CreateParams    capturePrm;
#ifdef USE_DEI_SC_FOR_SECONDARY
    NsfLink_CreateParams        nsfPrm;
#endif
#ifdef USE_SCLR_FOR_SCD
    NsfLink_CreateParams            nsfParam;
    SclrLink_CreateParams           sclrPrm;
#endif
    SelectLink_CreateParams selectPrm;
    DeiLink_CreateParams        deiPrm[MAX_DEI_LINK];
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
    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm[2];
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm[2];
    AlgLink_CreateParams                dspAlgPrm[2];

    System_LinkInfo                   bitsProducerLinkInfo;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 nullId;
    UInt32 grpxId[VDIS_DEV_MAX];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    AvsyncLink_LinkSynchConfigParams   avsyncCfg[VDIS_DEV_MAX];

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];
    UInt32 vipInstId;

    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm;
    UInt32 ipcBitsOutDSPId;


    UInt32 selectId;
    UInt32 i,j, chId;
    UInt32 numSubChains;
    Bool   enableSdtv;
    Bool   enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool   osdFormat        = gVsysModuleContext.vsysConfig.osdFormat; // 0 for 420SP and 1 for 422i
    Bool   enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;

    /* Disabling SDTV temporarily to enable ratio based resolution setting in DEI; should be fixed */
    enableSdtv = TRUE;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm[0]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm[1]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm[0]);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm[1]);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm[0]);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm[1]);
    MULTICH_INIT_STRUCT(DecLink_CreateParams, decPrm);
    MULTICH_INIT_STRUCT(EncLink_CreateParams, encPrm);
        MULTICH_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm[0]);
            MULTICH_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm[1]);
#ifdef USE_SCLR_FOR_SCD
    MULTICH_INIT_STRUCT(NsfLink_CreateParams,nsfParam);
#endif
#ifdef USE_DEI_SC_FOR_SECONDARY
    MULTICH_INIT_STRUCT(NsfLink_CreateParams,nsfPrm);
#endif
    for (i = 0; i < VDIS_DEV_MAX;i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams ,displayPrm[i]);
        MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[i]);
    }
    printf("\n********* Entered usecase 16CH Progressive <816x> Cap/Enc/Dec/Dis \n\n");

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
#ifdef USE_DEI_SC_FOR_SECONDARY
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
#endif
    selectId                        = SYSTEM_VPSS_LINK_ID_SELECT_0;

    if(enableOsdAlgLink)
    {
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_ALG_0;
        gVcapModuleContext.ipcFramesOutVpssId[0] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
        gVcapModuleContext.ipcFramesInDspId[0] = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
    }
    if(enableScdAlgLink)
    {
#ifdef USE_SCLR_FOR_SCD
        gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
        gVcapModuleContext.sclrId[0]       = SYSTEM_LINK_ID_SCLR_INST_0;
#endif
        gVcapModuleContext.ipcFramesOutVpssId[1] = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
        gVcapModuleContext.ipcFramesInDspId[1]   = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_1;
        gVcapModuleContext.dspAlgId[1]           = SYSTEM_LINK_ID_ALG_1;
        ipcBitsOutDSPId                          = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
        gVcapModuleContext.ipcBitsInHLOSId       = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

    }
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[1].numSwMsInst = 1;
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; /* ON CHIP HDMI */
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; /* OFF CHIP HDMI */
    gVdisModuleContext.displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; /* OFF CHIP HDMI */

    mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX]  = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[LIVE_DECODE_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_1;
    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        mergeId[DEI_SC_CIF_MERGE_LINK_IDX]      = SYSTEM_VPSS_LINK_ID_MERGE_2;
        mergeId[D1_CIF_MERGE_LINK_IDX]          = SYSTEM_VPSS_LINK_ID_MERGE_3;
    }
    else
    {
        mergeId[DEI_SC_CIF_MERGE_LINK_IDX]      = SYSTEM_LINK_ID_INVALID;
        mergeId[D1_CIF_MERGE_LINK_IDX]          = SYSTEM_LINK_ID_INVALID;
    }
    nullId                              = SYSTEM_VPSS_LINK_ID_NULL_0;
    dupId[D1_DUP_LINK_IDX]              = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_DUP_1;
    dupId[CIF_DUP_LINK_IDX]             = SYSTEM_VPSS_LINK_ID_DUP_2;
    dupId[CAPTURE_DISPLAY_DUP_LINK_IDX] = SYSTEM_VPSS_LINK_ID_DUP_3;
    grpxId[0]                       = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]                       = SYSTEM_LINK_ID_GRPX_1;
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
    grpxId[2]                       = SYSTEM_LINK_ID_GRPX_2;
#endif


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
    capturePrm.outQueParams[0].nextLink   = dupId[CAPTURE_DISPLAY_DUP_LINK_IDX];

    dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].notifyNextLink = TRUE;
    dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].inQueParams.prevLinkId = gVcapModuleContext.captureId;
    dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].inQueParams.prevLinkQueId = 0;

    numSubChains                          = 2;

    /* If SWOSD is enabled and osdFormat is 422i link SWOSD after capture and before DEI*/
    if(enableOsdAlgLink & osdFormat)
    {
        dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].numOutQue = 1;
        dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].outQueParams[0].nextLink = gVcapModuleContext.ipcFramesOutVpssId[0];
        if (enableSdtv == TRUE)
        {
            dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].numOutQue = 2;
            dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].outQueParams[1].nextLink = gVdisModuleContext.displayId[2];
        }

        ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkId = dupId[CAPTURE_DISPLAY_DUP_LINK_IDX];
        ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyPrevLink = TRUE;

        ipcFramesOutVpssPrm[0].baseCreateParams.numOutQue = numSubChains;
        ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.deiId[0];
        ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[1].nextLink = gVcapModuleContext.deiId[1];

        ipcFramesOutVpssPrm[0].baseCreateParams.notifyNextLink = TRUE;

        ipcFramesOutVpssPrm[0].baseCreateParams.processLink = gVcapModuleContext.ipcFramesInDspId[0];
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyProcessLink = TRUE;
        ipcFramesOutVpssPrm[0].baseCreateParams.noNotifyMode = FALSE;

        //prevLink->processLink->nextLink
        ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
        ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm[0].baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[0];
        ipcFramesInDspPrm[0].baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesInDspPrm[0].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm[0].baseCreateParams.noNotifyMode   = FALSE;

        dspAlgPrm[0].enableOSDAlg = TRUE;
        dspAlgPrm[0].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
        dspAlgPrm[0].inQueParams.prevLinkQueId = 0;
    }
    else
    {
        dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].numOutQue = 1;
        dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].outQueParams[0].nextLink = selectId;

        selectPrm.numOutQue = 2;

        selectPrm.inQueParams.prevLinkId = dupId[CAPTURE_DISPLAY_DUP_LINK_IDX];
        selectPrm.inQueParams.prevLinkQueId  = 0;

        selectPrm.outQueParams[0].nextLink = gVcapModuleContext.deiId[0];
        selectPrm.outQueParams[1].nextLink = gVcapModuleContext.deiId[1];
        
        selectPrm.outQueChInfo[0].outQueId = 0;
        selectPrm.outQueChInfo[0].numOutCh = NUM_DEI_CHAN;

        selectPrm.outQueChInfo[1].outQueId = 1;
        selectPrm.outQueChInfo[1].numOutCh = NUM_DEI_CHAN;

        for(chId=0; chId<NUM_DEI_CHAN; chId++)
        {
            selectPrm.outQueChInfo[0].inChNum[chId] = chId;
            selectPrm.outQueChInfo[1].inChNum[chId] = NUM_DEI_CHAN + chId;
        }

        if (enableSdtv == TRUE)
        {
            dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].numOutQue = 2;
            dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].outQueParams[1].nextLink = gVdisModuleContext.displayId[2];
        }
    }

    capturePrm.numVipInst                 = 2*numSubChains;

    capturePrm.tilerEnable                = FALSE;
    capturePrm.numBufsPerCh               = 10;
    capturePrm.numExtraBufs               = 6;
    capturePrm.maxBlindAreasPerCh       = 4;

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

    for(i=0; i<numSubChains; i++)
    {
        DeiLink_CreateParams_Init(&deiPrm[i]);
        if(enableOsdAlgLink & osdFormat)
            deiPrm[i].inQueParams.prevLinkId                      = gVcapModuleContext.ipcFramesOutVpssId[0];
        else
            deiPrm[i].inQueParams.prevLinkId                      = selectId;
        deiPrm[i].inQueParams.prevLinkQueId                     = i;

        /* Set Output Scaling at DEI based on ratio */
#ifdef USE_DEI_SC_FOR_SECONDARY
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator = 2;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 2;

        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

#else
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.widthRatio.denominator = 2;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0].ratio.heightRatio.denominator = 2;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT][0];
#endif
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];

        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink            = mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX];
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = TRUE;
        if (gVsysModuleContext.vsysConfig.enableSecondaryOut == FALSE)
        {
#ifdef USE_DEI_SC_FOR_SECONDARY
            deiPrm[i].enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = FALSE;
            deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            = nullId;
#else
            deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink          = nullId;
            deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]                      = FALSE;
#endif
        }
        else
        {
#ifdef USE_DEI_SC_FOR_SECONDARY
            deiPrm[i].enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
            deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            = mergeId[DEI_SC_CIF_MERGE_LINK_IDX];
            deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink          = nullId;
            deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]                      = FALSE;
#else
            deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink   = mergeId[DEI_SC_CIF_MERGE_LINK_IDX];
            deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]    = TRUE;
#endif
        }
        if(enableOsdAlgLink & osdFormat)
        {
            /* If osdFormat is 422i then tiler can be enabled because */
            /* swosd will be applied before DEI */
            deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TRUE;
        }
        else if (enableOsdAlgLink & (!osdFormat))
        {
            /* If osdFormat is 420sp then tiler must be disabled because */
            /* swosd will be applied after DEI */
            deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TRUE;
        }
        else
        {
            /* If osd is not enabled then by default enable the tiler mode */
            deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]  = TRUE;//FALSE;
        }
        deiPrm[i].comprEnable                                   = FALSE;
        deiPrm[i].setVipScYuv422Format                          = FALSE;

        mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX].numInQue                     = numSubChains;
        mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = gVcapModuleContext.deiId[i];
        mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;

        /* If SWOSD is to be applied for 420sp data, link the DSP after Merge
           and before dup */
        if(enableOsdAlgLink & (!osdFormat))
        {

            mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX].outQueParams.nextLink = gVcapModuleContext.ipcFramesOutVpssId[0];
            mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX].notifyNextLink        = TRUE;

            ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkId = mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX];
            ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
            ipcFramesOutVpssPrm[0].baseCreateParams.notifyPrevLink = TRUE;

            ipcFramesOutVpssPrm[0].baseCreateParams.numOutQue = 1;
            ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[0].nextLink = dupId[D1_DUP_LINK_IDX];
            ipcFramesOutVpssPrm[0].baseCreateParams.notifyNextLink = TRUE;

            ipcFramesOutVpssPrm[0].baseCreateParams.processLink = gVcapModuleContext.ipcFramesInDspId[0];
            ipcFramesOutVpssPrm[0].baseCreateParams.notifyProcessLink = TRUE;
            ipcFramesOutVpssPrm[0].baseCreateParams.noNotifyMode = FALSE;

            //prevLink->processLink->nextLink
            ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[0];
            ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
            ipcFramesInDspPrm[0].baseCreateParams.numOutQue   = 1;
            ipcFramesInDspPrm[0].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[0];
            ipcFramesInDspPrm[0].baseCreateParams.notifyPrevLink = TRUE;
            ipcFramesInDspPrm[0].baseCreateParams.notifyNextLink = TRUE;
            ipcFramesInDspPrm[0].baseCreateParams.noNotifyMode   = FALSE;

            dspAlgPrm[0].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[0];
            dspAlgPrm[0].inQueParams.prevLinkQueId = 0;
        }
        else
        {
            mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX].outQueParams.nextLink        = dupId[D1_DUP_LINK_IDX];
            mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX].notifyNextLink               = TRUE;
        }

        if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
        {
            mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].numInQue                      = numSubChains;
            mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].inQueParams[i].prevLinkId     = gVcapModuleContext.deiId[i];
#ifdef USE_DEI_SC_FOR_SECONDARY
            mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId  = DEI_LINK_OUT_QUE_DEI_SC;
            mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].outQueParams.nextLink         = gVcapModuleContext.nsfId[0];
#else
            mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId  = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
            mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].outQueParams.nextLink         = dupId[CIF_DUP_LINK_IDX];
#endif
            mergePrm[DEI_SC_CIF_MERGE_LINK_IDX].notifyNextLink                = TRUE;
        }

#ifdef USE_DEI_SC_FOR_SECONDARY
    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        NsfLink_CreateParams_Init(&nsfPrm);
        nsfPrm.bypassNsf                        = TRUE;
        nsfPrm.tilerEnable                      = FALSE;
        nsfPrm.inQueParams.prevLinkId           =mergeId[DEI_SC_CIF_MERGE_LINK_IDX];
        nsfPrm.inQueParams.prevLinkQueId        = 0;
        nsfPrm.numOutQue                        = 1;
        nsfPrm.outQueParams[0].nextLink         = dupId[CIF_DUP_LINK_IDX];
        nsfPrm.numBufsPerCh                     = 6;
    }
#endif

    }

    if(enableOsdAlgLink & (!osdFormat))
    {
        dupPrm[D1_DUP_LINK_IDX].inQueParams.prevLinkId         = gVcapModuleContext.ipcFramesOutVpssId[0];
        dupPrm[D1_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    }
    else
    {
        dupPrm[D1_DUP_LINK_IDX].inQueParams.prevLinkId         = mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX];
        dupPrm[D1_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    }

    if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
    {
#ifdef USE_DEI_SC_FOR_SECONDARY
        dupPrm[D1_DUP_LINK_IDX].numOutQue                      = 3;
#else
        dupPrm[D1_DUP_LINK_IDX].numOutQue                      = 2;
#endif
    }
    else
    {
#ifdef USE_DEI_SC_FOR_SECONDARY
        dupPrm[D1_DUP_LINK_IDX].numOutQue                      = 2;
#else
        dupPrm[D1_DUP_LINK_IDX].numOutQue                      = 1;
#endif
    }

#ifdef USE_DEI_SC_FOR_SECONDARY
        if (i == 0)
        {
            dupPrm[D1_DUP_LINK_IDX].outQueParams[i].nextLink     = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
        }
        else
        {
            if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
                dupPrm[D1_DUP_LINK_IDX].outQueParams[i].nextLink     = mergeId[D1_CIF_MERGE_LINK_IDX];
            else
                dupPrm[D1_DUP_LINK_IDX].outQueParams[i].nextLink     = ipcOutVpssId;
            if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
            {
                dupPrm[D1_DUP_LINK_IDX].outQueParams[i+1].nextLink     = mergeId[D1_CIF_MERGE_LINK_IDX];
            }
         }
#else
    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
        dupPrm[D1_DUP_LINK_IDX].outQueParams[0].nextLink     = mergeId[D1_CIF_MERGE_LINK_IDX];
    else
        dupPrm[D1_DUP_LINK_IDX].outQueParams[0].nextLink     = ipcOutVpssId;
    if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
    {
        dupPrm[D1_DUP_LINK_IDX].outQueParams[1].nextLink     = mergeId[D1_CIF_MERGE_LINK_IDX];
    }
#endif
    dupPrm[D1_DUP_LINK_IDX].notifyNextLink                 = TRUE;
    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        dupPrm[CIF_DUP_LINK_IDX].numOutQue = 2;

#ifdef USE_DEI_SC_FOR_SECONDARY
        dupPrm[CIF_DUP_LINK_IDX].inQueParams.prevLinkId =gVcapModuleContext.nsfId[0];
        dupPrm[CIF_DUP_LINK_IDX].inQueParams.prevLinkQueId = 0;
#else
        dupPrm[CIF_DUP_LINK_IDX].inQueParams.prevLinkId = mergeId[DEI_SC_CIF_MERGE_LINK_IDX];
        dupPrm[CIF_DUP_LINK_IDX].inQueParams.prevLinkQueId = 0;
#endif
        dupPrm[CIF_DUP_LINK_IDX].outQueParams[0].nextLink = mergeId[D1_CIF_MERGE_LINK_IDX];
        dupPrm[CIF_DUP_LINK_IDX].outQueParams[1].nextLink = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
        dupPrm[CIF_DUP_LINK_IDX].notifyNextLink = TRUE;


        if(enableScdAlgLink)
        {
#ifndef USE_SCLR_FOR_SCD
            dupPrm[CIF_DUP_LINK_IDX].numOutQue = 3;
            dupPrm[CIF_DUP_LINK_IDX].outQueParams[2].nextLink  = gVcapModuleContext.ipcFramesOutVpssId[1];

            ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkId = dupId[CIF_DUP_LINK_IDX];
            ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 2;

            ipcFramesOutVpssPrm[1].baseCreateParams.inputFrameRate = 30;
            ipcFramesOutVpssPrm[1].baseCreateParams.outputFrameRate = 2;
#else
            dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].numOutQue = 3;
            dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX].outQueParams[2].nextLink = gVcapModuleContext.sclrId[0];

            SclrLink_CreateParams_Init(&sclrPrm);
            sclrPrm.inQueParams.prevLinkId             = dupId[CAPTURE_DISPLAY_DUP_LINK_IDX];
            sclrPrm.inQueParams.prevLinkQueId          = 2;
            sclrPrm.outQueParams.nextLink              = gVcapModuleContext.nsfId[0];
            sclrPrm.tilerEnable                        = FALSE;
            sclrPrm.enableLineSkipSc                   = TRUE;//FALSE;
            sclrPrm.inputFrameRate                     = 30;
            sclrPrm.outputFrameRate                    = 2;

            sclrPrm.scaleMode                                    = DEI_SCALE_MODE_RATIO;
            sclrPrm.outScaleFactor.ratio.widthRatio.numerator    = 1;
            sclrPrm.outScaleFactor.ratio.widthRatio.denominator  = 2;
            sclrPrm.outScaleFactor.ratio.heightRatio.numerator   = 1;
            sclrPrm.outScaleFactor.ratio.heightRatio.denominator = 1;
            sclrPrm.numBufsPerCh                                 = 2;

            NsfLink_CreateParams_Init(&nsfParam);
            nsfParam.bypassNsf       = TRUE;
            nsfParam.inputFrameRate  = 1;
            nsfParam.outputFrameRate = 1;
            nsfParam.tilerEnable     = FALSE;
            nsfParam.inQueParams.prevLinkId    = gVcapModuleContext.sclrId[0];
            nsfParam.inQueParams.prevLinkQueId = 0;
            nsfParam.numOutQue                 = 1;
            nsfParam.outQueParams[0].nextLink  = gVcapModuleContext.ipcFramesOutVpssId[1];
            nsfParam.numBufsPerCh              = 2;

            ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkId    = gVcapModuleContext.nsfId[0];
            ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;

            ipcFramesOutVpssPrm[1].baseCreateParams.inputFrameRate  = 30;
            ipcFramesOutVpssPrm[1].baseCreateParams.outputFrameRate = 30;
#endif

            ipcFramesOutVpssPrm[1].baseCreateParams.notifyPrevLink = TRUE;

            ipcFramesOutVpssPrm[1].baseCreateParams.numOutQue = 1;
            ipcFramesOutVpssPrm[1].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.ipcFramesInDspId[1];//SYSTEM_LINK_ID_INVALID;
            ipcFramesOutVpssPrm[1].baseCreateParams.notifyNextLink = TRUE;

            ipcFramesOutVpssPrm[1].baseCreateParams.processLink = SYSTEM_LINK_ID_INVALID;//gVcapModuleContext.ipcFramesInDspId[1];
            ipcFramesOutVpssPrm[1].baseCreateParams.notifyProcessLink = FALSE;//TRUE;
            ipcFramesOutVpssPrm[1].baseCreateParams.noNotifyMode = FALSE;

            ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkId = gVcapModuleContext.ipcFramesOutVpssId[1];
            ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
            ipcFramesInDspPrm[1].baseCreateParams.numOutQue   = 1;
            ipcFramesInDspPrm[1].baseCreateParams.outQueParams[0].nextLink = gVcapModuleContext.dspAlgId[1];
            ipcFramesInDspPrm[1].baseCreateParams.notifyPrevLink = TRUE;
            ipcFramesInDspPrm[1].baseCreateParams.notifyNextLink = TRUE;
            ipcFramesInDspPrm[1].baseCreateParams.noNotifyMode   = FALSE;

            dspAlgPrm[1].inQueParams.prevLinkId = gVcapModuleContext.ipcFramesInDspId[1];
            dspAlgPrm[1].inQueParams.prevLinkQueId = 0;
        }

        if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
        {
               mergePrm[D1_CIF_MERGE_LINK_IDX].numInQue                        = 3;
        }
        else
        {
            mergePrm[D1_CIF_MERGE_LINK_IDX].numInQue                        = numSubChains;
        }
        mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[0].prevLinkId       = dupId[D1_DUP_LINK_IDX];
#ifdef USE_DEI_SC_FOR_SECONDARY
        mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId    = 1;
#else
        mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId    = 0;
#endif
        mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[1].prevLinkId       = dupId[CIF_DUP_LINK_IDX];
        mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId    = 0;

        if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
        {
            mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[2].prevLinkId       = dupId[D1_DUP_LINK_IDX];
#ifdef USE_DEI_SC_FOR_SECONDARY
            mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId    = 2;
#else
            mergePrm[D1_CIF_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId    = 1;
#endif
        }
        mergePrm[D1_CIF_MERGE_LINK_IDX].outQueParams.nextLink           = ipcOutVpssId;
        mergePrm[D1_CIF_MERGE_LINK_IDX].notifyNextLink                  = TRUE;
    }

    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        ipcOutVpssPrm.inQueParams.prevLinkId    = mergeId[D1_CIF_MERGE_LINK_IDX];
        ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    }
    else
    {
        ipcOutVpssPrm.inQueParams.prevLinkId    = dupId[D1_DUP_LINK_IDX];
#ifdef USE_DEI_SC_FOR_SECONDARY
        ipcOutVpssPrm.inQueParams.prevLinkQueId = 1;
#else
        ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
#endif
    }
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = TRUE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.outQueParams[0].nextLink  = gVencModuleContext.encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    encPrm.numBufPerCh[0] = 6; //D1
    encPrm.numBufPerCh[1] = 6; //CIF
    /* available buffers per channel with CIF and MJPEG encoder support is less*/
    if (gVsysModuleContext.vsysConfig.enableMjpegEnc == TRUE)
    {
        encPrm.numBufPerCh[0] = 6;
        encPrm.numBufPerCh[1] = 6;
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
            pLinkChPrm->enableHighSpeed         = FALSE;
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
                pLinkChPrm->enableHighSpeed         = TRUE;
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
                pLinkDynPrm->inputFrameRate        = pDynPrm->inputFrameRate;
                pLinkDynPrm->qpMin                 = 0;
                pLinkDynPrm->qpMax                 = 0;
                pLinkDynPrm->qpInit                = -1;
                pLinkDynPrm->vbrDuration           = 0;
                pLinkDynPrm->vbrSensitivity        = 0;
            }
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
        dspAlgPrm[0].enableOSDAlg = TRUE;
        dspAlgPrm[0].enableSCDAlg = FALSE;
        dspAlgPrm[0].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink = SYSTEM_LINK_ID_INVALID;


        for(chId = 0; chId < ALG_LINK_OSD_MAX_CH; chId++)
        {
            AlgLink_OsdChWinParams * chWinPrm = &dspAlgPrm[0].osdChCreateParams[chId].chDefaultParams;

            /* set osd window max width and height */
            dspAlgPrm[0].osdChCreateParams[chId].maxWidth  = EXAMPLE_OSD_WIN_MAX_WIDTH;
            dspAlgPrm[0].osdChCreateParams[chId].maxHeight = EXAMPLE_OSD_WIN_MAX_HEIGHT;

            chWinPrm->chId = chId;
            chWinPrm->numWindows = 0;
        }
    }

    if (enableScdAlgLink)
    {
        Int32   numBlksInFrame;
        Int32   numHorzBlks, numVertBlks, chIdx;
        Uint32  x, y, i;

        dspAlgPrm[1].enableOSDAlg = FALSE;
        dspAlgPrm[1].enableSCDAlg = TRUE;
        dspAlgPrm[1].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = ipcBitsOutDSPId;

        dspAlgPrm[1].scdCreateParams.maxWidth               = 352;
        if(Vcap_isPalMode())
           dspAlgPrm[1].scdCreateParams.maxHeight           = 288;
        else
           dspAlgPrm[1].scdCreateParams.maxHeight           = 240;
        dspAlgPrm[1].scdCreateParams.maxStride              = 352;
        dspAlgPrm[1].scdCreateParams.numValidChForSCD       = 16;

        dspAlgPrm[1].scdCreateParams.numSecs2WaitB4Init     = 3;
        dspAlgPrm[1].scdCreateParams.numSecs2WaitB4FrmAlert = 1;
        dspAlgPrm[1].scdCreateParams.inputFrameRate         = 2;
        dspAlgPrm[1].scdCreateParams.outputFrameRate        = 2;
        dspAlgPrm[1].scdCreateParams.numSecs2WaitAfterFrmAlert = 2;
        dspAlgPrm[1].scdCreateParams.numBufPerCh               = 2;

        dspAlgPrm[1].scdCreateParams.enableMotionNotify    = FALSE;
        dspAlgPrm[1].scdCreateParams.enableTamperNotify    = FALSE;

       // Configure array to monitor scene changes in all frame blocks, i.e., motion detection.
       // Each block is fixed to be 32x10 in size when height is 240,
       // Each block is fixed to be 32x11 in size when height is 288
        numHorzBlks    = dspAlgPrm[1].scdCreateParams.maxWidth / 32;
        if((dspAlgPrm[1].scdCreateParams.maxHeight%10) == 0)
           numVertBlks    = dspAlgPrm[1].scdCreateParams.maxHeight / 10;
        else   /* For 288 Block height becomes 12 */
           numVertBlks    = dspAlgPrm[1].scdCreateParams.maxHeight / 12;

        numBlksInFrame = numHorzBlks * numVertBlks;

        for(chIdx = 0; chIdx < dspAlgPrm[1].scdCreateParams.numValidChForSCD; chIdx++)
        {
           AlgLink_ScdChParams * chPrm = &dspAlgPrm[1].scdCreateParams.chDefaultParams[chIdx];

           chPrm->blkNumBlksInFrame = numBlksInFrame;
           chPrm->chId               = SCDChannelMonitor[chIdx];
           chPrm->mode               = ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
           chPrm->frmIgnoreLightsON  = FALSE;
           chPrm->frmIgnoreLightsOFF = FALSE;
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

        ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkId    = gVcapModuleContext.dspAlgId[1];
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
        dspAlgPrm[1].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;
    }


#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif
    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));
    System_linkCreate (dupId[CAPTURE_DISPLAY_DUP_LINK_IDX], &dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX], sizeof(dupPrm[CAPTURE_DISPLAY_DUP_LINK_IDX]));

    if(enableOsdAlgLink & osdFormat)
    {
        System_linkCreate(gVcapModuleContext.ipcFramesOutVpssId[0], &ipcFramesOutVpssPrm[0], sizeof(ipcFramesOutVpssPrm[0]));
        System_linkCreate(gVcapModuleContext.ipcFramesInDspId[0], &ipcFramesInDspPrm[0], sizeof(ipcFramesInDspPrm[0]));
        System_linkCreate(gVcapModuleContext.dspAlgId[0] , &dspAlgPrm[0], sizeof(dspAlgPrm[0]));
    }
    else
    {
        System_linkCreate (selectId, &selectPrm, sizeof(selectPrm));
    }


    for(i=0; i<numSubChains; i++)
        System_linkCreate(gVcapModuleContext.deiId[i]  , &deiPrm[i], sizeof(deiPrm[i]));

    System_linkCreate(mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX], &mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX], sizeof(mergePrm[DEI_VIP_SC_D1_MERGE_LINK_IDX]));

    if(enableOsdAlgLink & !osdFormat)
    {
        System_linkCreate(gVcapModuleContext.ipcFramesOutVpssId[0], &ipcFramesOutVpssPrm[0], sizeof(ipcFramesOutVpssPrm[0]));
        System_linkCreate(gVcapModuleContext.ipcFramesInDspId[0], &ipcFramesInDspPrm[0], sizeof(ipcFramesInDspPrm[0]));
        System_linkCreate(gVcapModuleContext.dspAlgId[0] , &dspAlgPrm[0], sizeof(dspAlgPrm[0]));
    }
    System_linkCreate(dupId[D1_DUP_LINK_IDX], &dupPrm[D1_DUP_LINK_IDX], sizeof(dupPrm[D1_DUP_LINK_IDX]));

    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        System_linkCreate(mergeId[DEI_SC_CIF_MERGE_LINK_IDX], &mergePrm[DEI_SC_CIF_MERGE_LINK_IDX], sizeof(mergePrm[DEI_SC_CIF_MERGE_LINK_IDX]));
#ifdef USE_DEI_SC_FOR_SECONDARY
        System_linkCreate(gVcapModuleContext.nsfId[0],&nsfPrm,sizeof(nsfPrm));
#endif
        System_linkCreate(dupId[CIF_DUP_LINK_IDX], &dupPrm[CIF_DUP_LINK_IDX], sizeof(dupPrm[CIF_DUP_LINK_IDX]));

        if(enableScdAlgLink)
        {
#ifdef USE_SCLR_FOR_SCD
            System_linkCreate(gVcapModuleContext.sclrId[0], &sclrPrm, sizeof(sclrPrm));

            System_linkCreate(gVcapModuleContext.nsfId[0], &nsfParam, sizeof(nsfParam));
#endif
            System_linkCreate(gVcapModuleContext.ipcFramesOutVpssId[1], &ipcFramesOutVpssPrm[1], sizeof(ipcFramesOutVpssPrm[1]));
            System_linkCreate(gVcapModuleContext.ipcFramesInDspId[1], &ipcFramesInDspPrm[1], sizeof(ipcFramesInDspPrm[1]));
            System_linkCreate(gVcapModuleContext.dspAlgId[1] , &dspAlgPrm[1], sizeof(dspAlgPrm[1]));
            System_linkCreate(ipcBitsOutDSPId, &ipcBitsOutDspPrm, sizeof(ipcBitsOutDspPrm));
            System_linkCreate(gVcapModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm[1], sizeof(ipcBitsInHostPrm[1]));
        }
        System_linkCreate(mergeId[D1_CIF_MERGE_LINK_IDX], &mergePrm[D1_CIF_MERGE_LINK_IDX], sizeof(mergePrm[D1_CIF_MERGE_LINK_IDX]));
    }
    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );

    System_linkCreate(gVencModuleContext.encId, &encPrm, sizeof(encPrm));

    System_linkCreate(gVencModuleContext.ipcBitsOutRTOSId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(gVencModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm[0], sizeof(ipcBitsInHostPrm[0]));

    System_linkGetInfo(gVencModuleContext.ipcBitsInHLOSId,&bitsProducerLinkInfo);
    OSA_assert(bitsProducerLinkInfo.numQue = 1);
    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.ipcBitsInRTOSId;

    if (bitsProducerLinkInfo.queInfo[0].numCh > gVencModuleContext.vencConfig.numPrimaryChn)
        bitsProducerLinkInfo.queInfo[0].numCh = gVencModuleContext.vencConfig.numPrimaryChn;

    printf ("Reducing bitsProducerLinkInfo.numCh to %d\n", bitsProducerLinkInfo.queInfo[0].numCh);

    MultiCh_ipcBitsInitCreateParams_BitsOutHLOS(&ipcBitsOutHostPrm,
                                               &bitsProducerLinkInfo.queInfo[0]);
    ipcBitsOutHostPrm.numBufPerCh[0] = 8;
    ipcBitsOutHostPrm.numBufPerCh[1] = 8;
    ipcBitsOutHostPrm.numBufPerCh[2] = 8;
    ipcBitsOutHostPrm.numBufPerCh[3] = 8;
    ipcBitsOutHostPrm.numBufPerCh[4] = 8;

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
    }
    decPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsInRTOSId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink  = ipcOutVideoId;
    decPrm.tilerEnable = TRUE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                 = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink  = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = FALSE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = TRUE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink  = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = TRUE;

    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
#ifdef USE_DEI_SC_FOR_SECONDARY
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                     = 3;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = dupId[D1_DUP_LINK_IDX];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = 0;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = ipcInVpssId;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = 0;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[2].prevLinkId    = dupId[CIF_DUP_LINK_IDX];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId = 1;
#else
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                     = 4;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = gVcapModuleContext.deiId[0];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = gVcapModuleContext.deiId[1];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[2].prevLinkId    = ipcInVpssId;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId = 0;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[3].prevLinkId    = dupId[CIF_DUP_LINK_IDX];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[3].prevLinkQueId = 1;
#endif
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].outQueParams.nextLink        = dupId[LIVE_DECODE_DUP_LINK_IDX];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].notifyNextLink               = TRUE;
    }
    else
    {
#ifdef USE_DEI_SC_FOR_SECONDARY
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                     = 2;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = dupId[D1_DUP_LINK_IDX];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = 0;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = ipcInVpssId;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = 0;
#else
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                     = 3;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = gVcapModuleContext.deiId[0];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = gVcapModuleContext.deiId[1];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[2].prevLinkId    = ipcInVpssId;
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId = 0;
#endif
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].outQueParams.nextLink        = dupId[LIVE_DECODE_DUP_LINK_IDX];
        mergePrm[LIVE_DECODE_MERGE_LINK_IDX].notifyNextLink               = TRUE;
    }
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkId         = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].numOutQue                      = 2;
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[0].nextLink       = gVdisModuleContext.swMsId[0];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[1].nextLink       = gVdisModuleContext.swMsId[1];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].notifyNextLink                 = TRUE;

    for(i=0; i<2; i++)
    {
        VDIS_DEV vdDevId = VDIS_DEV_HDMI;
                
        swMsPrm[i].inQueParams.prevLinkId    = dupId[LIVE_DECODE_DUP_LINK_IDX];
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = gVdisModuleContext.displayId[i];
        swMsPrm[i].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN + 4;
        swMsPrm[i].maxOutRes                 = VSYS_STD_1080P_60;
        swMsPrm[i].initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[i].resolution;
        swMsPrm[i].lineSkipMode = FALSE;
        swMsPrm[i].enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(i, &swMsPrm[i], FALSE);
        if(i == 0)
        {
           vdDevId = VDIS_DEV_HDMI;
           swMsPrm[i].enableOuputDup = TRUE;
           swMsPrm[i].enableProcessTieWithDisplay = TRUE;
        }
        else if(i == 1)
        {
           vdDevId = VDIS_DEV_DVO2;
           swMsPrm[i].enableOuputDup = TRUE;
           swMsPrm[i].enableProcessTieWithDisplay = TRUE;
        }
        mulich_progressive_set_avsync_prm(&avsyncCfg[i],i,vdDevId);

        displayPrm[i].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                   = swMsPrm[i].initOutRes;
    }

    if(enableSdtv == TRUE)
    {
        displayPrm[2].numInputQueues = 1;
        displayPrm[2].activeQueue    = 0;
        displayPrm[2].displayRes     = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;

        displayPrm[2].inQueParams[0].prevLinkId = dupId[CAPTURE_DISPLAY_DUP_LINK_IDX];
        if(enableOsdAlgLink & osdFormat)
        {
            displayPrm[2].inQueParams[0].prevLinkQueId = 1;
        }
        else
        {
            displayPrm[2].inQueParams[0].prevLinkQueId = 1;
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

    for(i=0; i<2; i++)
        System_linkCreate(gVdisModuleContext.displayId[i], &displayPrm[i], sizeof(displayPrm[i]));

    if(enableSdtv)
    {
        System_linkCreate(gVdisModuleContext.displayId[2], &displayPrm[2], sizeof(displayPrm[2]));
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
        MultiCh_setDec2DispMap(VDIS_DEV_HDCOMP,gVdecModuleContext.vdecConfig.numChn,0,inChInfo.startChNum);
   }


}

Void MultiCh_deleteProgressiveVcapVencVdecVdis()
{
    UInt32 i;
    UInt32 numSubChains;
    Bool enableOsdAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool enableScdAlgLink = gVsysModuleContext.vsysConfig.enableScd;
    Bool osdFormat        = gVsysModuleContext.vsysConfig.osdFormat; // 0 for 420SP and 1 for 422i

    Bool   enableSdtv  = TRUE;
    UInt32 grpxId[VDIS_DEV_MAX];

    UInt32 selectId;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    UInt32 ipcBitsOutDSPId;

    ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
    /* Disabling SDTV temporarily to enable ratio based resolution setting in DEI; should be fixed */
    enableSdtv = TRUE;

    numSubChains = 2;
    mergeId[DEI_VIP_SC_D1_MERGE_LINK_IDX]  = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[LIVE_DECODE_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_1;
    if (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
    {
        mergeId[DEI_SC_CIF_MERGE_LINK_IDX]      = SYSTEM_VPSS_LINK_ID_MERGE_2;
        mergeId[D1_CIF_MERGE_LINK_IDX]          = SYSTEM_VPSS_LINK_ID_MERGE_3;
    }
    else
    {
        mergeId[DEI_SC_CIF_MERGE_LINK_IDX]      = SYSTEM_LINK_ID_INVALID;
        mergeId[D1_CIF_MERGE_LINK_IDX]          = SYSTEM_LINK_ID_INVALID;
    }
    selectId                        = SYSTEM_VPSS_LINK_ID_SELECT_0;

    dupId[D1_DUP_LINK_IDX]              = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_DUP_1;
    dupId[CIF_DUP_LINK_IDX]             = SYSTEM_VPSS_LINK_ID_DUP_2;
    dupId[CAPTURE_DISPLAY_DUP_LINK_IDX] = SYSTEM_VPSS_LINK_ID_DUP_3;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;
    grpxId[0]    = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]    = SYSTEM_LINK_ID_GRPX_1;
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
    grpxId[2]    = SYSTEM_LINK_ID_GRPX_2;
#endif

    gVdecModuleContext.ipcBitsOutHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

    System_linkDelete(gVcapModuleContext.captureId);

    if(enableOsdAlgLink)
    {
        System_linkDelete(gVcapModuleContext.ipcFramesOutVpssId[0]);
        System_linkDelete(gVcapModuleContext.ipcFramesInDspId[0]);
        System_linkDelete(gVcapModuleContext.dspAlgId[0]);
    }
    if(enableOsdAlgLink & !osdFormat)
    {
        System_linkDelete(selectId);
    }
    if(enableScdAlgLink)
    {
#ifdef USE_SCLR_FOR_SCD
        System_linkDelete(gVcapModuleContext.sclrId[0]);
        System_linkDelete(gVcapModuleContext.nsfId[0]);
#endif
        System_linkDelete(gVcapModuleContext.ipcFramesOutVpssId[1]);
        System_linkDelete(gVcapModuleContext.ipcFramesInDspId[1]);
        System_linkDelete(gVcapModuleContext.dspAlgId[1]);
        System_linkDelete(ipcBitsOutDSPId);
        System_linkDelete(gVcapModuleContext.ipcBitsInHLOSId);
    }

    for(i=0; i<numSubChains; i++)
        System_linkDelete(gVcapModuleContext.deiId[i]);

#ifdef USE_DEI_SC_FOR_SECONDARY
    System_linkDelete(gVcapModuleContext.nsfId[0]);
#endif

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

    for(i=0; i<numSubChains; i++)
        System_linkDelete(gVdisModuleContext.swMsId[i] );

    for(i=0; i<numSubChains; i++)
        System_linkDelete(gVdisModuleContext.displayId[i]);

    if(enableSdtv)
    {
        System_linkDelete(gVdisModuleContext.displayId[2]);
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



/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/*  ========================= TI_816X_BUILD case ==============================
                          |
                        IPC_BITS_OUT_A8 (BitStream)
                          |
                        IPC_BITS_IN (Video)
                          |
                        DEC (YUV420SP)
                          |
                        IPC_OUT (Video)
                          |
                        IPC_IN (Vpss)
                          |   (32 D1 + 10 720P + 6 1080P)
                        MP_SCLR (Vpss)
                          |     (TI816x only)
                         DUP
               (48CH)    | |    (48CH)
         +---------------+ +-------------+
         |                               |
         |                               |
      SW Mosaic                       SW Mosaic
     (SC5 SC1 YUV422I)                (SC4 SC2 YUV422I)
        |  |                             |  |
        |  |                             |  |
       (DDR)(422I)                      (DDR)(422I)
          |                               |
  GRPX0   |                      GRPX1    |
     |    |                          |    |
     On-Chip HDMI                    Off-Chip HDMI
       1080p60                        1080p60
*/

/* ========================= TI_814X_BUILD case ==============================
                          |
                        IPC_BITS_OUT_A8 (BitStream)
                          |
                        IPC_BITS_IN (Video)
                          |
                        DEC (YUV420SP)
                          |
                        IPC_OUT (Video)
                          |
                        IPC_IN (Vpss)
                          |   (16 channels)
                         DUP
               (16CH)    | |    (16CH)
         +---------------+ +-------------+
         |                               |
         |                               |
      SW Mosaic                       SW Mosaic
     (SC5 YUV422I)                (SC5 YUV422I)
        |  |                             |  |
        |  |                             |  |
       (DDR)(422I)                      (DDR)(422I)
          |                               |
  GRPX0   |----Tied---|                   |
     |    |           |                   |
     On-Chip HDMI  Off-Chip HDMI         SDTV
       1080p60       1080p60
*/

#include "multich_common.h"
#include "multich_ipcbits.h"
#include "mcfw/interfaces/link_api/system_tiler.h"
#include "mcfw/interfaces/link_api/avsync_hlos.h"

/* =============================================================================
 * Externs
 * =============================================================================
 */


/* =============================================================================
 * Use case code
 * =============================================================================
 */



#define MAX_DEC_OUT_FRAMES_PER_CH                           (5)
#define MULTICH_VDEC_VDIS_IPCFRAMEEXPORT_NUM_CHANNELS       (16)
#define MULTICH_VDEC_VDIS_IPCFRAMEEXPORT_FRAME_WIDTH        (720)
#define MULTICH_VDEC_VDIS_IPCFRAMEEXPORT_FRAME_HEIGHT       (480)
#define MULTICH_NUM_SWMS_MAX_BUFFERS                        (7)



#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 0,
        .EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0},

        .DecNumCh  = 16,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    },
};
#else
/*
Max channels per IVA-HD MUST be < 16,
i.e total number of channels for decode can be 48.

Example CH allocation for SD+HD,

32CH SD    :  0 to 31
 6CH 1080P : 32 to 38
10CH 720P  : 39 to 48

*/
static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 0,
        .EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0},

        .DecNumCh  = 17,
        .DecChList = {   0,  3,  6,  9, 12, 15, 18, 21,
                        24, 27, 30, 33, 36, 39, 42, 45,
                        48
                     },
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 0,
        .EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0},

        .DecNumCh  = 17,
        .DecChList = {   1,  4,  7, 10, 13, 16, 19, 22,
                        25, 28, 31, 34, 37, 40, 43, 46,
                        49
                     },
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 0,
        .EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0},

        .DecNumCh  = 16,
        .DecChList = {   2,  5,  8, 11, 14, 17, 20, 23,
                        26, 29, 32, 35, 38, 41, 44, 47
                     },
    },
};
#endif

typedef struct {

    UInt32 dupId;
    UInt32 ipcOutVideoId;
    UInt32 ipcInVpssId;
    UInt32 mergeId;
    Bool   enableVideoFrameExport;
    AvsyncLink_LinkSynchConfigParams   avsyncCfg[VDIS_DEV_MAX];
    UInt32 mpSclrId;

} MultiCh_VdecVdisObj;

MultiCh_VdecVdisObj gMultiCh_VdecVdisObj;

static Void MultiCh_setIpcFramesOutInQueInfo(System_LinkQueInfo *inQueInfo)
{
    Int i;

    inQueInfo->numCh = MULTICH_VDEC_VDIS_IPCFRAMEEXPORT_NUM_CHANNELS;
    for (i = 0; i < inQueInfo->numCh; i++)
    {
        inQueInfo->chInfo[i].bufType = SYSTEM_BUF_TYPE_VIDFRAME;
        inQueInfo->chInfo[i].dataFormat = SYSTEM_DF_YUV422I_YUYV;
        inQueInfo->chInfo[i].memType    = SYSTEM_MT_NONTILEDMEM;
        inQueInfo->chInfo[i].scanFormat = SYSTEM_SF_PROGRESSIVE;
        inQueInfo->chInfo[i].startX     = 0;
        inQueInfo->chInfo[i].startY     = 0;
        inQueInfo->chInfo[i].width      =
                   MULTICH_VDEC_VDIS_IPCFRAMEEXPORT_FRAME_WIDTH;
        inQueInfo->chInfo[i].height     =
                   MULTICH_VDEC_VDIS_IPCFRAMEEXPORT_FRAME_HEIGHT;
        inQueInfo->chInfo[i].pitch[0]   =
                   MULTICH_VDEC_VDIS_IPCFRAMEEXPORT_FRAME_WIDTH * 2;
        inQueInfo->chInfo[i].pitch[1]   = 0;
        inQueInfo->chInfo[i].pitch[2]   = 0;
    }
}

static
Void mulich_vdec_vdis_set_avsync_vidque_prm(Avsync_SynchConfigParams *queCfg,
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
        (gVsysModuleContext.vsysConfig.enableAVsync))
    {
        queCfg->avsyncEnable = TRUE;
    }
    else
    {
        queCfg->avsyncEnable = FALSE;
    }
    queCfg->clkAdjustPolicy.refClkType = AVSYNC_REFCLKADJUST_NONE;
    queCfg->clkAdjustPolicy.clkAdjustLead = AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LEAD_MS;
    queCfg->clkAdjustPolicy.clkAdjustLag  = AVSYNC_VIDEO_TIMEBASESHIFT_MAX_LAG_MS;
    queCfg->vidSynchPolicy.maxReplayLead  = AVSYNC_VIDEO_FUTURE_FRAME_DROP_THRESHOLD_MS;
    queCfg->vidSynchPolicy.playMaxLag  = 180;
    queCfg->vidSynchPolicy.playMaxLead = 0;
    queCfg->vidSynchPolicy.doMarginScaling = FALSE;
    queCfg->playTimerStartTimeout = 0;
    queCfg->playStartMode = AVSYNC_PLAYBACK_START_MODE_WAITSYNCH;
    queCfg->ptsInitMode   = AVSYNC_PTS_INIT_MODE_APP;
}

static
Void mulich_vdec_vdis_set_avsync_prm(AvsyncLink_LinkSynchConfigParams *avsyncPrm,
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
        mulich_vdec_vdis_set_avsync_vidque_prm(&avsyncPrm->queCfg[i],
                                               i,
                                               0,
                                               (0 + gVdecModuleContext.vdecConfig.numChn),
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

Void MultiCh_createVdecVdis()
{
    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    DecLink_CreateParams        decPrm;
    IpcLink_CreateParams        ipcOutVideoPrm;
    IpcLink_CreateParams        ipcInVpssPrm;
    DupLink_CreateParams        dupPrm;
    static SwMsLink_CreateParams       swMsPrm[VDIS_DEV_MAX];
    DisplayLink_CreateParams    displayPrm[VDIS_DEV_MAX];
    IpcFramesOutLinkHLOS_CreateParams  ipcFramesOutHostPrm;
    IpcFramesInLinkRTOS_CreateParams   ipcFramesInVpssFromHostPrm;
    MergeLink_CreateParams             mergePrm;

#ifdef TI_816X_BUILD
    MpSclrLink_CreateParams            mpSclrPrm;
#endif/* TI_816X_BUILD */

    UInt32 i;
    UInt32 enableGrpx;
    Bool tilerEnable;
    Bool enableVideoFrameExport;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(DecLink_CreateParams, decPrm);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkHLOS_CreateParams ,ipcFramesOutHostPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  ,ipcFramesInVpssFromHostPrm);
    for (i = 0; i < VDIS_DEV_MAX;i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
        MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[i]);
    }

#ifdef TI_816X_BUILD
    MULTICH_INIT_STRUCT(MpSclrLink_CreateParams, mpSclrPrm);
#endif /* TI_816X_BUILD */

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
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    tilerEnable  = FALSE;
    enableGrpx   = FALSE;
#else
    tilerEnable  = FALSE;
    enableGrpx   = TRUE;
#endif

#if (tilerEnable == FALSE)
    {
        /* Disable tiler allocator for this usecase
         * for that tiler memory can be reused for
         * non-tiled allocation
         */
        SystemTiler_disableAllocator();
    }
#endif

    enableVideoFrameExport = FALSE;

    gVdecModuleContext.ipcBitsOutHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.decId            = SYSTEM_LINK_ID_VDEC_0;

    gMultiCh_VdecVdisObj.ipcOutVideoId  = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    gMultiCh_VdecVdisObj.ipcInVpssId    = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;
    gMultiCh_VdecVdisObj.dupId          = SYSTEM_VPSS_LINK_ID_DUP_0;

#ifdef TI_816X_BUILD
    gMultiCh_VdecVdisObj.mpSclrId       = SYSTEM_LINK_ID_MP_SCLR_INST_0;
    gVdisModuleContext.mpSclrId         = SYSTEM_LINK_ID_MP_SCLR_INST_0;
#endif /* TI_816X_BUILD */

    gVdisModuleContext.swMsId[0]        = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;

    gVdisModuleContext.displayId[0]     = SYSTEM_LINK_ID_DISPLAY_0; // ON AND OFF CHIP HDMI

    if (gVsysModuleContext.vsysConfig.numDisplays > 1)
    {
        gVdisModuleContext.swMsId[1]        = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        gVdisModuleContext.displayId[1]     = SYSTEM_LINK_ID_DISPLAY_2; // SDTV
#else
        gVdisModuleContext.displayId[1]     = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI
#endif
    }

    if (enableVideoFrameExport)
    {
        gMultiCh_VdecVdisObj.mergeId                 = SYSTEM_VPSS_LINK_ID_MERGE_0;
        gVdisModuleContext.ipcFramesOutHostId        = SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0;
        gVdisModuleContext.ipcFramesInVpssFromHostId = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_0;
    }

    if(enableGrpx)
    {
        // GRPX is enabled in Vdis_start() based on the link ID set here
        gVdisModuleContext.grpxId[0]    = SYSTEM_LINK_ID_GRPX_0;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        gVdisModuleContext.grpxId[1]    = SYSTEM_LINK_ID_INVALID;
#else
        gVdisModuleContext.grpxId[1]    = SYSTEM_LINK_ID_GRPX_1;
#endif
    }

    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink= gVdecModuleContext.ipcBitsInRTOSId;
    ipcBitsOutHostPrm.baseCreateParams.notifyNextLink       = FALSE;
    ipcBitsOutHostPrm.baseCreateParams.notifyPrevLink       = FALSE;
    ipcBitsOutHostPrm.baseCreateParams.noNotifyMode         = TRUE;
    ipcBitsOutHostPrm.baseCreateParams.numOutQue            = 1;
    ipcBitsOutHostPrm.inQueInfo.numCh                       = gVdecModuleContext.vdecConfig.numChn;
    ipcBitsOutHostPrm.bufPoolPerCh                          = TRUE;

    for (i=0; i<ipcBitsOutHostPrm.inQueInfo.numCh; i++)
    {
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].width =
            gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoWidth;

        ipcBitsOutHostPrm.inQueInfo.chInfo[i].height =
            gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoHeight;

        ipcBitsOutHostPrm.inQueInfo.chInfo[i].scanFormat =
            SYSTEM_SF_PROGRESSIVE;

        ipcBitsOutHostPrm.inQueInfo.chInfo[i].bufType        = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].codingformat   = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].dataFormat     = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].memType        = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].startX         = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].startY         = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].pitch[0]       = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].pitch[1]       = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].pitch[2]       = 0; // NOT USED
        ipcBitsOutHostPrm.numBufPerCh[i] = 6;
    }

    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsOutHLOSId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink  = gVdecModuleContext.decId;
    ipcBitsInVideoPrm.baseCreateParams.noNotifyMode              = TRUE;
    ipcBitsInVideoPrm.baseCreateParams.notifyNextLink            = TRUE;
    ipcBitsInVideoPrm.baseCreateParams.notifyPrevLink            = FALSE;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue                 = 1;

    for (i=0; i<ipcBitsOutHostPrm.inQueInfo.numCh; i++)
    {
        if(gVdecModuleContext.vdecConfig.decChannelParams[i].isCodec == VDEC_CHN_H264)
            decPrm.chCreateParams[i].format                 = IVIDEO_H264HP;
        else if(gVdecModuleContext.vdecConfig.decChannelParams[i].isCodec == VDEC_CHN_MPEG4)
            decPrm.chCreateParams[i].format                 = IVIDEO_MPEG4ASP;
        else if(gVdecModuleContext.vdecConfig.decChannelParams[i].isCodec == VDEC_CHN_MJPEG)
            decPrm.chCreateParams[i].format                 = IVIDEO_MJPEG;

        decPrm.chCreateParams[i].numBufPerCh 
                         = gVdecModuleContext.vdecConfig.decChannelParams[i].numBufPerCh;
        decPrm.chCreateParams[i].profile                = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].displayDelay
                         = gVdecModuleContext.vdecConfig.decChannelParams[i].displayDelay;
        decPrm.chCreateParams[i].dpbBufSizeInFrames = IH264VDEC_DPB_NUMFRAMES_AUTO;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = FALSE;

        decPrm.chCreateParams[i].targetMaxWidth  =
            ipcBitsOutHostPrm.inQueInfo.chInfo[i].width;

        decPrm.chCreateParams[i].targetMaxHeight =
            ipcBitsOutHostPrm.inQueInfo.chInfo[i].height;

        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate =
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.frameRate;

        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.targetBitRate;

    }

    decPrm.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsInRTOSId;
    decPrm.inQueParams.prevLinkQueId    = 0;
    decPrm.outQueParams.nextLink        = gMultiCh_VdecVdisObj.ipcOutVideoId;
    decPrm.tilerEnable                  = tilerEnable;

    ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.outQueParams[0].nextLink     = gMultiCh_VdecVdisObj.ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = TRUE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.numOutQue                 = 1;

    ipcInVpssPrm.inQueParams.prevLinkId    = gMultiCh_VdecVdisObj.ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = TRUE;
    ipcInVpssPrm.numOutQue                 = 1;

    if (enableVideoFrameExport)
    {
        ipcFramesOutHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesOutHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesOutHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutHostPrm.baseCreateParams.numOutQue = 1;
        ipcFramesOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdisModuleContext.ipcFramesInVpssFromHostId;
        MultiCh_setIpcFramesOutInQueInfo(&ipcFramesOutHostPrm.inQueInfo);

        ipcFramesInVpssFromHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkId = gVdisModuleContext.ipcFramesOutHostId;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInVpssFromHostPrm.baseCreateParams.numOutQue = 1;
        ipcFramesInVpssFromHostPrm.baseCreateParams.outQueParams[0].nextLink = gMultiCh_VdecVdisObj.mergeId;

        
        ipcInVpssPrm.outQueParams[0].nextLink     = gMultiCh_VdecVdisObj.mergeId;
        
        mergePrm.numInQue                     = 2;
        mergePrm.inQueParams[0].prevLinkId    = gMultiCh_VdecVdisObj.ipcInVpssId;
        mergePrm.inQueParams[0].prevLinkQueId = 0;
        mergePrm.inQueParams[1].prevLinkId    = gVdisModuleContext.ipcFramesInVpssFromHostId;
        mergePrm.inQueParams[1].prevLinkQueId = 0;

#ifdef TI_816X_BUILD
        mergePrm.outQueParams.nextLink        = gMultiCh_VdecVdisObj.mpSclrId;
        mergePrm.notifyNextLink               = TRUE;
        mpSclrPrm.inQueParams.prevLinkId      = gMultiCh_VdecVdisObj.mergeId;
    }
    else
    {
        mpSclrPrm.inQueParams.prevLinkId        = gMultiCh_VdecVdisObj.ipcInVpssId;
        ipcInVpssPrm.outQueParams[0].nextLink   = gMultiCh_VdecVdisObj.mpSclrId;

    }
    mpSclrPrm.pathId = MP_SCLR_LINK_SC5;
    mpSclrPrm.numCh = 2;
    mpSclrPrm.enableLineSkip = FALSE;

    mpSclrPrm.outQueParams.nextLink       = gMultiCh_VdecVdisObj.dupId;
    dupPrm.inQueParams.prevLinkId         = gMultiCh_VdecVdisObj.mpSclrId;
#else
        mergePrm.outQueParams.nextLink        = gMultiCh_VdecVdisObj.dupId;
        mergePrm.notifyNextLink               = TRUE;
        dupPrm.inQueParams.prevLinkId         = gMultiCh_VdecVdisObj.mergeId;
    }
    else
    {
        dupPrm.inQueParams.prevLinkId         = gMultiCh_VdecVdisObj.ipcInVpssId;
        ipcInVpssPrm.outQueParams[0].nextLink     = gMultiCh_VdecVdisObj.dupId;

    }

#endif /* TI_816X_BUILD */

    dupPrm.inQueParams.prevLinkQueId      = 0;
    dupPrm.numOutQue                      = gVsysModuleContext.vsysConfig.numDisplays;
    dupPrm.outQueParams[0].nextLink       = gVdisModuleContext.swMsId[0];
    dupPrm.outQueParams[1].nextLink       = gVdisModuleContext.swMsId[1];
    dupPrm.notifyNextLink                 = TRUE;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)

    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[0].swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_DEI_SC_NO_DEI;
 
    swMsPrm[0].swMsInstStartWin[0]  = 0;
    swMsPrm[0].swMsInstStartWin[1]  = 10;

    swMsPrm[1].numSwMsInst = 1;
    swMsPrm[1].swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_SC5;

    swMsPrm[1].swMsInstStartWin[0]  = 0;
    swMsPrm[1].swMsInstStartWin[1]  = 10;

    swMsPrm[0].enableProcessTieWithDisplay = TRUE;
    swMsPrm[1].enableProcessTieWithDisplay = FALSE;

#else
    swMsPrm[0].swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_VIP1_SC;
    swMsPrm[0].swMsInstId[1]        = SYSTEM_SW_MS_SC_INST_DEI_SC_NO_DEI;

    swMsPrm[1].swMsInstId[0]        = SYSTEM_SW_MS_SC_INST_DEIHQ_SC_NO_DEI;
    swMsPrm[1].swMsInstId[1]        = SYSTEM_SW_MS_SC_INST_SC5;

    swMsPrm[0].numSwMsInst          = 2;

    swMsPrm[0].swMsInstStartWin[0]  = 0;
    swMsPrm[0].swMsInstStartWin[1]  = 16;

    swMsPrm[1].numSwMsInst          = swMsPrm[0].numSwMsInst;
    swMsPrm[1].swMsInstStartWin[0]  = swMsPrm[0].swMsInstStartWin[0];
    swMsPrm[1].swMsInstStartWin[1]  = swMsPrm[0].swMsInstStartWin[1];

    swMsPrm[0].enableProcessTieWithDisplay = TRUE;
    swMsPrm[1].enableProcessTieWithDisplay = TRUE;

#endif
    for(i=0; i<gVsysModuleContext.vsysConfig.numDisplays; i++)
    {
        VDIS_DEV vdDevId = VDIS_DEV_HDMI;

        swMsPrm[i].inQueParams.prevLinkId     = gMultiCh_VdecVdisObj.dupId;
        swMsPrm[i].inQueParams.prevLinkQueId  = i;
        swMsPrm[i].outQueParams.nextLink      = gVdisModuleContext.displayId[i];
        swMsPrm[i].numOutBuf                 = MULTICH_NUM_SWMS_MAX_BUFFERS;

        /* Disable inQue drop at SwMs as input may arrive very fast in VDEC->VDIS use case */
        swMsPrm[i].maxInputQueLen             = SYSTEM_SW_MS_INVALID_INPUT_QUE_LEN;
        if (i == 0)
        {
            vdDevId = VDIS_DEV_HDMI;
            swMsPrm[i].maxOutRes              = VSYS_STD_1080P_60;
            swMsPrm[i].initOutRes             = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_HDMI].resolution;
        }
        else if (i == 1)
        {
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
            vdDevId = VDIS_DEV_SD;
            swMsPrm[i].maxOutRes              = VSYS_STD_PAL;
            swMsPrm[i].outQueParams.nextLink  = SYSTEM_LINK_ID_DISPLAY_2;
            swMsPrm[i].initOutRes             = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;

#else
            vdDevId = VDIS_DEV_DVO2;
            swMsPrm[i].maxOutRes              = VSYS_STD_1080P_60;
            swMsPrm[i].initOutRes             = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_DVO2].resolution;
#endif
        }
        /* low cost line skip mode of scaling can be used, when tiler is off */
        if(tilerEnable)
            swMsPrm[i].lineSkipMode           = FALSE;
        else
            swMsPrm[i].lineSkipMode           = TRUE;

        swMsPrm[i].enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(vdDevId, &swMsPrm[i], FALSE);    /* both from 0-16 chnl */

        displayPrm[i].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                = swMsPrm[i].initOutRes;
        if (i == 1)
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                        displayPrm[i].displayRes            = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
#else
                        displayPrm[i].displayRes            = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_DVO2].resolution;
#endif
        mulich_vdec_vdis_set_avsync_prm(&gMultiCh_VdecVdisObj.avsyncCfg[i],i,vdDevId);
    }

    System_linkCreate(gVdecModuleContext.ipcBitsOutHLOSId,&ipcBitsOutHostPrm,sizeof(ipcBitsOutHostPrm));
    System_linkCreate(gVdecModuleContext.ipcBitsInRTOSId,&ipcBitsInVideoPrm,sizeof(ipcBitsInVideoPrm));
    System_linkCreate(gVdecModuleContext.decId, &decPrm, sizeof(decPrm));

    System_linkCreate(gMultiCh_VdecVdisObj.ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(gMultiCh_VdecVdisObj.ipcInVpssId  , &ipcInVpssPrm, sizeof(ipcInVpssPrm));

    if (enableVideoFrameExport)
    {
        System_linkCreate(gVdisModuleContext.ipcFramesOutHostId     , &ipcFramesOutHostPrm    , sizeof(ipcFramesOutHostPrm));
        System_linkCreate(gVdisModuleContext.ipcFramesInVpssFromHostId     , &ipcFramesInVpssFromHostPrm    , sizeof(ipcFramesInVpssFromHostPrm));
        System_linkCreate(gMultiCh_VdecVdisObj.mergeId,&mergePrm,sizeof(mergePrm));
    }

#ifdef TI_816X_BUILD
    System_linkCreate(gMultiCh_VdecVdisObj.mpSclrId, &mpSclrPrm, sizeof(mpSclrPrm));
#endif /* TI_816X_BUILD */
    System_linkCreate(gMultiCh_VdecVdisObj.dupId     , &dupPrm    , sizeof(dupPrm));

    for(i=0; i<gVsysModuleContext.vsysConfig.numDisplays; i++)
        System_linkCreate(gVdisModuleContext.swMsId[i]  , &swMsPrm[i], sizeof(swMsPrm[i]));

    for(i=0; i<gVsysModuleContext.vsysConfig.numDisplays; i++)
        System_linkCreate(gVdisModuleContext.displayId[i], &displayPrm[i], sizeof(displayPrm[i]));

    MultiCh_memPrintHeapStatus();
    gMultiCh_VdecVdisObj.enableVideoFrameExport = enableVideoFrameExport;
    {
        MultiCh_setDec2DispMap(VDIS_DEV_HDMI,gVdecModuleContext.vdecConfig.numChn,0,0);
        #if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        MultiCh_setDec2DispMap(VDIS_DEV_SD,gVdecModuleContext.vdecConfig.numChn,0,0);
        #else
        MultiCh_setDec2DispMap(VDIS_DEV_HDCOMP,gVdecModuleContext.vdecConfig.numChn,0,0);
        #endif
   }

}

Void MultiCh_deleteVdecVdis()
{
    /* delete can be done in any order */

    Vdec_delete();
    Vdis_delete();

    if (gMultiCh_VdecVdisObj.enableVideoFrameExport)
    {
        System_linkDelete(gMultiCh_VdecVdisObj.mergeId);
    }

#ifdef TI_816X_BUILD
    System_linkDelete(gMultiCh_VdecVdisObj.mpSclrId);
#endif /* TI_816X_BUILD */

    System_linkDelete(gMultiCh_VdecVdisObj.dupId);
    System_linkDelete(gMultiCh_VdecVdisObj.ipcOutVideoId );
    System_linkDelete(gMultiCh_VdecVdisObj.ipcInVpssId );

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);


#if (tilerEnable == FALSE)
    {
        /* Disable tiler allocator for this usecase
         * for that tiler memory can be reused for
         * non-tiled allocation
         */
        SystemTiler_enableAllocator();
    }
#endif

}

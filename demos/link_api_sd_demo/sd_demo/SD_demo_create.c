
/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_swMs.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_scd_bits_wr.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_initDeinit.h>

#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>

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
      MERGE1                      +--------------------------MERGE0                         +------------+
        |                                                        |                          |
        |                                                        |                          |
        |16CH D1 Preview                           IPC Frames Out0 (M3)                IPC Frames In(DSP)--ALGLINK
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
/* To enable or disable graphics in the application */

#define DEI_VIP_SC_MERGE_LINK_IDX   (0)
#define DEI_SC_D1_MERGE_LINK_IDX    (1)
#define CAP_SC5_MERGE_LINK_IDX      (2)
#define NUM_MERGE_LINK              (3)

#define NUM_DEI_LINK                 (2)
#define NUM_DUP_LINK                 (2)
/* To select if FBDEV interface is used for Graphics */
#define CAP_DUP_LINK_IDX_0          (0)
#define CAP_DUP_LINK_IDX_1          (1)
static UInt8 SCDChannelMonitor[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 24,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 24,
        .EncChList = {8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31, 40, 41, 42, 43, 44, 45, 46, 47},
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

typedef struct
{
    CaptureLink_CreateParams           capturePrm;
    DeiLink_CreateParams               deiPrm[NUM_DEI_LINK];
    MergeLink_CreateParams             mergePrm[NUM_MERGE_LINK];
    DupLink_CreateParams               dupPrm[NUM_DUP_LINK];
    NsfLink_CreateParams               nsfPrm;
    SclrLink_CreateParams              sclrPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssToHostPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInHostPrm;
    IpcFramesOutLinkHLOS_CreateParams  ipcFramesOutHostPrm;
    IpcFramesInLinkRTOS_CreateParams   ipcFramesInVpssFromHostPrm;
    IpcLink_CreateParams               ipcOutVpssPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm[2];

    IpcLink_CreateParams               ipcInVideoPrm;
    EncLink_CreateParams               encPrm;
    IpcBitsOutLinkRTOS_CreateParams    ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams     ipcBitsInHostPrm[2];

    IpcFramesInLinkRTOS_CreateParams   ipcFramesInDspPrm[2];
    AlgLink_CreateParams               dspAlgPrm[2];
    IpcBitsOutLinkRTOS_CreateParams    ipcBitsOutDspPrm;

    DisplayLink_CreateParams           displayPrm;
    SwMsLink_CreateParams       swMsPrm[2];
}SD_Demo_CreateParam;

SD_Demo_CreateParam gSD_Demo_CreateParam;

/******************************************************************************/
/* The link diagram, of this video chain is shown above                       */
/* Progressive Half-D1 (704x288) video is captured and fed to DEI and scalar  */
/* link                                                                       */
/* Privacy mask is applied in capture link hence propogaes to all the links   */
/* DEI is working in bypass mode wherein de-interlacing gets disable and only */
/* selected scalar of DEI path is used.                                       */ 
/* Scalar link is used to scale Half-D1 stream to CIF stream which is         */
/* subsequently fed to DSP link for SCD */
/* DEI has 3 o/p queues that generates 3 different D1 o/p streams.            */
/*         DEI-SC  Queue - D1 @ 30 FPS       for Display/Software  mosaic     */
/*         DEI-VIP Queue - D1 @ 30 FPS       for H264 Encoding                */
/*         DEI-VIP Secondary Queue D1 @ 1FPS for MJPEG Encoding               */
/* Software OSD is done before encoder. Channels only in the encoder path has */
/* NSF link is used for YUV conversion from 422 to 420 format.                */
/* OSD enabled.                                                               */
/******************************************************************************/

Void SD_Demo_create()
{
    UInt32                             i;
    System_LinkInfo                    framesProducerLinkInfo;


    gSD_Demo_ctrl.numSubChains = 2;

    SD_Demo_videoDecoderInit();

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    /* Initialize link IDs. All the links have unique ID.
     * Each link is identified by a system wide unique 32-bit link ID as defined 
     * in <system_linkId.h> 
     * The link ID determines on which processor the link runs as shown below 
     * 
     *       **Bits**               **Description**
     *  ----------------------------------------------------------
     *         0..27                   Link ID
     *
     *        28..31                 Processor ID on which this link runs
     *                               0: DSP
     *                               1: Video M3
     *                               2: VPSS M3
     *                               3: HOST A8
     */
     
    gSD_Demo_ctrl.captureId                          = SYSTEM_LINK_ID_CAPTURE;      
    gSD_Demo_ctrl.deiId[0]                           = SYSTEM_LINK_ID_DEI_HQ_0;
    gSD_Demo_ctrl.deiId[1]                           = SYSTEM_LINK_ID_DEI_0;

    gSD_Demo_ctrl.mergeId[DEI_VIP_SC_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_0;
    gSD_Demo_ctrl.mergeId[DEI_SC_D1_MERGE_LINK_IDX]  = SYSTEM_VPSS_LINK_ID_MERGE_1;

    gSD_Demo_ctrl.ipcOutVpssId                       = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;

    if(gSD_Demo_ctrl.enableVideoFramesExport)
    {
        gSD_Demo_ctrl.ipcFramesOutVpssToHostId       = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
        gSD_Demo_ctrl.ipcFramesInHostId              = SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0;
        gSD_Demo_ctrl.ipcFramesOutHostId             = SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0;
        gSD_Demo_ctrl.ipcFramesInVpssFromHostId      = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_0;
    }
    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
        gSD_Demo_ctrl.ipcFramesOutVpssId[0]          = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
    }

    if(gSD_Demo_ctrl.enableScdAlgLink)
    {
        gSD_Demo_ctrl.dupId[CAP_DUP_LINK_IDX_0]       = SYSTEM_VPSS_LINK_ID_DUP_0;
        gSD_Demo_ctrl.dupId[CAP_DUP_LINK_IDX_1]       = SYSTEM_VPSS_LINK_ID_DUP_1;
        gSD_Demo_ctrl.mergeId[CAP_SC5_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_2;

        gSD_Demo_ctrl.nsfId[0]                        = SYSTEM_LINK_ID_NSF_0;
        gSD_Demo_ctrl.sclrId[0]                       = SYSTEM_LINK_ID_SCLR_INST_0;

        gSD_Demo_ctrl.ipcFramesOutVpssId[1]           = SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_2;
   }

    gSD_Demo_ctrl.ipcInVideoId                        = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    gSD_Demo_ctrl.encId                               = SYSTEM_LINK_ID_VENC_0;
    gSD_Demo_ctrl.ipcBitsOutRTOSId                    = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    gSD_Demo_ctrl.ipcBitsInHLOSId[0]                  = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
        gSD_Demo_ctrl.dspAlgId[0]                     = SYSTEM_LINK_ID_ALG_0  ;
        gSD_Demo_ctrl.ipcFramesInDspId[0]             = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
    }

    if(gSD_Demo_ctrl.enableScdAlgLink)
    {
        gSD_Demo_ctrl.ipcFramesInDspId[1]             = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_1;
        gSD_Demo_ctrl.dspAlgId[1]                     = SYSTEM_LINK_ID_ALG_1;
        gSD_Demo_ctrl.ipcBitsOutDSPId                 = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
        gSD_Demo_ctrl.ipcBitsInHLOSId[1]              = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
    }

    gSD_Demo_ctrl.swMsId[0]                          = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gSD_Demo_ctrl.displayId[0]                       = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI

    /* Set create parammeter of Capture Links and associated VPSS-M3 links */
    SD_Demo_capParamSet();

    /* Set create parammeter of Encoder Links and associated Video-M3 links */
    SD_Demo_encParamSet();

    /* Set create parammeter of DSP-ALG Links and associated DSP links */
    SD_Demo_algParamSet();

    /* Set create parammeter of Display Links and associated VPSS-M3 links */
    SD_Demo_disParamSet();

    /* Start link creation. 
     * Creates a link - allocates driver, codec, memory resources.
     * Link creation should happen in the order of Source to the Sink link.
     * Source link generates video data e.g. frames, Bit streams. 
     * Sink link consumes video data or f/w to the next link (e.g. Display, FileWrite etc.)
     * Other associated links such as IPC, Merge, Dup etcs should also be 
     * created from Source to Sink as per video data flow*/
     
    System_linkCreate (gSD_Demo_ctrl.captureId, &gSD_Demo_CreateParam.capturePrm, sizeof(gSD_Demo_CreateParam.capturePrm));

    /* Create Only if SCD Link is enabled for Tamper/Motion Detect */
    if(gSD_Demo_ctrl.enableScdAlgLink)
    {
      for (i=0; i<NUM_DUP_LINK; i++)
          System_linkCreate(gSD_Demo_ctrl.dupId[i], &gSD_Demo_CreateParam.dupPrm[i], sizeof(gSD_Demo_CreateParam.dupPrm[i]));

      System_linkCreate(  gSD_Demo_ctrl.mergeId [CAP_SC5_MERGE_LINK_IDX],
                              &gSD_Demo_CreateParam.mergePrm [CAP_SC5_MERGE_LINK_IDX],
                              sizeof(gSD_Demo_CreateParam.mergePrm [CAP_SC5_MERGE_LINK_IDX])
                        );
      System_linkCreate(gSD_Demo_ctrl.sclrId[0], &gSD_Demo_CreateParam.sclrPrm, sizeof(gSD_Demo_CreateParam.sclrPrm));
      System_linkCreate(gSD_Demo_ctrl.nsfId[0], &gSD_Demo_CreateParam.nsfPrm, sizeof(gSD_Demo_CreateParam.nsfPrm));
    }

    for(i=0; i<NUM_DEI_LINK; i++)
        System_linkCreate(gSD_Demo_ctrl.deiId[i]  , &gSD_Demo_CreateParam.deiPrm[i], sizeof(gSD_Demo_CreateParam.deiPrm[i]));

    System_linkCreate(gSD_Demo_ctrl.mergeId [DEI_SC_D1_MERGE_LINK_IDX],
                            &gSD_Demo_CreateParam.mergePrm [DEI_SC_D1_MERGE_LINK_IDX],
                            sizeof(gSD_Demo_CreateParam.mergePrm [DEI_SC_D1_MERGE_LINK_IDX])
                        );

    System_linkCreate(gSD_Demo_ctrl.mergeId [DEI_VIP_SC_MERGE_LINK_IDX],
                        &gSD_Demo_CreateParam.mergePrm [DEI_VIP_SC_MERGE_LINK_IDX],
                        sizeof(gSD_Demo_CreateParam.mergePrm [DEI_VIP_SC_MERGE_LINK_IDX])
                    );

    if(gSD_Demo_ctrl.enableVideoFramesExport)
    {
        System_linkCreate(gSD_Demo_ctrl.ipcFramesOutVpssToHostId     , &gSD_Demo_CreateParam.ipcFramesOutVpssToHostPrm    , sizeof(gSD_Demo_CreateParam.ipcFramesOutVpssToHostPrm));
        System_linkCreate(gSD_Demo_ctrl.ipcFramesInHostId     , &gSD_Demo_CreateParam.ipcFramesInHostPrm    , sizeof(gSD_Demo_CreateParam.ipcFramesInHostPrm));
        System_linkGetInfo(gSD_Demo_ctrl.ipcFramesInHostId,&framesProducerLinkInfo);
        OSA_assert(framesProducerLinkInfo.numQue == 1);
        gSD_Demo_CreateParam.ipcFramesOutHostPrm.inQueInfo = framesProducerLinkInfo.queInfo[0];
        System_linkCreate(gSD_Demo_ctrl.ipcFramesOutHostId     , &gSD_Demo_CreateParam.ipcFramesOutHostPrm    , sizeof(gSD_Demo_CreateParam.ipcFramesOutHostPrm));
        System_linkCreate(gSD_Demo_ctrl.ipcFramesInVpssFromHostId     , &gSD_Demo_CreateParam.ipcFramesInVpssFromHostPrm    , sizeof(gSD_Demo_CreateParam.ipcFramesInVpssFromHostPrm));
    }


    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
        System_linkCreate(gSD_Demo_ctrl.ipcFramesOutVpssId[0], &gSD_Demo_CreateParam.ipcFramesOutVpssPrm[0], sizeof(gSD_Demo_CreateParam.ipcFramesOutVpssPrm[0]));
        System_linkCreate(gSD_Demo_ctrl.ipcFramesInDspId[0], &gSD_Demo_CreateParam.ipcFramesInDspPrm[0], sizeof(gSD_Demo_CreateParam.ipcFramesInDspPrm[0]));
        System_linkCreate(gSD_Demo_ctrl.dspAlgId[0] , &gSD_Demo_CreateParam.dspAlgPrm[0], sizeof(gSD_Demo_CreateParam.dspAlgPrm[0]));
    }

    System_linkCreate(gSD_Demo_ctrl.ipcOutVpssId , &gSD_Demo_CreateParam.ipcOutVpssPrm , sizeof(gSD_Demo_CreateParam.ipcOutVpssPrm) );
    System_linkCreate(gSD_Demo_ctrl.ipcInVideoId , &gSD_Demo_CreateParam.ipcInVideoPrm , sizeof(gSD_Demo_CreateParam.ipcInVideoPrm) );

    System_linkCreate(gSD_Demo_ctrl.encId, &gSD_Demo_CreateParam.encPrm, sizeof(gSD_Demo_CreateParam.encPrm));
    System_linkCreate(gSD_Demo_ctrl.ipcBitsOutRTOSId, &gSD_Demo_CreateParam.ipcBitsOutVideoPrm, sizeof(gSD_Demo_CreateParam.ipcBitsOutVideoPrm));
    System_linkCreate(gSD_Demo_ctrl.ipcBitsInHLOSId[0], &gSD_Demo_CreateParam.ipcBitsInHostPrm[0], sizeof(gSD_Demo_CreateParam.ipcBitsInHostPrm[0]));

    if(gSD_Demo_ctrl.enableScdAlgLink)
    {
        System_linkCreate(gSD_Demo_ctrl.ipcFramesOutVpssId[1], &gSD_Demo_CreateParam.ipcFramesOutVpssPrm[1], sizeof(gSD_Demo_CreateParam.ipcFramesOutVpssPrm[1]));
        System_linkCreate(gSD_Demo_ctrl.ipcFramesInDspId[1], &gSD_Demo_CreateParam.ipcFramesInDspPrm[1], sizeof(gSD_Demo_CreateParam.ipcFramesInDspPrm[1]));
        System_linkCreate(gSD_Demo_ctrl.dspAlgId[1] , &gSD_Demo_CreateParam.dspAlgPrm[1], sizeof(gSD_Demo_CreateParam.dspAlgPrm[1]));
        System_linkCreate(gSD_Demo_ctrl.ipcBitsOutDSPId, &gSD_Demo_CreateParam.ipcBitsOutDspPrm, sizeof(gSD_Demo_CreateParam.ipcBitsOutDspPrm));
        System_linkCreate(gSD_Demo_ctrl.ipcBitsInHLOSId[1], &gSD_Demo_CreateParam.ipcBitsInHostPrm[1], sizeof(gSD_Demo_CreateParam.ipcBitsInHostPrm[1]));

    }

    System_linkCreate(gSD_Demo_ctrl.swMsId[0]  , &gSD_Demo_CreateParam.swMsPrm[0], sizeof(gSD_Demo_CreateParam.swMsPrm[0]));
    System_linkCreate(gSD_Demo_ctrl.displayId[0], &gSD_Demo_CreateParam.displayPrm, sizeof(gSD_Demo_CreateParam.displayPrm));
}

Void SD_Demo_capParamSet()
{
    CaptureLink_VipInstParams          *pCaptureInstPrm;
    CaptureLink_OutParams              *pCaptureOutPrm;
    CaptureLink_CreateParams           capturePrm;
    DeiLink_CreateParams               deiPrm[NUM_DEI_LINK];
    MergeLink_CreateParams             mergePrm[NUM_MERGE_LINK];
    DupLink_CreateParams               dupPrm[NUM_DUP_LINK];
    NsfLink_CreateParams               nsfPrm;
    SclrLink_CreateParams              sclrPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssToHostPrm;
    IpcFramesInLinkHLOS_CreateParams   ipcFramesInHostPrm;
    IpcFramesOutLinkHLOS_CreateParams  ipcFramesOutHostPrm;
    IpcFramesInLinkRTOS_CreateParams   ipcFramesInVpssFromHostPrm;
    IpcLink_CreateParams               ipcOutVpssPrm;
    IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm[2];

    UInt32                             vipInstId;
    UInt32                             i;

    /* Initialize link create structure, Set default create time parameter  */ 
    SD_DEMO_INIT_STRUCT(CaptureLink_CreateParams, capturePrm);
    SD_DEMO_INIT_STRUCT(DeiLink_CreateParams, deiPrm[0]);
    SD_DEMO_INIT_STRUCT(DeiLink_CreateParams, deiPrm[1]);
    SD_DEMO_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);

    if (gSD_Demo_ctrl.enableVideoFramesExport)
    {
        SD_DEMO_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams ,ipcFramesOutVpssToHostPrm);
        SD_DEMO_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams  ,ipcFramesInHostPrm);
        SD_DEMO_INIT_STRUCT(IpcFramesOutLinkHLOS_CreateParams ,ipcFramesOutHostPrm);
        SD_DEMO_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  ,ipcFramesInVpssFromHostPrm);
    }
	    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
        SD_DEMO_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm[0]);
    }
    if(gSD_Demo_ctrl.enableScdAlgLink)
    {
        SD_DEMO_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams,ipcFramesOutVpssPrm[1]);
    }

    CaptureLink_CreateParams_Init(&capturePrm);
    capturePrm.enableSdCrop = FALSE;
    capturePrm.isPalMode = gSD_Demo_ctrl.tvp5158Handle[0]->isPalMode;
    
    capturePrm.numVipInst               = 2*gSD_Demo_ctrl.numSubChains;
    capturePrm.tilerEnable              = FALSE;
    
    /* Max no. of privacy mask in a frame.                                    */
    /* Detailed initilization is done in SD_Demo_blindAreaInit() function call */
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
        if (vipInstId >= 2)
          pCaptureOutPrm->outQueId          = 1;
        else
          pCaptureOutPrm->outQueId          = 0;
    }

    if (gSD_Demo_ctrl.enableScdAlgLink)
    {
        capturePrm.outQueParams[0].nextLink = gSD_Demo_ctrl.dupId[CAP_DUP_LINK_IDX_0];
        capturePrm.outQueParams[1].nextLink = gSD_Demo_ctrl.dupId[CAP_DUP_LINK_IDX_1];

        dupPrm[CAP_DUP_LINK_IDX_0].inQueParams.prevLinkId    = gSD_Demo_ctrl.captureId;
        dupPrm[CAP_DUP_LINK_IDX_0].inQueParams.prevLinkQueId = 0;
        dupPrm[CAP_DUP_LINK_IDX_0].notifyNextLink            = TRUE;
        dupPrm[CAP_DUP_LINK_IDX_0].numOutQue                 = 2;
        dupPrm[CAP_DUP_LINK_IDX_0].outQueParams[0].nextLink  = gSD_Demo_ctrl.deiId[0];
        dupPrm[CAP_DUP_LINK_IDX_0].outQueParams[1].nextLink  = gSD_Demo_ctrl.mergeId[CAP_SC5_MERGE_LINK_IDX];

        dupPrm[CAP_DUP_LINK_IDX_1].inQueParams.prevLinkId    = gSD_Demo_ctrl.captureId;
        dupPrm[CAP_DUP_LINK_IDX_1].inQueParams.prevLinkQueId = 1;
        dupPrm[CAP_DUP_LINK_IDX_1].notifyNextLink            = TRUE;

        /* No. of duplicate data from Dup Link */
        dupPrm[CAP_DUP_LINK_IDX_1].numOutQue                 = 2;

        /* Set next link of all the output queues of Dup Link. Set for 2 queues */
        dupPrm[CAP_DUP_LINK_IDX_1].outQueParams[0].nextLink  = gSD_Demo_ctrl.deiId[1];
        dupPrm[CAP_DUP_LINK_IDX_1].outQueParams[1].nextLink  = gSD_Demo_ctrl.mergeId[CAP_SC5_MERGE_LINK_IDX];
     }
     else
     {
        capturePrm.outQueParams[0].nextLink = gSD_Demo_ctrl.deiId[0];
        capturePrm.outQueParams[1].nextLink = gSD_Demo_ctrl.deiId[1];
     }

    for(i=0; i<NUM_DEI_LINK; i++)
    {
        Int32 chId;

        DeiLink_CreateParams_Init(&deiPrm[i]);

        if (gSD_Demo_ctrl.enableScdAlgLink)
        {
          deiPrm[i].inQueParams.prevLinkId                        = gSD_Demo_ctrl.dupId[i];
          deiPrm[i].inQueParams.prevLinkQueId                     = 0;
        }
        else
        {
          deiPrm[i].inQueParams.prevLinkId                        = gSD_Demo_ctrl.captureId;
          deiPrm[i].inQueParams.prevLinkQueId                     = i;
        }
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink               = gSD_Demo_ctrl.mergeId[DEI_SC_D1_MERGE_LINK_IDX];
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT].nextLink = gSD_Demo_ctrl.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
        deiPrm[i].outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink               = gSD_Demo_ctrl.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_DEI_SC]               = TRUE;
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = TRUE;
        deiPrm[i].enableOut[DEI_LINK_OUT_QUE_VIP_SC]               = TRUE;
        deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = FALSE;
        deiPrm[i].comprEnable                                   = FALSE;
        deiPrm[i].setVipScYuv422Format                          = FALSE;
        deiPrm[i].enableDeiForceBypass                          = FALSE;
        deiPrm[i].enableLineSkipSc                              = FALSE;

        /* Set Output Scaling at DEI */
        /* Scaling mode used in the scalar. Two different scalling modes are supported. */
        /*   DEI_SCALE_MODE_RATIO    : Specify ratio of i/p to o/p for width and height */
        /*   DEI_SCALE_MODE_ABSOLUTE : Absolute scale mode, provide width & height      */

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

        /* Specify Input and  Output FPS. DEI link will drop frames as per    */
        /*  the ratio of i/p and o/p FPS specified here                       */

        /* FPS rates of DEI queue connected to Display */ 
        deiPrm[i].inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC]  = 60;
        deiPrm[i].outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 30;

        /* FPS rates of DEI queue connected to encoder for H264 encoding */ 
        deiPrm[i].inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]  = 30;
        deiPrm[i].outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;

        /* FPS rates of DEI queue connected to encoder for MJPEG encoding */ 
        deiPrm[i].inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT]  = 30;
        deiPrm[i].outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT] = 1;

        /* No. of input queues to be combine in merge Link */
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].numInQue                     = gSD_Demo_ctrl.numSubChains;

        /* Details of each of input links and their queue no. on which data is available*/
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = gSD_Demo_ctrl.deiId[i];
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    }

    /* No. of input queues to be combine in merge Link */
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].numInQue                     = 4;

    /* Details of each of input links and their queue no. on which data is available*/
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = gSD_Demo_ctrl.deiId[0];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = gSD_Demo_ctrl.deiId[1];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[2].prevLinkId    = gSD_Demo_ctrl.deiId[0];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[3].prevLinkId    = gSD_Demo_ctrl.deiId[1];
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].inQueParams[3].prevLinkQueId = DEI_LINK_OUT_QUE_VIP_SC_SECONDARY_OUT;
    mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].notifyNextLink               = TRUE;


    if (gSD_Demo_ctrl.enableOsdAlgLink)
    {
        mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].outQueParams.nextLink = gSD_Demo_ctrl.ipcFramesOutVpssId[0];
    }
    else
    {
        mergePrm[DEI_VIP_SC_MERGE_LINK_IDX].outQueParams.nextLink = gSD_Demo_ctrl.ipcOutVpssId;
    }

    if (gSD_Demo_ctrl.enableOsdAlgLink)
    {
        ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
        ipcFramesOutVpssPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyPrevLink = TRUE;

        ipcFramesOutVpssPrm[0].baseCreateParams.numOutQue = 1;
        ipcFramesOutVpssPrm[0].baseCreateParams.outQueParams[0].nextLink = gSD_Demo_ctrl.ipcOutVpssId;
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyNextLink = TRUE;

        ipcFramesOutVpssPrm[0].baseCreateParams.processLink = gSD_Demo_ctrl.ipcFramesInDspId[0];
        ipcFramesOutVpssPrm[0].baseCreateParams.notifyProcessLink = TRUE;
        ipcFramesOutVpssPrm[0].baseCreateParams.noNotifyMode = FALSE;
    }

    if (gSD_Demo_ctrl.enableScdAlgLink)
    {
        mergePrm[CAP_SC5_MERGE_LINK_IDX].numInQue                     = 2;
        mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = gSD_Demo_ctrl.dupId[CAP_DUP_LINK_IDX_0];
        mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = 1;
        mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = gSD_Demo_ctrl.dupId[CAP_DUP_LINK_IDX_1];
        mergePrm[CAP_SC5_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = 1;
        mergePrm[CAP_SC5_MERGE_LINK_IDX].notifyNextLink               = TRUE;
        mergePrm[CAP_SC5_MERGE_LINK_IDX].outQueParams.nextLink        = gSD_Demo_ctrl.sclrId[0];


        SclrLink_CreateParams_Init(&sclrPrm);
        sclrPrm.inQueParams.prevLinkId             = gSD_Demo_ctrl.mergeId[CAP_SC5_MERGE_LINK_IDX];
        sclrPrm.inQueParams.prevLinkQueId          = 0;
        sclrPrm.outQueParams.nextLink              = gSD_Demo_ctrl.nsfId[0];
        sclrPrm.tilerEnable                        = FALSE;
        sclrPrm.enableLineSkipSc                   = TRUE;//FALSE;

        /* Specify Input and  Output FPS. Scalar link will drop frames as per */ 
        /*  the ratio of i/p and o/p FPS specified here                       */
        sclrPrm.inputFrameRate                     = 60;
        sclrPrm.outputFrameRate                    = 2;

        /* Set Output Scaling at Scalar link, Here o/p is 352x288*/
        /* Scaling mode used in the scalar. Two different scalling modes are supported. */
        /*   SCLR_SCALE_MODE_RATIO    : Specify ratio of i/p to o/p for width and height*/
        /*   SCLR_SCALE_MODE_ABSOLUTE : Absolute scale mode, provide width & height     */

        sclrPrm.scaleMode                          = SCLR_SCALE_MODE_RATIO;
        sclrPrm.outScaleFactor.ratio.widthRatio.numerator    = 1;
        sclrPrm.outScaleFactor.ratio.widthRatio.denominator  = 2;
        sclrPrm.outScaleFactor.ratio.heightRatio.numerator   = 1;
        sclrPrm.outScaleFactor.ratio.heightRatio.denominator = 1;

        NsfLink_CreateParams_Init(&nsfPrm);
        
        nsfPrm.inQueParams.prevLinkId    = gSD_Demo_ctrl.sclrId[0];
        nsfPrm.inQueParams.prevLinkQueId = 0;

        /* NSF used in bypass mode, only YUV convertion is done */
        nsfPrm.bypassNsf                 = TRUE;

        /* Input and Output FPS of NSF link, As ratio of these two is 1, NSF does not drop any frames */
        nsfPrm.inputFrameRate            = 30;
        nsfPrm.outputFrameRate           = 30;
        nsfPrm.tilerEnable               = FALSE;
        nsfPrm.numOutQue                 = 1;
        nsfPrm.outQueParams[0].nextLink  = gSD_Demo_ctrl.ipcFramesOutVpssId[1];
        nsfPrm.numBufsPerCh              = 4;

        /* Set IPC link params, send data from VPSS to DSP */ 
        ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.nsfId[0];;
        ipcFramesOutVpssPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutVpssPrm[1].baseCreateParams.notifyPrevLink            = TRUE;
        ipcFramesOutVpssPrm[1].baseCreateParams.numOutQue = 1;
        ipcFramesOutVpssPrm[1].baseCreateParams.outQueParams[0].nextLink = gSD_Demo_ctrl.ipcFramesInDspId[1];
        ipcFramesOutVpssPrm[1].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesOutVpssPrm[1].baseCreateParams.noNotifyMode = FALSE;

        ipcFramesOutVpssPrm[1].baseCreateParams.processLink = SYSTEM_LINK_ID_INVALID;
        ipcFramesOutVpssPrm[1].baseCreateParams.notifyProcessLink = FALSE;
    }
    if (gSD_Demo_ctrl.enableVideoFramesExport)
    {
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].outQueParams.nextLink = gSD_Demo_ctrl.ipcFramesOutVpssToHostId;
    }
    else
    {
        mergePrm[DEI_SC_D1_MERGE_LINK_IDX].outQueParams.nextLink = gSD_Demo_ctrl.swMsId[0];
    }
    mergePrm[DEI_SC_D1_MERGE_LINK_IDX].notifyNextLink        = TRUE;

    if (gSD_Demo_ctrl.enableVideoFramesExport)
    {
        /* Set IPC link params, send data from VPSS to A-8 */ 
        ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.mergeId[DEI_SC_D1_MERGE_LINK_IDX];
        ipcFramesOutVpssToHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;

        ipcFramesOutVpssToHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesOutVpssToHostPrm.baseCreateParams.outQueParams[0].nextLink = gSD_Demo_ctrl.ipcFramesInHostId;

        /* Set IPC link params, receive data from VPSS to A-8 */ 
        ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.ipcFramesOutVpssToHostId;
        ipcFramesInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesInHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesInHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInHostPrm.baseCreateParams.outQueParams[0].nextLink = SYSTEM_LINK_ID_INVALID;
        ipcFramesInHostPrm.exportOnlyPhyAddr = TRUE;

        /* Initialize Call back functions */
        SD_Demo_ipcFramesInSetCbInfo(&ipcFramesInHostPrm);

        /* Set IPC link params, send data from A-8 to VPSS */ 
        /* Previous link ID invalid, Data exchange happens via call back */
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
        ipcFramesOutHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesOutHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesOutHostPrm.baseCreateParams.notifyNextLink = FALSE;
        ipcFramesOutHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gSD_Demo_ctrl.ipcFramesInVpssFromHostId;

        /* Set IPC link params, receive data from A-8 to VPSS */ 
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.ipcFramesOutHostId;
        ipcFramesInVpssFromHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;

        ipcFramesInVpssFromHostPrm.baseCreateParams.noNotifyMode = TRUE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.notifyPrevLink = FALSE;
        ipcFramesInVpssFromHostPrm.baseCreateParams.outQueParams[0].nextLink = gSD_Demo_ctrl.swMsId[0];
    }
    if (gSD_Demo_ctrl.enableOsdAlgLink)
    {
        ipcOutVpssPrm.inQueParams.prevLinkId = gSD_Demo_ctrl.ipcFramesOutVpssId[0];
    }
    else
    {
        ipcOutVpssPrm.inQueParams.prevLinkId = gSD_Demo_ctrl.mergeId[DEI_VIP_SC_MERGE_LINK_IDX];
    }

    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink  = gSD_Demo_ctrl.ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = TRUE;


    /* Copy create param to global structure */
    memcpy(&gSD_Demo_CreateParam.capturePrm, &capturePrm, sizeof(CaptureLink_CreateParams));

    for(i=0; i<NUM_DEI_LINK; i++)
         memcpy(&gSD_Demo_CreateParam.deiPrm[i], &deiPrm[i], sizeof(DeiLink_CreateParams));

    for(i=0; i<NUM_MERGE_LINK; i++)
       memcpy(&gSD_Demo_CreateParam.mergePrm[i], &mergePrm[i], sizeof(MergeLink_CreateParams));

    if (gSD_Demo_ctrl.enableVideoFramesExport)
    {
        memcpy(&gSD_Demo_CreateParam.ipcFramesOutVpssToHostPrm, &ipcFramesOutVpssToHostPrm, sizeof(IpcFramesOutLinkRTOS_CreateParams));
        memcpy(&gSD_Demo_CreateParam.ipcFramesInHostPrm, &ipcFramesInHostPrm, sizeof(IpcFramesInLinkHLOS_CreateParams));
        memcpy(&gSD_Demo_CreateParam.ipcFramesOutHostPrm, &ipcFramesOutHostPrm, sizeof(IpcFramesOutLinkHLOS_CreateParams));    
        memcpy(&gSD_Demo_CreateParam.ipcFramesInVpssFromHostPrm, &ipcFramesInVpssFromHostPrm, sizeof(IpcFramesInLinkRTOS_CreateParams));
    }
    memcpy(&gSD_Demo_CreateParam.ipcOutVpssPrm, &ipcOutVpssPrm, sizeof(IpcLink_CreateParams));

    if (gSD_Demo_ctrl.enableOsdAlgLink)
    {
       memcpy(&gSD_Demo_CreateParam.ipcFramesOutVpssPrm[0], &ipcFramesOutVpssPrm[0], sizeof(IpcFramesOutLinkRTOS_CreateParams));
    }

    if (gSD_Demo_ctrl.enableScdAlgLink)
    {
        for(i=0; i<NUM_DUP_LINK; i++)    
           memcpy(&gSD_Demo_CreateParam.dupPrm[i], &dupPrm[i], sizeof(DupLink_CreateParams));

        memcpy(&gSD_Demo_CreateParam.nsfPrm, &nsfPrm, sizeof(NsfLink_CreateParams));
        memcpy(&gSD_Demo_CreateParam.sclrPrm, &sclrPrm, sizeof(SclrLink_CreateParams));    

        memcpy(&gSD_Demo_CreateParam.ipcFramesOutVpssPrm[1], &ipcFramesOutVpssPrm[1], sizeof(IpcFramesOutLinkRTOS_CreateParams));
    }

}

Void SD_Demo_encParamSet()
{
    UInt32                             i;
    IpcLink_CreateParams               ipcInVideoPrm;
    EncLink_CreateParams               encPrm;
    IpcBitsOutLinkRTOS_CreateParams    ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams     ipcBitsInHostPrm;
    
    /* Initialize link create structure, Set default create time parameter  */ 
    SD_DEMO_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    SD_DEMO_INIT_STRUCT(EncLink_CreateParams, encPrm);
    SD_DEMO_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    SD_DEMO_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);


    ipcInVideoPrm.inQueParams.prevLinkId    = gSD_Demo_ctrl.ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.outQueParams[0].nextLink  = gSD_Demo_ctrl.encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    encPrm.numBufPerCh[0] = 6;
    encPrm.numBufPerCh[1] = 6;
    encPrm.numBufPerCh[2] = 6;
    encPrm.numBufPerCh[3] = 6;

    {
        /* H264 channels are referred as Primary channels */
        UInt32 numPrimaryChn = gSD_Demo_ctrl.numCapChannels;

        /* MJPEG channels are referred as Secondary channels */
        UInt32 numSecondaryChn = gSD_Demo_ctrl.numCapChannels;

        EncLink_ChCreateParams *pLinkChPrm;
        EncLink_ChDynamicParams *pLinkDynPrm;

        /* Set H264 Encoder create param */
        for (i=0; i<numPrimaryChn; i++)
        {
            pLinkChPrm  = &encPrm.chCreateParams[i];
            pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

            pLinkChPrm->format                  = IVIDEO_H264HP;
            pLinkChPrm->profile                 = IH264_HIGH_PROFILE;
            pLinkChPrm->dataLayout              = IVIDEO_FIELD_SEPARATED;
            pLinkChPrm->fieldMergeEncodeEnable  = FALSE;
            pLinkChPrm->enableAnalyticinfo      = 0;
            pLinkChPrm->maxBitRate              = -1;
            pLinkChPrm->encodingPreset          = 3;//VENC_XDM_USER_DEFINED;
            pLinkChPrm->rateControlPreset       = 0;//VENC_RATE_CTRL_VBR;

            pLinkDynPrm->intraFrameInterval     = 30;
            pLinkDynPrm->targetBitRate          = 2*1000*1000;
            pLinkDynPrm->interFrameInterval     = 1;
            pLinkDynPrm->mvAccuracy             = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
            pLinkDynPrm->inputFrameRate         = 30;
            pLinkDynPrm->rcAlg                  = 0;//VENC_RATE_CTRL_VBR;
            pLinkDynPrm->qpMin                  = 10;
            pLinkDynPrm->qpMax                  = 40;
            pLinkDynPrm->qpInit                 = -1;
            pLinkDynPrm->vbrDuration            = 8;
            pLinkDynPrm->vbrSensitivity         = 0;
        }

        /* Set MJPEG Encoder create param */
        for (i=numPrimaryChn;
            i<(numPrimaryChn
                     + numSecondaryChn);
              i++)
        {
          pLinkChPrm  = &encPrm.chCreateParams[i];
          pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;


          pLinkChPrm->format                 = IVIDEO_MJPEG;
          pLinkChPrm->profile                = 0;
          pLinkChPrm->dataLayout             = IVIDEO_FIELD_SEPARATED;
          pLinkChPrm->fieldMergeEncodeEnable = FALSE;
          pLinkChPrm->enableAnalyticinfo     = 0;
          pLinkChPrm->maxBitRate             = 0;
          pLinkChPrm->encodingPreset         = 0;
          pLinkChPrm->rateControlPreset      = 0;

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

        encPrm.inQueParams.prevLinkId   = gSD_Demo_ctrl.ipcInVideoId;
        encPrm.inQueParams.prevLinkQueId= 0;
        encPrm.outQueParams.nextLink    = gSD_Demo_ctrl.ipcBitsOutRTOSId;
    }

    /* Set IPC link params, send bit stream from Video-M3 to A-8 */ 
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId    = gSD_Demo_ctrl.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink   = gSD_Demo_ctrl.ipcBitsInHLOSId[0];
    SD_Demo_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm, TRUE);

    /* Set IPC link params, receive bit stream from Video-M3 to A-8 */ 
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId    = gSD_Demo_ctrl.ipcBitsOutRTOSId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInHostPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsInHostPrm.baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;
    /* Set call back function */
    SD_Demo_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);

    memcpy(&gSD_Demo_CreateParam.ipcInVideoPrm, &ipcInVideoPrm, sizeof(IpcLink_CreateParams));    
    memcpy(&gSD_Demo_CreateParam.encPrm, &encPrm, sizeof(EncLink_CreateParams));
    memcpy(&gSD_Demo_CreateParam.ipcBitsOutVideoPrm, &ipcBitsOutVideoPrm, sizeof(IpcBitsOutLinkRTOS_CreateParams));
    memcpy(&gSD_Demo_CreateParam.ipcBitsInHostPrm[0], &ipcBitsInHostPrm, sizeof(IpcBitsInLinkHLOS_CreateParams));
}
Void SD_Demo_algParamSet()
{

    IpcFramesInLinkRTOS_CreateParams   ipcFramesInDspPrm[2];
    AlgLink_CreateParams               dspAlgPrm[2];
    IpcBitsOutLinkRTOS_CreateParams    ipcBitsOutDspPrm;
    IpcBitsInLinkHLOS_CreateParams     ipcBitsInHostPrm;

    /* Initialize link create structure, Set default create time parameter  */     
    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
      SD_DEMO_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm[0]);
      SD_DEMO_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm[0]);
    }
    if(gSD_Demo_ctrl.enableScdAlgLink)
    {
      SD_DEMO_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams,ipcFramesInDspPrm[1]);
      SD_DEMO_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm[1]);
      SD_DEMO_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutDspPrm);
      SD_DEMO_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);
    }

    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
        ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.ipcFramesOutVpssId[0];
        ipcFramesInDspPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm[0].baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm[0].baseCreateParams.outQueParams[0].nextLink = gSD_Demo_ctrl.dspAlgId[0];
        ipcFramesInDspPrm[0].baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesInDspPrm[0].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm[0].baseCreateParams.noNotifyMode   = FALSE;

        dspAlgPrm[0].inQueParams.prevLinkId = gSD_Demo_ctrl.ipcFramesInDspId[0];
        dspAlgPrm[0].inQueParams.prevLinkQueId = 0;
    }
    
    if(gSD_Demo_ctrl.enableScdAlgLink)
    {
        ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.ipcFramesOutVpssId[1];
        ipcFramesInDspPrm[1].baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcFramesInDspPrm[1].baseCreateParams.numOutQue   = 1;
        ipcFramesInDspPrm[1].baseCreateParams.outQueParams[0].nextLink = gSD_Demo_ctrl.dspAlgId[1];
        ipcFramesInDspPrm[1].baseCreateParams.notifyPrevLink = TRUE;
        ipcFramesInDspPrm[1].baseCreateParams.notifyNextLink = TRUE;
        ipcFramesInDspPrm[1].baseCreateParams.noNotifyMode   = FALSE;

        dspAlgPrm[1].inQueParams.prevLinkId = gSD_Demo_ctrl.ipcFramesInDspId[1];
        dspAlgPrm[1].inQueParams.prevLinkQueId = 0;
    }

    /* OSD initialization of OSD param. Detailed configiration is done in SD_Demo_osdInit() Call */ 
    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
        int chId;
        dspAlgPrm[0].enableOSDAlg    = TRUE;
        dspAlgPrm[0].enableSCDAlg    = FALSE;
        dspAlgPrm[0].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink = SYSTEM_LINK_ID_INVALID;

        for(chId = 0; chId < ALG_LINK_OSD_MAX_CH; chId++)
        {
            AlgLink_OsdChWinParams * chWinPrm = &dspAlgPrm[0].osdChCreateParams[chId].chDefaultParams;

            /* set osd window max width and height */
            dspAlgPrm[0].osdChCreateParams[chId].maxWidth  = SD_DEMO_OSD_WIN_MAX_WIDTH;
            dspAlgPrm[0].osdChCreateParams[chId].maxHeight = SD_DEMO_OSD_WIN_MAX_HEIGHT;

            chWinPrm->chId = chId;
            chWinPrm->numWindows = 0;
        }
    }

    /* SCD initialization of SCD param. */ 
    if (gSD_Demo_ctrl.enableScdAlgLink)
    {
       Int32    numBlksInFrame;
       Int32    numHorzBlks, numVertBlks, chIdx;
       UInt32  x, y, i;

        dspAlgPrm[1].enableOSDAlg    = FALSE;
        dspAlgPrm[1].enableSCDAlg    = TRUE;
        dspAlgPrm[1].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = gSD_Demo_ctrl.ipcBitsOutDSPId;

        dspAlgPrm[1].scdCreateParams.maxWidth               = 352;

        if(gSD_Demo_CreateParam.capturePrm.isPalMode)
           dspAlgPrm[1].scdCreateParams.maxHeight           = 288;
        else
           dspAlgPrm[0].scdCreateParams.maxHeight           = 240;

        dspAlgPrm[1].scdCreateParams.maxStride              = 352;
        dspAlgPrm[1].scdCreateParams.numValidChForSCD       = 16;
        dspAlgPrm[1].scdCreateParams.numSecs2WaitB4Init     = 3;
        dspAlgPrm[1].scdCreateParams.numSecs2WaitB4FrmAlert = 1;

        /* SCD frame rates. SCD operates at low FPS because of DSP MHz contrains */
        dspAlgPrm[1].scdCreateParams.inputFrameRate         = 2;
        dspAlgPrm[1].scdCreateParams.outputFrameRate        = 2;

        /* Configure array to monitor scene changes in all frame blocks, i.e., motion detection.*/
        /* Each block is fixed to be 32x10 or 32x12 in size*/
        numHorzBlks     = dspAlgPrm[1].scdCreateParams.maxWidth / 32;
        if(dspAlgPrm[1].scdCreateParams.maxHeight == 240)
           numVertBlks    = dspAlgPrm[1].scdCreateParams.maxHeight / 10;
        else   /* For 288 Block height used is 12 */
           numVertBlks    = dspAlgPrm[1].scdCreateParams.maxHeight / 12;

        numBlksInFrame  = numHorzBlks * numVertBlks;

        for(chIdx = 0; chIdx < dspAlgPrm[1].scdCreateParams.numValidChForSCD; chIdx++)
        {
           AlgLink_ScdChParams * chPrm = &dspAlgPrm[1].scdCreateParams.chDefaultParams[chIdx];

           chPrm->blkNumBlksInFrame     = numBlksInFrame;
           chPrm->chId                  = SCDChannelMonitor[chIdx];
           /* Enabling both Tamper and Motion (block mode) detection*/
           chPrm->mode                  = ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;
           chPrm->frmIgnoreLightsON     = FALSE;
           chPrm->frmIgnoreLightsOFF    = FALSE;
           chPrm->frmSensitivity     = ALG_LINK_SCD_SENSITIVITY_VERYHIGH;
                chPrm->frmEdgeThreshold   = 100;

           i = 0;
           for(y = 0; y < numVertBlks; y++)
           {
             for(x = 0; x < numHorzBlks; x++)
             {
               /* Configuring params of all the blocks. Blocks are disabled.*/
               /* User can enable blocks and change sensitivity from UI */
               /* Read Section 2.1.2 in DM81xx_DVR_RDK_DemoGuide.pdf for more details on UI guide */
               chPrm->blkConfig[i].sensitivity = ALG_LINK_SCD_SENSITIVITY_LOW;
               chPrm->blkConfig[i].monitored     = 0;
               i++;
             }
           }
        }

         /* Set IPC link params, send scd metaData stream from DSP to A-8 */ 
        ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkId    = gSD_Demo_ctrl.dspAlgId[1];
        ipcBitsOutDspPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcBitsOutDspPrm.baseCreateParams.numOutQue                 = 1;
        ipcBitsOutDspPrm.baseCreateParams.outQueParams[0].nextLink  = gSD_Demo_ctrl.ipcBitsInHLOSId[1];
        SD_Demo_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutDspPrm,
                                                   TRUE);
        ipcBitsOutDspPrm.baseCreateParams.notifyNextLink              = TRUE;
        ipcBitsOutDspPrm.baseCreateParams.noNotifyMode                = FALSE;

        /* Set IPC link params, receive scd metaData stream from DSP to A-8 */ 
        ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId = gSD_Demo_ctrl.ipcBitsOutDSPId;
        ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
        ipcBitsInHostPrm.baseCreateParams.numOutQue                 = 1;
        ipcBitsInHostPrm.baseCreateParams.outQueParams[0].nextLink   = SYSTEM_LINK_ID_INVALID;

        /* Set call back functions */
        SD_Demo_ipcBitsInitCreateParams_BitsInHLOSVcap(&ipcBitsInHostPrm);

        ipcBitsInHostPrm.baseCreateParams.notifyPrevLink         = TRUE;
        ipcBitsInHostPrm.baseCreateParams.noNotifyMode              = FALSE;

    }
    else
    {
        dspAlgPrm[1].outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;//nullId;
    }
    if (gSD_Demo_ctrl.enableOsdAlgLink)
    {
       memcpy(&gSD_Demo_CreateParam.ipcFramesInDspPrm[0], &ipcFramesInDspPrm[0], sizeof(IpcFramesInLinkRTOS_CreateParams));
       memcpy(&gSD_Demo_CreateParam.dspAlgPrm[0], &dspAlgPrm[0], sizeof(AlgLink_CreateParams));
    }
     if (gSD_Demo_ctrl.enableOsdAlgLink)
    {
       memcpy(&gSD_Demo_CreateParam.ipcFramesInDspPrm[1], &ipcFramesInDspPrm[1], sizeof(IpcFramesInLinkRTOS_CreateParams));
       memcpy(&gSD_Demo_CreateParam.dspAlgPrm[1], &dspAlgPrm[1], sizeof(AlgLink_CreateParams));
       memcpy(&gSD_Demo_CreateParam.ipcBitsOutDspPrm, &ipcBitsOutDspPrm, sizeof(IpcBitsOutLinkRTOS_CreateParams));
       memcpy(&gSD_Demo_CreateParam.ipcBitsInHostPrm[1], &ipcBitsInHostPrm, sizeof(IpcBitsInLinkHLOS_CreateParams));
    }
}

Void SD_Demo_disParamSet()
{
    UInt32                             i;
    UInt32                             displayRes[SYSTEM_DC_MAX_VENC];
    SwMsLink_CreateParams              swMsPrm[2];
    DisplayLink_CreateParams           displayPrm;

    memcpy(displayRes,gSD_Demo_ctrl.displayRes,sizeof(displayRes));

    SD_DEMO_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);
    SD_DEMO_INIT_STRUCT(SwMsLink_CreateParams, swMsPrm[0]);

    

    if(gSD_Demo_ctrl.enableVideoFramesExport)
    {
       swMsPrm[0].inQueParams.prevLinkId    = gSD_Demo_ctrl.ipcFramesInVpssFromHostId;
    }
    else
    {
        swMsPrm[0].inQueParams.prevLinkId    = gSD_Demo_ctrl.mergeId[DEI_SC_D1_MERGE_LINK_IDX];
    }

    swMsPrm[0].inQueParams.prevLinkQueId = 0;
    swMsPrm[0].numSwMsInst               = 1;
    swMsPrm[0].swMsInstId[0]             = SYSTEM_SW_MS_SC_INST_SC5;

    swMsPrm[0].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
    swMsPrm[0].maxOutRes                 = displayRes[0];;
    swMsPrm[0].lineSkipMode              = TRUE;
    swMsPrm[0].layoutPrm.outputFPS       = 30;
    swMsPrm[0].enableLayoutGridDraw      = FALSE;
    swMsPrm[0].outQueParams.nextLink     = gSD_Demo_ctrl.displayId[0];

    /* Generating default Software mosaic layout    */
    /* Devide IS is 0 as only 1 Display is enabled. */
    /* Layout ID-0 : 2X2_PLUS_4CH */
    /*           1 : 2X2          */
    /*           2 : 1X1          */
    /*           3 : 1x1_PLUS_2PIP*/
    /*           4 : 3X3          */
    /*           5 : 4X4          */
    /*           6 : 5CH_PLUS_1CH */
    /*           7 : 7CH_PLUS_1CH */
    /* Layout ID 5 is chosen i.e 4x4 mosaic layout*/
    /* Populating Software mosaic create tile structure with the mosaic param */

    SD_Demo_swMsGenerateLayoutParams(0, 5, &swMsPrm[0]);

    displayPrm.inQueParams[0].prevLinkId    = gSD_Demo_ctrl.swMsId[0];
    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.displayRes                   = swMsPrm[0].maxOutRes;

    for(i=0; i<1; i++)    
       memcpy(&gSD_Demo_CreateParam.swMsPrm[i], &swMsPrm[i], sizeof(SwMsLink_CreateParams));

    memcpy(&gSD_Demo_CreateParam.displayPrm, &displayPrm, sizeof(DisplayLink_CreateParams));
}

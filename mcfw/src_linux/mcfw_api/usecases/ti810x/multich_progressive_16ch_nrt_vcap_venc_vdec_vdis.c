/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/* This usecase assumes that all 3 outputs - D1 /CIF <h.264> + QCIF <h.264> + D1 <MJPEG> are enabled */
/*------------------------------ D1/CIF <h.264> + QCIF <h.264> + D1 <MJPEG> -------------------------------------
                                        Capture (YUV422I) 16CH D1 60fps
                                                    |
                                                    |
                                             NSF_AFTER_CAPTURE
                                                  YUV420
                                                    |
                                                    |
              ------------------------------------------------------------------------------------------------------------------
                                          CAPTURE_DUP_LINK_IDX
              ------------------------------------------------------------------------------------------------------------------
                  |                  |                           |                       |                            |
                  |                  |                           |                       |                            |
                  |                  | 16D1                      | 16D1                  | 16D1 420 30fps             | 16D1 420 30fps
                  |             ------------                     | 7.5fps                | (Even fields)              | (Even fields)
                  |             Select Link                      |                       |                            |
                  |             ------------                     |                       |                            |
                  |                  | 4D1                       |                       |                            |
                  |                  |                           |                       |                            |
                  |        -------------------------     ----------------------    --------------------------- --------------------
           16D1 420 30fps       DEI [SC+VIP0SC]             DEI [SC+VIP0SC]               DEI [SC2+VIP1SC]         SCLR [SC5]
                  |                 <DEI-SC>                    <VIP0-SC>                     <VIP1-SC>
           (Even fields)   -------------------------     ----------------------    --------------------------- --------------------
                  |                  | 4D1                       | 16D1                  | 16QCIF                     | 16D1 MJPEG
                  |                  | 30/60fps                  | 7.5fps                | 30fps                      | 1fps
                  |                  |                           | (YUV420)              | (YUV422)                   | (YUV422)
                  |                  |                           |                       |                            |
                  |                  |                           |                       |                            |
                  |                  |                           |                       |                            |NSF_AFTER_SCALAR
                  |                  |                           |                       |                            |(YUV420)
                  |                  |                           |                       |                            |
                  |                  |                           |                       |                            |
                  |                  |                           |0                      |1                           |2
                  |                  |                           |16D1                   |16QCIF                      |16D1
                  |                  |                           |H.264                  |H.264                       |MJPEG
                  |                  |                           |7.5fps                 |30fps                       |1fps
                  |       |-----------               ---------------------------------------------------------------------
                  |       |                                        D1_CIF_QCIF_MERGE_LINK_IDX
                  |       |                          ---------------------------------------------------------------------
                  |       |                                              |
                  |       |                                              |
                  |       |                                        FRAMESOUT(VPSS)-----------<<<processLink>>>----FramesInDSP-----ALG_LINK <OSD, SCD Algs>
                  |       |                                              |
          CH4..19 |       |                                              |
                  |       |CH0..3                                  IPCM3OUT(VPSS)---IPCM3IN(VID)----ENC-----IPCBITS_RTOSOUT(VID)----IPCBITS_HLOSIN(HOST)-----FILEOUT
                  |       |                                                                                                                                        |
                  |       |                                                                                                                                        |
                  |       |          CH20..35                                                                                                                      |
                  |       |         ----------------IPCM3IN(VPSS)---------IPCM3OUT(VID)-------------DEC---------------IPCBITS_RTOSIN(VID)-------------IPCBITS_HLOSOUT(HOST)
                  |       |4D1      |
           16D1   |       |30/60fps |
           30fps  |       |         |16D1 7.5fps
         -----------------------------
           LIVE_DECODE_MERGE_LINK_IDX
         -----------------------------
                       |
                       |CH0..35
           ------------------------
           LIVE_DECODE_DUP_LINK_IDX
           ------------------------
                      |||
      +---------------+|+----------------+
      |                                  |
      |                                  |
  SW Mosaic 1                       SW Mosaic 0
 (SC2 YUV422I)                     (SC5 YUV422I)
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


#include "mcfw/interfaces/link_api/system_tiler.h"

// Disabling tiler until memory alloc issues are fixed. Low performance will be observed in non tiler mode
#define TILER_ENABLE    FALSE//TRUE


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
        .EncNumCh  = 48,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 , 14, 15,16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47},
        .DecNumCh  = 16,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 , 14, 15},
    },

};

#define ENABLE_DEI_FOR_D1

#define NUM_SCD_CHANNELS        (16)

#define      ADD_DEI_FOR_1_AND_4CH_60_FPS
#define      ADD_NSF_AFTER_CAPTURE

//#define     NSF_AFTER_DEI_SC_LINK_IDX            0
#define     NSF_AFTER_SC5_LINK_IDX               0

#ifdef       ADD_NSF_AFTER_CAPTURE
    #define     NUM_NSF_LINK                     2
    #define     NSF_AFTER_CAPTURE_LINK_IDX       1
#else
    #define     NUM_NSF_LINK                     1
#endif

#define     NUM_LIVE_CH                 16
#define     NUM_DEI_LIVE_CH             4

#define     NUM_SELECT_LINK             1
#define     DEI_LIVE_SELECT             0

#ifdef      ADD_DEI_FOR_1_AND_4CH_60_FPS
#define     NUM_DEI_LINK                3
#define     DEI_VIP0_SC_DEI_SC_2        0
#define     DEI_BYPASS_VIP1_SC_SC2      1
#define     DEI_VIP0_SC_DEI_SC          2
#else
#define     NUM_DEI_LINK                2
#define     DEI_VIP0_SC_DEI_SC_2        0
#define     DEI_BYPASS_VIP1_SC_SC2      1
#endif

#define     NUM_MERGE_LINK              2
#define     D1_CIF_QCIF_MERGE_LINK_IDX  0
#define     LIVE_DECODE_MERGE_LINK_IDX  1

#define     NUM_DUP_LINK                2
#define     CAPTURE_DUP_LINK_IDX        0
#define     LIVE_DECODE_DUP_LINK_IDX    1

#define     NUM_CAPTURE_BUFFERS         8
#define     NUM_DEI_LIVE_BUFFERS        5
#define     NUM_DEI_D1_BUFFERS          3
#define     NUM_DEI_QCIF_BUFFERS        3
#define     NUM_NSF_BUFFERS             1
#define     NUM_DEI_MJPEG_BUFFERS       1
#define     NUM_ENCODE_D1_BUFFERS       3
#define     NUM_ENCODE_CIF_BUFFERS      3
#define     NUM_ENCODE_MJPEG_BUFFERS    1
#define     NUM_DECODE_BUFFERS          3
#define     NUM_IPC_OUT_BUFFERS         3
#define     NUM_SWMS_0_BUFFERS          6
#define     NUM_SWMS_1_BUFFERS          8
#define     NUM_SWMS_MAX_BUFFERS        8


#define     NUM_IPC_LINK                4
#define     IPC_OUT_VPSS_LINK_IDX       0
#define     IPC_IN_VPSS_LINK_IDX        1
#define     IPC_OUT_VIDEO_LINK_IDX      2
#define     IPC_IN_VIDEO_LINK_IDX       3

#define     NUM_CAPTURE_DEVICES         4


Void set_DEI_BYPASS_VIP1_SC_SC2_paramsNrt  (
                        DeiLink_CreateParams *deiPrm,
                        Uint32  prevLinkId,
                        Uint32 prevLinkQ,
                        Uint32 nextLinkIdFor_DEI_SC,
                        Uint32 nextLinkIdFor_VIP_SC
            )
{
    Int32 chId;

    deiPrm->inQueParams.prevLinkId = prevLinkId;
    deiPrm->inQueParams.prevLinkQueId  = prevLinkQ;

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = TRUE;

    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_DEI_SC]                 = TRUE;
    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_VIP_SC]                 = FALSE;

    deiPrm->outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            = nextLinkIdFor_DEI_SC;
    deiPrm->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink            = nextLinkIdFor_VIP_SC;

    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TILER_ENABLE;
    deiPrm->comprEnable                        = FALSE;
    deiPrm->setVipScYuv422Format               = FALSE;
    deiPrm->enableLineSkipSc                   = FALSE;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]  = 25;
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 25;

    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC] = NUM_DEI_QCIF_BUFFERS;

    /* Set Output Scaling at DEI based on ratio */
    for (chId = 0; chId < DEI_LINK_MAX_CH; chId++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.numerator = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.denominator = 4;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.numerator = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.denominator = 2;
    }

    deiPrm->enableDeiForceBypass = TRUE;
}

static Void set_DEI_VIP0_SC_DEI_SC_paramsNrt (
                            SelectLink_CreateParams *selectPrm,
                            DeiLink_CreateParams *deiPrm,
                            Uint32 prevLinkId,
                            Uint32 prevLinkQ,
                            Uint32 nextLinkIdFor_DEI_SC,
                            Uint32 nextLinkIdFor_VIP_SC,
                            UInt32 deiSelectId,
                            UInt32 deiId
                        )
{
    Int32 chId;

    selectPrm->numOutQue = 1;

    selectPrm->inQueParams.prevLinkId = prevLinkId;
    selectPrm->inQueParams.prevLinkQueId  = prevLinkQ;

    selectPrm->outQueParams[0].nextLink = deiId;

    selectPrm->outQueChInfo[0].outQueId = 0;
    selectPrm->outQueChInfo[0].numOutCh = NUM_DEI_LIVE_CH;

    for(chId=0; chId<selectPrm->outQueChInfo[0].numOutCh; chId++)
    {
        selectPrm->outQueChInfo[0].inChNum[chId] = chId;
    }

    deiPrm->inQueParams.prevLinkId = deiSelectId;
    deiPrm->inQueParams.prevLinkQueId  = 0;

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = TRUE;

    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_DEI_SC]                 = FALSE;
    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_VIP_SC]                 = TRUE;

    deiPrm->outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            = nextLinkIdFor_DEI_SC;
    deiPrm->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink            = nextLinkIdFor_VIP_SC;

    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TILER_ENABLE;
    deiPrm->comprEnable                        = FALSE;
    deiPrm->setVipScYuv422Format               = FALSE;
    deiPrm->enableLineSkipSc                   = FALSE;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC]  = 50;
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 50;

    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC] = NUM_DEI_LIVE_BUFFERS;

    /* Set Output Scaling at DEI based on ratio */
    for (chId = 0; chId < DEI_LINK_MAX_CH; chId++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.numerator = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.denominator = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.numerator = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.denominator = 1;
    }

    deiPrm->enableDeiForceBypass = FALSE;

}

Void set_DEI_VIP0_SC_DEI_SC_2_paramsNrt (
                            DeiLink_CreateParams *deiPrm,
                            Uint32  prevLinkId,
                            Uint32 prevLinkQ,
                            Uint32 nextLinkIdFor_DEI_SC,
                            Uint32 nextLinkIdFor_VIP_SC
                        )
{
    Int32 chId;

    deiPrm->inQueParams.prevLinkId = prevLinkId;
    deiPrm->inQueParams.prevLinkQueId  = prevLinkQ;

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC]                        = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC]                        = TRUE;

    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_DEI_SC]                 = TRUE;
    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_VIP_SC]                 = FALSE;

    deiPrm->outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink            = nextLinkIdFor_DEI_SC;
    deiPrm->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink            = nextLinkIdFor_VIP_SC;

    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TILER_ENABLE;
    deiPrm->comprEnable                        = FALSE;
    deiPrm->setVipScYuv422Format               = FALSE;
    deiPrm->enableLineSkipSc                   = FALSE;
#ifdef ENABLE_DEI_FOR_D1
    deiPrm->inputDeiFrameRate  = 31;
#else
    deiPrm->inputDeiFrameRate  = 25;
#endif
    deiPrm->outputDeiFrameRate = 6;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC]  = 6;
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 1;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]  = 6;
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 6;

    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC] = NUM_DEI_D1_BUFFERS;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC] = NUM_DEI_MJPEG_BUFFERS;

    /* Set Output Scaling at DEI based on ratio */
    for (chId = 0; chId < DEI_LINK_MAX_CH; chId++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.numerator = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.denominator = 1;

        #ifdef ENABLE_DEI_FOR_D1
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.numerator = 1;
        #else
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.numerator = 2;
        #endif

        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.denominator = 1;

        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.numerator = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.denominator = 1;

        #ifdef ENABLE_DEI_FOR_D1
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.numerator = 1;
        #else
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.numerator = 2;
        #endif

        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.denominator = 1;
    }

    #ifdef ENABLE_DEI_FOR_D1
    deiPrm->enableDeiForceBypass = FALSE;
    #else
    deiPrm->enableDeiForceBypass = TRUE;
    #endif
}


static Void setLinkIdsNrt (Bool enableOsd, UInt32 mergeId[], UInt32 dupId[], UInt32 ipcId[], UInt32 selectId[])
{
    gVcapModuleContext.captureId               = SYSTEM_LINK_ID_CAPTURE;
    if(enableOsd)
    {
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_VIDEO_ALG_0  ;
    }

    //gVcapModuleContext.nsfId[NSF_AFTER_DEI_SC_LINK_IDX]     = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.nsfId[NSF_AFTER_SC5_LINK_IDX]        = SYSTEM_LINK_ID_NSF_0;

    selectId[DEI_LIVE_SELECT] = SYSTEM_VPSS_LINK_ID_SELECT_0;

#ifdef     ADD_NSF_AFTER_CAPTURE
    gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE_LINK_IDX]    = SYSTEM_LINK_ID_NSF_1;
#endif
#ifdef      ADD_DEI_FOR_1_AND_4CH_60_FPS
    gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC]            = SYSTEM_LINK_ID_DEI_0;
#endif
    gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_2]          = SYSTEM_LINK_ID_DEI_1;
    gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2]        = SYSTEM_LINK_ID_DEI_HQ_0;

    gVcapModuleContext.sclrId[0]    = SYSTEM_LINK_ID_SCLR_INST_0;
    gVencModuleContext.encId        = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId        = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; /* ON AND OFF CHIP HDMI */
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_2; /* SDTV */

    mergeId[LIVE_DECODE_MERGE_LINK_IDX]      = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[D1_CIF_QCIF_MERGE_LINK_IDX]      = SYSTEM_VPSS_LINK_ID_MERGE_1;

    dupId[CAPTURE_DUP_LINK_IDX]             = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX]         = SYSTEM_VPSS_LINK_ID_DUP_1;

    ipcId[IPC_OUT_VPSS_LINK_IDX]   = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcId[IPC_IN_VIDEO_LINK_IDX]   = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcId[IPC_OUT_VIDEO_LINK_IDX]  = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcId[IPC_IN_VPSS_LINK_IDX]    = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    gVencModuleContext.ipcBitsOutRTOSId       = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    gVencModuleContext.ipcBitsInHLOSId        = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.ipcBitsOutHLOSId       = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId        = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;
}


Void setMJPEGEncChParamsNrt (EncLink_CreateParams *encPrm, Int32 startChNum, Int32 endChNum)
{
    Int32 i;

    EncLink_ChCreateParams *pLinkChPrm;
    EncLink_ChDynamicParams *pLinkDynPrm;
    VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
    VENC_CHN_PARAMS_S *pChPrm;

    for (i=startChNum; i<endChNum; i++)
    {
         pLinkChPrm  = &encPrm->chCreateParams[i];
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
         pLinkDynPrm->inputFrameRate        = 1; //pDynPrm->inputFrameRate;
         pLinkDynPrm->qpMin                 = 0;
         pLinkDynPrm->qpMax                 = 0;
         pLinkDynPrm->qpInit                = -1;
         pLinkDynPrm->vbrDuration           = 0;
         pLinkDynPrm->vbrSensitivity        = 0;
    }
}
Void setH264EncChParamsNrt(EncLink_CreateParams *encPrm, Int32 startChNum, Int32 endChNum)
{
    Int32 i;

    EncLink_ChCreateParams *pLinkChPrm;
    EncLink_ChDynamicParams *pLinkDynPrm;
    VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
    VENC_CHN_PARAMS_S *pChPrm;

    for (i=startChNum; i<endChNum; i++)
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
        pLinkDynPrm->rcAlg                  = pDynPrm->rcAlg;
        pLinkDynPrm->qpMin                  = pDynPrm->qpMin;
        pLinkDynPrm->qpMax                  = pDynPrm->qpMax;
        pLinkDynPrm->qpInit                 = pDynPrm->qpInit;
        pLinkDynPrm->vbrDuration            = pDynPrm->vbrDuration;
        pLinkDynPrm->vbrSensitivity         = pDynPrm->vbrSensitivity;
    }
}

static void skipOddFieldAtSCNrt(UInt32 numChs)
{
    SclrLink_chDynamicSkipFidType prm;
    UInt32 chId;

    for(chId=0; chId<numChs; chId++)
    {
        prm.chId = chId;
        prm.fidType = 1;

        System_linkControl(
            gVcapModuleContext.sclrId[0],
            SCLR_LINK_CMD_SKIP_FID_TYPE,
            &prm,
            sizeof(prm),
            TRUE
        );
    }
}

/* This usecase assumes secondary out, MJPEG are enabled */
Void MultiCh_createProgressive16ChNrtVcapVencVdecVdis()
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm[NUM_NSF_LINK];
    DeiLink_CreateParams        deiPrm[NUM_DEI_LINK];
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
    IpcFramesOutLinkRTOS_CreateParams    ipcFramesOutVpssPrm;
    AlgLink_CreateParams                        dspAlgPrm;
    SclrLink_CreateParams                       sclrPrm;
    System_LinkInfo                   bitsProducerLinkInfo;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 ipcId[NUM_IPC_LINK];
    UInt32 selectId[NUM_SELECT_LINK];

    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

    SelectLink_CreateParams           selectPrm[NUM_SELECT_LINK];

    UInt32 vipInstId;
    UInt32 i;
    Bool   enableAlgLink = gVsysModuleContext.vsysConfig.enableOsd;
    Bool   enableScd     = gVsysModuleContext.vsysConfig.enableScd;
    Bool   enableOsd     = gVsysModuleContext.vsysConfig.enableOsd;

    enableScd = TRUE;
    enableOsd = TRUE;

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
    MULTICH_INIT_STRUCT(SclrLink_CreateParams, sclrPrm);

    for (i = 0; i < VDIS_DEV_MAX;i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams, displayPrm[i]);
        MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[i]);
    }
    for (i = 0; i < NUM_NSF_LINK; i++)
        MULTICH_INIT_STRUCT(NsfLink_CreateParams, nsfPrm[i]);

    for (i = 0; i < NUM_DEI_LINK;i++)
        MULTICH_INIT_STRUCT(DeiLink_CreateParams, deiPrm[i]);

    for (i = 0; i < NUM_SELECT_LINK;i++)
        MULTICH_INIT_STRUCT(SelectLink_CreateParams, selectPrm[i]);

    printf("\n********* Entered usecase 16CH <810x> Enc/Dec OSD %s SCD %s \n\n",
            enableOsd == TRUE ? "Enabled" : "Disabled",
            enableScd == TRUE ? "Enabled" : "Disabled"
            );

    MultiCh_detectBoard();

#if (TILER_ENABLE == FALSE)
    /* Temp Fix - Disable tiler allocator for this usecase so that tiler memory can be reused for non-tiled allocation */
    SystemTiler_disableAllocator();
#endif

    System_linkControl(SYSTEM_LINK_ID_M3VPSS, SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES, NULL, 0, TRUE );
    System_linkControl(SYSTEM_LINK_ID_M3VIDEO, SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL, &systemVid_encDecIvaChMapTbl, sizeof(SystemVideo_Ivahd2ChMap_Tbl), TRUE);

    gVcapModuleContext.ipcBitsInHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
    setLinkIdsNrt(enableAlgLink, mergeId, dupId, ipcId, selectId);

    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[1].numSwMsInst = 1;
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEI_SC_NO_DEI;
    //swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEIHQ_SC_NO_DEI;

    CaptureLink_CreateParams_Init(&capturePrm);
#ifdef     ADD_NSF_AFTER_CAPTURE
    capturePrm.outQueParams[0].nextLink   = gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE_LINK_IDX];
#else
    capturePrm.outQueParams[0].nextLink   = dupId[CAPTURE_DUP_LINK_IDX];
#endif
    capturePrm.numVipInst                = 4;
    capturePrm.tilerEnable               = FALSE;
    capturePrm.numBufsPerCh              = NUM_CAPTURE_BUFFERS;
    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                                         = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId                              = (SYSTEM_CAPTURE_INST_VIP0_PORTA+
                                                                   vipInstId)%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId                         = SYSTEM_DEVICE_VID_DEC_TVP5158_DRV;
        pCaptureInstPrm->inDataFormat                           = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard                               = SYSTEM_STD_MUX_4CH_D1;
        pCaptureInstPrm->numOutput                              = 1;

        pCaptureOutPrm                                          = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat                              = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable                                = FALSE;
        pCaptureOutPrm->scOutWidth                              = 0;
        pCaptureOutPrm->scOutHeight                             = 0;
        pCaptureOutPrm->outQueId                                = 0;
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

    if(gVsysModuleContext.enableFastUsecaseSwitch==FALSE)
    Vcap_configVideoDecoder(vidDecVideoModeArgs, NUM_CAPTURE_DEVICES);

#ifdef     ADD_NSF_AFTER_CAPTURE
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].bypassNsf                    = TRUE;
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].tilerEnable                  = FALSE;
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].inQueParams.prevLinkId       = gVcapModuleContext.captureId;
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].inQueParams.prevLinkQueId    = 0;
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].numOutQue                    = 1;
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].outQueParams[0].nextLink     = dupId[CAPTURE_DUP_LINK_IDX];
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].numBufsPerCh                 = NUM_CAPTURE_BUFFERS-2;
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].inputFrameRate               = 30;
    nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX].outputFrameRate              = 30;

    dupPrm[CAPTURE_DUP_LINK_IDX].inQueParams.prevLinkId           = gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE_LINK_IDX];
#else
    dupPrm[CAPTURE_DUP_LINK_IDX].inQueParams.prevLinkId           = gVcapModuleContext.captureId;
#endif

    dupPrm[CAPTURE_DUP_LINK_IDX].inQueParams.prevLinkQueId      = 0;
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[0].nextLink       = gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2];
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[1].nextLink       = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[2].nextLink       = gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_2];
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[3].nextLink       = gVcapModuleContext.sclrId[0];
    dupPrm[CAPTURE_DUP_LINK_IDX].numOutQue = 4;
#ifdef      ADD_DEI_FOR_1_AND_4CH_60_FPS
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[4].nextLink       = selectId[DEI_LIVE_SELECT];
    dupPrm[CAPTURE_DUP_LINK_IDX].numOutQue++;
#endif
    dupPrm[CAPTURE_DUP_LINK_IDX].notifyNextLink                 = TRUE;

#ifdef      ADD_DEI_FOR_1_AND_4CH_60_FPS
    /****************** De-Interlacer Settigs ********************/
    set_DEI_VIP0_SC_DEI_SC_paramsNrt(&selectPrm[DEI_LIVE_SELECT], &deiPrm[DEI_VIP0_SC_DEI_SC],
                                     dupId[CAPTURE_DUP_LINK_IDX],  4,
                                     mergeId[LIVE_DECODE_MERGE_LINK_IDX],
                                     SYSTEM_LINK_ID_INVALID,
                                     selectId[DEI_LIVE_SELECT],
                                     gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC]);

#endif
    set_DEI_VIP0_SC_DEI_SC_2_paramsNrt(&deiPrm[DEI_VIP0_SC_DEI_SC_2],
                                       dupId[CAPTURE_DUP_LINK_IDX],  2,
                                       //gVcapModuleContext.nsfId[NSF_AFTER_DEI_SC_LINK_IDX],
                                       SYSTEM_LINK_ID_INVALID,
                                       mergeId[D1_CIF_QCIF_MERGE_LINK_IDX]);

    /*************** SC2 / VIPSC Settings ******************/
    set_DEI_BYPASS_VIP1_SC_SC2_paramsNrt(&deiPrm[DEI_BYPASS_VIP1_SC_SC2],
                                         dupId[CAPTURE_DUP_LINK_IDX], 0,
                                         SYSTEM_LINK_ID_INVALID,
                                         mergeId[D1_CIF_QCIF_MERGE_LINK_IDX]);
/*
    nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX].bypassNsf                     = TRUE;
    nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX].tilerEnable                   = TILER_ENABLE;
    nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX].inQueParams.prevLinkId        = gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_2];
    nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX].inQueParams.prevLinkQueId     = DEI_LINK_OUT_QUE_DEI_SC;
    nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX].numOutQue                     = 1;
    nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX].outQueParams[0].nextLink      = mergeId[D1_CIF_QCIF_MERGE_LINK_IDX];
    nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX].numBufsPerCh                  = NUM_NSF_BUFFERS;
*/
    sclrPrm.enableLineSkipSc = FALSE;
    sclrPrm.inputFrameRate   = 25;
    sclrPrm.outputFrameRate  = 1;
    sclrPrm.inQueParams.prevLinkId    = dupId[CAPTURE_DUP_LINK_IDX];
    sclrPrm.inQueParams.prevLinkQueId = 3;
    sclrPrm.outQueParams.nextLink     = gVcapModuleContext.nsfId[NSF_AFTER_SC5_LINK_IDX];
    sclrPrm.scaleMode = DEI_SCALE_MODE_RATIO;
    sclrPrm.outScaleFactor.ratio.heightRatio.numerator   = 2;
    sclrPrm.outScaleFactor.ratio.heightRatio.denominator = 1;
    sclrPrm.outScaleFactor.ratio.widthRatio.numerator    = 1;
    sclrPrm.outScaleFactor.ratio.widthRatio.denominator  = 1;
    sclrPrm.tilerEnable = FALSE; // force tiler disable;
    sclrPrm.numBufsPerCh = 1;

    nsfPrm[NSF_AFTER_SC5_LINK_IDX].bypassNsf                     = TRUE;
    nsfPrm[NSF_AFTER_SC5_LINK_IDX].tilerEnable                   = TILER_ENABLE;
    nsfPrm[NSF_AFTER_SC5_LINK_IDX].inQueParams.prevLinkId        = gVcapModuleContext.sclrId[0];
    nsfPrm[NSF_AFTER_SC5_LINK_IDX].inQueParams.prevLinkQueId     = 0;
    nsfPrm[NSF_AFTER_SC5_LINK_IDX].numOutQue                     = 1;
    nsfPrm[NSF_AFTER_SC5_LINK_IDX].outQueParams[0].nextLink      = mergeId[D1_CIF_QCIF_MERGE_LINK_IDX];
    nsfPrm[NSF_AFTER_SC5_LINK_IDX].numBufsPerCh                  = 1;

    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[0].prevLinkId       = gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_2];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId    = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[1].prevLinkId       = gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId    = DEI_LINK_OUT_QUE_VIP_SC;
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[2].prevLinkId       = gVcapModuleContext.nsfId[NSF_AFTER_SC5_LINK_IDX];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId    = 0;

    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].notifyNextLink                  = TRUE;
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].numInQue = 3;

    ipcOutVpssPrm.inQueParams.prevLinkQueId    = 0;

    dspAlgPrm.enableOSDAlg = enableOsd;
        mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].outQueParams.nextLink  = ipcId[IPC_OUT_VPSS_LINK_IDX];
        ipcOutVpssPrm.inQueParams.prevLinkId                        = mergeId[D1_CIF_QCIF_MERGE_LINK_IDX];
    
    ipcOutVpssPrm.numOutQue                    = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcId[IPC_IN_VIDEO_LINK_IDX];
    ipcOutVpssPrm.notifyNextLink               = TRUE;
    ipcOutVpssPrm.notifyPrevLink               = TRUE;
    ipcOutVpssPrm.noNotifyMode                 = FALSE;

    ipcInVideoPrm.inQueParams.prevLinkId       = ipcId[IPC_OUT_VPSS_LINK_IDX];
    ipcInVideoPrm.inQueParams.prevLinkQueId    = 0;
    ipcInVideoPrm.numOutQue                    = 1;
    if (enableAlgLink)
    {
		ipcInVideoPrm.outQueParams[0].nextLink     				 = gVcapModuleContext.dspAlgId[0];
		dspAlgPrm.inQueParams.prevLinkId                         = ipcId[IPC_IN_VIDEO_LINK_IDX];
		dspAlgPrm.inQueParams.prevLinkQueId                      = 0;
		dspAlgPrm.outQueParams[ALG_LINK_FRAMES_OUT_QUE].nextLink = gVencModuleContext.encId;
		dspAlgPrm.enableOSDAlg = TRUE;
	}
	else {
		ipcInVideoPrm.outQueParams[0].nextLink     = gVencModuleContext.encId;
	}
    ipcInVideoPrm.notifyNextLink               = TRUE;
    ipcInVideoPrm.notifyPrevLink               = TRUE;
    ipcInVideoPrm.noNotifyMode                 = FALSE;

    {
        EncLink_CreateParams_Init(&encPrm);

        encPrm.numBufPerCh[0] = NUM_ENCODE_D1_BUFFERS;
        encPrm.numBufPerCh[1] = NUM_ENCODE_CIF_BUFFERS;
        encPrm.numBufPerCh[2] = NUM_ENCODE_MJPEG_BUFFERS;

        /* Primary Stream Params*/
        setH264EncChParamsNrt(&encPrm, 0, gVencModuleContext.vencConfig.numPrimaryChn);

        /* Secondary Stream Params */
        setH264EncChParamsNrt(&encPrm, gVencModuleContext.vencConfig.numPrimaryChn, (gVencModuleContext.vencConfig.numPrimaryChn + gVencModuleContext.vencConfig.numSecondaryChn));

        /* MJPEG  Params */
        setMJPEGEncChParamsNrt(&encPrm, gVencModuleContext.vencConfig.numPrimaryChn + gVencModuleContext.vencConfig.numSecondaryChn, VENC_CHN_MAX);

		if (enableAlgLink)
		{
			encPrm.inQueParams.prevLinkId    = gVcapModuleContext.dspAlgId[0];
			encPrm.inQueParams.prevLinkQueId = ALG_LINK_FRAMES_OUT_QUE;
		}
		else {
			encPrm.inQueParams.prevLinkId    = ipcId[IPC_IN_VIDEO_LINK_IDX];
			encPrm.inQueParams.prevLinkQueId = 0;
		}
        encPrm.outQueParams.nextLink     = gVencModuleContext.ipcBitsOutRTOSId;
    }

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId      = gVencModuleContext.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                   = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink    = gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm, TRUE);

    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkId     = gVencModuleContext.ipcBitsOutRTOSId;
    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkQueId  = 0;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm[0]);

    dspAlgPrm.enableOSDAlg = enableOsd;

    if(enableOsd)
    {
        int chId;

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

    dspAlgPrm.enableSCDAlg              = enableScd;

    if (enableScd)
    {
        UInt32 i, startChId;

        AlgLink_ScdCreateParams *pScdCreatePrm;
        AlgLink_ScdChParams *pScdChPrm;

        pScdCreatePrm = &dspAlgPrm.scdCreateParams;

        pScdCreatePrm->maxWidth               = 176;
        pScdCreatePrm->maxHeight              = 144;
        pScdCreatePrm->maxStride              = 192;
        pScdCreatePrm->numValidChForSCD       = NUM_SCD_CHANNELS;
        pScdCreatePrm->numSecs2WaitB4Init     = 3;
        pScdCreatePrm->numSecs2WaitB4FrmAlert = 1;
        pScdCreatePrm->inputFrameRate         = 5;
        pScdCreatePrm->outputFrameRate        = 5;

        pScdCreatePrm->numSecs2WaitAfterFrmAlert = 1;

        pScdCreatePrm->enableTamperNotify = FALSE;

        /* enable SCD only for CIF CHs */
        startChId = 16;
        for(i=0; i<pScdCreatePrm->numValidChForSCD; i++)
        {
            pScdChPrm = &pScdCreatePrm->chDefaultParams[i];

            pScdChPrm->chId = startChId;
            pScdChPrm->mode                = ALG_LINK_SCD_DETECTMODE_MONITOR_FULL_FRAME;
            pScdChPrm->frmIgnoreLightsON   = FALSE;
            pScdChPrm->frmIgnoreLightsOFF  = FALSE;
            pScdChPrm->frmSensitivity      = ALG_LINK_SCD_SENSITIVITY_VERYHIGH;
            pScdChPrm->frmEdgeThreshold    = 0;
            pScdChPrm->blkNumBlksInFrame   = 0;

            startChId++;
        }
    }
    else
    {
        dspAlgPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink     = SYSTEM_LINK_ID_INVALID;
    }

#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif

    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));

#ifdef     ADD_NSF_AFTER_CAPTURE
    System_linkCreate(gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE_LINK_IDX], &nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX], sizeof(nsfPrm[NSF_AFTER_CAPTURE_LINK_IDX]));
#endif
    System_linkCreate(dupId[CAPTURE_DUP_LINK_IDX], &dupPrm[CAPTURE_DUP_LINK_IDX], sizeof(dupPrm[CAPTURE_DUP_LINK_IDX]));
#ifdef      ADD_DEI_FOR_1_AND_4CH_60_FPS
    System_linkCreate(selectId[DEI_LIVE_SELECT], &selectPrm[DEI_LIVE_SELECT], sizeof(selectPrm[DEI_LIVE_SELECT]));
    System_linkCreate(gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC], &deiPrm[DEI_VIP0_SC_DEI_SC], sizeof(deiPrm[DEI_VIP0_SC_DEI_SC]));
#endif
    System_linkCreate(gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_2], &deiPrm[DEI_VIP0_SC_DEI_SC_2], sizeof(deiPrm[DEI_VIP0_SC_DEI_SC_2]));
    System_linkCreate(gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2], &deiPrm[DEI_BYPASS_VIP1_SC_SC2], sizeof(deiPrm[DEI_BYPASS_VIP1_SC_SC2]));
    //System_linkCreate(gVcapModuleContext.nsfId[NSF_AFTER_DEI_SC_LINK_IDX] , &nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX], sizeof(nsfPrm[NSF_AFTER_DEI_SC_LINK_IDX]));
    System_linkCreate(gVcapModuleContext.sclrId[0], &sclrPrm, sizeof(sclrPrm));
    System_linkCreate(gVcapModuleContext.nsfId[NSF_AFTER_SC5_LINK_IDX] , &nsfPrm[NSF_AFTER_SC5_LINK_IDX], sizeof(nsfPrm[NSF_AFTER_SC5_LINK_IDX]));

    System_linkCreate(mergeId[D1_CIF_QCIF_MERGE_LINK_IDX], &mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX], sizeof(mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX]));

    System_linkCreate(ipcId[IPC_OUT_VPSS_LINK_IDX], &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcId[IPC_IN_VIDEO_LINK_IDX], &ipcInVideoPrm , sizeof(ipcInVideoPrm) );

    if(enableAlgLink)
    {
        /* only create OSD alg */
        System_linkCreate(gVcapModuleContext.dspAlgId[0] , &dspAlgPrm, sizeof(dspAlgPrm));
    }

    System_linkCreate(gVencModuleContext.encId, &encPrm, sizeof(encPrm));

    System_linkCreate(gVencModuleContext.ipcBitsOutRTOSId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(gVencModuleContext.ipcBitsInHLOSId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));

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
        decPrm.chCreateParams[i].numBufPerCh                          = NUM_DECODE_BUFFERS;
    }
    decPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsInRTOSId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcId[IPC_OUT_VIDEO_LINK_IDX];
    decPrm.tilerEnable               = TILER_ENABLE;
    //decPrm.numBufPerPool[0]          = NUM_DECODE_BUFFERS;

    ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                 = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink  = ipcId[IPC_IN_VPSS_LINK_IDX];
    ipcOutVideoPrm.notifyNextLink            = TRUE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = FALSE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcId[IPC_OUT_VIDEO_LINK_IDX];;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink  = mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = TRUE;
    ipcInVpssPrm.noNotifyMode              = FALSE;

    i = 0;
#ifdef      ADD_DEI_FOR_1_AND_4CH_60_FPS
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    i++;
#endif
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = dupId[CAPTURE_DUP_LINK_IDX];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = 1;
    i++;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = ipcId[IPC_IN_VPSS_LINK_IDX];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = 0;
    i++;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                     = i;
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
        swMsPrm[i].outQueParams.nextLink     = gVdisModuleContext.displayId[i];
        swMsPrm[i].maxInputQueLen            = 4;
        swMsPrm[i].maxOutRes                 = VSYS_STD_1080P_60;
        swMsPrm[i].initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[i].resolution;
        swMsPrm[0].numOutBuf                 = NUM_SWMS_0_BUFFERS;
        swMsPrm[1].numOutBuf                 = NUM_SWMS_1_BUFFERS;
        if (i == 1)
		{
            swMsPrm[i].maxOutRes                 = VSYS_STD_PAL;
            swMsPrm[i].initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;

		}	

#ifdef  SYSTEM_USE_TILER
        swMsPrm[i].lineSkipMode = FALSE; // Double pitch not possible in tiler mode; so Line skip not possible
#else
        swMsPrm[i].lineSkipMode = TRUE; // Set to TRUE for Enable low cost scaling
#endif
        swMsPrm[i].enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(VDIS_DEV_HDMI, &swMsPrm[0], TRUE);
        MultiCh_swMsGetDefaultLayoutPrm(VDIS_DEV_SD, &swMsPrm[1], TRUE);

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

    System_linkCreate(ipcId[IPC_OUT_VIDEO_LINK_IDX], &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcId[IPC_IN_VPSS_LINK_IDX]  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(mergeId[LIVE_DECODE_MERGE_LINK_IDX], &mergePrm[LIVE_DECODE_MERGE_LINK_IDX], sizeof(mergePrm[LIVE_DECODE_MERGE_LINK_IDX]));
    System_linkCreate(dupId[LIVE_DECODE_DUP_LINK_IDX], &dupPrm[LIVE_DECODE_DUP_LINK_IDX], sizeof(dupPrm[LIVE_DECODE_DUP_LINK_IDX]));

    for(i=0; i<2; i++)
        System_linkCreate(gVdisModuleContext.swMsId[i]  , &swMsPrm[i], sizeof(swMsPrm[i]));

    for(i=0; i<2; i++)  /* Both tied VENCs HDMI and DVO2 are handled by single link instance */
        System_linkCreate(gVdisModuleContext.displayId[i], &displayPrm[i], sizeof(displayPrm[i]));

    skipOddFieldAtSCNrt(NUM_LIVE_CH);

    MultiCh_progressive16ChVcapVencVdecVdisSwitchLayout(VDIS_DEV_HDMI, &swMsPrm[0].layoutPrm);
    MultiCh_progressive16ChVcapVencVdecVdisSwitchLayout(VDIS_DEV_SD, &swMsPrm[1].layoutPrm);

    {
        MergeLink_InLinkChInfo inChInfo;

        MergeLink_InLinkChInfo_Init(&inChInfo);
        inChInfo.inLinkID = ipcId[IPC_IN_VPSS_LINK_IDX];
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


Void MultiCh_deleteProgressive16ChNrtVcapVencVdecVdis()
{
    UInt32 i;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 selectId[NUM_SELECT_LINK];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    UInt32 ipcBitsOutDSPId;
    mergeId[LIVE_DECODE_MERGE_LINK_IDX]      = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[D1_CIF_QCIF_MERGE_LINK_IDX]      = SYSTEM_VPSS_LINK_ID_MERGE_1;

    dupId[CAPTURE_DUP_LINK_IDX]              = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX]          = SYSTEM_VPSS_LINK_ID_DUP_1;

    ipcOutVpssId                             = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId                             = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId                            = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId                              = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;
    ipcBitsOutDSPId                          = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;

    selectId[DEI_LIVE_SELECT]                = SYSTEM_VPSS_LINK_ID_SELECT_0;

    gVdecModuleContext.ipcBitsOutHLOSId      = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId       = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

    System_linkDelete(gVcapModuleContext.captureId);
    for(i=0; i<NUM_NSF_LINK; i++)
    {
        if (gVcapModuleContext.nsfId[i] != SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gVcapModuleContext.nsfId[i]);
    }

    for(i=0; i<NUM_DEI_LINK; i++)
    {
        if (gVcapModuleContext.deiId[i] != SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gVcapModuleContext.deiId[i]);
    }

    System_linkDelete(gVcapModuleContext.sclrId[0]);

#ifdef      ADD_DEI_FOR_1_AND_4CH_60_FPS
    System_linkDelete(selectId[DEI_LIVE_SELECT]);
#endif

    System_linkDelete(ipcOutVpssId );
    System_linkDelete(ipcInVideoId );

    if(gVsysModuleContext.vsysConfig.enableOsd)
    {
        System_linkDelete(gVcapModuleContext.dspAlgId[0]);
    }

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
        System_linkDelete(gVdisModuleContext.displayId[i]);

    for(i=0; i<NUM_DUP_LINK; i++)
    {
        if (dupId[i] != SYSTEM_LINK_ID_INVALID )
            System_linkDelete(dupId[i]);
    }
    for(i=0; i<NUM_MERGE_LINK; i++)
    {
        if (mergeId[i] != SYSTEM_LINK_ID_INVALID )
           System_linkDelete(mergeId[i]);
    }

    if(gVsysModuleContext.enableFastUsecaseSwitch==FALSE)
    {
        Vcap_deleteVideoDecoder();
    }

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

#if (TILER_ENABLE == FALSE)
    /* Reenable tiler allocator that was disabled by this usecase */
    SystemTiler_enableAllocator();
#endif
}

Int32 MultiCh_progressive16ChNrtVcapVencVdecVdisSetCapFrameRate(VCAP_CHN vcChnId, VCAP_STRM vStrmId, Int32 inputFrameRate, Int32 outputFrameRate)
{
    DeiLink_ChFpsParams params;
    SclrLink_ChFpsParams sclrParams;
    Int32 status = ERROR_NONE;
    UInt32 deiId;

    switch (vStrmId)
    {
        case 0: /* preview path */
        {
            //printf("ATTENTION: Frame rate control in preview path is not supported in this usecase!!\n");
            return ERROR_NONE;
            //break;
        }
        case 1: /* primary path */
        {
            //printf("ATTENTION: Input frame rate is fixed to 7fps(NTSC) or 6fps(PAL)\n inputFrameRate will be forced to 7/6fps!!\n");
            if (outputFrameRate > inputFrameRate/8)
            {
                printf("ATTENTION: Output frame should not be more than 7fps(NTSC) or 6fps(PAL)!!\n");
                return ERROR_NONE;
            }
            deiId = DEI_VIP0_SC_DEI_SC_2;
            params.chId = vcChnId;
            params.inputFrameRate  = inputFrameRate/8;
            params.outputFrameRate = outputFrameRate;
            params.streamId = DEI_LINK_OUT_QUE_VIP_SC;
            break;
        }
        case 2: /* secondary path */
        {
            deiId = DEI_BYPASS_VIP1_SC_SC2;
            params.chId = vcChnId;
            params.inputFrameRate  = inputFrameRate;
            params.outputFrameRate = outputFrameRate;
            params.streamId = DEI_LINK_OUT_QUE_VIP_SC;
            break;
        }
        case 3: /* MJPEG path */
        {

            sclrParams.chId = vcChnId;
            sclrParams.inputFrameRate = inputFrameRate;
            sclrParams.outputFrameRate = outputFrameRate;

            status = System_linkControl(gVcapModuleContext.sclrId[0], SCLR_LINK_CMD_SET_FRAME_RATE,
                               &sclrParams, sizeof(sclrParams), TRUE);
            return status;
            /*
            deiId = DEI_VIP0_SC_DEI_SC_2;
            params.chId = vcChnId;
            params.inputFrameRate  = inputFrameRate/8;
            params.outputFrameRate = outputFrameRate;
            params.streamId = DEI_LINK_OUT_QUE_DEI_SC;
            break;
            */
        }
        default:
        {
            printf("ATTENTION: INVALID vStrmId %d, should be [0, 3]!!\n", vStrmId);
            return status;
        }
    }// end of switch
    if(gVcapModuleContext.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
    {
        status = System_linkControl(gVcapModuleContext.deiId[deiId], DEI_LINK_CMD_SET_FRAME_RATE,
                       &params, sizeof(params), TRUE);
    }

    return status;
}

Int32 MultiCh_progressive16ChNrtVcapVencVdecVdisEnableDisableCapChn(VCAP_CHN vcChnId, VCAP_STRM vStrmId, Bool enableChn)
{
    DeiLink_ChannelInfo channelInfo;
    SclrLink_ChannelInfo SclrchannelInfo;
    Int32 status = ERROR_NONE;
    UInt32 cmdId, deiId;

    if(enableChn)
        cmdId = DEI_LINK_CMD_ENABLE_CHANNEL;
    else
        cmdId = DEI_LINK_CMD_DISABLE_CHANNEL;

    switch (vStrmId)
    {
        case 0: /* preview path */
        {
            //printf("ATTENTION: Disable/Enable control in preview path is not supported in this usecase!!\n");
            return ERROR_NONE;
            //break;
        }
        case 1: /* primary path */
        {
            deiId = DEI_VIP0_SC_DEI_SC_2;
            channelInfo.channelId = vcChnId;
            channelInfo.streamId  = DEI_LINK_OUT_QUE_VIP_SC;
            channelInfo.enable    = enableChn;
            break;
        }
        case 2: /* secondary path */
        {
            deiId = DEI_BYPASS_VIP1_SC_SC2;
            channelInfo.channelId = vcChnId;
            channelInfo.streamId  = DEI_LINK_OUT_QUE_VIP_SC;
            channelInfo.enable    = enableChn;
            break;
        }
        case 3: /* MJPEG path */
        {
            /* MJPEG channel, control SC5 for enable/disable */
            SclrchannelInfo.channelId = vcChnId;
            SclrchannelInfo.enable    = enableChn;
            if(enableChn)
                cmdId = SCLR_LINK_CMD_ENABLE_CHANNEL;
            else
                cmdId = SCLR_LINK_CMD_DISABLE_CHANNEL;

            if(gVcapModuleContext.sclrId[0]!=SYSTEM_LINK_ID_INVALID)
            {
                status = System_linkControl(
                                        gVcapModuleContext.sclrId[0],
                                        cmdId,
                                        &(SclrchannelInfo),
                                        sizeof(SclrLink_ChannelInfo),
                                        TRUE
                                        );
            }
            return status;
            /*
            deiId = DEI_VIP0_SC_DEI_SC_2;
            channelInfo.channelId = vcChnId;
            channelInfo.streamId  = DEI_LINK_OUT_QUE_DEI_SC;
            channelInfo.enable    = enableChn;
            break;
            */
        }
        default:
        {
            printf("ATTENTION: INVALID vStrmId %d, should be [0, 3]!!\n", vStrmId);
            return status;
        }
    }// end of switch
    if(gVcapModuleContext.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
    {
        status = System_linkControl(gVcapModuleContext.deiId[deiId],
                                    cmdId,
                                    &(channelInfo),
                                    sizeof(DeiLink_ChannelInfo),
                                    TRUE
                                    );
    }

    return status;
}

Int32 MultiCh_progressive16ChNrtVcapVencVdecVdisSetOutRes(VCAP_CHN vcChnId, VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam)
{
    DeiLink_chDynamicSetOutRes params = {0};
    Int32 status = ERROR_NONE;
    UInt32 deiId;

    params.chId = vcChnId;
    switch ((UInt32)psCapChnDynaParam->chDynamicRes.pathId)
    {
        case 0: /* preview path */
        {
            //printf("ATTENTION: Dynamic resolution change in preview path is not supported in this usecase!!\n");
            return ERROR_NONE;
            //break;
        }
        case 1: /* primary path */
        {
            deiId        = DEI_VIP0_SC_DEI_SC_2;
            params.queId = DEI_LINK_OUT_QUE_VIP_SC;
            break;
        }
        case 2: /* secondary path */
        {
            deiId        = DEI_BYPASS_VIP1_SC_SC2;
            params.queId = DEI_LINK_OUT_QUE_VIP_SC;
            break;
        }
        default:
        {
            printf("ATTENTION: INVALID pathId %d, should be [0, 2]!!\n", psCapChnDynaParam->chDynamicRes.pathId);
            return status;
        }
    }// end of switch
    if(gVcapModuleContext.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
    {
        status = System_linkControl(
                                    gVcapModuleContext.deiId[deiId],
                                    DEI_LINK_CMD_GET_OUTPUTRESOLUTION,
                                    &(params),
                                    sizeof(params),
                                    TRUE
                                    );
        params.width = SystemUtils_align(psCapChnDynaParam->chDynamicRes.width, 16);
        params.height = SystemUtils_align(psCapChnDynaParam->chDynamicRes.height, 1);
        status = System_linkControl(
                                    gVcapModuleContext.deiId[deiId],
                                    DEI_LINK_CMD_SET_OUTPUTRESOLUTION,
                                    &(params),
                                    sizeof(params),
                                    TRUE
                                    );
    }

    return status;
}

Int32 MultiCh_progressive16ChNrtVcapVencVdecVdisGetOutRes(VCAP_CHN vcChnId, VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam)
{
  DeiLink_chDynamicSetOutRes params = {0};
  Int32 status = ERROR_NONE;
  UInt32 deiId;

  switch ((UInt32)psCapChnDynaParam->chDynamicRes.pathId)
  {
      case 0: /* preview path */
      {
          //printf("ATTENTION: Dynamic resolution change in preview path is not supported in this usecase!!\n");
          return ERROR_NONE;
          //break;
      }
      case 1: /* primary path */
      {
          deiId        = DEI_VIP0_SC_DEI_SC_2;
          params.queId = DEI_LINK_OUT_QUE_VIP_SC;
          break;
      }
      case 2: /* secondary path */
      {
          deiId        = DEI_BYPASS_VIP1_SC_SC2;
          params.queId = DEI_LINK_OUT_QUE_VIP_SC;
          break;
      }
      default:
      {
          printf("ATTENTION: INVALID pathId %d, should be [0, 2]!!\n", psCapChnDynaParam->chDynamicRes.pathId);
          return status;
      }
  }// end of switch
  params.chId = vcChnId;

  status = System_linkControl(
                              gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2],
                              DEI_LINK_CMD_GET_OUTPUTRESOLUTION,
                              &(params),
                              sizeof(params),
                              TRUE
                              );
  psCapChnDynaParam->chDynamicRes.width = params.width;
  psCapChnDynaParam->chDynamicRes.height = params.height;
  return status;

}

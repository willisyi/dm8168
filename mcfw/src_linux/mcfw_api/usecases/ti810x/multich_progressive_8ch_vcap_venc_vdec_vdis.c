/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/* This usecase assumes that all 3 outputs - D1 /CIF <h.264> + QCIF <h.264> + D1 <MJPEG> are enabled */
/*------------------------------ D1/CIF <h.264> + QCIF <h.264> + D1 <MJPEG> -------------------------------------


                                                 Capture (YUV422I) 8CH D1 60fps
                                                              |
                                                              |
                                                        NSF_AFTER_CAPTURE
                                                            YUV420
                                                              |
              -----------------------------------------------------------------------------------------------------------
                                                       CAPTURE_DUP_LINK_IDX
              -----------------------------------------------------------------------------------------------------------
               |0                      |3                            |1                                               |2
               |                       |                             |                                                |
               |                       | 8xD1                        | 8xD1                                           |
               |                 ------------                   -------------------------------------                 |
               |                 Select Link                               Select Link                                |
               |                 ------------                   -------------------------------------                 |
               |                       | 4xD1                       0| 2xD1                 1|                        |
               |                       |                             |                       | 6xD1                   | 8xD1 420 30fps (Even fields)
               |           -------------------------                 |                       | 30fps                  |
 8D1 420 30fps |              DEI_0  <name-DEI2>           -------------------               | (Even fields)          |
               |                <DEI-SC1>                  DEI_HQ_0 <name-DEI3>              |                        |
  (Even fields)|           -------------------------       VIP0-SC3    DEI-SC1               |                   -----------
               |                       | 4xD1              -------------------               |                    SCALAR-SC5
               |                       | 30/60fps              |           |        ---------------------------   -----------
               |                       |                       |    -------------        DEI_1 <name-DEI0>            |
               |                       |                       |    NSF_FOR_2QCIF    (VIP1-SC4)    DEI_SC2            |8D1 422 1fps MJPEG
               |                       |                       |     YUV420|        ---------------------------       |(YUV422I)
               |                       |                       |    -------------      |              |               |
               |                       |                       |           |           |              |               |
               |                       |                       |           |           |        ----------------  ----------------
               |                       |                       |           |           |        NSF_AFTER_DEI_SC  NSF_AFTER_SCALAR
               |                       |                       |           |           |           (YUV420)         (YUV420)
               |                       |                       |           |           |        ----------------  ----------------
               |                       |                       |           |           |              |               |
               |                       |                       |           |           |              |               |
               |                       |                       |           |           |              |               |
               |                       |                      0|          2|          1|              |3              | 4
               |                       |            CH 0.4  2D1|      2QCIF|      6CIF | CH 1..3      |6QCIF          |
               |                       |                  H.264|      H.264|     H.264 |    5..7      |H.264          |8D1 422 1fps MJPEG
               |                       |                  30fps|     30fps |     30fps |              |30fps          |
               |                       |               -------------------------------------------------------------------------------
               |                       |                                                      D1_CIF_QCIF_MERGE_LINK_IDX
               |                       |               -------------------------------------------------------------------------------
               |                       |                                                 |
               |                       |                                                 |
               |                       |                                                 |
               |                       |                                             ALG_LINK <OSD, SCD Algs>
               |                       |                                                 |
               |                       |                                                 |
       CH4..11 |                       |                                                 |
               |                       |CH0..3                                   IPCM3OUT(VPSS)---IPCM3IN(VID)---ENC---IPCBITS_RTOSOUT(VID)---IPCBITS_HLOSIN(HOST)---FILEOUT
               |                       |                                                                                                                               |
               |                       |                                                                                                                               |
               |                       |                   CH12..19                                                                                                    |
               |                       |                   --------------IPCM3IN(VPSS)------IPCM3OUT(VID)--------DEC--------IPCBITS_RTOSIN(VID)---------IPCBITS_HLOSOUT(HOST)
               |                       |4D1                |
        8D1    |                       |30/60fps           |
        30fps  |                       |                   |2D1+6CIF <or 8CIF> 30fps
            ----------------------------------------------------
                       LIVE_DECODE_MERGE_LINK_IDX
            ----------------------------------------------------
                                       |
                                       |CH0..19
                            ------------------------
                            LIVE_DECODE_DUP_LINK_IDX
                            ------------------------
                                      |||
                                      |||
                      +---------------+|+----------------+
                      |                                  |
                      |                                  |
                 SW Mosaic 1                       SW Mosaic 0
              (DEI-SC1 YUV422I)                     (SC5 YUV422I)
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


#include "mcfw/interfaces/link_api/system_tiler.h"

// Disabling tiler until memory alloc issues are fixed. Low performance will be observed in non tiler mode
#define TILER_ENABLE        TRUE

#define NUM_SCD_CHANNELS    (8)

/* =============================================================================
 * Externs
 * =============================================================================
 */


/* =============================================================================
 * Use case code
 * =============================================================================
 */


#define     NUM_NSF_LINK                     4
#define     NSF_AFTER_BYPASS_DEI_SC_LINK     0
#define     NSF_AFTER_SCALAR_LINK            1
#define     NSF_FOR_2QCIF                    2
#define     NSF_AFTER_CAPTURE                3

#define     NUM_LIVE_CH                      8
#define     NUM_DEI_LIVE_CH                  4
#define     NUM_DEI_D1_CH                    2
#define     NUM_DEI_CIF_CH                   6

#define     NUM_SELECT_LINK                  2
#define     DEI_LIVE_SELECT                  0
#define     DEI_2_D1_CH_SELECT               1

#define     NUM_DEI_LINK                     3
#define     DEI_VIP0_SC_DEI_SC_FOR_2D1       0
#define     DEI_BYPASS_VIP1_SC_SC2           1
#define     DEI_VIP0_SC_DEI_SC               2

#define     NUM_MERGE_LINK                   2
#define     D1_CIF_QCIF_MERGE_LINK_IDX       0
#define     LIVE_DECODE_MERGE_LINK_IDX       1

#define     NUM_DUP_LINK                     2
#define     CAPTURE_DUP_LINK_IDX             0
#define     LIVE_DECODE_DUP_LINK_IDX         1

#define     NUM_CAPTURE_BUFFERS              8
#define     NUM_NSF_BUFFERS                  6
#define     NUM_ENCODE_D1_BUFFERS            6
#define     NUM_ENCODE_CIF_BUFFERS           6
#define     NUM_DECODE_BUFFERS               4
#define     NUM_IPC_OUT_BUFFERS              8
#define     NUM_SWMS_MAX_BUFFERS             6
#define     NUM_PRI_ENC_IN_BUFFERS           4
#define     NUM_SEC_ENC_IN_BUFFERS           4

#define     NUM_IPC_LINK                     4
#define     IPC_OUT_VPSS_LINK_IDX            0
#define     IPC_IN_VPSS_LINK_IDX             1
#define     IPC_OUT_VIDEO_LINK_IDX           2
#define     IPC_IN_VIDEO_LINK_IDX            3

#define     NUM_CAPTURE_DEVICES              2

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 24,
        .EncChList = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
            20, 21, 22, 23},
        .DecNumCh  = 8,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7},
    },
};

static const Uint32 m_D1_Ch [NUM_DEI_D1_CH]    = {0, 4};
static const Uint32 m_CIF_Ch[NUM_DEI_CIF_CH]   = {1, 2, 3, 5, 6, 7};

/* Whether or not each channel should be de-interlaced or not */
static       Bool   m_D1_Ch_DEI[NUM_DEI_D1_CH] = {TRUE, TRUE};

static Bool m_PreviewFullFpsCh[NUM_LIVE_CH];



static Void set_DEI_VIP0_SC_2D1_CH_params  (
    SelectLink_CreateParams *selectPrm,
    DeiLink_CreateParams    *deiPrm,
    Uint32                   prevLinkId,
    Uint32                   prevLinkQ,
    Uint32                   nextLinkIdFor_DEI_SC,
    Uint32                   nextLinkIdFor_VIP_SC,
    UInt32                   deiSelectId,
    UInt32                   deiId)
{
    Int32 chId;

    selectPrm->numOutQue                 = 2;
    
    selectPrm->inQueParams.prevLinkId    = prevLinkId;
    selectPrm->inQueParams.prevLinkQueId = prevLinkQ;

    selectPrm->outQueParams[0].nextLink = deiId;
    selectPrm->outQueChInfo[0].outQueId = 0;
    selectPrm->outQueChInfo[0].numOutCh = NUM_DEI_D1_CH;

    for (chId = 0; chId < NUM_DEI_D1_CH; chId++)
    {
        selectPrm->outQueChInfo[0].inChNum[chId] = m_D1_Ch[chId];
    }

    deiPrm->inQueParams.prevLinkId    = deiSelectId;
    deiPrm->inQueParams.prevLinkQueId = 0;

    for (chId = 0; chId < NUM_DEI_D1_CH; chId++)
    {
        /* QCIF */
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].scaleMode                     = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.numerator    = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.denominator  = 4;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.numerator   = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.denominator = 4;

        /* D1 */
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].scaleMode                     = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.numerator    = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.denominator  = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.numerator   = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.denominator = 1;
    }

    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC] = NUM_PRI_ENC_IN_BUFFERS;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC] = NUM_SEC_ENC_IN_BUFFERS;

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC] = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC] = TRUE;

    deiPrm->outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink = nextLinkIdFor_DEI_SC;
    deiPrm->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = nextLinkIdFor_VIP_SC;

    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TILER_ENABLE;
    deiPrm->comprEnable          = FALSE;
    deiPrm->setVipScYuv422Format = FALSE;

    deiPrm->inputDeiFrameRate  = 60;
    deiPrm->outputDeiFrameRate = 30;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 30;
    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;

    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 30;      // QCIF
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;      // D1
    
    deiPrm->enableDeiForceBypass = FALSE;
}



static Void set_DEI_BYPASS_VIP1_SC_SC2_params(
    SelectLink_CreateParams *selectPrm,
    DeiLink_CreateParams    *deiPrm,
    Uint32                   prevLinkId,
    Uint32                   prevLinkQ,
    Uint32                   nextLinkIdFor_DEI_SC,
    Uint32                   nextLinkIdFor_VIP_SC,
    UInt32                   deiSelectId,
    UInt32                   deiId)
{
    Int32 chId;

    /*
        Below params set in set_DEI_VIP0_SC_2D1_CH_params()

        set_DEI_VIP0_SC_2D1_CH_params() MUST be called before this API

        selectPrm->numOutQue;

        selectPrm->inQueParams.prevLinkId ;
        selectPrm->inQueParams.prevLinkQueId;
    */

    selectPrm->outQueParams[1].nextLink = deiId;

    selectPrm->outQueChInfo[1].outQueId = 1;
    selectPrm->outQueChInfo[1].numOutCh = NUM_DEI_CIF_CH;

    for (chId = 0; chId < NUM_DEI_CIF_CH; chId++)
    {
        selectPrm->outQueChInfo[1].inChNum[chId] = m_CIF_Ch[chId];
    }

    deiPrm->inQueParams.prevLinkId    = deiSelectId;
    deiPrm->inQueParams.prevLinkQueId = 1;

    for (chId = 0; chId < NUM_DEI_CIF_CH; chId++)
    {
        /* QCIF */
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].scaleMode                     = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.numerator    = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.widthRatio.denominator  = 4;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.numerator   = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId].ratio.heightRatio.denominator = 2;

        /* Main ch */
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].scaleMode                     = DEI_SCALE_MODE_RATIO;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.numerator    = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.widthRatio.denominator  = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.numerator   = 1;
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId].ratio.heightRatio.denominator = 1;
    }

    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_DEI_SC] = NUM_PRI_ENC_IN_BUFFERS;
    deiPrm->numBufsPerCh[DEI_LINK_OUT_QUE_VIP_SC] = NUM_SEC_ENC_IN_BUFFERS;

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC] = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC] = TRUE;

    deiPrm->outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink = nextLinkIdFor_DEI_SC;
    deiPrm->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = nextLinkIdFor_VIP_SC;

    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TILER_ENABLE;
    deiPrm->comprEnable           = FALSE;
    deiPrm->setVipScYuv422Format  = FALSE;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 30;
    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;

    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 30;      // QCIF
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;      // CIF

    deiPrm->enableDeiForceBypass = TRUE;
}



// For preview
static Void set_DEI_VIP0_SC_DEI_SC_params (
    SelectLink_CreateParams *selectPrm,
    DeiLink_CreateParams    *deiPrm,
    Uint32                   prevLinkId,
    Uint32                   prevLinkQ,
    Uint32                   nextLinkIdFor_DEI_SC,
    Uint32                   nextLinkIdFor_VIP_SC,
    UInt32                   deiSelectId,
    UInt32                   deiId)
{
    Int32 chId;

    selectPrm->numOutQue = 1;

    selectPrm->inQueParams.prevLinkId    = prevLinkId;
    selectPrm->inQueParams.prevLinkQueId = prevLinkQ;

    selectPrm->outQueParams[0].nextLink = deiId;
    selectPrm->outQueChInfo[0].outQueId = 0;
    selectPrm->outQueChInfo[0].numOutCh = NUM_DEI_LIVE_CH;

    for (chId = 0; chId < NUM_DEI_LIVE_CH; chId++)
    {
        selectPrm->outQueChInfo[0].inChNum[chId] = chId;
    }

    deiPrm->inQueParams.prevLinkId    = deiSelectId;
    deiPrm->inQueParams.prevLinkQueId = 0;

    /* Set Output Scaling at DEI based on ratio */
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator    = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator  = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator   = 1;
    deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator = 1;
    for (chId = 1; chId < DEI_LINK_MAX_CH; chId++)
    {
        deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = 
            deiPrm->outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];
    }

    deiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC] = TRUE;
    deiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC] = TRUE;

    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_DEI_SC] = FALSE;
    deiPrm->generateBlankOut[DEI_LINK_OUT_QUE_VIP_SC] = TRUE;

    deiPrm->outQueParams[DEI_LINK_OUT_QUE_DEI_SC].nextLink = nextLinkIdFor_DEI_SC;
    deiPrm->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = nextLinkIdFor_VIP_SC;

    deiPrm->tilerEnable[DEI_LINK_OUT_QUE_VIP_SC] = TILER_ENABLE;
    deiPrm->comprEnable          = FALSE;
    deiPrm->setVipScYuv422Format = FALSE;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_DEI_SC]  = 60;
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_DEI_SC] = 60;

    deiPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]  = 60;
    deiPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 30;

    deiPrm->enableDeiForceBypass = FALSE;
}



static Void setLinkIds(
        Bool   enableAlgLink, 
        UInt32 mergeId[], 
        UInt32 dupId[], 
        UInt32 ipcId[], 
        UInt32 selectId[])
{
    gVcapModuleContext.captureId = SYSTEM_LINK_ID_CAPTURE;

    if (enableAlgLink)
    {
        gVcapModuleContext.dspAlgId[0] = SYSTEM_LINK_ID_VIDEO_ALG_0;
    }

    gVcapModuleContext.nsfId[NSF_AFTER_BYPASS_DEI_SC_LINK] = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.nsfId[NSF_AFTER_SCALAR_LINK]        = SYSTEM_LINK_ID_NSF_1;
    gVcapModuleContext.nsfId[NSF_FOR_2QCIF]                = SYSTEM_LINK_ID_NSF_2;
    gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE]            = SYSTEM_LINK_ID_NSF_3;

    selectId[DEI_LIVE_SELECT]    = SYSTEM_VPSS_LINK_ID_SELECT_0;
    selectId[DEI_2_D1_CH_SELECT] = SYSTEM_VPSS_LINK_ID_SELECT_1;

    gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC]         = SYSTEM_LINK_ID_DEI_0;
    gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_FOR_2D1] = SYSTEM_LINK_ID_DEI_1;

    gVcapModuleContext.sclrId[0] = SYSTEM_LINK_ID_SCLR_INST_0;
    gVencModuleContext.encId     = SYSTEM_LINK_ID_VENC_0;
    gVdecModuleContext.decId     = SYSTEM_LINK_ID_VDEC_0;

    gVdisModuleContext.swMsId[0] = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1] = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; /* ON AND OFF CHIP HDMI */
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_2; /* SDTV */

    mergeId[LIVE_DECODE_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[D1_CIF_QCIF_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_1;

    dupId[CAPTURE_DUP_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX] = SYSTEM_VPSS_LINK_ID_DUP_1;

    ipcId[IPC_OUT_VPSS_LINK_IDX]  = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcId[IPC_IN_VIDEO_LINK_IDX]  = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcId[IPC_OUT_VIDEO_LINK_IDX] = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcId[IPC_IN_VPSS_LINK_IDX]   = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    gVencModuleContext.ipcBitsOutRTOSId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    gVencModuleContext.ipcBitsInHLOSId  = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.ipcBitsOutHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;
    gVcapModuleContext.ipcBitsInHLOSId  = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1;
}


#if 0   // [TODO] Is this function needed?
static Void setDspLinksParams(
        IpcFramesOutLinkRTOS_CreateParams *ipcFramesOutVpssPrm,
        IpcFramesInLinkRTOS_CreateParams  *ipcFramesInDspPrm,
        UInt32                             prevLinkId,
        UInt32                             prevLinkQId,
        UInt32                             ipcId[])
{
    ipcFramesOutVpssPrm->baseCreateParams.inQueParams.prevLinkId    = prevLinkId;
    ipcFramesOutVpssPrm->baseCreateParams.inQueParams.prevLinkQueId = prevLinkQId;
    ipcFramesOutVpssPrm->baseCreateParams.outQueParams[0].nextLink  = 
        ipcId[IPC_OUT_VPSS_LINK_IDX];
    ipcFramesOutVpssPrm->baseCreateParams.processLink               = 
        gVcapModuleContext.ipcFramesInDspId[0];
    ipcFramesOutVpssPrm->baseCreateParams.notifyPrevLink            = TRUE;
    ipcFramesOutVpssPrm->baseCreateParams.notifyNextLink            = TRUE;
    ipcFramesOutVpssPrm->baseCreateParams.notifyProcessLink         = TRUE;
    ipcFramesOutVpssPrm->baseCreateParams.noNotifyMode              = FALSE;
    ipcFramesOutVpssPrm->baseCreateParams.numOutQue                 = 1;

    ipcFramesInDspPrm->baseCreateParams.inQueParams.prevLinkId      = 
        gVcapModuleContext.ipcFramesOutVpssId[0];
    ipcFramesInDspPrm->baseCreateParams.inQueParams.prevLinkQueId   = 0;
    ipcFramesInDspPrm->baseCreateParams.outQueParams[0].nextLink    = 
        gVcapModuleContext.dspAlgId[0];
    ipcFramesInDspPrm->baseCreateParams.notifyPrevLink              = TRUE;
    ipcFramesInDspPrm->baseCreateParams.notifyNextLink              = TRUE;
    ipcFramesInDspPrm->baseCreateParams.noNotifyMode                = FALSE;
    ipcFramesInDspPrm->baseCreateParams.numOutQue                   = 1;
}
#endif


static Void setMJPEGEncChParams(
        EncLink_CreateParams *encPrm, 
        Int32                 startChNum, 
        Int32                 endChNum)
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
         pLinkChPrm->maxBitRate             = 0;
         pLinkChPrm->encodingPreset         = 0;
         pLinkChPrm->rateControlPreset      = 0;

         pLinkDynPrm->intraFrameInterval    = 0;
         pLinkDynPrm->targetBitRate         = 100*1000;
         pLinkDynPrm->interFrameInterval    = 0;
         pLinkDynPrm->mvAccuracy            = 0;
         pLinkDynPrm->inputFrameRate        = 1; 
         pLinkDynPrm->qpMin                 = 0;
         pLinkDynPrm->qpMax                 = 0;
         pLinkDynPrm->qpInit                = -1;
         pLinkDynPrm->vbrDuration           = 0;
         pLinkDynPrm->vbrSensitivity        = 0;
    }
}



static Void setH264EncChParams(
        EncLink_CreateParams *encPrm, 
        Int32                 startChNum, 
        Int32                 endChNum)
{
    Int32 i;

    EncLink_ChCreateParams   *pLinkChPrm;
    EncLink_ChDynamicParams  *pLinkDynPrm;
    VENC_CHN_DYNAMIC_PARAM_S *pDynPrm;
    VENC_CHN_PARAMS_S        *pChPrm;

    for (i = startChNum; i < endChNum; i++)
    {
        pLinkChPrm  = &encPrm->chCreateParams[i];
        pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

        pChPrm      = &gVencModuleContext.vencConfig.encChannelParams[i];
        pDynPrm     = &pChPrm->dynamicParam;

        pLinkChPrm->format                  = IVIDEO_H264HP;
        pLinkChPrm->profile                 = 
            gVencModuleContext.vencConfig.h264Profile[i];
        pLinkChPrm->dataLayout              = IVIDEO_FIELD_SEPARATED;
        pLinkChPrm->fieldMergeEncodeEnable  = FALSE;
        pLinkChPrm->enableAnalyticinfo      = pChPrm->enableAnalyticinfo;
        pLinkChPrm->maxBitRate              = pChPrm->maxBitRate;
        pLinkChPrm->encodingPreset          = pChPrm->encodingPreset;
        pLinkChPrm->rateControlPreset       = pChPrm->rcType;
        pLinkChPrm->enableHighSpeed         = TRUE;

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



static void skipOddFieldAtSC(UInt32 numChs)
{
    SclrLink_chDynamicSkipFidType prm;
    UInt32 chId;

    for (chId = 0; chId < numChs; chId++)
    {
        prm.chId = chId;
        prm.fidType = 1;

        System_linkControl(
            gVcapModuleContext.sclrId[0],
            SCLR_LINK_CMD_SKIP_FID_TYPE,
            &prm,
            sizeof(prm),
            TRUE);
    }
}



static void selectCaptureFullFpsChs(UInt32 numChs, UInt32 fullFpsChs[])
{
    CaptureLink_SkipOddFields prm;
    UInt32 chId;

    prm.queId                        = 0;
    prm.skipOddFieldsChBitMask = 0xFFFF;
    prm.oddFieldSkipRatio = CAPTURE_LINK_ODD_FIELD_SKIP_ALL;

    for (chId = 0; chId < numChs; chId++)
    {
        prm.skipOddFieldsChBitMask &= ~(1 << fullFpsChs[chId]);
    }

    /* Don't skip for ch of D1 encode, 
       [TODO] Can skip for ch for 2CIF encode. Then, DEI should be bypassed for ch */
    if (gVsysModuleContext.vsysConfig.systemUseCase == 
            VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH)
    {
        for (chId = 0; chId < NUM_DEI_D1_CH; chId++)
        {
            //if (m_D1_Ch_DEI[chId])
            {
                prm.skipOddFieldsChBitMask &= ~(1 << m_D1_Ch[chId]);
            }
        }
    }

    System_linkControl(
        gVcapModuleContext.captureId,
        CAPTURE_LINK_CMD_SKIP_ODD_FIELDS,
        &prm,
        sizeof(prm),
        TRUE);
}



static void liveDeiFlush(UInt32 numCh, UInt32 flushChs[])
{
    DeiLink_ChFlushParams prm;
    UInt32 chId;

    for (chId = 0; chId < numCh; chId++)
    {
        prm.chId = flushChs[chId];

        System_linkControl(
            gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC],
            DEI_LINK_CMD_FLUSH_CHANNEL_INPUT,
            &prm,
            sizeof(prm),
            TRUE);
    }
}



static void selectDeiLiveChs(UInt32 numSelectChs, UInt32 selectChs[])
{
    UInt32 selectId[NUM_SELECT_LINK], chId, numFlushChs, flushChs[NUM_DEI_LIVE_CH];
    SelectLink_OutQueChInfo prm;

    if (numSelectChs > NUM_DEI_LIVE_CH)
    {
        return;
    }

    selectId[DEI_LIVE_SELECT] = SYSTEM_VPSS_LINK_ID_SELECT_0;

    prm.outQueId = 0;
    prm.numOutCh = numSelectChs;
    for (chId = 0; chId < numSelectChs; chId++)
    {
        prm.inChNum[chId] = selectChs[chId];
    }

    /* skip odd fields at capture if not need in the use-case to save DDR BW */
    selectCaptureFullFpsChs(numSelectChs, selectChs);

    System_linkControl(
        selectId[DEI_LIVE_SELECT],
        SELECT_LINK_CMD_SET_OUT_QUE_CH_INFO,
        &prm,
        sizeof(prm),
        TRUE);

    numFlushChs = NUM_DEI_LIVE_CH - numSelectChs;

    for (chId = 0; chId < numFlushChs; chId++)
    {
        flushChs[chId] = (NUM_DEI_LIVE_CH - 1) - chId;
    }

    liveDeiFlush(numFlushChs, flushChs);
}



static void getDeiLiveChIds(UInt32 *numSelectChs, UInt32 selectChs[])
{
    SelectLink_OutQueChInfo prm;
    UInt32 selectId[NUM_SELECT_LINK], chId;

    selectId[DEI_LIVE_SELECT]               = SYSTEM_VPSS_LINK_ID_SELECT_0;

    prm.outQueId = 0;
    prm.numOutCh = 0;

    System_linkControl(
        selectId[DEI_LIVE_SELECT],
        SELECT_LINK_CMD_GET_OUT_QUE_CH_INFO,
        &prm,
        sizeof(prm),
        TRUE
    );

    *numSelectChs = prm.numOutCh;
    for (chId = 0; chId < *numSelectChs; chId++)
    {
        selectChs[chId] = prm.inChNum[chId];
    }
}



static void set_DEI_VIP0_SC_DEI_SC_outputFPS(UInt32 inputFPS, UInt32 outputFPS)
{
    UInt32 deiId, chId;
    DeiLink_ChFpsParams deiFrameRate;

    deiId = gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC];

    for (chId = 0; chId < NUM_DEI_LIVE_CH; chId++)
    {
        deiFrameRate.chId            = chId;
        deiFrameRate.streamId        = DEI_LINK_OUT_QUE_DEI_SC;
        deiFrameRate.inputFrameRate  = inputFPS;
        deiFrameRate.outputFrameRate = outputFPS;

        System_linkControl(
            deiId,
            DEI_LINK_CMD_SET_FRAME_RATE,
            &deiFrameRate,
            sizeof(deiFrameRate),
            TRUE);
    }
}



/* This usecase assumes secondary out, MJPEG are enabled */
Void MultiCh_createProgressive8ChVcapVencVdecVdis()
{
    CaptureLink_CreateParams          capturePrm;
    NsfLink_CreateParams              nsfPrm[NUM_NSF_LINK];
    DeiLink_CreateParams              deiPrm[NUM_DEI_LINK];
    MergeLink_CreateParams            mergePrm[NUM_MERGE_LINK];
    DupLink_CreateParams              dupPrm[NUM_DUP_LINK];
    static SwMsLink_CreateParams      swMsPrm[VDIS_DEV_MAX];
    DisplayLink_CreateParams          displayPrm[VDIS_DEV_MAX];
    IpcLink_CreateParams              ipcOutVpssPrm;
    IpcLink_CreateParams              ipcInVpssPrm;
    IpcLink_CreateParams              ipcOutVideoPrm;
    IpcLink_CreateParams              ipcInVideoPrm;
    EncLink_CreateParams              encPrm;
    DecLink_CreateParams              decPrm;
    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams    ipcBitsInHostPrm[2];
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    CaptureLink_VipInstParams         *pCaptureInstPrm;
    CaptureLink_OutParams             *pCaptureOutPrm;
    IpcFramesInLinkRTOS_CreateParams  ipcFramesInDspPrm;
    IpcFramesOutLinkRTOS_CreateParams ipcFramesOutVpssPrm;
    AlgLink_CreateParams              dspAlgPrm;
    SclrLink_CreateParams             sclrPrm;
    System_LinkInfo                   bitsProducerLinkInfo;
    SelectLink_CreateParams           selectPrm[NUM_SELECT_LINK];
    UInt32                            mergeId[NUM_MERGE_LINK];
    UInt32                            dupId[NUM_DUP_LINK];
    UInt32                            ipcId[NUM_IPC_LINK];
    UInt32                            selectId[NUM_SELECT_LINK];
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutDspPrm;
    UInt32                            ipcBitsOutDSPId;
    VCAP_VIDDEC_PARAMS_S              vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];
    UInt32                            vipInstId;
    UInt32                            i;
    Bool                              enableAlgLink;
    Bool                              enableOsd;
    Bool                              enableScd;


    enableScd = gVsysModuleContext.vsysConfig.enableScd;
    enableOsd = gVsysModuleContext.vsysConfig.enableOsd;

    enableScd = TRUE;

    enableAlgLink = enableScd | enableOsd;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams, ipcBitsOutHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams, ipcBitsOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams, ipcBitsOutDspPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams, ipcBitsInHostPrm[0]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams, ipcBitsInHostPrm[1]);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams, ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(DecLink_CreateParams, decPrm);
    MULTICH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams, ipcFramesInDspPrm);
    MULTICH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams, ipcFramesOutVpssPrm);
    MULTICH_INIT_STRUCT(EncLink_CreateParams, encPrm);
    MULTICH_INIT_STRUCT(AlgLink_CreateParams, dspAlgPrm);
    MULTICH_INIT_STRUCT(SclrLink_CreateParams, sclrPrm);

    for (i = 0; i < VDIS_DEV_MAX; i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams, displayPrm[i]);
        MULTICH_INIT_STRUCT(SwMsLink_CreateParams, swMsPrm[i]);
    }

    for (i = 0; i < NUM_NSF_LINK; i++)
    {
        MULTICH_INIT_STRUCT(NsfLink_CreateParams, nsfPrm[i]);
    }

    for (i = 0; i < NUM_DEI_LINK; i++)
    {
        MULTICH_INIT_STRUCT(DeiLink_CreateParams, deiPrm[i]);
    }

    for (i = 0; i < NUM_SELECT_LINK; i++)
    {
        MULTICH_INIT_STRUCT(SelectLink_CreateParams, selectPrm[i]);
    }

    printf("\n********* Entered usecase 8CH <810x> Enc/Dec OSD %s SCD %s \n\n",
            enableOsd == TRUE ? "Enabled" : "Disabled",
            enableScd == TRUE ? "Enabled" : "Disabled");

    MultiCh_detectBoard();

#if     (TILER_ENABLE == FALSE)
    /* Temp Fix - Disable tiler allocator for this usecase so that tiler memory can
       be reused for non-tiled allocation */
    SystemTiler_disableAllocator();
#endif
    

    System_linkControl(
            SYSTEM_LINK_ID_M3VPSS, 
            SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES, 
            NULL, 
            0, 
            TRUE);
    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO, 
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL, 
        &systemVid_encDecIvaChMapTbl, 
        sizeof(SystemVideo_Ivahd2ChMap_Tbl), 
        TRUE);

    ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;
    setLinkIds(enableAlgLink, mergeId, dupId, ipcId, selectId);

    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[1].numSwMsInst = 1;
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEIHQ_SC_NO_DEI;

    CaptureLink_CreateParams_Init(&capturePrm);
    capturePrm.outQueParams[0].nextLink =
        gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE];

    /* This is for TVP5158 Audio Channels - Change it to 16 if there are 16 audio
       channels connected in cascade */
    capturePrm.numVipInst       = 2;
    capturePrm.tilerEnable      = FALSE;
    capturePrm.numBufsPerCh     = NUM_CAPTURE_BUFFERS;
    for (vipInstId = 0; vipInstId < capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                 = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId      = SYSTEM_CAPTURE_INST_VIP0_PORTA + vipInstId;
        pCaptureInstPrm->videoDecoderId = SYSTEM_DEVICE_VID_DEC_TVP5158_DRV;
        pCaptureInstPrm->inDataFormat   = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard       = SYSTEM_STD_MUX_4CH_D1;
        pCaptureInstPrm->numOutput      = 1;

        pCaptureOutPrm                  = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat      = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable        = FALSE;
        pCaptureOutPrm->scOutWidth      = 0;
        pCaptureOutPrm->scOutHeight     = 0;
        pCaptureOutPrm->outQueId        = 0;
     }

    for (i = 0; i < NUM_CAPTURE_DEVICES; i++)
    {
        vidDecVideoModeArgs[i].videoIfMode            = 
            DEVICE_CAPT_VIDEO_IF_MODE_8BIT;
        vidDecVideoModeArgs[i].videoDataFormat        = SYSTEM_DF_YUV422P;
        vidDecVideoModeArgs[i].standard               = SYSTEM_STD_MUX_4CH_D1;
        vidDecVideoModeArgs[i].videoCaptureMode       =
            DEVICE_CAPT_VIDEO_CAPTURE_MODE_MULTI_CH_PIXEL_MUX_EMBEDDED_SYNC;
        vidDecVideoModeArgs[i].videoSystem            = 
            DEVICE_VIDEO_DECODER_VIDEO_SYSTEM_AUTO_DETECT;
        vidDecVideoModeArgs[i].videoCropEnable        = FALSE;
        vidDecVideoModeArgs[i].videoAutoDetectTimeout = -1;
    }
    Vcap_configVideoDecoder(vidDecVideoModeArgs, NUM_CAPTURE_DEVICES);

    nsfPrm[NSF_AFTER_CAPTURE].bypassNsf                 = TRUE;
    nsfPrm[NSF_AFTER_CAPTURE].tilerEnable               = FALSE;
    nsfPrm[NSF_AFTER_CAPTURE].inQueParams.prevLinkId    = 
        gVcapModuleContext.captureId;
    nsfPrm[NSF_AFTER_CAPTURE].inQueParams.prevLinkQueId = 0;
    nsfPrm[NSF_AFTER_CAPTURE].numOutQue                 = 1;
    nsfPrm[NSF_AFTER_CAPTURE].outQueParams[0].nextLink  = 
        dupId[CAPTURE_DUP_LINK_IDX];
    nsfPrm[NSF_AFTER_CAPTURE].numBufsPerCh              = NUM_NSF_BUFFERS;

    dupPrm[CAPTURE_DUP_LINK_IDX].inQueParams.prevLinkId    = 
        gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE];
    dupPrm[CAPTURE_DUP_LINK_IDX].inQueParams.prevLinkQueId = 0;
    dupPrm[CAPTURE_DUP_LINK_IDX].notifyNextLink            = TRUE;
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[0].nextLink  = 
        mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[1].nextLink  = 
        selectId[DEI_2_D1_CH_SELECT];
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[2].nextLink  = 
        gVcapModuleContext.sclrId[0];
    dupPrm[CAPTURE_DUP_LINK_IDX].numOutQue                 = 3;
    dupPrm[CAPTURE_DUP_LINK_IDX].outQueParams[3].nextLink  = 
        selectId[DEI_LIVE_SELECT];
    dupPrm[CAPTURE_DUP_LINK_IDX].numOutQue++;

    /****************** 4D1 Live Preview DEI Settings ********************/
    set_DEI_VIP0_SC_DEI_SC_params(
        &selectPrm[DEI_LIVE_SELECT], 
        &deiPrm[DEI_VIP0_SC_DEI_SC],
        dupId[CAPTURE_DUP_LINK_IDX],  
        3,
        mergeId[LIVE_DECODE_MERGE_LINK_IDX],
        SYSTEM_LINK_ID_INVALID,
        selectId[DEI_LIVE_SELECT],
        gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC]);

    /****************** 2D1 De-Interlacer Settigs ********************/
    set_DEI_VIP0_SC_2D1_CH_params(
        &selectPrm[DEI_2_D1_CH_SELECT], 
        &deiPrm[DEI_VIP0_SC_DEI_SC_FOR_2D1],
        dupId[CAPTURE_DUP_LINK_IDX],  
        1,
        gVcapModuleContext.nsfId[NSF_FOR_2QCIF],
        mergeId[D1_CIF_QCIF_MERGE_LINK_IDX],
        selectId[DEI_2_D1_CH_SELECT],
        gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_FOR_2D1]);

    /*************** 6CIF SC2 / VIPSC Settings ******************/
    set_DEI_BYPASS_VIP1_SC_SC2_params(
        &selectPrm[DEI_2_D1_CH_SELECT], 
        &deiPrm[DEI_BYPASS_VIP1_SC_SC2],
        dupId[CAPTURE_DUP_LINK_IDX], 
        1,
        gVcapModuleContext.nsfId[NSF_AFTER_BYPASS_DEI_SC_LINK],
        mergeId[D1_CIF_QCIF_MERGE_LINK_IDX],
        selectId[DEI_2_D1_CH_SELECT],
        gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2]);

    nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK].bypassNsf                 = TRUE;
    // [TODO] Disable TILER for SCD
    nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK].tilerEnable               = FALSE;
    nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK].inQueParams.prevLinkId    = 
        gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2];
    nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK].inQueParams.prevLinkQueId = 
        DEI_LINK_OUT_QUE_DEI_SC;
    nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK].numOutQue                 = 1;
    nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK].outQueParams[0].nextLink  = 
        mergeId[D1_CIF_QCIF_MERGE_LINK_IDX];
    nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK].numBufsPerCh              = NUM_NSF_BUFFERS;

    nsfPrm[NSF_FOR_2QCIF].bypassNsf                 = TRUE;
    // [TODO] Disable TILER for SCD
    nsfPrm[NSF_FOR_2QCIF].tilerEnable               = FALSE;
    nsfPrm[NSF_FOR_2QCIF].inQueParams.prevLinkId    = 
        gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_FOR_2D1];
    nsfPrm[NSF_FOR_2QCIF].inQueParams.prevLinkQueId = DEI_LINK_OUT_QUE_DEI_SC;
    nsfPrm[NSF_FOR_2QCIF].numOutQue                 = 1;
    nsfPrm[NSF_FOR_2QCIF].outQueParams[0].nextLink  = 
        mergeId[D1_CIF_QCIF_MERGE_LINK_IDX];
    nsfPrm[NSF_FOR_2QCIF].numBufsPerCh              = NUM_NSF_BUFFERS;

    /*************** 8ch/1fps MJPEG SC5 Settings *******************/
    sclrPrm.enableLineSkipSc                             = FALSE;
    sclrPrm.inputFrameRate                               = 30;
    sclrPrm.outputFrameRate                              = 1;
    sclrPrm.inQueParams.prevLinkId                       = 
        dupId[CAPTURE_DUP_LINK_IDX];
    sclrPrm.inQueParams.prevLinkQueId                    = 2;
    sclrPrm.outQueParams.nextLink                        = gVcapModuleContext.nsfId[NSF_AFTER_SCALAR_LINK];
    sclrPrm.scaleMode                                    = DEI_SCALE_MODE_RATIO;
    sclrPrm.outScaleFactor.ratio.heightRatio.numerator   = 2;
    sclrPrm.outScaleFactor.ratio.heightRatio.denominator = 1;
    sclrPrm.outScaleFactor.ratio.widthRatio.numerator    = 1;
    sclrPrm.outScaleFactor.ratio.widthRatio.denominator  = 1;
    sclrPrm.tilerEnable                                  = FALSE; // force tiler disable;
    sclrPrm.numBufsPerCh                                 = 1;

    nsfPrm[NSF_AFTER_SCALAR_LINK].bypassNsf                 = TRUE;
    nsfPrm[NSF_AFTER_SCALAR_LINK].tilerEnable               = TILER_ENABLE;
    nsfPrm[NSF_AFTER_SCALAR_LINK].inQueParams.prevLinkId    = 
        gVcapModuleContext.sclrId[0];
    nsfPrm[NSF_AFTER_SCALAR_LINK].inQueParams.prevLinkQueId = 0;
    nsfPrm[NSF_AFTER_SCALAR_LINK].numOutQue                 = 1;
    nsfPrm[NSF_AFTER_SCALAR_LINK].outQueParams[0].nextLink  = 
        mergeId[D1_CIF_QCIF_MERGE_LINK_IDX];
    nsfPrm[NSF_AFTER_SCALAR_LINK].numBufsPerCh              = 1;

    /* CH0,4 - D1 */
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[0].prevLinkId    = 
        gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_FOR_2D1];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[0].prevLinkQueId = 
        DEI_LINK_OUT_QUE_VIP_SC;
    /* CH1,2,3,5,6,7 - CIF */
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[1].prevLinkId    = 
        gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[1].prevLinkQueId = 
        DEI_LINK_OUT_QUE_VIP_SC;
    /* CH0,4 - QCIF */
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[2].prevLinkId    = 
        gVcapModuleContext.nsfId[NSF_FOR_2QCIF];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[2].prevLinkQueId = 0;
    /* CH1,2,3,5,6,7 - QCIF */
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[3].prevLinkId    = 
        gVcapModuleContext.nsfId[NSF_AFTER_BYPASS_DEI_SC_LINK];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[3].prevLinkQueId = 0;
    /* CH0..7 - D1 FOR MJPEG */
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[4].prevLinkId    = 
        gVcapModuleContext.nsfId[NSF_AFTER_SCALAR_LINK];
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].inQueParams[4].prevLinkQueId = 0;

    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].notifyNextLink = TRUE;
    mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].numInQue       = 5;
        mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX].outQueParams.nextLink =
            ipcId[IPC_OUT_VPSS_LINK_IDX];

        ipcOutVpssPrm.inQueParams.prevLinkId    = mergeId[D1_CIF_QCIF_MERGE_LINK_IDX];
        ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
   
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink  = ipcId[IPC_IN_VIDEO_LINK_IDX];
    ipcOutVpssPrm.notifyNextLink            = TRUE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = FALSE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcId[IPC_OUT_VPSS_LINK_IDX];
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    if (enableAlgLink)
    {
		ipcInVideoPrm.outQueParams[0].nextLink  = gVcapModuleContext.dspAlgId[0];

		dspAlgPrm.enableOSDAlg                  = TRUE;
		dspAlgPrm.inQueParams.prevLinkId        = ipcId[IPC_IN_VIDEO_LINK_IDX];
		dspAlgPrm.inQueParams.prevLinkQueId     = 0;
		dspAlgPrm.outQueParams[ALG_LINK_FRAMES_OUT_QUE].nextLink =
				gVencModuleContext.encId;
    }
    else {
		ipcInVideoPrm.outQueParams[0].nextLink  = gVencModuleContext.encId;
    }
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = TRUE;
    ipcInVideoPrm.noNotifyMode              = FALSE;

    EncLink_CreateParams_Init(&encPrm);

    encPrm.numBufPerCh[0] = NUM_ENCODE_D1_BUFFERS;
    encPrm.numBufPerCh[1] = NUM_ENCODE_CIF_BUFFERS;

    /* Primary Stream Params*/
    setH264EncChParams(
        &encPrm, 
        0, 
        gVencModuleContext.vencConfig.numPrimaryChn);

    /* Secondary Stream Params */
    setH264EncChParams(
        &encPrm, 
        gVencModuleContext.vencConfig.numPrimaryChn, 
        (gVencModuleContext.vencConfig.numPrimaryChn 
         + gVencModuleContext.vencConfig.numSecondaryChn));

    /* MJPEG  Params */
    setMJPEGEncChParams(
        &encPrm, 
        gVencModuleContext.vencConfig.numPrimaryChn 
        + gVencModuleContext.vencConfig.numSecondaryChn, 
        VENC_CHN_MAX);

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

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId    = 
        gVencModuleContext.encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink  = 
        gVencModuleContext.ipcBitsInHLOSId;
    MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm, TRUE);

    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkId    = 
        gVencModuleContext.ipcBitsOutRTOSId;
    ipcBitsInHostPrm[0].baseCreateParams.inQueParams.prevLinkQueId = 0;
    MultiCh_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm[0]);

    if (enableOsd)
    {
        Int32 chId;

        dspAlgPrm.enableOSDAlg = TRUE;

        for (chId = 0; chId < ALG_LINK_OSD_MAX_CH; chId++)
        {
            AlgLink_OsdChWinParams * chWinPrm = 
                &dspAlgPrm.osdChCreateParams[chId].chDefaultParams;

            /* set osd window max width and height */
            dspAlgPrm.osdChCreateParams[chId].maxWidth  = EXAMPLE_OSD_WIN_MAX_WIDTH;
            dspAlgPrm.osdChCreateParams[chId].maxHeight = EXAMPLE_OSD_WIN_MAX_HEIGHT;

            chWinPrm->chId       = chId;
            chWinPrm->numWindows = 0;
        }
    }
    dspAlgPrm.enableSCDAlg                                = enableScd;
    dspAlgPrm.outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink = SYSTEM_LINK_ID_INVALID;

    if (enableScd)
    {
        UInt32                   i, startChId;
        AlgLink_ScdCreateParams *pScdCreatePrm;
        AlgLink_ScdChParams     *pScdChPrm;

        pScdCreatePrm = &dspAlgPrm.scdCreateParams;

        pScdCreatePrm->maxWidth                  = 176;
        pScdCreatePrm->maxHeight                 = 144;
        pScdCreatePrm->maxStride                 = 192;
        pScdCreatePrm->numValidChForSCD          = NUM_SCD_CHANNELS;
        pScdCreatePrm->numSecs2WaitB4Init        = 3;
        pScdCreatePrm->numSecs2WaitB4FrmAlert    = 1;
        pScdCreatePrm->inputFrameRate            = 5;
        pScdCreatePrm->outputFrameRate           = 5;
        pScdCreatePrm->numSecs2WaitAfterFrmAlert = 1;
        pScdCreatePrm->enableTamperNotify        = FALSE;

        /* enable SCD only for CIF CHs */
        startChId = 8;
        for (i = 0; i < pScdCreatePrm->numValidChForSCD; i++)
        {
            pScdChPrm = &pScdCreatePrm->chDefaultParams[i];

            pScdChPrm->chId               = startChId;
            pScdChPrm->mode               = 
                ALG_LINK_SCD_DETECTMODE_MONITOR_FULL_FRAME;
            pScdChPrm->frmIgnoreLightsON  = FALSE;
            pScdChPrm->frmIgnoreLightsOFF = FALSE;
            pScdChPrm->frmSensitivity     = ALG_LINK_SCD_SENSITIVITY_VERYHIGH;
            pScdChPrm->frmEdgeThreshold   = 0;
            pScdChPrm->blkNumBlksInFrame  = 0;

            startChId++;
        }
    }

#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif

    System_linkCreate(
            gVcapModuleContext.captureId, 
            &capturePrm, 
            sizeof(capturePrm));
    System_linkCreate(
        gVcapModuleContext.nsfId[NSF_AFTER_CAPTURE], 
        &nsfPrm[NSF_AFTER_CAPTURE], 
        sizeof(nsfPrm[NSF_AFTER_CAPTURE]));
    System_linkCreate(
        dupId[CAPTURE_DUP_LINK_IDX], 
        &dupPrm[CAPTURE_DUP_LINK_IDX], 
        sizeof(dupPrm[CAPTURE_DUP_LINK_IDX]));
    System_linkCreate(
        selectId[DEI_LIVE_SELECT], 
        &selectPrm[DEI_LIVE_SELECT], 
        sizeof(selectPrm[DEI_LIVE_SELECT]));
    System_linkCreate(
        gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC], 
        &deiPrm[DEI_VIP0_SC_DEI_SC], 
        sizeof(deiPrm[DEI_VIP0_SC_DEI_SC]));
    System_linkCreate(
        selectId[DEI_2_D1_CH_SELECT], 
        &selectPrm[DEI_2_D1_CH_SELECT], 
        sizeof(selectPrm[DEI_2_D1_CH_SELECT]));
    System_linkCreate(
        gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC_FOR_2D1], 
        &deiPrm[DEI_VIP0_SC_DEI_SC_FOR_2D1], 
        sizeof(deiPrm[DEI_VIP0_SC_DEI_SC_FOR_2D1]));
    System_linkCreate(
        gVcapModuleContext.nsfId[NSF_FOR_2QCIF], 
        &nsfPrm[NSF_FOR_2QCIF], 
        sizeof(nsfPrm[NSF_FOR_2QCIF]));
    System_linkCreate(
        gVcapModuleContext.deiId[DEI_BYPASS_VIP1_SC_SC2], 
        &deiPrm[DEI_BYPASS_VIP1_SC_SC2], 
        sizeof(deiPrm[DEI_BYPASS_VIP1_SC_SC2]));
    System_linkCreate(
        gVcapModuleContext.nsfId[NSF_AFTER_BYPASS_DEI_SC_LINK], 
        &nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK], 
        sizeof(nsfPrm[NSF_AFTER_BYPASS_DEI_SC_LINK]));
    System_linkCreate(gVcapModuleContext.sclrId[0], &sclrPrm, sizeof(sclrPrm));
    System_linkCreate(
        gVcapModuleContext.nsfId[NSF_AFTER_SCALAR_LINK], 
        &nsfPrm[NSF_AFTER_SCALAR_LINK], 
        sizeof(nsfPrm[NSF_AFTER_SCALAR_LINK]));
    System_linkCreate(
        mergeId[D1_CIF_QCIF_MERGE_LINK_IDX], 
        &mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX], 
        sizeof(mergePrm[D1_CIF_QCIF_MERGE_LINK_IDX]));
    System_linkCreate(
        ipcId[IPC_OUT_VPSS_LINK_IDX], 
        &ipcOutVpssPrm, 
        sizeof(ipcOutVpssPrm));
    System_linkCreate(
        ipcId[IPC_IN_VIDEO_LINK_IDX], 
        &ipcInVideoPrm, 
        sizeof(ipcInVideoPrm));
    if (enableAlgLink)
    {
        System_linkCreate(
            gVcapModuleContext.dspAlgId[0], 
            &dspAlgPrm, 
            sizeof(dspAlgPrm));
    }
    System_linkCreate(
        gVencModuleContext.encId, 
        &encPrm, 
        sizeof(encPrm));
    System_linkCreate(
        gVencModuleContext.ipcBitsOutRTOSId, 
        &ipcBitsOutVideoPrm, 
        sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(
        gVencModuleContext.ipcBitsInHLOSId, 
        &ipcBitsInHostPrm, 
        sizeof(ipcBitsInHostPrm));

    System_linkGetInfo(gVencModuleContext.ipcBitsInHLOSId,&bitsProducerLinkInfo);
    OSA_assert(bitsProducerLinkInfo.numQue == 1);
    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink =
        gVdecModuleContext.ipcBitsInRTOSId;

    printf("\n\n========bitsProducerLinkInfo============\n");
    printf(
        "numQ %d, numCh %d\n", 
        bitsProducerLinkInfo.numQue, 
        bitsProducerLinkInfo.queInfo[0].numCh);
    {
        Int32 i;
        for (i = 0; i < bitsProducerLinkInfo.queInfo[0].numCh; i++)
        {
            printf(
                "Ch [%d] Width %d, Height %d\n",
                i,
                bitsProducerLinkInfo.queInfo[0].chInfo[i].width,
                bitsProducerLinkInfo.queInfo[0].chInfo[i].height);
        }
    }
    printf("\n====================\n\n");

    if (bitsProducerLinkInfo.queInfo[0].numCh > 
            gVencModuleContext.vencConfig.numPrimaryChn)
    {
        bitsProducerLinkInfo.queInfo[0].numCh = 
            gVencModuleContext.vencConfig.numPrimaryChn;
    }

    printf("Reducing bitsProducerLinkInfo.numCh to %d\n", 
            bitsProducerLinkInfo.queInfo[0].numCh);

    MultiCh_ipcBitsInitCreateParams_BitsOutHLOS(
            &ipcBitsOutHostPrm,            
            &bitsProducerLinkInfo.queInfo[0]);
    ipcBitsOutHostPrm.numBufPerCh[0]  = NUM_IPC_OUT_BUFFERS;
    if (gVdecModuleContext.vdecConfig.forceUseDecChannelParams)
    {
        /* use channel info provided by user instead of from encoder */
        UInt32 chId;
        System_LinkChInfo *pChInfo;

        ipcBitsOutHostPrm.inQueInfo.numCh = gVdecModuleContext.vdecConfig.numChn;

        for (chId = 0; chId < ipcBitsOutHostPrm.inQueInfo.numCh; chId++)
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

            pChInfo->width      = 
                gVdecModuleContext.vdecConfig.decChannelParams[chId].maxVideoWidth;
            pChInfo->height     = 
                gVdecModuleContext.vdecConfig.decChannelParams[chId].maxVideoHeight;
            pChInfo->scanFormat = SYSTEM_SF_PROGRESSIVE;
        }
    }
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId    = 
        gVdecModuleContext.ipcBitsOutHLOSId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink  = 
        gVdecModuleContext.decId;
    MultiCh_ipcBitsInitCreateParams_BitsInRTOS(&ipcBitsInVideoPrm, TRUE);

    for (i = 0; i < gVdecModuleContext.vdecConfig.numChn; i++) 
    {
        decPrm.chCreateParams[i].format                 = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile                = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = FALSE;
        decPrm.chCreateParams[i].targetMaxWidth         = 
            ipcBitsOutHostPrm.inQueInfo.chInfo[i].width;
        decPrm.chCreateParams[i].targetMaxHeight        = 
            ipcBitsOutHostPrm.inQueInfo.chInfo[i].height;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = 
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.frameRate;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate   = 
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.targetBitRate;
        /* Max ref frames is only 2 as this is closed loop decoder */
        decPrm.chCreateParams[i].dpbBufSizeInFrames = 2;
        decPrm.chCreateParams[i].numBufPerCh        = NUM_DECODE_BUFFERS;
    }
    decPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsInRTOSId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcId[IPC_OUT_VIDEO_LINK_IDX];
    decPrm.tilerEnable               = TILER_ENABLE;

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
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = 
        gVcapModuleContext.deiId[DEI_VIP0_SC_DEI_SC];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = 
        DEI_LINK_OUT_QUE_DEI_SC;
    i++;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = 
        dupId[CAPTURE_DUP_LINK_IDX];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = 0;
    i++;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkId    = 
        ipcId[IPC_IN_VPSS_LINK_IDX];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].inQueParams[i].prevLinkQueId = 0;
    i++;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].numInQue                     = i;
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].outQueParams.nextLink        = 
        dupId[LIVE_DECODE_DUP_LINK_IDX];
    mergePrm[LIVE_DECODE_MERGE_LINK_IDX].notifyNextLink               = TRUE;

    dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkId    = 
        mergeId[LIVE_DECODE_MERGE_LINK_IDX];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].inQueParams.prevLinkQueId = 0;
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].numOutQue                 = 2;
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[0].nextLink  = 
        gVdisModuleContext.swMsId[0];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].outQueParams[1].nextLink  = 
        gVdisModuleContext.swMsId[1];
    dupPrm[LIVE_DECODE_DUP_LINK_IDX].notifyNextLink            = TRUE;

    for (i = 0; i < 2; i++)
    {
        swMsPrm[i].inQueParams.prevLinkId    = dupId[LIVE_DECODE_DUP_LINK_IDX];
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = gVdisModuleContext.displayId[i];
        swMsPrm[i].maxInputQueLen            = 4;
        swMsPrm[i].maxOutRes                 = VSYS_STD_1080P_60;
        swMsPrm[i].initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[i].resolution;
        swMsPrm[i].numOutBuf                 = NUM_SWMS_MAX_BUFFERS;
        if (i == 1)
        {
            swMsPrm[i].numOutBuf  = NUM_SWMS_MAX_BUFFERS + 2;
            swMsPrm[i].maxOutRes  = VSYS_STD_PAL;
            swMsPrm[i].initOutRes = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
        }

#ifdef  SYSTEM_USE_TILER
        swMsPrm[i].lineSkipMode = FALSE; // Double pitch not possible in tiler mode; so Line skip not possible
#else
        swMsPrm[i].lineSkipMode = TRUE; // Set to TRUE for Enable low cost scaling
#endif
        swMsPrm[i].enableLayoutGridDraw = 
            gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(VDIS_DEV_HDMI, &swMsPrm[0], TRUE);
        MultiCh_swMsGetDefaultLayoutPrm(VDIS_DEV_SD,   &swMsPrm[1], TRUE);

        displayPrm[i].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                   = swMsPrm[i].initOutRes;
        if (i == 1)
        {
            displayPrm[i].displayRes                   = 
                gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;
            displayPrm[i].forceFieldSeparatedInputMode = TRUE;
        }
    }

    System_linkCreate(
            gVdecModuleContext.ipcBitsOutHLOSId,
            &ipcBitsOutHostPrm,
            sizeof(ipcBitsOutHostPrm));
    System_linkCreate(
            gVdecModuleContext.ipcBitsInRTOSId,
            &ipcBitsInVideoPrm,
            sizeof(ipcBitsInVideoPrm));
    System_linkCreate(gVdecModuleContext.decId, &decPrm, sizeof(decPrm));

    System_linkCreate(
            ipcId[IPC_OUT_VIDEO_LINK_IDX], 
            &ipcOutVideoPrm, 
            sizeof(ipcOutVideoPrm));
    System_linkCreate(
            ipcId[IPC_IN_VPSS_LINK_IDX], 
            &ipcInVpssPrm, 
            sizeof(ipcInVpssPrm));
    System_linkCreate(
            mergeId[LIVE_DECODE_MERGE_LINK_IDX], 
            &mergePrm[LIVE_DECODE_MERGE_LINK_IDX], 
            sizeof(mergePrm[LIVE_DECODE_MERGE_LINK_IDX]));
    System_linkCreate(
            dupId[LIVE_DECODE_DUP_LINK_IDX], 
            &dupPrm[LIVE_DECODE_DUP_LINK_IDX], 
            sizeof(dupPrm[LIVE_DECODE_DUP_LINK_IDX]));

    for (i = 0; i < 2; i++)
    {
        System_linkCreate(
                gVdisModuleContext.swMsId[i], 
                &swMsPrm[i], 
                sizeof(swMsPrm[i]));
    }

    for (i = 0; i < 2; i++)  /* Both tied VENCs HDMI and DVO2 are handled by single link instance */
    {
        System_linkCreate(
                gVdisModuleContext.displayId[i], 
                &displayPrm[i], 
                sizeof(displayPrm[i]));
    }

    skipOddFieldAtSC(NUM_LIVE_CH);


    MultiCh_progressive8ChVcapVencVdecVdisSwitchLayout(
            VDIS_DEV_HDMI,
            &swMsPrm[0].layoutPrm);
    MultiCh_progressive8ChVcapVencVdecVdisSwitchLayout(
            VDIS_DEV_SD,
            &swMsPrm[1].layoutPrm);
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



Void MultiCh_deleteProgressive8ChVcapVencVdecVdis()
{
    UInt32 i;
    UInt32 mergeId[NUM_MERGE_LINK];
    UInt32 dupId[NUM_DUP_LINK];
    UInt32 selectId[NUM_SELECT_LINK];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    UInt32 ipcBitsOutDSPId;
    Bool   enableAlgLink = gVsysModuleContext.vsysConfig.enableOsd;

    mergeId[LIVE_DECODE_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId[D1_CIF_QCIF_MERGE_LINK_IDX] = SYSTEM_VPSS_LINK_ID_MERGE_1;

    dupId[CAPTURE_DUP_LINK_IDX]         = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId[LIVE_DECODE_DUP_LINK_IDX]     = SYSTEM_VPSS_LINK_ID_DUP_1;

    selectId[DEI_LIVE_SELECT]           = SYSTEM_VPSS_LINK_ID_SELECT_0;
    selectId[DEI_2_D1_CH_SELECT]        = SYSTEM_VPSS_LINK_ID_SELECT_1;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    ipcBitsOutDSPId = SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0;

    gVdecModuleContext.ipcBitsOutHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;

#if 1
    System_linkDelete(gVcapModuleContext.captureId);
    for (i = 0; i < NUM_NSF_LINK; i++)
    {
        if (gVcapModuleContext.nsfId[i] != SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gVcapModuleContext.nsfId[i]);
        }
    }

    for (i = 0; i < NUM_DEI_LINK; i++)
    {
        if (gVcapModuleContext.deiId[i] != SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gVcapModuleContext.deiId[i]);
        }
    }


    System_linkDelete(gVcapModuleContext.sclrId[0]);
#else
    if (gVsysModuleContext.vsysConfig.enableScd)
    {
        System_linkDelete(ipcBitsOutDSPId);
    }

    Vcap_delete();
#endif
    for (i = 0; i < NUM_SELECT_LINK; i++)
    {
        System_linkDelete(selectId[i]);
    }

    System_linkDelete(ipcOutVpssId );
    System_linkDelete(ipcInVideoId );

    if (enableAlgLink)
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

    for (i = 0; i < 2; i++)
    {
        System_linkDelete(gVdisModuleContext.swMsId[i]);
    }

    for (i = 0; i < 2; i++)
    {
        System_linkDelete(gVdisModuleContext.displayId[i]);
    }

    for (i = 0; i < NUM_DUP_LINK; i++)
    {
        if (dupId[i] != SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(dupId[i]);
        }
    }
    for (i = 0; i < NUM_MERGE_LINK; i++)
    {
        if (mergeId[i] != SYSTEM_LINK_ID_INVALID)
        {
           System_linkDelete(mergeId[i]);
        }
    }

    if (gVsysModuleContext.enableFastUsecaseSwitch == FALSE)
    {
        Vcap_deleteVideoDecoder();
    }

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

#if     (TILER_ENABLE == FALSE)
    /* Reenable tiler allocator that was disabled by this usecase */
    SystemTiler_enableAllocator();
#endif
}



Void MultiCh_progressive8ChVcapVencVdecVdisSetSWMSOutputFPS(
        SwMsLink_LayoutPrm *pSwMsLayout)
{
    Bool livePresent = FALSE;

    /* Set the requestFPS to 60/50 only if live channels are present in layout */
    if (pSwMsLayout->outputFPS >= 50)
    {
        Int32 ch;

        /* For 7x1, 1CH, 6CH, ch0 should be live */
        if (pSwMsLayout->numWin == 4)
        {
            for (ch = 0; ch < pSwMsLayout->numWin; ch++)
            {
                if (pSwMsLayout->winInfo[ch].channelNum < 
                        gVcapModuleContext.numChannels)
                {
                    livePresent = TRUE;
                }
            }
        }
        else
        {
            if (pSwMsLayout->winInfo[0].channelNum < gVcapModuleContext.numChannels)
            {
                livePresent = TRUE;
            }
        }
        if (livePresent == FALSE)
        {
            pSwMsLayout->outputFPS = pSwMsLayout->outputFPS / 2;
        }
    }
    printf("++++++++++ LIVE CHANNEL %s.... Setting SWMS outputFPS to %d\n",
            livePresent == TRUE ? "PRESENT" : "NOT PRESENT",
            pSwMsLayout->outputFPS);
}



Int32 MultiCh_progressive8ChVcapVencVdecVdisSwitchLayout(
        VDIS_DEV            vdDevId, 
        SwMsLink_LayoutPrm *pSwMsLayout)
{
    UInt32 winId, chId;
    UInt32 swMsId    = SYSTEM_LINK_ID_INVALID;
    UInt32 displayId = SYSTEM_LINK_ID_INVALID;
    Bool   changeDisplayInputMode = FALSE;
    UInt32 numSelectChs;
    UInt32 selectChs[NUM_DEI_LIVE_CH];

    if (vdDevId == VDIS_DEV_HDMI ||
        vdDevId == VDIS_DEV_DVO2 ||
		vdDevId == VDIS_DEV_HDCOMP)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if (vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];

        /* for DM814x, switch to 60fps input, interlaced 60fps output when
            psVdMosaicParam->outputFPS is set to 60fps */
        changeDisplayInputMode = TRUE;
        displayId = gVdisModuleContext.displayId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
        return -1;
    }

    MultiCh_progressive8ChVcapVencVdecVdisSetSWMSOutputFPS(pSwMsLayout);

    /*
        At SW Moisac,
         0.. 3 - DEI output CHs - can be dynamically mapped by select link
         4..11 - Capture CHs
        12..19 - Decode CHs

        For user,
        0..7  - Capture CHs
        8..15 - Decode CHs
    */

    numSelectChs = 0;

    memset(m_PreviewFullFpsCh, 0, sizeof(m_PreviewFullFpsCh));

    for (winId = 0; winId < pSwMsLayout->numWin; winId++)
    {
        chId = pSwMsLayout->winInfo[winId].channelNum;

        if (chId != SYSTEM_SW_MS_INVALID_ID)
        {
            /* valid Ch mapping */
            if (chId < NUM_LIVE_CH &&                          
                pSwMsLayout->winInfo[winId].bypass == FALSE &&
                numSelectChs < NUM_DEI_LIVE_CH)             
            {
                /* Use DEI'ed Chs */
                selectChs[numSelectChs]  = chId;
                chId                     = numSelectChs;
                numSelectChs++;
                m_PreviewFullFpsCh[chId] = TRUE;
            }
            else
            {
                /* Playback CH or dont need DEI'ed Chs */
                chId += NUM_DEI_LIVE_CH;
            }
        }

        pSwMsLayout->winInfo[winId].channelNum = chId;
    }

    if (vdDevId != VDIS_DEV_SD)
    {
        /* Dont select CHs for SD display, since HDMI will select the CHs and SD
           display should not override them */
        selectDeiLiveChs(numSelectChs, selectChs);
    }

    if (pSwMsLayout->outputFPS <= 30 )
    {
        /* skip alternate frames at DEI */
        set_DEI_VIP0_SC_DEI_SC_outputFPS(60, 30);
    }
    else
    {
        /* DO NOT skip alternate frames at DEI */
        set_DEI_VIP0_SC_DEI_SC_outputFPS(60, 60);
    }

    System_linkControl(
            swMsId, 
            SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, 
            pSwMsLayout, 
            sizeof(*pSwMsLayout), TRUE);

    if (changeDisplayInputMode)
    {
        DisplayLink_SwitchInputMode prm;

        prm.enableFieldSeparatedInputMode = 
            (pSwMsLayout->outputFPS >= 50) ?  TRUE : FALSE;

        System_linkControl(
                displayId, 
                DISPLAY_LINK_CMD_SWITCH_INPUT_MODE, 
                &prm, 
                sizeof(prm), 
                TRUE);
    }

    return 0;
}



void MultiCh_progressive8ChVcapVencVdecVdisSwmsChReMap(
        VDIS_MOSAIC_S *psVdMosaicParam)
{
    /*
        SW Moisac Ch ID     -> McFW CH ID
        >= NUM_DEI_LIVE_CH     swMsChId  - NUM_DEI_LIVE_CH
        <  NUM_DEI_LIVE_CH      swMsChId
     */
    Int32  winId, chId;
    UInt32 numSelectChs;
    UInt32 selectChs[NUM_DEI_LIVE_CH];

    getDeiLiveChIds(&numSelectChs, selectChs);

    for (winId = 0; winId < psVdMosaicParam->numberOfWindows; winId++)
    {
        chId = psVdMosaicParam->chnMap[winId];

        if (chId >= NUM_DEI_LIVE_CH )
        {
            if (chId != (UInt32)(VDIS_CHN_INVALID) )
            {
                chId -= NUM_DEI_LIVE_CH;
            }
        }
        else
        {
            if (chId < numSelectChs)
            {
                chId = selectChs[chId];
            }
        }

        psVdMosaicParam->chnMap[winId] = chId;
    }
}



/*
    At capture level, channels
              0 ~ 3,  strmId = 0 map to De-interlaced channels <frame rate control not possible>
              4 ~ 11, strmId = 0 map to capture channels <frame rate control not possible>
              0 ~ 7,  strmId = 1 map to D1+CIF channels from DEI_HQ
              8 ~ 15, strmId = 2 map to CIF channels from DEI_HQ
 */
Int32 MultiCh_progressive8ChVcapVencVdecVdisSetCapFrameRate(
        VCAP_CHN  vcChnId, 
        VCAP_STRM vcStrmId, 
        Int32     inputFrameRate, 
        Int32     outputFrameRate)
{
    DeiLink_ChFpsParams params;
    Int32               status = ERROR_FAIL;
    UInt32              dei = DEI_VIP0_SC_DEI_SC;

    params.chId            = vcChnId;
    params.inputFrameRate  = inputFrameRate;
    params.outputFrameRate = outputFrameRate;
    
    switch (vcStrmId)
    {
    case 0:
        dei             = DEI_VIP0_SC_DEI_SC;
        params.streamId = DEI_LINK_OUT_QUE_DEI_SC;
        break;

    case 1:
    case 2:
        dei             = DEI_BYPASS_VIP1_SC_SC2;
        params.streamId = (vcStrmId == 1) 
                        ? DEI_LINK_OUT_QUE_VIP_SC 
                        : DEI_LINK_OUT_QUE_DEI_SC;
        break;
    }

    if (gVcapModuleContext.deiId[dei] != SYSTEM_LINK_ID_INVALID)
    {
        status = System_linkControl(
                gVcapModuleContext.deiId[dei], 
                DEI_LINK_CMD_SET_FRAME_RATE,       
                &params, 
                sizeof(params), 
                TRUE);
    }

    return status;
}



Int32 MultiCh_progressive8ChVcapVencVdecVdis_enableDisableCapChn(
        VCAP_CHN  vcChnId, 
        VCAP_STRM vcStrmId, 
        Bool      enableChn)
{
    DeiLink_ChannelInfo channelInfo;
    char  *onOffName[] = { "OFF ", "ON" };
    UInt32 cmdId;
    Int32  status = ERROR_FAIL;
    Int32  deiIdx;

    if (enableChn)
        cmdId = DEI_LINK_CMD_ENABLE_CHANNEL;
    else
        cmdId = DEI_LINK_CMD_DISABLE_CHANNEL;

    switch (vcStrmId)
    {
        case 0:
            printf("Preview path enable / disable not possible\n");
            break;

        case 1:
        case 2:
            if (vcChnId == 0 || vcChnId == 1)
                deiIdx = DEI_VIP0_SC_DEI_SC_FOR_2D1;
            else
                deiIdx = DEI_BYPASS_VIP1_SC_SC2;

            if (vcStrmId == 1)
                channelInfo.streamId  = DEI_LINK_OUT_QUE_VIP_SC;
            else
                channelInfo.streamId  = DEI_LINK_OUT_QUE_DEI_SC;
            channelInfo.channelId = vcChnId;
            channelInfo.enable    = enableChn;
            if (gVcapModuleContext.deiId[deiIdx] != 
                    SYSTEM_LINK_ID_INVALID)
            {
                status = System_linkControl(                        
                        gVcapModuleContext.deiId[deiIdx],          
                        cmdId,                        
                        &(channelInfo),                              
                        sizeof(DeiLink_ChannelInfo),                     
                        TRUE);
            }
            printf(" VCAP: CH%d STRM%d = [%s]\n", 
                    vcChnId, 
                    vcStrmId, 
                    onOffName[enableChn]);
            break;
    }
    return status;
}



Int32 MultiCh_progressive8ChVcapVencVdecVdis_setCapDynamicParamChn(
        VCAP_CHN                  vcChnId, 
        VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam, 
        VCAP_PARAMS_E             paramId)
{
    DeiLink_chDynamicSetOutRes params = {0};
    Int32                      status = ERROR_FAIL;
    UInt32                     maxWidth, maxHeight;
    UInt32                     deiId = DEI_BYPASS_VIP1_SC_SC2;
    UInt32                     ch;

    params.width  = psCapChnDynaParam->chDynamicRes.width;
    params.height = psCapChnDynaParam->chDynamicRes.height;
    params.queId  = (psCapChnDynaParam->chDynamicRes.pathId == 1) 
                  ? DEI_LINK_OUT_QUE_VIP_SC 
                  : DEI_LINK_OUT_QUE_DEI_SC;

    deiId = (vcChnId == m_D1_Ch[0] || vcChnId == m_D1_Ch[1])
          ? DEI_VIP0_SC_DEI_SC_FOR_2D1
          : DEI_BYPASS_VIP1_SC_SC2;
    
    if (paramId == VCAP_RESOLUTION)
    {
        /* Validate parameters */
        if (psCapChnDynaParam->chDynamicRes.pathId == 1)
        {
            if (vcChnId == m_D1_Ch[0] || vcChnId == m_D1_Ch[1])
            {
                maxWidth  = 704;
                maxHeight = (Vcap_isPalMode() == TRUE) ? 576 : 480;
                if (params.width > maxWidth || params.height > maxHeight)
                {
                    printf("Ch%d (%dx%d) : Max resolution is D1 !!!!!!\n", 
                            vcChnId, 
                            params.width, 
                            params.height);
                    return ERROR_FAIL;
                }
                if (vcChnId == m_D1_Ch[0])
                {
                    params.chId = 0;
                    if (params.width == 704 && 
                        params.height == (Vcap_isPalMode() ? 576 : 480))
                    {
                        m_D1_Ch_DEI[0] = TRUE;
                    }
                    else 
                    {
                        m_D1_Ch_DEI[0] = FALSE;
                    }
                }
                else if (vcChnId == m_D1_Ch[1])
                {
                    params.chId = 1;
                    if (params.width == 704 && 
                       params.height == (Vcap_isPalMode() ? 576 : 480))
                    {
                        m_D1_Ch_DEI[1] = TRUE;
                    }
                    else
                    {
                        m_D1_Ch_DEI[1] = FALSE;
                    }
                }
            }
            else
            {
                for (ch = 0; ch < NUM_DEI_CIF_CH; ch++)
                {
                    if (vcChnId == m_CIF_Ch[ch])
                    {
                        params.chId = ch;
                    }
                }
                maxWidth  = 704;
                maxHeight = (Vcap_isPalMode() == TRUE) ? 288 : 240;
                if (params.width > maxWidth || params.height > maxHeight)
                {
                    printf("Ch%d (%dx%d) : Max resolution is 2CIF !!!!!!\n", 
                            vcChnId, 
                            params.width, 
                            params.height);
                    return ERROR_FAIL;
                }
            }
        }
        else
        {
            params.chId = vcChnId;
            maxWidth    = 176;
            maxHeight   = (Vcap_isPalMode() == TRUE) ? 144 : 120;
            if (params.width > maxWidth || params.height > maxHeight)
            {
                printf("Ch%d (%dx%d) : Max resolution is QCIF !!!!!!\n", 
                        vcChnId, 
                        params.width, 
                        params.height);
                return ERROR_FAIL;
            }
        }
        status = System_linkControl(
                gVcapModuleContext.deiId[deiId],
                DEI_LINK_CMD_GET_OUTPUTRESOLUTION,
                &params,
                sizeof(params),
                TRUE);
        params.width  = SystemUtils_align(psCapChnDynaParam->chDynamicRes.width, 16);
        params.height = psCapChnDynaParam->chDynamicRes.height;
        printf ("Setting DEI %d, Ch %d, Q %d with new resolution - %d X %d!!!!!!\n",
            deiId, params.chId, params.queId, params.width, params.height);
        status = System_linkControl(
                gVcapModuleContext.deiId[deiId],
                DEI_LINK_CMD_SET_OUTPUTRESOLUTION,
                &(params),
                sizeof(params),
                TRUE);
    }
    return ERROR_NONE;
}



Int32 MultiCh_progressive8ChVcapVencVdecVdis_getCapDynamicParamChn(
        VCAP_CHN                  vcChnId, 
        VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam, 
        VCAP_PARAMS_E             paramId)
{
    DeiLink_chDynamicSetOutRes params = {0};
    Int32  status = ERROR_FAIL, deiId = DEI_BYPASS_VIP1_SC_SC2;
    Uint32 ch;
    Bool   found;

    if (psCapChnDynaParam->chDynamicRes.pathId == 1)
    {
        found = FALSE;
        for (ch = 0; ch < NUM_DEI_D1_CH; ch++)
        {
            if (vcChnId == m_D1_Ch[ch])
            {
                deiId       = DEI_VIP0_SC_DEI_SC_FOR_2D1;
                params.chId = ch;
                found       = TRUE;
            }
        }
        if (!found)     // one of the 6 CIF channels
        {
            deiId        = DEI_BYPASS_VIP1_SC_SC2;
            params.queId = DEI_LINK_OUT_QUE_VIP_SC;
            for (ch = 0; ch < NUM_DEI_CIF_CH; ch++)
            {
                if (vcChnId == ch)
                {
                    params.chId = ch;
                }
            }
        }         
    }
    else
    {
        params.queId = DEI_LINK_OUT_QUE_DEI_SC;
        params.chId  = vcChnId;
    }

    status = System_linkControl(
            gVcapModuleContext.deiId[deiId],
            DEI_LINK_CMD_GET_OUTPUTRESOLUTION,
            &params,
            sizeof(params),
            TRUE);
    psCapChnDynaParam->chDynamicRes.width  = params.width;
    psCapChnDynaParam->chDynamicRes.height = params.height;

    return status;
}


/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/*
                        Capture (YUV422I) 16CH D1 60fps
                          |
                          |
                        NSF (YUV420SP) -------------------+
                          |                               |
                          |                               |
  NULL SRC (YUV420SP)  DEIH (VIP-SC YUV420 )     DEI (VIP-SC YUV420 )
          |               |                               |
          +------------+  |  +----------------------------+
                       |  |  |
                       |  |  |
                        MERGE
                          |
                          |
                         DUP
                         |||
         +---------------+|+------------+
         |                |             |
         |                |             |
      SW Mosaic       SW Mosaic         |
      (SC5 YUV422I)  (SC5 YUV422I)      |
         |                |             |
 GRPX0   |       GRPX1,2  |             |
    |    |           |    |             |
    On-Chip HDMI    Off-Chip HDMI  SDTV (NTSC)
      1080p60         1080p60        480i60
*/

#include "multich_common.h"

/* =============================================================================
 * Externs
 * =============================================================================
 */


#define     NUM_CAPTURE_DEVICES          (4)



/* =============================================================================
 * Use case code
 * =============================================================================
 */

Void MultiCh_createVcapVdis()
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
    DeiLink_CreateParams        deiPrm[2];
    NullSrcLink_CreateParams    nullSrcPrm;
    MergeLink_CreateParams      mergePrm;
    DupLink_CreateParams        dupPrm;
    static SwMsLink_CreateParams       swMsPrm[VDIS_DEV_MAX];
    DisplayLink_CreateParams    displayPrm[VDIS_DEV_MAX];

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;

    VCAP_VIDDEC_PARAMS_S vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

    UInt32 grpxId[VDIS_DEV_MAX];
    UInt32 nullId;
    UInt32 mergeId, dupId;
    UInt32 deiOutQue;

    UInt32 vipInstId;
    UInt32 i;
    UInt32 numSubChains;
    Bool enableSdtv;

    for (i = 0; i < VDIS_DEV_MAX; i++)
    {
        MULTICH_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
        MULTICH_INIT_STRUCT(SwMsLink_CreateParams ,swMsPrm[i]);
    }

    MultiCh_detectBoard();

    System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES,
        NULL,
        0,
        TRUE
        );

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    gVcapModuleContext.nullSrcId    = SYSTEM_VPSS_LINK_ID_NULL_SRC_0;
    mergeId                         = SYSTEM_VPSS_LINK_ID_MERGE_0;
    dupId                           = SYSTEM_VPSS_LINK_ID_DUP_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;

    swMsPrm[0].numSwMsInst   = 1;
    swMsPrm[1].numSwMsInst   = 1;
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI
    gVdisModuleContext.displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; // OFF CHIP HDMI
    grpxId[0]                       = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]                       = SYSTEM_LINK_ID_GRPX_1;
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
    grpxId[2]                       = SYSTEM_LINK_ID_GRPX_2;
#endif
    nullId                          = SYSTEM_VPSS_LINK_ID_NULL_0;

    numSubChains             = 2;
    deiOutQue                = DEI_LINK_OUT_QUE_VIP_SC;
    enableSdtv               = FALSE;

    CaptureLink_CreateParams_Init(&capturePrm);

    capturePrm.numVipInst    = 2 * numSubChains;
    capturePrm.outQueParams[0].nextLink = gVcapModuleContext.nsfId[0];

    capturePrm.tilerEnable              = FALSE;
    capturePrm.enableSdCrop             = FALSE;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = (SYSTEM_CAPTURE_INST_VIP0_PORTA+vipInstId)%SYSTEM_CAPTURE_INST_MAX;
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

    NsfLink_CreateParams_Init(&nsfPrm);
    nsfPrm.bypassNsf                 = FALSE;
    nsfPrm.tilerEnable               = FALSE;
    nsfPrm.inQueParams.prevLinkId    = gVcapModuleContext.captureId;
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.numOutQue                 = numSubChains;
    nsfPrm.outQueParams[0].nextLink  = gVcapModuleContext.deiId[0];
    nsfPrm.outQueParams[1].nextLink  = gVcapModuleContext.deiId[1];

    nullSrcPrm.outQueParams.nextLink      = mergeId;
    nullSrcPrm.timerPeriod                = 33;
    nullSrcPrm.inputInfo.numCh            = 16;
    for(i=0; i<nullSrcPrm.inputInfo.numCh; i++)
    {
        System_LinkChInfo *pChInfo;

        pChInfo = &nullSrcPrm.inputInfo.chInfo[i];

        pChInfo->dataFormat = SYSTEM_DF_YUV420SP_UV;
        pChInfo->memType    = SYSTEM_MT_NONTILEDMEM;
        pChInfo->startX     = 48;
        pChInfo->startY     = 32;
        pChInfo->width      = 720;
        pChInfo->height     = 576;
        pChInfo->pitch[0]   = SystemUtils_align(pChInfo->width+pChInfo->startX, SYSTEM_BUFFER_ALIGNMENT);
        pChInfo->pitch[1]   = pChInfo->pitch[0];
        pChInfo->pitch[2]   = 0;
        pChInfo->scanFormat = SYSTEM_SF_PROGRESSIVE;
    }

    for(i=0; i<numSubChains; i++)
    {
        Int32 chId;

        DeiLink_CreateParams_Init(&deiPrm[i]);

        deiPrm[i].inQueParams.prevLinkId                        = gVcapModuleContext.nsfId[0];
        deiPrm[i].inQueParams.prevLinkQueId                     = i;
        deiPrm[i].outQueParams[deiOutQue].nextLink              = mergeId;
        deiPrm[i].outQueParams[deiOutQue^1].nextLink            = nullId;
        deiPrm[i].enableOut[deiOutQue]                          = TRUE;
        deiPrm[i].enableOut[deiOutQue^1]                        = FALSE;
        deiPrm[i].tilerEnable[DEI_LINK_OUT_QUE_VIP_SC]          = FALSE;

        deiPrm[i].comprEnable                                   = FALSE;
        deiPrm[i].setVipScYuv422Format                          = FALSE;

        /* DEI Path Scalar ratio is 1:2, D1 => CIF */
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.numerator    = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.widthRatio.denominator  = 2;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0].ratio.heightRatio.denominator         = 2;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_DEI_SC][0];

        /* VIP Scalar ratio is 1:1 */
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_RATIO;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.numerator    = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.widthRatio.denominator  = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.numerator   = 1;
        deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].ratio.heightRatio.denominator     = 1;
        for (chId=1; chId < DEI_LINK_MAX_CH; chId++)
            deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][chId] = deiPrm[i].outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0];

        mergePrm.numInQue                     = numSubChains;
        mergePrm.inQueParams[i].prevLinkId    = gVcapModuleContext.deiId[i];
        mergePrm.inQueParams[i].prevLinkQueId = deiOutQue;
        mergePrm.outQueParams.nextLink        = dupId;
        mergePrm.notifyNextLink               = TRUE;

        dupPrm.inQueParams.prevLinkId         = mergeId;
        dupPrm.inQueParams.prevLinkQueId      = 0;
        dupPrm.numOutQue                      = numSubChains;
        dupPrm.outQueParams[i].nextLink       = gVdisModuleContext.swMsId[i];
        dupPrm.notifyNextLink                 = TRUE;
    }

    mergePrm.numInQue                     = 3;
    mergePrm.inQueParams[i].prevLinkId    = gVcapModuleContext.nullSrcId;
    mergePrm.inQueParams[i].prevLinkQueId = 0;

    for(i=0; i<numSubChains; i++)
    {
        swMsPrm[i].inQueParams.prevLinkId    = dupId;
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = gVdisModuleContext.displayId[i];
        swMsPrm[i].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes                 = VSYS_STD_1080P_60;
        swMsPrm[i].initOutRes                = gVdisModuleContext.vdisConfig.deviceParams[i].resolution;
        swMsPrm[i].lineSkipMode = TRUE;

        swMsPrm[i].enableLayoutGridDraw = gVdisModuleContext.vdisConfig.enableLayoutGridDraw;

        MultiCh_swMsGetDefaultLayoutPrm(0, &swMsPrm[i], FALSE); /* Since only live preview is there, show it on both displays */

        displayPrm[i].inQueParams[0].prevLinkId    = gVdisModuleContext.swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                = swMsPrm[i].initOutRes;
    }

    if(enableSdtv)
    {
        dupPrm.numOutQue                      = 3;
        dupPrm.outQueParams[2].nextLink       = gVdisModuleContext.displayId[2];
    }

    displayPrm[2].numInputQueues = 1;
    displayPrm[2].activeQueue    = 0;
    displayPrm[2].inQueParams[0].prevLinkId    = dupId;
    displayPrm[2].inQueParams[0].prevLinkQueId = 2;
    displayPrm[2].displayRes = gVdisModuleContext.vdisConfig.deviceParams[VDIS_DEV_SD].resolution;


#ifndef SYSTEM_USE_VIDEO_DECODER
    capturePrm.isPalMode = Vcap_isPalMode();
#endif

    System_linkCreate (gVcapModuleContext.captureId, &capturePrm, sizeof(capturePrm));
    System_linkCreate(gVcapModuleContext.nsfId[0] , &nsfPrm, sizeof(nsfPrm));

    for(i=0; i<numSubChains; i++)
        System_linkCreate(gVcapModuleContext.deiId[i]  , &deiPrm[i], sizeof(deiPrm[i]));

    System_linkCreate(gVcapModuleContext.nullSrcId , &nullSrcPrm, sizeof(nullSrcPrm));
    System_linkCreate(mergeId   , &mergePrm  , sizeof(mergePrm));
    System_linkCreate(dupId     , &dupPrm    , sizeof(dupPrm));

    for(i=0; i<numSubChains; i++)
        System_linkCreate(gVdisModuleContext.swMsId[i]  , &swMsPrm[i], sizeof(swMsPrm[i]));

    for(i=0; i<numSubChains; i++)
        System_linkCreate(gVdisModuleContext.displayId[i], &displayPrm[i], sizeof(displayPrm[i]));

    if(enableSdtv)
    {
        System_linkCreate(gVdisModuleContext.displayId[2], &displayPrm[2], sizeof(displayPrm[2]));
    }

    MultiCh_memPrintHeapStatus();

}


Void MultiCh_deleteVcapVdis()
{
    UInt32 grpxId[VDIS_DEV_MAX];
    UInt32 nullId;
    UInt32 mergeId, dupId;
    UInt32 i;
    UInt32 numSubChains;
    UInt32 enableSdtv   = FALSE;

    numSubChains = 2;

    gVcapModuleContext.captureId    = SYSTEM_LINK_ID_CAPTURE;
    gVcapModuleContext.nsfId[0]     = SYSTEM_LINK_ID_NSF_0;
    gVcapModuleContext.deiId[0]     = SYSTEM_LINK_ID_DEI_HQ_0;
    gVcapModuleContext.deiId[1]     = SYSTEM_LINK_ID_DEI_0;
    gVcapModuleContext.nullSrcId    = SYSTEM_VPSS_LINK_ID_NULL_SRC_0;
    mergeId      = SYSTEM_VPSS_LINK_ID_MERGE_0;
    dupId        = SYSTEM_VPSS_LINK_ID_DUP_0;

    gVdisModuleContext.swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    gVdisModuleContext.swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;

    gVdisModuleContext.displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    gVdisModuleContext.displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI
    gVdisModuleContext.displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; // OFF CHIP HDMI
    grpxId[0]    = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]    = SYSTEM_LINK_ID_GRPX_1;
#if 0    /* Enabling graphics only for ON CHIP HDMI an OFF CHIP HDMI*/
    grpxId[2]    = SYSTEM_LINK_ID_GRPX_2;
#endif
    nullId       = SYSTEM_VPSS_LINK_ID_NULL_0;

    System_linkDelete(gVcapModuleContext.captureId);
    System_linkDelete(gVcapModuleContext.nsfId[0]);

    for(i=0; i<numSubChains; i++)
        System_linkDelete(gVcapModuleContext.deiId[i] );

    System_linkDelete(gVcapModuleContext.nullSrcId);
    System_linkDelete(mergeId);
    System_linkDelete(dupId);

    for(i=0; i<numSubChains; i++)
        System_linkDelete(gVdisModuleContext.swMsId[i] );

    for(i=0; i<numSubChains; i++)
        System_linkDelete(gVdisModuleContext.displayId[i]);

    if(enableSdtv)
    {
        System_linkDelete(gVdisModuleContext.displayId[2]);
    }

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    MultiCh_prfLoadCalcEnable(FALSE, TRUE, FALSE);

}

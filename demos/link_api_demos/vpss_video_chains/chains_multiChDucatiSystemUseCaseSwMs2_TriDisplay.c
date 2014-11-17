/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <demos/graphic/graphic.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>


/**
                       Capture (YUV422I) 16CH D1 60fps
                          |
                         NSF (YUV420SP)
                          |
                        dup1-----IPCM3OUT(VPS)--IPCM3IN(VID)--ENC
                          |                                    |
                          +------IPCM3IN(VPS)--IPCM3OUT(VID)--DEC
                          |
                        MERGE
                          |
                         DUP2
                         |||
         +---------------+|+------------+
         |                |             |
         |                |             |
      SW Mosaic       SW Mosaic        SW Moasic
      (DEIH YUV422I)  (DEI YUV422I)    (SC5 YUV422I)
         |                |             |
 GRPX0   |       GRPX1,2  |             |
    |    |           |    |             |
    On-Chip HDMI    Off-Chip HDMI  SDTV (NTSC)
      1080p60         1080p60        480i60
*/

/* To enable or disable graphics in the application */
#define ENABLE_GRPX 0

/* To select if FBDEV interface is used for Graphics */
#define USE_FBDEV   0

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 10,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0 , 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 16,
        .EncChList = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
        .DecNumCh  = 12,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0 , 0, 0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 6,
        .EncChList = {10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .DecNumCh  = 4,
        .DecChList = {12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

Void chains_multiChDucatiSystemUseCaseSwMsTriDisplay2(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams    capturePrm;
    NsfLink_CreateParams        nsfPrm;
    DupLink_CreateParams        dup1Prm;
    DupLink_CreateParams        dup2Prm;
    static SwMsLink_CreateParams       swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    DisplayLink_CreateParams    displayPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    IpcLink_CreateParams        ipcOutVpssPrm;
    IpcLink_CreateParams        ipcInVpssPrm;
    IpcLink_CreateParams        ipcOutVideoPrm;
    IpcLink_CreateParams        ipcInVideoPrm;
    EncLink_CreateParams        encPrm;
    DecLink_CreateParams        decPrm;
    MergeLink_CreateParams      mergePrm;

    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;

    UInt32 captureId;
    UInt32 nsfId;
    UInt32 dup1Id, dup2Id, mergeId;
    UInt32 encId, decId;
    UInt32 swMsId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 displayId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 grpxId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    UInt32 vipInstId;
    UInt32 i, numSubChains;

    UInt32 numDisplay,  enableGrpx;
    UInt32 displayRes[SYSTEM_DC_MAX_VENC];

    Bool enabledup2;
    Bool switchCh;
    Bool switchLayout;

    char ch;

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams, capturePrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(DecLink_CreateParams, decPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams, encPrm);
    for (i = 0; i < CHAINS_SW_MS_MAX_DISPLAYS; i++)
    {
        CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
        CHAINS_INIT_STRUCT(SwMsLink_CreateParams, swMsPrm[i]);
    }

    captureId    = SYSTEM_LINK_ID_CAPTURE;
    nsfId        = SYSTEM_LINK_ID_NSF_0;
    dup1Id       = SYSTEM_VPSS_LINK_ID_DUP_0;
    dup2Id       = SYSTEM_VPSS_LINK_ID_DUP_1;
    mergeId      = SYSTEM_VPSS_LINK_ID_MERGE_0;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    encId        = SYSTEM_LINK_ID_VENC_0;
    decId        = SYSTEM_LINK_ID_VDEC_0;


    swMsId[0]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    swMsPrm[0].numSwMsInst = 1;
    swMsPrm[0].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEIHQ_SC;
    swMsId[1]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    swMsPrm[1].numSwMsInst = 1;
    swMsPrm[1].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_DEI_SC;
    swMsId[2]      = SYSTEM_LINK_ID_SW_MS_MULTI_INST_2;
    swMsPrm[2].numSwMsInst = 1;
    swMsPrm[2].swMsInstId[0] = SYSTEM_SW_MS_SC_INST_SC5;

    displayId[0] = SYSTEM_LINK_ID_DISPLAY_0; // OFF CHIP HDMI
    displayId[1] = SYSTEM_LINK_ID_DISPLAY_1; // ON CHIP HDMI
    displayId[2] = SYSTEM_LINK_ID_DISPLAY_2; // SDTV

    grpxId[0]    = SYSTEM_LINK_ID_GRPX_0;
    grpxId[1]    = SYSTEM_LINK_ID_GRPX_1;
    grpxId[2]    = SYSTEM_LINK_ID_GRPX_2;

    enableGrpx     = ENABLE_GRPX;
    numDisplay     = 3;
    memcpy(displayRes,chainsCfg->displayRes,sizeof(displayRes));
    numSubChains   = 2;

    enabledup2 = FALSE;
    if(numDisplay>1)
        enabledup2 = TRUE;

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    CaptureLink_CreateParams_Init(&capturePrm);

    capturePrm.numVipInst    = 4;
    capturePrm.outQueParams[0].nextLink = nsfId;
    capturePrm.tilerEnable              = FALSE;

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
        pCaptureOutPrm->outQueId            = 0;
    }
    chainsCfg->bypassNsf             = TRUE;
    nsfPrm.bypassNsf                 = chainsCfg->bypassNsf;
    nsfPrm.tilerEnable               = TRUE;
    nsfPrm.inQueParams.prevLinkId    = captureId;
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.numOutQue                 = 1;
    nsfPrm.outQueParams[0].nextLink  = dup1Id;

    dup1Prm.inQueParams.prevLinkId  = nsfId;
    dup1Prm.inQueParams.prevLinkQueId   = 0;
    dup1Prm.numOutQue                   = numSubChains;
    dup1Prm.notifyNextLink              = TRUE;
    dup1Prm.outQueParams[0].nextLink = ipcOutVpssId;
    dup1Prm.outQueParams[1].nextLink = mergeId;

    ipcOutVpssPrm.inQueParams.prevLinkId    = dup1Id;
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 1;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = FALSE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = TRUE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.outQueParams[0].nextLink     = encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = TRUE;

    for (i=0; i<16; i++) {
        encPrm.chCreateParams[i].format     = IVIDEO_H264HP;
        encPrm.chCreateParams[i].profile    = IH264_HIGH_PROFILE;
        encPrm.chCreateParams[i].dataLayout = IVIDEO_FIELD_SEPARATED;
        encPrm.chCreateParams[i].fieldMergeEncodeEnable  = TRUE;
        encPrm.chCreateParams[i].defaultDynamicParams.intraFrameInterval = 30;
        encPrm.chCreateParams[i].encodingPreset = XDM_DEFAULT;
        encPrm.chCreateParams[i].enableAnalyticinfo = 0;
        encPrm.chCreateParams[i].enableWaterMarking = 0;
        encPrm.chCreateParams[i].rateControlPreset =
                                 IVIDEO_STORAGE;
        encPrm.chCreateParams[i].defaultDynamicParams.inputFrameRate = 30;
        encPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
                                 (2 * 1000 * 1000);
        encPrm.chCreateParams[i].defaultDynamicParams.interFrameInterval = 1;
        encPrm.chCreateParams[i].defaultDynamicParams.mvAccuracy =
                                 IVIDENC2_MOTIONVECTOR_QUARTERPEL;
    }
    encPrm.inQueParams.prevLinkId   = ipcInVideoId;
    encPrm.inQueParams.prevLinkQueId= 0;
    encPrm.outQueParams.nextLink = decId;

    for (i=0; i<16; i++) {
        decPrm.chCreateParams[i].format          = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile         = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].targetMaxWidth  = 720;
        decPrm.chCreateParams[i].targetMaxHeight = 576;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = TRUE;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = 30;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
                                 (2 * 1000 * 1000);
    }
    decPrm.inQueParams.prevLinkId = encId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcOutVideoId;
    decPrm.tilerEnable = TRUE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                 = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink     = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = FALSE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = TRUE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink     = mergeId;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = TRUE;

    mergePrm.numInQue                     = numSubChains;
    mergePrm.inQueParams[0].prevLinkId    = dup1Id;
    mergePrm.inQueParams[0].prevLinkQueId = 0;
    mergePrm.inQueParams[1].prevLinkId    = ipcInVpssId;
    mergePrm.inQueParams[1].prevLinkQueId = 0;
    mergePrm.notifyNextLink               = TRUE;

    if(enabledup2)
       mergePrm.outQueParams.nextLink        = dup2Id;
    else
       mergePrm.outQueParams.nextLink        = swMsId[0];

    dup2Prm.inQueParams.prevLinkId      = mergeId;
    dup2Prm.inQueParams.prevLinkQueId   = 0;
    dup2Prm.numOutQue                   = numDisplay;
    dup2Prm.notifyNextLink              = TRUE;


    for(i=0; i<numDisplay; i++)
    {
        if(enabledup2)
            swMsPrm[i].inQueParams.prevLinkId = dup2Id;
        else
            swMsPrm[i].inQueParams.prevLinkId = mergeId;

        swMsPrm[i].inQueParams.prevLinkQueId = i;

        swMsPrm[i].outQueParams.nextLink = displayId[i];

        swMsPrm[i].lineSkipMode = TRUE;
        swMsPrm[i].layoutPrm.outputFPS = 60;
        swMsPrm[i].maxInputQueLen = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes   = displayRes[i];

        /* SDTV is @ 30fps */
        if(i==2) {
            swMsPrm[i].lineSkipMode = TRUE;
            swMsPrm[i].layoutPrm.outputFPS = 30;
            swMsPrm[i].maxOutRes         = VSYS_STD_NTSC;
        }

        Chains_swMsGenerateLayoutParams(i, 0, &swMsPrm[i]);


        dup2Prm.outQueParams[i].nextLink = swMsId[i];

        displayPrm[i].inQueParams[0].prevLinkId    = swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].displayRes                = swMsPrm[i].maxOutRes        ;
    }

    Chains_displayCtrlInit(displayRes);

    System_linkCreate (captureId, &capturePrm, sizeof(capturePrm));
    System_linkControl(captureId, CAPTURE_LINK_CMD_CONFIGURE_VIP_DECODERS, NULL, 0, TRUE);

    System_linkCreate(nsfId     , &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(dup1Id     , &dup1Prm, sizeof(dup1Prm));
    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );
    System_linkCreate(encId, &encPrm, sizeof(encPrm));
    System_linkCreate(decId, &decPrm, sizeof(decPrm));
    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(mergeId   , &mergePrm  , sizeof(mergePrm));

    if(enabledup2)
        System_linkCreate(dup2Id     , &dup2Prm, sizeof(dup2Prm));

    for(i=0; i<numDisplay; i++)
    {
        System_linkCreate(swMsId[i], &swMsPrm[i], sizeof(swMsPrm[i]));
        System_linkCreate(displayId[i], &displayPrm[i], sizeof(displayPrm[i]));
    }

    Chains_memPrintHeapStatus();
    //Utils_encdecHdvicpPrfInit();

    {
        if(enableGrpx)
        {
#if USE_FBDEV
            grpx_init(GRPX_FORMAT_RGB888);
#else
            for(i=0; i<numDisplay; i++)
               Chains_grpxEnable(grpxId[i], TRUE);
#endif
        }

        for(i=0; i<numDisplay; i++)
        {
            System_linkStart(displayId[i]);
            System_linkStart(swMsId[i] );
        }

        if(enabledup2)
            System_linkStart(dup2Id    );

        System_linkStart(decId);
        System_linkStart(encId);
        System_linkStart(dup1Id    );

        System_linkStart(nsfId    );

        /* Start taking CPU load just before starting of links */
        Chains_prfLoadCalcEnable(TRUE, FALSE, FALSE);

        System_linkStart(captureId);
        if(enableGrpx)
        {
#if USE_FBDEV
            grpx_draw();
#endif
        }

        while(1)
        {
            ch = Chains_menuRunTime();

            switchLayout = FALSE;
            switchCh = FALSE;

            if(ch=='0')
                break;
            if(ch=='p')
                System_linkControl(captureId,
                       CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
            if(ch=='s')
               switchLayout = TRUE;
            if(ch=='c')
               switchCh = TRUE;
            if(ch=='i')
            {
                System_linkControl(encId,
                       ENC_LINK_CMD_PRINT_IVAHD_STATISTICS, NULL, 0, TRUE);
            }
            if(ch=='m')
            {
                printf(" STATISTICS for SwMs with linkId: %d \n", swMsId[0]);
                System_linkControl(swMsId[0],
                       SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS, NULL, 0, TRUE);
                printf(" STATISTICS for SwMs with linkId: %d \n", swMsId[1]);
                System_linkControl(swMsId[1],
                       SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS, NULL, 0, TRUE);
            }
            Chains_swMsSwitchLayout(swMsId, swMsPrm, switchLayout,
                                    switchCh, numDisplay);
        }

        System_linkStop(captureId);
        System_linkStop(nsfId    );
        System_linkStop(dup1Id    );

        System_linkStop(encId);
        System_linkStop(decId);

        if(enabledup2)
            System_linkStop(dup2Id    );

        for(i=0; i<numDisplay; i++)
        {
            System_linkStop(swMsId[i] );
            System_linkStop(displayId[i]);
        }

          if(enableGrpx)
        {
#if USE_FBDEV
            grpx_exit();
#else
            for(i=0; i<numDisplay; i++)
                Chains_grpxEnable(grpxId[i], FALSE);
#endif
        }
    }

    System_linkDelete(captureId   );
    System_linkDelete(nsfId       );

    System_linkDelete(dup1Id       );
    System_linkDelete(ipcOutVpssId );
    System_linkDelete(ipcInVideoId );

    System_linkDelete(encId);
    System_linkDelete(decId);

    System_linkDelete(ipcOutVideoId);
    System_linkDelete(ipcInVpssId  );

    System_linkDelete(mergeId);

    if(enabledup2)
        System_linkDelete(dup2Id       );
    for(i=0; i<numDisplay; i++)
    {
        System_linkDelete(swMsId[i]);
        System_linkDelete(displayId[i]);
    }

    Chains_displayCtrlDeInit();

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    Chains_prfLoadCalcEnable(FALSE, TRUE, FALSE);
    //Utils_encdecHdvicpPrfPrint();

}



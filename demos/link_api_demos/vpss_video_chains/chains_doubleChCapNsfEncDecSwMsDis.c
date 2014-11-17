/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <mcfw/interfaces/ti_venc.h>

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 1,
        .EncChList = {0},
        .DecNumCh  = 0,
        .DecChList = {0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 0,
        .EncChList = {0},
        .DecNumCh  = 1,
        .DecChList = {0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 1,
        .EncChList = {1},
        .DecNumCh  = 1,
        .DecChList = {1},
    },
};

Void Chains_doubleChCapNsfEncDecSwMsDis(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams    capturePrm;
    CaptureLink_VipInstParams   *pCaptureInstPrm;
    CaptureLink_OutParams       *pCaptureOutPrm;
    NsfLink_CreateParams        nsfPrm;
    IpcLink_CreateParams        ipcOutVpssPrm;
    IpcLink_CreateParams        ipcInVpssPrm;
    EncLink_CreateParams        encPrm;
    DecLink_CreateParams        decPrm;
    IpcLink_CreateParams        ipcOutVideoPrm;
    IpcLink_CreateParams        ipcInVideoPrm;
    SelectLink_CreateParams     selPrm;
    SwMsLink_CreateParams       swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    DisplayLink_CreateParams    displayPrm[CHAINS_SW_MS_MAX_DISPLAYS];

    UInt32 captureId, vipInstId;
    UInt32 nsfId;
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 encId, decId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    UInt32 selId;
    UInt32 swMsId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 displayId[CHAINS_SW_MS_MAX_DISPLAYS];

    UInt32 i;
    char   ch;

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams,capturePrm);
    CHAINS_INIT_STRUCT(NsfLink_CreateParams,nsfPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams,encPrm);
    CHAINS_INIT_STRUCT(DecLink_CreateParams,decPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(SelectLink_CreateParams,selPrm);
    for (i=0; i<CHAINS_SW_MS_MAX_DISPLAYS; i++) {
        CHAINS_INIT_STRUCT(SwMsLink_CreateParams,swMsPrm[i]);
        CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
    }

    captureId     = SYSTEM_LINK_ID_CAPTURE;
    nsfId         = SYSTEM_LINK_ID_NSF_0;
    ipcOutVpssId  = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId  = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    encId         = SYSTEM_LINK_ID_VENC_0;
    decId         = SYSTEM_LINK_ID_VDEC_0;
    ipcOutVideoId = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId   = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;
    selId         = SYSTEM_VPSS_LINK_ID_SELECT_0;
    swMsId[0]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    swMsId[1]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    displayId[0]  = SYSTEM_LINK_ID_DISPLAY_0;
    displayId[1]  = SYSTEM_LINK_ID_DISPLAY_1;

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    capturePrm.numVipInst               = 2;
    capturePrm.tilerEnable              = FALSE;
    capturePrm.numBufsPerCh             = 16;
    capturePrm.numExtraBufs             = 0;
    capturePrm.maxBlindAreasPerCh       = 0;
    capturePrm.isPalMode                = FALSE;
    capturePrm.enableSdCrop             = FALSE;
    capturePrm.doCropInCapture          = FALSE;

    for(i=0; i<capturePrm.numVipInst; i++)
    {
    	vipInstId = SYSTEM_CAPTURE_INST_VIP0_PORTA + 2*i;
        pCaptureInstPrm                     = &capturePrm.vipInst[i];
        pCaptureInstPrm->vipInstId          = vipInstId%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_SII9233A_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = chainsCfg->displayRes[i];
        pCaptureInstPrm->numOutput          = 1;
        pCaptureInstPrm->numChPerOutput     = 1;
        pCaptureInstPrm->frameCaptureMode   = FALSE;
        pCaptureInstPrm->fieldsMerged       = FALSE;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = chainsCfg->channelConf[i].width;
        pCaptureOutPrm->scOutHeight         = chainsCfg->channelConf[i].height;
        pCaptureOutPrm->outQueId            = 0;
    }
    capturePrm.outQueParams[0].nextLink     = nsfId;

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    for (i=0; i<gChains_ctrl.channelNum; i++) {
        chainsCfg->displayRes[i] = capturePrm.vipInst[i].standard;
        Vsys_getResSize(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].width, &chainsCfg->channelConf[i].height);
        Vsys_getResRate(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].frameRate);
    }

    nsfPrm.inQueParams.prevLinkId           = captureId;
    nsfPrm.inQueParams.prevLinkQueId        = 0;
    nsfPrm.outQueParams[0].nextLink         = ipcOutVpssId;
    nsfPrm.bypassNsf                        = TRUE;
    nsfPrm.tilerEnable                      = TRUE;
    nsfPrm.numOutQue                        = 1;
    nsfPrm.numBufsPerCh                     = 0;

    ipcOutVpssPrm.inQueParams.prevLinkId    = nsfId;
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.numChPerOutQue[0]         = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink  = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = TRUE;
    ipcOutVpssPrm.notifyPrevLink            = FALSE;
    ipcOutVpssPrm.noNotifyMode              = FALSE;
    ipcOutVpssPrm.numChPerOutQue[0]         = 0;
    ipcOutVpssPrm.equallyDivideChAcrossOutQues = FALSE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.numChPerOutQue[0]         = 1;
    ipcInVideoPrm.outQueParams[0].nextLink  = encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = FALSE;
    ipcInVideoPrm.numChPerOutQue[0]         = 0;
    ipcInVideoPrm.equallyDivideChAcrossOutQues = FALSE;

    for (i=0; i<2; i++) {
        encPrm.chCreateParams[i].format                             = chainsCfg->channelConf[i].encFormat;
        encPrm.chCreateParams[i].profile                            = chainsCfg->channelConf[i].encProfile;
        encPrm.chCreateParams[i].dataLayout                         = IVIDEO_FIELD_SEPARATED;
        encPrm.chCreateParams[i].fieldMergeEncodeEnable             = FALSE;
        encPrm.chCreateParams[i].enableAnalyticinfo                 = FALSE;
        encPrm.chCreateParams[i].enableWaterMarking                 = FALSE;
        encPrm.chCreateParams[i].encodingPreset                     = XDM_USER_DEFINED;
        encPrm.chCreateParams[i].rateControlPreset                  = IVIDEO_USER_DEFINED;
        encPrm.chCreateParams[i].enableHighSpeed                    = FALSE;
        encPrm.chCreateParams[i].defaultDynamicParams.interFrameInterval  = 1;
        encPrm.chCreateParams[i].defaultDynamicParams.intraFrameInterval  = chainsCfg->channelConf[i].intraFrameInterval;
        encPrm.chCreateParams[i].defaultDynamicParams.inputFrameRate      = chainsCfg->channelConf[i].frameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate     = chainsCfg->channelConf[i].encFrameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetBitRate       = chainsCfg->channelConf[i].bitRate;
        encPrm.chCreateParams[i].defaultDynamicParams.mvAccuracy          = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
        encPrm.chCreateParams[i].defaultDynamicParams.rcAlg               = chainsCfg->channelConf[i].rateCtrl;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMin               = 0;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMax               = 51;
        encPrm.chCreateParams[i].defaultDynamicParams.qpInit              = -1;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrDuration         = 8;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrSensitivity      = 0;
    }
    encPrm.inQueParams.prevLinkId   = ipcInVideoId;
    encPrm.inQueParams.prevLinkQueId= 0;
    encPrm.outQueParams.nextLink    = decId;
    encPrm.numBufPerCh[0]           = 0;

    Chains_displayCtrlInit(chainsCfg->displayRes);

    System_linkCreate(nsfId, &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(ipcOutVpssId, &ipcOutVpssPrm, sizeof(ipcOutVpssPrm));
    System_linkCreate(ipcInVideoId, &ipcInVideoPrm, sizeof(ipcInVideoPrm));
    System_linkCreate(encId, &encPrm, sizeof(encPrm));

    for (i=0; i<2; i++) {
        decPrm.chCreateParams[i].format                               = chainsCfg->channelConf[i].encFormat;
        decPrm.chCreateParams[i].profile                              = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].targetMaxWidth                       = chainsCfg->channelConf[i].width;
        decPrm.chCreateParams[i].targetMaxHeight                      = chainsCfg->channelConf[i].height;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable               = FALSE;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = chainsCfg->channelConf[i].encFrameRate;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate   = chainsCfg->channelConf[i].bitRate;
        decPrm.chCreateParams[i].numBufPerCh                          = 0;
    }
    decPrm.inQueParams.prevLinkId    = encId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcOutVideoId;
    decPrm.tilerEnable               = TRUE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                 = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink  = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = TRUE;
    ipcOutVideoPrm.notifyPrevLink            = FALSE;
    ipcOutVideoPrm.noNotifyMode              = FALSE;
    ipcOutVideoPrm.numChPerOutQue[0]         = 0;
    ipcOutVideoPrm.equallyDivideChAcrossOutQues = FALSE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink  = selId;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = FALSE;
    ipcInVpssPrm.numChPerOutQue[0]         = 0;
    ipcInVpssPrm.equallyDivideChAcrossOutQues = FALSE;

    selPrm.inQueParams.prevLinkId       = ipcInVpssId;
    selPrm.inQueParams.prevLinkQueId    = 0;
    selPrm.numOutQue                    = 2;
    selPrm.outQueParams[0].nextLink     = swMsId[0];
    selPrm.outQueChInfo[0].outQueId     = 0;
    selPrm.outQueChInfo[0].numOutCh     = 1;
    selPrm.outQueChInfo[0].inChNum[0]   = 0;
    selPrm.outQueParams[1].nextLink     = swMsId[1];
    selPrm.outQueChInfo[1].outQueId     = 1;
    selPrm.outQueChInfo[1].numOutCh     = 1;
    selPrm.outQueChInfo[1].inChNum[0]   = 1;

    for (i=0; i<2; i++) {
        swMsPrm[i].numSwMsInst               = 1;
        swMsPrm[i].swMsInstId[0]             = SYSTEM_SW_MS_SC_INST_SC5;
        swMsPrm[i].inQueParams.prevLinkId    = selId;
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = displayId[i];
        swMsPrm[i].lineSkipMode              = FALSE;
        swMsPrm[i].enableLayoutGridDraw      = FALSE;
        swMsPrm[i].layoutPrm.outputFPS       = chainsCfg->channelConf[i].frameRate;
        swMsPrm[i].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes                 = chainsCfg->displayRes[i];
        swMsPrm[i].numOutBuf                 = 0;
        Chains_swMsSetLayoutParams(0, &swMsPrm[i]);

        displayPrm[i].numInputQueues                = 1;
        displayPrm[i].activeQueue                   = 0;
        displayPrm[i].inQueParams[0].prevLinkId     = swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId  = 0;
        displayPrm[i].displayRes                    = chainsCfg->displayRes[i];
        displayPrm[i].forceFieldSeparatedInputMode  = FALSE;
    }

    System_linkCreate(decId, &decPrm, sizeof(decPrm));
    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId, &ipcInVpssPrm, sizeof(ipcInVpssPrm));
    System_linkCreate(selId, &selPrm, sizeof(selPrm));
    for (i=0; i<2; i++) {
        System_linkCreate(swMsId[i], &swMsPrm[i], sizeof(swMsPrm[i]));
        System_linkCreate(displayId[i], &displayPrm[i], sizeof(displayPrm[i]));
    }

    Chains_memPrintHeapStatus();

    {
        for (i=0; i<2; i++) {
            System_linkStart(displayId[i]);
            System_linkStart(swMsId[i]);
        }
        System_linkStart(selId);
        System_linkStart(decId);
        System_linkStart(encId);
        System_linkStart(nsfId);
        System_linkStart(captureId);

        while(1)
        {
            ch = Chains_menuRunTime();
            if(ch=='0')
                break;
            if(ch=='v')
                System_linkControl(captureId, CAPTURE_LINK_CMD_FORCE_RESET, NULL, 0, TRUE);
            if(ch=='p')
                System_linkControl(captureId, CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
        }

        System_linkStop(captureId);
        System_linkStop(nsfId);
        System_linkStop(encId);
        System_linkStop(decId);
        System_linkStop(selId);
        for (i=0; i<2; i++) {
            System_linkStop(swMsId[i]);
            System_linkStop(displayId[i]);
        }
    }

    System_linkDelete(captureId);
    System_linkDelete(nsfId);
    System_linkDelete(ipcOutVpssId);
    System_linkDelete(ipcInVideoId);
    System_linkDelete(encId);
    System_linkDelete(decId);
    System_linkDelete(ipcOutVideoId);
    System_linkDelete(ipcInVpssId);
    System_linkDelete(selId);
    for (i=0; i<2; i++) {
        System_linkDelete(swMsId[i]);
        System_linkDelete(displayId[i]);
    }

    Chains_displayCtrlDeInit();
}

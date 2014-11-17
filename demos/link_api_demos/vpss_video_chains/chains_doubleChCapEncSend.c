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
        .EncNumCh  = 1,
        .EncChList = {1},
        .DecNumCh  = 0,
        .DecChList = {0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 2,
        .EncChList = {2,3},
        .DecNumCh  = 0,
        .DecChList = {0},
    },
};

Void Chains_doubleChCapEncSend(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams            capturePrm;
    CaptureLink_VipInstParams           *pCaptureInstPrm;
    CaptureLink_OutParams               *pCaptureOutPrm;
    DupLink_CreateParams                dupPrm;
    DupLink_CreateParams                dup2Prm;
    SwMsLink_CreateParams               swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    DisplayLink_CreateParams            displayPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    IpcLink_CreateParams                ipcOutVpssPrm;
    IpcLink_CreateParams                ipcInVideoPrm;
    EncLink_CreateParams                encPrm;
    IpcBitsOutLinkRTOS_CreateParams     ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams      ipcBitsInHostPrm;

    UInt32 captureId, vipInstId;
    UInt32 dupId;
    UInt32 dup2Id;
    UInt32 swMsId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 displayId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 ipcOutVpssId;
    UInt32 ipcInVideoId;
    UInt32 encId;
    UInt32 ipcBitsOutVideoId;
    UInt32 ipcBitsInHostId;

    UInt32 enableDisplay = FALSE;
    UInt32 i;
    char   ch;

    Chains_ipcBitsInit();

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams,capturePrm);
    for (i=0; i<CHAINS_SW_MS_MAX_DISPLAYS; i++) {
        CHAINS_INIT_STRUCT(SwMsLink_CreateParams,swMsPrm[i]);
        CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
    }
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams,encPrm);
    CHAINS_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);

    captureId     = SYSTEM_LINK_ID_CAPTURE;
    dupId         = SYSTEM_VPSS_LINK_ID_DUP_0;
    dup2Id        = SYSTEM_VPSS_LINK_ID_DUP_1;
    swMsId[0]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    swMsId[1]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    displayId[0]  = SYSTEM_LINK_ID_DISPLAY_0;
    displayId[1]  = SYSTEM_LINK_ID_DISPLAY_1;

    encId         = SYSTEM_LINK_ID_VENC_0;
    ipcOutVpssId  = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId  = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcBitsOutVideoId   = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInHostId     = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    capturePrm.numVipInst               = 2;
    capturePrm.tilerEnable              = TRUE;
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
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV420SP_UV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = gChains_ctrl.channelConf[i].width;
        pCaptureOutPrm->scOutHeight         = gChains_ctrl.channelConf[i].height;
        pCaptureOutPrm->outQueId            = 0;
    }
    capturePrm.outQueParams[0].nextLink     = dupId;

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    for (i=0; i<gChains_ctrl.channelNum; i++) {
        chainsCfg->displayRes[i] = capturePrm.vipInst[i].standard;
        Vsys_getResSize(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].width, &chainsCfg->channelConf[i].height);
        Vsys_getResRate(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].frameRate);
    }

    dupPrm.inQueParams.prevLinkId           = captureId;
    dupPrm.inQueParams.prevLinkQueId        = 0;
    dupPrm.numOutQue                        = 2;
    dupPrm.outQueParams[0].nextLink         = ipcOutVpssId;
    dupPrm.outQueParams[1].nextLink         = dup2Id;
    dupPrm.notifyNextLink                   = TRUE;

    ipcOutVpssPrm.inQueParams.prevLinkId    = dupId;
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

    for (i=0; i<gChains_ctrl.channelNum; i++) {
    //for (i=0; i<1; i++) {
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
    encPrm.outQueParams.nextLink    = ipcBitsOutVideoId;
    encPrm.numBufPerCh[0]           = 0;

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId      = encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId   = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                   = 1;
    ipcBitsOutVideoPrm.baseCreateParams.numChPerOutQue[0]           = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink    = ipcBitsInHostId;
    ipcBitsOutVideoPrm.baseCreateParams.notifyNextLink              = FALSE;
    ipcBitsOutVideoPrm.baseCreateParams.notifyPrevLink              = FALSE;
    ipcBitsOutVideoPrm.baseCreateParams.noNotifyMode                = TRUE;
    Chains_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm,FALSE);

    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId        = ipcBitsOutVideoId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId     = 0;
    ipcBitsInHostPrm.baseCreateParams.numOutQue                     = 1;
    ipcBitsInHostPrm.baseCreateParams.numChPerOutQue[0]             = 1;
    ipcBitsInHostPrm.baseCreateParams.notifyNextLink                = FALSE;
    ipcBitsInHostPrm.baseCreateParams.notifyPrevLink                = FALSE;
    ipcBitsInHostPrm.baseCreateParams.noNotifyMode                  = TRUE;
    Chains_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);

    Chains_displayCtrlInit(chainsCfg->displayRes);
    System_linkCreate(dupId, &dupPrm, sizeof(dupPrm));
    System_linkCreate(ipcOutVpssId, &ipcOutVpssPrm, sizeof(ipcOutVpssPrm));
    System_linkCreate(ipcInVideoId, &ipcInVideoPrm, sizeof(ipcInVideoPrm));
    System_linkCreate(encId, &encPrm, sizeof(encPrm));
    System_linkCreate(ipcBitsOutVideoId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(ipcBitsInHostId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));

    dup2Prm.inQueParams.prevLinkId           = dupId;
    dup2Prm.inQueParams.prevLinkQueId        = 1;
    dup2Prm.numOutQue                        = 2;
    dup2Prm.outQueParams[0].nextLink         = swMsId[0];
    dup2Prm.outQueParams[1].nextLink         = swMsId[1];
    dup2Prm.notifyNextLink                   = TRUE;

    for (i=0; i<2; i++) {
        swMsPrm[i].numSwMsInst               = 1;
        swMsPrm[i].swMsInstId[0]             = SYSTEM_SW_MS_SC_INST_SC5;
        swMsPrm[i].inQueParams.prevLinkId    = dup2Id;
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = displayId[i];
        swMsPrm[i].lineSkipMode              = FALSE;
        swMsPrm[i].enableLayoutGridDraw      = FALSE;
        swMsPrm[i].layoutPrm.outputFPS       = chainsCfg->channelConf[i].frameRate;
        swMsPrm[i].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes                 = chainsCfg->displayRes[i];
        swMsPrm[i].numOutBuf                 = 0;
        Chains_swMsSetLayoutParams(i, &swMsPrm[i]);

        displayPrm[i].numInputQueues                = 1;
        displayPrm[i].activeQueue                   = 0;
        displayPrm[i].inQueParams[0].prevLinkId     = swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId  = 0;
        displayPrm[i].displayRes                    = chainsCfg->displayRes[i];
        displayPrm[i].forceFieldSeparatedInputMode  = FALSE;
    }

    System_linkCreate(dup2Id, &dup2Prm, sizeof(dup2Prm));
    for (i=0; i<2; i++)
    {
        System_linkCreate(swMsId[i], &swMsPrm[i], sizeof(swMsPrm[i]));
        System_linkCreate(displayId[i], &displayPrm[i], sizeof(displayPrm[i]));
    }

    Chains_memPrintHeapStatus();

    {
        System_linkStart(encId);
        for (i=0; i<2; i++) {
            System_linkStart(displayId[i]);
            System_linkStart(swMsId[i]);
        }
        System_linkStart(dup2Id);
        System_linkStart(dupId);
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
        System_linkStop(dupId);
        System_linkStop(dup2Id);
        for (i=0; i<2; i++) {
            System_linkStop(swMsId[i]);
            System_linkStop(displayId[i]);
        }
        System_linkStop(encId);
        Chains_ipcBitsStop();
        System_linkStop(ipcBitsOutVideoId);
        System_linkStop(ipcBitsInHostId);
    }

    System_linkDelete(captureId);
    System_linkDelete(dupId);
    System_linkDelete(dup2Id);
    for (i=0; i<2; i++) {
        System_linkDelete(swMsId[i]);
        System_linkDelete(displayId[i]);
    }
    System_linkDelete(ipcOutVpssId);
    System_linkDelete(ipcInVideoId);
    System_linkDelete(encId);
    System_linkDelete(ipcBitsOutVideoId);
    System_linkDelete(ipcBitsInHostId);
    Chains_ipcBitsExit();

    Chains_displayCtrlDeInit();
}

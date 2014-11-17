/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>

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
        .EncNumCh  = 0,
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

Void Chains_singleChRecvDecDis(Chains_Ctrl *chainsCfg)
{
    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    DecLink_CreateParams              decPrm;
    IpcLink_CreateParams              ipcOutVideoPrm;
    IpcLink_CreateParams              ipcInVpssPrm;
    SwMsLink_CreateParams             swMsPrm;
    DisplayLink_CreateParams          displayPrm;

    UInt32 ipcBitsOutHostId;
    UInt32 ipcBitsInVideoId;
    UInt32 decId;
    UInt32 ipcOutVideoId;
    UInt32 ipcInVpssId;
    UInt32 swMsId;
    UInt32 displayId;

    UInt32 i;
    char   ch;

    Chains_ipcBitsInit();

    CHAINS_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    CHAINS_INIT_STRUCT(DecLink_CreateParams, decPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    CHAINS_INIT_STRUCT(SwMsLink_CreateParams, swMsPrm);
    CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);

    ipcBitsOutHostId    = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInVideoId    = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;
    decId               = SYSTEM_LINK_ID_VDEC_0;
    ipcOutVideoId       = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId         = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;
    swMsId              = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    displayId           = SYSTEM_LINK_ID_DISPLAY_1;

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    Chains_displayCtrlInit(chainsCfg->displayRes);

    System_LinkQueInfo queInfo;
    queInfo.numCh = 1;
    queInfo.chInfo[0].bufType           = SYSTEM_BUF_TYPE_VIDBITSTREAM;
    queInfo.chInfo[0].codingformat      = chainsCfg->channelConf[0].encFormat;
    queInfo.chInfo[0].height            = chainsCfg->channelConf[0].height;
    queInfo.chInfo[0].width             = chainsCfg->channelConf[0].width;
    queInfo.chInfo[0].memType           = SYSTEM_MT_TILEDMEM;
    queInfo.chInfo[0].dataFormat        = SYSTEM_DF_YUV420P;
    queInfo.chInfo[0].scanFormat        = SYSTEM_SF_PROGRESSIVE;

    ipcBitsOutHostPrm.baseCreateParams.numOutQue                    = 1;
    ipcBitsOutHostPrm.baseCreateParams.numChPerOutQue[0]            = 1;
    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink     = ipcBitsInVideoId;
    ipcBitsOutHostPrm.bufPoolPerCh                                  = FALSE;
    ipcBitsOutHostPrm.numBufPerCh[0]                                = 0;
    Chains_ipcBitsInitCreateParams_BitsOutHLOS(&ipcBitsOutHostPrm,
                                               &queInfo);

    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId       = ipcBitsOutHostId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId    = 0;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue                    = 1;
    ipcBitsInVideoPrm.baseCreateParams.numChPerOutQue[0]            = 1;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink     = decId;
    Chains_ipcBitsInitCreateParams_BitsInRTOS(&ipcBitsInVideoPrm,
                                              TRUE);

    for (i=0; i<1; i++) {
        decPrm.chCreateParams[i].format                               = chainsCfg->channelConf[i].encFormat;
        decPrm.chCreateParams[i].profile                              = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].targetMaxWidth                       = chainsCfg->channelConf[i].width;
        decPrm.chCreateParams[i].targetMaxHeight                      = chainsCfg->channelConf[i].height;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable               = FALSE;
        decPrm.chCreateParams[i].algCreateStatus                      = DEC_LINK_ALG_CREATE_STATUS_CREATE;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = chainsCfg->channelConf[i].encFrameRate;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate   = chainsCfg->channelConf[i].bitRate;
        decPrm.chCreateParams[i].numBufPerCh                          = 16;
    }
    decPrm.inQueParams.prevLinkId    = ipcBitsInVideoId;
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

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink  = swMsId;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = FALSE;

    swMsPrm.numSwMsInst               = 1;
    swMsPrm.swMsInstId[0]             = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm.inQueParams.prevLinkId    = ipcInVpssId;
    swMsPrm.inQueParams.prevLinkQueId = 0;
    swMsPrm.outQueParams.nextLink     = displayId;
    swMsPrm.lineSkipMode              = FALSE;
    swMsPrm.enableLayoutGridDraw      = FALSE;
    swMsPrm.layoutPrm.outputFPS       = chainsCfg->channelConf[0].frameRate;
    swMsPrm.maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
    swMsPrm.maxOutRes                 = chainsCfg->displayRes[0];
    swMsPrm.numOutBuf                 = 0;

    Chains_swMsGenerateLayoutParams(0, 2, &swMsPrm);

    displayPrm.inQueParams[0].prevLinkId    = swMsId;
    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.displayRes                   = chainsCfg->displayRes[0];

    System_linkCreate(ipcBitsOutHostId,&ipcBitsOutHostPrm,sizeof(ipcBitsOutHostPrm));
    System_linkCreate(ipcBitsInVideoId,&ipcBitsInVideoPrm,sizeof(ipcBitsInVideoPrm));
    System_linkCreate(decId, &decPrm, sizeof(decPrm));
    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(swMsId, &swMsPrm, sizeof(swMsPrm));
    System_linkCreate(displayId, &displayPrm, sizeof(displayPrm));

    Chains_memPrintHeapStatus();

    {
        System_linkStart(displayId);
        System_linkStart(swMsId);
        System_linkStart(decId);
        System_linkStart(ipcBitsInVideoId);
        System_linkStart(ipcBitsOutHostId);

        while(1)
        {
            ch = Chains_menuRunTime();
            if(ch=='0')
                break;
        }

        Chains_ipcBitsStop();
        System_linkStop(ipcBitsOutHostId);
        System_linkStop(ipcBitsInVideoId);
        System_linkStop(decId);
        System_linkStop(swMsId);
        System_linkStop(displayId);
    }

    System_linkDelete(ipcBitsOutHostId);
    System_linkDelete(ipcBitsInVideoId);
    System_linkDelete(decId);
    System_linkDelete(ipcOutVideoId);
    System_linkDelete(ipcInVpssId);
    System_linkDelete(swMsId);
    System_linkDelete(displayId);

    Chains_displayCtrlDeInit();
    Chains_ipcBitsExit();
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>


/**
                         SRC (YUV420SP)
                          |
                          |
                         ENC (BitStream)
                          |
                         IPC_BITS_OUT_M3 (BitStream)
                          |
                         IPC_BITS_IN_A8 (BitStream)
                          |
                         fwrite() - write to harddisk
                          |
                         fread() - read from harddisk
                          |
                         IPC_BITS_OUT_A8 (BitStream)
                          |
                         IPC_BITS_IN_M3 (BitStream)
                          |
                         DEC (YUV420SP)
                          |
                          |
                         SNK
*/
#define SRC_NUM_CH 16
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

Void Chains_multiChEncDecLoopBack(Chains_Ctrl *chainsCfg)
{
    NullSrcLink_CreateParams srcPrm;
    EncLink_CreateParams     encPrm;
    DecLink_CreateParams     decPrm;
    NullLink_CreateParams    snkPrm;
    IpcLink_CreateParams     ipcOutVpssPrm;
    IpcLink_CreateParams     ipcInVpssPrm;
    IpcLink_CreateParams     ipcOutVideoPrm;
    IpcLink_CreateParams     ipcInVideoPrm;
    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsOutLinkRTOS_CreateParams   ipcBitsOutVideoPrm;
    IpcBitsInLinkHLOS_CreateParams    ipcBitsInHostPrm;
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    Int i;
    Bool isProgressive;
    System_LinkInfo bitsProducerLinkInfo;

    UInt32 srcId, encId, decId, snkId;
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;
    UInt32 ipcBitsOutVideoId, ipcBitsInHostId;
    UInt32 ipcBitsInVideoId, ipcBitsOutHostId;
    char ch;

    Chains_ipcBitsInit();
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    CHAINS_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams,ipcBitsOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams,ipcBitsInHostPrm);
    CHAINS_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    CHAINS_INIT_STRUCT(DecLink_CreateParams, decPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams, encPrm);

    srcId  = SYSTEM_VPSS_LINK_ID_NULL_SRC_0;
    encId  = SYSTEM_LINK_ID_VENC_0;
    decId  = SYSTEM_LINK_ID_VDEC_0;
    snkId  = SYSTEM_VPSS_LINK_ID_NULL_0;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    ipcBitsOutVideoId = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInHostId   = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;
    ipcBitsOutHostId  = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    ipcBitsInVideoId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;


    srcPrm.tilerEnable = TRUE;
    decPrm.tilerEnable = TRUE;
    isProgressive = TRUE;

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    srcPrm.outQueParams.nextLink = ipcOutVpssId;
    if (isProgressive){
        srcPrm.timerPeriod          = 30;
    }else {
        srcPrm.timerPeriod          = 15;
    }
    srcPrm.inputInfo.numCh          = SRC_NUM_CH;
    for(i=0; i<srcPrm.inputInfo.numCh; i++)
    {
        System_LinkChInfo *pChInfo;

        pChInfo = &srcPrm.inputInfo.chInfo[i];

        pChInfo->dataFormat = SYSTEM_DF_YUV420SP_UV;
        pChInfo->memType    = SYSTEM_MT_TILEDMEM;
        pChInfo->width      = 720;
        if (isProgressive){
            pChInfo->height     = 480;
            pChInfo->scanFormat = SYSTEM_SF_PROGRESSIVE;
        } else{
            pChInfo->height     = 480/2;
            pChInfo->scanFormat = SYSTEM_SF_INTERLACED;
        }
        pChInfo->pitch[0]   = SystemUtils_align(pChInfo->width+pChInfo->startX,
                                                SYSTEM_BUFFER_ALIGNMENT);
        pChInfo->pitch[1]   = pChInfo->pitch[0];
        pChInfo->pitch[2]   = 0;
    }

    ipcOutVpssPrm.inQueParams.prevLinkId    = srcId;
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 1;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink     = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = TRUE;
    ipcOutVpssPrm.notifyPrevLink            = TRUE;
    ipcOutVpssPrm.noNotifyMode              = FALSE;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.outQueParams[0].nextLink     = encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = TRUE;
    ipcInVideoPrm.noNotifyMode              = FALSE;

    for (i=0; i<SRC_NUM_CH; i++) {
        encPrm.chCreateParams[i].format     = IVIDEO_H264HP;
        encPrm.chCreateParams[i].profile    = IH264_HIGH_PROFILE;
        encPrm.chCreateParams[i].dataLayout = IVIDEO_FIELD_SEPARATED;
        if (isProgressive)
            encPrm.chCreateParams[i].fieldMergeEncodeEnable  = FALSE;
        else
            encPrm.chCreateParams[i].fieldMergeEncodeEnable  = TRUE;
        encPrm.chCreateParams[i].maxBitRate = -1;
        encPrm.chCreateParams[i].encodingPreset = 3;
        encPrm.chCreateParams[i].rateControlPreset = 0;
        encPrm.chCreateParams[i].enableHighSpeed = 0;
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
        encPrm.chCreateParams[i].defaultDynamicParams.rcAlg = 0 ;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMin = 10;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMax = 40;
        encPrm.chCreateParams[i].defaultDynamicParams.qpInit = -1;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrDuration = 8;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrSensitivity = 0;
    }
    encPrm.inQueParams.prevLinkId   = ipcInVideoId;
    encPrm.inQueParams.prevLinkQueId= 0;
    encPrm.outQueParams.nextLink = ipcBitsOutVideoId;
    encPrm.numBufPerCh[0] = 6;
    encPrm.numBufPerCh[1] = 6;
    encPrm.numBufPerCh[2] = 6;
    encPrm.numBufPerCh[3] = 6;

    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkId = encId;
    ipcBitsOutVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsOutVideoPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsOutVideoPrm.baseCreateParams.outQueParams[0].nextLink = ipcBitsInHostId;
    Chains_ipcBitsInitCreateParams_BitsOutRTOS(&ipcBitsOutVideoPrm,
                                               TRUE);

    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkId = ipcBitsOutVideoId;
    ipcBitsInHostPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    Chains_ipcBitsInitCreateParams_BitsInHLOS(&ipcBitsInHostPrm);

    System_linkCreate(srcId, &srcPrm, sizeof(srcPrm));
    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );
    System_linkCreate(encId, &encPrm, sizeof(encPrm));
    System_linkCreate(ipcBitsOutVideoId, &ipcBitsOutVideoPrm, sizeof(ipcBitsOutVideoPrm));
    System_linkCreate(ipcBitsInHostId, &ipcBitsInHostPrm, sizeof(ipcBitsInHostPrm));


    System_linkGetInfo(ipcBitsInHostId,&bitsProducerLinkInfo);
    OSA_assert(bitsProducerLinkInfo.numQue = 1);
    ipcBitsOutHostPrm.baseCreateParams.numOutQue                = 1;
    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = ipcBitsInVideoId;
    Chains_ipcBitsInitCreateParams_BitsOutHLOS(&ipcBitsOutHostPrm,
                                               &bitsProducerLinkInfo.queInfo[0]);
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId = ipcBitsOutHostId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue                 = 1;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink = decId;
    Chains_ipcBitsInitCreateParams_BitsInRTOS(&ipcBitsInVideoPrm,
                                              TRUE);
    for (i=0; i<SRC_NUM_CH; i++) {
        decPrm.chCreateParams[i].format          = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile         = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].targetMaxWidth  = 720;
        decPrm.chCreateParams[i].targetMaxHeight = 576;
        if (isProgressive)
            decPrm.chCreateParams[i].fieldMergeDecodeEnable  = FALSE;
        else
            decPrm.chCreateParams[i].fieldMergeDecodeEnable  = TRUE;
        decPrm.chCreateParams[i].numBufPerCh = 6;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = 30;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
                                 (2 * 1000 * 1000);
    }
    decPrm.inQueParams.prevLinkId = ipcBitsInVideoId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcOutVideoId;

    ipcOutVideoPrm.inQueParams.prevLinkId    = decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                   = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink     = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = TRUE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.noNotifyMode              = FALSE;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                    = 1;
    ipcInVpssPrm.outQueParams[0].nextLink     = snkId;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = TRUE;
    ipcInVpssPrm.noNotifyMode              = FALSE;

    snkPrm.numInQue = 1;
    snkPrm.inQueParams[0].prevLinkId = ipcInVpssId;
    snkPrm.inQueParams[0].prevLinkQueId = 0;

    System_linkCreate(ipcBitsOutHostId,&ipcBitsOutHostPrm,sizeof(ipcBitsOutHostPrm));
    System_linkCreate(ipcBitsInVideoId,&ipcBitsInVideoPrm,sizeof(ipcBitsInVideoPrm));
    System_linkCreate(decId, &decPrm, sizeof(decPrm));
    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(snkId, &snkPrm, sizeof(snkPrm));

    Chains_memPrintHeapStatus();
    //Utils_encdecHdvicpPrfInit();

    {
        System_linkStart(snkId);
        System_linkStart(encId);
        System_linkStart(decId);
        System_linkStart(srcId);
        System_linkStart(ipcBitsOutHostId);

        /* Start taking CPU load just before starting of links */
        Chains_prfLoadCalcEnable(TRUE, FALSE, FALSE);

        while(1)
        {
            ch = Chains_menuRunTime();
            if(ch=='0')
                break;
            if(ch=='i')
            {
                System_linkControl(encId,
                       ENC_LINK_CMD_PRINT_IVAHD_STATISTICS, NULL, 0, TRUE);
            }
        }

        System_linkStop(srcId);
        System_linkStop(encId);
        Chains_ipcBitsStop();
        System_linkStop(ipcBitsOutHostId);
        System_linkStop(ipcBitsInVideoId);
        System_linkStop(decId);
        System_linkStop(snkId);
    }

    System_linkDelete(srcId);
    System_linkDelete(ipcOutVpssId );
    System_linkDelete(ipcInVideoId );
    System_linkDelete(encId);
    System_linkDelete(ipcBitsOutVideoId);
    System_linkDelete(ipcBitsInHostId);
    System_linkDelete(ipcBitsOutHostId);
    System_linkDelete(ipcBitsInVideoId);
    System_linkDelete(decId);
    System_linkDelete(ipcOutVideoId);
    System_linkDelete(ipcInVpssId  );
    System_linkDelete(snkId);

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    Chains_prfLoadCalcEnable(FALSE, TRUE, FALSE);
    //Utils_encdecHdvicpPrfPrint();
    Chains_ipcBitsExit();

}


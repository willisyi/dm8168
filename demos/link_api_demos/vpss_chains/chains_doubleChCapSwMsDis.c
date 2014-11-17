/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <demos/link_api_demos/common/chains_ipcBuf.h>
#include <demos/link_api_demos/common/chains_ipcFrames.h>


Void Chains_doubleChCapSwMsDis(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams  capturePrm;
    CaptureLink_VipInstParams *pCaptureInstPrm;
    CaptureLink_OutParams     *pCaptureOutPrm;
    MergeLink_CreateParams    mergePrm0;
    MergeLink_CreateParams    mergePrm1;
    MergeLink_CreateParams    mergePrm2;
    DupLink_CreateParams      dupPrm0;
    DupLink_CreateParams      dupPrm1;
    DupLink_CreateParams      dupPrm2;
    SwMsLink_CreateParams     swMsPrm[CHAINS_SW_MS_MAX_DISPLAYS];
    DisplayLink_CreateParams  displayPrm[CHAINS_SW_MS_MAX_DISPLAYS];

    UInt32 captureId, vipInstId;
    UInt32 mergeId0;
    UInt32 mergeId1;
    UInt32 mergeId2;
    UInt32 dupId0;
    UInt32 dupId1;
    UInt32 dupId2;
    UInt32 swMsId[CHAINS_SW_MS_MAX_DISPLAYS];
    UInt32 displayId[CHAINS_SW_MS_MAX_DISPLAYS];

    Int32 i;
    char ch;
    UInt32 layoutId = 0;

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams,capturePrm);
    for (i=0; i<CHAINS_SW_MS_MAX_DISPLAYS; i++) {
        CHAINS_INIT_STRUCT(SwMsLink_CreateParams,swMsPrm[i]);
        CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm[i]);
    }

    captureId     = SYSTEM_LINK_ID_CAPTURE;
    mergeId0       = SYSTEM_VPSS_LINK_ID_MERGE_0;
    mergeId1       = SYSTEM_VPSS_LINK_ID_MERGE_1;
    mergeId2       = SYSTEM_VPSS_LINK_ID_MERGE_2;
    dupId0         = SYSTEM_VPSS_LINK_ID_DUP_0;
    dupId1         = SYSTEM_VPSS_LINK_ID_DUP_1;
    dupId2         = SYSTEM_VPSS_LINK_ID_DUP_2;
    swMsId[0]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    swMsId[1]     = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
    displayId[0]  = SYSTEM_LINK_ID_DISPLAY_0; // ON CHIP HDMI
    displayId[1]  = SYSTEM_LINK_ID_DISPLAY_1; // OFF CHIP HDMI

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
        pCaptureInstPrm->numOutput          = 2;
        pCaptureInstPrm->numChPerOutput     = 1;
        pCaptureInstPrm->frameCaptureMode   = FALSE;
        pCaptureInstPrm->fieldsMerged       = FALSE;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = gChains_ctrl.channelConf[i].width;
        pCaptureOutPrm->scOutHeight         = gChains_ctrl.channelConf[i].height;
        pCaptureOutPrm->outQueId            = 0;

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[1];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = TRUE;
        pCaptureOutPrm->scOutWidth          = gChains_ctrl.channelConf[2+i].width;
        pCaptureOutPrm->scOutHeight         = gChains_ctrl.channelConf[2+i].height;
        pCaptureOutPrm->outQueId            = 1;
    }
    capturePrm.outQueParams[0].nextLink     = mergeId0;
    capturePrm.outQueParams[1].nextLink     = mergeId0;

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    for (i=0; i<gChains_ctrl.channelNum; i++) {
        chainsCfg->displayRes[i] = capturePrm.vipInst[i].standard;
        Vsys_getResSize(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].width, &chainsCfg->channelConf[i].height);
        Vsys_getResRate(chainsCfg->displayRes[i], &chainsCfg->channelConf[i].frameRate);
    }

    mergePrm0.numInQue                       = 2;
    mergePrm0.inQueParams[0].prevLinkId      = captureId;
    mergePrm0.inQueParams[0].prevLinkQueId   = 0;
    mergePrm0.inQueParams[1].prevLinkId      = captureId;
    mergePrm0.inQueParams[1].prevLinkQueId   = 1;
    mergePrm0.outQueParams.nextLink          = dupId0;
    mergePrm0.notifyNextLink                 = TRUE;

    dupPrm0.inQueParams.prevLinkId           = mergeId0;
    dupPrm0.inQueParams.prevLinkQueId        = 0;
    dupPrm0.numOutQue                        = 2;
    dupPrm0.outQueParams[0].nextLink         = mergeId1;
    dupPrm0.outQueParams[1].nextLink         = mergeId1;
    dupPrm0.notifyNextLink                   = TRUE;

    mergePrm1.numInQue                       = 2;
    mergePrm1.inQueParams[0].prevLinkId      = dupId0;
    mergePrm1.inQueParams[0].prevLinkQueId   = 0;
    mergePrm1.inQueParams[1].prevLinkId      = dupId0;
    mergePrm1.inQueParams[1].prevLinkQueId   = 1;
    mergePrm1.outQueParams.nextLink          = dupId1;
    mergePrm1.notifyNextLink                 = TRUE;

    dupPrm1.inQueParams.prevLinkId           = mergeId1;
    dupPrm1.inQueParams.prevLinkQueId        = 0;
    dupPrm1.numOutQue                        = 2;
    dupPrm1.outQueParams[0].nextLink         = mergeId2;
    dupPrm1.outQueParams[1].nextLink         = mergeId2;
    dupPrm1.notifyNextLink                   = TRUE;

    mergePrm2.numInQue                       = 2;
    mergePrm2.inQueParams[0].prevLinkId      = dupId1;
    mergePrm2.inQueParams[0].prevLinkQueId   = 0;
    mergePrm2.inQueParams[1].prevLinkId      = dupId1;
    mergePrm2.inQueParams[1].prevLinkQueId   = 1;
    mergePrm2.outQueParams.nextLink          = dupId2;
    mergePrm2.notifyNextLink                 = TRUE;

    dupPrm2.inQueParams.prevLinkId           = mergeId2;
    dupPrm2.inQueParams.prevLinkQueId        = 0;
    dupPrm2.numOutQue                        = 2;
    dupPrm2.outQueParams[0].nextLink         = swMsId[0];
    dupPrm2.outQueParams[1].nextLink         = swMsId[1];
    dupPrm2.notifyNextLink                   = TRUE;

    for (i=0; i<2; i++) {
    	swMsPrm[i].numSwMsInst               =1;
        if(0==i)
        {
           swMsPrm[i].swMsInstId[0]             =SYSTEM_SW_MS_SC_INST_SC5;
        }
        else
        {
            swMsPrm[i].swMsInstId[0]             =SYSTEM_SW_MS_SC_INST_VIP0_SC;
        }

        swMsPrm[i].inQueParams.prevLinkId    = dupId2;
        swMsPrm[i].inQueParams.prevLinkQueId = i;
        swMsPrm[i].outQueParams.nextLink     = displayId[i];
        swMsPrm[i].lineSkipMode              = FALSE;
        swMsPrm[i].enableLayoutGridDraw      = FALSE;
        swMsPrm[i].layoutPrm.outputFPS       = chainsCfg->channelConf[i].frameRate;
        swMsPrm[i].maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
        swMsPrm[i].maxOutRes                 = chainsCfg->displayRes[i];
        swMsPrm[i].numOutBuf                 = 0;
        Chains_swMsSetLayoutParams(layoutId+i, &swMsPrm[i]);

        displayPrm[i].displayRes                   = chainsCfg->displayRes[i];
        displayPrm[i].numInputQueues               = 1;
        displayPrm[i].activeQueue                  = 0;
        displayPrm[i].inQueParams[0].prevLinkId    = swMsId[i];
        displayPrm[i].inQueParams[0].prevLinkQueId = 0;
        displayPrm[i].forceFieldSeparatedInputMode = FALSE;
    }

    Chains_displayCtrlInit(chainsCfg->displayRes);
    System_linkCreate(mergeId0, &mergePrm0, sizeof(mergePrm0));
    System_linkCreate(dupId0, &dupPrm0, sizeof(dupPrm0));
    System_linkCreate(mergeId1, &mergePrm1, sizeof(mergePrm1));
    System_linkCreate(dupId1, &dupPrm1, sizeof(dupPrm1));
    System_linkCreate(mergeId2, &mergePrm2, sizeof(mergePrm2));
    System_linkCreate(dupId2, &dupPrm2, sizeof(dupPrm2));
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
        System_linkStart(dupId2);
        System_linkStart(mergeId2);
        System_linkStart(dupId1);
        System_linkStart(mergeId1);
        System_linkStart(dupId0);
        System_linkStart(mergeId0);
        System_linkStart(captureId);

        while(1)
        {
            ch = Chains_menuRunTime();
            if(ch=='0')
                break;
            if(ch=='v')
                System_linkControl(captureId, CAPTURE_LINK_CMD_FORCE_RESET, NULL, 0, TRUE);
            if(ch=='p') {
                System_linkControl(captureId, CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
            }
            if(ch=='m') {
                layoutId += 1;
                for (i=0; i<2; i++) {
                    Chains_swMsSetLayoutParams(layoutId+i, &swMsPrm[i]);
                    System_linkControl(swMsId[i], SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, &swMsPrm[i].layoutPrm, sizeof(swMsPrm[i].layoutPrm), TRUE);
                }
            }
#if 1
            {
            	 Bool switchCh = FALSE;
            	 Bool switchLayout = FALSE;
            	 if(ch=='s')
            	 {
            		 switchLayout = TRUE;
            		 Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 2);
            		printf("ssss\n");

            	 }

            	 if(ch=='c')
            	 {
            		 switchCh = TRUE;
            		 Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 2);
            	 }

            	 if(ch == '1')
            	 {
            		 switchCh = TRUE;
            		 Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 0);
            		 continue;
            	 }
            	 if(ch == '2')
            	 {
            		 switchCh = TRUE;
            		 Chains_swMsSwitchLayout(&swMsId, &swMsPrm, switchLayout, switchCh, 1);
            		 continue;
            	 }

            }
#endif
        }

        System_linkStop(captureId);
        System_linkStop(mergeId0);
        System_linkStop(dupId0);
        System_linkStop(mergeId1);
        System_linkStop(dupId1);
        System_linkStop(mergeId2);
        System_linkStop(dupId2);
        for (i=0; i<2; i++) {
            System_linkStop(swMsId[i]);
            System_linkStop(displayId[i]);
        }
    }

    System_linkDelete(captureId);
    System_linkDelete(mergeId0);
    System_linkDelete(dupId0);
    System_linkDelete(mergeId1);
    System_linkDelete(dupId1);
    System_linkDelete(mergeId2);
    System_linkDelete(dupId2);
    for (i=0; i<2; i++) {
        System_linkDelete(swMsId[i]);
        System_linkDelete(displayId[i]);
    }

    Chains_displayCtrlDeInit();
}

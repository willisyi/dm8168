
/**
  \file demo_spl_usecases.c
  \brief
  */
#include <sys/time.h>
#include <demo_spl_usecases.h>
#include <demo_vcap_venc_vdec_vdis_bits_rdwr.h>
#include <demo_scd_bits_wr.h>

/* used to set resolution / buf size of ipcbits for varying resolutions based on usecase */
static VcapVencVdecVdis_res    ipcbits_res[MCFW_IPCBITS_RESOLUTION_TYPES];

static Int64 get_current_time_to_msec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((Int64)tv.tv_sec*1000 + tv.tv_usec/1000);
}

static
Void VcapVencVdecVdisSplUsecase_setDemoInfo(int demoId)
{
    switch (demoId)
    {
        case    DEMO_HYBRIDDVR_16CH:
            ipcbits_res[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_res[0].height = MCFW_IPCBITS_D1_HEIGHT;
            ipcbits_res[1].width = MCFW_IPCBITS_HD_WIDTH;
            ipcbits_res[1].height = MCFW_IPCBITS_HD_HEIGHT;
            gDemo_info.maxVcapChannels = 16;
            gDemo_info.maxVencChannels = (16+16);
            gDemo_info.maxVdecChannels = 16+4;
            gDemo_info.maxVdisChannels =
                (gDemo_info.maxVcapChannels + gDemo_info.maxVdecChannels);
            gDemo_info.VsysNumChs  = 16;
            gDemo_info.usecase = VSYS_USECASE_MULTICHN_HYBRID_DVR_16CH;
            gDemo_info.Type    = DEMO_TYPE_PROGRESSIVE;
            gDemo_info.osdEnable   = TRUE;
            gDemo_info.scdTileConfigInitFlag = FALSE;
            gDemo_info.numEncChannels[0] = 16;
            gDemo_info.numEncChannels[1] = 16;
            gDemo_info.numEncChannels[2] = 0;
            gDemo_info.bitRateKbps[0]    = 2000;
            gDemo_info.bitRateKbps[1]    = 500;
            gDemo_info.numDeis = 2;
            gDemo_info.numSwMs = 2;
            gDemo_info.numDisplays = 2;
            gDemo_info.defaultLayoutId[VDIS_DEV_HDMI] = DEMO_LAYOUT_MODE_16CH;
            gDemo_info.defaultLayoutId[VDIS_DEV_SD] = DEMO_LAYOUT_MODE_7CH_1CH;
            gDemo_info.scdEnable = TRUE;
            break;

        case    DEMO_CARDVR_4CH:
            ipcbits_res[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_res[0].height = MCFW_IPCBITS_D1_HEIGHT;
            ipcbits_res[1].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_res[1].height = MCFW_IPCBITS_D1_HEIGHT;
            gDemo_info.maxVcapChannels = 8;
            gDemo_info.maxVdisChannels = 8;
            gDemo_info.maxVencChannels = 5;
            gDemo_info.maxVdecChannels = 1;
            gDemo_info.VsysNumChs  = 8;
            gDemo_info.usecase = VSYS_USECASE_MULTICHN_CAR_DVR_4CH;
            gDemo_info.Type    = DEMO_TYPE_PROGRESSIVE;
            gDemo_info.osdEnable   = TRUE;
            gDemo_info.scdTileConfigInitFlag = FALSE;
            gDemo_info.numEncChannels[0] = 1;
            gDemo_info.numEncChannels[1] = gDemo_info.maxVencChannels - gDemo_info.numEncChannels[0];
            gDemo_info.numEncChannels[2] = 0;
            gDemo_info.bitRateKbps[0]    = 2000;
            gDemo_info.bitRateKbps[1]    = 2000;
            gDemo_info.numDeis = 2;
            gDemo_info.numSwMs = 1;
            gDemo_info.numDisplays = 1;
            gDemo_info.defaultLayoutId[VDIS_DEV_HDMI] = DEMO_LAYOUT_MODE_16CH;
            gDemo_info.defaultLayoutId[VDIS_DEV_SD] = DEMO_LAYOUT_MODE_4CH;
            gDemo_info.scdEnable = FALSE;
            break;

        case    DEMO_HYBRIDENC_36CH:
            ipcbits_res[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_res[0].height = MCFW_IPCBITS_D1_HEIGHT;
            ipcbits_res[1].width = MCFW_IPCBITS_HD_WIDTH;
            ipcbits_res[1].height = MCFW_IPCBITS_HD_HEIGHT;
            gDemo_info.maxVcapChannels = 9;
            gDemo_info.maxVencChannels = (9+9+9+9);
            gDemo_info.maxVdecChannels = 1;
            gDemo_info.maxVdisChannels =
                (gDemo_info.maxVcapChannels + gDemo_info.maxVdecChannels);
            gDemo_info.VsysNumChs  = 9;
            gDemo_info.usecase = VSYS_USECASE_MULTICHN_HYBRID_ENC_36CH;
            gDemo_info.Type    = DEMO_TYPE_PROGRESSIVE;
            gDemo_info.osdEnable   = FALSE;
            gDemo_info.scdTileConfigInitFlag = FALSE;
            gDemo_info.numEncChannels[0] = 1;
            gDemo_info.numEncChannels[1] = 17;
            gDemo_info.numEncChannels[2] = 18;
            gDemo_info.bitRateKbps[0]    = 2000;
            gDemo_info.bitRateKbps[1]    = 1000;
            gDemo_info.numDeis = 2;
            gDemo_info.numSwMs = 1;
            gDemo_info.numDisplays = 1;
            gDemo_info.defaultLayoutId[VDIS_DEV_HDMI] = DEMO_LAYOUT_MODE_16CH;
            gDemo_info.scdEnable = TRUE;
            break;

        default:
            break;
    }
}

static
Void VcapVencVdecVdisSplUsecase_setVcapParams(Demo_Info *demoInfo,
                                              VCAP_PARAMS_S *vcapParams)
{
    Vcap_params_init(vcapParams);
    vcapParams->numChn = demoInfo->maxVcapChannels;
}

static
Void VcapVencVdecVdisSplUsecase_setVencParams(Demo_Info *demoInfo,
                                              VENC_PARAMS_S *vencParams)
{
    Venc_params_init(vencParams);
    vencParams->numPrimaryChn = demoInfo->numEncChannels[0];
    vencParams->numSecondaryChn = demoInfo->numEncChannels[1];
    /*Enabling generation of motion vector for channel 0 only,
         * for other channels please add to the below line*/
    vencParams->encChannelParams[0].enableAnalyticinfo = 1;
    //vencParams.encChannelParams[1].enableAnalyticinfo = 1;
    vencParams->encChannelParams[0].maxBitRate = -1;
}

static
Void VcapVencVdecVdisSplUsecase_setVsysParams(Demo_Info *demoInfo,
                                              VSYS_PARAMS_S *vsysParams)
{
    Vsys_params_init(vsysParams);
    vsysParams->numChs  = demoInfo->VsysNumChs;
    vsysParams->systemUseCase = demoInfo->usecase;
    vsysParams->enableOsd     = demoInfo->osdEnable;
    vsysParams->numDeis       = demoInfo->numDeis;
    vsysParams->numSwMs       = demoInfo->numSwMs;
    vsysParams->numDisplays   = demoInfo->numDisplays;
    if (demoInfo->numEncChannels[1])
    {
        vsysParams->enableSecondaryOut = TRUE;
    }
    else
    {
        vsysParams->enableSecondaryOut = FALSE;
    }
    vsysParams->enableCapture = TRUE;
    vsysParams->enableScd     = demoInfo->scdEnable;
    vsysParams->enableNsf     = FALSE;
    if (demoInfo->numEncChannels[2])
    {
        vsysParams->enableMjpegEnc = TRUE;
    }
    else
    {
        vsysParams->enableMjpegEnc = FALSE;
    }
    vsysParams->enableNullSrc = FALSE;
    vsysParams->enableAVsync  = TRUE;
}

static
Void VcapVencVdecVdisSplUsecase_setVdecParams(Demo_Info *demoInfo,
                                              VDEC_PARAMS_S *vdecParams)
{
    Vdec_params_init(vdecParams);

    vdecParams->numChn = demoInfo->maxVdecChannels;
}

static
Void VcapVencVdecVdisSplUsecase_setVdisParams(Demo_Info *demoInfo,
                                              VDIS_PARAMS_S *vdisParams)
{
    Bool forceLowCostScale = FALSE;

    Vdis_params_init(vdisParams);

    vdisParams->numChannels = demoInfo->maxVdisChannels;
    /* Override the context here as needed */
    vdisParams->deviceParams[VDIS_DEV_HDMI].resolution   = DEMO_HD_DISPLAY_DEFAULT_STD;
    vdisParams->deviceParams[VDIS_DEV_HDCOMP].resolution = DEMO_HD_DISPLAY_DEFAULT_STD;
    Vdis_tiedVencInit(VDIS_DEV_HDCOMP, VDIS_DEV_DVO2, vdisParams);
    vdisParams->deviceParams[VDIS_DEV_SD].resolution     = VSYS_STD_NTSC;
    vdisParams->enableLayoutGridDraw = FALSE;

    forceLowCostScale = TRUE;

    /* set for 3 displays */

    Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, demoInfo->maxVdisChannels,
                          demoInfo->defaultLayoutId[VDIS_DEV_HDMI],
                          &vdisParams->mosaicParams[VDIS_DEV_HDMI], forceLowCostScale,
                          demoInfo->Type,
                          vdisParams->deviceParams[VDIS_DEV_HDMI].resolution);
    vdisParams->mosaicParams[VDIS_DEV_HDMI].userSetDefaultSWMLayout = TRUE;


    Demo_swMsGenerateLayout(VDIS_DEV_SD, 0, demoInfo->maxVdisChannels,
                          demoInfo->defaultLayoutId[VDIS_DEV_SD],
                          &vdisParams->mosaicParams[VDIS_DEV_SD], FALSE,
                          demoInfo->Type,
                          vdisParams->deviceParams[VDIS_DEV_SD].resolution);
    vdisParams->mosaicParams[VDIS_DEV_SD].userSetDefaultSWMLayout = TRUE;

}

static
Void VcapVencVdecVdisSplUsecase_setOsdConfig(VCAP_PARAMS_S *vcapParams)
{
    UInt8 osdFormat[ALG_LINK_OSD_MAX_CH];
    UInt32 chId;

    /* Most of progressive use cases have OSD in YUV420 SP format */
    memset(osdFormat, SYSTEM_DF_YUV420SP_UV, ALG_LINK_OSD_MAX_CH);

    /* Create and initialize OSD window buffers */
    Demo_osdInit(vcapParams->numChn, osdFormat);

    for(chId = 0; chId < vcapParams->numChn; chId++)
    {
        vcapParams->channelParams[chId].dynamicParams.osdChWinPrm = &g_osdChParam[chId];
        /* Initialize osdLink with created and set win params */
        Vcap_setDynamicParamChn(chId, &vcapParams->channelParams[chId].dynamicParams, VCAP_OSDWINPRM);
    }
}

static
Void VcapVencVdecVdisSplUsecase_setVencDynamicParams(Demo_Info *demoInfo,
                                                     VENC_PARAMS_S *vencParams)
{
    UInt32 chId;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };
    VSYS_VIDEO_STANDARD_E vidStandard;

    vidStandard = Demo_captureGetSignalStandard();
    /* Setting FPS for the Encoder Channels */
    for(chId=0; chId<vencParams->numPrimaryChn; chId++)
    {
        if(vidStandard == VSYS_STD_PAL)
        {
            memset(&params, 0, sizeof(params));

            Venc_setInputFrameRate(chId, 25);

            params.frameRate = 25;
            params.targetBitRate = demoInfo->bitRateKbps[0] * 1000;
            Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);
        }
        else if(vidStandard == VSYS_STD_NTSC)
        {
            memset(&params, 0, sizeof(params));
            Venc_setInputFrameRate(chId, 30);

            params.frameRate = 30;
            params.targetBitRate = demoInfo->bitRateKbps[0] * 1000;
            Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

        }
        else
        {
           printf (" DEMO: No video detected at CH [%d] !!!\n",
                     chId);
        }
    }
    for(chId = vencParams->numPrimaryChn;
        chId < (vencParams->numPrimaryChn + vencParams->numSecondaryChn);
        chId++)
    {
        if(vidStandard == VSYS_STD_PAL)
        {
            memset(&params, 0, sizeof(params));

            Venc_setInputFrameRate(chId, 25);

            params.frameRate = 25;
            params.targetBitRate = demoInfo->bitRateKbps[1] * 1000;
            Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);
        }
        else if(vidStandard == VSYS_STD_NTSC)
        {
            memset(&params, 0, sizeof(params));

            Venc_setInputFrameRate(chId, 30);
            params.frameRate = 30;

            params.targetBitRate = demoInfo->bitRateKbps[1] * 1000;

            Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);
        }
    }
}


Void VcapVencVdecVdisSplUsecase_start(int demoId)
{
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VENC_PARAMS_S vencParams;
    VDEC_PARAMS_S vdecParams;
    VDIS_PARAMS_S vdisParams;
    Bool enableFWrite = FALSE;
    Bool enableLayerWrite = FALSE;
    UInt64 wallTimeBase;




    VcapVencVdecVdisSplUsecase_setDemoInfo(demoId);
    VcapVencVdecVdisSplUsecase_setVcapParams(&gDemo_info,&vcapParams);
    VcapVencVdecVdisSplUsecase_setVencParams(&gDemo_info,&vencParams);
    VcapVencVdecVdisSplUsecase_setVsysParams(&gDemo_info,&vsysParams);
    VcapVencVdecVdisSplUsecase_setVdecParams(&gDemo_info,&vdecParams);
    VcapVencVdecVdisSplUsecase_setVdisParams(&gDemo_info,&vdisParams);



    printf ("--------------- CHANNEL DETAILS-------------\n");
    printf ("Capture Channels => %d\n", vcapParams.numChn);
    printf ("Enc Channels => Primary %d, Secondary %d\n", vencParams.numPrimaryChn, vencParams.numSecondaryChn);
    printf ("Dec Channels => %d\n", vdecParams.numChn);
    printf ("Disp Channels => %d\n", vdisParams.numChannels);
    printf ("-------------------------------------------\n");

    /* Override the context here as needed */
    Vsys_init(&vsysParams);

    /* Override the context here as needed */
    Vcap_init(&vcapParams);

    /* Override the context here as needed */
    Venc_init(&vencParams);


    /* Override the context here as needed */
    Vdec_init(&vdecParams);


    Vdis_init(&vdisParams);

    if(vsysParams.enableScd == TRUE)
    {
        Scd_bitsWriteCreate(demoId);
    }

    enableFWrite = Demo_getFileWriteEnable();

    /* Init the application specific module which will handle bitstream exchange */
    VcapVencVdecVdis_ipcBitsInit(ipcbits_res, enableFWrite,enableLayerWrite);

    wallTimeBase = get_current_time_to_msec();
    wallTimeBase = 0;
    Vdis_setWallTimeBase(wallTimeBase);

    Vsys_configureDisplay();

#if USE_FBDEV
    grpx_fb_init(GRPX_FORMAT_RGB565);
#endif

    /* Create Link instances and connects compoent blocks */
    Vsys_create();

    if (vsysParams.enableOsd)
    {
        VcapVencVdecVdisSplUsecase_setOsdConfig(&vcapParams);
    }


#ifndef SYSTEM_DISABLE_AUDIO
        Demo_audioEnable(TRUE);
#endif


    /* Start components in reverse order */
    Vdis_start();
    Vdec_start();
    Venc_start();
    Vcap_start();

#if USE_FBDEV
     //grpx_fb_draw(VDIS_DEV_HDMI);
#ifdef TI_816X_BUILD
     //grpx_fb_draw(VDIS_DEV_DVO2);
#endif
#endif

    display_process_init();
    VcapVencVdecVdisSplUsecase_setVencDynamicParams(&gDemo_info,&vencParams);

}

Void VcapVencVdecVdisSplUsecase_stop()
{
    VSYS_PARAMS_S contextInf;
    Vsys_getContext(&contextInf);
    display_process_deinit();

//    if(contextInf.enableScd)
//        Scd_bitsWriteStop();

    VcapVencVdecVdis_ipcBitsStop();
    /* Stop components */
    Vcap_stop();
    Venc_stop();
    Vdec_stop();
    Vdis_stop();

#if USE_FBDEV
     grpx_fb_exit();
#endif

	 /* Free the osd buffers */
	Demo_osdDeinit();	

    Vsys_delete();

	Vsys_deConfigureDisplay();

    if(contextInf.enableScd)
        Scd_bitsWriteDelete();


    VcapVencVdecVdis_ipcBitsExit();

    /* De-initialize components */
    Vcap_exit();
    Venc_exit();
    Vdec_exit();
    Vdis_exit();

    /* Stop audio capture <which might use avsync APIs> before system deInit */
#ifndef SYSTEM_DISABLE_AUDIO
    Demo_audioEnable(FALSE);
#endif

    Vsys_exit();

}



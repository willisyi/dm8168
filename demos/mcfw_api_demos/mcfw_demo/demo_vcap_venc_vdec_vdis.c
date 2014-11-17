
/**
  \file demo_vcap_venc_vdec_vdis.c
  \brief
  */
#include <sys/time.h>
#include <demo_vcap_venc_vdec_vdis.h>
#include <demo_scd_bits_wr.h>

/* Setting secondary out <CIF> for 30 frames - this is the validated frame rate;
any higher number will impact performance. */

#ifdef TI_816X_BUILD
#define     CIF_FPS_ENC_NTSC         (30)
#define     CIF_FPS_ENC_PAL          (25)
#endif
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#define     CIF_FPS_ENC_NTSC         (30)
#define     CIF_FPS_ENC_PAL          (25)
#endif
#define     CIF_BITRATE         (500)
#define     MJPEG_BITRATE       (100)

#ifdef TI_8107_BUILD
static int Demo_change8ChSwitchUsecase(int usecase);
#endif



static Int64 get_current_time_to_msec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((Int64)tv.tv_sec*1000 + tv.tv_usec/1000);
}

/* used in 814x 16ch usecase */
#define H264_PRIMARY_STREAM_PROFILE     VENC_CHN_MAIN_PROFILE
#define H264_SECONDARY_STREAM_PROFILE   VENC_CHN_BASELINE_PROFILE

/* used to set resolution / buf size of ipcbits for varying resolutions based on usecase */
VcapVencVdecVdis_res    ipcbits_resArray[MCFW_IPCBITS_RESOLUTION_TYPES];


Void VcapVencVdecVdis_setFileWriteMask(Int32 systemUseCase)
{
    if(systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
    {
        if (((MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_DEFAULT) & (MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL_1_CHANNEL16))
                != (MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL_1_CHANNEL16))
        {
            VcapVencVdecVdis_ipcBitsSetFileWriteMask(MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL_1_CHANNEL16);
        }
    }
    else if (systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
    {
        if (((MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_DEFAULT) & (MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL16))
                != (MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL16))
        {
            VcapVencVdecVdis_ipcBitsSetFileWriteMask(MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL16);
        }
    }
}

Void VcapVencVdecVdis_setEncParans(VENC_PARAMS_S *pVencParams, VSYS_PARAMS_S *pVsysParams)
{
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    Int32 i;

    if(pVsysParams->systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF
        || pVsysParams->systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
    {
        for (i=0; i < VENC_PRIMARY_CHANNELS; i++)
        {
            pVencParams->h264Profile[i] = H264_PRIMARY_STREAM_PROFILE;
        }
        for (i=VENC_PRIMARY_CHANNELS; i < (2*VENC_PRIMARY_CHANNELS); i++)
        {
            pVencParams->h264Profile[i] = H264_SECONDARY_STREAM_PROFILE;
        }
        /* QCIF channels */
        for (i=VENC_PRIMARY_CHANNELS; i < (2*VENC_PRIMARY_CHANNELS); i++)
        {
            pVencParams->encChannelParams[i].dynamicParam.targetBitRate = .2 * 1000 * 1000;
        }
    }
    if(pVsysParams->systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
    {
        pVencParams->encChannelParams[0].dynamicParam.targetBitRate = 2 * 1000 * 1000;
        /* CIF channels */
        for (i=1; i < VENC_PRIMARY_CHANNELS; i++)
        {
            pVencParams->encChannelParams[i].dynamicParam.targetBitRate = .5 * 1000 * 1000;
        }
    }
#ifdef TI_8107_BUILD
    if (pVsysParams->systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
    {
        for (i = 0; i < 2*VENC_PRIMARY_CHANNELS; i++)
        {
            /*This is an optimization option used for the 810X 16CH CIF DVR usecase*/
            pVencParams->encChannelParams[i].numTemporalLayer = VENC_TEMPORAL_LAYERS_2;
        }
    }
    if (pVsysParams->systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT)
    {
        for (i = 0; i < VENC_PRIMARY_CHANNELS / 2; i++)
        {
            pVencParams->h264Profile[i] = H264_PRIMARY_STREAM_PROFILE;
        }
        /* QCIF channels */
        for (i = VENC_PRIMARY_CHANNELS / 2; i < VENC_PRIMARY_CHANNELS; i++)
        {
            pVencParams->h264Profile[i] = H264_SECONDARY_STREAM_PROFILE;
            pVencParams->encChannelParams[i].dynamicParam.targetBitRate = .2 * 1000 * 1000;
        }
    }
#endif
#endif
}

Void VcapVencVdecVdis_setChannels(int demoId, int *pEnable2ndOut)
{
    ipcbits_resArray[1].width = 0;
    ipcbits_resArray[1].height = 0;

    switch (demoId)
    {
        case    DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE:
            ipcbits_resArray[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_resArray[0].height = MCFW_IPCBITS_D1_HEIGHT;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
            gDemo_info.maxVcapChannels = 4;
            gDemo_info.maxVdisChannels = 8;
            gDemo_info.maxVencChannels = 4;
            gDemo_info.maxVdecChannels = 4;
            gDemo_info.VsysNumChs  = 4;
#else
            gDemo_info.maxVcapChannels = 16;
            gDemo_info.maxVdisChannels = 32;
            gDemo_info.maxVencChannels = 16;
            gDemo_info.maxVdecChannels = 16;
            gDemo_info.VsysNumChs  = 16;
#endif
            break;

        case    DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1:
        case    DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT:
            ipcbits_resArray[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_resArray[0].height = MCFW_IPCBITS_D1_HEIGHT;
            ipcbits_resArray[1].width = MCFW_IPCBITS_CIF_WIDTH;
            ipcbits_resArray[1].height = MCFW_IPCBITS_CIF_HEIGHT;
            gDemo_info.maxVcapChannels = 16;
            gDemo_info.maxVdisChannels = 32;
            gDemo_info.maxVencChannels = 16;
            gDemo_info.maxVdecChannels = 16;
            gDemo_info.VsysNumChs  = 16;

            break;

        case    DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED:
            ipcbits_resArray[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_resArray[0].height = MCFW_IPCBITS_D1_HEIGHT;
#ifdef TI_816X_BUILD
            gDemo_info.maxVcapChannels = 16;
            gDemo_info.maxVdisChannels = 32;
            gDemo_info.maxVencChannels = 16;
            gDemo_info.maxVdecChannels = 16;
            gDemo_info.VsysNumChs  = 16;
#endif
            break;
        case    DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_4CH:
            ipcbits_resArray[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_resArray[0].height = MCFW_IPCBITS_D1_HEIGHT;
            gDemo_info.maxVcapChannels = 4;
            gDemo_info.maxVdisChannels = 8;
            gDemo_info.maxVencChannels = 4;
            gDemo_info.maxVdecChannels = 4;
            gDemo_info.VsysNumChs  = 4;
            break;

        case    DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH:
        case    DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT:
            ipcbits_resArray[0].width = MCFW_IPCBITS_D1_WIDTH;
            ipcbits_resArray[0].height = MCFW_IPCBITS_D1_HEIGHT;
            gDemo_info.maxVcapChannels = 8;
            gDemo_info.maxVdisChannels = 16;
            gDemo_info.maxVencChannels = 8;
            gDemo_info.maxVdecChannels = 8;
            gDemo_info.VsysNumChs  = 8;
            break;

        default:
            break;
    }
}


Void Demo_disableMJPEG ()
{
    int i;

    for (i = 32; i < 48; i++)
    {
        Venc_disableChn (i);
    }
}

Void VcapVencVdecVdis_start( Bool doProgressiveVenc, Bool enableSecondaryOut, int demoId)
{
    UInt32 i;
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VENC_PARAMS_S vencParams;
    VDEC_PARAMS_S vdecParams;
    VDIS_PARAMS_S vdisParams;
    Bool forceLowCostScale = FALSE, enableFWrite = FALSE, enableLayerWrite = FALSE;
    Int32 Enable2ndOut = enableSecondaryOut;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };
    UInt32 chId;
    UInt8 osdFormat[ALG_LINK_OSD_MAX_CH];
    UInt64 wallTimeBase;

    Vsys_params_init(&vsysParams);
    Vcap_params_init(&vcapParams);
    Venc_params_init(&vencParams);
    Vdec_params_init(&vdecParams);
    Vdis_params_init(&vdisParams);

    VcapVencVdecVdis_setChannels(demoId, &Enable2ndOut);


    vcapParams.numChn = gDemo_info.maxVcapChannels;
    vencParams.numPrimaryChn = gDemo_info.maxVencChannels;
    vencParams.numSecondaryChn = 0;
    vdecParams.numChn = gDemo_info.maxVdecChannels;
    vdisParams.numChannels = gDemo_info.maxVdisChannels;
    vsysParams.numChs  = gDemo_info.VsysNumChs;

    enableSecondaryOut = (Bool)Enable2ndOut;

    /* Most of progressive use cases have OSD in YUV420 SP format */
    memset(osdFormat, SYSTEM_DF_YUV420SP_UV, ALG_LINK_OSD_MAX_CH);

    if(osdFormat[0] == SYSTEM_DF_YUV420SP_UV)
    {
        vsysParams.osdFormat = FALSE;
    }
    else
    {
        vsysParams.osdFormat = TRUE;
    }

    if( doProgressiveVenc)
    {
        switch (demoId)
        {
            case  DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE:
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC;
#if !defined(TI_814X_BUILD) && !defined(TI_8107_BUILD)
                if(vsysParams.osdFormat == TRUE)
                   memset(osdFormat, SYSTEM_DF_YUV422I_YUYV, ALG_LINK_OSD_MAX_CH);
#endif
                break;
            case  DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1:
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF;
                break;
            case  DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_4CH:
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH;
                break;
            case  DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH:
#if defined(TI_814X_BUILD)
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH;
#endif
#if defined(TI_8107_BUILD)
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH;
#endif
                break;
            case  DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT:
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT;
                break;
#if defined(TI_8107_BUILD)
            case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT:
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT;
                break;
#endif
            default:
                vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF;
        }

        if (enableSecondaryOut == FALSE)
        {
            vsysParams.enableSecondaryOut = FALSE;
            vsysParams.enableNsf     = FALSE;
        }
        else
        {
            vsysParams.enableSecondaryOut = TRUE;
            vsysParams.enableNsf     = TRUE;
            vsysParams.enableScd     = FALSE;
            /*enableMjpegEnc should always be true in all usecases*/
            vsysParams.enableMjpegEnc = TRUE;

            if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_4CH) ||
               (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE))
#if defined(TI_8107_BUILD)
               vsysParams.enableScd     = FALSE;
#else
               vsysParams.enableScd     = TRUE;
#endif

        }

        vsysParams.enableCapture = TRUE;
        vsysParams.enableNullSrc = FALSE;

        vsysParams.enableOsd     = TRUE;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#ifdef TI_8107_BUILD
        vsysParams.enableScd     = FALSE;
#else
        if(vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF
            || vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
            vsysParams.enableScd     = FALSE;
        else
            vsysParams.enableScd     = TRUE;
        if(vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF
            || vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH)
            vdecParams.forceUseDecChannelParams = TRUE;
#endif
        vsysParams.numDeis       = 1;
        vsysParams.numSwMs       = 2;
        vsysParams.numDisplays   = 2;
        vsysParams.enableAVsync  = TRUE;
#else
        vsysParams.numDeis       = 2;
        vsysParams.numSwMs       = 2;
        vsysParams.numDisplays   = 3;
        vsysParams.enableAVsync  = TRUE;
#endif
    }
    else
    {
        vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_INTERLACED_VCAP_VDIS_VENC_VDEC;
        vsysParams.enableCapture = TRUE;
        vsysParams.enableNsf     = TRUE;
        vsysParams.enableNullSrc = FALSE;
        vsysParams.numDeis       = 0;
#ifdef TI_8107_BUILD
        vsysParams.enableOsd     = FALSE;
#else
        vsysParams.enableOsd     = TRUE;
#endif

        vsysParams.enableSecondaryOut = enableSecondaryOut;
        vdecParams.forceUseDecChannelParams = TRUE;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        vsysParams.numSwMs       = 2;
        vsysParams.numDisplays   = 2;
#else
        vsysParams.numSwMs       = 2;
        vsysParams.numDisplays   = 2;
#endif
    }
    if (enableSecondaryOut)
    {
        vencParams.numSecondaryChn = gDemo_info.maxVencChannels;
        /*enableMjpegEnc should always be true in all usecases*/
        gDemo_info.maxVencChannels *= 3;
    }

    printf ("--------------- CHANNEL DETAILS-------------\n");
    printf ("Capture Channels => %d\n", vcapParams.numChn);
    printf ("Enc Channels => Primary %d, Secondary %d\n", vencParams.numPrimaryChn, vencParams.numSecondaryChn);
    printf ("Dec Channels => %d\n", vdecParams.numChn);
    printf ("Disp Channels => %d\n", vdisParams.numChannels);
    printf ("-------------------------------------------\n");

    Vsys_enableFastUsecaseSwitch(FALSE);

    /* Override the context here as needed */
    Vsys_init(&vsysParams);

    /* Override the context here as needed */
    Vcap_init(&vcapParams);

    /*Enabling generation of motion vector for channel 0 only,
         * for other channels please add to the below line*/

    vencParams.encChannelParams[0].enableAnalyticinfo = 1;
    //vencParams.encChannelParams[1].enableAnalyticinfo = 1;
    vencParams.encChannelParams[0].maxBitRate = -1;

    /*Note:Specific for h264 Encoder: Enabling this flag adds svc enxtension
          header to the stream, not all decoders are generally able to play back such a stream. */
    /* Needs to be enabled to IH264_SVC_EXTENSION_FLAG_ENABLE for the
          svc extension headers to be present in the stream*/
    /*!!! Note: This flag needs to be enabled for the temporalId to be parsed
         out from the stream.*/
    vencParams.encChannelParams[0].numTemporalLayer = VENC_TEMPORAL_LAYERS_1;
    vencParams.encChannelParams[0].enableSVCExtensionFlag =
                                       VENC_IH264_SVC_EXTENSION_FLAG_DISABLE;

    VcapVencVdecVdis_setEncParans(&vencParams,&vsysParams);

    /* Override the context here as needed */
    Venc_init(&vencParams);


    /* Override the context here as needed */
    Vdec_init(&vdecParams);

    /* Override the context here as needed */
    vdisParams.deviceParams[VDIS_DEV_HDMI].resolution   = DEMO_HD_DISPLAY_DEFAULT_STD;
    vdisParams.deviceParams[VDIS_DEV_HDCOMP].resolution = DEMO_HD_DISPLAY_DEFAULT_STD;
    Vdis_tiedVencInit(VDIS_DEV_HDCOMP, VDIS_DEV_DVO2, &vdisParams);

    vdisParams.deviceParams[VDIS_DEV_SD].resolution     = VSYS_STD_NTSC;

    vdisParams.enableLayoutGridDraw = FALSE;

    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_INTERLACED_VCAP_VDIS_VENC_VDEC)
        forceLowCostScale = TRUE;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    i = 0;
    /* set for 2 displays */
    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
    {
        Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, gDemo_info.maxVdisChannels,
                          DEMO_LAYOUT_MODE_16CH,
                          &vdisParams.mosaicParams[VDIS_DEV_HDMI], forceLowCostScale,
                          gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
    }
    else
    {
        Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, gDemo_info.maxVdisChannels,
                          DEMO_LAYOUT_MODE_16CH,
                          &vdisParams.mosaicParams[VDIS_DEV_HDMI], forceLowCostScale,
                          gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
    }
    vdisParams.mosaicParams[VDIS_DEV_HDMI].userSetDefaultSWMLayout = TRUE;
    /* Set swMS outputFPS as 60 or 50 for this default 7x1 layout for TI814X */
#if defined(TI_814X_BUILD)
    Demo_swMsSetOutputFPS(&vdisParams.mosaicParams[VDIS_DEV_HDMI], Demo_swMsGetOutputFPS(&vdisParams.mosaicParams[VDIS_DEV_HDMI])*2);
#else   // TI_8107_BUILD
    Demo_swMsSetOutputFPS(
            &vdisParams.mosaicParams[VDIS_DEV_HDMI],
            Demo_swMsGetOutputFPS(&vdisParams.mosaicParams[VDIS_DEV_HDMI]));
#endif
    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
    {
        Demo_swMsGenerateLayout(VDIS_DEV_SD, 0, gDemo_info.maxVdisChannels,
                          DEMO_LAYOUT_MODE_16CH,
                          &vdisParams.mosaicParams[VDIS_DEV_SD], forceLowCostScale,
                          gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_SD].resolution);
    }
    else
    {
        Demo_swMsGenerateLayout(VDIS_DEV_SD, 0, gDemo_info.maxVdisChannels,
                          DEMO_LAYOUT_MODE_16CH,
                          &vdisParams.mosaicParams[VDIS_DEV_SD], forceLowCostScale,
                          gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_SD].resolution);
    }
    vdisParams.mosaicParams[VDIS_DEV_SD].userSetDefaultSWMLayout = TRUE;
#if defined(TI_814X_BUILD)
    Demo_swMsSetOutputFPS(&vdisParams.mosaicParams[VDIS_DEV_SD], Demo_swMsGetOutputFPS(&vdisParams.mosaicParams[VDIS_DEV_SD])*2);
#else
    Demo_swMsSetOutputFPS(
            &vdisParams.mosaicParams[VDIS_DEV_SD],
            Demo_swMsGetOutputFPS(&vdisParams.mosaicParams[VDIS_DEV_SD]));
#endif
#else
    /* set for 3 displays */

    i = 0;
    Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, gDemo_info.maxVdisChannels,
                          DEMO_LAYOUT_MODE_16CH,
                          &vdisParams.mosaicParams[i], forceLowCostScale,
                          gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
    vdisParams.mosaicParams[i].userSetDefaultSWMLayout = TRUE;


    if (vsysParams.systemUseCase != VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
    {
        i = 1;
        Demo_swMsGenerateLayout(VDIS_DEV_HDCOMP, 16, gDemo_info.maxVdisChannels,
                              DEMO_LAYOUT_MODE_16CH,
                              &vdisParams.mosaicParams[i], forceLowCostScale,
                              gDemo_info.Type,
                              vdisParams.deviceParams[VDIS_DEV_HDCOMP].resolution);
        vdisParams.mosaicParams[i].userSetDefaultSWMLayout = TRUE;
    }
    if (vsysParams.systemUseCase != VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
        i = 2;
    else
        i = 1;

    /* For DM8168 16 CH Progressive usecase, SDTV does not support mosaic.
      * SDTV input is directly fed from the capture link */
    if(vsysParams.systemUseCase != VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
    {
        Demo_swMsGenerateLayout(VDIS_DEV_SD, 0, gDemo_info.maxVdisChannels,
                              DEMO_LAYOUT_MODE_7CH_1CH,
                              &vdisParams.mosaicParams[i], forceLowCostScale,
                              gDemo_info.Type,
                              vdisParams.deviceParams[VDIS_DEV_SD].resolution);
        vdisParams.mosaicParams[i].userSetDefaultSWMLayout = TRUE;
    }
#endif

    Vdis_init(&vdisParams);

    enableFWrite = Demo_getFileWriteEnable();

    /* Init the application specific module which will handle bitstream exchange */
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    VcapVencVdecVdis_setFileWriteMask(vsysParams.systemUseCase);
#endif

    /*h264 Enc: Needs to be checked for any channel where this flag is enabled,
         * right now just checking for channel 0, default value for other codecs is 0*/
    if(vencParams.encChannelParams[0].enableSVCExtensionFlag > 0)
        enableLayerWrite = TRUE;

    VcapVencVdecVdis_ipcBitsSetFileExtension("h264");
    VcapVencVdecVdis_ipcBitsInit(ipcbits_resArray, enableFWrite, enableLayerWrite);

    wallTimeBase = get_current_time_to_msec();
    //wallTimeBase = 0;
    Vdis_setWallTimeBase(wallTimeBase);

    /* Configure display in order to start grpx before video */
    Vsys_configureDisplay();

#if USE_FBDEV
    grpx_init(GRPX_FORMAT_RGB565);
#endif

    if(vsysParams.enableScd == TRUE)
    {
        Scd_bitsWriteCreate(demoId);
    }

    /* Create Link instances and connects compoent blocks */
    Vsys_create();


    if(vsysParams.enableOsd)
    {
        gDemo_info.osdEnable = TRUE;

        /* Create and initialize OSD window buffers */
        Demo_osdInit(gDemo_info.maxVencChannels, osdFormat);
        if ((vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH)            ||
            (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH) ||
            (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT))
        {
            /* set different OSD params for the secondary stream */
            for(chId = 8; chId < 16; chId++)
            {
                g_osdChParam[chId].numWindows = 2;
            }
        }
        if ((vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)||
            (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF))
        {
            /* set different OSD params for the secondary stream */
            for(chId = 16; chId < 32; chId++)
            {
                g_osdChParam[chId].numWindows = 2;
            }
        }


        for(chId = 0; chId < gDemo_info.maxVencChannels; chId++)
        {
            vcapParams.channelParams[chId].dynamicParams.osdChWinPrm = &g_osdChParam[chId];
            /* Initailize osdLink with created and set win params */
            Vcap_setDynamicParamChn(chId, &vcapParams.channelParams[chId].dynamicParams, VCAP_OSDWINPRM);

            vcapParams.channelParams[chId].dynamicParams.osdChBlindWinPrm = &g_osdChBlindParam[chId];
            /* Initailize osdLink with created and set win params */
            Vcap_setDynamicParamChn(chId, &vcapParams.channelParams[chId].dynamicParams, VCAP_OSDBLINDWINPRM);

        }
    }



#ifndef SYSTEM_DISABLE_AUDIO
        Demo_audioEnable(TRUE);
#endif

#if defined(TI_814X_BUILD)
    if( (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC) ||
         (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH) ||
          (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF) ||
           (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT));
    {
       Demo_blindAreaInit(gDemo_info.maxVcapChannels,vsysParams.systemUseCase);
    }
#endif
#if defined(TI_816X_BUILD)
    if(vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
    {
       Demo_blindAreaInit(gDemo_info.maxVcapChannels,vsysParams.systemUseCase);
    }
#endif

#if !defined(TI_814X_BUILD) && !defined(TI_8107_BUILD)
    if(enableSecondaryOut)
    {
        if(doProgressiveVenc)
        {
            Int32 chId;

            /* now use VIP-SC secondary output, so input to VIP-SC and VIP-SC secondary channel are both
               half of the real input framerate */
            for (chId=0; chId < vencParams.numSecondaryChn ; chId++)
            {
                /* At capture level, CIF stream id is 0. Set for CIF channels 0 ~ MAX_CH */
                Vcap_setFrameRate(chId, 2, 30, 30);
                if(vsysParams.enableScd == TRUE)
                {
                    /* At capture level, CIF stream for SCD, CIF channels 0 ~ MAX_CH */
                    Vcap_skipFidType(chId, VIDEO_FID_TYPE_BOT_FIELD);
                }

            }

            for (chId=0; chId < vencParams.numPrimaryChn; chId++)
            {
                /* At capture level, D1 stream id is 1. Set for D1 channels 0 ~ MAX_CH */
                if(vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                {
                    Vcap_setFrameRate(chId, 1, 60, 60);
                    Vcap_setFrameRate(chId, 0, 60, 60);
                }
                else
                {
                    Vcap_setFrameRate(chId, 1, 30, 30);

                    Vcap_setFrameRate(chId, 0, 60, 0);
                }

                if(vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
                {
                    Int32 i;

                    for (i=0; i<vdisParams.mosaicParams[0].numberOfWindows; i++)
                    {
                        if((vdisParams.mosaicParams[0].chnMap[i] == chId) && (vdisParams.mosaicParams[0].useLowCostScaling[i] == FALSE))
                            Vcap_setFrameRate(chId,0,60,30);
                    }

                    for (i=0; i<vdisParams.mosaicParams[1].numberOfWindows; i++)
                    {
                        if((vdisParams.mosaicParams[1].chnMap[i] == chId) && (vdisParams.mosaicParams[1].useLowCostScaling[i] == FALSE))
                            Vcap_setFrameRate(chId,0,60,30);
                    }
                    for (i=0; i<vdisParams.mosaicParams[2].numberOfWindows; i++)
                    {
                        if((vdisParams.mosaicParams[2].chnMap[i] == chId) && (vdisParams.mosaicParams[2].useLowCostScaling[i] == FALSE))
                            Vcap_setFrameRate(chId,0,60,30);
                    }
                }
            }
        }
        else
        {
            Int32 chId;

            /* now use VIP-SC secondary output, so input to VIP-SC and VIP-SC secondary channel are both
               half of the real input framerate */
            for (chId=0; chId < vencParams.numSecondaryChn ; chId++)
            {
                /* At capture level, CIF stream id is 0. Set for CIF channels 0 ~ MAX_CH */
                Vcap_setFrameRate(chId, 0, 30, 16);
                Vcap_skipFidType(chId, VIDEO_FID_TYPE_BOT_FIELD);
            }

            for (chId=0; chId < vencParams.numPrimaryChn; chId++)
            {
                /* At capture level, D1 stream id is 1. Set for D1 channels 0 ~ MAX_CH */
                Vcap_setFrameRate(chId, 1, 60, 60);
            }

        }
    }
    else
    {
        if(doProgressiveVenc)
        {
            for (chId=0; chId < vencParams.numPrimaryChn; chId++)
            {
                /* At capture level, D1 stream id is 1. Set for D1 channels 0 ~ MAX_CH */
                if(vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                {
                    Vcap_setFrameRate(chId, 1, 60, 30);
                    Vcap_setFrameRate(chId, 0, 60, 60);
                }
                else
                {
                    Vcap_setFrameRate(chId, 1, 60, 30);

                    Vcap_setFrameRate(chId, 0, 60, 0);
                }

                if(vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
                {
                    Int32 i;

                    for (i=0; i<vdisParams.mosaicParams[0].numberOfWindows; i++)
                    {
                        if((vdisParams.mosaicParams[0].chnMap[i] == chId) && (vdisParams.mosaicParams[0].useLowCostScaling[i] == FALSE))
                            Vcap_setFrameRate(chId,0,60,30);
                    }

                    for (i=0; i<vdisParams.mosaicParams[1].numberOfWindows; i++)
                    {
                        if((vdisParams.mosaicParams[1].chnMap[i] == chId) && (vdisParams.mosaicParams[1].useLowCostScaling[i] == FALSE))
                            Vcap_setFrameRate(chId,0,60,30);
                    }
                    for (i=0; i<vdisParams.mosaicParams[2].numberOfWindows; i++)
                    {
                        if((vdisParams.mosaicParams[2].chnMap[i] == chId) && (vdisParams.mosaicParams[2].useLowCostScaling[i] == FALSE))
                            Vcap_setFrameRate(chId,0,60,30);
                    }
                }
            }
        }
    }
#else
    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH)
    {
        for (chId=0; chId < vencParams.numSecondaryChn ; chId++)
        {
            Vcap_skipFidType(chId, VIDEO_FID_TYPE_BOT_FIELD);
        }
    }
#endif

    /* Start components in reverse order */
    Vdis_start();
    Vdec_start();
    Venc_start();
    Vcap_start();

    display_process_init();

    {
        /* Setting FPS for the Encoder Channels */
        for(chId=0; chId<vcapParams.numChn; chId++)
        {
            if(Demo_captureGetSignalStandard() == VSYS_STD_PAL)
            {
                if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
                {
                    Vcap_setFrameRate(chId,1, 50, 6);   /* Primary stream */
                    Vcap_setFrameRate(chId,2, 25, 4);   /* Secondary stream */
                    Vcap_setFrameRate(chId,3, 25, 1);   /* MJPEG stream */
                }
                if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT)
                {
                    Vcap_setFrameRate(chId,1, 50, 25);   /* Primary stream */
                    Vcap_setFrameRate(chId,2, 25, 25);   /* Secondary stream */
                    Vcap_setFrameRate(chId,3, 25, 1);    /* MJPEG stream */
                }
                memset(&params, 0, sizeof(params));

                Venc_setInputFrameRate(chId, 25);

                params.frameRate = 25;
                Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

                if(enableSecondaryOut)
                {
                    memset(&params, 0, sizeof(params));

                    Venc_setInputFrameRate(chId+vencParams.numPrimaryChn, 25);

                    params.frameRate = CIF_FPS_ENC_PAL;
                    params.targetBitRate = CIF_BITRATE * 1000;
                    Venc_setDynamicParam(chId+vencParams.numPrimaryChn, 0, &params, VENC_FRAMERATE);
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                    /* Set MJPEG Encode to 30 fps as frame rate control is done @ NSF for 4D1 case */
                    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC
                            || vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                    {
                        memset(&params, 0, sizeof(params));

                        Venc_setInputFrameRate(chId+(vencParams.numPrimaryChn * 2), 25);

                        params.frameRate = 25;
                        params.targetBitRate = MJPEG_BITRATE * 1000;

                        Venc_setDynamicParam(chId+(vencParams.numPrimaryChn * 2), 0, &params, VENC_FRAMERATE);
                    }
#endif
#ifdef TI_816X_BUILD
                    /* Set MJPEG Encode to 30 fps as frame rate control is done @ NSF for 4D1 case */
                    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                    {
                        memset(&params, 0, sizeof(params));

                        Venc_setInputFrameRate(chId+(vencParams.numPrimaryChn * 2), 25);

                        params.frameRate = 25;
                        params.targetBitRate = MJPEG_BITRATE * 1000;

                        Venc_setDynamicParam(chId+(vencParams.numPrimaryChn * 2), 0, &params, VENC_FRAMERATE);
                    }
#endif
                }
            }
            else if(Demo_captureGetSignalStandard() == VSYS_STD_NTSC)
            {
                if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
                {
                    Vcap_setFrameRate(chId,1, 60, 7);   /* Primary stream */
                    Vcap_setFrameRate(chId,2, 30, 4);  /* Secondary stream */
                    Vcap_setFrameRate(chId,3, 30, 1);   /* MJPEG stream */
                }
                if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT)
                {
                    Vcap_setFrameRate(chId,1, 60, 15);   /* Primary stream */
                    Vcap_setFrameRate(chId,2, 30, 30);   /* Secondary stream */
                    Vcap_setFrameRate(chId,3, 30, 1);    /* MJPEG stream */
                }
                memset(&params, 0, sizeof(params));
                Venc_setInputFrameRate(chId, 30);

                params.frameRate = 30;

                Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

                if(enableSecondaryOut)
                {
                    memset(&params, 0, sizeof(params));
                    if (doProgressiveVenc)
                    {
                        Venc_setInputFrameRate(chId+vencParams.numPrimaryChn, 30);
                        params.frameRate = CIF_FPS_ENC_NTSC;
                    }
                    else
                    {
                        Venc_setInputFrameRate(chId+vencParams.numPrimaryChn, 16);
                        params.frameRate = 16;
                    }

                    params.targetBitRate = CIF_BITRATE * 1000;

                    Venc_setDynamicParam(chId+vencParams.numPrimaryChn, 0, &params, VENC_FRAMERATE);
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                    /* Set MJPEG Encode to 30 fps as frame rate control is done @ NSF for 4D1 case */
                    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC
                            || vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                    {
                        memset(&params, 0, sizeof(params));

                        Venc_setInputFrameRate(chId+(vencParams.numPrimaryChn * 2), 30);

                        params.frameRate = 30;
                        params.targetBitRate = MJPEG_BITRATE * 1000;

                        Venc_setDynamicParam(chId+(vencParams.numPrimaryChn * 2), 0, &params, VENC_FRAMERATE);
                    }
#endif
#ifdef TI_816X_BUILD
                    /* Set MJPEG Encode to 30 fps as frame rate control is done @ NSF for 4D1 case */
                    if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                    {
                        memset(&params, 0, sizeof(params));

                        Venc_setInputFrameRate(chId+(vencParams.numPrimaryChn * 2), 30);

                        params.frameRate = 30;
                        params.targetBitRate = MJPEG_BITRATE * 1000;

                        Venc_setDynamicParam(chId+(vencParams.numPrimaryChn * 2), 0, &params, VENC_FRAMERATE);
                    }
#endif
                }
            }
            else
            {
                if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
                {
                    Vcap_setFrameRate(chId,1, 60, 7);   /* Primary stream */
                    Vcap_setFrameRate(chId,2, 30, 4);  /* Secondary stream */
                    Vcap_setFrameRate(chId,3, 30, 1);   /* MJPEG stream */
                }
                memset(&params, 0, sizeof(params));
                Venc_setInputFrameRate(chId, 30);

                params.frameRate = 30;

                Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

                if(enableSecondaryOut)
                {
                    memset(&params, 0, sizeof(params));
                    if (doProgressiveVenc)
                    {
                        Venc_setInputFrameRate(chId+vencParams.numPrimaryChn, 30);
                        params.frameRate = CIF_FPS_ENC_NTSC;
                    }
                    else
                    {
                        Venc_setInputFrameRate(chId+vencParams.numPrimaryChn, 16);
                        params.frameRate = 16;
                    }

                    params.targetBitRate = CIF_BITRATE * 1000;

                    Venc_setDynamicParam(chId+vencParams.numPrimaryChn, 0, &params, VENC_FRAMERATE);

                }

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                /* Set MJPEG Encode to 30 fps as frame rate control is done @ NSF for 4D1 case */
                if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC
                        || vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                {
                    memset(&params, 0, sizeof(params));

                    Venc_setInputFrameRate(chId+(vencParams.numPrimaryChn * 2), 1);

                    params.frameRate = 1;
                    params.targetBitRate = MJPEG_BITRATE * 1000;

                    Venc_setDynamicParam(chId+(vencParams.numPrimaryChn * 2), 0, &params, VENC_FRAMERATE);
                }
#endif
#ifdef TI_816X_BUILD
                /* Set MJPEG Encode to 30 fps as frame rate control is done @ NSF for 4D1 case */
                if (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
                {
                    memset(&params, 0, sizeof(params));

                    Venc_setInputFrameRate(chId+(vencParams.numPrimaryChn * 2), 1);

                    params.frameRate = 1;
                    params.targetBitRate = MJPEG_BITRATE * 1000;

                    Venc_setDynamicParam(chId+(vencParams.numPrimaryChn * 2), 0, &params, VENC_FRAMERATE);
                }
#endif
                printf (" DEMO: No video detected at CH [%d] !!!\n",
                     chId);

            }
        }

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        if ((vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH)
         || (vsysParams.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH)
           )
        {
            VENC_CHN_DYNAMIC_PARAM_S params_venc = { 0 };
            VCAP_CHN_DYNAMIC_PARAM_S params_vcap = { 0 };
            /*HACK HACK set secondary enc fps to 1fps */
            memset(&params_venc, 0, sizeof(params_venc));
            params_venc.frameRate = 30;
            for (chId = 8; chId < 16; chId++)
            {
                Venc_setInputFrameRate(chId, 30);
                Venc_setDynamicParam(chId, 0, &params_venc, VENC_FRAMERATE);
            }
            memset(&params_vcap, 0, sizeof(params_vcap));
            params_vcap.chDynamicRes.pathId = 1;
            params_vcap.chDynamicRes.width  = 704;
            params_vcap.chDynamicRes.height = 480;
            Vcap_setDynamicParamChn(0, &params_vcap, VCAP_RESOLUTION);
            Vcap_setDynamicParamChn(4, &params_vcap, VCAP_RESOLUTION);
            params_vcap.chDynamicRes.width  = 352;
            params_vcap.chDynamicRes.height = 240;
            Vcap_setDynamicParamChn(1, &params_vcap, VCAP_RESOLUTION);
            Vcap_setDynamicParamChn(2, &params_vcap, VCAP_RESOLUTION);
            Vcap_setDynamicParamChn(3, &params_vcap, VCAP_RESOLUTION);
            Vcap_setDynamicParamChn(5, &params_vcap, VCAP_RESOLUTION);
            Vcap_setDynamicParamChn(6, &params_vcap, VCAP_RESOLUTION);
            Vcap_setDynamicParamChn(7, &params_vcap, VCAP_RESOLUTION);
        }
#endif
    }
}



Void VcapVencVdecVdis_stop()
{
    VSYS_PARAMS_S contextInf;
    Vsys_getContext(&contextInf);
    display_process_deinit();

    Vsys_enableFastUsecaseSwitch(FALSE);

    printf("++++++++ VcapVencVdecVdis_stop()\n");
//    if(contextInf.enableScd)
//        Scd_bitsWriteStop();

    /* Stop components */
    Vcap_stop();
    Venc_stop();
    Vdec_stop();
    Vdis_stop();


#if USE_FBDEV
     grpx_exit();
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

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
/* TODO: add NTSC/PAL check ...*/
int Demo_change8ChMode(int demoId)
{
    int value;
    int chId;

    VENC_CHN_DYNAMIC_PARAM_S params_venc = { 0 };
    VCAP_CHN_DYNAMIC_PARAM_S params_vcap = { 0 };


    if (demoId != DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH &&
        demoId != DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT)
    {
        printf("This function is valid ONLY for DM814X and DM810X 8CH usecase!!!!!\n");
        return 0;
    }

    value = Demo_getIntValue("Select Mode(0:2D1+6CIF, 1:8 2CIF, 2:8D1 non-realtime)", 0, 2, 0);

    /*HACK HACK set secondary enc fps to 1fps */
    memset(&params_venc, 0, sizeof(params_venc));
    params_venc.frameRate = 30;
    for (chId = 8; chId < 16; chId++)
    {
        Venc_setInputFrameRate(chId, 30);
        Venc_setDynamicParam(chId, 0, &params_venc, VENC_FRAMERATE);
    }
    switch (value)
    {
    case 0:
#if defined(TI_8107_BUILD)
        if (DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT == demoId)
        {
            Demo_change8ChSwitchUsecase(0);
        }
#endif
        memset(&params_vcap, 0, sizeof(params_vcap));
        memset(&params_venc, 0, sizeof(params_venc));
        params_venc.frameRate = 30;
        for (chId = 0; chId < 8; chId++)
        {
            Venc_setInputFrameRate(chId, 30);
            Venc_setDynamicParam(chId, 0, &params_venc, VENC_FRAMERATE);
        }
        params_vcap.chDynamicRes.pathId = 1;
        params_vcap.chDynamicRes.width  = 704;
        params_vcap.chDynamicRes.height = 480;
        Vcap_setDynamicParamChn(0, &params_vcap, VCAP_RESOLUTION);
        Vcap_setDynamicParamChn(4, &params_vcap, VCAP_RESOLUTION);

        params_vcap.chDynamicRes.width  = 352;
        params_vcap.chDynamicRes.height = 240;
        Vcap_setDynamicParamChn(1, &params_vcap, VCAP_RESOLUTION);
        Vcap_setDynamicParamChn(2, &params_vcap, VCAP_RESOLUTION);
        Vcap_setDynamicParamChn(3, &params_vcap, VCAP_RESOLUTION);
        Vcap_setDynamicParamChn(5, &params_vcap, VCAP_RESOLUTION);
        Vcap_setDynamicParamChn(6, &params_vcap, VCAP_RESOLUTION);
        Vcap_setDynamicParamChn(7, &params_vcap, VCAP_RESOLUTION);

        demoId = DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH;

        break;
    case 1:
#if defined(TI_8107_BUILD)
        if (DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT == demoId)
        {
            Demo_change8ChSwitchUsecase(0);
        }
#endif
        memset(&params_vcap, 0, sizeof(params_vcap));
        memset(&params_venc, 0, sizeof(params_venc));
        params_vcap.chDynamicRes.pathId = 1;
        params_vcap.chDynamicRes.width  = 704;
        params_vcap.chDynamicRes.height = 240;
        params_venc.frameRate = 30;
        for (chId = 0; chId < 8; chId++)
        {
            Vcap_setDynamicParamChn(chId, &params_vcap, VCAP_RESOLUTION);
            Venc_setInputFrameRate(chId, 30);
            Venc_setDynamicParam(chId, 0, &params_venc, VENC_FRAMERATE);
        }
        demoId = DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH;
        break;
    case 2:
#if defined(TI_814X_BUILD)
        memset(&params_vcap, 0, sizeof(params_vcap));
        memset(&params_venc, 0, sizeof(params_venc));
        params_vcap.chDynamicRes.pathId = 1;
        params_vcap.chDynamicRes.width  = 704;
        params_vcap.chDynamicRes.height = 480;
        params_venc.frameRate = 15;
        for (chId = 0; chId < 8; chId++)
        {
            Vcap_setDynamicParamChn(chId, &params_vcap, VCAP_RESOLUTION);
            Venc_setInputFrameRate(chId, 30);
            Venc_setDynamicParam(chId, 0, &params_venc, VENC_FRAMERATE);
        }
#endif
#if defined(TI_8107_BUILD)
        if (DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH == demoId)
        {
            Demo_change8ChSwitchUsecase(2);
        }
        demoId = DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT;
#endif
        break;
    default:
        break;
    }

    return demoId;
}



#if defined(TI_8107_BUILD)
static int Demo_change8ChSwitchUsecase(int usecase)
{
    VSYS_PARAMS_S contextInf;
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VENC_PARAMS_S vencParams;
    VDEC_PARAMS_S vdecParams;
    VDIS_PARAMS_S vdisParams;
    Bool forceLowCostScale = FALSE, enableFWrite = FALSE, enableLayerWrite = FALSE;
    UInt32 chId;
    UInt8 osdFormat[ALG_LINK_OSD_MAX_CH];
    UInt32 demoId;

    /*******************************
             Stop usecase
    ******************************/
    Vsys_getContext(&contextInf);

    /* Stop components */
    Vcap_stop();
    Venc_stop();
    Vdec_stop();
    Vdis_stop();

    /* Free the osd buffers */
    Demo_osdDeinit();

    /* skip some de-init to allow fast use-case switch */
    Vsys_enableFastUsecaseSwitch(TRUE);

    Vsys_delete();

    if( contextInf.enableScd)
    {
        Scd_bitsWriteDelete();
    }

    VcapVencVdecVdis_ipcBitsExit();

    /* De-initialize components */
    Vcap_exit();
    Venc_exit();
    Vdec_exit();
    Vdis_exit();
    Vsys_exit();

    /*******************************
                     Start usecase
     ******************************/
    Vsys_params_init(&vsysParams);
    Vcap_params_init(&vcapParams);
    Venc_params_init(&vencParams);
    Vdec_params_init(&vdecParams);
    Vdis_params_init(&vdisParams);

    vcapParams.numChn          = 8;
    vencParams.numPrimaryChn   = 8;
    vencParams.numSecondaryChn = 8;
    vdecParams.numChn          = 8;
    vdisParams.numChannels     = 16;
    vsysParams.numChs          = 8;

    /* Most of progressive use cases have OSD in YUV420 SP format */
    memset(osdFormat, SYSTEM_DF_YUV420SP_UV, ALG_LINK_OSD_MAX_CH);

    /* Set the demo Id which is going to start */
    if (0 == usecase || 1 == usecase)
    {
        vsysParams.systemUseCase =
            VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH;
        demoId                   = DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH;
    }
    else
    {
        vsysParams.systemUseCase =
            VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT;
        demoId                   = DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT;
    }

    vsysParams.enableSecondaryOut = TRUE;
    vsysParams.enableNsf          = TRUE;
    vsysParams.enableMjpegEnc     = TRUE;
    vsysParams.enableCapture      = TRUE;
    vsysParams.enableNullSrc      = FALSE;
    vsysParams.enableOsd          = TRUE;
    vsysParams.enableScd          = TRUE;
    vsysParams.numDeis            = 1;
    vsysParams.numSwMs            = 2;
    vsysParams.numDisplays        = 2;
    vsysParams.enableAVsync       = TRUE;

    /* Override the context here as needed */
    Vsys_init(&vsysParams);

    /* Do reinit which doesnt uninit tvp5158 handles */
#if     0
    /* Override the context here as needed */
    Vcap_init(&vcapParams);
#else
    /* Override the context here as needed */
    Vcap_reInit(&vcapParams);
#endif

    /*Enabling generation of motion vector for channel 0 only,
         * for other channels please add to the below line*/

    vencParams.encChannelParams[0].enableAnalyticinfo = 1;
    vencParams.encChannelParams[0].maxBitRate = -1;
    VcapVencVdecVdis_setEncParans(&vencParams,&vsysParams);

    /* Override the context here as needed */
    Venc_init(&vencParams);

    /* Override the context here as needed */
    Vdec_init(&vdecParams);

    /* Override the context here as needed */
    vdisParams.deviceParams[VDIS_DEV_HDMI].resolution   = DEMO_HD_DISPLAY_DEFAULT_STD;
    vdisParams.deviceParams[VDIS_DEV_HDCOMP].resolution = DEMO_HD_DISPLAY_DEFAULT_STD;
    Vdis_tiedVencInit(VDIS_DEV_HDCOMP, VDIS_DEV_DVO2, &vdisParams);

    vdisParams.deviceParams[VDIS_DEV_SD].resolution     = VSYS_STD_NTSC;
    vdisParams.enableLayoutGridDraw = FALSE;

    /* set for 2 displays */
    Demo_swMsGenerateLayout(
            VDIS_DEV_HDMI,
            0,
            vdisParams.numChannels,
            DEMO_LAYOUT_MODE_7CH_1CH,
            &vdisParams.mosaicParams[VDIS_DEV_HDMI],
            forceLowCostScale,
            DEMO_TYPE_PROGRESSIVE,
            vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
    vdisParams.mosaicParams[VDIS_DEV_HDMI].userSetDefaultSWMLayout = TRUE;
    Demo_swMsGenerateLayout(
            VDIS_DEV_SD,
            0,
            vdisParams.numChannels,
            DEMO_LAYOUT_MODE_7CH_1CH,
            &vdisParams.mosaicParams[VDIS_DEV_SD],
            forceLowCostScale,
            DEMO_TYPE_PROGRESSIVE,
            vdisParams.deviceParams[VDIS_DEV_SD].resolution);
    vdisParams.mosaicParams[VDIS_DEV_SD].userSetDefaultSWMLayout = TRUE;

    Vdis_init(&vdisParams);

    if (vsysParams.enableScd == TRUE)
    {
        Scd_bitsWriteCreate(demoId);
    }

    enableFWrite = FALSE;//Demo_getFileWriteEnable();
    if (vencParams.encChannelParams[0].enableSVCExtensionFlag > 0)
    {
        enableLayerWrite = TRUE;
    }

    /* Init the application specific module which will handle bitstream exchange */
    ipcbits_resArray[0].width  = MCFW_IPCBITS_D1_WIDTH;
    ipcbits_resArray[0].height = MCFW_IPCBITS_D1_HEIGHT;
    ipcbits_resArray[1].width  = MCFW_IPCBITS_CIF_WIDTH;
    ipcbits_resArray[1].height = MCFW_IPCBITS_CIF_HEIGHT;
    VcapVencVdecVdis_ipcBitsInit(ipcbits_resArray, enableFWrite,enableLayerWrite);

    /* Create Link instances and connects compoent blocks */

    /* skip some init to allow fast use-case switch */
    Vsys_enableFastUsecaseSwitch(TRUE);

    Vsys_create();

    /* reset flag so that normal use-case exit free all resources */
    Vsys_enableFastUsecaseSwitch(FALSE);

    if (vsysParams.enableOsd)
    {
        gDemo_info.osdEnable = TRUE;

        /* Create and initialize OSD window buffers */
        Demo_osdInit(gDemo_info.maxVencChannels, osdFormat);

        for (chId = 0; chId < gDemo_info.maxVencChannels; chId++)
        {
            vcapParams.channelParams[chId].dynamicParams.osdChWinPrm = &g_osdChParam[chId];
            /* Initailize osdLink with created and set win params */
            Vcap_setDynamicParamChn(chId, &vcapParams.channelParams[chId].dynamicParams, VCAP_OSDWINPRM);
        }
    }

    /* Start components in reverse order */
    Vdis_start();
    Vdec_start();
    Venc_start();
    Vcap_start();

    return 0;
}
#endif



int Demo_change16ChSwitchUsecase(int userInputValue)
{
    VSYS_PARAMS_S contextInf;
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VENC_PARAMS_S vencParams;
    VDEC_PARAMS_S vdecParams;
    VDIS_PARAMS_S vdisParams;
    Bool forceLowCostScale = FALSE, enableFWrite = FALSE, enableLayerWrite = FALSE;
    UInt32 chId;
    UInt8 osdFormat[ALG_LINK_OSD_MAX_CH];
    UInt32 demoId;

    /*******************************
                     Stop usecase
         ******************************/
    Vsys_getContext(&contextInf);

    /* Stop components */
    Vcap_stop();
    Venc_stop();
    Vdec_stop();
    Vdis_stop();

    /* Free the osd buffers */
    Demo_osdDeinit();

    /* skip some de-init to allow fast use-case switch */
    Vsys_enableFastUsecaseSwitch(TRUE);

    Vsys_delete();

    if(contextInf.enableScd)
        Scd_bitsWriteDelete();

    VcapVencVdecVdis_ipcBitsExit();

    /* De-initialize components */
    Vcap_exit();
    Venc_exit();
    Vdec_exit();
    Vdis_exit();
    Vsys_exit();

    /*******************************
                     Start usecase
     ******************************/
    Vsys_params_init(&vsysParams);
    Vcap_params_init(&vcapParams);
    Venc_params_init(&vencParams);
    Vdec_params_init(&vdecParams);
    Vdis_params_init(&vdisParams);

    vcapParams.numChn          = 16;
    vencParams.numPrimaryChn   = 16;
    vencParams.numSecondaryChn = 16;
    vdecParams.numChn          = 16;
    vdisParams.numChannels     = 32;
    vsysParams.numChs          = 16;

    /* Most of progressive use cases have OSD in YUV420 SP format */
    memset(osdFormat, SYSTEM_DF_YUV420SP_UV, ALG_LINK_OSD_MAX_CH);

    /* Set the demo Id which is going to start */
    if (userInputValue == 1)
    {
        vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT;
        demoId = DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT;
    }
    else
    {
        vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF;
        demoId = DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1;
    }

    vsysParams.enableSecondaryOut = TRUE;
    vsysParams.enableNsf          = TRUE;
    vsysParams.enableMjpegEnc     = TRUE;
    vsysParams.enableCapture      = TRUE;
    vsysParams.enableNullSrc      = FALSE;

    vsysParams.enableOsd          = TRUE;
    vsysParams.enableScd          = FALSE;

    vsysParams.numDeis            = 1;
    vsysParams.numSwMs            = 2;
    vsysParams.numDisplays        = 2;
    vsysParams.enableAVsync       = TRUE;

    /* Override the context here as needed */
    Vsys_init(&vsysParams);

    /* Do reinit which doesnt uninit tvp5158 handles */
#if     0
    /* Override the context here as needed */
    Vcap_init(&vcapParams);
#else
    /* Override the context here as needed */
    Vcap_reInit(&vcapParams);
#endif

    /*Enabling generation of motion vector for channel 0 only,
         * for other channels please add to the below line*/

    vencParams.encChannelParams[0].enableAnalyticinfo = 1;
    vencParams.encChannelParams[0].maxBitRate = -1;
    VcapVencVdecVdis_setEncParans(&vencParams,&vsysParams);

    /* Override the context here as needed */
    Venc_init(&vencParams);

    /* Override the context here as needed */
    Vdec_init(&vdecParams);

    /* Override the context here as needed */
    vdisParams.deviceParams[VDIS_DEV_HDMI].resolution   = DEMO_HD_DISPLAY_DEFAULT_STD;
    vdisParams.deviceParams[VDIS_DEV_HDCOMP].resolution = DEMO_HD_DISPLAY_DEFAULT_STD;
    Vdis_tiedVencInit(VDIS_DEV_HDCOMP, VDIS_DEV_DVO2, &vdisParams);

    vdisParams.deviceParams[VDIS_DEV_SD].resolution     = VSYS_STD_NTSC;
    vdisParams.enableLayoutGridDraw = FALSE;

    /* set for 2 displays */
    Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, vdisParams.numChannels,
                          DEMO_LAYOUT_MODE_7CH_1CH,
                          &vdisParams.mosaicParams[VDIS_DEV_HDMI], forceLowCostScale,
                          DEMO_TYPE_PROGRESSIVE,
                          vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
    vdisParams.mosaicParams[VDIS_DEV_HDMI].userSetDefaultSWMLayout = TRUE;
    Demo_swMsGenerateLayout(VDIS_DEV_SD, 0, vdisParams.numChannels,
                          DEMO_LAYOUT_MODE_7CH_1CH,
                          &vdisParams.mosaicParams[VDIS_DEV_SD], forceLowCostScale,
                          DEMO_TYPE_PROGRESSIVE,
                          vdisParams.deviceParams[VDIS_DEV_SD].resolution);
    vdisParams.mosaicParams[VDIS_DEV_SD].userSetDefaultSWMLayout = TRUE;

    Vdis_init(&vdisParams);

    if(vsysParams.enableScd == TRUE)
    {
        Scd_bitsWriteCreate(demoId);
    }

    enableFWrite = FALSE;//Demo_getFileWriteEnable();
    if(vencParams.encChannelParams[0].enableSVCExtensionFlag > 0)
        enableLayerWrite = TRUE;

    /* Init the application specific module which will handle bitstream exchange */
    ipcbits_resArray[0].width  = MCFW_IPCBITS_D1_WIDTH;
    ipcbits_resArray[0].height  = MCFW_IPCBITS_D1_HEIGHT;
    ipcbits_resArray[1].width  = MCFW_IPCBITS_CIF_WIDTH;
    ipcbits_resArray[1].height = MCFW_IPCBITS_CIF_HEIGHT;
    VcapVencVdecVdis_ipcBitsInit(ipcbits_resArray, enableFWrite,enableLayerWrite);

    /* Create Link instances and connects compoent blocks */

    /* skip some init to allow fast use-case switch */
    Vsys_enableFastUsecaseSwitch(TRUE);

    Vsys_create();

    /* reset flag so that normal use-case exit free all resources */
    Vsys_enableFastUsecaseSwitch(FALSE);

    if(vsysParams.enableOsd)
    {
        gDemo_info.osdEnable = TRUE;

        /* Create and initialize OSD window buffers */
        Demo_osdInit(gDemo_info.maxVencChannels, osdFormat);

        for(chId = 0; chId < gDemo_info.maxVencChannels; chId++)
        {
            vcapParams.channelParams[chId].dynamicParams.osdChWinPrm = &g_osdChParam[chId];
            /* Initailize osdLink with created and set win params */
            Vcap_setDynamicParamChn(chId, &vcapParams.channelParams[chId].dynamicParams, VCAP_OSDWINPRM);
        }
    }

    /* Start components in reverse order */
    Vdis_start();
    Vdec_start();
    Venc_start();
    Vcap_start();

    return 0;
}

/* Only for TI_814X and TI_8107 16ch usecases
    Switch working mode between 16ch CIF realtime mode and 16ch D1 non-realtime mode */
int Demo_change16ChMode(int demoId)
{
    UInt32 value;

    if ((demoId != DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1) &&
       (demoId != DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT))
    {
        printf("This function is ONLY valid for TI_814X or TI_8107 16CH usecase!!!!!\n");
        return 0;
    }

    value = Demo_getIntValue("Select Mode(0:1D1+15CIF realtime, 1:16D1 non-realtime, 2:Switch Continously)", 0, 2, 0);

    if(value==0)
        Demo_change16ChSwitchUsecase(value);
    if(value==1)
        Demo_change16ChSwitchUsecase(value);
    if(value==2)
    {
        int loop=10000, i;

        for(i=0; i<loop; i++)
        {
            printf(" ###\n");
            printf(" ###\n");
            printf(" ### Starting Iteration %d of %d\n", i, loop);
            printf(" ###\n");
            printf(" ### Switching to 1D1 + 15CIF real-time\n");
            printf(" ###\n");
            Demo_change16ChSwitchUsecase(0);
            OSA_waitMsecs(15*1000);
            Demo_printInfo(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1);
            OSA_waitMsecs(1*1000);
            printf(" ###\n");
            printf(" ### Switching to 16D1 Non real-time\n");
            printf(" ###\n");
            Demo_change16ChSwitchUsecase(1);
            OSA_waitMsecs(15*1000);
            Demo_printInfo(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT);
            OSA_waitMsecs(1*1000);
            printf(" ###\n");
            printf(" ### Completed Iteration %d of %d\n", i, loop);
            printf(" ###\n");
            printf(" ###\n");
        }
    }

    return 0;
}
#endif


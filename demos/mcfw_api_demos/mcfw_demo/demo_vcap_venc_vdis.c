/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
  \file demo_vcap_venc_vdis.c
  \brief
  */

#include <demo_vcap_venc_vdis.h>
#include <demo_scd_bits_wr.h>

#define     CIF_FPS_ENC_NTSC         (30)
#define     CIF_FPS_ENC_PAL          (25)

#define     CIF_BITRATE         (500)

VcapVenc_Ctrl gVcapVenc_ctrl;

Void VcapVenc_start(Bool hdDemo)
{
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VENC_PARAMS_S vencParams;
    VDIS_PARAMS_S vdisParams;

    UInt32 useCase;
    UInt32 chId;
	UInt8  osdFormat[ALG_LINK_OSD_MAX_CH];

    Vsys_params_init(&vsysParams);
    if(hdDemo)
        useCase = VSYS_USECASE_MULTICHN_HD_VCAP_VENC;
    else
        useCase = VSYS_USECASE_MULTICHN_VCAP_VENC;


    Vcap_params_init(&vcapParams);
    Venc_params_init(&vencParams);
    Vdis_params_init(&vdisParams);

    if(hdDemo)
    {
        gDemo_info.maxVcapChannels = 4;
        gDemo_info.maxVdisChannels = 1;
        gDemo_info.maxVencChannels = 4;
        gDemo_info.maxVdecChannels = 0;

        vcapParams.numChn = 4;
        vcapParams.numDevices = 4;
        vencParams.numPrimaryChn = 4;
        vencParams.numSecondaryChn = 4;
        vdisParams.numChannels = 1;
    }
    else
    {
        gDemo_info.maxVcapChannels = 16;
        gDemo_info.maxVdisChannels = 16;
        gDemo_info.maxVencChannels = 16;
        gDemo_info.maxVdecChannels = 0;

        vcapParams.numChn = 16;
        vencParams.numPrimaryChn = 16;
        vencParams.numSecondaryChn = 16;
        vdisParams.numChannels = 16;
    }

    vsysParams.systemUseCase        = useCase;
    vsysParams.enableCapture        = TRUE;
    vsysParams.enableNsf            = FALSE;
    vsysParams.enableEncode         = TRUE;
    vsysParams.enableDecode         = FALSE;
    vsysParams.enableNullSrc        = FALSE;
    vsysParams.numDeis              = 2;
    if(hdDemo)
    {
        vsysParams.numSwMs              = 0;
        vsysParams.numDisplays          = 2;
        vsysParams.enableSecondaryOut   = TRUE;
        vsysParams.enableMjpegEnc       = TRUE;
        vsysParams.enableSclr           = TRUE;
        vsysParams.enableOsd            = TRUE;
        vsysParams.enableScd            = FALSE;//TRUE;
    }
    else
    {
        vsysParams.numSwMs              = 1;
        vsysParams.numDisplays          = 1;
        vsysParams.enableSecondaryOut   = TRUE; /* NOT USED for this use case */
        vsysParams.enableMjpegEnc       = TRUE;
        vsysParams.enableOsd            = TRUE;
        vsysParams.enableScd            = TRUE;
    }
    if (vsysParams.enableSecondaryOut)
    {
        if(vsysParams.enableMjpegEnc == FALSE)
           gDemo_info.maxVencChannels *= 2;
        else
           gDemo_info.maxVencChannels *= 3;
    }


    vdisParams.deviceParams[VDIS_DEV_HDMI].resolution   = DEMO_HD_DISPLAY_DEFAULT_STD;
    printf ("--------------- CHANNEL DETAILS-------------\n");
    printf ("Capture Channels => %d\n", vcapParams.numChn);
    printf ("Enc Channels => Primary %d, Secondary %d\n", vencParams.numPrimaryChn, vencParams.numSecondaryChn);
    printf ("Disp Channels => %d\n", vdisParams.numChannels);
    printf ("-------------------------------------------\n");

    Vsys_init(&vsysParams);

    Vcap_init(&vcapParams);
    Venc_init(&vencParams);
    if(hdDemo == FALSE)
    {
        Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, gDemo_info.maxVdisChannels,
                              DEMO_LAYOUT_MODE_7CH_1CH,
                              &vdisParams.mosaicParams[0], TRUE,
                              gDemo_info.Type,
                              vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
        vdisParams.mosaicParams[0].userSetDefaultSWMLayout = TRUE;
    }
    Vdis_init(&vdisParams);

    if(vsysParams.enableScd == TRUE)
    {
        Scd_bitsWriteCreate(useCase);
    }

    /* Init the application specific module which will handle bitstream exchange */
    VcapVenc_bitsWriteCreate();

    /* Init the application specific module which will handle video frame exchange */
    VcapVenc_ipcFramesCreate(useCase);
    VcapVenc_ipcFramesInSetCbInfo();

 	/* Configure display */
 	Vsys_configureDisplay();

    /* Create Link instances and connects compoent blocks */
    Vsys_create();

	if(vsysParams.enableOsd == TRUE)
	{
		gDemo_info.osdEnable = TRUE;
		/* OSd in A8 path */
		if(hdDemo)
		{
			osdFormat[0] = SYSTEM_DF_YUV420SP_UV;
			osdFormat[1] = SYSTEM_DF_YUV422I_YUYV;
		}
		else
		{
			memset(osdFormat, SYSTEM_DF_YUV420SP_UV, gDemo_info.maxVencChannels);
		}

		Demo_osdInit(gDemo_info.maxVencChannels, osdFormat);	

		for(chId = 0; chId < gDemo_info.maxVencChannels; chId++)
		{
			vcapParams.channelParams[chId].dynamicParams.osdChWinPrm = &g_osdChParam[chId];
			/* Initailize osdLink with created and set win params */
			Vcap_setDynamicParamChn(chId, &vcapParams.channelParams[chId].dynamicParams, VCAP_OSDWINPRM);
		}
	}

     Demo_blindAreaInit(gDemo_info.maxVcapChannels,useCase);

    {
        Int32 chId;
        if(hdDemo)
        {
            for (chId=0; chId < vencParams.numPrimaryChn; chId++)
            {
                Vcap_setFrameRate(chId, 0, 30, 30);
            }

            for (chId=0; chId < vencParams.numPrimaryChn ; chId++)
            {
                Vcap_setFrameRate(chId, 1, 30, 30);
            }

            /* Frame rate control for Secondary Path */
            for (chId=0; chId < vencParams.numSecondaryChn ; chId++)
            {
              /* At capture level, CIF stream id is 2. Set for CIF channels 0 ~ MAX_CH */
              Vcap_skipFidType(chId, VIDEO_FID_TYPE_BOT_FIELD);
              Vcap_setFrameRate(chId, 2, 30, 30);
            }
            /* Frame rate control for MJPEG */
            for (chId=0; chId < vencParams.numSecondaryChn ; chId++)
            {
              /* At capture level, CIF stream id is 2. Set for CIF channels 0 ~ MAX_CH */
              Vcap_skipFidType(chId, VIDEO_FID_TYPE_BOT_FIELD);
              Vcap_setFrameRate(chId, 3, 30, 1);
            }

        }
        else
        {
            /* now use VIP-SC secondary output, so input to VIP-SC and VIP-SC secondary channel are both
               half of the real input framerate */
            for (chId=0; chId < vencParams.numSecondaryChn ; chId++)
            {
                /* At capture level, CIF stream id is 2. Set for CIF channels 0 ~ MAX_CH */
                Vcap_skipFidType(chId, VIDEO_FID_TYPE_BOT_FIELD);
                Vcap_setFrameRate(chId, 2, 30, 30);
            }

            for (chId=0; chId < vencParams.numPrimaryChn; chId++)
            {
                /* At capture level, D1 stream id is 1. Set for D1 channels 0 ~ MAX_CH */
                Vcap_setFrameRate(chId, 0, 30, 30);
                Vcap_setFrameRate(chId, 1, 30, 30);
                Vcap_setFrameRate(chId, 3, 30, 1);
            }

        }
    }

    /* Start components in reverse order */
    Vdis_start();
    Venc_start();
    Vcap_start();


    {
        UInt32 chId;

        VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;
        VCAP_VIDEO_SOURCE_CH_STATUS_S *pChStatus;
        VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

        Vcap_getVideoSourceStatus( &videoSourceStatus );

        for(chId=0; chId<videoSourceStatus.numChannels; chId++)
        {
            pChStatus = &videoSourceStatus.chStatus[chId];

            if(pChStatus->isVideoDetect)
            {
                if(pChStatus->frameHeight == 288)
                {
                    if(hdDemo)
                    {
                        /* As HD demo works in DEIBypass mode FPS ratio is take care by that */
						Vcap_setFrameRate(chId, 0, 25, 25);
                    }
                    else
                    {
                        Vcap_setFrameRate(chId, 0, 50, 25);
                    }
                    Vcap_setFrameRate(chId, 1, 25, 25);
#ifdef CUSTOM_SD_DEMO
                    Vcap_setFrameRate(chId, 2, 25, 0);
#else
                    Vcap_setFrameRate(chId, 2, 25, 25);
#endif
                    Vcap_setFrameRate(chId, 3, 25, 1);
                    memset(&params, 0, sizeof(params));

                    Venc_setInputFrameRate(chId, 25);

                    params.frameRate = 25;
                    Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

                    if(vencParams.numSecondaryChn != 0)
                    {
                        memset(&params, 0, sizeof(params));

                        Venc_setInputFrameRate(chId+vencParams.numPrimaryChn, 25);

                        params.frameRate = CIF_FPS_ENC_PAL;
                        params.targetBitRate = CIF_BITRATE * 1000;
                        Venc_setDynamicParam(chId+vencParams.numPrimaryChn, 0, &params, VENC_FRAMERATE);
                    }

                }
                else if(pChStatus->frameHeight == 240)
                {
                    if(hdDemo)
                    {
                        /* As HD demo works in DEIBypass mode FPS ratio is take care by that */
                        Vcap_setFrameRate(chId, 0, 30, 30);
                    }
                    else
                    {
                        Vcap_setFrameRate(chId, 0, 60, 30);
                    }
                    Vcap_setFrameRate(chId, 1, 30, 30);
#ifdef CUSTOM_SD_DEMO
                    Vcap_setFrameRate(chId, 2, 30, 0);
#else
                    Vcap_setFrameRate(chId, 2, 30, 30);
#endif
                    Vcap_setFrameRate(chId, 3, 30, 1);

                    memset(&params, 0, sizeof(params));

                    Venc_setInputFrameRate(chId, 30);

                    params.frameRate = 30;
                    Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

                    if(vencParams.numSecondaryChn != 0)
                    {
                        memset(&params, 0, sizeof(params));

                        Venc_setInputFrameRate(chId+vencParams.numPrimaryChn, 30);

                        params.frameRate = CIF_FPS_ENC_NTSC;
                        params.targetBitRate = CIF_BITRATE * 1000;

                        Venc_setDynamicParam(chId+vencParams.numPrimaryChn, 0, &params, VENC_FRAMERATE);
                    }
                }
            }
            else
            {
                printf (" DEMO: %2d: No video detected at CH [%d,%d] !!!\n",
                     chId, pChStatus->vipInstId, pChStatus->chId);

            }
        }
    }
    VcapVenc_resetStatistics();
}

Void VcapVenc_stop()
{
    VSYS_PARAMS_S contextInf;
    Vsys_getContext(&contextInf);

    if(contextInf.enableScd)
        Scd_bitsWriteStop();

    VcapVenc_ipcFramesStop();
    /* Stop components */

    /* Stop components */
    Vcap_stop();
    Venc_stop();
    Vdis_stop();

    Demo_osdDeinit();
    VcapVenc_bitsWriteDelete();
    VcapVenc_ipcFramesDelete();

    Vsys_delete();

 	/* De-configure display */
 	Vsys_deConfigureDisplay();


    if(contextInf.enableScd)
        Scd_bitsWriteDelete();

    /* De-initialize components */
    Vcap_exit();
    Venc_exit();
    Vdis_exit();
    Vsys_exit();
}


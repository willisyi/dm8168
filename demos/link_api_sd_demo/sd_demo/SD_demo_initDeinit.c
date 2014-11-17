/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_initDeinit.h>

#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>

Void SD_Demo_setDefaultCfg(SD_Demo_Ctrl * sd_Demo_ctrl)
{
      UInt32 devId;
      UInt32 displayResDefault[SYSTEM_DC_MAX_VENC] =
        {VSYS_STD_1080P_60,   //SYSTEM_DC_VENC_HDMI,
         VSYS_STD_1080P_60,    //SYSTEM_DC_VENC_HDCOMP,
         VSYS_STD_1080P_60,    //SYSTEM_DC_VENC_DVO2
         VSYS_STD_NTSC        //SYSTEM_DC_VENC_SD,
        };

       memcpy(sd_Demo_ctrl->displayRes,displayResDefault,sizeof(sd_Demo_ctrl->displayRes));

       sd_Demo_ctrl->captureId              = SYSTEM_LINK_ID_INVALID;

       for(devId = 0; devId < MAX_ALG_LINK; devId++)
       {
          sd_Demo_ctrl->dspAlgId[devId] = SYSTEM_LINK_ID_INVALID;
       }
       for(devId = 0; devId < MAX_NSF_LINK; devId++)
       {
          sd_Demo_ctrl->nsfId[devId] = SYSTEM_LINK_ID_INVALID;
       }
       for(devId = 0; devId < MAX_SCLR_LINK; devId++)
       {
          sd_Demo_ctrl->sclrId[devId] = SYSTEM_LINK_ID_INVALID;
       }
       for(devId = 0; devId < MAX_DEI_LINK; devId++)
       {
          sd_Demo_ctrl->deiId[devId] = SYSTEM_LINK_ID_INVALID;
       }
       for(devId = 0; devId < MAX_SWMS_LINK; devId++)
       {
          sd_Demo_ctrl->swMsId[devId] = SYSTEM_LINK_ID_INVALID;
       }
       for(devId = 0; devId < MAX_DISPLAY_LINK; devId++)
       {
          sd_Demo_ctrl->displayId[devId] = SYSTEM_LINK_ID_INVALID;
       }
       for(devId = 0; devId < MAX_MERGE_LINK; devId++)
       {
          sd_Demo_ctrl->mergeId[devId] = SYSTEM_LINK_ID_INVALID;
       }
       for(devId = 0; devId < MAX_DUP_LINK; devId++)
       {
          sd_Demo_ctrl->dupId[devId] = SYSTEM_LINK_ID_INVALID;
       }

       sd_Demo_ctrl->encId                     = SYSTEM_LINK_ID_INVALID;
       sd_Demo_ctrl->decId                     = SYSTEM_LINK_ID_INVALID;
       
       sd_Demo_ctrl->ipcInVideoId                 = SYSTEM_LINK_ID_INVALID;
       sd_Demo_ctrl->ipcOutVpssId                = SYSTEM_LINK_ID_INVALID;

       for(devId = 0; devId < MAX_IPC_FRAMES_LINK; devId++)
       {
           sd_Demo_ctrl->ipcFramesOutVpssId[devId]        = SYSTEM_LINK_ID_INVALID;
           sd_Demo_ctrl->ipcFramesInDspId[devId]          = SYSTEM_LINK_ID_INVALID;
       }
       sd_Demo_ctrl->ipcFramesOutVpssToHostId  = SYSTEM_LINK_ID_INVALID;
       sd_Demo_ctrl->ipcFramesInHostId         = SYSTEM_LINK_ID_INVALID;
       sd_Demo_ctrl->ipcFramesOutHostId        = SYSTEM_LINK_ID_INVALID;
       sd_Demo_ctrl->ipcFramesInVpssFromHostId = SYSTEM_LINK_ID_INVALID;
       sd_Demo_ctrl->ipcBitsOutDSPId           = SYSTEM_LINK_ID_INVALID;
       sd_Demo_ctrl->ipcBitsOutRTOSId          = SYSTEM_LINK_ID_INVALID;

       for(devId = 0; devId < MAX_IPC_BITS_LINK; devId++)
       {
          sd_Demo_ctrl->ipcBitsInHLOSId[devId] = SYSTEM_LINK_ID_INVALID;
       }

       sd_Demo_ctrl->enableOsdAlgLink               = FALSE;
       sd_Demo_ctrl->enableScdAlgLink               = FALSE;
       sd_Demo_ctrl->enableVideoFramesExport        = FALSE;
       sd_Demo_ctrl->isPalMode                      = FALSE;
       sd_Demo_ctrl->callbackArg                    = NULL;
       sd_Demo_ctrl->bitscallbackArg                = NULL;

       for(devId = 0; devId < NUM_CAPTURE_DEVICES; devId++)
       {
           sd_Demo_ctrl->tvp5158Handle[devId] = NULL;
       }
      sd_Demo_ctrl->sii9022aHandle = NULL;
}

int SD_Demo_init()
{
    /* Disable tiler allocator for this usecase
     * for that tiler memory can be reused for
     * non-tiled allocation
     */
    SystemTiler_disableAllocator();


    /* Setting Default Values in the structure.
     * Initialize LinkIDs to invalid values.
     */
    SD_Demo_setDefaultCfg(&gSD_Demo_ctrl);
    
    /*Display Board information.
    */
    SD_Demo_detectBoard();

    System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES,
        NULL,
        0,
        TRUE
        );

    SD_Demo_memPrintHeapStatus();

    SD_Demo_displayCtrlInit();

    /* Start taking CPU load just before starting of links */
    SD_Demo_prfLoadCalcEnable(TRUE, FALSE, FALSE);

    return 0;
}

int SD_Demo_deinit()
{

    SD_Demo_displayCtrlDeInit();

    /* Print the HWI, SWI and all tasks load */
    /* Reset the accumulated timer ticks */
    SD_Demo_prfLoadCalcEnable(FALSE, TRUE, FALSE);

    /* Reenable tiler allocator taht was disabled by this usecase
     * at delete time.
     */
    SystemTiler_enableAllocator();

    return 0;
}

Int32 SD_Demo_displayCtrlInit()
{
    Int32   status, i;


    for (i = 0; i < SYSTEM_DC_MAX_VENC; i++)
    {
        gSD_Demo_ctrl.prm.deviceParams[i].resolution = gSD_Demo_ctrl.displayRes[i];
    }

    SD_Demo_vDisParams_init(&gSD_Demo_ctrl.prm);

    status = SD_Demo_displayInit(&gSD_Demo_ctrl.prm);

    status = System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_INIT,
        &gSD_Demo_ctrl.prm,
        sizeof(gSD_Demo_ctrl.prm),
        TRUE
        );
    UTILS_assert(status==OSA_SOK);
    return status;
}

Int32 SD_Demo_vDisParams_init(VDIS_PARAMS_S * pContext)
{
    UInt16 i, j, width, height;

    memset(pContext, 0, sizeof(VDIS_PARAMS_S));
    for(i = 0; i < NUM_DISPLAY_DEVICES; i++)
    {
        pContext->deviceParams[i].backGroundColor = 0;

        pContext->deviceParams[i].resolution  = VSYS_STD_1080P_60;

        width  = 1920;
        height = 1080;

        if(i == DIS_DEV_SD)
        {
            pContext->deviceParams[i].resolution  = VSYS_STD_NTSC;

            width  = 720;
            height = 480;

        }

        pContext->mosaicParams[i].displayWindow.start_X = 0;
        pContext->mosaicParams[i].displayWindow.start_Y = 0;
        pContext->mosaicParams[i].displayWindow.width   = width;
        pContext->mosaicParams[i].displayWindow.height  = height;

        pContext->mosaicParams[i].numberOfWindows       = 1;
        pContext->mosaicParams[i].outputFPS             = 30;

        for(j=0;j<pContext->mosaicParams[i].numberOfWindows;j++)
        {
            pContext->mosaicParams[i].winList[j].start_X    = 0;
            pContext->mosaicParams[i].winList[j].start_Y    = 0;
            pContext->mosaicParams[i].winList[j].width      = width;
            pContext->mosaicParams[i].winList[j].height     = height;

            pContext->mosaicParams[i].chnMap[j]             = j;
        }
        pContext->mosaicParams[i].userSetDefaultSWMLayout = TRUE;
    }

    /* Configure output Info for vencs */

    pContext->tiedDevicesMask = DIS_VENC_HDMI | DIS_VENC_DVO2;
    pContext->enableConfigExtVideoEncoder = TRUE;

    pContext->deviceParams[DIS_DEV_DVO2].enable = TRUE;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.vencNodeNum = DIS_VENC_DVO2;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.aFmt = DIS_A_OUTPUT_COMPOSITE;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.dvoFidPolarity = DIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.dvoVsPolarity = DIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.dvoHsPolarity = DIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.dvoActVidPolarity = DIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.dvoFmt = DIS_DVOFMT_DOUBLECHAN;
    pContext->deviceParams[DIS_DEV_DVO2].outputInfo.dataFormat = SYSTEM_DF_YUV422SP_UV;

    pContext->deviceParams[DIS_DEV_HDMI].enable = TRUE;
    pContext->deviceParams[DIS_DEV_HDMI].outputInfo.vencNodeNum = DIS_VENC_HDMI;
    pContext->deviceParams[DIS_DEV_HDMI].outputInfo.aFmt = DIS_A_OUTPUT_COMPOSITE;

    pContext->deviceParams[DIS_DEV_HDMI].outputInfo.dvoFmt = DIS_DVOFMT_TRIPLECHAN_EMBSYNC;
    pContext->deviceParams[DIS_DEV_HDMI].outputInfo.dataFormat = SYSTEM_DF_RGB24_888;

    pContext->deviceParams[DIS_DEV_SD].enable = TRUE;
    pContext->deviceParams[DIS_DEV_SD].outputInfo.vencNodeNum = DIS_VENC_SD;
    pContext->deviceParams[DIS_DEV_SD].outputInfo.aFmt = DIS_A_OUTPUT_COMPOSITE;
    pContext->deviceParams[DIS_DEV_SD].outputInfo.dvoFmt = DIS_DVOFMT_TRIPLECHAN_DISCSYNC;
    pContext->deviceParams[DIS_DEV_SD].outputInfo.dataFormat = SYSTEM_DF_RGB24_888;

    pContext->deviceParams[DIS_DEV_HDCOMP].enable = TRUE;
    pContext->deviceParams[DIS_DEV_HDCOMP].outputInfo.vencNodeNum = DIS_VENC_HDCOMP;
    pContext->deviceParams[DIS_DEV_HDCOMP].outputInfo.aFmt = DIS_A_OUTPUT_COMPONENT;
    pContext->deviceParams[DIS_DEV_HDCOMP].outputInfo.dvoFmt = DIS_DVOFMT_TRIPLECHAN_EMBSYNC;

    pContext->deviceParams[DIS_DEV_HDCOMP].outputInfo.dataFormat = SYSTEM_DF_RGB24_888;
    pContext->tiedDevicesMask = DIS_VENC_HDCOMP | DIS_VENC_DVO2;
    pContext->enableEdgeEnhancement = TRUE;

    pContext->deviceParams[SYSTEM_DC_VENC_HDMI].colorSpaceMode = DIS_CSC_MODE_SDTV_GRAPHICS_Y2R;

    pContext->enableLayoutGridDraw = FALSE;
    
    return 0;
}

Int32 SD_Demo_displayInit(VDIS_PARAMS_S * pContext)
{
    int status;
    Device_VideoEncoderCreateParams      encCreateParams;
    Device_VideoEncoderCreateStatus      encCreateStatus;
    Device_HdmiChipId                    hdmiId;
    Device_SiI9022aHpdPrms               hpdPrms;
    Device_SiI9022aModeParams            modePrms;

    /* Initialize and create video encoders */

    Device_sii9022aInit();

    /* Open HDMI Tx */
    encCreateParams.deviceI2cInstId = 1;

    encCreateParams.deviceI2cAddr   = Device_getVidDecI2cAddr(
                                                     DEVICE_VID_ENC_SII9022A_DRV,
                                                     0);
    encCreateParams.inpClk = 0;
    encCreateParams.hdmiHotPlugGpioIntrLine = 0;

    encCreateParams.syncMode = DEVICE_VIDEO_ENCODER_EMBEDDED_SYNC;
    encCreateParams.clkEdge = FALSE;


    gSD_Demo_ctrl.sii9022aHandle =
                          Device_sii9022aCreate(DEVICE_VID_ENC_SII9022A_DRV,
                                                 0, // instId - need to change
                                                 &(encCreateParams),
                                                 &(encCreateStatus));

    status = Device_sii9022aControl(gSD_Demo_ctrl.sii9022aHandle,
                                   IOCTL_DEVICE_SII9022A_GET_DETAILED_CHIP_ID,
                                   &hdmiId,
                                   NULL);
    OSA_assert(status >= 0);

    status = Device_sii9022aControl(gSD_Demo_ctrl.sii9022aHandle,
                                   IOCTL_DEVICE_SII9022A_QUERY_HPD,
                                   &hpdPrms,
                                   NULL);
    OSA_assert(status >= 0);

    modePrms.standard = VSYS_STD_1080P_60;

    status = Device_sii9022aControl(gSD_Demo_ctrl.sii9022aHandle,
                                   IOCTL_DEVICE_VIDEO_ENCODER_SET_MODE,
                                   &modePrms,
                                   NULL);
    OSA_assert(status >= 0);

#ifdef TI816X_EVM

    System_Ths7360SfCtrl   thsCtrl;
    UInt32                 standard;

    Device_thsFiltersInit();

    /* TBD : Retrieve from DIS  */
    standard = VSYS_STD_1080P_60;

    /* THS is tied to HDCOMP/HDDAC only for EVM */
    switch (standard)
    {
        case VSYS_STD_720P_60:
        case VSYS_STD_720P_50:
        case VSYS_STD_1080I_60:
        case VSYS_STD_1080I_50:
        case VSYS_STD_1080P_30:
            thsCtrl = DEVICE_THS7360_SF_HD_MODE;
            break;

        default:
        case VSYS_STD_1080P_60:
        case VSYS_STD_1080P_50:
            thsCtrl = DEVICE_THS7360_SF_TRUE_HD_MODE;
            break;
    }
    Device_ths7360SetSfParams(thsCtrl);
    Device_ths7360SetSdParams(DEVICE_THSFILTER_ENABLE_MODULE);

#endif
    return  0;
}


Int32 SD_Demo_displayCtrlDeInit()
{
    Int32 status;

    status = System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_DEINIT,
        NULL,
        0,
        TRUE
        );
    UTILS_assert(status==OSA_SOK);

    return status;
}

Int32 SD_Demo_videoDecoderInit()
{
    Int32 i;
    
    Device_VideoDecoderVideoModeParams vidDecVideoModeArgs[NUM_CAPTURE_DEVICES];

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

    SD_Demo_configVideoDecoder(vidDecVideoModeArgs, NUM_CAPTURE_DEVICES);

    return 0;
}

Int32 SD_Demo_configVideoDecoder(Device_VideoDecoderVideoModeParams * modeParams, UInt32 numDevices)
{
    Int32 status = 0;


//    if(Config.enableConfigExtVideoDecoder)
    {
        Int32 status = 0;
        Device_VideoDecoderChipIdParams      vidDecChipIdArgs;
        Device_VideoDecoderChipIdStatus      vidDecChipIdStatus;
        VCAP_VIDEO_SOURCE_STATUS_PARAMS_S    videoStatusArgs;
        VCAP_VIDEO_SOURCE_CH_STATUS_S        videoStatus;
        Int32                                i;
        Device_VideoDecoderCreateParams      createArgs;
        Device_VideoDecoderCreateStatus      createStatusArgs;

        Device_VideoDecoderVideoModeParams   vidDecVideoModeArgs;
        for(i = 0; i < numDevices; i++)
        {
            gSD_Demo_ctrl.tvp5158Handle[i] = NULL;
        }


        /* Initialize and create video decoders */
        Device_tvp5158Init();

        for(i = 0; i < numDevices; i++)
        {
            memset(&createArgs, 0, sizeof(Device_VideoDecoderCreateParams));

            createArgs.deviceI2cInstId    = 1;

            createArgs.numDevicesAtPort   = 1;
            createArgs.deviceI2cAddr[i]
                                          = Device_getVidDecI2cAddr(
                                                             DEVICE_VID_DEC_TVP5158_DRV,
                                                             i);
            createArgs.deviceResetGpio[i] = DEVICE_VIDEO_DECODER_GPIO_NONE;

            gSD_Demo_ctrl.tvp5158Handle[i] = Device_tvp5158Create(
                                                        DEVICE_VID_DEC_TVP5158_DRV,
                                                         i, // instId - need to change
                                                         &(createArgs),
                                                         &(createStatusArgs));

            vidDecChipIdArgs.deviceNum = 0;

            status = Device_tvp5158Control(gSD_Demo_ctrl.tvp5158Handle[i],
                                           IOCTL_DEVICE_VIDEO_DECODER_GET_CHIP_ID,
                                           &vidDecChipIdArgs,
                                           &vidDecChipIdStatus);
            if (status >= 0)
            {
                videoStatusArgs.channelNum = 0;

                status = Device_tvp5158Control(gSD_Demo_ctrl.tvp5158Handle[i],
                                               IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS,
                                               &videoStatusArgs, &videoStatus);

                if (videoStatus.isVideoDetect)
                {
                    printf("\nCAPTURE: Detected video (%dx%d@%dHz, %d)!!!\n",
                               videoStatus.frameWidth,
                               videoStatus.frameHeight,
                               1000000 / videoStatus.frameInterval,
                               videoStatus.isInterlaced);

                    /* Assumption here is height width of video remains same across channels */
                    if (videoStatus.frameHeight == 288)
                        gSD_Demo_ctrl.tvp5158Handle[i]->isPalMode = TRUE;

                }
                else
                {
                    printf("\nCAPTURE ERROR: Could not detect video at tvp instace %d!!!\n",i);
                }
            }
            else
            {
                printf("\nCAPTURE ERROR: Could not get chip id for tvp instace %d!!!\n",i);
            }
        }

        /* Configure video decoder */

        for(i = 0; i < numDevices; i++)
        {
            if(modeParams != NULL)
            {

                memset(&vidDecVideoModeArgs,
                       0, sizeof(Device_VideoDecoderVideoModeParams));

                memcpy(&vidDecVideoModeArgs,
                       &modeParams[i],
                       sizeof(Device_VideoDecoderVideoModeParams));

                status = Device_tvp5158Control(gSD_Demo_ctrl.tvp5158Handle[i],
                                               IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_MODE,
                                               &vidDecVideoModeArgs,
                                               NULL);
            }
            else
            {
                status = -1;
            }
        }

    }

    return status;
}

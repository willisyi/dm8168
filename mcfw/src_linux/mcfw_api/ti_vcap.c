/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "ti_vsys_priv.h"
#include "ti_vcap_priv.h"

#include <device.h>
#include <device_videoDecoder.h>


#define VCAP_TRACE_ENABLE_FXN_ENTRY_EXIT           (0)
#define VCAP_TRACE_INFO_PRINT_INTERVAL             (8192)


#if VCAP_TRACE_ENABLE_FXN_ENTRY_EXIT
#define VCAP_TRACE_FXN(str,...)                    do {                           \
                                                     static Int printInterval = 0;\
                                                     if ((printInterval % VCAP_TRACE_INFO_PRINT_INTERVAL) == 0) \
                                                     {                                                          \
                                                         OSA_printf("TI_VCAP:%s function:%s",str,__func__);     \
                                                         OSA_printf(__VA_ARGS__);                               \
                                                     }                                                          \
                                                     printInterval++;                                           \
                                                   } while (0)
#define VCAP_TRACE_FXN_ENTRY(...)                  VCAP_TRACE_FXN("Entered",__VA_ARGS__)
#define VCAP_TRACE_FXN_EXIT(...)                   VCAP_TRACE_FXN("Leaving",__VA_ARGS__)
#else
#define VCAP_TRACE_FXN_ENTRY(...)
#define VCAP_TRACE_FXN_EXIT(...)
#endif

/* =============================================================================
 * Globals
 * =============================================================================
 */

VCAP_MODULE_CONTEXT_S gVcapModuleContext;

/* =============================================================================
 * Vcap module APIs
 * =============================================================================
 */


Void Vcap_params_init(VCAP_PARAMS_S * pContext)
{
    UInt16 devId, chnId, strmId;

    memset(pContext, 0, sizeof(VCAP_PARAMS_S));
    for(devId = 0; devId < VCAP_DEV_MAX; devId++)
    {
        pContext->deviceParams[devId].portEnable = FALSE;
        pContext->deviceParams[devId].portMode   = VCAP_MODE_BT656_8BIT_YUV422;
        pContext->deviceParams[devId].dataFormat = VCAP_MULTICHN_OPTION_4D1;
        pContext->deviceParams[devId].signalType = VS_AUTO_DETECT;
    }
    for(chnId = 0; chnId < VCAP_CHN_MAX; chnId++)
    {
        for(strmId=0; strmId<VCAP_STRM_MAX; strmId++)
        {
            pContext->channelParams[chnId].strmEnable[strmId]             = FALSE;
            pContext->channelParams[chnId].strmFormat[strmId]             = VF_YUV422I_UYVY;
            pContext->channelParams[chnId].strmResolution[strmId].start_X = 0;
            pContext->channelParams[chnId].strmResolution[strmId].start_Y = 0;
            pContext->channelParams[chnId].strmResolution[strmId].width   = 0;
            pContext->channelParams[chnId].strmResolution[strmId].height  = 0;
        }
        pContext->channelParams[chnId].deviceId                   = 0;
        pContext->channelParams[chnId].dynamicParams.contrast     = 0;
        pContext->channelParams[chnId].dynamicParams.satauration  = 0;
        pContext->channelParams[chnId].dynamicParams.brightness   = 0;
        pContext->channelParams[chnId].dynamicParams.hue          = 0;
    }

    pContext->enableConfigExtVideoDecoder = TRUE;


}


Int32 Vcap_init(VCAP_PARAMS_S * pContext)
{
    //UInt16 devId,
    UInt16 chnId, strmId, linkId;

    /* Mark all links related to capture as invalid by default, they will be setup with valid IDs later */
    gVcapModuleContext.captureId             = SYSTEM_LINK_ID_INVALID;
    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        gVcapModuleContext.nsfId[linkId]                 = SYSTEM_LINK_ID_INVALID;
    }
    gVcapModuleContext.capSwMsId             = SYSTEM_LINK_ID_INVALID;
    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        gVcapModuleContext.sclrId[linkId]             = SYSTEM_LINK_ID_INVALID;
    }
    for(linkId=0; linkId<MAX_ALG_LINK; linkId++)
    {
        gVcapModuleContext.dspAlgId[linkId]           = SYSTEM_LINK_ID_INVALID;
    }
    for(linkId=0; linkId<MAX_IPC_FRAMES_LINK; linkId++)
    {
        gVcapModuleContext.ipcFramesOutVpssId[linkId]    = SYSTEM_LINK_ID_INVALID;
        gVcapModuleContext.ipcFramesInDspId[linkId]      = SYSTEM_LINK_ID_INVALID;
    }
    for(linkId=0; linkId<MAX_DEI_LINK; linkId++)
    {
        gVcapModuleContext.deiId[linkId] = SYSTEM_LINK_ID_INVALID;
    }
    gVcapModuleContext.nullSrcId                = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcFramesOutVpssToHostId = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcFramesInHostId        = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcBitsInHLOSId          = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.callbackFxn.newDataAvailableCb       = NULL;
    gVcapModuleContext.callbackArg                          = NULL;
    gVcapModuleContext.bitscallbackFxn.newDataAvailableCb   = NULL;
    gVcapModuleContext.bitscallbackArg                      = NULL;

    /* initialize counters */
    gVcapModuleContext.numChannels = 0;

    if(pContext==NULL)
    {
        Vcap_params_init(&gVcapModuleContext.vcapConfig);
    }
    else
    {

        for (chnId = 0; chnId < VCAP_CHN_MAX; chnId++)
        {
            for(strmId=0; strmId<VCAP_STRM_MAX; strmId++)
            {
                if(pContext->channelParams[chnId].strmEnable[strmId])
                {
                    gVcapModuleContext.numChannels++;
                }
            }
        }

        memcpy(&gVcapModuleContext.vcapConfig, pContext, sizeof(VCAP_PARAMS_S));

        gVcapModuleContext.numChannels = pContext->numChn;

#ifndef SYSTEM_USE_VIDEO_DECODER
        for(chnId = 0; chnId < VCAP_DEV_MAX; chnId++)
        {
            gVcapModuleContext.tvp5158Handle[chnId] = NULL;
        }

        gVcapModuleContext.colorPrm.videoBrightness = 0x80;    /* TUNED for
                                                                * specific
                                                                * scene's, to
                                                                * make black
                                                                * blacker */
        gVcapModuleContext.colorPrm.videoContrast   = 0x80;     /* TUNED for
                                                                * specific
                                                                * scene's, to
                                                                * make black
                                                                * blacker */
        gVcapModuleContext.colorPrm.videoSaturation  = 0x80;    /* default */

        gVcapModuleContext.colorPrm.videoHue         = 0x00;

#endif

        gVcapModuleContext.isPalMode = FALSE;

    }
    return 0;
}


/* Init done once; do not uninit tvp5158Handle */
Int32 Vcap_reInit(VCAP_PARAMS_S * pContext)
{
    //UInt16 devId,
    UInt16 chnId, strmId, linkId;

    /* Mark all links related to capture as invalid by default, they will be setup with valid IDs later */
    gVcapModuleContext.captureId             = SYSTEM_LINK_ID_INVALID;
    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        gVcapModuleContext.nsfId[linkId]                 = SYSTEM_LINK_ID_INVALID;
    }
    gVcapModuleContext.capSwMsId             = SYSTEM_LINK_ID_INVALID;
    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        gVcapModuleContext.sclrId[linkId]             = SYSTEM_LINK_ID_INVALID;
    }
    for(linkId=0; linkId<MAX_ALG_LINK; linkId++)
    {
        gVcapModuleContext.dspAlgId[linkId]           = SYSTEM_LINK_ID_INVALID;
    }
    for(linkId=0; linkId<MAX_IPC_FRAMES_LINK; linkId++)
    {
        gVcapModuleContext.ipcFramesOutVpssId[linkId]    = SYSTEM_LINK_ID_INVALID;
        gVcapModuleContext.ipcFramesInDspId[linkId]      = SYSTEM_LINK_ID_INVALID;
    }
    for(linkId=0; linkId<MAX_DEI_LINK; linkId++)
    {
        gVcapModuleContext.deiId[linkId] = SYSTEM_LINK_ID_INVALID;
    }
    gVcapModuleContext.nullSrcId                = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcFramesOutVpssToHostId = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcFramesInHostId        = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.ipcBitsInHLOSId          = SYSTEM_LINK_ID_INVALID;
    gVcapModuleContext.callbackFxn.newDataAvailableCb       = NULL;
    gVcapModuleContext.callbackArg                          = NULL;
    gVcapModuleContext.bitscallbackFxn.newDataAvailableCb   = NULL;
    gVcapModuleContext.bitscallbackArg                      = NULL;

    /* initialize counters */
    gVcapModuleContext.numChannels = 0;

    if(pContext==NULL)
    {
        Vcap_params_init(&gVcapModuleContext.vcapConfig);
    }
    else
    {

        for (chnId = 0; chnId < VCAP_CHN_MAX; chnId++)
        {
            for(strmId=0; strmId<VCAP_STRM_MAX; strmId++)
            {
                if(pContext->channelParams[chnId].strmEnable[strmId])
                {
                    gVcapModuleContext.numChannels++;
                }
            }
        }

        memcpy(&gVcapModuleContext.vcapConfig, pContext, sizeof(VCAP_PARAMS_S));

        gVcapModuleContext.numChannels = pContext->numChn;

#ifndef SYSTEM_USE_VIDEO_DECODER
        gVcapModuleContext.colorPrm.videoBrightness = 0x80;    /* TUNED for
                                                                * specific
                                                                * scene's, to
                                                                * make black
                                                                * blacker */
        gVcapModuleContext.colorPrm.videoContrast   = 0x80;     /* TUNED for
                                                                * specific
                                                                * scene's, to
                                                                * make black
                                                                * blacker */
        gVcapModuleContext.colorPrm.videoSaturation  = 0x80;    /* default */

        gVcapModuleContext.colorPrm.videoHue         = 0x00;

#endif

        gVcapModuleContext.isPalMode = FALSE;

    }
    return 0;
}

Void Vcap_exit()
{
    /* Empty for now */
}


Int32 Vcap_start()
{
    UInt32 linkId, i;
    Int32  status = 0;

	for(i = 0; i < VCAP_DEV_MAX; i++)
	{
		if(gVcapModuleContext.tvp5158Handle[i])
		{
	        status = Device_tvp5158Control(gVcapModuleContext.tvp5158Handle[i],
	                               DEVICE_CMD_START,
	                               NULL,
	                               NULL);
		}
	}
    /* start can happen in any order, except its recommended to start capture Link the last */
    if(gVcapModuleContext.ipcFramesOutVpssToHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gVcapModuleContext.ipcFramesOutVpssToHostId);
    }

    if(gVcapModuleContext.ipcFramesInHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gVcapModuleContext.ipcFramesInHostId);
    }

    if(gVcapModuleContext.ipcBitsInHLOSId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gVcapModuleContext.ipcBitsInHLOSId);
    }
    /* start can happen in any order, except its recommended to start capture Link the last */
    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        if(gVcapModuleContext.nsfId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gVcapModuleContext.nsfId[linkId]);
        }
    }

    /* start can happen in any order, except its recommended to start capture Link the last */
    for(linkId = 0; linkId < MAX_IPC_FRAMES_LINK; linkId++)
    {
        if(gVcapModuleContext.ipcFramesOutVpssId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gVcapModuleContext.ipcFramesOutVpssId[linkId]);
        }
        if(gVcapModuleContext.ipcFramesInDspId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gVcapModuleContext.ipcFramesInDspId[linkId]);
        }
    }

    for(linkId = 0; linkId < MAX_DEI_LINK; linkId++)
    {
        if(gVcapModuleContext.deiId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gVcapModuleContext.deiId[linkId]);
        }
    }

    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        if(gVcapModuleContext.sclrId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gVcapModuleContext.sclrId[linkId]);
        }
    }

    if(gVcapModuleContext.nullSrcId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gVcapModuleContext.nullSrcId);
    }

    /* Start taking CPU load just before starting of capture link */
    MultiCh_prfLoadCalcEnable(TRUE, FALSE, FALSE);

    if(gVcapModuleContext.capSwMsId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gVcapModuleContext.capSwMsId);
    }

    if(gVcapModuleContext.captureId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gVcapModuleContext.captureId);
    }

    return 0;
}


Int32 Vcap_stop()
{
    UInt32 linkId, i;
    Int32 status = 0;

	for(i = 0; i < VCAP_DEV_MAX; i++)
	{
		if(gVcapModuleContext.tvp5158Handle[i])
		{
	        status = Device_tvp5158Control(gVcapModuleContext.tvp5158Handle[i],
	                               DEVICE_CMD_STOP,
	                               NULL,
	                               NULL);
		}
	}

    /* stop needs to be in the reseverse order of create */

    if(gVcapModuleContext.captureId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gVcapModuleContext.captureId);
    }

    if(gVcapModuleContext.nullSrcId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gVcapModuleContext.nullSrcId);
    }

    if(gVcapModuleContext.capSwMsId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gVcapModuleContext.capSwMsId);
    }

    for(linkId = 0; linkId < MAX_IPC_FRAMES_LINK; linkId++)
    {
        if(gVcapModuleContext.ipcFramesOutVpssId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
           System_linkStop(gVcapModuleContext.ipcFramesOutVpssId[linkId]);
        }
        if(gVcapModuleContext.ipcFramesInDspId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gVcapModuleContext.ipcFramesInDspId[linkId]);
        }
    }

    for(linkId = 0;linkId < MAX_DEI_LINK;linkId++)
    {
        if(gVcapModuleContext.deiId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gVcapModuleContext.deiId[linkId]);
        }
    }

    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        if(gVcapModuleContext.nsfId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gVcapModuleContext.nsfId[linkId]);
        }
    }

    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        if(gVcapModuleContext.sclrId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gVcapModuleContext.sclrId[linkId]);
        }
    }

    if(gVcapModuleContext.ipcFramesOutVpssToHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gVcapModuleContext.ipcFramesOutVpssToHostId);
    }

    if(gVcapModuleContext.ipcFramesInHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gVcapModuleContext.ipcFramesInHostId);
    }

    if(gVcapModuleContext.ipcBitsInHLOSId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gVcapModuleContext.ipcBitsInHLOSId);
    }

    return 0;
}


/*
    Select the channel in capture link for whom extra buffers are needed
*/
Int32 Vcap_setExtraFramesChId(UInt32 chId)
{
    CaptureLink_ExtraFramesChId  prm;
    Int32 status;

    /* currently in all McFW use-case only QUE0 of capture is used */
    prm.queId = 0;
    prm.chId  = chId;

    status = System_linkControl(
            gVcapModuleContext.captureId,
            CAPTURE_LINK_CMD_SET_EXTRA_FRAMES_CH_ID,
            &prm,
            sizeof(prm),
            TRUE
            );

    return status;
}

/**
 * \brief:
 *      Get capture channels enabled
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       Number of capture channels
*/
Int32 Vcap_getNumChannels(Void)
{
    return gVcapModuleContext.vcapConfig.numChn;
}

Int32 Vcap_setParamDevice(VCAP_DEV vcDevId, VCAP_DEV_PARAM_S *psvcDevParam, VCAP_PARAMS_E paramId)
{
    memcpy(&gVcapModuleContext.vcapConfig.deviceParams[vcDevId], psvcDevParam, sizeof(VCAP_DEV_PARAM_S));
    return 0;
}


Int32 Vcap_getParamDevice(VCAP_DEV vcDevId, VCAP_DEV_PARAM_S *psvcDevParam, VCAP_PARAMS_E paramId)
{
    memcpy(psvcDevParam,&gVcapModuleContext.vcapConfig.deviceParams[vcDevId],sizeof(VCAP_DEV_PARAM_S));
    return 0;
}

Int32 Vcap_enableDevice(VCAP_DEV vcDevId)
{
    return 0;
}

Int32 Vcap_disableDevice(VCAP_DEV vcDevId)
{
    return 0;
}

Int32 Vcap_setParamChn(VCAP_CHN vcChnId, VCAP_CHN_PARAM_S *psCapChnParam, VCAP_PARAMS_E paramId)
{
    memcpy(&gVcapModuleContext.vcapConfig.channelParams[vcChnId], psCapChnParam, sizeof(VCAP_CHN_PARAM_S));
    return 0;
}

Int32 Vcap_getParamChn(VCAP_CHN vcChnId, VCAP_CHN_PARAM_S *psCapChnParam, VCAP_PARAMS_E paramId)
{
    memcpy(psCapChnParam, &gVcapModuleContext.vcapConfig.channelParams[vcChnId], sizeof(VCAP_CHN_PARAM_S));
    return 0;
}

Int32 Vcap_setDynamicParamChn(VCAP_CHN vcChnId, VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam, VCAP_PARAMS_E paramId)
{
    Int32 status = ERROR_NONE;

#ifdef SYSTEM_USE_VIDEO_DECODER
    CaptureLink_ColorParams vcapColor = {0};
#endif

    DeiLink_chDynamicSetOutRes params = {0};
    SclrLink_chDynamicSetOutRes prms = {0};
    UInt32 deiId = 0;
    UInt32 scdAlgLinkId;


#ifdef TI_816X_BUILD
    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
         scdAlgLinkId = gVcapModuleContext.dspAlgId[1];
    else
#endif
         scdAlgLinkId = gVcapModuleContext.dspAlgId[0];


    if(gVcapModuleContext.captureId==SYSTEM_LINK_ID_INVALID)
        return ERROR_FAIL;

    switch(paramId)
    {
        case VCAP_FORMAT:
            break;
        case VCAP_RESOLUTION:            
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
            if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
            {
                return MultiCh_progressive16ChVcapVencVdecVdis_setCapDynamicParamChn(vcChnId, psCapChnDynaParam, paramId);
            }
            if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
            {
                return MultiCh_progressive16ChNrtVcapVencVdecVdisSetOutRes(vcChnId, psCapChnDynaParam);
            }            
#endif
#if defined(TI_8107_BUILD)
            if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH)
            {
                return MultiCh_progressive8ChVcapVencVdecVdis_setCapDynamicParamChn(vcChnId, psCapChnDynaParam, paramId);
            }
#endif
            if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
            {
                if(psCapChnDynaParam->chDynamicRes.pathId == 2) /* Secondary Encode Stream Channel */
                {
                    prms.chId = vcChnId;
                    status = System_linkControl(
                                                gVcapModuleContext.sclrId[0],
                                                SCLR_LINK_CMD_GET_OUTPUTRESOLUTION,
                                                &(prms),
                                                sizeof(prms),
                                                TRUE
                                                );
                    prms.width = SystemUtils_align(psCapChnDynaParam->chDynamicRes.width, 16);
                    prms.height = SystemUtils_align(psCapChnDynaParam->chDynamicRes.height, 1);
                    status = System_linkControl(
                                                gVcapModuleContext.sclrId[0],
                                                SCLR_LINK_CMD_SET_OUTPUTRESOLUTION,
                                                &(prms),
                                                sizeof(prms),
                                                TRUE
                                                );
                }
                else if(psCapChnDynaParam->chDynamicRes.pathId == 3) /* MJPEG Encode Stream Channel */
                {
                    prms.chId = vcChnId;
                    status = System_linkControl(
                                                gVcapModuleContext.sclrId[1],
                                                SCLR_LINK_CMD_GET_OUTPUTRESOLUTION,
                                                &(prms),
                                                sizeof(prms),
                                                TRUE
                                                );
                    prms.width = SystemUtils_align(psCapChnDynaParam->chDynamicRes.width, 16);
                    prms.height = SystemUtils_align(psCapChnDynaParam->chDynamicRes.height, 1);
                    status = System_linkControl(
                                                gVcapModuleContext.sclrId[1],
                                                SCLR_LINK_CMD_SET_OUTPUTRESOLUTION,
                                                &(prms),
                                                sizeof(prms),
                                                TRUE
                                                );
                }
                else
                {
                    deiId = (vcChnId < 2)? 0: 1;
                    params.chId = vcChnId;
                    if(psCapChnDynaParam->chDynamicRes.pathId == 1)
                        params.queId = 1;
                    else
                        params.queId = 0;
                    status = System_linkControl(
                                                gVcapModuleContext.deiId[deiId],
                                                DEI_LINK_CMD_GET_OUTPUTRESOLUTION,
                                                &(params),
                                                sizeof(params),
                                                TRUE
                                                );
                    params.width = SystemUtils_align(psCapChnDynaParam->chDynamicRes.width, 16);
                    params.height = SystemUtils_align(psCapChnDynaParam->chDynamicRes.height, 1);
                    status = System_linkControl(
                                                gVcapModuleContext.deiId[deiId],
                                                DEI_LINK_CMD_SET_OUTPUTRESOLUTION,
                                                &(params),
                                                sizeof(params),
                                                TRUE

                                                );
                }


            }
            else
            {
               params.chId = vcChnId;
               params.queId = (UInt32)psCapChnDynaParam->chDynamicRes.pathId;

#ifdef TI_816X_BUILD
               deiId = (vcChnId < 8)? 0: 1;
               params.chId = (vcChnId >= 8)? (vcChnId-8): vcChnId;
#endif
               if(((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_VCAP_VENC)
                   ||(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH))
                   &&(params.queId == 2)) /* secondary channel, control SC5 for enable/disable */
               {
                prms.chId = vcChnId;
                status = System_linkControl(
                                           gVcapModuleContext.sclrId[0],
                                           SCLR_LINK_CMD_GET_OUTPUTRESOLUTION,
                                           &(prms),
                                           sizeof(prms),
                                           TRUE
                                           );
                prms.width = SystemUtils_align(psCapChnDynaParam->chDynamicRes.width, 16);
                prms.height = SystemUtils_align(psCapChnDynaParam->chDynamicRes.height, 1);
                status = System_linkControl(
                                           gVcapModuleContext.sclrId[0],
                                           SCLR_LINK_CMD_SET_OUTPUTRESOLUTION,
                                           &(prms),
                                           sizeof(prms),
                                           TRUE
                                           );
               }
               else
               {
                status = System_linkControl(
                                            gVcapModuleContext.deiId[deiId],
                                            DEI_LINK_CMD_GET_OUTPUTRESOLUTION,
                                            &(params),
                                            sizeof(params),
                                            TRUE
                                            );
                params.width = SystemUtils_align(psCapChnDynaParam->chDynamicRes.width, 16);
                params.height = SystemUtils_align(psCapChnDynaParam->chDynamicRes.height, 1);
                status = System_linkControl(
                                            gVcapModuleContext.deiId[deiId],
                                            DEI_LINK_CMD_SET_OUTPUTRESOLUTION,
                                            &(params),
                                            sizeof(params),
                                            TRUE
                                            );
               }
            }
            break;
        case VCAP_CONTRAST:
#ifdef SYSTEM_USE_VIDEO_DECODER
            vcapColor.contrast = (psCapChnDynaParam->contrast);
            vcapColor.chId     =  vcChnId;
            status = System_linkControl(
                                        gVcapModuleContext.captureId,
                                        CAPTURE_LINK_CMD_CHANGE_CONTRAST,
                                        &(vcapColor),
                                        sizeof(vcapColor),
                                        TRUE
                                        );
#else
            status = Vcap_setColor(psCapChnDynaParam->contrast,
                          gVcapModuleContext.colorPrm.videoBrightness,
                          gVcapModuleContext.colorPrm.videoSaturation,
                          gVcapModuleContext.colorPrm.videoHue,
                          vcChnId);
#endif
            break;
        case VCAP_SATURATION:
#ifdef SYSTEM_USE_VIDEO_DECODER
            vcapColor.satauration = (psCapChnDynaParam->satauration);
            vcapColor.chId     =  vcChnId;
            status = System_linkControl(
                                        gVcapModuleContext.captureId,
                                        CAPTURE_LINK_CMD_CHANGE_SATURATION,
                                        &(vcapColor),
                                        sizeof(vcapColor),
                                        TRUE
                                        );
#else
            status = Vcap_setColor(
                          gVcapModuleContext.colorPrm.videoContrast,
                          gVcapModuleContext.colorPrm.videoBrightness,
                          psCapChnDynaParam->satauration,
                          gVcapModuleContext.colorPrm.videoHue,
                          vcChnId);

#endif
        break;
        case VCAP_BRIGHTNESS:
#ifdef SYSTEM_USE_VIDEO_DECODER
            vcapColor.brightness = (psCapChnDynaParam->brightness);
            vcapColor.chId     =  vcChnId;
            status = System_linkControl(
                                        gVcapModuleContext.captureId,
                                        CAPTURE_LINK_CMD_CHANGE_BRIGHTNESS,
                                        &(vcapColor),
                                        sizeof(vcapColor),
                                        TRUE
                                        );
#else
            status = Vcap_setColor(
                          gVcapModuleContext.colorPrm.videoContrast,
                          psCapChnDynaParam->brightness,
                          gVcapModuleContext.colorPrm.videoSaturation,
                          gVcapModuleContext.colorPrm.videoHue,
                          vcChnId);
#endif
            break;
        case VCAP_HUE:
#ifdef SYSTEM_USE_VIDEO_DECODER
            vcapColor.hue = (psCapChnDynaParam->hue);
            vcapColor.chId     =  vcChnId;
            status = System_linkControl(
                                        gVcapModuleContext.captureId,
                                        CAPTURE_LINK_CMD_CHANGE_HUE,
                                        &(vcapColor),
                                        sizeof(vcapColor),
                                        TRUE
                                        );
#else
            status = Vcap_setColor(
                          gVcapModuleContext.colorPrm.videoContrast,
                          gVcapModuleContext.colorPrm.videoBrightness,
                          gVcapModuleContext.colorPrm.videoSaturation,
                          psCapChnDynaParam->hue,
                          vcChnId);

#endif
            break;
        case VCAP_PORTMODE:
            break;
        case VCAP_SIGNALTYPE:
            break;
        case VCAP_OSDWINPRM:
            status = System_linkControl(
                                        gVcapModuleContext.dspAlgId[0],
                                        ALG_LINK_OSD_CMD_SET_CHANNEL_WIN_PRM,
                                        psCapChnDynaParam->osdChWinPrm,
                                        sizeof(AlgLink_OsdChWinParams),
                                        TRUE
                                        );
            break;
        case VCAP_OSDBLINDWINPRM:
            #if defined(TI_8107_BUILD)
            status = System_linkControl(
                                        gVcapModuleContext.dspAlgId[0],
                                        ALG_LINK_OSD_CMD_SET_CHANNEL_BLIND_WIN_PRM,
                                        psCapChnDynaParam->osdChBlindWinPrm,
                                        sizeof(AlgLink_OsdChBlindWinParams),
                                        TRUE
                                        );
            #endif
            break;

        case VCAP_SCDMODE:
            status = System_linkControl(
                                        scdAlgLinkId,
                                        ALG_LINK_SCD_CMD_SET_CHANNEL_MODE,
                                        &(psCapChnDynaParam->scdChPrm),
                                        sizeof(psCapChnDynaParam->scdChPrm),
                                        TRUE
                                        );
            break;
        case VCAP_SCDSENSITIVITY:
            status = System_linkControl(
                                        scdAlgLinkId,
                                        ALG_LINK_SCD_CMD_SET_CHANNEL_SENSITIVITY,
                                        &(psCapChnDynaParam->scdChPrm),
                                        sizeof(psCapChnDynaParam->scdChPrm),
                                        TRUE
                                        );
            break;

        case VCAP_IGNORELIGHTSOFF:
            status = System_linkControl(
                                        scdAlgLinkId,
                                        ALG_LINK_SCD_CMD_SET_CHANNEL_IGNORELIGHTSOFF,
                                        &(psCapChnDynaParam->scdChPrm),
                                        sizeof(psCapChnDynaParam->scdChPrm),
                                        TRUE
                                        );
            break;
        case VCAP_SCDBLOCKCONFIG:
                status = System_linkControl(
                                            scdAlgLinkId,
                                            ALG_LINK_SCD_CMD_SET_CHANNEL_BLOCKCONFIG,
                                            &(psCapChnDynaParam->scdChBlkPrm),
                                            sizeof(psCapChnDynaParam->scdChBlkPrm),
                                            TRUE
                                            );
            break;

        case VCAP_IGNORELIGHTSON:
            status = System_linkControl(
                                        scdAlgLinkId,
                                        ALG_LINK_SCD_CMD_SET_CHANNEL_IGNORELIGHTSON,
                                        &(psCapChnDynaParam->scdChPrm),
                                        sizeof(psCapChnDynaParam->scdChPrm),
                                        TRUE
                                        );
            break;
        case VCAP_BLINDAREACONFIG:
            status = System_linkControl(
                                        gVcapModuleContext.captureId,
                                        CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA,
                                        &(psCapChnDynaParam->captureBlindInfo),
                                        sizeof(psCapChnDynaParam->captureBlindInfo),
                                        TRUE
                                        );
            break;
        case VCAP_ALL:
#ifdef SYSTEM_USE_VIDEO_DECODER
            vcapColor.contrast = (psCapChnDynaParam->contrast);
            vcapColor.chId     =  vcChnId;
            status = System_linkControl(
                                        gVcapModuleContext.captureId,
                                        CAPTURE_LINK_CMD_CHANGE_CONTRAST,
                                        &(vcapColor),
                                        sizeof(vcapColor),
                                        TRUE
                                        );

            if (status >= 0){
            vcapColor.satauration = (psCapChnDynaParam->satauration);
            vcapColor.chId     =  vcChnId;
                status = System_linkControl(
                                            gVcapModuleContext.captureId,
                                            CAPTURE_LINK_CMD_CHANGE_SATURATION,
                                            &(vcapColor),
                                            sizeof(vcapColor),
                                            TRUE
                                            );
            }
            if (status >= 0){
            vcapColor.brightness = (psCapChnDynaParam->brightness);
            vcapColor.chId     =  vcChnId;
                status = System_linkControl(
                                            gVcapModuleContext.captureId,
                                            CAPTURE_LINK_CMD_CHANGE_BRIGHTNESS,
                                            &(vcapColor),
                                            sizeof(vcapColor),
                                            TRUE
                                          );
            }
            if (status >= 0){
            vcapColor.hue = (psCapChnDynaParam->hue);
            vcapColor.chId     =  vcChnId;
                status = System_linkControl(
                                            gVcapModuleContext.captureId,
                                            CAPTURE_LINK_CMD_CHANGE_HUE,
                                            &(vcapColor),
                                            sizeof(vcapColor),
                                            TRUE
                                          );
            }
#else
            status = Vcap_setColor(
                          psCapChnDynaParam->contrast,
                          psCapChnDynaParam->brightness,
                          psCapChnDynaParam->satauration,
                          psCapChnDynaParam->hue,
                          vcChnId);
#endif
            break;
        default:
            break;

    }

    return status;
}

Int32 Vcap_setAudioModeParam(UInt32 numChannels, UInt32 samplingHz,UInt32 audioVolume)
{
#ifdef SYSTEM_USE_VIDEO_DECODER
    Int32 status = ERROR_NONE;
    Capture_AudioModeParams audioModeParams;


    audioModeParams.samplingHz = samplingHz;
    audioModeParams.audioVolume = audioVolume;
    audioModeParams.numAudioChannels = numChannels;

    /* Fixed Setting for  TVP5158*/
    audioModeParams.deviceNum = 0;
    audioModeParams.masterModeEnable    = 1;
    audioModeParams.dspModeEnable       = 0;
    audioModeParams.ulawEnable          = 0;
    audioModeParams.deviceNum           = 0;

    status = System_linkControl(
                 gVcapModuleContext.captureId,
                 CAPTURE_LINK_CMD_SET_AUDIO_CODEC_PARAMS,
                 &audioModeParams,
                 sizeof(audioModeParams),
                 TRUE);
    return status;
#else
    UInt32 i =0;
    Int32 status = ERROR_NONE;
    static Int32 tvpCascadedStage =0;
    Device_Tvp5158AudioModeParams audArgs;
    Device_VideoDecoderCreateStatus      createStatusArgs;

    /* Fixed Setting for  TVP5158*/
    audArgs.deviceNum           = 0; /* Should not changed */
    audArgs.samplingHz          = samplingHz;
    audArgs.masterModeEnable    = 1;
    audArgs.dspModeEnable       = 0;
    audArgs.ulawEnable          = 0;
    audArgs.audioVolume         = audioVolume;//High value will create gliches and noise in Captured Audio
    audArgs.numAudioChannels    = numChannels;

    for(i=0;i<(numChannels/VCAP_MAX_CHN_PER_DEVICE);i++)
    {
        switch (numChannels)
        {
            case 4:
            {
                audArgs.tdmChannelNum       = 1; // pDrvState->audiohwPortProperties.tdmChannelNum;
                audArgs.cascadeStage        = 0;
            }
            break;
            case 8:
            {
                audArgs.tdmChannelNum       = 2; // pDrvState->audiohwPortProperties.tdmChannelNum;
                audArgs.cascadeStage        = tvpCascadedStage;
                tvpCascadedStage++; /* In case of Cascading the cascadeStage  should be incremented by 1 */
            }
            break;
            case 16:
#if TI_816X_BUILD//This has to be enabled when 16 channel audio cascading is supported in the board
            {
                audArgs.tdmChannelNum       = 4; // pDrvState->audiohwPortProperties.tdmChannelNum;
                audArgs.cascadeStage        = tvpCascadedStage;
                tvpCascadedStage++; /* In case of Cascading the cascadeStage  should be incremented by 1 */
            }
#else
            {
                audArgs.tdmChannelNum       = 1; // pDrvState->audiohwPortProperties.tdmChannelNum;
                audArgs.cascadeStage        = 0;
            }
#endif
            break;
            default:
                printf("\n Audio Channels are not supported\n");
            break;
        }

        status = Device_tvp5158Control(gVcapModuleContext.tvp5158Handle[i],
                                       IOCTL_DEVICE_TVP5158_SET_AUDIO_MODE,
                                       &audArgs,
                                       &createStatusArgs);
        printf("CAPTURE: Audio configuration done... - status %d!!!\n",
                   status);
    }
    return status;

#endif

}

/**
 * \brief:
 *      Get capture Channel dynamic parameters
 * \input:
 *      vcChnId             -- capture Channel id
 *      psCapChnDynaParam   -- Device dynamic parameter structure
 * \output:
 *      NA
 * \return
*       ERROR_NONE    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vcap_getDynamicParamChn(VCAP_CHN vcChnId, VCAP_CHN_DYNAMIC_PARAM_S *psCapChnDynaParam, VCAP_PARAMS_E paramId)
{
    Int32 status = ERROR_NONE;

    DeiLink_chDynamicSetOutRes params = {0};
    SclrLink_chDynamicSetOutRes prms = {0};
    UInt32 deiId = 0;
    UInt32 scdAlgLinkId;

#ifdef TI_816X_BUILD
    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
         scdAlgLinkId = gVcapModuleContext.dspAlgId[1];
    else
#endif
         scdAlgLinkId = gVcapModuleContext.dspAlgId[0];


    if(gVcapModuleContext.captureId==SYSTEM_LINK_ID_INVALID)
        return ERROR_FAIL;

    switch(paramId)
    {
        case VCAP_RESOLUTION:
            #if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
            if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
            {
                return MultiCh_progressive16ChVcapVencVdecVdis_getCapDynamicParamChn(vcChnId, psCapChnDynaParam, paramId);
            }
            if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
            {
                return MultiCh_progressive16ChNrtVcapVencVdecVdisGetOutRes(vcChnId, psCapChnDynaParam);
            }
            #endif
#if defined(TI_8107_BUILD)
            if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH)
            {
                return MultiCh_progressive8ChVcapVencVdecVdis_getCapDynamicParamChn(vcChnId, psCapChnDynaParam, paramId);
            }
#endif
            params.chId = vcChnId;
            params.queId = (UInt32)psCapChnDynaParam->chDynamicRes.pathId;
            #ifdef TI_816X_BUILD
            deiId = (vcChnId < 8)? 0: 1;
            params.chId = (vcChnId >= 8)? (vcChnId-8): vcChnId;
            #endif
            if(((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_VCAP_VENC)
                ||(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH))
                &&(params.queId == 2)) /* secondary channel, control SC5 for enable/disable */
            {
                prms.chId = vcChnId;
                status = System_linkControl(
                                       gVcapModuleContext.sclrId[0],
                                       SCLR_LINK_CMD_GET_OUTPUTRESOLUTION,
                                       &(prms),
                                       sizeof(prms),
                                       TRUE
                                       );
                psCapChnDynaParam->chDynamicRes.width = params.width;
                psCapChnDynaParam->chDynamicRes.height = params.height;
            }
            else
            {
                status = System_linkControl(
                                        gVcapModuleContext.deiId[deiId],
                                        DEI_LINK_CMD_GET_OUTPUTRESOLUTION,
                                        &(params),
                                        sizeof(params),
                                        TRUE
                                        );
                psCapChnDynaParam->chDynamicRes.width = params.width;
                psCapChnDynaParam->chDynamicRes.height = params.height;
            }
            break;

        case VCAP_SCDGETALLCHFRAMESTATUS:

            psCapChnDynaParam->scdAllChFrameStatus.numCh = 0;

            status = System_linkControl(
                                        scdAlgLinkId,
                                        ALG_LINK_SCD_CMD_GET_ALL_CHANNEL_FRAME_STATUS,
                                        &(psCapChnDynaParam->scdAllChFrameStatus),
                                        sizeof(psCapChnDynaParam->scdAllChFrameStatus),
                                        TRUE
                                        );
            break;

        default:
            status = ERROR_FAIL;
            break;
    }

    return status;
}


/**
 * \brief:
 *      Skip any specific FID type. This is an additional control in capture side itself; is really useful for secondary stream <CIF>.
 *      Stream 0 is D1 & Stream 1 is CIF.
 * \input:
 *      vcChnId             -- capture Channel id
 *      fidType             -- TOP/BOTTOM field
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vcap_skipFidType(VCAP_CHN vcChnId, Int32 fidType)
{
    Int32 status = ERROR_NONE;
    SclrLink_chDynamicSkipFidType params;

    params.chId = vcChnId;
    params.fidType = fidType;
    status = System_linkControl(
                                gVcapModuleContext.sclrId[0],
                                SCLR_LINK_CMD_SKIP_FID_TYPE,
                                &(params),
                                sizeof(params),
                                TRUE
                               );

    return status;
}

/**
 * \brief:
 *      Set capture frame rate. This is an additional control in capture side itself; is really useful for secondary stream <CIF>.
 *      Stream 0 is D1 & Stream 1 is CIF.
 * \input:
 *      vcChnId             -- capture Channel id
 *      vStrmId             -- Stream Id
 *      frameRate          -- Frame Rate
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vcap_setFrameRate(VCAP_CHN vcChnId, VCAP_STRM vStrmId, Int32 inputFrameRate, Int32 outputFrameRate)
{
    DeiLink_ChFpsParams params;
    SclrLink_ChFpsParams sclrParams;
    NsfLink_ChFpsParams nsfParams;
    Int32 status = ERROR_NONE;
    UInt32 noOfDEIChan,deiId;
    
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
    {
        return MultiCh_progressive16ChVcapVencVdecVdisSetCapFrameRate(vcChnId, vStrmId, inputFrameRate, outputFrameRate);
    }
    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
    {
        return MultiCh_progressive16ChNrtVcapVencVdecVdisSetCapFrameRate(vcChnId, vStrmId, inputFrameRate, outputFrameRate);
    }
#endif
#if defined(TI_8107_BUILD)
    if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH)
    {
        return MultiCh_progressive8ChVcapVencVdecVdisSetCapFrameRate(vcChnId, vStrmId, inputFrameRate, outputFrameRate);
    }
    if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT)
    {
        return MultiCh_progressive8ChNrtVcapVencVdecVdisSetCapFrameRate(vcChnId, vStrmId, inputFrameRate, outputFrameRate);
    }
#endif

    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
    {
      noOfDEIChan = 2;
      deiId = vcChnId/noOfDEIChan;
    }
    else
    {
       noOfDEIChan = DEI_LINK_MAX_CH;
       deiId = vcChnId/noOfDEIChan;
    }

    if(vcChnId>=255 || deiId >= MAX_DEI_LINK)
    {
        /* invalid parameter */
        return -1;
    }

    params.chId = vcChnId%noOfDEIChan;
    params.streamId = vStrmId;

    params.inputFrameRate = inputFrameRate;
    params.outputFrameRate = outputFrameRate;

    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
    {

        if ((vStrmId == 0) || (vStrmId == 1)) /* Primary stream and Frame to A8*/
        {
            if(gVcapModuleContext.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
            {
                status = System_linkControl(gVcapModuleContext.deiId[deiId], DEI_LINK_CMD_SET_FRAME_RATE,
                               &params, sizeof(params), TRUE);
            }
        }

        if (vStrmId == 2) /* secondary stream */
        {
            sclrParams.chId = vcChnId;
            sclrParams.inputFrameRate = inputFrameRate;
            sclrParams.outputFrameRate = outputFrameRate;

            status = System_linkControl(gVcapModuleContext.sclrId[0], SCLR_LINK_CMD_SET_FRAME_RATE,
                               &sclrParams, sizeof(sclrParams), TRUE);
        }
        if (vStrmId == 3)  /* MJPEG stream */
        {
            sclrParams.chId = vcChnId;
            sclrParams.inputFrameRate = inputFrameRate;
            sclrParams.outputFrameRate = outputFrameRate;

            status = System_linkControl(gVcapModuleContext.sclrId[1], SCLR_LINK_CMD_SET_FRAME_RATE,
                               &sclrParams, sizeof(sclrParams), TRUE);
        }

    }
    else
    {
        if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_VCAP_VENC)
           ||(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH))
        {
            if (vStrmId == 2) /* secondary stream */
            {
                sclrParams.chId = vcChnId;
                sclrParams.inputFrameRate = inputFrameRate;
                sclrParams.outputFrameRate = outputFrameRate;

                status = System_linkControl(gVcapModuleContext.sclrId[0], SCLR_LINK_CMD_SET_FRAME_RATE,
                                   &sclrParams, sizeof(sclrParams), TRUE);
                return status;
            }
            if (vStrmId == 3) /* MJPEG stream */
            {
                params.streamId = 2;
            }
        }

        if(gVcapModuleContext.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
        {
            status = System_linkControl(gVcapModuleContext.deiId[deiId], DEI_LINK_CMD_SET_FRAME_RATE,
                           &params, sizeof(params), TRUE);
   #if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
            if(vStrmId==0)
            {
                /* since JPEG stream is generated from preview stream, if preview stream
                    FPS changes we need to change the input FPS for JPEG stream
                    output FPS is still kept to 1

                    ONLY applicable for 4D1 814x progressive use-case
                 */
               if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
               {
                    if(gVcapModuleContext.nsfId[0]!=SYSTEM_LINK_ID_INVALID)
                    {
                        NsfLink_ChFpsParams nsfParams;

                        nsfParams.chId = params.chId;
                        nsfParams.inputFrameRate = params.outputFrameRate;
                        nsfParams.outputFrameRate = 1;

                        status = System_linkControl(gVcapModuleContext.nsfId[0], NSF_LINK_CMD_SET_FRAME_RATE,
                                   &nsfParams, sizeof(nsfParams), TRUE);

                    }
               }
            }
   #endif
        }

   }
   if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_INTERLACED_VCAP_VDIS_VENC_VDEC)

   {
        if(vStrmId == 0)
        {
            sclrParams.chId = vcChnId;

            sclrParams.inputFrameRate = inputFrameRate;
            sclrParams.outputFrameRate = outputFrameRate;
            status = System_linkControl(gVcapModuleContext.sclrId[0], SCLR_LINK_CMD_SET_FRAME_RATE,
                               &sclrParams, sizeof(sclrParams), TRUE);
        }

        if(vStrmId == 1)
        {
            nsfParams.chId = vcChnId;
            nsfParams.inputFrameRate  = inputFrameRate;
            nsfParams.outputFrameRate = outputFrameRate;
            status = System_linkControl(gVcapModuleContext.nsfId[0], NSF_LINK_CMD_SET_FRAME_RATE,
                           &nsfParams, sizeof(nsfParams), TRUE);
        }
   }
    return status;
}

/**
 * \brief:
 *      Get capture frame rate. Not available now
 * \input:
 *      vcChnId             -- capture Channel id
 *      vStrmId             -- Stream Id
 *      frameRate          -- Frame Rate
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vcap_getFrameRate(VCAP_CHN vcChnId, VCAP_STRM vStrmId)
{
    return 0;
}


Int32 Vcap_enableDisableChn(VCAP_CHN vcChnId, VCAP_STRM vcStrmId, Bool enableChn)
{
    char *onOffName[] = { "OFF ", "ON" };
    Int32 status = ERROR_FAIL;
    DeiLink_ChannelInfo channelInfo;
    SclrLink_ChannelInfo SclrchannelInfo;
    UInt32 cmdId, deiId;
    UInt32 noOfDEIChan;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
    {
        return MultiCh_progressive16ChVcapVencVdecVdis_enableDisableCapChn(vcChnId, vcStrmId, enableChn);
    }
    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT)
    {
        status = MultiCh_progressive16ChNrtVcapVencVdecVdisEnableDisableCapChn(vcChnId, vcStrmId, enableChn);
        if (status == ERROR_NONE)
            printf(" VCAP: CH%d STRM%d = [%s]\n", vcChnId, vcChnId, onOffName[enableChn]);
        return status;
    }
#endif
#if defined(TI_8107_BUILD)
    if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH)
    {
        return MultiCh_progressive8ChVcapVencVdecVdis_enableDisableCapChn(vcChnId, vcStrmId, enableChn);
    }
#endif
    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
    {
      noOfDEIChan = 2;
    }
    else
    {
      noOfDEIChan = DEI_LINK_MAX_CH;
    }

    channelInfo.channelId = vcChnId%noOfDEIChan;
    channelInfo.streamId  = vcStrmId;
    channelInfo.enable    = enableChn;

    deiId = vcChnId/noOfDEIChan;

    if(enableChn)
        cmdId = DEI_LINK_CMD_ENABLE_CHANNEL;
    else
        cmdId = DEI_LINK_CMD_DISABLE_CHANNEL;


    if(channelInfo.streamId == 2)
    {
        /* secondary channel, control SC5 for enable/disable */
        SclrchannelInfo.channelId = vcChnId;
        SclrchannelInfo.enable    = enableChn;

        if(enableChn)
            cmdId = SCLR_LINK_CMD_ENABLE_CHANNEL;
        else
            cmdId = SCLR_LINK_CMD_DISABLE_CHANNEL;

        if(gVcapModuleContext.sclrId[0]!=SYSTEM_LINK_ID_INVALID)
        {
            status = System_linkControl(
                                        gVcapModuleContext.sclrId[0],
                                        cmdId,
                                        &(SclrchannelInfo),
                                        sizeof(SclrLink_ChannelInfo),
                                        TRUE
                                        );
        }
    }
    else if(channelInfo.streamId == 3)
    {
        /* secondary channel, control SC5 for enable/disable */
        SclrchannelInfo.channelId = vcChnId;
        SclrchannelInfo.enable    = enableChn;

        if(enableChn)
            cmdId = SCLR_LINK_CMD_ENABLE_CHANNEL;
        else
            cmdId = SCLR_LINK_CMD_DISABLE_CHANNEL;

        if(gVcapModuleContext.sclrId[1]!=SYSTEM_LINK_ID_INVALID)
        {
            status = System_linkControl(
                                        gVcapModuleContext.sclrId[1],
                                        cmdId,
                                        &(SclrchannelInfo),
                                        sizeof(SclrLink_ChannelInfo),
                                        TRUE
                                        );
        }
    }
    else
    {
        if(deiId < MAX_DEI_LINK)
        {
            if(gVcapModuleContext.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
            {
                status = System_linkControl(
                                            gVcapModuleContext.deiId[deiId],
                                            cmdId,
                                            &(channelInfo),
                                            sizeof(DeiLink_ChannelInfo),
                                            TRUE
                                            );
            }
        }
    }

    printf(" VCAP: CH%d STRM%d = [%s]\n", vcChnId, vcStrmId, onOffName[enableChn]);

    return status;

}

Int32 Vcap_enableChn(VCAP_CHN vcChnId, VCAP_STRM vcStrmId)
{
    return Vcap_enableDisableChn(vcChnId, vcStrmId, TRUE);
}


Int32 Vcap_disableChn(VCAP_CHN vcChnId, VCAP_STRM vcStrmId)
{
    return Vcap_enableDisableChn(vcChnId, vcStrmId, FALSE);
}


Int32 Vcap_lock2DisplayChn( VCAP_CHN vcChnId,  VDIS_CHN vdChn) // need discuss for intput/output
{
    return 0;
}

Int32 Vcap_unLock2DisplayChn(VCAP_CHN vcChnId)
{
    return 0;
}

Int32 Vcap_getChnBufferBlocking(VCAP_CHN vcChnId, UInt8 *pChnBuffer, UInt32 uiTimeoutMs)       // consider later
{
    return 0;
}

Int32 Vcap_getChnBufferNonBlocking(VCAP_CHN vcChnId, UInt8 *pChnBuffer)        // consider later
{
    return 0;
}

Int32 Vcap_releaseChnBuffer(VCAP_CHN vcChnId, UInt8 *pChnBuffer)   // consider later
{
    return 0;
}

Int32 Vcap_delete()
{
    UInt32 linkId, i;
    /* delete can be done in any order */

    if(gVcapModuleContext.captureId!=SYSTEM_LINK_ID_INVALID)
        System_linkDelete(gVcapModuleContext.captureId);

    if(gVcapModuleContext.capSwMsId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gVcapModuleContext.capSwMsId);
    }

    for(linkId=0; linkId<MAX_IPC_FRAMES_LINK; linkId++)
    {
        if(gVcapModuleContext.ipcFramesOutVpssId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
           System_linkDelete(gVcapModuleContext.ipcFramesOutVpssId[linkId]);
        }

        if(gVcapModuleContext.ipcFramesInDspId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gVcapModuleContext.ipcFramesInDspId[linkId]);
        }
    }

    for(linkId=0; linkId<MAX_ALG_LINK; linkId++)
    {
        if(gVcapModuleContext.dspAlgId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gVcapModuleContext.dspAlgId[linkId]);
        }
    }

    for(linkId=0; linkId<MAX_DEI_LINK; linkId++)
    {
        if(gVcapModuleContext.deiId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gVcapModuleContext.deiId[linkId]);
    }

    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        if(gVcapModuleContext.nsfId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gVcapModuleContext.nsfId[linkId]);
    }

    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        if(gVcapModuleContext.sclrId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gVcapModuleContext.sclrId[linkId]);
    }

    if(gVcapModuleContext.ipcFramesOutVpssToHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gVcapModuleContext.ipcFramesOutVpssToHostId);
    }

    if(gVcapModuleContext.ipcFramesInHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gVcapModuleContext.ipcFramesInHostId);
    }
    if(gVcapModuleContext.ipcBitsInHLOSId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gVcapModuleContext.ipcBitsInHLOSId);
    }

    for(i = 0; i < VCAP_DEV_MAX; i++)
    {
        if(gVcapModuleContext.tvp5158Handle[i])
        {
            Device_tvp5158Delete(gVcapModuleContext.tvp5158Handle[i], NULL);
        }
    }

    return 0;
}

/**
 * \brief:
 *              register call back which will post the message
 * \input:
 *              callback                -- callback function
 * \output:
 *              NA
 * \return
*               TI_MEDIA_SUCCESS        --      while success
*               ERROR_CODE                      --      refer for err defination
*/
Int32 Vcap_registerCallback(VCAP_CALLBACK_S * callback, Ptr arg)
{
    gVcapModuleContext.callbackFxn = *callback;
    gVcapModuleContext.callbackArg = arg;

    return 0;
}

/**
 * \brief:
 *              register bits call back which will post the message
 * \input:
 *              callback                -- callback function
 * \output:
 *              NA
 * \return
*               TI_MEDIA_SUCCESS        --      while success
*               ERROR_CODE                      --      refer for err defination
*/
Int32 Vcap_registerBitsCallback(VCAP_CALLBACK_S * callback, Ptr arg)
{
    gVcapModuleContext.bitscallbackFxn = *callback;
    gVcapModuleContext.bitscallbackArg = arg;

    return 0;
}

/**
    \brief Returns Bits Callback info registered by the application

    \param callback            [OUT] Pointer to User specified callbacks

    \param arg                 [OUT] Pointer to Callback context

    \return ERROR_NONE on success
*/
Int32 Vcap_getBitsCallbackInfo(VCAP_CALLBACK_S ** callback, Ptr *arg)
{
    *callback = &gVcapModuleContext.bitscallbackFxn;
    *arg      = gVcapModuleContext.bitscallbackArg;

    return 0;
}

/**
 * \brief:
 *              MCFW layer call back which will invoke the app registered callback
 * \input:
 *              callback                -- callback function
 * \output:
 *              NA
 * \return
*               TI_MEDIA_SUCCESS        --      while success
*               ERROR_CODE                      --      refer for err defination
*/
Void Vcap_ipcFramesInCbFxn(Ptr cbCtx)
{
    OSA_assert(cbCtx == &gVcapModuleContext);

    VCAP_TRACE_FXN_ENTRY("Vcap_ipcFramesInCbFxn");

    if (gVcapModuleContext.callbackFxn.newDataAvailableCb)
    {
        gVcapModuleContext.callbackFxn.newDataAvailableCb(gVcapModuleContext.callbackArg);
    }
}

static Void Vcap_copyVidFrameInfoLink2McFw(VIDEO_FRAMEBUF_S *dstBuf,
                                           VIDFrame_Buf    *srcBuf)
{
    Int i,j;
    OSA_assert(VIDEO_MAX_FIELDS == VIDFRAME_MAX_FIELDS);
    OSA_assert(VIDEO_MAX_PLANES == VIDFRAME_MAX_PLANES);

    for (i = 0; i < VIDEO_MAX_FIELDS; i++)
    {
        for (j = 0; j < VIDEO_MAX_PLANES; j++)
        {
            dstBuf->addr[i][j] = srcBuf->addr[i][j];
            dstBuf->phyAddr[i][j] = srcBuf->phyAddr[i][j];
        }
    }
    dstBuf->channelNum  = srcBuf->channelNum;
    dstBuf->fid         = srcBuf->fid;
    dstBuf->frameWidth  = srcBuf->frameWidth;
    dstBuf->frameHeight = srcBuf->frameHeight;
    dstBuf->linkPrivate = srcBuf->linkPrivate;
    dstBuf->timeStamp   = srcBuf->timeStamp;
    dstBuf->framePitch[0] = srcBuf->framePitch[0];
    dstBuf->framePitch[1] = srcBuf->framePitch[1];

    VCAP_TRACE_FXN_EXIT("VidFrameInfo:"
                         "virt[0][0]:%p,"
                         "phy[0][0]:%p,"
                         "channelNum:%d,"
                         "fid:%d,"
                         "frameWidth:%d,"
                         "frameHeight:%d,"
                         "framePitch[0]:%d,"
                         "framePitch[1]:%d,"
                         "timeStamp:%d,",
                         dstBuf->addr[0][0],
                         dstBuf->phyAddr[0][0],
                         dstBuf->channelNum,
                         dstBuf->fid,
                         dstBuf->frameWidth,
                         dstBuf->frameHeight,
                         dstBuf->framePitch[0],
                         dstBuf->framePitch[1],
                         dstBuf->timeStamp);
}

static Void Vcap_copyVidFrameInfoMcFw2Link(VIDFrame_Buf *dstBuf,
                                           VIDEO_FRAMEBUF_S    *srcBuf)
{
    Int i,j;
    OSA_assert(VIDEO_MAX_FIELDS == VIDFRAME_MAX_FIELDS);
    OSA_assert(VIDEO_MAX_PLANES == VIDFRAME_MAX_PLANES);

    for (i = 0; i < VIDEO_MAX_FIELDS; i++)
    {
        for (j = 0; j < VIDEO_MAX_PLANES; j++)
        {
            dstBuf->addr[i][j] = srcBuf->addr[i][j];
            dstBuf->phyAddr[i][j] = srcBuf->phyAddr[i][j];
        }
    }
    dstBuf->channelNum  = srcBuf->channelNum;
    dstBuf->fid         = srcBuf->fid;
    dstBuf->frameWidth  = srcBuf->frameWidth;
    dstBuf->frameHeight = srcBuf->frameHeight;
    dstBuf->linkPrivate = srcBuf->linkPrivate;
    dstBuf->timeStamp   = srcBuf->timeStamp;
    dstBuf->framePitch[0] = srcBuf->framePitch[0];
    dstBuf->framePitch[1] = srcBuf->framePitch[1];

    VCAP_TRACE_FXN_EXIT("VidFrameInfo:"
                         "virt[0][0]:%p,"
                         "phy[0][0]:%p,"
                         "channelNum:%d,"
                         "fid:%d,"
                         "frameWidth:%d,"
                         "frameHeight:%d,"
                         "framePitch[0]:%d,"
                         "framePitch[1]:%d,"
                         "timeStamp:%d,",
                         dstBuf->addr[0][0],
                         dstBuf->phyAddr[0][0],
                         dstBuf->channelNum,
                         dstBuf->fid,
                         dstBuf->frameWidth,
                         dstBuf->frameHeight,
                         dstBuf->framePitch[0],
                         dstBuf->framePitch[1],
                         dstBuf->timeStamp);
}

static Void Vcap_copyBitBufInfoLink2McFw(VALG_FRAMERESULTBUF_S *dstBuf,
                                         Bitstream_Buf    *srcBuf)
{
    dstBuf->reserved       = (UInt32)srcBuf;
    dstBuf->bufVirtAddr    = srcBuf->addr;
    dstBuf->bufSize        = srcBuf->bufSize;
    dstBuf->chnId          = srcBuf->channelNum;
    dstBuf->filledBufSize  = srcBuf->fillLength;
    dstBuf->timestamp      = srcBuf->timeStamp;
    dstBuf->upperTimeStamp = srcBuf->upperTimeStamp;
    dstBuf->lowerTimeStamp = srcBuf->lowerTimeStamp;
    dstBuf->bufPhysAddr    = (Void *)srcBuf->phyAddr;
    dstBuf->frameWidth     = srcBuf->frameWidth;
    dstBuf->frameHeight    = srcBuf->frameHeight;

    VCAP_TRACE_FXN_EXIT("BitBufInfo:"
                         "virt:%p,"
                         "bufSize:%d,"
                         "chnId:%d,"
                         "filledBufSize:%d,"
                         "timeStamp:%d,"
                         "phy:%p,"
                         "width:%d"
                         "height:%d",
                         dstBuf->bufVirtAddr,
                         dstBuf->bufSize,
                         dstBuf->chnId,
                         dstBuf->filledBufSize,
                         dstBuf->timestamp,
                         dstBuf->bufPhysAddr,
                         dstBuf->frameWidth,
                         dstBuf->frameHeight);
}

/**
    \brief Get encoded buffers from McFW

    \param pBitsBufList [OUT]   List of Bistream Buffers returned by the function
    \param timeout      [IN]    VSYS_WAIT_FOREVER or VSYS_NO_WAIT or timeout in units of msec

    \return SUCCESS or FAIL
 */
Int32 Vcap_getAlgResultBuffer(VALG_FRAMERESULTBUF_LIST_S *pBitsBufList, UInt32 timeout)
{
    Bitstream_BufList ipcBufList;
    Bitstream_Buf *pInBuf;
    VALG_FRAMERESULTBUF_S *pOutBuf;
    UInt32 i;

    VCAP_TRACE_FXN_ENTRY();
    pBitsBufList->numBufs = 0;
    ipcBufList.numBufs = 0;

    IpcBitsInLink_getFullVideoBitStreamBufs(gVcapModuleContext.ipcBitsInHLOSId,
                                            &ipcBufList);

    pBitsBufList->numBufs = ipcBufList.numBufs;
    for (i = 0; i < ipcBufList.numBufs; i++)
    {
        pOutBuf = &pBitsBufList->bitsBuf[i];
        pInBuf = ipcBufList.bufs[i];

        Vcap_copyBitBufInfoLink2McFw(pOutBuf,pInBuf);
    }

    VCAP_TRACE_FXN_EXIT("NumBufs Received:%d",pBitsBufList->numBufs);
    return 0;
}
/**
    \brief Release encoded buffers to McFW

    Buffers returned by Vcap_getBitstreamBuffer() are returned to the framework
    for resue after user is done using the encoded bitstreams

    \param pBitsBufList [IN]   List of Bistream Buffers

    \return SUCCESS or FAIL
 */
Int32 Vcap_releaseAlgResultBuffer(VALG_FRAMERESULTBUF_LIST_S *pBitsBufList)
{
    VALG_FRAMERESULTBUF_S *pOutBuf;
    Bitstream_BufList ipcBufList;
    UInt32 i;
    Int status = 0;

    VCAP_TRACE_FXN_ENTRY("Num bufs released:%d",pBitsBufList->numBufs);
    ipcBufList.numBufs = pBitsBufList->numBufs;
    for (i = 0; i < ipcBufList.numBufs; i++)
    {
        pOutBuf = &pBitsBufList->bitsBuf[i];
        ipcBufList.bufs[i] = (Bitstream_Buf*)pBitsBufList->bitsBuf[i].reserved;
    }
    if (ipcBufList.numBufs)
    {
        status =
        IpcBitsInLink_putEmptyVideoBitStreamBufs(gVcapModuleContext.ipcBitsInHLOSId,
                                                 &ipcBufList);
    }
    VCAP_TRACE_FXN_ENTRY("Buf release status:%d",status);
    return 0;
}


/**
    \brief Request filled video buffers from framework

    User calls this API to get full video frames from the framework.
    After getting the video frames, user will
    - consume the video frames
    - and then call Vcap_putEmptyVideoFrames() to free the video frames back to the framework

    \param pFrameBufList    [OUT]  List of video frames returned by the framework
    \param timeout          [IN]   TIMEOUT_WAIT_FOREVER or TIMEOUT_NO_WAIT or timeout in msecs

    \return ERROR_NONE on success
*/
Int32 Vcap_getFullVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList, UInt32 timeout)
{
    VIDFrame_BufList  vidBufList;
    VIDFrame_Buf     *pInBuf;
    VIDEO_FRAMEBUF_S *pOutBuf;
    UInt32 i;

    VCAP_TRACE_FXN_ENTRY();
    pFrameBufList->numFrames = 0;
    vidBufList.numFrames = 0;
    IpcFramesInLink_getFullVideoFrames(gVcapModuleContext.ipcFramesInHostId,
                                       &vidBufList);

    pFrameBufList->numFrames = vidBufList.numFrames;
    for (i = 0; i < vidBufList.numFrames; i++)
    {
        pOutBuf = &pFrameBufList->frames[i];
        pInBuf = &vidBufList.frames[i];

        Vcap_copyVidFrameInfoLink2McFw(pOutBuf,pInBuf);
    }

    VCAP_TRACE_FXN_EXIT("NumFrames Received:%d",pFrameBufList->numFrames);
    return 0;
}
/**
    \brief Give consumed video frames back to the application to be freed

    Buffers that are were previously got from Vcap_getFullVideoFrames can be
    freed back to the framework by invoking this API.

    \param pFrameBufList [IN]   List of video frames

    \return ERROR_NONE on success
*/
Int32 Vcap_putEmptyVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList)
{
    VIDEO_FRAMEBUF_S *pSrcBuf;
    VIDFrame_Buf     *pDstBuf;
    VIDFrame_BufList  vidBufList;
    UInt32 i;
    Int status = 0;

    VCAP_TRACE_FXN_ENTRY("Num bufs released:%d",pFrameBufList->numFrames);
    vidBufList.numFrames = pFrameBufList->numFrames;
    for (i = 0; i < vidBufList.numFrames; i++)
    {
        pSrcBuf = &pFrameBufList->frames[i];
        pDstBuf = &vidBufList.frames[i];
        Vcap_copyVidFrameInfoMcFw2Link(pDstBuf,pSrcBuf);
    }
    if (vidBufList.numFrames)
    {
        status =
        IpcFramesInLink_putEmptyVideoFrames(gVcapModuleContext.ipcFramesInHostId,
                                            &vidBufList);
    }
    VCAP_TRACE_FXN_ENTRY("VIDFrame release status:%d",status);
    return 0;
}
/**
    \brief Detects video per channel at the video input.

    \param pFrameBufList [IN]   List of video frames

    \return ERROR_NONE on success
*/

Int32 Vcap_detectVideo()
{

    Int status = 0;

    if((   gVcapModuleContext.vcapConfig.enableConfigExtVideoDecoder)
       && (gVcapModuleContext.captureId!=SYSTEM_LINK_ID_INVALID))
    {
        status = System_linkControl(gVcapModuleContext.captureId,
                                CAPTURE_LINK_CMD_DETECT_VIDEO,
                                NULL,
                                0,
                                TRUE);
    }
    return status;

}

Int32 Vcap_getVideoSourceStatus(VCAP_VIDEO_SOURCE_STATUS_S *pStatus)
{
    Int32 status = 0;

    if(gVcapModuleContext.vcapConfig.enableConfigExtVideoDecoder)
    {
#ifdef SYSTEM_USE_VIDEO_DECODER
        if(gVcapModuleContext.captureId!=SYSTEM_LINK_ID_INVALID)
        {
            status = System_linkControl(gVcapModuleContext.captureId,
                                    CAPTURE_LINK_CMD_GET_VIDEO_STATUS,
                                    pStatus,
                                    sizeof(*pStatus),
                                    TRUE);

        }
#else

        VCAP_VIDEO_SOURCE_STATUS_PARAMS_S  videoStatusArgs;
        VCAP_VIDEO_SOURCE_CH_STATUS_S      videoStatus;
        UInt32 chId;

        Bool isVideoStatusChanged;

        VCAP_VIDEO_SOURCE_CH_STATUS_S *pVidStatus;

        isVideoStatusChanged = FALSE;

        for (chId = 0; chId < gVcapModuleContext.numChannels; chId++)
        {
            pVidStatus = &gVcapModuleContext.videoStatus.chStatus[chId];

            videoStatusArgs.channelNum = chId;

            if(gVcapModuleContext.tvp5158Handle[chId/VCAP_MAX_CHN_PER_DEVICE]==NULL)
                continue;

            status = Device_tvp5158Control(
                     gVcapModuleContext.tvp5158Handle[chId/VCAP_MAX_CHN_PER_DEVICE],
                     IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS,
                     &videoStatusArgs,
                     &videoStatus);

            if(videoStatus.isVideoDetect)
            {
                if(videoStatus.frameWidth != pVidStatus->frameWidth
                        ||
                    videoStatus.frameHeight != pVidStatus->frameHeight
                        ||
                    videoStatus.frameInterval != pVidStatus->frameInterval
                        ||
                    videoStatus.isInterlaced != pVidStatus->isInterlaced

                )
                {
                    isVideoStatusChanged = TRUE;

                    #if 0
                        printf
                        (" $$CAPTURE: Detected video at CH [%d] (%dx%d@%dHz, %d)!!!\n",
                         chId, videoStatus.frameWidth,
                         videoStatus.frameHeight,
                         1000000 / videoStatus.frameInterval,
                         videoStatus.isInterlaced);
                    #endif
                }

                pVidStatus->isVideoDetect = videoStatus.isVideoDetect;
                pVidStatus->frameWidth    = videoStatus.frameWidth;
                pVidStatus->frameHeight   = videoStatus.frameHeight;
                pVidStatus->frameInterval = videoStatus.frameInterval;
                pVidStatus->isInterlaced  = videoStatus.isInterlaced;
                pVidStatus->vipInstId = (chId/VCAP_MAX_CHN_PER_DEVICE);
                pVidStatus->chId = chId%VCAP_MAX_CHN_PER_DEVICE;
            }
            else
            {
                if(pVidStatus->isVideoDetect)
                {
                    /* video was previously detected and now is not detected */
                    #if 0
                     printf(" %d: CAPTURE: No video detected at CH [%d,%d] !!!\n",
                     Utils_getCurTimeInMsec(), instId, chId);
                    #endif

                    isVideoStatusChanged = TRUE;
                }

                pVidStatus->isVideoDetect = 0;
                pVidStatus->frameWidth = 0;
                pVidStatus->frameHeight = 0;
                pVidStatus->frameInterval = 0;
                pVidStatus->isInterlaced = 0;
                pVidStatus->vipInstId = (chId/VCAP_MAX_CHN_PER_DEVICE);
                pVidStatus->chId = chId%VCAP_MAX_CHN_PER_DEVICE;
            }
       }

        gVcapModuleContext.videoStatus.numChannels = gVcapModuleContext.numChannels;

#endif
    }

    if(pStatus != NULL)
    {
        memcpy(pStatus,
               &(gVcapModuleContext.videoStatus),
               sizeof(VCAP_VIDEO_SOURCE_STATUS_S));
    }

    return status;
}


Int32 Vcap_setVideoSourceStatus(VCAP_VIDEO_SOURCE_STATUS_S *pStatus)
{

    if(pStatus != NULL)
    {
        memcpy(&(gVcapModuleContext.videoStatus),
               pStatus,
               sizeof(VCAP_VIDEO_SOURCE_STATUS_S));

        /* Assumption here is height width of video remains same across channels */
        if(pStatus->chStatus[0].frameHeight == 288)
            gVcapModuleContext.isPalMode = TRUE;

    }

    return 0;
}

Int32 Vcap_configVideoDecoder(VCAP_VIDDEC_PARAMS_S * modeParams, UInt32 numDevices)
{
    Int32 status = 0;


    if(gVcapModuleContext.vcapConfig.enableConfigExtVideoDecoder)
    {
#ifdef SYSTEM_USE_VIDEO_DECODER
        status = System_linkControl(gVcapModuleContext.captureId,
                                    CAPTURE_LINK_CMD_CONFIGURE_VIP_DECODERS,
                                    NULL,
                                    0,
                                    TRUE);
#else
        Int32 status = 0;
        Device_VideoDecoderChipIdParams      vidDecChipIdArgs;
        Device_VideoDecoderChipIdStatus      vidDecChipIdStatus;
        VCAP_VIDEO_SOURCE_STATUS_PARAMS_S    videoStatusArgs;
        VCAP_VIDEO_SOURCE_CH_STATUS_S        videoStatus;
        Int32                                i;
        Device_VideoDecoderCreateParams      createArgs;
        Device_VideoDecoderCreateStatus      createStatusArgs;

        VCAP_VIDDEC_PARAMS_S                 vidDecVideoModeArgs;


        /* Initialize and create video decoders */
        Device_tvp5158Init();

        for(i = 0; i < numDevices; i++)
        {
            memset(&createArgs, 0, sizeof(Device_VideoDecoderCreateParams));

#if defined (TI_814X_BUILD) || defined (TI_8107_BUILD)
            createArgs.deviceI2cInstId    = 2;
#endif
#ifdef TI_816X_BUILD
            createArgs.deviceI2cInstId    = 1;
#endif

            createArgs.numDevicesAtPort   = 1;
            createArgs.deviceI2cAddr[0]
                                          = Device_getVidDecI2cAddr(
                                                             DEVICE_VID_DEC_TVP5158_DRV,
                                                             i);
            createArgs.deviceResetGpio[0] = DEVICE_VIDEO_DECODER_GPIO_NONE;

            gVcapModuleContext.tvp5158Handle[i] = Device_tvp5158Create(
                                                        DEVICE_VID_DEC_TVP5158_DRV,
                                                         i, // instId - need to change
                                                         &(createArgs),
                                                         &(createStatusArgs));


            vidDecChipIdArgs.deviceNum = 0;

            status = Device_tvp5158Control(gVcapModuleContext.tvp5158Handle[i],
                                           IOCTL_DEVICE_VIDEO_DECODER_GET_CHIP_ID,
                                           &vidDecChipIdArgs,
                                           &vidDecChipIdStatus);
            if (status >= 0)
            {
                videoStatusArgs.channelNum = 0;

                status = Device_tvp5158Control(gVcapModuleContext.tvp5158Handle[i],
                                               IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS,
                                               &videoStatusArgs, &videoStatus);

                if (videoStatus.isVideoDetect)
                {
                    printf(" VCAP: TVP5158-%d (0x%02x): Detected video (%dx%d@%dHz, %d) !!!\n",
                               i,
                               createArgs.deviceI2cAddr[0],
                               videoStatus.frameWidth,
                               videoStatus.frameHeight,
                               1000000 / videoStatus.frameInterval,
                               videoStatus.isInterlaced);

                    /* Assumption here is height width of video remains same across channels */
                    if (videoStatus.frameHeight == 288)
                        gVcapModuleContext.tvp5158Handle[i]->isPalMode = TRUE;

                }
                else
                {
                    printf(" VCAP: TVP5158-%d (0x%02x):  NO Video Detected !!!\n", i, createArgs.deviceI2cAddr[0]);
                }
            }
            else
            {
                printf(" VCAP: TVP5158-%d (0x%02x): Device not found !!!\n", i, createArgs.deviceI2cAddr[0]);
            }

#ifdef SYSTEM_ENABLE_AUDIO

            static Int32 tvpCascadedStage =0;
            Device_Tvp5158AudioModeParams audArgs;
            VSYS_PARAMS_S vsysContext;

            /* Fixed Setting for  TVP5158*/
            audArgs.deviceNum           = 0; /* Should not changed */
            audArgs.samplingHz          = 16000;
            audArgs.masterModeEnable    = 1;
            audArgs.dspModeEnable       = 0;
            audArgs.ulawEnable          = 0;
            audArgs.audioVolume         = 5;//High value will create gliches and noise in Captured Audio
            /*should not be changed*/

            Vsys_getContext(&vsysContext);

            switch (vsysContext.numChs)
            {
                case 4:
                {
                    audArgs.tdmChannelNum       = 1; // pDrvState->audiohwPortProperties.tdmChannelNum;
                    audArgs.cascadeStage        = 0;
                }
                break;
                case 8:
                {
                    audArgs.tdmChannelNum       = 2; // pDrvState->audiohwPortProperties.tdmChannelNum;
                    audArgs.cascadeStage        = tvpCascadedStage;
                    tvpCascadedStage++; /* In case of Cascading the cascadeStage  should be incremented by 1 */
                }
                break;
                case 16:
#if TI_816X_BUILD //This has to be enabled when 16 channel audio cascading is supported in the board
                {
                    audArgs.tdmChannelNum       = 4; // pDrvState->audiohwPortProperties.tdmChannelNum;
                    audArgs.cascadeStage        = tvpCascadedStage;
                    tvpCascadedStage++; /* In case of Cascading the cascadeStage  should be incremented by 1 */
                }
#else
                {
                    audArgs.tdmChannelNum       = 1; // pDrvState->audiohwPortProperties.tdmChannelNum;
                    audArgs.cascadeStage        = 0;
                }
#endif
                break;
                default:
                    printf("\n Audio Channels are not supported\n");
                break;
            }

            status = Device_tvp5158Control(gVcapModuleContext.tvp5158Handle[i],
                                           IOCTL_DEVICE_TVP5158_SET_AUDIO_MODE,
                                           &audArgs,
                                           &createStatusArgs);

#endif
        }

        /* Configure video decoder */

        for(i = 0; i < numDevices; i++)
        {
            if(modeParams != NULL)
            {
                memset(&vidDecVideoModeArgs,
                       0, sizeof(VCAP_VIDDEC_PARAMS_S));

                memcpy(&vidDecVideoModeArgs,
                       &modeParams[i],
                       sizeof(VCAP_VIDDEC_PARAMS_S));

                status = Device_tvp5158Control(gVcapModuleContext.tvp5158Handle[i],
                                               IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_MODE,
                                               &vidDecVideoModeArgs,
                                               NULL);
            }
            else
            {
                status = -1;
            }
        }
#endif
    }

    return status;
}


Int32 Vcap_deleteVideoDecoder()
{
    Int32 i;

    for(i = 0; i < VCAP_DEV_MAX; i++)
    {
        if(gVcapModuleContext.tvp5158Handle[i])
        {
            Device_tvp5158Delete(gVcapModuleContext.tvp5158Handle[i], NULL);
            gVcapModuleContext.tvp5158Handle[i] = NULL;
        }
    }
    return 0;
}
Int32 Vcap_setColor(Int32 contrast, Int32 brightness, Int32 saturation, Int32 hue, Int32 chId)
{

    if(gVcapModuleContext.vcapConfig.enableConfigExtVideoDecoder)
    {
#ifndef SYSTEM_USE_VIDEO_DECODER
        Int32 status = 0;

        Device_VideoDecoderColorParams colorPrm;

        gVcapModuleContext.colorPrm.videoBrightness = brightness;
        gVcapModuleContext.colorPrm.videoContrast   = contrast;
        gVcapModuleContext.colorPrm.videoSaturation = saturation;
        gVcapModuleContext.colorPrm.videoHue        = hue;

        if(gVcapModuleContext.colorPrm.videoBrightness<0)
            gVcapModuleContext.colorPrm.videoBrightness = 0;
        if(gVcapModuleContext.colorPrm.videoBrightness>255)
            gVcapModuleContext.colorPrm.videoBrightness = 255;

        if(gVcapModuleContext.colorPrm.videoContrast<0)
            gVcapModuleContext.colorPrm.videoContrast = 0;
        if(gVcapModuleContext.colorPrm.videoContrast>255)
            gVcapModuleContext.colorPrm.videoContrast = 255;

        if(gVcapModuleContext.colorPrm.videoSaturation<0)
            gVcapModuleContext.colorPrm.videoSaturation = 0;
        if(gVcapModuleContext.colorPrm.videoSaturation>255)
            gVcapModuleContext.colorPrm.videoSaturation = 255;

        if(gVcapModuleContext.colorPrm.videoHue<0)
            gVcapModuleContext.colorPrm.videoHue = 0;
        if(gVcapModuleContext.colorPrm.videoHue>255)
            gVcapModuleContext.colorPrm.videoHue = 255;

        colorPrm.channelNum      = chId;
        colorPrm.videoBrightness = gVcapModuleContext.colorPrm.videoBrightness;
        colorPrm.videoContrast   = gVcapModuleContext.colorPrm.videoContrast;
        colorPrm.videoSaturation = gVcapModuleContext.colorPrm.videoSaturation;
        colorPrm.videoSharpness  = DEVICE_VIDEO_DECODER_NO_CHANGE;
        colorPrm.videoHue        = gVcapModuleContext.colorPrm.videoHue;

        status = Device_tvp5158Control(
                 gVcapModuleContext.tvp5158Handle[chId/VCAP_MAX_CHN_PER_DEVICE],
                 IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_COLOR,
                 &colorPrm,
                 NULL);

        if (status >= 0)
        {
            printf
                ("CAPTURE: channel %d: Color parameter setting successful !!!\n",
                 chId);
        }
        else
        {
            printf
                ("CAPTURE: channel %d: Color parameter setting successful !!!\n",
                 chId);
        }
#endif
    }
    return 0;
}

Bool Vcap_isPalMode()
{
    if(gVcapModuleContext.vcapConfig.enableConfigExtVideoDecoder)
    {
        if(gVcapModuleContext.tvp5158Handle[0])
        {
            return(gVcapModuleContext.tvp5158Handle[0]->isPalMode);
        }
        else
        {
            printf(" VCAP: TVP5158: ERROR: Handle not created !!!\n");

            return FALSE;
        }
    }
    else
    {
        return (gVcapModuleContext.isPalMode);
    }
}



/*=============================================================
 * @file:       ti_vdis.c
 *
 * @brief:  Video Display function declare.
 *
 * @vers:       0.5.0.0 2011-06
 *
 *=============================================================
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#include "ti_vsys_priv.h"
#include "ti_vcap_priv.h"
#include "ti_vdis_priv.h"
#include "ti_vdis.h"
#include "ti_vdis_timings.h"


#include <mcfw/interfaces/link_api/displayLink.h>
#include <mcfw/interfaces/link_api/swMsLink.h>
#include <mcfw/interfaces/link_api/deiLink.h>
#include <mcfw/interfaces/link_api/avsync_hlos.h>

#define VDIS_TRACE_ENABLE_FXN_ENTRY_EXIT           (0)
#define VDIS_TRACE_INFO_PRINT_INTERVAL             (8192)


#if VDIS_TRACE_ENABLE_FXN_ENTRY_EXIT
#define VDIS_TRACE_FXN(str,...)                    do {                           \
                                                     static Int printInterval = 0;\
                                                     if ((printInterval % VDIS_TRACE_INFO_PRINT_INTERVAL) == 0) \
                                                     {                                                          \
                                                         OSA_printf("TI_VDIS:%s function:%s",str,__func__);     \
                                                         OSA_printf(__VA_ARGS__);                               \
                                                     }                                                          \
                                                     printInterval++;                                           \
                                                   } while (0)
#define VDIS_TRACE_FXN_ENTRY(...)                  VDIS_TRACE_FXN("Entered",__VA_ARGS__)
#define VDIS_TRACE_FXN_EXIT(...)                   VDIS_TRACE_FXN("Leaving",__VA_ARGS__)
#else
#define VDIS_TRACE_FXN_ENTRY(...)
#define VDIS_TRACE_FXN_EXIT(...)
#endif

/* =============================================================================
 * Globals
 * =============================================================================
 */

VDIS_MODULE_CONTEXT_S gVdisModuleContext = {
    .initDone = FALSE,
    .displayConfigInitDone = FALSE
};

VDIS_CHN_MAP_INFO_S gVdisChnMapInfo[VDIS_DEV_MAX];

/* =============================================================================
 * Vdis internal APIs prototype
 * =============================================================================
 */
static Void Vdis_setDefaultConfig();

static Void Vdis_swMs_PrintLayoutParams(VDIS_DEV vdDevId, SwMsLink_LayoutPrm * vdMosaicParam);


/* =============================================================================
 * Vdis module APIs
 * =============================================================================
 */
/**
 * \brief:
 *      Initialize parameters to be passed to init
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Void Vdis_params_init(VDIS_PARAMS_S * pContext)
{
    UInt16 i, j, width, height;

    memset(pContext, 0, sizeof(VDIS_PARAMS_S));
    for(i = 0; i < VDIS_DEV_MAX; i++)
    {
        pContext->deviceParams[i].backGroundColor = 0;

        pContext->deviceParams[i].resolution  = VSYS_STD_1080P_60;

        width  = 1920;
        height = 1080;

        if(i == VDIS_DEV_SD)
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

    pContext->tiedDevicesMask = VDIS_VENC_HDMI | VDIS_VENC_DVO2;

    pContext->enableConfigExtVideoEncoder = TRUE;

#if defined(TI814X_DVR) || defined(TI810X_DVR)
    pContext->enableConfigExtVideoEncoder = FALSE;
#endif

    pContext->deviceParams[VDIS_DEV_DVO2].enable = TRUE;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.vencNodeNum = VDIS_VENC_DVO2;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.aFmt = VDIS_A_OUTPUT_COMPOSITE;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dvoFidPolarity = VDIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dvoVsPolarity = VDIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dvoHsPolarity = VDIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dvoActVidPolarity = VDIS_POLARITY_ACT_HIGH;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dvoFmt = VDIS_DVOFMT_DOUBLECHAN;
    pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dataFormat = SYSTEM_DF_YUV422SP_UV;

    pContext->deviceParams[VDIS_DEV_HDMI].enable = TRUE;
    pContext->deviceParams[VDIS_DEV_HDMI].outputInfo.vencNodeNum = VDIS_VENC_HDMI;
    pContext->deviceParams[VDIS_DEV_HDMI].outputInfo.aFmt = VDIS_A_OUTPUT_COMPOSITE;
    pContext->deviceParams[VDIS_DEV_HDMI].outputInfo.dvoFmt = VDIS_DVOFMT_TRIPLECHAN_EMBSYNC;
    pContext->deviceParams[VDIS_DEV_HDMI].outputInfo.dataFormat = SYSTEM_DF_RGB24_888;

    pContext->deviceParams[VDIS_DEV_SD].enable = TRUE;
    pContext->deviceParams[VDIS_DEV_SD].outputInfo.vencNodeNum = VDIS_VENC_SD;
    pContext->deviceParams[VDIS_DEV_SD].outputInfo.aFmt = VDIS_A_OUTPUT_COMPOSITE;
    pContext->deviceParams[VDIS_DEV_SD].outputInfo.dvoFmt = VDIS_DVOFMT_TRIPLECHAN_DISCSYNC;
    //pContext->deviceParams[VDIS_DEV_SD].outputInfo.dvoFmt = VDIS_DVOFMT_SINGLECHAN;
    pContext->deviceParams[VDIS_DEV_SD].outputInfo.dataFormat = SYSTEM_DF_RGB24_888;

#if defined(TI_816X_BUILD) || defined (TI_8107_BUILD)
    pContext->deviceParams[VDIS_DEV_HDCOMP].enable = TRUE;
    pContext->deviceParams[VDIS_DEV_HDCOMP].outputInfo.vencNodeNum = VDIS_VENC_HDCOMP;
    pContext->deviceParams[VDIS_DEV_HDCOMP].outputInfo.aFmt = VDIS_A_OUTPUT_COMPONENT;
    pContext->deviceParams[VDIS_DEV_HDCOMP].outputInfo.dvoFmt = VDIS_DVOFMT_TRIPLECHAN_EMBSYNC;
#if defined (TI_8107_BUILD)
    pContext->deviceParams[VDIS_DEV_HDCOMP].outputInfo.dvoFmt = VDIS_DVOFMT_TRIPLECHAN_DISCSYNC;
#endif
#if defined(TI816X_DVR) || defined(TI8107_DVR) || defined(TI8107_EVM)
        pContext->deviceParams[VDIS_DEV_HDCOMP].outputInfo.dataFormat = SYSTEM_DF_RGB24_888;
#endif
#if defined (TI816X_EVM)
        pContext->deviceParams[VDIS_DEV_HDCOMP].outputInfo.dataFormat = SYSTEM_DF_YUV444P;
#endif
#ifdef TI816X_CZ
        pContext->deviceParams[VDIS_DEV_HDCOMP].outputInfo.dataFormat = SYSTEM_DF_YUV422P;
#endif
#ifdef TI816X_ETV
        pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dvoFmt = VDIS_DVOFMT_TRIPLECHAN_DISCSYNC;
        pContext->deviceParams[VDIS_DEV_DVO2].outputInfo.dataFormat = SYSTEM_DF_RGB24_888;
#endif
    pContext->tiedDevicesMask = VDIS_VENC_HDCOMP | VDIS_VENC_DVO2;
    pContext->enableEdgeEnhancement = TRUE;

#endif

    pContext->deviceParams[SYSTEM_DC_VENC_HDMI].colorSpaceMode = VDIS_CSC_MODE_SDTV_GRAPHICS_Y2R;

#if defined(TI816X_EVM) || defined(TI8107_EVM) || defined(TI8107_DVR)
    pContext->enableConfigExtThsFilter = TRUE;
#endif

    pContext->enableLayoutGridDraw = FALSE;

}

/**
* \brief:
*       Setup the Tied Venc Configuration
* \input:
*       Tied Display Devices, Vdis Context
* \output:
*        NA
* \return:
*        ERROR_NOERROR        -- while success
*        ERROR_CODE             -- refer for err defination
*/

Int32 Vdis_tiedVencInit(VDIS_DEV VdDevId1, VDIS_DEV VdDevId2, VDIS_PARAMS_S * pContext)
{
#if defined (TI_814X_BUILD)
    pContext->tiedDevicesMask = VDIS_VENC_DVO2 | VDIS_VENC_HDMI;
    pContext->deviceParams[VDIS_DEV_DVO2].resolution = pContext->deviceParams[VDIS_DEV_HDMI].resolution;
#endif

#if defined (TI_8107_BUILD)
    pContext->tiedDevicesMask = VDIS_VENC_HDMI | VDIS_VENC_HDCOMP;
    pContext->deviceParams[VDIS_VENC_HDCOMP].resolution = pContext->deviceParams[VDIS_DEV_HDMI].resolution;
#endif


#ifdef TI_816X_BUILD
    if(VdDevId1 == VDIS_DEV_HDMI)
    {
        if(VdDevId2 == VDIS_DEV_HDCOMP)
        {
            pContext->tiedDevicesMask = VDIS_VENC_HDCOMP | VDIS_VENC_HDMI;
            pContext->deviceParams[VDIS_DEV_HDCOMP].resolution = pContext->deviceParams[VDIS_DEV_HDMI].resolution ;
            /* GRPX1 should be pointing to DVO2 Node*/
            system("echo 1:dvo2 > /sys/devices/platform/vpss/graphics1/nodes");


        }
        else if(VdDevId2 == VDIS_DEV_DVO2)
        {
            pContext->tiedDevicesMask = VDIS_VENC_DVO2 | VDIS_VENC_HDMI;
            pContext->deviceParams[VDIS_DEV_DVO2].resolution = pContext->deviceParams[VDIS_DEV_HDMI].resolution;
            /* GRPX1 should be pointing to HDCOMP Node*/
            system("echo 1:hdcomp > /sys/devices/platform/vpss/graphics1/nodes");
        }
    }

    if(VdDevId1 == VDIS_DEV_HDCOMP)
    {
        if(VdDevId2 == VDIS_DEV_HDMI)
        {
            pContext->tiedDevicesMask = VDIS_VENC_HDCOMP | VDIS_VENC_HDMI;
            pContext->deviceParams[VDIS_DEV_HDMI].resolution = pContext->deviceParams[VDIS_DEV_HDCOMP].resolution;
            /* GRPX1 should be pointing to DVO2 Node*/
            system("echo 1:dvo2 > /sys/devices/platform/vpss/graphics1/nodes");
        }
        else if(VdDevId2 == VDIS_DEV_DVO2)
        {
            pContext->tiedDevicesMask = VDIS_VENC_DVO2 | VDIS_VENC_HDCOMP;
            pContext->deviceParams[VDIS_DEV_DVO2].resolution = pContext->deviceParams[VDIS_DEV_HDCOMP].resolution;
            /* GRPX1 should be pointing to DVO2/HDCOMP Node*/
            system("echo 1:dvo2 > /sys/devices/platform/vpss/graphics1/nodes");
        }
    }

    if(VdDevId1 == VDIS_DEV_DVO2)
    {
        if(VdDevId2 == VDIS_DEV_HDCOMP)
        {
            pContext->tiedDevicesMask = VDIS_VENC_HDCOMP | VDIS_VENC_DVO2;
            pContext->deviceParams[VDIS_DEV_HDCOMP].resolution = pContext->deviceParams[VDIS_DEV_DVO2].resolution;
            /* GRPX1 should be pointing to DVO2/HDCOMP Node*/
            system("echo 1:dvo2 > /sys/devices/platform/vpss/graphics1/nodes");
        }
        else if(VdDevId2 == VDIS_DEV_HDMI)
        {
            pContext->tiedDevicesMask = VDIS_VENC_DVO2 | VDIS_VENC_HDMI;
            pContext->deviceParams[VDIS_DEV_HDMI].resolution = pContext->deviceParams[VDIS_DEV_DVO2].resolution;
            /* GRPX1 should be pointing to HDCOMP Node*/
            system("echo 1:hdcomp > /sys/devices/platform/vpss/graphics1/nodes");
        }
    }
#endif
    return 0;
}
/**
 * \brief:
 *      Initialize Vdis instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_init(VDIS_PARAMS_S * pContext)
{
    UInt32 devId;
    Int32  status = 0;

#ifndef SYSTEM_USE_VIDEO_DECODER
    Device_VideoEncoderCreateParams      encCreateParams;
    Device_VideoEncoderCreateStatus      encCreateStatus;
    Device_HdmiChipId                    hdmiId;
    Device_SiI9022aHpdPrms               hpdPrms;
    Device_SiI9022aModeParams            modePrms;
#endif

    for(devId = 0; devId < VDIS_DEV_MAX; devId++)
    {
        gVdisModuleContext.swMsId[devId] = SYSTEM_LINK_ID_INVALID;
        gVdisModuleContext.displayId[devId] = SYSTEM_LINK_ID_INVALID;
        gVdisModuleContext.grpxId[devId] = SYSTEM_LINK_ID_INVALID;
    }
    gVdisModuleContext.ipcFramesInVpssFromHostId = SYSTEM_LINK_ID_INVALID;
    gVdisModuleContext.ipcFramesOutHostId        = SYSTEM_LINK_ID_INVALID;

    Vdis_setDefaultConfig();

    if(pContext==NULL)
    {
        Vdis_params_init(&gVdisModuleContext.vdisConfig);
    }
    else
    {
        memcpy(&gVdisModuleContext.vdisConfig, pContext, sizeof(VDIS_PARAMS_S));

        if(gVdisModuleContext.vdisConfig.enableConfigExtVideoEncoder)
        {
#ifndef SYSTEM_USE_VIDEO_DECODER
            /* Initialize and create video encoders */

            Device_sii9022aInit();

            /* Open HDMI Tx */
#if defined (TI_814X_BUILD) || defined (TI_8107_BUILD)
                encCreateParams.deviceI2cInstId = 2;
#endif
#ifdef TI_816X_BUILD
                encCreateParams.deviceI2cInstId = 1;
#endif
            encCreateParams.deviceI2cAddr   = Device_getVidDecI2cAddr(
                                                             DEVICE_VID_ENC_SII9022A_DRV,
                                                             0);
            encCreateParams.inpClk = 0;
            encCreateParams.hdmiHotPlugGpioIntrLine = 0;

            encCreateParams.syncMode = DEVICE_VIDEO_ENCODER_EMBEDDED_SYNC;
            encCreateParams.clkEdge = FALSE;

#if defined (TI_814X_BUILD) || defined (TI_8107_BUILD)
            encCreateParams.clkEdge = FALSE;
#endif /* TI_814X_BUILD */

            gVdisModuleContext.sii9022aHandle =
                                  Device_sii9022aCreate(DEVICE_VID_ENC_SII9022A_DRV,
                                                         0, // instId - need to change
                                                         &(encCreateParams),
                                                         &(encCreateStatus));

            status = Device_sii9022aControl(gVdisModuleContext.sii9022aHandle,
                                           IOCTL_DEVICE_SII9022A_GET_DETAILED_CHIP_ID,
                                           &hdmiId,
                                           NULL);
            OSA_assert(status >= 0);

            status = Device_sii9022aControl(gVdisModuleContext.sii9022aHandle,
                                           IOCTL_DEVICE_SII9022A_QUERY_HPD,
                                           &hpdPrms,
                                           NULL);
            OSA_assert(status >= 0);

            modePrms.standard = VSYS_STD_1080P_60;

            status = Device_sii9022aControl(gVdisModuleContext.sii9022aHandle,
                                           IOCTL_DEVICE_VIDEO_ENCODER_SET_MODE,
                                           &modePrms,
                                           NULL);
            OSA_assert(status >= 0);

#endif
        }

        if(gVdisModuleContext.vdisConfig.enableConfigExtThsFilter)
        {

#ifndef SYSTEM_USE_VIDEO_DECODER
#if defined (TI816X_EVM) || defined (TI_8107_BUILD)

            System_Ths7360SfCtrl   thsCtrl;
            UInt32                 standard;

            Device_thsFiltersInit();

            /* TBD : Retrieve from vdis  */
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
#endif
        }
        for (devId = 0;devId < VDIS_DEV_MAX;devId++)
        {
            AvsyncLink_LinkSynchConfigParams_Init(&gVdisModuleContext.avsyncCfg[devId]);
            gVdisModuleContext.avsyncCfg[devId].audioDevId       = AVSYNC_INVALID_AUDDEVID;
            gVdisModuleContext.avsyncCfg[devId].displayLinkID    = SYSTEM_LINK_ID_DISPLAY_FIRST + devId;
            gVdisModuleContext.avsyncCfg[devId].videoSynchLinkID = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0 +
                                        Vdis_getSwMsContextIndex((VDIS_DEV)devId);
            gVdisModuleContext.avsyncCfg[devId].numCh = 0;
            gVdisModuleContext.avsyncCfg[devId].syncMasterChnum = AVSYNC_INVALID_CHNUM;
        }

        gVdisModuleContext.mpSclrId = SYSTEM_LINK_ID_INVALID;
        gVdisModuleContext.initDone = TRUE;
    }

    return  status;

}

/**
 * \brief:
 *      Finalize Vdis instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Void Vdis_exit()
{
    if(gVdisModuleContext.vdisConfig.enableConfigExtVideoEncoder)
    {
        Device_sii9022aDelete(gVdisModuleContext.sii9022aHandle,NULL);
    }

    if(gVdisModuleContext.vdisConfig.enableConfigExtThsFilter)
    {
#ifndef SYSTEM_USE_VIDEO_DECODER
#if defined(TI816X_EVM) || defined (TI_8107_BUILD)
        Device_thsFiltersDeInit();
#endif
#endif
    }
    gVdisModuleContext.initDone = FALSE;

}

/**
 * \brief:
 *      Start Vdis instance for display
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_start()
{
    UInt32 devId;
    Int32 status = 0;


    if(gVdisModuleContext.vdisConfig.enableConfigExtVideoEncoder)
    {
        status = Device_sii9022aControl(gVdisModuleContext.sii9022aHandle,
                                        DEVICE_CMD_START,
                                        NULL,
                                        NULL);

    }


    for(devId = 0; devId < VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.displayId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gVdisModuleContext.displayId[devId]);
        }
    }

    for(devId = 0; devId < VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.swMsId[devId]!=SYSTEM_LINK_ID_INVALID)
    {
            System_linkStart(gVdisModuleContext.swMsId[devId]);
        }
    }

    if (gVdisModuleContext.ipcFramesInVpssFromHostId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gVdisModuleContext.ipcFramesInVpssFromHostId);
        OSA_assert(gVdisModuleContext.ipcFramesOutHostId != SYSTEM_LINK_ID_INVALID);
        System_linkStart(gVdisModuleContext.ipcFramesOutHostId);
    }

    return status;
}

/**
 * \brief:
 *      Stop Vdis instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_stop()
{
    UInt32 devId;
    Int32 status = 0;

    if(gVdisModuleContext.vdisConfig.enableConfigExtVideoEncoder)
    {
        status = Device_sii9022aControl(gVdisModuleContext.sii9022aHandle,
                                    DEVICE_CMD_STOP,
                                    NULL,
                                    NULL);
    }

    if (gVdisModuleContext.ipcFramesOutHostId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gVdisModuleContext.ipcFramesOutHostId);
        OSA_assert(gVdisModuleContext.ipcFramesInVpssFromHostId != SYSTEM_LINK_ID_INVALID);
        System_linkStop(gVdisModuleContext.ipcFramesInVpssFromHostId);
    }

    for(devId = 0; devId < VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.swMsId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gVdisModuleContext.swMsId[devId]);
        }
    }

    for(devId = 0; devId < VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.displayId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gVdisModuleContext.displayId[devId]);
        }
    }

    return 0;
}

/* Generic description */
/* There will be 3 levels as:
   1. Device -- device ports setting, different chip have different mapping.
   2. Mosaic -- multiple set for video window layout, just switch different mosaic id for different layout.
   3. Channel -- virtual channel, which need add to mosaic window

   need support
    privacy mask for specific channel
    insert raw data to specific channel -- ?? display or capture??
*/

/* =============================================================================
 * Device related APIs
 * =============================================================================
 */
/**
 * \brief:
 *      Set display device parameters
 * \input:
 *      vdDevId             -- capture device id, refer to VCAP_DEV define
 *      psVdDevParam        -- Device parameter structure
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_setParamDevice(VDIS_DEV VdDevId, VDIS_DEV_PARAM_S *psVdDevParam)
{
    memcpy(&gVdisModuleContext.vdisConfig.deviceParams[VdDevId], psVdDevParam, sizeof(VDIS_DEV_PARAM_S));
    return 0;
}

/**
 * \brief:
 *      Get display device parameters
 * \input:
 *      vdDevId             -- capture device id, refer to VCAP_DEV define
 * \output:
 *      psVdDevParam    -- Device parameter structure
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_getParamDevice(VDIS_DEV VdDevId, VDIS_DEV_PARAM_S *psVdDevParam)
{
    memcpy(psVdDevParam, &gVdisModuleContext.vdisConfig.deviceParams[VdDevId], sizeof(VDIS_DEV_PARAM_S));
    return 0;
}
/**
 * \brief:
 *      Get display device parameters
 * \input:
 *      vdDevId             -- capture device id, refer to VCAP_DEV define
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_enableDevice(VDIS_DEV vdDevId)
{
    return 0;
}
/**
 * \brief:
 *      Get display device parameters
 * \input:
 *      vdDevId             -- capture device id, refer to VCAP_DEV define
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_disableDevice(VDIS_DEV vdDevId)
{
    return 0;
}


/* =============================================================================
 * Mosaic related APIs
 * =============================================================================
 */

/**
 * \brief:
 *      Set the mosaic window parameters
 * \input:
 *      vdDevId                -- Device id
 *      psVdMosaicParam -- Mosaic layout window parameters
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS          --    while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_setMosaicParams(VDIS_DEV vdDevId, VDIS_MOSAIC_S *psVdMosaicParam )
{
    UInt32 winId, chId;
    UInt32 swMsId = SYSTEM_LINK_ID_INVALID;
    UInt32 displayId = SYSTEM_LINK_ID_INVALID;
    Bool changeDisplayInputMode = FALSE;
    SwMsLink_LayoutPrm vdisLayoutPrm;

#if defined(TI_814X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];

        /* for DM814x, switch to 60fps input, interlaced 60fps output when
            psVdMosaicParam->outputFPS is set to 60fps */
        changeDisplayInputMode = TRUE;
        displayId = gVdisModuleContext.displayId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }
#endif

#if defined(TI_8107_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if ((vdDevId == VDIS_DEV_DVO2) || (vdDevId == VDIS_DEV_HDCOMP))
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];

        /* for DM814x, switch to 60fps input, interlaced 60fps output when
            psVdMosaicParam->outputFPS is set to 60fps */
        changeDisplayInputMode = TRUE;
        displayId = gVdisModuleContext.displayId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }
#endif

#if defined(TI_816X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[2];
    }
#endif

    if(swMsId==SYSTEM_LINK_ID_INVALID)
        return -1;


    /* Get display resolution and coordinates */

    /* Assign mosaic layout number and number of windows */
    vdisLayoutPrm.numWin = psVdMosaicParam->numberOfWindows;
    vdisLayoutPrm.onlyCh2WinMapChanged = psVdMosaicParam->onlyCh2WinMapChanged;
    vdisLayoutPrm.outputFPS = psVdMosaicParam->outputFPS;

    /* Assign each windows coordinates, size and mapping */
    for(winId=0; winId<vdisLayoutPrm.numWin; winId++)
    {
        vdisLayoutPrm.winInfo[winId].channelNum         = psVdMosaicParam->chnMap[winId];
        vdisLayoutPrm.winInfo[winId].bufAddrOffset      = -1;
        vdisLayoutPrm.winInfo[winId].width              = psVdMosaicParam->winList[winId].width;
        vdisLayoutPrm.winInfo[winId].height             = psVdMosaicParam->winList[winId].height;
        vdisLayoutPrm.winInfo[winId].startX             = psVdMosaicParam->winList[winId].start_X;
        vdisLayoutPrm.winInfo[winId].startY             = psVdMosaicParam->winList[winId].start_Y;
        vdisLayoutPrm.winInfo[winId].bypass             = psVdMosaicParam->useLowCostScaling[winId];
        #ifdef TI_816X_BUILD
        /* support CIF preview when lowCostSC is enabled */
        if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC) &&
            (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE))
        {
            if (psVdMosaicParam->useLowCostScaling[winId] == TRUE)
            {
                if (psVdMosaicParam->chnMap[winId] < VDIS_CHN_MAX/2)
                {
                    vdisLayoutPrm.winInfo[winId].channelNum = psVdMosaicParam->chnMap[winId] + VDIS_CHN_MAX;
                    gVdisModuleContext.vdisConfig.mosaicParams[vdDevId].useLowCostScaling[winId] = TRUE;
                }
                vdisLayoutPrm.winInfo[winId].bypass = FALSE;
            }
            else
            {
                if(psVdMosaicParam->chnMap[winId] != VDIS_CHN_INVALID)
                    gVdisModuleContext.vdisConfig.mosaicParams[vdDevId].useLowCostScaling[winId] = FALSE;
            }
        }
        #endif


        chId = psVdMosaicParam->chnMap[winId];

        if((chId < gVdisModuleContext.vdisConfig.numChannels) && (chId >= 0))
        {
            gVdisChnMapInfo[vdDevId].ch2WinMap[chId] = winId;

            if(gVdisChnMapInfo[vdDevId].isEnableChn[chId] == FALSE)
            {
                vdisLayoutPrm.winInfo[winId].channelNum = SYSTEM_SW_MS_INVALID_ID;
            }
        }
    }

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF) ||
        (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT))
    {
        /* mosaic switching handled in use-case specific function */
        return MultiCh_progressive16ChVcapVencVdecVdisSwitchLayout(vdDevId, &(vdisLayoutPrm));
    }
#endif
#if defined(TI_8107_BUILD)
    if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH) ||
        (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT))
    {
        /* mosaic switching handled in use-case specific function */
        return MultiCh_progressive8ChVcapVencVdecVdisSwitchLayout(vdDevId, &(vdisLayoutPrm));
    }
#endif

    Vdis_swMs_PrintLayoutParams(vdDevId, &vdisLayoutPrm);
    System_linkControl(swMsId, SYSTEM_SW_MS_LINK_CMD_SWITCH_LAYOUT, &(vdisLayoutPrm), sizeof(vdisLayoutPrm), TRUE);

    if(changeDisplayInputMode)
    {
        DisplayLink_SwitchInputMode prm;

        prm.enableFieldSeparatedInputMode = FALSE;

        if(vdisLayoutPrm.outputFPS >= 50 )
            prm.enableFieldSeparatedInputMode = TRUE;

        System_linkControl(displayId, DISPLAY_LINK_CMD_SWITCH_INPUT_MODE, &prm, sizeof(prm), TRUE);
    }

    return 0;
}
/**
 * \brief:
 *      Get the mosaic window parameters
 * \input:
 *      vdMosaicId      -- Mosaic id
 * \output:
 *      psVdMosaicParam -- Mosaic layout window parameters
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_getMosaicParams(VDIS_DEV vdDevId, VDIS_MOSAIC_S *psVdMosaicParam)
{
    SwMsLink_LayoutPrm vdisLayoutPrm;
    UInt32 winId;

    UInt32 swMsId = 0;

    /* Initialize numWindows to zero by default */
    psVdMosaicParam->numberOfWindows = 0;
    psVdMosaicParam->outputFPS = 30;

#if defined(TI_814X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }
#endif

#if defined(TI_8107_BUILD)

    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if ((vdDevId == VDIS_DEV_DVO2) || (vdDevId == VDIS_DEV_HDCOMP))
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }

#endif

#if defined(TI_816X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[2];
    }
#endif


    if(swMsId==SYSTEM_LINK_ID_INVALID)
        return -1;

    System_linkControl(swMsId, SYSTEM_SW_MS_LINK_CMD_GET_LAYOUT_PARAMS, &(vdisLayoutPrm), sizeof(vdisLayoutPrm), TRUE);

    /* Get display resolution and coordinates */
    psVdMosaicParam->displayWindow.width    = 0;
    psVdMosaicParam->displayWindow.height   = 0;
    psVdMosaicParam->displayWindow.start_X  = 0;
    psVdMosaicParam->displayWindow.start_Y  = 0;

    /* get number of windows */
    psVdMosaicParam->numberOfWindows = vdisLayoutPrm.numWin;
    psVdMosaicParam->onlyCh2WinMapChanged = vdisLayoutPrm.onlyCh2WinMapChanged;
    psVdMosaicParam->outputFPS = vdisLayoutPrm.outputFPS;


    /* Get each windows coordinates, size and mapping */
    for(winId=0; winId<vdisLayoutPrm.numWin; winId++)
    {
        psVdMosaicParam->chnMap[winId]              = vdisLayoutPrm.winInfo[winId].channelNum;
        psVdMosaicParam->winList[winId].width       = vdisLayoutPrm.winInfo[winId].width;
        psVdMosaicParam->winList[winId].height      = vdisLayoutPrm.winInfo[winId].height;
        psVdMosaicParam->winList[winId].start_X     = vdisLayoutPrm.winInfo[winId].startX;
        psVdMosaicParam->winList[winId].start_Y     = vdisLayoutPrm.winInfo[winId].startY;
        psVdMosaicParam->useLowCostScaling[winId]   = vdisLayoutPrm.winInfo[winId].bypass;
        #ifdef TI_816X_BUILD
        /* support CIF preview when lowCostSC is enabled */
        if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC) &&
            (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE))
        {
            if ((gVdisModuleContext.vdisConfig.mosaicParams[vdDevId].useLowCostScaling[winId] == TRUE) &&
                (psVdMosaicParam->chnMap[winId] != VDIS_CHN_INVALID))
            {
                psVdMosaicParam->chnMap[winId] = psVdMosaicParam->chnMap[winId] % VDIS_CHN_MAX;
            }
            psVdMosaicParam->useLowCostScaling[winId] =
                gVdisModuleContext.vdisConfig.mosaicParams[vdDevId].useLowCostScaling[winId];
        }
        #endif
    }

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF) ||
        (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT))
    {
        MultiCh_progressive16ChVcapVencVdecVdisSwmsChReMap(psVdMosaicParam);
    }
#endif
#if defined(TI_8107_BUILD)
    if (gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH)
    {
        MultiCh_progressive8ChVcapVencVdecVdisSwmsChReMap(psVdMosaicParam);
    }
#endif
    return 0;
}



/**
 * \brief:
 *      pause the mosaic window, repeat the last frame
 * \input:
 *      vdMosaicId      -- Mosaic id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_pauseMosaic(VDIS_DEV vdDevId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_PauseParams  pauseParams;

        pauseParams.chNum     = AVSYNC_ALL_CHANNEL_ID;
        pauseParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        status = Avsync_doPause(&pauseParams);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}
/**
 * \brief:
 *      resume the mosaic window
 * \input:
 *      vdMosaicId      -- Mosaic id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_resumeMosaic(VDIS_MOSAIC vdDevId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {
        Avsync_UnPauseParams  unpauseParams;

        unpauseParams.chNum     = AVSYNC_ALL_CHANNEL_ID;
        unpauseParams.displayLinkID = Vdis_getDisplayId(vdDevId);

        Avsync_doUnPause(&unpauseParams);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}
/**
 * \brief:
 *      Step the mosaic window based on command
 * \input:
 *      vdMosaicId      -- Mosaic id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_stepMosaic(VDIS_MOSAIC vdDevId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_StepFwdParams  stepFwdParams;

        stepFwdParams.chNum     = AVSYNC_ALL_CHANNEL_ID;
        stepFwdParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        status = Avsync_stepFwd(&stepFwdParams);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}

/* ------------ Channel related APIs ------------*/
/**
 * \brief:
 *      Set the display channel to mosaic window mapping
 * \input:
 *      vdDevId         --Device id
 *      usChnMap        --Array Mapping Channel to Window
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS        --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdis_setMosaicChn(VDIS_DEV vdDevId, VDIS_CHN *usChnMap)
{
    UInt32 winId, status;
    VDIS_MOSAIC_S sVdMosaicParam;


    status = Vdis_getMosaicParams(vdDevId,&sVdMosaicParam);

    if(status<0)
        return status;

    sVdMosaicParam.onlyCh2WinMapChanged = TRUE;

    for(winId=0;winId<VDIS_MOSAIC_WIN_MAX;winId++)
    {
        sVdMosaicParam.chnMap[winId] = *(usChnMap+winId);

        if(sVdMosaicParam.chnMap[winId] >= gVdisModuleContext.vdisConfig.numChannels)
        {
            sVdMosaicParam.chnMap[winId] = sVdMosaicParam.chnMap[winId] - gVdisModuleContext.vdisConfig.numChannels;
        }
    }
    status = Vdis_setMosaicParams(vdDevId, &sVdMosaicParam);
    return status;
}



/**
 * \brief:
 *      Diable the display channel with show blank frame
 * \input:
 *      vdChnId         -- display channel id
 *      vdDevId         -- Device Id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS        --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_disableChn(VDIS_DEV vdDevId, VDIS_CHN vdChId)
{
    VDIS_MOSAIC_S sVdMosaicParam;
    UInt32 winId, status;
#if TI_VDIS_DEBUG
       UInt32 chId = 0;
#endif
    status = Vdis_getMosaicParams(vdDevId,&sVdMosaicParam);

    if(status<0)
        return status;

    sVdMosaicParam.onlyCh2WinMapChanged = FALSE;

    for(winId=0;winId<sVdMosaicParam.numberOfWindows;winId++)
    {
        if(sVdMosaicParam.chnMap[winId] == vdChId )
        {
            sVdMosaicParam.chnMap[winId] = VDIS_CHN_INVALID;
            printf("Disabling Channel Number: %d Window %d Device %d\n", vdChId, winId, vdDevId);
            break;
        }
    }

    /* Update whether channel is enabled or disabled */
    gVdisChnMapInfo[vdDevId].isEnableChn[vdChId] = FALSE;
#if TI_VDIS_DEBUG
    printf("\%s >> ----------- Device: %d \n", __FUNCTION__, vdDevId);
    printf("\nCHANNEL | WINDOW | ENABLE(1)/DISABLE(0)\n");
    for(chId=0;chId<gVdisModuleContext.vdisConfig.numChannels;chId++)
    {
        printf(" %6d | %6d | %6d\n", chId, gVdisChnMapInfo[vdDevId].ch2WinMap[chId], gVdisChnMapInfo[vdDevId].isEnableChn[chId]);
    }
#endif
    status = Vdis_setMosaicParams(vdDevId, &sVdMosaicParam);


    return status;
}

/**
 * \brief:
 *      Diable all the display channel with show blank frame
 * \input:
 *      vdDevId         -- Device Id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS        --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_disableAllChn(VDIS_DEV vdDevId)
{
    VDIS_MOSAIC_S sVdMosaicParam;
    UInt32 winId, status;
    UInt32 chId = 0;

    status = Vdis_getMosaicParams(vdDevId,&sVdMosaicParam);

    if(status<0)
        return status;

    sVdMosaicParam.onlyCh2WinMapChanged = FALSE;

    for(winId=0;winId<sVdMosaicParam.numberOfWindows;winId++)
    {
        sVdMosaicParam.chnMap[winId] = VDIS_CHN_INVALID;
        printf("Disabling Channel Number: %d Window %d Device %d\n", winId, winId, vdDevId);
        /* Update whether channel is enabled or disabled */
        gVdisChnMapInfo[vdDevId].isEnableChn[winId] = FALSE;
    }

#if TI_VDIS_DEBUG
    printf("\%s >> ----------- Device: %d \n", __FUNCTION__, vdDevId);
    printf("\nCHANNEL | WINDOW | ENABLE(1)/DISABLE(0)\n");
    for(chId=0;chId<gVdisModuleContext.vdisConfig.numChannels;chId++)
    {
        printf(" %6d | %6d | %6d\n", chId, gVdisChnMapInfo[vdDevId].ch2WinMap[chId], gVdisChnMapInfo[vdDevId].isEnableChn[chId]);
    }
#endif
    status = Vdis_setMosaicParams(vdDevId, &sVdMosaicParam);

    /* Reset the ch2WinMap with VDIS_CHN_INVALID for all channles at the
     * end of Vdis_disableAllChn() as it need to freshly assigned the
     * ch2WinMap for required channels during Vdis_setMosaicChn() */
    for(chId=0; chId<VDIS_CHN_MAX; chId++)
    {
        gVdisChnMapInfo[vdDevId].ch2WinMap[chId] = VDIS_CHN_INVALID;
    }

    return status;
}

/**
 * \brief:
 *      Pause the display channel with repeat last frame
 * \input:
 *      vdChnId         -- display channel id
 *      vdDevId         -- Device Id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdis_pauseChn(VDIS_DEV vdDevId, VDIS_CHN VdChnId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_PauseParams  pauseParams;

        pauseParams.chNum     = VdChnId;
        pauseParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        status = Avsync_doPause(&pauseParams);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;

}

/**
 * \brief:
 *      Resume the display channel
 * \input:
 *           vdDevId            -- Device Id
 *      vdChnId         -- display channel id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_resumeChn(VDIS_DEV vdDevId, VDIS_CHN VdChnId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_UnPauseParams  unpauseParams;

        unpauseParams.chNum     = VdChnId;
        unpauseParams.displayLinkID = Vdis_getDisplayId(vdDevId);

        Avsync_doUnPause(&unpauseParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

/**
 * \brief:
 *      Step the display channel with command
 * \input:
 *           vdDevId            -- Device Id
 *      vdChnId         -- display channel id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdis_stepChn(VDIS_DEV vdDevId, VDIS_CHN VdChnId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_StepFwdParams  stepFwdParams;

        stepFwdParams.chNum     = VdChnId;
        stepFwdParams.displayLinkID = Vdis_getDisplayId(vdDevId);

        Avsync_stepFwd(&stepFwdParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

/**
 * \brief:
 *      Enable the display channel
 * \input:
 *      vdChnId         -- display channel id
 *      vdDevId         -- Device Id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS        --  while success
*       ERROR_CODE          --  refer for err defination
*/


Int32 Vdis_enableChn(VDIS_DEV vdDevId, VDIS_CHN vdChId)
{
    VDIS_MOSAIC_S sVdMosaicParam;
    UInt32 status;
#if TI_VDIS_DEBUG
       UInt32 chId = 0;
#endif
    status = Vdis_getMosaicParams(vdDevId,&sVdMosaicParam);

    if(status<0)
        return status;

    sVdMosaicParam.onlyCh2WinMapChanged = FALSE;

    /* Update whether channel is enabled or disabled */
    gVdisChnMapInfo[vdDevId].isEnableChn[vdChId] = TRUE;

    if(gVdisChnMapInfo[vdDevId].ch2WinMap[vdChId] < sVdMosaicParam.numberOfWindows)
    {
        sVdMosaicParam.chnMap[(gVdisChnMapInfo[vdDevId].ch2WinMap[vdChId])] = vdChId;
        printf("Enabling Channel Number: %d Window %d Device %d\n", vdChId, (gVdisChnMapInfo[vdDevId].ch2WinMap[vdChId]), vdDevId);
    }
#if TI_VDIS_DEBUG
    printf("\%s >> ----------- Device: %d \n", __FUNCTION__, vdDevId);
    printf("\nCHANNEL | WINDOW | ENABLE(1)/DISABLE(0)\n");
    for(chId=0;chId<gVdisModuleContext.vdisConfig.numChannels;chId++)
    {
        printf(" %6d | %6d | %6d\n", chId, gVdisChnMapInfo[vdDevId].ch2WinMap[chId], gVdisChnMapInfo[vdDevId].isEnableChn[chId]);
    }
#endif
    status = Vdis_setMosaicParams(vdDevId, &sVdMosaicParam);

    return status;
}
/**
 * \brief:
 *      Enable all the display channel
 * \input:
 *      vdDevId         -- Device Id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS        --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdis_enableAllChn(VDIS_DEV vdDevId)
{
    VDIS_MOSAIC_S sVdMosaicParam;
    UInt32 status,winId;
    UInt32 chId = 0;

    status = Vdis_getMosaicParams(vdDevId,&sVdMosaicParam);

    if(status<0)
        return status;

    sVdMosaicParam.onlyCh2WinMapChanged = FALSE;

    for(winId=0;winId<sVdMosaicParam.numberOfWindows;winId++)
    {
        /* Update whether channel is enabled or disabled */
        gVdisChnMapInfo[vdDevId].isEnableChn[winId] = TRUE;

        /* Search for the chId associated with each windowId and populate the
         * sVdMosaicParam.chnMap[] used to set mosaic params */
        for (chId=0; chId< VDIS_CHN_MAX; chId++)
        {
            if(gVdisChnMapInfo[vdDevId].ch2WinMap[chId] == winId)
            {
                sVdMosaicParam.chnMap[winId] = chId;
                break;
            }
        }
        OSA_assert(chId < VDIS_CHN_MAX);
    }


#if TI_VDIS_DEBUG
    printf("\%s >> ----------- Device: %d \n", __FUNCTION__, vdDevId);
    printf("\nCHANNEL | WINDOW | ENABLE(1)/DISABLE(0)\n");
    for(chId=0;chId<gVdisModuleContext.vdisConfig.numChannels;chId++)
    {
        printf(" %6d | %6d | %6d\n", chId, gVdisChnMapInfo[vdDevId].ch2WinMap[chId], gVdisChnMapInfo[vdDevId].isEnableChn[chId]);
    }
#endif
    status = Vdis_setMosaicParams(vdDevId, &sVdMosaicParam);

    return status;
}


Bool  Vdis_isEnableChn(VDIS_DEV vdDevId, VDIS_CHN vdChId)
{
    return 0;
}


/**
    \brief Playback in a timescaled manner (fast/slow playback)

    This API is used to control playback speed

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID
    \param timeScaleX1000  [IN] playback speed in X1000 factor.
                                For 1x this value should be   1000
                                For 0.5x this value should be 500
                                For 2.0x this value should be 2000

*/

Int32 Vdis_setPlaybackSpeed(VDIS_DEV vdDevId,VDIS_CHN vdispChnId, UInt32 timeScaleX1000,
                            UInt32 seqId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_TimeScaleParams  playRate;

        playRate.chNum     = vdispChnId;
        playRate.displayLinkID = Vdis_getDisplayId(vdDevId);
        playRate.timeScaleX1000 = timeScaleX1000;
        playRate.displaySeqId = seqId;

        status = Avsync_setPlaybackSpeed(&playRate);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}

/**
    \brief Playback in a timescaled manner (fast/slow playback) for all channels

    This API is used to control playback speed

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID
    \param timeScaleX1000  [IN] playback speed in X1000 factor.
                                For 1x this value should be   1000
                                For 0.5x this value should be 500
                                For 2.0x this value should be 2000

*/

Int32 Vdis_mosaicSetPlaybackSpeed(VDIS_DEV vdDevId,UInt32 timeScaleX1000,
                                  UInt32 seqId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_TimeScaleParams  playRate;

        playRate.chNum     = AVSYNC_ALL_CHANNEL_ID;
        playRate.displayLinkID = Vdis_getDisplayId(vdDevId);
        playRate.timeScaleX1000 = timeScaleX1000;
        playRate.displaySeqId = seqId;

        status = Avsync_setPlaybackSpeed(&playRate);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}


/**
    \brief Resume normal playback 1x

    API is used to resume normal playback after
    previous slow/fast playback

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID

*/
Int32 Vdis_resumeNormalPlayback(VDIS_DEV vdDevId,VDIS_CHN vdispChnId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {
        Avsync_TimeScaleParams  playRate;

        playRate.chNum     = vdispChnId;
        playRate.displayLinkID = Vdis_getDisplayId(vdDevId);
        playRate.timeScaleX1000 = AVSYNC_TIMESCALE_NORMAL_PLAY;

        status = Avsync_setPlaybackSpeed(&playRate);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

/**
    \brief Resume normal playback 1x for all channels

    API is used to resume normal playback after
    previous slow/fast playback

    \param vdDevId  [IN] display device ID
    \param vcChnId  [IN] decode channel ID

*/
Int32 Vdis_mosaicResumeNormalPlayback(VDIS_DEV vdDevId)
{
    Int32 status = 0;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {
        Avsync_TimeScaleParams  playRate;

        playRate.chNum     = AVSYNC_ALL_CHANNEL_ID;
        playRate.displayLinkID = Vdis_getDisplayId(vdDevId);
        playRate.timeScaleX1000 = AVSYNC_TIMESCALE_NORMAL_PLAY;

        status = Avsync_setPlaybackSpeed(&playRate);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}


Int32 Vdis_getScreenBuffer(VDIS_DEV vdDevId, VDIS_SCREEN_BUF_INFO_S *psScreenBugInfo)
{
    return 0;
}
Int32 Vdis_releaseScreenBuffer(VDIS_DEV vdDevId, VDIS_SCREEN_BUF_INFO_S *psScreenBugInfo)
{
    return 0;
}
VSYS_VIDEO_STANDARD_E Vdis_getResolution(VDIS_DEV devId)
{
    return(gVdisModuleContext.vdisConfig.deviceParams[devId].resolution);
}

Int32 Vdis_getDisplayId(VDIS_DEV vdDevId)
{
    Int32 displayId = 0;
#if defined(TI_814X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        displayId = gVdisModuleContext.displayId[0];
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        displayId = gVdisModuleContext.displayId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        displayId = gVdisModuleContext.displayId[1];
    }
    else
    {
        displayId = SYSTEM_LINK_ID_INVALID;
    }
#endif

#if defined(TI_8107_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        displayId = gVdisModuleContext.displayId[0];
    }
    else if(( vdDevId == VDIS_DEV_HDCOMP) || ( vdDevId == VDIS_DEV_DVO2))
    {
        displayId = gVdisModuleContext.displayId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        displayId = gVdisModuleContext.displayId[1];
    }
    else
    {
        displayId = SYSTEM_LINK_ID_INVALID;
    }
#endif

#if defined(TI_816X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        displayId = gVdisModuleContext.displayId[0];
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        displayId = gVdisModuleContext.displayId[1];
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        displayId = gVdisModuleContext.displayId[1];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        displayId = gVdisModuleContext.displayId[2];
    }
#endif

    return(displayId);
}

Int32 Vdis_getDisplayContextIndex(VDIS_DEV vdDevId)
{
    Int32 displayId = 0;

#if defined(TI_814X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        displayId = 0;
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        displayId = 0;
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        displayId = 1;
    }
    else
    {
        displayId = 0;
    }
#endif

#if defined(TI_8107_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        displayId = 0;
    }
    else if(( vdDevId == VDIS_DEV_HDCOMP) || ( vdDevId == VDIS_DEV_DVO2))
    {
        displayId = 0;
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        displayId = 1;
    }
    else
    {
        displayId = 0;
    }
#endif

#if defined(TI_816X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        displayId = 0;
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        displayId = 1;
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        displayId = 1;
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        displayId = 2;
    }
#endif

    return(displayId);
}

Int32 Vdis_getSwMsId(VDIS_DEV vdDevId)
{
    UInt32 swMsId = SYSTEM_LINK_ID_INVALID;

#ifdef TI_814X_BUILD
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }
#else
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[2];
    }
#endif

    return(swMsId);
}

Int32 Vdis_getSwMsContextIndex(VDIS_DEV vdDevId)
{
    Int32 swMsId = VDIS_DEV_MAX;

#ifdef TI_814X_BUILD
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = 0;
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = 0;
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = 1;
    }
    else
    {
        swMsId = VDIS_DEV_MAX;
    }
#else
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = 0;
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        swMsId = 1;
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = 1;
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = 2;
    }
#endif

    return(swMsId);
}


Int32 Vdis_setResolution(VDIS_DEV devId, UInt32 resolution)
{
    Int32 status = 0;
    DisplayLink_RtParams params;

    Char  gBuff[10];


    gVdisModuleContext.vdisConfig.deviceParams[devId].resolution = resolution;

    /* Check for tied vencs */
    if(!gVdisModuleContext.vdisConfig.tiedDevicesMask) {

        switch(devId)
        {
            case VDIS_DEV_HDMI:
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_OFF);
                switch(resolution) {
                    case VSYS_STD_1080P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_1080P_60);
                    break;
                    case VSYS_STD_1080P_50:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_1080P_50);
                    break;
                    case VSYS_STD_720P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_720P_60);
                    break;
                    case VSYS_STD_XGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_XGA_60);
                    break;
                    case VSYS_STD_SXGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_SXGA_60);
                    break;
                    default:
                        printf("\n Resolution not supported for HDMI!! \n");
                    break;
                }
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_ON);
            break;
#if defined(TI_816X_BUILD) || defined(TI_8107_BUILD)
            case VDIS_DEV_HDCOMP:
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_OFF);
                switch(resolution) {
                    case VSYS_STD_1080P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP,  VDIS_TIMINGS_1080P_60);
                    break;
                    case VSYS_STD_1080P_50:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP,  VDIS_TIMINGS_1080P_50);
                    break;
                    case VSYS_STD_720P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP,  VDIS_TIMINGS_720P_60);
                    break;
                    case VSYS_STD_XGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP,  VDIS_TIMINGS_XGA_60);
                    break;
                    case VSYS_STD_SXGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP,  VDIS_TIMINGS_SXGA_60);
                    break;
                    default:
                        printf("\n Resolution not supported for HDCOMP!! \n");
                    break;
                }
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_ON);
            break;
#endif
            case VDIS_DEV_DVO2:
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_OFF);
                switch(resolution) {
                    case VSYS_STD_1080P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_1080P_60);
                    break;
                    case VSYS_STD_1080P_50:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_1080P_50);
                    break;
                    case VSYS_STD_720P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_720P_60);
                    break;
                    case VSYS_STD_XGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_XGA_60);
                    break;
                    case VSYS_STD_SXGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_SXGA_60);
                    break;
                    default:
                        printf("\n Resolution not supported for DVO2!! \n");
                    break;
                }
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_ON);
            break;
            default:
                printf("\n Resolution not supported for DVO2!! \n");
             break;
        }

    }
    else{

#if defined(TI_814X_BUILD)
        if (devId == VDIS_DEV_HDMI || devId == VDIS_DEV_DVO2 ) {
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_OFF);
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_OFF);
            switch(resolution) {
                case VSYS_STD_1080P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_1080P_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_1080P_60);
                break;
                case VSYS_STD_1080P_50:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_1080P_50);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_1080P_50);
                break;
                case VSYS_STD_720P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_720P_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_720P_60);
                break;
                case VSYS_STD_XGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_XGA_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_XGA_60);
                break;
                case VSYS_STD_SXGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_SXGA_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_SXGA_60);
                break;
                default:
                    printf("\n Resolution not supported for this Venc!! \n");
                break;
            }
            sprintf(gBuff, "%d", gVdisModuleContext.vdisConfig.tiedDevicesMask);
            /* Tie DVO2 and HDMI from A8 side */
            Vdis_sysfsCmd(2,VDIS_SYSFSCMD_SETTIEDVENCS, gBuff);

            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_ON);
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_ON);
        }
        else {
            printf("\n Venc not supported!! \n");
        }
#endif

#if defined(TI_8107_BUILD)
        if (devId == VDIS_DEV_HDCOMP || devId == VDIS_DEV_HDMI)
        {
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI,   VDIS_OFF);
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_OFF);
            switch(resolution) {
                case VSYS_STD_1080P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,   VDIS_TIMINGS_1080P_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_1080P_60);
                break;
                case VSYS_STD_1080P_50:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,   VDIS_TIMINGS_1080P_50);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_1080P_50);
                break;
                case VSYS_STD_720P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,   VDIS_TIMINGS_720P_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_720P_60);
                break;
                case VSYS_STD_XGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,   VDIS_TIMINGS_XGA_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_XGA_60);
                break;
                case VSYS_STD_SXGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,   VDIS_TIMINGS_SXGA_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_SXGA_60);
                break;
                default:
                    printf("\n Resolution not supported for this Venc!! \n");
                break;
            }

            sprintf(gBuff, "%d", gVdisModuleContext.vdisConfig.tiedDevicesMask);
            /* Tie HDMI and HDCOMP from A8 side */
            Vdis_sysfsCmd(2,VDIS_SYSFSCMD_SETTIEDVENCS, gBuff);

            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI,   VDIS_ON);
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_ON);

        }
        else {
            printf("\n Venc not supported!! \n");
        }
#endif

#if defined(TI_816X_BUILD)
        if(gVdisModuleContext.vdisConfig.tiedDevicesMask == (VDIS_VENC_HDCOMP | VDIS_VENC_DVO2))
        {
        if (devId == VDIS_DEV_HDCOMP || devId == VDIS_DEV_DVO2) {
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_OFF);
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2,   VDIS_OFF);

            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC_CLKSRC, VDIS_SYSFS_DVO2, "aclk");

            switch(resolution) {
                case VSYS_STD_1080P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_1080P_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_1080P_60);
                break;
                case VSYS_STD_1080P_50:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_1080P_50);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_1080P_50);
                break;
                case VSYS_STD_720P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_720P_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_720P_60);
                break;
                case VSYS_STD_XGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_XGA_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_XGA_60);
                break;
                case VSYS_STD_SXGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_SXGA_60);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_SXGA_60);
                break;
                case VSYS_STD_480P:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_480P);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_480P);
                break;
                case VSYS_STD_576P:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_576P);
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_576P);
                break;
                default:
                    printf("\n Resolution not supported for this Venc!! \n");
                break;
            }
                /* Tie HDCOMP and DVO2 from A8 side */

            sprintf(gBuff, "%d", gVdisModuleContext.vdisConfig.tiedDevicesMask);
            /* Tie DVO2 and HDCOMP from A8 side */
            Vdis_sysfsCmd(2,VDIS_SYSFSCMD_SETTIEDVENCS, gBuff);

            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_ON);
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2  , VDIS_ON);

        }
        else if (devId == VDIS_DEV_HDMI) {
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_OFF);
            switch(resolution) {
                case VSYS_STD_1080P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_1080P_60);
                break;
                case VSYS_STD_1080P_50:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_1080P_50);
                break;
                case VSYS_STD_720P_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_720P_60);
                break;
                case VSYS_STD_XGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_XGA_60);
                break;
                case VSYS_STD_SXGA_60:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI,  VDIS_TIMINGS_SXGA_60);
                break;
                case VSYS_STD_480P:
                     Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_480P);
                break;
                case VSYS_STD_576P:
                    Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_576P);
                break;
                default:
                    printf("\n Resolution not supported for HDMI!! \n");
                break;
            }
            Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_ON);
        }
        else {
            printf("\n Venc not supported!! \n");
            }
        }
        if(gVdisModuleContext.vdisConfig.tiedDevicesMask == (VDIS_VENC_HDCOMP | VDIS_VENC_HDMI))
        {
            if (devId == VDIS_DEV_HDCOMP || devId == VDIS_DEV_HDMI)
            {
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_OFF);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI,   VDIS_OFF);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2,   VDIS_OFF);

                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC_CLKSRC, VDIS_SYSFS_HDMI, "dclk");
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC_CLKSRC, VDIS_SYSFS_HDCOMP, "dclk");
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC_CLKSRC, VDIS_SYSFS_DVO2, "aclk");

                switch(resolution) {
                    case VSYS_STD_1080P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_1080P_60);
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_1080P_60);
                    break;
                    case VSYS_STD_1080P_50:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_1080P_50);
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_1080P_50);
                    break;
                    case VSYS_STD_720P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_720P_60);
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_720P_60);
                    break;
                    case VSYS_STD_XGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_XGA_60);
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_XGA_60);
                    break;
                    case VSYS_STD_SXGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_SXGA_60);
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_SXGA_60);
                    break;
                    case VSYS_STD_480P:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_480P);
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_480P);
                    break;
                    case VSYS_STD_576P:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDCOMP, VDIS_TIMINGS_576P);
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_HDMI  , VDIS_TIMINGS_576P);
                    break;
                    default:
                        printf("\n Resolution not supported for HDMI!! \n");
                    break;
                }
                sprintf(gBuff, "%d", gVdisModuleContext.vdisConfig.tiedDevicesMask);
                /* Tie HDMI and HDCOMP from A8 side */
                Vdis_sysfsCmd(2,VDIS_SYSFSCMD_SETTIEDVENCS, gBuff);

                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_ON);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI  , VDIS_ON);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2,   VDIS_ON);
            }
            else if (devId == VDIS_DEV_DVO2) {
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_OFF);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_OFF);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_OFF);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC_CLKSRC, VDIS_SYSFS_HDMI, "dclk");
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC_CLKSRC, VDIS_SYSFS_HDCOMP, "dclk");
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC_CLKSRC, VDIS_SYSFS_DVO2, "aclk");
                switch(resolution) {
                    case VSYS_STD_1080P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_1080P_60);
                    break;
                    case VSYS_STD_1080P_50:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_1080P_50);
                    break;
                    case VSYS_STD_720P_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_720P_60);
                    break;
                    case VSYS_STD_XGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_XGA_60);
                    break;
                    case VSYS_STD_SXGA_60:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2,  VDIS_TIMINGS_SXGA_60);
                    break;
                    case VSYS_STD_480P:
                         Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_480P);
                    break;
                    case VSYS_STD_576P:
                        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETTIMINGS, VDIS_SYSFS_DVO2  , VDIS_TIMINGS_576P);
                    break;
                    default:
                        printf("\n Resolution not supported for DVO2!! \n");
                    break;
                }
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_ON);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_ON);
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_ON);
            }
            else {
                printf("\n Venc not supported!! \n");
            }
        }
#endif
    }

    /* Assuming SD will not be tied */
    if(devId == VDIS_DEV_SD){
        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_SD, VDIS_OFF);
        switch(resolution) {
            case VSYS_STD_NTSC:
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SET_MODE, VDIS_SYSFS_SD,  VDIS_MODE_NTSC);
            break;
            case VSYS_STD_PAL:
                Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SET_MODE, VDIS_SYSFS_SD,  VDIS_MODE_PAL);
            break;
            default:
                printf("\n Resolution not supported for SD!! \n");
            break;
        }
        Vdis_sysfsCmd(3,VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_SD, VDIS_ON);
    }

    params.resolution = resolution;

    status = System_linkControl(Vdis_getDisplayId(devId),
                                DISPLAY_LINK_CMD_CHANGE_RESOLUTION,
                                &params,
                                sizeof(DisplayLink_RtParams),
                                TRUE
                                );
    return status;
}

/* =============================================================================
 * Vdis module APIs
 * =============================================================================
 */
/**
 * \brief:
 *      Re-Initialize Venc parameters to perform display shift adjustments
 *      Call this API when ever want to do diplsy shift adjustments,
 *      perferably along with display resolution chnage
 *      Please note that this API should not be called before the Vsys_create()
 * \input:
 *      VDIS_DEV devId
 * \output:
 *      NA
 * \return
 *      NA
*/
Void Vdis_DisplayShiftAdjust(VDIS_DEV devId)
{
    Int32 status;

    gVdisModuleContext.vdisConfig.deviceParams[devId].
                       outputInfo.dvoVsPolarity = VDIS_POLARITY_ACT_LOW;
    gVdisModuleContext.vdisConfig.deviceParams[devId].
                       outputInfo.dvoHsPolarity = VDIS_POLARITY_ACT_LOW;

    status = System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_SET_DISPLAYCTRL_VENC_OUTPUT,
        &(gVdisModuleContext.vdisConfig.deviceParams[devId]),
        sizeof(VDIS_DEV_PARAM_S),
        TRUE
        );
    UTILS_assert(status==OSA_SOK);
}

Int32 Vdis_stopDrv(VDIS_DEV vdDevId)
{
    Int32 status;

    status = System_linkControl(Vdis_getDisplayId(vdDevId),
                                DISPLAY_LINK_CMD_STOP_DRV,
                                NULL,
                                0,
                                TRUE
                                );
    return status;

}

Int32 Vdis_startDrv(VDIS_DEV vdDevId)
{
    Int32 status;

    status = System_linkControl(Vdis_getDisplayId(vdDevId),
                                DISPLAY_LINK_CMD_START_DRV,
                                NULL,
                                0,
                                TRUE
                                );

    return status;

}

/* =============================================================================
 * Vdis internal APIs
 * =============================================================================
 */

 /**
 * \brief:
 *      Set Default Vdis Config
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
 *      NONE
*/
static Void Vdis_setDefaultConfig()
{
    UInt32 chId, devId;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    /* WARNING !!! !This requires changes for 8 channel usecases */
    for (devId = 0; devId<VDIS_DEV_MAX; devId++)
    {
        for(chId=0;chId<VDIS_CHN_MAX;chId++)
        {
            gVdisChnMapInfo[devId].ch2WinMap[chId]      = chId;
            gVdisChnMapInfo[devId].isEnableChn[chId]    = TRUE;
        }
    }
#else
    /* For 8168, first 16 channels goto display 0, next 16 to display 1 */
    devId = 0;
    for(chId=0;chId<VDIS_CHN_MAX/2;chId++)
    {
        gVdisChnMapInfo[devId].ch2WinMap[chId]      = chId;
        gVdisChnMapInfo[devId].isEnableChn[chId]    = TRUE;
    }
    for(chId=VDIS_CHN_MAX/2;chId<VDIS_CHN_MAX;chId++)
    {
        gVdisChnMapInfo[devId].ch2WinMap[chId]      = VDIS_CHN_INVALID;
        gVdisChnMapInfo[devId].isEnableChn[chId]    = TRUE;
    }


    for (devId = 1; devId<VDIS_DEV_MAX; devId++)
    {
         for(chId=0;chId<VDIS_CHN_MAX/2;chId++)
         {
             gVdisChnMapInfo[devId].ch2WinMap[chId]      = VDIS_CHN_INVALID;
             gVdisChnMapInfo[devId].isEnableChn[chId]    = TRUE;
         }
         for(chId=VDIS_CHN_MAX/2;chId<VDIS_CHN_MAX;chId++)
         {
             gVdisChnMapInfo[devId].ch2WinMap[chId]      = chId - (VDIS_CHN_MAX/2);
             gVdisChnMapInfo[devId].isEnableChn[chId]    = TRUE;
         }
    }
#endif
#if TI_VDIS_DEBUG
    for (devId = 0; devId<VDIS_DEV_MAX; devId++)
    {
        printf("\%s >> ----------- Device: %d \n", __FUNCTION__, devId);
        printf("\nCHANNEL | WINDOW | ENABLE(1)/DISABLE(0)\n");
        for(chId=0;chId<VDIS_CHN_MAX;chId++)
        {
            printf(" %6d | %6d | %6d\n", chId, gVdisChnMapInfo[devId].ch2WinMap[chId], gVdisChnMapInfo[devId].isEnableChn[chId]);
        }
    }
#endif
}

Int32 Vdis_delete()
{
    UInt32 devId;

    if (gVdisModuleContext.ipcFramesInVpssFromHostId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gVdisModuleContext.ipcFramesInVpssFromHostId);
        OSA_assert(gVdisModuleContext.ipcFramesOutHostId != SYSTEM_LINK_ID_INVALID);
        System_linkDelete(gVdisModuleContext.ipcFramesOutHostId);
    }
    for(devId = 0; devId < VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.swMsId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gVdisModuleContext.swMsId[devId] );
        }
        if(gVdisModuleContext.displayId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gVdisModuleContext.displayId[devId]);
        }
    }

    return 0;
}

static Void Vdis_copyVidFrameInfoLink2McFw(VIDEO_FRAMEBUF_S *dstBuf,
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

    VDIS_TRACE_FXN_EXIT("VidFrameInfo:"
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

static Void Vdis_copyVidFrameInfoMcFw2Link(VIDFrame_Buf *dstBuf,
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

    VDIS_TRACE_FXN_EXIT("VidFrameInfo:"
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


/**
    \brief Send filled video gBuffers to framework for display

    User calls this API to put full video frames for display

    \param pFrameBufList    [OUT]  List of video frames to be displayed

    \return ERROR_NONE on success
*/
Int32 Vdis_putFullVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList)
{
    VIDEO_FRAMEBUF_S *pSrcBuf;
    VIDFrame_Buf     *pDstBuf;
    VIDFrame_BufList  vidBufList;
    UInt32 i;
    Int status = 0;

    VDIS_TRACE_FXN_ENTRY("Num bufs put:%d",pFrameBufList->numFrames);
    vidBufList.numFrames = pFrameBufList->numFrames;
    for (i = 0; i < vidBufList.numFrames; i++)
    {
        pSrcBuf = &pFrameBufList->frames[i];
        pDstBuf = &vidBufList.frames[i];
        Vdis_copyVidFrameInfoMcFw2Link(pDstBuf,pSrcBuf);
    }
    if (vidBufList.numFrames)
    {
        status =
        IpcFramesOutLink_putFullVideoFrames(gVdisModuleContext.ipcFramesOutHostId,
                                            &vidBufList);
    }
    VDIS_TRACE_FXN_ENTRY("VIDFrame release status:%d",status);
    return 0;
}


/**
    \brief Give displayed video frames back to the application

    Buffers that are were previously put by Vdis_putFullVideoFrames can be
    freed back to the framework by invoking this API.

    \param pFrameBufList [IN]   List of video frames

    \return ERROR_NONE on success
*/
Int32 Vdis_getEmptyVideoFrames(VIDEO_FRAMEBUF_LIST_S *pFrameBufList, UInt32 timeout)
{
    VIDFrame_BufList  vidBufList;
    VIDFrame_Buf     *pInBuf;
    VIDEO_FRAMEBUF_S *pOutBuf;
    UInt32 i;

    VDIS_TRACE_FXN_ENTRY();
    pFrameBufList->numFrames = 0;
    vidBufList.numFrames = 0;
    IpcFramesOutLink_getEmptyVideoFrames(gVdisModuleContext.ipcFramesOutHostId,
                                         &vidBufList);

    pFrameBufList->numFrames = vidBufList.numFrames;
    for (i = 0; i < vidBufList.numFrames; i++)
    {
        pOutBuf = &pFrameBufList->frames[i];
        pInBuf = &vidBufList.frames[i];

        Vdis_copyVidFrameInfoLink2McFw(pOutBuf,pInBuf);
    }

    VDIS_TRACE_FXN_EXIT("NumFrames Received:%d",pFrameBufList->numFrames);
    return 0;
}

Void Vdis_swMs_PrintLayoutParams(VDIS_DEV vdDevId, SwMsLink_LayoutPrm * vdMosaicParam)
{
#if TI_VDIS_DEBUG
    UInt32 chNum;

    printf ("\n ***  SW Mosaic Parameters *** NumWindows %d,  onlyCh2WinMapChanged %d\n",
            vdMosaicParam->numWin,
            vdMosaicParam->onlyCh2WinMapChanged);

    printf( " \n"
            " ***  SW Mosaic Parameters *** \n"
            " \n"
            " Win | Ch  | Output     |  Output         | Low Cost \n"
            " Num | Num | Start X, Y |  Width x Height | ON / OFF \n"
            " --------------------------------------------------- \n"
            );

    for (chNum=0; chNum < VDIS_CHN_MAX; chNum++)
    {
        printf ("\n ChNum %d, WinMap %d, Enable %d",
        chNum,
        gVdisChnMapInfo[vdDevId].ch2WinMap[chNum],
        gVdisChnMapInfo[vdDevId].isEnableChn[chNum]);
    }
    printf( " \n");
#endif
}

static
Void  Vdis_copyChannelInfo(VIDEO_CHANNEL_LIST_INFO_S *dst,
                           System_LinkQueInfo        *src)
{
    Int i;

    OSA_COMPILETIME_ASSERT(OSA_ARRAYSIZE(src->chInfo)  ==
                           OSA_ARRAYSIZE(dst->chInfo));
    OSA_assert(src->numCh <= OSA_ARRAYSIZE(src->chInfo));
    dst->numCh = src->numCh;
    for (i = 0; i < src->numCh; i++)
    {
        dst->chInfo[i].width  = src->chInfo[i].width;
        dst->chInfo[i].height = src->chInfo[i].height;
    }
}

Int32 Vdis_getChannelInfo(VIDEO_CHANNEL_LIST_INFO_S *channelListInfo)
{
    System_LinkQueInfo inQueInfo;
    Int32 status;

    status =
    IpcFramesOutLink_getInQueInfo(gVdisModuleContext.ipcFramesOutHostId,
                                  &inQueInfo);
    if (status == ERROR_NONE)
    {
        Vdis_copyChannelInfo(channelListInfo,&inQueInfo);
    }
    else
    {
        inQueInfo.numCh = 0;
    }
    return status;
}

/**
 * \brief:
 *      Switch the queue from which frame are displayed
 * \input:
 *      vdDevId                -- Device id
 *      queueId                -- QueueID to switch to
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS          --    while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_switchActiveQueue(VDIS_DEV vdDevId, UInt32 queueId)
{
    Int32 status = ERROR_NONE;
    DisplayLink_SwitchActiveQueueParams prm;

    /* Multiple input queues is supported only for SD display
     * presently
     */
    if(vdDevId == VDIS_DEV_SD)
    {
        prm.activeQueueId = queueId;
        System_linkControl(Vdis_getDisplayId(vdDevId),
                           DISPLAY_LINK_CMD_SWITCH_ACTIVE_QUEUE,
                           &prm,
                           sizeof(prm),
                           TRUE);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}
/**
 * \brief:
 *      Switch the queue from which frame are displayed
 * \input:
 *      vdDevId                -- Device id
 *      channelId              -- channelId to switch to
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS          --    while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_switchActiveChannel(VDIS_DEV vdDevId, UInt32 channelId)
{
    Int32 status = ERROR_NONE;
    UInt32 displayID;
    DisplayLink_SwitchChannelParams params = {0};

    if(gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
    {
        displayID = gVdisModuleContext.displayId[vdDevId];
        params.activeChId = channelId;
        System_linkControl(displayID,
                           DISPLAY_LINK_CMD_SWITCH_CH,
                           &params,
                           sizeof(params),
                           TRUE);
    }

    return status;
}

/**
 * \brief:
 *      Switch the channel Id for SDTV live bypass path
 * \input:
 *      vdDevId                -- Device id
 *      chId                -- chID to switch to
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS          --    while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdis_switchSDTVChId(VDIS_DEV vdDevId, UInt32 chId)
{
    Int32 status = ERROR_NONE;
    DisplayLink_SwitchChannelParams prm;


    /* Multiple input queues is supported only for SD display
     * presently
     */
    if(vdDevId == VDIS_DEV_SD)
    {
        Vcap_setExtraFramesChId(chId);

        prm.activeChId = chId;
        System_linkControl(Vdis_getDisplayId(vdDevId),
                           DISPLAY_LINK_CMD_SWITCH_CH,
                           &prm,
                           sizeof(prm),
                           TRUE);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}
/**
  * \brief:
  *     get channel input information from win ID
  * \input:
  *     vdDevId     -- Mosaic id
  *     winId       -- win ID which we want to get the channel information
  * \output:
  *     psChnlInfo  -- channel input infomation
  * \return:
  *     Always return success
  */

Int32 Vdis_getChnlInfoFromWinId(VDIS_DEV vdDevId, UInt32 winId,WINDOW_S * psChnlInfo)
{
    SwMsLink_WinInfo inputInfo;
    UInt32 swMsId = 0;

#if defined(TI_814X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }
#endif

#if defined(TI_8107_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(( vdDevId == VDIS_DEV_DVO2) || ( vdDevId == VDIS_DEV_HDCOMP))
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }
#endif


#if defined(TI_816X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[2];
    }
#endif

    inputInfo.winId = winId;

    System_linkControl(swMsId, SYSTEM_SW_MS_LINK_CMD_GET_INPUT_CHNL_INFO, &(inputInfo), sizeof(inputInfo), TRUE);

    psChnlInfo->width       = inputInfo.width;
    psChnlInfo->height      = inputInfo.height;
    psChnlInfo->start_X     = inputInfo.startX;
    psChnlInfo->start_Y     = inputInfo.startY;

    return 0;
}


/**
  * \brief:
  *     set channel crop settings from window ID
  * \input:
  *     vdDevId     -- Mosaic id
  *     winId       -- win ID which we want to get the channel information
  *     cropParam   -- crop settings to apply
  * \return:
  *     Always return success
  */

Int32 Vdis_SetCropParam(VDIS_DEV vdDevId, UInt32 winId,WINDOW_S cropParam)
{

    SwMsLink_WinInfo inputInfo;
    UInt32 swMsId = 0;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if( vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }
#endif

#if defined(TI_8107_BUILD)

    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(( vdDevId == VDIS_DEV_DVO2) || ( vdDevId == VDIS_DEV_HDCOMP))
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else
    {
        swMsId = SYSTEM_LINK_ID_INVALID;
    }

#endif

#if defined(TI_816X_BUILD)
    if( vdDevId == VDIS_DEV_HDMI)
    {
        swMsId = gVdisModuleContext.swMsId[0];
    }
    else if(vdDevId == VDIS_DEV_HDCOMP)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_DVO2)
    {
        swMsId = gVdisModuleContext.swMsId[1];
    }
    else if(vdDevId == VDIS_DEV_SD)
    {
        swMsId = gVdisModuleContext.swMsId[2];
    }
#endif

    inputInfo.winId     = winId;
    inputInfo.width     = cropParam.width;
    inputInfo.height    = cropParam.height;
    inputInfo.startX    = cropParam.start_X;
    inputInfo.startY    = cropParam.start_Y;

    System_linkControl(swMsId, SYSTEM_SW_MS_LINK_CMD_SET_CROP_PARAM, &(inputInfo), sizeof(inputInfo), TRUE);

    return 0;
}

/**
  * \brief:
  *     Flush buffers in SwMs
  * \input:
  *     vdDevId     -- Mosaic id
  *     vdispChnId  -- SwMS channel ID
  *     holdLastFrame -- Flag to indicate whether last frame should be held in SwMS or not
  * \return:
  *     Always return success
  */

Int32 Vdis_flushSwMs(VDIS_DEV vdDevId,VDIS_CHN vdispChnId,UInt32 holdLastFrame)
{
    Int32 swMsId = Vdis_getSwMsId(vdDevId);
    SwMsLink_FlushParams flushPrm;
    Int32 status;

    if (VDIS_CHN_ALL == vdispChnId)
    {
        vdispChnId = SYSTEM_SW_MS_ALL_CH_ID;
    }
    flushPrm.chNum         = vdispChnId;
    flushPrm.holdLastFrame = holdLastFrame;

    status =
    System_linkControl(swMsId,
                       SYSTEM_SW_MS_LINK_CMD_FLUSH_BUFFERS,
                       &flushPrm, sizeof(flushPrm), TRUE);

    return status;
}

Int32 Vdis_setAvsyncConfig(VDIS_DEV vdDevId,
                           AvsyncLink_LinkSynchConfigParams *avsyncConfig)
{
    Int32 status = ERROR_NONE;

    if (vdDevId < VDIS_DEV_MAX)
    {
        gVdisModuleContext.avsyncCfg[vdDevId] = *avsyncConfig;
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;

}

Int32 Vdis_getAvsyncConfig(VDIS_DEV vdDevId,
                          AvsyncLink_LinkSynchConfigParams *avsyncConfig)
{
    Int32 status = ERROR_NONE;

    OSA_assert(gVdisModuleContext.initDone = TRUE);
    if (vdDevId < VDIS_DEV_MAX)
    {
        *avsyncConfig = gVdisModuleContext.avsyncCfg[vdDevId];
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}

Int32 Vdis_setVideoBackendDelay(VDIS_DEV vdDevId,
                                UInt32 backendDelayMS)
{
    Int32 status = ERROR_NONE;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_VideoBackendDelayParams  delayParams;

        delayParams.backendDelayMS = backendDelayMS;
        delayParams.displayLinkID = Vdis_getDisplayId(vdDevId);

        status = Avsync_setVideoBackEndDelay(&delayParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

Int32 Vdis_seek(VDIS_DEV vdDevId,
                VDIS_CHN VdChnId,
                UInt64 seekAudPTS,
                UInt64 seekVidPTS,
                UInt32 seqId)
{
    Int32 status = ERROR_NONE;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_SeekParams  seekParams;

        seekParams.displayLinkID   = Vdis_getDisplayId(vdDevId);
        if (VDIS_CHN_ALL == VdChnId)
        {
            seekParams.chNum       = AVSYNC_ALL_CHANNEL_ID;
        }
        else
        {
            seekParams.chNum       = VdChnId;
        }
        seekParams.seekAudPTS  = seekAudPTS;
        seekParams.seekVidPTS  = seekVidPTS;
        seekParams.displaySeqId = seqId;
        status = Avsync_seekPlayback(&seekParams);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}


Int32 Vdis_setWallTimeBase(UInt64 wallTimeBase)
{
    return (Avsync_setWallTimeBase(wallTimeBase));
}


Int32 Vdis_setFirstVidPTS(VDIS_DEV vdDevId,
                          VDIS_CHN VdChnId,
                          UInt64 firstVidPTS)
{
    Int status = ERROR_NONE;
    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_FirstVidPTSParams ptsParams;

        ptsParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        ptsParams.chNum     = VdChnId;
        ptsParams.firstVidPTS = firstVidPTS;

        status = Avsync_setFirstVidPTS(&ptsParams);
    }
    else
    {
        status = ERROR_FAIL;
    }


    return status;
}

Int32 Vdis_setFirstAudPTS(VDIS_DEV vdDevId,
                          VDIS_CHN VdChnId,
                          UInt64 firstAudPTS)
{
    Int status = ERROR_NONE;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_FirstAudPTSParams ptsParams;

        ptsParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        ptsParams.chNum     = VdChnId;
        ptsParams.firstAudPTS = firstAudPTS;

        status = Avsync_setFirstAudPTS(&ptsParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

Int32 Vdis_resetChPlayerTime(VDIS_DEV vdDevId,
                             VDIS_CHN VdChnId)
{
    Int status = ERROR_NONE;
    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_ResetPlayerTimerParams resetParams;

        resetParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        resetParams.chNum     = VdChnId;

        status = Avsync_resetPlayerTimer(&resetParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

Int32 Vdis_resetMosaicPlayerTime(VDIS_DEV vdDevId)
{
    Int status = ERROR_NONE;
    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_ResetPlayerTimerParams resetParams;

        resetParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        resetParams.chNum     = AVSYNC_ALL_CHANNEL_ID;

        status = Avsync_resetPlayerTimer(&resetParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}


Int32 Vdis_setChPlayerStatePlay(VDIS_DEV vdDevId,
                                VDIS_CHN VdChnId,
                                UInt32 seqId)
{
    Int status = ERROR_NONE;

    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_PlayParams playParams;

        playParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        playParams.chNum     = VdChnId;
        playParams.displaySeqId = seqId;

        status = Avsync_doPlay(&playParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

Int32 Vdis_setMosaicPlayerStatePlay(VDIS_DEV vdDevId,
                                    UInt32 seqId)
{
    Int status = ERROR_NONE;
    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_PlayParams playParams;

        playParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        playParams.chNum     = AVSYNC_ALL_CHANNEL_ID;
        playParams.displaySeqId = seqId;

        status = Avsync_doPlay(&playParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

Int32 Vdis_scanCh(VDIS_DEV vdDevId,
                  VDIS_CHN VdChnId,
                  UInt32 frameDisplayDurationMS,
                  UInt32 seqId)
{
    Int status = ERROR_NONE;
    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_ScanParams scanParams;

        scanParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        scanParams.chNum     = VdChnId;
        scanParams.frameDisplayDurationMS = frameDisplayDurationMS;
        scanParams.displaySeqId = seqId;

        status = Avsync_doScan(&scanParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

Int32 Vdis_scanMosaic(VDIS_DEV vdDevId,
                      UInt32 frameDisplayDurationMS,
                      UInt32 seqId)
{
    Int status = ERROR_NONE;
    if ((Vdis_getDisplayContextIndex(vdDevId) < OSA_ARRAYSIZE(gVdisModuleContext.displayId)
        &&
        (SYSTEM_LINK_ID_INVALID !=Vdis_getDisplayId(vdDevId))))
    {

        Avsync_ScanParams scanParams;

        scanParams.displayLinkID = Vdis_getDisplayId(vdDevId);
        scanParams.chNum     = AVSYNC_ALL_CHANNEL_ID;
        scanParams.frameDisplayDurationMS = frameDisplayDurationMS;
        AVSYNC_INIT_SEQID(scanParams.displaySeqId);
        scanParams.displaySeqId = seqId;

        status = Avsync_doScan(&scanParams);
    }
    else
    {
        status = ERROR_FAIL;
    }

    return status;
}

Int32 Vdis_printAvsyncStatistics()
{
    Int status = ERROR_NONE;

    Avsync_printStats();
    return status;
}

/**
  * \brief:
  *     Creates and executes sysfs command
  * \input:
  *     numArgs     -- number of arguments in sysfs command
  *     ....        -- Variable arguments, first string followed by integer Args
  * \return:
  *     Always return success
  */
Int32 Vdis_sysfsCmd(UInt32 numArgs, ...)
{
    Char    sysfsBuffer[100];
    va_list listPointer;

    va_start(listPointer, numArgs);

    if(numArgs == 2)
        VDIS_SYSFSCMD_ARG2(sysfsBuffer, va_arg(listPointer, String), va_arg(listPointer, String));
    if(numArgs == 3)
        VDIS_SYSFSCMD_ARG3(sysfsBuffer, va_arg(listPointer, String), va_arg(listPointer, Int32), va_arg( listPointer, String));
    if(numArgs == 4)
        VDIS_SYSFSCMD_ARG4(sysfsBuffer, va_arg(listPointer, String), va_arg(listPointer, Int32), va_arg( listPointer, Int32), va_arg( listPointer, String));

    va_end(listPointer);

    return 0;
}

/**
  * \brief:
  *     Checks the status of grpx blender
  * \input:
  *     grpxId     -- graphics blender id
  * \return:
  *     return success if graphics blender is on
  *     return failure if graphics blender is off
  */
Int32 Vdis_isGrpxOn(Int32 grpxId, String sysfsBuffer, Int32 * r)
{
    char filename[100];
    Int32 retVal;

    VDIS_CMD_IS_GRPX_ON(filename, sysfsBuffer, VDIS_SYSFSCMD_GET_GRPX, grpxId, 1, retVal);
    *r = retVal;

    return 0;
}



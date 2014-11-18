/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


/** ========================================================================== */
/** Include Files */
/** ========================================================================== */

#include "system_priv_m3vpss.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>

/**
 *  App_dispConfigHdmi
 *  Configures the display controller to connect the paths, enabling muxes
 *  and configuring blenders and VENCs.
 */
Int32 System_hdmiConfig(UInt32 displayRes)
{
    Int32 retVal;
    Vps_HdmiChipId hdmiId;
    Vps_SiI9022aHpdPrms hpdPrms;
    Vps_SiI9022aModeParams modePrms;

    retVal = FVID2_control(gSystem_objVpss.hdmiHandle,
                           IOCTL_VPS_SII9022A_GET_DETAILED_CHIP_ID,
                           &hdmiId, NULL);

    if (FVID2_SOK == retVal)
    {
#if 0                                                      // def
                                                           // SYSTEM_DEBUG_DISPLAY
        Vps_printf("DevId %x Prod RevId %x TPI RevId %x HDCP RevId %x\n",
                   hdmiId.deviceId,
                   hdmiId.deviceProdRevId, hdmiId.tpiRevId, hdmiId.hdcpRevTpi);
#endif
    }
    else
    {
        Vps_printf(" HDMI: ERROR: Could not Get Detailed Chip Id !!!\n");
    }

    retVal = FVID2_control(gSystem_objVpss.hdmiHandle,
                           IOCTL_VPS_SII9022A_QUERY_HPD, &hpdPrms, NULL);

    if (FVID2_SOK == retVal)
    {
#if 0                                                      // def
                                                           // SYSTEM_DEBUG_DISPLAY
        Vps_printf("hpdEvtPending %x busError %x hpdStatus %x\n",
                   hpdPrms.hpdEvtPending, hpdPrms.busError, hpdPrms.hpdStatus);
#endif
    }
    else
    {
        Vps_printf("Could not Get HPD\n");
    }

    modePrms.standard = displayRes;

    retVal = FVID2_control(gSystem_objVpss.hdmiHandle,
                           IOCTL_VPS_VIDEO_ENCODER_SET_MODE, &modePrms, NULL);

    if (FVID2_SOK == retVal)
    {
#ifdef SYSTEM_DEBUG_DISPLAY
        Vps_printf(" %d: HDMI: HDMI Config ... DONE !!!\n", Utils_getCurTimeInMsec());
#endif
    }
    else
    {
#ifdef SYSTEM_DEBUG_DISPLAY
        Vps_printf(" %d: HDMI: HDMI Config ... ERROR !!!\n", Utils_getCurTimeInMsec());
#endif
    }

    return (retVal);
}

/**
 *  App_dispConfigHdmi
 *  Configures the display controller to connect the paths, enabling muxes
 *  and configuring blenders and VENCs.
 */
Int32 System_sii9134Config(UInt32 displayRes)
{
    Int32 retVal;
    Vps_HdmiChipId hdmiId;
    Vps_SiI9022aHpdPrms hpdPrms;
    Vps_SiI9022aModeParams modePrms;

    retVal = FVID2_control(gSystem_objVpss.hdmiHandle,
                           IOCTL_VPS_SII9022A_GET_DETAILED_CHIP_ID,
                           &hdmiId, NULL);

    if (FVID2_SOK == retVal)
    {
#if 0                                                      // def
                                                           // SYSTEM_DEBUG_DISPLAY
        Vps_printf("DevId %x Prod RevId %x TPI RevId %x HDCP RevId %x\n",
                   hdmiId.deviceId,
                   hdmiId.deviceProdRevId, hdmiId.tpiRevId, hdmiId.hdcpRevTpi);
#endif
    }
    else
    {
        Vps_printf(" HDMI: ERROR: Could not Get Detailed Chip Id !!!\n");
    }

    retVal = FVID2_control(gSystem_objVpss.hdmiHandle,
                           IOCTL_VPS_SII9022A_QUERY_HPD, &hpdPrms, NULL);

    if (FVID2_SOK == retVal)
    {
#if 0                                                      // def
                                                           // SYSTEM_DEBUG_DISPLAY
        Vps_printf("hpdEvtPending %x busError %x hpdStatus %x\n",
                   hpdPrms.hpdEvtPending, hpdPrms.busError, hpdPrms.hpdStatus);
#endif
    }
    else
    {
        Vps_printf("Could not Get HPD\n");
    }

    modePrms.standard = displayRes;

    retVal = FVID2_control(gSystem_objVpss.hdmiHandle,
                           IOCTL_VPS_VIDEO_ENCODER_SET_MODE, &modePrms, NULL);

    if (FVID2_SOK == retVal)
    {
#ifdef SYSTEM_DEBUG_DISPLAY
        Vps_printf(" %d: HDMI: HDMI Config ... DONE !!!\n", Utils_getCurTimeInMsec());
#endif
    }
    else
    {
#ifdef SYSTEM_DEBUG_DISPLAY
        Vps_printf(" %d: HDMI: HDMI Config ... ERROR !!!\n", Utils_getCurTimeInMsec());
#endif
    }

    return (retVal);
}

Int32 System_hdmiCreate(UInt32 displayRes, System_PlatformBoardId boardId)
{
    Vps_VideoEncoderCreateParams encCreateParams;
    Vps_VideoEncoderCreateStatus encCreateStatus;
    Int32 retVal;

    /* Open HDMI Tx */
    encCreateParams.deviceI2cInstId = Vps_platformGetI2cInstId();
    encCreateParams.deviceI2cAddr =
        Vps_platformGetVidEncI2cAddr(FVID2_VPS_VID_ENC_SII9022A_DRV);
    encCreateParams.inpClk = 0;
    encCreateParams.hdmiHotPlugGpioIntrLine = 0;

    if (SYSTEM_PLATFORM_BOARD_CZ == boardId)
    {
        encCreateParams.deviceI2cAddr =
            Vps_platformGetVidEncI2cAddr(FVID2_VPS_VID_ENC_SII9134_DRV);
        encCreateParams.syncMode = VPS_VIDEO_ENCODER_EMBEDDED_SYNC;
        encCreateParams.clkEdge = TRUE;

        gSystem_objVpss.hdmiHandle = FVID2_create(FVID2_VPS_VID_ENC_SII9134_DRV,
                                              0,
                                              &encCreateParams,
                                              &encCreateStatus,
                                              NULL);

        if (NULL == gSystem_objVpss.hdmiHandle)
        {
            Vps_printf(
                "%s: Error %d @ line %d\n", __FUNCTION__, 0, __LINE__);
            return FVID2_EFAIL;
        }

        retVal = System_sii9134Config(displayRes);
        UTILS_assert(retVal == FVID2_SOK);

        return retVal;
    }

    if (SYSTEM_PLATFORM_BOARD_VS == boardId)
    {
        encCreateParams.syncMode = VPS_VIDEO_ENCODER_EMBEDDED_SYNC;
        encCreateParams.clkEdge = FALSE;
    }
    else if (SYSTEM_PLATFORM_BOARD_VC == boardId)
    {
        encCreateParams.syncMode = VPS_VIDEO_ENCODER_EXTERNAL_SYNC;
        encCreateParams.clkEdge = TRUE;
    }
    else if (SYSTEM_PLATFORM_BOARD_CATALOG == boardId)
    {
        encCreateParams.syncMode = VPS_VIDEO_ENCODER_EXTERNAL_SYNC;
        encCreateParams.clkEdge = TRUE;
    }
    else if (SYSTEM_PLATFORM_BOARD_ETV == boardId)
    {
        encCreateParams.syncMode = VPS_VIDEO_ENCODER_EXTERNAL_SYNC;
        encCreateParams.clkEdge = TRUE;
    }
    else
    {
        encCreateParams.syncMode = VPS_VIDEO_ENCODER_EMBEDDED_SYNC;
        encCreateParams.clkEdge = FALSE;
    }
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    encCreateParams.clkEdge = FALSE;
#endif                                                     /* TI_814X_BUILD */
    gSystem_objVpss.hdmiHandle = FVID2_create(FVID2_VPS_VID_ENC_SII9022A_DRV,
                                              0,
                                              &encCreateParams,
                                              &encCreateStatus, NULL);

    if (NULL == gSystem_objVpss.hdmiHandle)
    {
        Vps_printf("%s: Error %d @ line %d\n", __FUNCTION__, 0, __LINE__);
        return FVID2_EFAIL;
    }

    retVal = System_hdmiConfig(displayRes);
    UTILS_assert(retVal == FVID2_SOK);

    return retVal;
}

Int32 System_hdmiStart(UInt32 displayRes, System_PlatformBoardId boardId)
{
    Int32 retVal;

#ifdef SYSTEM_DEBUG_DISPLAY
    Vps_printf(" %d: HDMI: Starting HDMI Transmitter ... !!!\n",
               Utils_getCurTimeInMsec());
#endif

    System_hdmiCreate(displayRes, boardId);

    retVal = FVID2_start(gSystem_objVpss.hdmiHandle, NULL);
    if (FVID2_SOK != retVal)
    {
        Vps_printf("%s: Error %d @ line %d\n", __FUNCTION__, retVal, __LINE__);
        return (retVal);
    }

#ifdef SYSTEM_DEBUG_DISPLAY
    Vps_printf(" %d: HDMI: Starting HDMI Transmitter ... DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return retVal;
}

Int32 System_hdmiStop()
{
    Int32 retVal;

#ifdef SYSTEM_DEBUG_DISPLAY
    Vps_printf(" %d: HDMI: Stopping HDMI Transmitter ... !!!\n",
               Utils_getCurTimeInMsec());
#endif

    retVal = FVID2_stop(gSystem_objVpss.hdmiHandle, NULL);
    if (FVID2_SOK != retVal)
    {
        Vps_printf("%s: Error %d @ line %d\n", __FUNCTION__, retVal, __LINE__);
        return (retVal);
    }

    System_hdmiDelete();

#ifdef SYSTEM_DEBUG_DISPLAY
    Vps_printf(" %d: HDMI: Stopping HDMI Transmitter ... DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return retVal;
}

Int32 System_hdmiDelete()
{
    Int32 retVal;

    retVal = FVID2_delete(gSystem_objVpss.hdmiHandle, NULL);
    if (FVID2_SOK != retVal)
    {
        Vps_printf("%s: Error %d @ line %d\n", __FUNCTION__, retVal, __LINE__);
        return retVal;
    }

    return retVal;
}

Int32 System_displayUnderflowPrint(Bool runTimePrint, Bool clearAll)
{
    if (runTimePrint)
    {
        Vps_rprintf(" %u: DISPLAY: UNDERFLOW COUNT: HDMI(BP0) %u, HDDAC(BP0) %d, DVO2(BP1) %u, SDDAC(SEC1) %u \n",
            Utils_getCurTimeInMsec(), gSystem_objVpss.displayUnderflowCount[0],
            gSystem_objVpss.displayUnderflowCount[1],
            gSystem_objVpss.displayUnderflowCount[2],
            gSystem_objVpss.displayUnderflowCount[3]);
    }
    else
    {
        Vps_printf(" %u: DISPLAY: UNDERFLOW COUNT: HDMI(BP0) %d, HDDAC(BP0) %d, DVO2(BP1) %d, SDDAC(SEC1) %d \n",
            Utils_getCurTimeInMsec(), gSystem_objVpss.displayUnderflowCount[0],
            gSystem_objVpss.displayUnderflowCount[1],
            gSystem_objVpss.displayUnderflowCount[2],
            gSystem_objVpss.displayUnderflowCount[3]);
    }

    if (clearAll)
    {
        System_displayUnderflowCheck(TRUE);
    }

    return FVID2_SOK;
}

Int32 VpsHal_vpsClkcModuleCountUnderFlow(Bool clearAll,
                                         UInt32 * vencUnderflowCounter);

Int32 System_displayUnderflowCheck(Bool clearAll)
{
    if (clearAll)
    {
        gSystem_objVpss.displayUnderflowCount[0] = 0;
        gSystem_objVpss.displayUnderflowCount[1] = 0;
        gSystem_objVpss.displayUnderflowCount[2] = 0;
        gSystem_objVpss.displayUnderflowCount[3] = 0;
    }
    VpsHal_vpsClkcModuleCountUnderFlow(clearAll,
                                       gSystem_objVpss.displayUnderflowCount);

    return FVID2_SOK;
}

/**
 *  System_dispSetPixClk
 *  Configure Pixel Clock.
 */
Int32 System_dispSetPixClk()
{
    Int32 retVal = FVID2_SOK;

    gSystem_objVpss.systemDrvHandle = FVID2_create(FVID2_VPS_VID_SYSTEM_DRV,
                                                   0, NULL, NULL, NULL);
    if (NULL == gSystem_objVpss.systemDrvHandle)
    {
        Vps_printf("%s: Error %d @ line %d\n", __FUNCTION__, retVal, __LINE__);
        return (FVID2_EFAIL);
    }

    gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_D].outputVenc = VPS_SYSTEM_VPLL_OUTPUT_VENC_D;
    retVal = FVID2_control(gSystem_objVpss.systemDrvHandle,
                           IOCTL_VPS_VID_SYSTEM_SET_VIDEO_PLL,
                           &(gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_D]), NULL);
    if (FVID2_SOK != retVal)
    {
        Vps_printf("%s: Error %d @ line %d\n", __FUNCTION__, retVal, __LINE__);
        return (retVal);
    }

#if defined(TI_816X_BUILD) 
    gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_A].outputVenc = VPS_SYSTEM_VPLL_OUTPUT_VENC_A;
    retVal = FVID2_control(gSystem_objVpss.systemDrvHandle,
                           IOCTL_VPS_VID_SYSTEM_SET_VIDEO_PLL,
                           &(gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_A]), NULL);
    if (FVID2_SOK != retVal)
    {
        Vps_printf("%s: Error %d @ line %d\n", __FUNCTION__, retVal, __LINE__);
        return (retVal);
    }
#endif

    FVID2_delete(gSystem_objVpss.systemDrvHandle, NULL);

    return (FVID2_SOK);
}

/**
 *  System_dispCheckStopList
 *  Frees the VPDMA lists that are busy.
 */
Int32 System_dispCheckStopList()
{
    volatile UInt32 status = 0;
    volatile UInt32 retryCount = 60;

    /*[BOOTSCREEN] Entry: This block of code is to check the VPDMA lists,
        and wait until VPDMA list becomes NOT busy.
        This is required since a VPDMA list is started during Boot Time Splash Screen
    */
    if((*(UInt32 *)VPS_MODULE_CLK) == 2)
    {
        /* VPSS is powered ON */
        do
        {
            status = *(volatile UInt32 *)VPDMA_LIST_STAT_SYNC & 0x00FF0000;

            if( status == 0 )
            {
                /* all lists are free */
                break;
            }

            Vps_printf(" %d: SYSTEM: VPDMA is Busy (0x%08x) !!!\n",
                        Utils_getCurTimeInMsec(), status);
            Task_sleep(1000);
        }while(retryCount--);

        status = *(volatile UInt32 *)VPDMA_LIST_STAT_SYNC & 0x00FF0000;
        if( status == 0 )
        {
            Vps_printf(" %d: SYSTEM: All VPDMA Free !!! \n",
                    Utils_getCurTimeInMsec());
        }
        else
        {
            Vps_printf(" %d: SYSTEM: VPDMA is Busy (0x%08x) !!! ---> ERROR !!!!\n",
                        Utils_getCurTimeInMsec(), status);
        }
    }
    /*[BOOTSCREEN] Exit */

    return FVID2_SOK;
}

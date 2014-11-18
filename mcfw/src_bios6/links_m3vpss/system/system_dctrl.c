/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include "system_priv_m3vpss.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include "system_dctrl_modeInfo.h"

#if defined(TI_816X_BUILD)
/* Display Controller Configuration */
/* To tie DVO2 and HDCOMP together refer following Mesh */
Vps_DcConfig gSystem_dctrlTriDisplayConfigDvo2 = {
    VPS_DC_USERSETTINGS,                                   /* Use Case */
    /* Edge information */
    {
     {VPS_DC_BP0_INPUT_PATH, VPS_DC_VCOMP_MUX}     ,
     {VPS_DC_VCOMP_MUX, VPS_DC_VCOMP}     ,
     {VPS_DC_CIG_NON_CONSTRAINED_OUTPUT, VPS_DC_HDMI_BLEND}     ,
     {VPS_DC_BP1_INPUT_PATH, VPS_DC_HDCOMP_MUX}     ,
     {VPS_DC_HDCOMP_MUX, VPS_DC_CIG_PIP_INPUT}     ,
     {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_DVO2_BLEND}     ,
     {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_HDCOMP_BLEND}     ,
     {VPS_DC_SEC1_INPUT_PATH, VPS_DC_SDVENC_MUX}     ,
     {VPS_DC_SDVENC_MUX, VPS_DC_SDVENC_BLEND}     ,
     {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDMI_BLEND}     ,
     {VPS_DC_GRPX1_INPUT_PATH, VPS_DC_HDCOMP_BLEND}     ,
     {VPS_DC_GRPX1_INPUT_PATH, VPS_DC_DVO2_BLEND}
     }
    ,
    12,
    /* VENC information */
    {
     /* Mode information */
     {
      {VPS_DC_VENC_HDMI, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_HDCOMP, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_DVO2, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_SD, {FVID2_STD_NTSC}
       }
      }
     ,
     (VPS_DC_VENC_DVO2 | VPS_DC_VENC_HDCOMP),              /* Tied VENC bit
                                                            * mask */
     4u                                                    /* Number of VENCs
                                                            */
     }
};

/* To tie HDMI and HDCOMP together refer following Mesh */
Vps_DcConfig gSystem_dctrlTriDisplayConfigHdmi = {
    VPS_DC_USERSETTINGS,                                   /* Use Case */
    /* Edge information */
    {
     {VPS_DC_BP0_INPUT_PATH, VPS_DC_HDCOMP_MUX},
     {VPS_DC_HDCOMP_MUX, VPS_DC_CIG_PIP_INPUT},
     {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_HDMI_BLEND},
     {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_HDCOMP_BLEND},
     {VPS_DC_BP1_INPUT_PATH, VPS_DC_VCOMP_MUX},
     {VPS_DC_VCOMP_MUX, VPS_DC_VCOMP},
     {VPS_DC_CIG_NON_CONSTRAINED_OUTPUT, VPS_DC_DVO2_BLEND},
     {VPS_DC_SEC1_INPUT_PATH, VPS_DC_SDVENC_MUX},
     {VPS_DC_SDVENC_MUX, VPS_DC_SDVENC_BLEND},
     {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDMI_BLEND},
     {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDCOMP_BLEND},
     {VPS_DC_GRPX1_INPUT_PATH, VPS_DC_DVO2_BLEND},
     {VPS_DC_GRPX2_INPUT_PATH, VPS_DC_SDVENC_BLEND},
     } ,
    13,
    /* VENC information */
    {
     /* Mode information */
     {
      {VPS_DC_VENC_HDMI, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_HDCOMP, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_DVO2, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_SD, {FVID2_STD_NTSC}
       }
      }
     ,
     (VPS_DC_VENC_HDMI | VPS_DC_VENC_HDCOMP),              /* Tied VENC bit
                                                            * mask */
     4u                                                    /* Number of VENCs */
     }
};
#endif


#if defined(TI_814X_BUILD)
/* Display Controller Configuration */
/* To tie DVO2 and HDCOMP together refer following Mesh */
Vps_DcConfig gSystem_dctrlTriDisplayConfig = {
    VPS_DC_USERSETTINGS,                                   /* Use Case */
    /* Edge information */
    {
     {VPS_DC_BP0_INPUT_PATH, VPS_DC_HDCOMP_MUX}     ,
     {VPS_DC_HDCOMP_MUX, VPS_DC_CIG_PIP_INPUT}      ,
     {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_HDMI_BLEND}     ,
     {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_DVO2_BLEND}     ,
     {VPS_DC_SEC1_INPUT_PATH, VPS_DC_SDVENC_MUX}    ,
     {VPS_DC_SDVENC_MUX, VPS_DC_SDVENC_BLEND}       ,
     {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDMI_BLEND}   ,
     {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_DVO2_BLEND}  ,
     {VPS_DC_GRPX2_INPUT_PATH, VPS_DC_SDVENC_BLEND}
    }

    ,
    9,
    /* VENC information */
    {
     /* Mode information */
     {
      {VPS_DC_VENC_HDMI, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_DVO2, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_SD, {FVID2_STD_NTSC}
       }
      }
     ,
     (VPS_DC_VENC_HDMI | VPS_DC_VENC_DVO2),                /* Tied VENC bit
                                                            * mask */
     3u                                                    /* Number of VENCs
                                                            */
     }
};
#endif


#if defined(TI_8107_BUILD)
/* Display Controller Configuration */
/* To tie DVO2 and HDCOMP together refer following Mesh */
Vps_DcConfig gSystem_dctrlTriDisplayConfig = {
    VPS_DC_USERSETTINGS,                                   /* Use Case */
    /* Edge information */
    {
        {VPS_DC_BP0_INPUT_PATH, VPS_DC_VCOMP_MUX},
        {VPS_DC_VCOMP_MUX, VPS_DC_VCOMP},
        {VPS_DC_CIG_NON_CONSTRAINED_OUTPUT, VPS_DC_HDMI_BLEND},
        {VPS_DC_CIG_NON_CONSTRAINED_OUTPUT, VPS_DC_HDCOMP_BLEND},
        {VPS_DC_SEC1_INPUT_PATH, VPS_DC_SDVENC_MUX},
        {VPS_DC_SDVENC_MUX, VPS_DC_SDVENC_BLEND},
        {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDMI_BLEND},
        {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDCOMP_BLEND},
        {VPS_DC_GRPX2_INPUT_PATH, VPS_DC_SDVENC_BLEND},

    },

    9,
    /* VENC information */
    {
     /* Mode information */
     {
      {VPS_DC_VENC_HDMI, {FVID2_STD_1080P_60}
       }
      ,                                                    /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */
      {VPS_DC_VENC_HDCOMP, {FVID2_STD_1080P_60}
      },                                                   /* 1080p30 is mode
                                                            * is overwritten
                                                            * later inside
                                                            * System_displayCtrlInit
                                                            */

      {VPS_DC_VENC_SD, {FVID2_STD_NTSC}
       }
      }
     ,
     (VPS_DC_VENC_HDMI | VPS_DC_VENC_HDCOMP),                /* Tied VENC bit
                                                            * mask */
     3u                                                    /* Number of VENCs
                                                            */
     }
};
#endif

Int32 System_getClk(UInt32 displayRes)
{
    Int32 clkValue = VSYS_STD_MAX;
    switch(displayRes) {
        case VSYS_STD_1080P_60:
        case VSYS_STD_1080P_50:
            clkValue = 148500u;
            break;
        case VSYS_STD_1080P_30:
        case VSYS_STD_1080P_25:
        case VSYS_STD_1080P_24:
        case VSYS_STD_1080I_60:
        case VSYS_STD_1080I_50:
            clkValue = 74250u;
            break;
        case VSYS_STD_720P_60:
        case VSYS_STD_720P_50:
        case VSYS_STD_720P_30:
        case VSYS_STD_720P_25:
        case VSYS_STD_720P_24:
            clkValue = 74250u;
            break;
        case VSYS_STD_576P:
        case VSYS_STD_576I:
        case VSYS_STD_PAL:
        case VSYS_STD_480P:
        case VSYS_STD_480I:
        case VSYS_STD_NTSC:
            clkValue = 27000u;
            break;

        case VSYS_STD_WUXGA_60:
            clkValue = 193250u;
            break;
        case VSYS_STD_UXGA_60:
            clkValue = 162000u;
            break;
        case VSYS_STD_SXGAP_60:
            clkValue = 121750u;
            break;
        case VSYS_STD_1360_768_60:
            clkValue = 85500u;
            break;
        case VSYS_STD_SXGA_60:
            clkValue = 108000u;
            break;
        case VSYS_STD_WXGA_60:
            clkValue = 79500u;
            break;
        case VSYS_STD_XGA_60:
            clkValue = 65000u;
            break;
        case VSYS_STD_SVGA_60:
            clkValue = 40000u;
            break;
        case VSYS_STD_VGA_60:
            clkValue = 25175u;
            break;
        default:
            UTILS_assert(0);
            break;
    }
    return(clkValue);
}

Int32 System_displayCtrlInit(VDIS_PARAMS_S * pPrm)
{

    Int32                   driverRetVal, retVal;
    Vps_DcCreateConfig      dcCreateCfg;
    Vps_CscConfig           dcVcompCscConfig;
    Vps_CscConfig           dcHdcompCscConfig;
    Vps_CscConfig           dcSdCscConfig;
    Vps_DcEdeConfig         dcEdeCfg;
    Vps_DcVencClkSrc        clkSrc;
    Vps_DcConfig            *dctrlTriDisplayConfig = NULL;

#if defined(TI_814X_BUILD)
    /* Need to set this bit only for ti814x to support tied vencs, pin mux settings */
    (* (UInt32 *)0x481C52C8) = 0x01000000;
#endif

    System_displayUnderflowCheck(TRUE);
#if defined(TI_814X_BUILD) || defined (TI_8107_BUILD)
    dctrlTriDisplayConfig = &(gSystem_dctrlTriDisplayConfig);
    dctrlTriDisplayConfig->vencInfo.tiedVencs = pPrm->tiedDevicesMask;
#endif

#ifdef TI_816X_BUILD
    if(pPrm->tiedDevicesMask == (VPS_DC_VENC_HDMI | VPS_DC_VENC_HDCOMP))
    {
        dctrlTriDisplayConfig = &(gSystem_dctrlTriDisplayConfigHdmi);
        dctrlTriDisplayConfig->vencInfo.tiedVencs = pPrm->tiedDevicesMask;
    }
    if(pPrm->tiedDevicesMask == (VPS_DC_VENC_DVO2 | VPS_DC_VENC_HDCOMP))
    {
        dctrlTriDisplayConfig = &(gSystem_dctrlTriDisplayConfigDvo2);
        dctrlTriDisplayConfig->vencInfo.tiedVencs = pPrm->tiedDevicesMask;
    }
#endif
    memcpy(&gSystem_objVpss.displayCtrlCfg, dctrlTriDisplayConfig, sizeof(Vps_DcConfig));

    gSystem_objVpss.enableConfigExtVideoEncoder = pPrm->enableConfigExtVideoEncoder;


    retVal = System_configVencInfo(pPrm);
    UTILS_assert(retVal == 0);

//     /* Clock VENC_D is always tied to HDMI (DVO1)*/
    gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_D].outputClk =
                                System_getClk(pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution);

#if defined (TI_816X_BUILD) || defined(TI_8107_BUILD)
    if(dctrlTriDisplayConfig->vencInfo.tiedVencs ==
                                      (VPS_DC_VENC_DVO2 | VPS_DC_VENC_HDCOMP)) {
        UTILS_assert (pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution == pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution);
            gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_A].outputClk =
                                     System_getClk(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
     }
    if(dctrlTriDisplayConfig->vencInfo.tiedVencs ==
                                      (VPS_DC_VENC_HDMI | VPS_DC_VENC_HDCOMP)) {
        UTILS_assert (pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution == pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution);
        gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_A].outputClk =
                                 System_getClk(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
    }
    if(dctrlTriDisplayConfig->vencInfo.tiedVencs ==
                                      (VPS_DC_VENC_HDMI | VPS_DC_VENC_DVO2)) {
        UTILS_assert (pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution == pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
        gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_A].outputClk =
                                 System_getClk(pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution);
    }
    if(!(dctrlTriDisplayConfig->vencInfo.tiedVencs)) {
            gSystem_objVpss.vpllCfg[SYSTEM_VPLL_OUTPUT_VENC_A].outputClk =
                                     System_getClk(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
     }
#endif

    /* Configure pixel clock */
    retVal = System_dispSetPixClk();
    UTILS_assert(FVID2_SOK == retVal);

    dcVcompCscConfig.bypass  =
    dcHdcompCscConfig.bypass =
    dcSdCscConfig.bypass     = FALSE;
    dcVcompCscConfig.coeff   =
    dcHdcompCscConfig.coeff  =
    dcSdCscConfig.coeff      = NULL;

    dcVcompCscConfig.mode  =
    dcHdcompCscConfig.mode =
    dcSdCscConfig.mode     = pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].colorSpaceMode;

    memset(&dcCreateCfg, 0, sizeof(dcCreateCfg));

    if (pPrm->enableEdgeEnhancement) {
        dcEdeCfg.ltiEnable = TRUE;
        dcEdeCfg.horzPeaking = TRUE;
        dcEdeCfg.ctiEnable = TRUE;
        dcEdeCfg.transAdjustEnable = TRUE;
        dcEdeCfg.lumaPeaking = TRUE;
        dcEdeCfg.chromaPeaking = TRUE;
        dcEdeCfg.minClipLuma = 0;
        dcEdeCfg.maxClipLuma = 1023;
        dcEdeCfg.minClipChroma = 0;
        dcEdeCfg.maxClipChroma = 1023;
        dcEdeCfg.bypass = FALSE;

        dcCreateCfg.edeConfig       = &dcEdeCfg;
    }

    dcCreateCfg.vcompCscConfig  = &dcVcompCscConfig;
    dcCreateCfg.hdcompCscConfig = &dcHdcompCscConfig;
    dcCreateCfg.sdCscConfig     = &dcSdCscConfig;

    /* Open and configure display controller */
    gSystem_objVpss.fvidDisplayCtrl = FVID2_create(
                      FVID2_VPS_DCTRL_DRV,
                      VPS_DCTRL_INST_0,
                      &dcCreateCfg,
                      &driverRetVal,
                      NULL);
    //GT_assert( GT_DEFAULT_MASK, NULL != gSystem_objVpss.fvidDisplayCtrl );
    UTILS_assert(NULL != gSystem_objVpss.fvidDisplayCtrl);

    /* Set output in display controller */
    if (pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].enable) {
        retVal = FVID2_control(
                gSystem_objVpss.fvidDisplayCtrl,
                IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                &pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].outputInfo,
                NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);
    }

    if (pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].enable) {
        retVal = FVID2_control(
                    gSystem_objVpss.fvidDisplayCtrl,
                    IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                    &pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].outputInfo,
                    NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);
    }

    if (pPrm->deviceParams[SYSTEM_DC_VENC_SD].enable) {
        retVal = FVID2_control(
                    gSystem_objVpss.fvidDisplayCtrl,
                    IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                    &pPrm->deviceParams[SYSTEM_DC_VENC_SD].outputInfo,
                    NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);
    }

#if defined (TI_816X_BUILD) || defined(TI_8107_BUILD)
    if (pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].enable) {

#ifdef TI816X_DVR
    /*set the HDCOMP to VGA output if it is DVR and PG2.0*/
       if (Vps_platformGetCpuRev() >= SYSTEM_PLATFORM_CPU_REV_2_0)
            pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].outputInfo.dvoFmt =
                   VPS_DC_DVOFMT_TRIPLECHAN_DISCSYNC;
#endif
        retVal = FVID2_control(
                    gSystem_objVpss.fvidDisplayCtrl,
                    IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                    &pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].outputInfo,
                    NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);
    }
#endif

    /* Set the Clock source for VENC_DVO2 */
    if(dctrlTriDisplayConfig->vencInfo.tiedVencs ==
                                      (VPS_DC_VENC_HDMI | VPS_DC_VENC_DVO2)) {
        /* Set the Clock source for DVO2 */

        clkSrc.venc = VPS_DC_VENC_DVO2;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD;
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

        /* Set the Clock source for on-chip HDMI */
        clkSrc.venc = VPS_DC_VENC_HDMI;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD;
        // clkSrc is the same as DVO2 for this App
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);
#if defined (TI_816X_BUILD) || defined(TI_8107_BUILD)
        /* Set the Clock source for HDCOMP */
        clkSrc.venc = VPS_DC_VENC_HDCOMP;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCA;
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

#endif
    }
#if defined (TI_816X_BUILD) || defined(TI_8107_BUILD)

    if(dctrlTriDisplayConfig->vencInfo.tiedVencs ==
                                      (VPS_DC_VENC_DVO2 | VPS_DC_VENC_HDCOMP)) {
        /* Set the Clock source for off-chip HDMI */
        clkSrc.venc = VPS_DC_VENC_DVO2;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCA;
        // clkSrc is the same as DVO2 for this App
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

        /* Set the Clock source for VGA */
        clkSrc.venc = VPS_DC_VENC_HDCOMP;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCA;
        // clkSrc is the same as DVO2 for this App
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

        /* Set the Clock source for on-chip HDMI */
        clkSrc.venc = VPS_DC_VENC_HDMI;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD;
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

    }

    if(dctrlTriDisplayConfig->vencInfo.tiedVencs ==
                                      (VPS_DC_VENC_HDMI | VPS_DC_VENC_HDCOMP)) {
        /* Set the Clock source for on-chip HDMI */
        clkSrc.venc = VPS_DC_VENC_HDMI;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD;
        // clkSrc is the same as DVO2 for this App
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

        /* Set the Clock source for VGA */
        clkSrc.venc = VPS_DC_VENC_HDCOMP;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD;
        // clkSrc is the same as HDMI for this App
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

        /* Set the Clock source for off-chip HDMI */
        clkSrc.venc = VPS_DC_VENC_DVO2;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCA;
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

    }

#endif

    if(!(dctrlTriDisplayConfig->vencInfo.tiedVencs)) {
        /* Set the Clock source for off-chip HDMI */
        clkSrc.venc = VPS_DC_VENC_DVO2;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCA;
        // clkSrc is the same as DVO2 for this App
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

        /* Set the Clock source for on-chip HDMI */
        clkSrc.venc = VPS_DC_VENC_HDMI;
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD;
        retVal = FVID2_control(
                     gSystem_objVpss.fvidDisplayCtrl,
                     IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                     &clkSrc,
                     NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);

    }

    if(pPrm->enableConfigExtThsFilter == TRUE) {
#ifdef SYSTEM_USE_VIDEO_DECODER
        System_Ths7360SfCtrl    thsCtrl;

        /* THS is tied to HDCOMP/HDDAC only for EVM */
        switch (gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[1].mInfo.standard)
        {
            case FVID2_STD_720P_60:
            case FVID2_STD_720P_50:
            case FVID2_STD_1080I_60:
            case FVID2_STD_1080I_50:
            case FVID2_STD_1080P_30:
                thsCtrl = SYSTEM_THS7360_SF_HD_MODE;
                break;

            default:
            case FVID2_STD_1080P_60:
            case FVID2_STD_1080P_50:
                thsCtrl = SYSTEM_THS7360_SF_TRUE_HD_MODE;
                break;
        }
        System_ths7360SetSfParams(thsCtrl);
        System_ths7360SetSdParams(SYSTEM_THSFILTER_ENABLE_MODULE);
#endif
    }

#if defined(TI_8107_BUILD)
    retVal = System_platformSelectHdCompClkSrc(SYSTEM_VPLL_OUTPUT_VENC_A);
    UTILS_assert(retVal == FVID2_SOK);
#endif

#ifdef TI816X_DVR
    retVal = System_platformSelectHdCompSyncSrc(SYSTEM_HDCOMP_SYNC_SRC_DVO1, 1);
    UTILS_assert(retVal == FVID2_SOK);
#endif
    retVal = FVID2_control(
                 gSystem_objVpss.fvidDisplayCtrl,
                 IOCTL_VPS_DCTRL_SET_CONFIG,
                 &gSystem_objVpss.displayCtrlCfg,
                 NULL);
    //GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);
    UTILS_assert(retVal == FVID2_SOK);

    if (gSystem_objVpss.enableConfigExtVideoEncoder)
    {
#ifdef SYSTEM_USE_VIDEO_DECODER
#ifndef TI8107_DVR
        System_hdmiStart(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution, System_getBoardId());
#endif
#endif
    }


    return retVal;
}

Int32 System_displayCtrlSetVencOutput(VDIS_DEV_PARAM_S * pPrm)
{
    Int32 retVal;

    /* Set output in display controller */
    if (pPrm->enable) {
        retVal = FVID2_control(
                gSystem_objVpss.fvidDisplayCtrl,
                IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                &pPrm->outputInfo,
                NULL);
        //GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
        UTILS_assert(retVal == FVID2_SOK);
    }

    return retVal;
}

Int32 System_displayCtrlDeInit(VDIS_PARAMS_S * pPrm)
{
    Int32 retVal;

#ifdef SYSTEM_USE_VIDEO_DECODER
#ifndef TI8107_DVR
    if (gSystem_objVpss.enableConfigExtVideoEncoder)
    {
        System_hdmiStop();
    }
#endif
#endif


    /* Remove and close display controller configuration */
    retVal = FVID2_control(gSystem_objVpss.fvidDisplayCtrl,
                           IOCTL_VPS_DCTRL_CLEAR_CONFIG,
                           &gSystem_objVpss.displayCtrlCfg, NULL);
    UTILS_assert(retVal == FVID2_SOK);

    retVal = FVID2_delete(gSystem_objVpss.fvidDisplayCtrl, NULL);
    UTILS_assert(retVal == FVID2_SOK);

    System_displayUnderflowPrint(FALSE, TRUE);

    return retVal;
}



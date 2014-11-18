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


typedef struct
{
    FVID2_ModeInfo mInfo;
}System_CustomModeInfo;


System_CustomModeInfo systemCustomModeInfo[] = {
    {VSYS_STD_SVGA_60, 800, 600, FVID2_SF_PROGRESSIVE,
        40000, 60, 40, 88, 128, 1, 23, 4}
};


Int32 System_getVencMode(Int32 resolution)
{
    return(resolution);
}

Int32 System_setVencModeInfo(UInt32 displayIndx, UInt32 standard)
{
    Int32 i;
    Int32 retVal = 0;
    Int32 numCustomResolutions = sizeof(systemCustomModeInfo)/sizeof(System_CustomModeInfo);

    for(i = 0; i < numCustomResolutions; i++)
    {
        if(systemCustomModeInfo[i].mInfo.standard == standard)
        {
            memcpy(&gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[displayIndx].mInfo,
                   &systemCustomModeInfo[i],
                   sizeof(systemCustomModeInfo));
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[displayIndx].mInfo.standard = FVID2_STD_CUSTOM;
            break;
        }
    }

    if(i == numCustomResolutions)
    {
        retVal = -1;
    }

    return retVal;
}



Int32 System_configVencInfo(VDIS_PARAMS_S * pPrm)
{
    Int32 retVal = 0;

    if(pPrm != NULL)
    {
#if defined (TI_816X_BUILD)
        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[0].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(0, pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution);
           UTILS_assert(0 == retVal);
        }

        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[1].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(1, pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution);
           UTILS_assert(0 == retVal);
        }

        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[2].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(2, pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
           UTILS_assert(0 == retVal);
        }

        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[3].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(3, pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution);
           UTILS_assert(0 == retVal);
        }
#endif

#if defined(TI_814X_BUILD)
        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[0].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(0, pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution);
           UTILS_assert(0 == retVal);
        }

        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[1].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(1, pPrm->deviceParams[SYSTEM_DC_VENC_DVO2].resolution);
           UTILS_assert(0 == retVal);
        }

        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[2].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(2, pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution);
           UTILS_assert(0 == retVal);
        }
#endif

#if defined(TI_8107_BUILD)
        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[0].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(0, pPrm->deviceParams[SYSTEM_DC_VENC_HDMI].resolution);
           UTILS_assert(0 == retVal);
        }

        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[1].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(1, pPrm->deviceParams[SYSTEM_DC_VENC_HDCOMP].resolution);
           UTILS_assert(0 == retVal);
        }

        if(System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution) != FVID2_STD_CUSTOM)
        {
            gSystem_objVpss.displayCtrlCfg.vencInfo.modeInfo[2].mInfo.standard =
                      System_getVencMode(pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution);
        }
        else
        {
           retVal = System_setVencModeInfo(2, pPrm->deviceParams[SYSTEM_DC_VENC_SD].resolution);
           UTILS_assert(0 == retVal);
        }
#endif
    }
    else {
        retVal = -1;
    }

    return retVal;

}



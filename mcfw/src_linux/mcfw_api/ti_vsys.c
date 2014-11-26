/*==============================================================================
 * @file:       ti_vsys.c
 *
 * @brief:      System mcfw function definition.
 *
 * @vers:       0.5.0.0 2011-06
 *
 *==============================================================================
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

/* =============================================================================
 * Globals
 * =============================================================================
 */

VSYS_MODULE_CONTEXT_S gVsysModuleContext = {

	.enableFastUsecaseSwitch = FALSE,
    .initDone   = FALSE
};

/* =============================================================================
 * Vsys module APIs
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
Int32 Vsys_params_init(VSYS_PARAMS_S * pContext)
{
    memset(pContext, 0, sizeof(VSYS_PARAMS_S));
    pContext->enableCapture = FALSE;
    pContext->enableNsf     = FALSE;
    pContext->enableOsd     = FALSE;
    pContext->osdFormat     = FALSE;
    pContext->enableScd     = FALSE;
    pContext->enableEncode  = FALSE;
    pContext->enableDecode  = FALSE;
    pContext->numDeis       = 0;
    pContext->numSwMs       = 0;
    pContext->numDisplays   = 0;
    pContext->systemUseCase = VSYS_USECASE_MAX;

    return 0;
}
/**
 * \brief:
 *      Initialize Vsys instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_init(VSYS_PARAMS_S * pContext)
{

    if(pContext==NULL)
    {
        Vsys_params_init(&gVsysModuleContext.vsysConfig);
    }
    else
    {
        memcpy(&gVsysModuleContext.vsysConfig, pContext, sizeof(VSYS_PARAMS_S));
    }
    Vcap_init(NULL);
    Vdis_init(NULL);

    Vdec_init(NULL);
    Venc_init(NULL);

    if(gVsysModuleContext.enableFastUsecaseSwitch==FALSE)
    {
        if(gVsysModuleContext.initDone == FALSE)
        {
            gVsysModuleContext.initDone = TRUE;
	    	System_init();
    	}
	}

    return 0;
}

/**
 * \brief:
 *      Finalize Vsys instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_exit()
{

    if(gVsysModuleContext.enableFastUsecaseSwitch==FALSE)
	{
        if(gVsysModuleContext.initDone == TRUE)
        {
            gVsysModuleContext.initDone = FALSE;
	        System_deInit();
    	}
	}

    return 0;
}

/**
 * \brief:
 *      This function configures display controller
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_configureDisplay()
{
    Int32 status = 0;
	VDIS_PARAMS_S * prm = &gVdisModuleContext.vdisConfig;

	if (gVsysModuleContext.enableFastUsecaseSwitch == FALSE)
	{
		if(gVdisModuleContext.displayConfigInitDone == FALSE)
		{
		    status = System_linkControl(
		        SYSTEM_LINK_ID_M3VPSS,
		        SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_INIT,
		        &(*prm),
		        sizeof(*prm),
		        TRUE
		        );
		    UTILS_assert(status==OSA_SOK);
	
            gVdisModuleContext.displayConfigInitDone = TRUE;
		}
	}

    return status;
}

/**
 * \brief:
 *      This function de-configures display controller
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_deConfigureDisplay()
{
    Int32 status = 0;
	VDIS_PARAMS_S * prm = &gVdisModuleContext.vdisConfig;

	if (gVsysModuleContext.enableFastUsecaseSwitch == FALSE)
	{
		if(gVdisModuleContext.displayConfigInitDone == TRUE)
		{
            gVdisModuleContext.displayConfigInitDone = FALSE;
	
		    status = System_linkControl(
		        SYSTEM_LINK_ID_M3VPSS,
		        SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_DEINIT,
		        &(*prm),
		        sizeof(*prm),
		        TRUE
		        );
		    UTILS_assert(status==OSA_SOK);
		}
	}

    return status;
}


/**
 * \brief:
 *      Creates instances of links and prepares chains
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_create()
{

	/* Configuring display if application has not done already */
	Vsys_configureDisplay();

    switch (gVsysModuleContext.vsysConfig.systemUseCase) {

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC:
            MultiCh_createProgressive4D1VcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF:
            MultiCh_createProgressive16ChVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_VDEC_VDIS:
            MultiCh_createVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH:
            MultiCh_createProgressive8D1VcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT:
            MultiCh_createProgressive16ChNrtVcapVencVdecVdis();
            break;
#endif
#if defined(TI_8107_BUILD)
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH:
            MultiCh_createProgressive8ChVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT:
            MultiCh_createProgressive8ChNrtVcapVencVdecVdis();
            break;
#endif
#if defined(TI_816X_BUILD)
        case VSYS_USECASE_MULTICHN_VCAP_VDIS:
            MultiCh_createVcapVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC:
            MultiCh_createProgressiveVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_INTERLACED_VCAP_VDIS_VENC_VDEC:
            MultiCh_createInterlacedVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_VDEC_VDIS:
            MultiCh_createVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_VCAP_VENC:
            MultiCh_createVcapVencVdis();
            break;
        case VSYS_USECASE_MULTICHN_HD_VCAP_VENC:// 3
            MultiChHd_createVcapVencVdis();
            break;
        case VSYS_USECASE_MULTICHN_HYBRID_DVR_16CH:
            MultiCh_createHybridDVR();
            break;
        case VSYS_USECASE_MULTICHN_HYBRID_ENC_36CH:
            MultiCh_createHybridEnc();
            break;
        case VSYS_USECASE_MAX:
#endif
        case VSYS_USECASE_MULTICHN_CAR_DVR_4CH:
            MultiCh_createCarDVR();
            break;
        case VSYS_USECASE_MULTICHN_CUSTOM:
            MultiCh_createCustomVcapVencVdecVdis();
            break;
        default:
            break;
    }
    return 0;
}

/**
 * \brief:
 *      Delete instances of links
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_delete()
{

    switch (gVsysModuleContext.vsysConfig.systemUseCase) {
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC:
            MultiCh_deleteProgressive4D1VcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF:
            MultiCh_deleteProgressive16ChVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_VDEC_VDIS:
            MultiCh_deleteVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH:
            MultiCh_deleteProgressive8D1VcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT:
            MultiCh_deleteProgressive16ChNrtVcapVencVdecVdis();
            break;
#endif
#if defined(TI_8107_BUILD)
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH:
            MultiCh_deleteProgressive8ChVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT:
            MultiCh_deleteProgressive8ChNrtVcapVencVdecVdis();
            break;
#endif
#if defined(TI_816X_BUILD)
        case VSYS_USECASE_MULTICHN_VCAP_VDIS:
            MultiCh_deleteVcapVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC:
            MultiCh_deleteProgressiveVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_INTERLACED_VCAP_VDIS_VENC_VDEC:
            MultiCh_deleteInterlacedVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_VDEC_VDIS:
            MultiCh_deleteVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_VCAP_VENC:
            MultiCh_deleteVcapVencVdis();
            break;
        case VSYS_USECASE_MULTICHN_HD_VCAP_VENC:
            MultiChHd_deleteVcapVencVdis();
            break;
#endif
        case VSYS_USECASE_MULTICHN_CUSTOM:
            MultiCh_deleteCustomVcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH:
            MultiCh_deleteProgressive4D1VcapVencVdecVdis();
            break;
        case VSYS_USECASE_MULTICHN_HYBRID_DVR_16CH:
            MultiCh_deleteHybridDVR();
            break;
        case VSYS_USECASE_MULTICHN_CAR_DVR_4CH:
            MultiCh_deleteCarDVR();
            break;
        case VSYS_USECASE_MULTICHN_HYBRID_ENC_36CH:
            MultiCh_deleteHybridEnc();
            break;
        case VSYS_USECASE_MAX:
        default:
            break;
    }

	Vsys_deConfigureDisplay();

    return 0;
}

Int32 Vsys_allocBuf(UInt32 srRegId, UInt32 bufSize, UInt32 bufAlign, Vsys_AllocBufInfo *bufInfo)
{
    IHeap_Handle heapHndl;

    heapHndl = SharedRegion_getHeap(srRegId);
    OSA_assert(heapHndl != NULL);

    bufInfo->virtAddr = NULL;
    bufInfo->physAddr = NULL;
    bufInfo->srPtr    = 0;

    bufInfo->virtAddr = Memory_alloc(heapHndl, bufSize, bufAlign, NULL);

    if(bufInfo->virtAddr==NULL)
        return -1;

    bufInfo->physAddr = Memory_translate (bufInfo->virtAddr, Memory_XltFlags_Virt2Phys);

    if(bufInfo->physAddr==NULL)
        return -1;

    bufInfo->srPtr = SharedRegion_getSRPtr(bufInfo->virtAddr,srRegId);

    return 0;
}

Int32 Vsys_freeBuf(UInt32 srRegId, UInt8 *virtAddr, UInt32 bufSize)
{
    IHeap_Handle heapHndl;

    heapHndl = SharedRegion_getHeap(srRegId);
    OSA_assert(heapHndl != NULL);

    OSA_assert(virtAddr != NULL);

    Memory_free(heapHndl, virtAddr, bufSize);

    return 0;
}


Int32 Vsys_printDetailedStatistics()
{
    UInt32 devId;

    if(gVcapModuleContext.captureId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVcapModuleContext.captureId,
                            CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS,
                            NULL, 0, TRUE);
    }

    for(devId=0; devId<MAX_SCLR_LINK; devId++)
    {
        if(gVcapModuleContext.sclrId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
             System_linkControl(gVcapModuleContext.sclrId[devId],
                                SCLR_LINK_CMD_PRINT_STATISTICS,
                                NULL, 0, TRUE);
        }
    }

   for(devId=0; devId<MAX_NSF_LINK; devId++)
    {
        if(gVcapModuleContext.nsfId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
             System_linkControl(gVcapModuleContext.nsfId[devId],
                                NSF_LINK_CMD_PRINT_STATISTICS,
                                NULL, 0, TRUE);
        }
    }

    OSA_waitMsecs(2000); // allow for print to complete

    for(devId=0; devId<MAX_DEI_LINK; devId++)
    {
        if(gVcapModuleContext.deiId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVcapModuleContext.deiId[devId],
                            DEI_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    if (gVcapModuleContext.capSwMsId != SYSTEM_LINK_ID_INVALID)
        System_linkControl(gVcapModuleContext.capSwMsId,
                        SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS,
                        NULL, 0, TRUE);
        
    for(devId=0; devId<VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.swMsId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVdisModuleContext.swMsId[devId],
                            SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    for(devId=0; devId<VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.displayId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVdisModuleContext.displayId[devId],
                            DISPLAY_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }
    for(devId=0; devId<MAX_IPC_FRAMES_LINK; devId++)
    {
        if(gVcapModuleContext.ipcFramesOutVpssId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVcapModuleContext.ipcFramesOutVpssId[devId],
                            IPCFRAMESOUTRTOS_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }


    OSA_waitMsecs(2000); // allow for print to complete

    for(devId=0; devId<MAX_ALG_LINK; devId++)
    {
        if(gVcapModuleContext.dspAlgId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVcapModuleContext.dspAlgId[devId],
                            ALG_LINK_SCD_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    OSA_waitMsecs(2000); // allow for print to complete

    if(gVencModuleContext.encId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVencModuleContext.encId,
                            ENC_LINK_CMD_PRINT_IVAHD_STATISTICS,
                            NULL, 0, TRUE);
    }
    else if (gVdecModuleContext.decId !=SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVdecModuleContext.decId,
                            DEC_LINK_CMD_PRINT_IVAHD_STATISTICS,
                            NULL, 0, TRUE);
     }

    if(gVencModuleContext.encId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVencModuleContext.encId,
                            ENC_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
    }
    if(gVdecModuleContext.decId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVdecModuleContext.decId,
                            DEC_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
    }

    OSA_waitMsecs(2000); // allow for print to complete

#ifdef TI_816X_BUILD
    if(gVdisModuleContext.mpSclrId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(SYSTEM_LINK_ID_MP_SCLR_INST_0,
                                MP_SCLR_LINK_CMD_PRINT_STATISTICS,
                                NULL, 0, TRUE);
    }
#endif /* TI_816X_BUILD */

    MultiCh_prfLoadPrint(TRUE,TRUE);
    return ERROR_NONE;
}

Int32 Vsys_printBufferStatistics()
{
    UInt32 devId;

    if (gVdecModuleContext.ipcBitsOutHLOSId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVdecModuleContext.ipcBitsOutHLOSId,
                            IPCBITSOUT_LINK_CMD_PRINT_BUFFER_STATISTICS,
                            NULL, 0, TRUE);
    }

    if(gVdecModuleContext.decId != SYSTEM_LINK_ID_INVALID)
    {
        int i;
        VDEC_BUFFER_STATS_S bufStats;

        bufStats.numCh = gVdecModuleContext.vdecConfig.numChn;
        for (i = 0; i < gVdecModuleContext.vdecConfig.numChn;i++)
        {
            bufStats.chId[i] = i;
        }
        Vdec_getBufferStatistics(&bufStats);
        printf("\r\n VDEC:Buffer Statistics");
        printf("\r\n ChId | InBufCnt | OutBufCnt");
        for (i = 0; i < gVdecModuleContext.vdecConfig.numChn;i++)
        {
            printf("\r\n %5d|%10d|%10d",
                   bufStats.chId[i],
                   bufStats.stats[i].numInBufQueCount,
                   bufStats.stats[i].numOutBufQueCount);
        }
        printf("\n");
    }

    if(gVcapModuleContext.captureId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVcapModuleContext.captureId,
                            CAPTURE_LINK_CMD_PRINT_BUFFER_STATISTICS,
                            NULL, 0, TRUE);
    }

   for(devId=0; devId<MAX_NSF_LINK; devId++)
    {
        if(gVcapModuleContext.nsfId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
             System_linkControl(gVcapModuleContext.nsfId[devId],
                                NSF_LINK_CMD_PRINT_BUFFER_STATISTICS,
                                NULL, 0, TRUE);
        }
    }

    for(devId=0; devId<MAX_DEI_LINK; devId++)
    {
        if(gVcapModuleContext.deiId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVcapModuleContext.deiId[devId],
                            DEI_LINK_CMD_PRINT_BUFFER_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    for(devId=0; devId<VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.swMsId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVdisModuleContext.swMsId[devId],
                            SYSTEM_SW_MS_LINK_CMD_PRINT_BUFFER_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    for(devId=0; devId<VDIS_DEV_MAX; devId++)
    {
        if(gVdisModuleContext.displayId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gVdisModuleContext.displayId[devId],
                            DISPLAY_LINK_CMD_PRINT_BUFFER_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    OSA_waitMsecs(500);

    if(gVencModuleContext.encId !=SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gVencModuleContext.encId,
                        ENC_LINK_CMD_PRINT_BUFFER_STATISTICS,
                        NULL, 0, TRUE);
    }

    OSA_waitMsecs(500);
    return ERROR_NONE;
}



/**
 * \brief:
 *      This function gives CPU ID, board type and revision.
 * \input:
 *      VSYS_PLATFORM_INFO_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_getPlatformInfo(VSYS_PLATFORM_INFO_S *pInfo)
{
    Int32 status;

    SystemVpss_PlatformInfo  platformInfo;

    status = System_linkControl(
                SYSTEM_LINK_ID_M3VPSS,
                SYSTEM_M3VPSS_CMD_GET_PLATFORM_INFO,
                &platformInfo,
                sizeof(platformInfo),
                TRUE
                );

    UTILS_assert(status==OSA_SOK);

    /* Get CPU version */
    pInfo->cpuRev = platformInfo.cpuRev;

    if (pInfo->cpuRev >= SYSTEM_PLATFORM_CPU_REV_MAX)
    {
        pInfo->cpuRev = SYSTEM_PLATFORM_CPU_REV_UNKNOWN;
    }
    /* Detect board */
    pInfo->boardId = platformInfo.boardId;

    if (pInfo->boardId >= SYSTEM_PLATFORM_BOARD_MAX)
    {
        pInfo->boardId = SYSTEM_PLATFORM_BOARD_UNKNOWN;
    }

    /* Get base board revision */
    pInfo->boardRev = platformInfo.baseBoardRev;

    if (pInfo->boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
    {
        pInfo->boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
    }

    if (pInfo->boardId != SYSTEM_PLATFORM_BOARD_UNKNOWN)
    {
        /* Get daughter card revision */
        pInfo->boardRev = platformInfo.dcBoardRev;
        if (pInfo->boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
        {
            pInfo->boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
        }
    }

    return ERROR_NONE;
}

Void Vsys_getContext(VSYS_PARAMS_S * contextInfo)
{
    if (contextInfo != NULL)
    {
        memcpy(contextInfo,
               &gVsysModuleContext.vsysConfig,
               sizeof(VSYS_PARAMS_S));
    }

}

Int32 Vsys_enableFastUsecaseSwitch(Bool enable)
{
    gVsysModuleContext.enableFastUsecaseSwitch = enable;

    return ERROR_NONE;
}

Int32 Vsys_getCoreStatus(VSYS_CORE_STATUS_TBL_S *coreStatusTbl)
{
    System_linkGetCoreStatus(coreStatusTbl);
    return ERROR_NONE;
}


Int32 Vsys_setIVAMap(VSYS_IVA2CHMAP_TBL_S *ivaTbl)
{
    Int32 status;

    status =  System_linkControl(SYSTEM_LINK_ID_M3VIDEO,
                                 SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
                                 ivaTbl,
                                 sizeof(*ivaTbl),
                                 TRUE);

    return ERROR_NONE;
}

Int32 Vsys_getIVAMap(VSYS_IVA2CHMAP_TBL_S *ivaTbl)
{
    Int32 status;

    status =  System_linkControl(SYSTEM_LINK_ID_M3VIDEO,
                                 SYSTEM_COMMON_CMD_GET_CH2IVAHD_MAP_TBL,
                                 ivaTbl,
                                 sizeof(*ivaTbl),
                                 TRUE);

    return ERROR_NONE;
}

/*
 * sample_init.c
 *
 * Sample Initialization for the EDMA3 Driver for BIOS 6 based applications.
 * It should be MANDATORILY done once before EDMA3 usage.
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
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

#include "utils_dma_bios6.h"
#include <ti/sdo/fc/edma3/edma3_config.h>

EDMA3_RM_Handle gEDMA_Hndl = NULL;
EDMA3_OS_Sem_Handle gEDMA_SEM_Hndl = NULL;


/* This is a hack to open EDMA handle via LLD_RM. Need to fix it later.      */
/* Using Separte List of available channel for EDMA open via LLD_RM          */
/* This done to avoid allocation of same channels that were allocated via FC */
#define UTILS_C6XDSP_EDMACH_ALLOC_3     (0xC0000000)
#define UTILS_C6XDSP_EDMACH_ALLOC_4     (0x00000000)

/**
 * \brief   EDMA3 Initialization
 *
 * This function initializes the EDMA3 Driver.
 *
  * \return  EDMA3_RM_SOK if success, else error code
 */
EDMA3_RM_Handle Utils_edma3Open (unsigned int edma3Id, EDMA3_RM_Result *errorCode)
    {
    EDMA3_RM_Result edma3Result = EDMA3_RM_E_INVALID_PARAM;
    EDMA3_RM_Handle rmEdma = NULL;

	if ((edma3Id > 0 ) || (errorCode == NULL))
		return rmEdma;

    Utils_dmaCheckStaticAllocationConlficts();

    /* Make the semaphore handle as NULL. */
    gUtils_dmaObj.semHandle = NULL;

    gUtils_dmaObj.region_id = Utils_getRegionId();

    /* get region ID for this processor */
    {
        Semaphore_Params semParams;
        EDMA3_RM_Param initCfg;
        EDMA3_RM_MiscParam miscParam;

        /* Configure it as master, if required */
        miscParam.isSlave = Utils_isGblConfigRequired(gUtils_dmaObj.region_id);

        Semaphore_Params_init(&semParams);
        initCfg.rmSemHandle = NULL;
        initCfg.rmSemHandle = EDMA3_semCreate(1, 1);//&semParams, &initCfg.rmSemHandle);
        /* Save the semaphore handle for future use */
        gUtils_dmaObj.semHandle = initCfg.rmSemHandle;

        initCfg.isMaster = FALSE;//TRUE;
        /* Choose shadow region according to the DSP# */
        initCfg.regionId = (EDMA3_RM_RegionId)gUtils_dmaObj.region_id;

        /* Driver instance specific config NULL */
//        initCfg.rmInstInitConfig = &gUtils_dmaRMInstInitConfig;
        initCfg.rmInstInitConfig = (EDMA3_RM_InstanceInitConfig  *)&gUtils_dmaInstInitConfig;

         /* This is a hack to open EDMA handle via LLD_RM. Need to fix it later.      */
         /* Using Separte List of available channel for EDMA open via LLD_RM          */
         /* This done to avoid allocation of same channels that were allocated via FC */

        initCfg.rmInstInitConfig->ownDmaChannels[0] = UTILS_C6XDSP_EDMACH_ALLOC_3;
        initCfg.rmInstInitConfig->ownDmaChannels[1] = UTILS_C6XDSP_EDMACH_ALLOC_4;

        initCfg.gblerrCbParams.gblerrCb   = ( EDMA3_RM_GblErrCallback) NULL;

        initCfg.gblerrCbParams.gblerrData = (void *)NULL;

        /* Open the Driver Instance */
        rmEdma = EDMA3_RM_open (edma3Id, (void *) &initCfg, &edma3Result);

        if ((edma3Result != EDMA3_RM_SOK) || (!rmEdma))
        {
             Vps_printf(" %d: UTILS: EDMA RM is not Created\n",Utils_getCurTimeInMsec());
             Vps_printf(" %d: UTILS: Creating Now\n",Utils_getCurTimeInMsec());
             edma3Result = EDMA3_RM_create (edma3Id, (EDMA3_RM_GblConfigParams *)&gUtils_dmaGblCfgParams ,
                                        (void *)&miscParam);
             if (edma3Result == EDMA3_RM_SOK)
             {
                 Vps_printf(" %d: UTILS: EDMA RM Created\n",Utils_getCurTimeInMsec());
                 initCfg.regionInitEnable = TRUE;
                 rmEdma = EDMA3_RM_open (edma3Id, (void *) &initCfg, &edma3Result);

                 if(rmEdma && (EDMA3_RM_SOK == edma3Result))
                     Vps_printf(" %d: UTILS: EDMA RM Opened\n",Utils_getCurTimeInMsec());
                 else
                     Vps_printf(" %d: UTILS: EDMA RM Open Failed: Return Code is %d\n",Utils_getCurTimeInMsec(),edma3Result);
             }
             else
                  Vps_printf(" %d: UTILS: EDMA RM Create Failed: Return Code is %d\n",Utils_getCurTimeInMsec(),edma3Result);

        }
     }

     if(rmEdma && (edma3Result == EDMA3_RM_SOK))
     {
        Vps_printf(" %d: UTILS: EDMA RM Opened\n",Utils_getCurTimeInMsec());
        gEDMA_Hndl = rmEdma;
        gEDMA_SEM_Hndl = gUtils_dmaObj.semHandle;

     }
     *errorCode = edma3Result;

     return rmEdma;
}


/**
 * \brief   EDMA3 De-initialization
 *
 * This function removes the EDMA3 Driver instance.
 *
  * \return  EDMA3_RM_SOK if success, else error code
 */
EDMA3_RM_Result Utils_edma3Close (unsigned int edma3Id, EDMA3_RM_Handle rmEdma)
    {
    EDMA3_RM_Result edma3Result = EDMA3_RM_SOK;
    EDMA3_RM_Instance * rmInst;
    EDMA3_RM_Obj      * resMgrObj;

    rmInst = (EDMA3_RM_Instance *) rmEdma;

    resMgrObj= rmInst->pResMgrObjHandle;
    
    /* Delete the semaphore */
    EDMA3_semDelete(gUtils_dmaObj.semHandle);

    /* Make the semaphore handle as NULL. */
    gUtils_dmaObj.semHandle = NULL;


    if(resMgrObj->numOpens == 1)
    {

        if((resMgrObj->state != EDMA3_RM_CLOSED) && (resMgrObj->state != EDMA3_RM_DELETED))
        {
            /* Now, close the EDMA3 Driver Instance */
            edma3Result = EDMA3_RM_close (rmEdma, NULL);
        }

        if (EDMA3_RM_SOK == edma3Result )
        {
           Vps_printf(" %d: UTILS: EDMA RM Closed\n",Utils_getCurTimeInMsec());

           edma3Result = EDMA3_RM_delete (edma3Id, NULL);
           if(EDMA3_RM_SOK == edma3Result)
               Vps_printf(" %d: UTILS: EDMA RM Deleted\n",Utils_getCurTimeInMsec());
           else
               Vps_printf(" %d: UTILS: EDMA RM Delete Fail: Return Code is %d\n",Utils_getCurTimeInMsec(),edma3Result);
        }
        else
        {
            Vps_printf(" %d: UTILS: EDMA RM Close Failed: Return Code is %d\n",Utils_getCurTimeInMsec(),edma3Result);
        }

    }
    else
    {
        /* Make the semaphore handle as NULL. */
        gUtils_dmaObj.semHandle = NULL;

        /* Now, close the EDMA3 Driver Instance */
        edma3Result = EDMA3_RM_close (rmEdma, NULL);
    }


    return edma3Result;
}

 

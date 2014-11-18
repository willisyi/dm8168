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


EDMA3_DRV_Handle gSYSTEM_EDMA_Hndl = NULL;
EDMA3_OS_Sem_Handle gSYSTEM_EDMA_SEM_Hndl = NULL;

/**
 * \brief   EDMA3 Initialization
 *
 * This function initializes the EDMA3 Driver and registers the
 * interrupt handlers.
 *
  * \return  EDMA3_DRV_SOK if success, else error code
 */
EDMA3_DRV_Handle Utils_edma3init (unsigned int edma3Id, EDMA3_DRV_Result *errorCode)
    {
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_E_INVALID_PARAM;
	EDMA3_DRV_Handle hEdma = NULL;

	if ((edma3Id > 0 ) || (errorCode == NULL))
		return hEdma;

    Utils_dmaCheckStaticAllocationConlficts();

    /* Make the semaphore handle as NULL. */
    gUtils_dmaObj.semHandle = NULL;

    gUtils_dmaObj.region_id = Utils_getRegionId();

    /* get region ID for this processor */
    {
        Semaphore_Params semParams;
	    EDMA3_DRV_InitConfig initCfg;
	    EDMA3_RM_MiscParam miscParam;

        /* Configure it as master, if required */
        miscParam.isSlave = Utils_isGblConfigRequired(gUtils_dmaObj.region_id);

        edma3Result = EDMA3_DRV_create (edma3Id, &gUtils_dmaGblCfgParams ,
                                        (void *)&miscParam);

        if (edma3Result == EDMA3_DRV_SOK)
        {
            /**
            * Driver Object created successfully.
            * Create a semaphore now for driver instance.
            */
            Semaphore_Params_init(&semParams);

            initCfg.drvSemHandle = NULL;
            edma3Result = Utils_edma3OsSemCreate(1, &semParams, &initCfg.drvSemHandle);
        }

        if (edma3Result == EDMA3_DRV_SOK)
        {
            /* Save the semaphore handle for future use */
            gUtils_dmaObj.semHandle = initCfg.drvSemHandle;

            initCfg.isMaster = TRUE;
            /* Choose shadow region according to the DSP# */
            initCfg.regionId = (EDMA3_RM_RegionId)gUtils_dmaObj.region_id;

            /* Driver instance specific config NULL */
            initCfg.drvInstInitConfig = &gUtils_dmaInstInitConfig;

            initCfg.gblerrCb = NULL;
            initCfg.gblerrData = NULL;

            /* Open the Driver Instance */
            hEdma = EDMA3_DRV_open (edma3Id, (void *) &initCfg, &edma3Result);
        }
    }

	if(hEdma && (edma3Result == EDMA3_DRV_SOK))
	{
        gSYSTEM_EDMA_Hndl = hEdma;
        gSYSTEM_EDMA_SEM_Hndl = gUtils_dmaObj.semHandle;

		/**
		* Register Interrupt Handlers for various interrupts
		* like transfer completion interrupt, CC error
		* interrupt, TC error interrupts etc, if required.
		*/
		Utils_registerEdma3Interrupts();
	}

	*errorCode = edma3Result;
	return hEdma;
}


/**
 * \brief   EDMA3 De-initialization
 *
 * This function removes the EDMA3 Driver instance and unregisters the
 * interrupt handlers.
 *
  * \return  EDMA3_DRV_SOK if success, else error code
 */
EDMA3_DRV_Result Utils_edma3deinit (unsigned int edma3Id, EDMA3_DRV_Handle hEdma)
    {
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_E_INVALID_PARAM;

    /* Unregister Interrupt Handlers first */
    Utils_unregisterEdma3Interrupts();

    /* Delete the semaphore */
    edma3Result = Utils_edma3OsSemDelete(gUtils_dmaObj.semHandle);

    if (EDMA3_DRV_SOK == edma3Result )
    {
        /* Make the semaphore handle as NULL. */
        gUtils_dmaObj.semHandle = NULL;

        /* Now, close the EDMA3 Driver Instance */
        edma3Result = EDMA3_DRV_close (hEdma, NULL);
    }

	if (EDMA3_DRV_SOK == edma3Result )
    {
        /* Now, delete the EDMA3 Driver Object */
        edma3Result = EDMA3_DRV_delete (edma3Id, NULL);
    }

    return edma3Result;
}


/**  To Register the ISRs with the underlying OS, if required. */
void Utils_registerEdma3Interrupts ()
{
    static UInt32 cookie = 0;
    //unsigned int numTc = 0;
    Hwi_Params hwiParams;
    Error_Block      eb;

    /* Initialize the Error Block                                             */
    Error_init(&eb);

    /* Disabling the global interrupts */
    cookie = Hwi_disable();

    /* Initialize the HWI parameters with user specified values */
    Hwi_Params_init(&hwiParams);

    /* argument for the ISR */
    hwiParams.arg = 0;

    Vps_printf(" %d: UTILS: DMA: HWI Create for INT%d !!!\n", Utils_getCurTimeInMsec(), gUtils_ccXferCompInt[gUtils_dmaObj.region_id]);

    gUtils_dmaObj.hwiCCXferCompInt = Hwi_create( gUtils_ccXferCompInt[gUtils_dmaObj.region_id],
                			(&lisrEdma3ComplHandler0),
                			(const Hwi_Params *) (&hwiParams),
                			&eb);
    if (TRUE == Error_check(&eb))
    {
        Vps_printf(" UTILS: DMA: HWI Create Failed !!!\n");
    }

    /* Restore interrupts */
    Hwi_restore(cookie);
}

/**  To Unregister the ISRs with the underlying OS, if previously registered. */
void Utils_unregisterEdma3Interrupts ()
    {
	static UInt32 cookie = 0;

    /* Disabling the global interrupts */
    cookie = Hwi_disable();

    Hwi_delete(&gUtils_dmaObj.hwiCCXferCompInt);

    /* Restore interrupts */
    Hwi_restore(cookie);
    }


/**
 * \brief   EDMA3 OS Protect Entry
 *
 *      This function saves the current state of protection in 'intState'
 *      variable passed by caller, if the protection level is
 *      EDMA3_OS_PROTECT_INTERRUPT. It then applies the requested level of
 *      protection.
 *      For EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION and
 *      EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR, variable 'intState' is ignored,
 *      and the requested interrupt is disabled.
 *      For EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR, '*intState' specifies the
 *      Transfer Controller number whose interrupt needs to be disabled.
 *
 * \param   level is numeric identifier of the desired degree of protection.
 * \param   intState is memory location where current state of protection is
 *      saved for future use while restoring it via edma3OsProtectExit() (Only
 *      for EDMA3_OS_PROTECT_INTERRUPT protection level).
 * \return  None
 */
void edma3OsProtectEntry (unsigned int edma3InstanceId,
							int level, unsigned int *intState)
    {
    if (((level == EDMA3_OS_PROTECT_INTERRUPT)
        || (level == EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR))
        && (intState == NULL))
        {
        return;
        }
    else
        {
        switch (level)
            {
            /* Disable all (global) interrupts */
            case EDMA3_OS_PROTECT_INTERRUPT :
                *intState = Hwi_disable();
                break;

            /* Disable scheduler */
            case EDMA3_OS_PROTECT_SCHEDULER :
				        Task_disable();
                break;

            /* Disable EDMA3 transfer completion interrupt only */
            case EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION :
                Hwi_disableInterrupt(gUtils_ccXferCompInt[gUtils_dmaObj.region_id]);
                break;

            /* Disable EDMA3 CC error interrupt only */
            case EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR :
                Hwi_disableInterrupt(gUtils_ccErrorInt);
                break;

            /* Disable EDMA3 TC error interrupt only */
            case EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR :
                switch (*intState)
                    {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        /* Fall through... */
                        /* Disable the corresponding interrupt */
                        Hwi_disableInterrupt(gUtils_tcErrorInt[*intState]);
                        break;

                     default:
                        break;
                    }

                break;

            default:
                break;
            }
        }
    }


/**
 * \brief   EDMA3 OS Protect Exit
 *
 *      This function undoes the protection enforced to original state
 *      as is specified by the variable 'intState' passed, if the protection
 *      level is EDMA3_OS_PROTECT_INTERRUPT.
 *      For EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION and
 *      EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR, variable 'intState' is ignored,
 *      and the requested interrupt is enabled.
 *      For EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR, 'intState' specifies the
 *      Transfer Controller number whose interrupt needs to be enabled.
 * \param   level is numeric identifier of the desired degree of protection.
 * \param   intState is original state of protection at time when the
 *      corresponding edma3OsProtectEntry() was called (Only
 *      for EDMA3_OS_PROTECT_INTERRUPT protection level).
 * \return  None
 */
void edma3OsProtectExit (unsigned int edma3InstanceId,
                        int level, unsigned int intState)
    {
    switch (level)
        {
        /* Enable all (global) interrupts */
        case EDMA3_OS_PROTECT_INTERRUPT :
            Hwi_restore(intState);
            break;

        /* Enable scheduler */
        case EDMA3_OS_PROTECT_SCHEDULER :
            Task_enable();
            break;

        /* Enable EDMA3 transfer completion interrupt only */
        case EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION :
            Hwi_enableInterrupt(gUtils_ccXferCompInt[gUtils_dmaObj.region_id]);
            break;

        /* Enable EDMA3 CC error interrupt only */
        case EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR :
            Hwi_enableInterrupt(gUtils_ccErrorInt);
            break;

        /* Enable EDMA3 TC error interrupt only */
        case EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR :
            switch (intState)
                {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                    /* Fall through... */
                    /* Enable the corresponding interrupt */
                    Hwi_enableInterrupt(gUtils_tcErrorInt[intState]);
                    break;

                 default:
                    break;
                }

            break;

        default:
            break;
        }
    }







/**
  * Counting Semaphore related functions (OS dependent) should be
  * called/implemented by the application. A handle to the semaphore
  * is required while opening the driver/resource manager instance.
  */

/**
 * \brief   EDMA3 OS Semaphore Create
 *
 *      This function creates a counting semaphore with specified
 *      attributes and initial value. It should be used to create a semaphore
 *      with initial value as '1'. The semaphore is then passed by the user
 *      to the EDMA3 driver/RM for proper sharing of resources.
 * \param   initVal [IN] is initial value for semaphore
 * \param   semParams [IN] is the semaphore attributes.
 * \param   hSem [OUT] is location to recieve the handle to just created
 *      semaphore
 * \return  EDMA3_DRV_SOK if succesful, else a suitable error code.
 */
EDMA3_DRV_Result Utils_edma3OsSemCreate(int initVal,
							const Semaphore_Params *semParams,
                           	EDMA3_OS_Sem_Handle *hSem)
    {
    EDMA3_DRV_Result semCreateResult = EDMA3_DRV_SOK;

    if(NULL == hSem)
        {
        semCreateResult = EDMA3_DRV_E_INVALID_PARAM;
        }
    else
        {
        *hSem = (EDMA3_OS_Sem_Handle)Semaphore_create(initVal, semParams, NULL);
        if ( (*hSem) == NULL )
            {
            semCreateResult = EDMA3_DRV_E_SEMAPHORE;
            }
        }

    return semCreateResult;
    }


/**
 * \brief   EDMA3 OS Semaphore Delete
 *
 *      This function deletes or removes the specified semaphore
 *      from the system. Associated dynamically allocated memory
 *      if any is also freed up.
 * \param   hSem [IN] handle to the semaphore to be deleted
 * \return  EDMA3_DRV_SOK if succesful else a suitable error code
 */
EDMA3_DRV_Result Utils_edma3OsSemDelete(EDMA3_OS_Sem_Handle hSem)
    {
    EDMA3_DRV_Result semDeleteResult = EDMA3_DRV_SOK;

    if(NULL == hSem)
        {
        semDeleteResult = EDMA3_DRV_E_INVALID_PARAM;
        }
    else
        {
		Semaphore_delete((Semaphore_Handle *)&hSem);
        }

    return semDeleteResult;
    }


/**
 * \brief   EDMA3 OS Semaphore Take
 *
 *      This function takes a semaphore token if available.
 *      If a semaphore is unavailable, it blocks currently
 *      running thread in wait (for specified duration) for
 *      a free semaphore.
 * \param   hSem [IN] is the handle of the specified semaphore
 * \param   mSecTimeout [IN] is wait time in milliseconds
 * \return  EDMA3_DRV_Result if successful else a suitable error code
 */
EDMA3_DRV_Result edma3OsSemTake(EDMA3_OS_Sem_Handle hSem, int mSecTimeout)
    {
    EDMA3_DRV_Result semTakeResult = EDMA3_DRV_SOK;
    unsigned short semPendResult;

    if(NULL == hSem)
        {
        semTakeResult = EDMA3_DRV_E_INVALID_PARAM;
        }
    else
        {
        semPendResult = Semaphore_pend(hSem, mSecTimeout);
        if (semPendResult == FALSE)
            {
            semTakeResult = EDMA3_DRV_E_SEMAPHORE;
            }
        }

    return semTakeResult;
    }


/**
 * \brief   EDMA3 OS Semaphore Give
 *
 *      This function gives or relinquishes an already
 *      acquired semaphore token
 * \param   hSem [IN] is the handle of the specified semaphore
 * \return  EDMA3_DRV_Result if successful else a suitable error code
 */
EDMA3_DRV_Result edma3OsSemGive(EDMA3_OS_Sem_Handle hSem)
    {
    EDMA3_DRV_Result semGiveResult = EDMA3_DRV_SOK;

    if(NULL == hSem)
        {
        semGiveResult = EDMA3_DRV_E_INVALID_PARAM;
        }
    else
        {
		Semaphore_post(hSem);
        }

    return semGiveResult;
    }




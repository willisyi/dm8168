/*
 * bios6_edma3_drv_sample.h
 *
 * Header file for the sample application for the EDMA3 Driver.
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

#ifndef _BIOS6_EDMA3_DRV_SAMPLE_H_
#define _BIOS6_EDMA3_DRV_SAMPLE_H_


#include <stdio.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/hal/Hwi.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <mcfw/interfaces/link_api/system.h>


#ifdef __TMS470__
#else
#include <ti/sysbios/family/c64p/EventCombiner.h>
#endif

/* Include EDMA3 Driver */
#include <ti/sdo/edma3/drv/edma3_drv.h>
#include <ti/sdo/edma3/rm/src/edma3_rl_cc.h>

#include <mcfw/src_bios6/utils/utils_dma.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UTILS_DMA_MIN_TX         (4)
#define UTILS_DMA_BYTES_TO_WRITE (sizeof(UInt32))

typedef struct {
    UInt32 edma3InstanceId;

    EDMA3_DRV_Handle hEdma;

    EDMA3_RM_Handle rmEdma;
    
    Hwi_Handle hwiCCXferCompInt;

    unsigned int region_id;

    /* Semaphore handles */
    EDMA3_OS_Sem_Handle semHandle;

} Utils_DmaObj;


extern Utils_DmaObj gUtils_dmaObj;

extern unsigned int gUtils_ccXferCompInt[EDMA3_MAX_REGIONS];
extern unsigned int gUtils_ccErrorInt;
extern unsigned int gUtils_tcErrorInt[EDMA3_MAX_TC];


/* External Global Configuration Structure */
extern EDMA3_DRV_GblConfigParams gUtils_dmaGblCfgParams;

/* External Instance Specific Configuration Structure */
extern EDMA3_DRV_InstanceInitConfig gUtils_dmaInstInitConfig;


Int32 Utils_dmaTrigger(Utils_DmaChObj *pObj);

/**  To Register the ISRs with the underlying OS, if required. */
void Utils_registerEdma3Interrupts ();
/**  To Unregister the ISRs with the underlying OS, if previously registered. */
void Utils_unregisterEdma3Interrupts ();

/* To find out the DSP# */
unsigned short Utils_getRegionId();

/**
 * To check whether the global EDMA3 configuration is required or not.
 * It should be done ONCE by any of the masters present in the system.
 * This function checks whether the global configuration is required by the
 * current master or not. In case of many masters, it should be done only
 * by one of the masters. Hence this function will return TRUE only once
 * and FALSE for all other masters.
 */
unsigned short Utils_isGblConfigRequired(unsigned int dspNum);



/**
 * \brief   EDMA3 Initialization
 *
 * This function initializes the EDMA3 Driver for the given EDMA3 controller
 * and opens a EDMA3 driver instance. It internally calls EDMA3_DRV_create() and
 * EDMA3_DRV_open(), in that order.
 *
 * It also registers interrupt handlers for various EDMA3 interrupts like
 * transfer completion or error interrupts.
 *
 *  \param  edma3Id 	[IN]		EDMA3 Controller Instance Id (Hardware
 *									instance id, starting from 0)
 *  \param  errorCode 	[IN/OUT]	Error code while opening DRV instance
 *  \return EDMA3_DRV_Handle: If successfully opened, the API will return the
 *                            associated driver's instance handle.
 */
EDMA3_DRV_Handle Utils_edma3init (unsigned int edma3Id, EDMA3_DRV_Result *errorCode);

/**
 * \brief   EDMA3 De-initialization
 *
 * This function de-initializes the EDMA3 Driver for the given EDMA3 controller
 * and closes the previously opened EDMA3 driver instance. It internally calls
 * EDMA3_DRV_close and EDMA3_DRV_delete(), in that order.
 *
 * It also un-registers the previously registered interrupt handlers for various
 * EDMA3 interrupts.
 *
 *  \param  edma3Id 	[IN]		EDMA3 Controller Instance Id (Hardware
 *									instance id, starting from 0)
 *  \param  hEdma		[IN]		EDMA3 Driver handle, returned while using
 *									edma3init().
 *  \return  EDMA3_DRV_SOK if success, else error code
 */
EDMA3_DRV_Result Utils_edma3deinit (unsigned int edma3Id, EDMA3_DRV_Handle hEdma);

/**
 * \brief   EDMA3 open
 *
 * This function initializes the EDMA3 Driver for the given EDMA3 controller
 * and opens a EDMA3 RM instance. It internally calls EDMA3_RM_open(),
 *  in that order.
 *
 * It also registers interrupt handlers for various EDMA3 interrupts like
 * transfer completion or error interrupts.
 *
 *  \param  edma3Id 	[IN]		EDMA3 Controller Instance Id (Hardware
 *									instance id, starting from 0)
 *  \param  errorCode 	[IN/OUT]	Error code while opening RM instance
 *  \return EDMA3_RM_Handle: If successfully opened, the API will return the
 *                            associated driver's instance handle.
 */
EDMA3_RM_Handle Utils_edma3Open (unsigned int edma3Id, EDMA3_RM_Result *errorCode);

/**
 * \brief   EDMA3 Close
 *
 * This function de-initializes the EDMA3 Driver for the given EDMA3 controller
 * and closes the previously opened EDMA3 RM instance. It internally calls
 * EDMA3_RM_close, in that order.
 *
 * It also un-registers the previously registered interrupt handlers for various
 * EDMA3 interrupts.
 *
 *  \param  edma3Id 	[IN]		EDMA3 Controller Instance Id (Hardware
 *									instance id, starting from 0)
 *  \param  rmEdma		[IN]		EDMA3 RM handle, returned while using
 *									edma3open().
 *  \return  EDMA3_RM_SOK if success, else error code
 */
EDMA3_RM_Result Utils_edma3Close (unsigned int edma3Id, EDMA3_RM_Handle rmEdma);

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
 * \param   hSem [OUT] is location to receive the handle to just created
 *      semaphore.
 * \return  EDMA3_DRV_SOK if successful, else a suitable error code.
 */
EDMA3_DRV_Result Utils_edma3OsSemCreate(int initVal,
							const Semaphore_Params *semParams,
							EDMA3_OS_Sem_Handle *hSem);



/**
 * \brief   EDMA3 OS Semaphore Delete
 *
 *      This function deletes or removes the specified semaphore
 *      from the system. Associated dynamically allocated memory
 *      if any is also freed up.
 * \param   hSem [IN] handle to the semaphore to be deleted
 * \return  EDMA3_DRV_SOK if successful else a suitable error code
 */
EDMA3_DRV_Result Utils_edma3OsSemDelete(EDMA3_OS_Sem_Handle hSem);



void Utils_dmaCheckStaticAllocationConlficts();

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif  /* _BIOS6_EDMA3_DRV_SAMPLE_H_ */


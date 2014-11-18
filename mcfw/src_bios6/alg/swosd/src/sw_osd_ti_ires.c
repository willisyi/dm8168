/*
 * Copyright (c) 2011, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 *  ======== sw_osd_ti_ires.c ========
 *  EDMA Codec1 algorithm's implementation of IRES_Fxns.
 */

#include <sw_osd_ti_priv.h>

/* IRES Function Declarations */
static IRES_Status SWOSD_TI_activateRes(IALG_Handle handle, IRES_Handle res);
static IRES_Status SWOSD_TI_activateAllRes(IALG_Handle handle);
static IRES_Status SWOSD_TI_deactivateRes(IALG_Handle h, IRES_Handle res);
static IRES_Status SWOSD_TI_deactivateAllRes(IALG_Handle handle);
static Int32 SWOSD_TI_numResources(IALG_Handle handle);
static IRES_Status SWOSD_TI_getResources(IALG_Handle handle,
        IRES_ResourceDescriptor *desc);
static IRES_Status SWOSD_TI_initResources(IALG_Handle h,
        IRES_ResourceDescriptor * desc, IRES_YieldFxn  yieldFxn,
        IRES_YieldArgs yieldArgs);
static IRES_Status SWOSD_TI_deInitResources(IALG_Handle h,
        IRES_ResourceDescriptor *desc);
static IRES_Status SWOSD_TI_reInitResources(IALG_Handle handle,
        IRES_ResourceDescriptor *desc, IRES_YieldFxn  yieldFxn,
        IRES_YieldArgs yieldArgs);

/*
 *  ======== SWOSD_TI_IRES ========
 */
IRES_Fxns SWOSD_TI_IRES = {
    &SWOSD_TI_IRES,
    SWOSD_TI_getResources,
    SWOSD_TI_numResources,
    SWOSD_TI_initResources,
    SWOSD_TI_reInitResources,
    SWOSD_TI_deInitResources,
    SWOSD_TI_activateRes,
    SWOSD_TI_activateAllRes,
    SWOSD_TI_deactivateRes,
    SWOSD_TI_deactivateAllRes
};

static UInt32 linked_params[SWOSD_DMA_CH_MAX]=
{
	4,   /* Channel  0:   */
	4,   /* Channel  1:   */
	1,   /* Channel  2:   */
	1,   /* Channel  3:   */
      1   /* Channel  4:    */
};

/*
 *  ======== SWOSD_TI_activateAllRes ========
 */
static IRES_Status SWOSD_TI_activateAllRes(IALG_Handle handle)
{

    (void)handle; /* kill warnings */
    /* Activate all resources - this example has only one. */
    /* Can be empty if no activation is necessary, or no IRES_SCRATCH resources
       were requested. */

    return (IRES_OK);
}

/*
 *  ======== SWOSD_TI_activateRes ========
 */
static IRES_Status SWOSD_TI_activateRes(IALG_Handle handle,
        IRES_Handle res)
{
    (void)handle; /* kill warnings */

    /* Check that res = alg->nullres */

    return (IRES_OK);
}

/*
 *  ======== SWOSD_TI_deactivateAllRes ========
 */
static IRES_Status SWOSD_TI_deactivateAllRes(IALG_Handle handle)
{
    IRES_Status      status = IRES_OK;

    (void)handle; /* kill warnings */

    /* Finalization or saving of state associated with IRES_SCRATCH resources
       can be done here. After this point, current codec no longer exclusively
       owns the scratch resources */

    return (status);
}

/*
 *  ======== SWOSD_TI_deactivateRes ========
 */
static IRES_Status SWOSD_TI_deactivateRes(IALG_Handle h, IRES_Handle res)
{
    (void)h;   //kill warnings
    (void)res; //kill warnings

    /* Check that res = alg->nullres */

    return (IRES_OK);
}

/*
 *  ======== SWOSD_TI_deInitResources ========
 */
static IRES_Status SWOSD_TI_deInitResources(IALG_Handle h,
        IRES_ResourceDescriptor *desc)
{
	SWOSD_TI_Obj  *alg = (SWOSD_TI_Obj *)h;
	UInt8 i;

	for(i = 0; i < SWOSD_DMA_CH_MAX; i++)
	{
		if (desc[i].handle == (IRES_Handle)alg->edmaHandle[i]) {
			ECPY_deleteHandle(alg->ecpyHandle[i]);
			ECPY_exit();

			alg->ecpyHandle[i] = NULL;
			alg->edmaHandle[i] = NULL;
		}
	}

	return (IRES_OK);
}

/*
 *  ======== SWOSD_TI_getResources ========
 */
static IRES_Status SWOSD_TI_getResources(IALG_Handle h,
        IRES_ResourceDescriptor *desc)
{
    SWOSD_TI_Obj  *alg = (SWOSD_TI_Obj *)h;
    IRES_EDMA3CHAN_ProtocolArgs  *edma3ProtocolArgs;	
    UInt32 i;

	IRES_EDMA3CHAN_SETPROTOCOLREVISION_2_0_0(&(alg->edmaRev));
    /*
     * This API could be called to query for resource requirements and after
     * having granted the resources, could also be queried for resource
     * holdings of the algorithm. The difference is that in the second case a
     * valid resource handle (that had been granted earlier) would be expected.
     */
	for(i = 0; i < SWOSD_DMA_CH_MAX; i++)
	{
		desc[i].resourceName = IRES_EDMA3CHAN_PROTOCOLNAME;
		//Request IRES_EDMA3CHAN version 2.0.0 to use with ECPY
		desc[i].revision = &(alg->edmaRev);

		edma3ProtocolArgs = &(alg->edma3ProtocolArgs[i]);

		edma3ProtocolArgs->size = sizeof(IRES_EDMA3CHAN_ProtocolArgs);
		edma3ProtocolArgs->mode = IRES_SCRATCH;

		edma3ProtocolArgs->numPaRams = linked_params[i];
		edma3ProtocolArgs->paRamIndex = IRES_EDMA3CHAN_PARAM_ANY;
		edma3ProtocolArgs->tccIndex = IRES_EDMA3CHAN_TCC_ANY;
		edma3ProtocolArgs->numTccs = 1;

		edma3ProtocolArgs->qdmaChan = IRES_EDMA3CHAN_CHAN_NONE;
		edma3ProtocolArgs->edmaChan = IRES_EDMA3CHAN_EDMACHAN_ANY;
		edma3ProtocolArgs->contiguousAllocation = TRUE;
		desc[i].protocolArgs = (IRES_ProtocolArgs *)edma3ProtocolArgs;
		desc[i].handle = (IRES_Handle)alg->edmaHandle[i];
	}

    return (IRES_OK);
}

/*
 *  ======== SWOSD_TI_initResources ========
 */
static IRES_Status SWOSD_TI_initResources(IALG_Handle h,
        IRES_ResourceDescriptor *desc, IRES_YieldFxn  yieldFxn,
        IRES_YieldArgs yieldArgs)
{
    SWOSD_TI_Obj  *alg = (SWOSD_TI_Obj *)h;
	SWOSD_DMAObj *dmaHandle = (SWOSD_DMAObj *)&alg->dmaHandle;
	UInt8 i;

    ECPY_init();

    /*
     * Resource manager has returned a resource handle. Save it in the
     * algorithm's instance object
     */
	for(i = 0; i < SWOSD_DMA_CH_MAX; i++)
	{
        IRES_EDMA3CHAN2_Handle edma3Handle = (IRES_EDMA3CHAN2_Handle) desc[i].handle;
				
		alg->edmaHandle[i] = edma3Handle;
		alg->ecpyHandle[i] = ECPY_createHandle(edma3Handle, h);

		dmaHandle->edma3ResourceHandles[i] = edma3Handle;
        /* Initialize EDMA resource structures for H264VDEC_DMAOpen() call */
        dmaHandle->edma_params_array[i]   = (UInt32 *) edma3Handle->assignedPaRamAddresses[0];
        dmaHandle->edma_tcc_array[i]      = edma3Handle->assignedTccIndices[0];
        dmaHandle->edma_phy_to_lgl_map[i] = edma3Handle->assignedEdmaChannelIndex;
	}
	
	dmaHandle->num_edma_channels = SWOSD_DMA_CH_MAX;
	dmaHandle->channel_mask_low = 0;
	dmaHandle->channel_mask_high = 0;
    return (IRES_OK);
}

/*
 *  ======== SWOSD_TI_numResources ========
 */
/* ARGSUSED */
static Int32 SWOSD_TI_numResources(IALG_Handle handle)
{
    return (SWOSD_DMA_CH_MAX);
}

/*
 *  ======== SWOSD_TI_reInitResources ========
 */
static IRES_Status SWOSD_TI_reInitResources(IALG_Handle handle,
        IRES_ResourceDescriptor *desc, IRES_YieldFxn  yieldFxn,
        IRES_YieldArgs yieldArgs)
{
    SWOSD_TI_Obj  *alg = (SWOSD_TI_Obj *)handle;
	UInt8 i;
	/*
	* This function implies that the resource holdings of the algorithms have
	* been changed.
	* Update them in the algorithm instance object.
	*/
	for(i = 0; i < SWOSD_DMA_CH_MAX; i++)
	{
		alg->edmaHandle[i] = (IRES_EDMA3CHAN2_Handle)desc[i].handle;
	}

    return (IRES_OK);
}
/*
 *  @(#) ti.sdo.fc.ires.examples.codecs.universal_dma; 1, 0, 0,1; 5-24-2011 12:32:07; /db/atree/library/trees/fc/fc.git/src/ fc-o19
 */


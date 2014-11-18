/*
 * HELLOWORLDALG_TI_ialg.c
 *
 *  Created on: Oct 18, 2011
 *      Author: a0216851
 */


/*
 *  Copyright 2011
 *  Texas Instruments Incorporated
 *
 *  All rights reserved.  Property of Texas Instruments Incorporated
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */
#include "helloWorldAlg.h"
#include "helloWorldAlg_TI_priv.h"
#include "../../mcfw/src_bios6/utils/utils_common.h"


/*------------------------------- MACROS ------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Set MTAB_NRECS to the number of Memtabs required for the algorithm.       */
/*---------------------------------------------------------------------------*/
#define MTAB_NRECS              8

#define IALGFXNS  \
    &HELLOWORLDALG_TI_IALG,   /* module ID */                         \
    NULL,                     /* activate */                          \
    HELLOWORLDALG_TI_alloc,   /* alloc */                             \
    NULL,                     /* control (NULL => no control ops) */  \
    NULL,                     /* deactivate */                        \
    HELLOWORLDALG_TI_free, 	  /* free */                              \
    HELLOWORLDALG_TI_initObj, /* init */                              \
    NULL,                     /* moved */                             \
    HELLOWORLDALG_TI_numAlloc /* numAlloc (NULL => IALG_MAXMEMRECS) */


IALG_Fxns HELLOWORLDALG_TI_IALG = {      /* module_vendor_interface */
    IALGFXNS
};

/* ===================================================================
*  @func     HELLOWORLDALG_TI_numAlloc
*
*  @desc     DSKT2 memory manager queries this algorithm for
*            number of different memory allocation requirements
*
*  @modif    This function modifies the following structures
*
*  @inputs   This function takes no inputs
*
*  @outputs  <argument name>
*            Total number of memory table required by this algorithm 
*
*  @return   Status of process call
*  ==================================================================
*/
int HELLOWORLDALG_TI_numAlloc(void)
{
    return (int)HELLOWORLDALG_MEM_REC_MAX;
}

/* ===================================================================
*  @func     HELLOWORLDALG_TI_alloc
*
*  @desc     DSKT2 memory manager make this function call to know the
*            memory allocation details of algorithm
*
*  @modif    This function modifies the following structures
*
*  @inputs   This function takes the following inputs
*            <IALG_Params>
*            Algorithm create time parameters
*            <IALG_Fxns>
*            pointer to ialg functions vector table. This is not used
*            by the module. It is part of ialg protocol
*           <IALG_MemRec>
*            Memory table records that algorithm needs to fill
*
*  @outputs  <argument name>
*            Description of usage
*
*  @return   Status of process call
*  ==================================================================
*/
int 
HELLOWORLDALG_TI_alloc(const IALG_Params    *algParams, 
             IALG_Fxns            **pf, 
             IALG_MemRec          memTab[])
{
    const HELLOWORLDALG_createPrm *params = (HELLOWORLDALG_createPrm *)algParams;
    U32 bufferSize              = params->maxWidth * params->maxHeight * params->maxChannels;
    S32	numBuffersRequested     = (int)HELLOWORLDALG_MEM_REC_MAX;
    S32 bytesPERSISTDARAM0      = 0;
    S32 bytesPERSISTEXTERNAL    = 0;
    S32 bytesSCRATCHDARAM0      = 0;

    (void)pf;	// not used; cast removes compiler warning


	/*************************************************************************/
	/* First entry is always the Algorithm object. This is handle to         */
	/* the algorithm                                                         */
	/*************************************************************************/	
    memTab[0].size 		= sizeof(HELLOWORLDALG_TI_Obj);
    memTab[0].alignment	= 8;
    memTab[0].space     = IALG_DARAM0;
    memTab[0].attrs     = IALG_PERSIST;
	bytesPERSISTDARAM0 += memTab[0].size;

	/*************************************************************************/
	/* Memory requirement details for internal memory. If you want to share  */
	/* DSP internal memory with other algorithms, request it as scracth      */
	/* memory, else you can set the memory attribute as IALG_PERSIST.        */
	/* 256 KB of L2 memory available on DSP out of which 128 KB set as cache */
	/* by application and hence 128 KB available as internal heap memory.    */
	/* Based on your algorithm need you can change the size of cache and heap*/
	/* If DSKT2 memory manager fails to allocate from internal memory, it    */
	/* will allocate the memory from external heap.                          */
	/* Here you can either request for one big chunk of internal memory from */
	/* DSKT2 memory manager and internally divide the memory pool among      */
	/* various buffers of algorithm or you can have many entries in memTab   */
	/* table for internal memory requirements of individual buffers          */
	/*************************************************************************/	
    memTab[HELLOWORLDALG_MEM_REC_INTERNAL].size 		= 
		HELLOWORLDALG_MEM_BYTES_INTERNAL;  
    memTab[HELLOWORLDALG_MEM_REC_INTERNAL].alignment	= 8;
    memTab[HELLOWORLDALG_MEM_REC_INTERNAL].space 		= IALG_DARAM0;
    memTab[HELLOWORLDALG_MEM_REC_INTERNAL].attrs 		= IALG_SCRATCH;
    bytesSCRATCHDARAM0                     += 
		memTab[HELLOWORLDALG_MEM_REC_INTERNAL].size;

	/*************************************************************************/	
	/* Request for memory from external memory pool. Again, based on algorith*/
	/* need you can request for scratch or persistent memory type here.      */
	/*************************************************************************/	
    memTab[HELLOWORLDALG_MEM_REC_EXTERNAL].size 	   = (bufferSize >> 5) * sizeof(int);
    memTab[HELLOWORLDALG_MEM_REC_EXTERNAL].alignment   = 128;
    memTab[HELLOWORLDALG_MEM_REC_EXTERNAL].space 	   = IALG_EXTERNAL;
    memTab[HELLOWORLDALG_MEM_REC_EXTERNAL].attrs 	   = IALG_PERSIST;
    bytesPERSISTEXTERNAL                   += memTab[HELLOWORLDALG_MEM_REC_EXTERNAL].size;

	/*************************************************************************/	
	/* Memory request from internal heap for algorithm static structure.     */
	/*************************************************************************/	
    memTab[HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM].size 		= sizeof(HELLOWORLDALG_Params);
    memTab[HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM].alignment	= 8;
    memTab[HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM].space 		= IALG_DARAM0;
    memTab[HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM].attrs 		= IALG_PERSIST;
	bytesPERSISTDARAM0                     += memTab[HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM].size;
	
	/*************************************************************************/	
	/* Request for further memory here by incrementing the memtab index      */ 
	/*************************************************************************/	
     Vps_printf("\n\n> HELLOWORLDALG: Algorithm memory requirements queried ::");
    Vps_printf("\n>      Number of buffers = %d ", numBuffersRequested);
    Vps_printf("\n>      Persistent Internal Memory   = %d bytes", bytesPERSISTDARAM0);
    Vps_printf("\n>      Persistent External Memory   = %d bytes", bytesPERSISTEXTERNAL);
    Vps_printf("\n>      Scratch Internal Memory      = %d bytes", bytesSCRATCHDARAM0);

    return numBuffersRequested;
}


/* ===================================================================
*  @func     HELLOWORLDALG_TI_free
*
*  @desc     DSKT2 memory manager make this function call to free the
*            memory allocated to the algorithm
*
*  @modif    This function modifies the following structures
*
*  @inputs   This function takes the following inputs
*            <IALG_Params>
*            Algorithm create time parameters
*           <IALG_MemRec>
*            Memory table records that algorithm needs to fill
*
*  @outputs  <argument name>
*            Description of usage
*
*  @return   Status of process call
*  ==================================================================
*/

int HELLOWORLDALG_TI_free(IALG_Handle handle, IALG_MemRec memTab[])
{
    (void)handle;	// not used; cast removes compiler warning
    return (HELLOWORLDALG_TI_alloc(NULL, NULL, memTab));
}


/* ===================================================================
*  @func     HELLOWORLDALG_TI_initObj
*
*  @desc     Once DSKT2 memory manager has allocated memory, it makes 
*            this function call. Here algorithm get pointers to those 
*            memory and initializes it's state buffer.
*
*  @modif    This function modifies the following structures
*
*  @inputs   This function takes the following inputs
*            <IALG_Handle>
*            Handle to the algoirthm 
*           <IALG_MemRec>
*            Memory table records with allocated buffer pointers
*            <IALG_Handle>
*            Not used by algorithm. This is to maintain IALG interface
*            protocol. 
*            <IALG_Params>
*            Algorithm create time parameters
*
*  @outputs  <argument name>
*            Description of usage
*
*  @return   Status of process call
*  ==================================================================
*/

int HELLOWORLDALG_TI_initObj(IALG_Handle 			handle,
                   const IALG_MemRec 	memTab[],
                   IALG_Handle 			p,
                   const IALG_Params 	*algParams)
{
    const HELLOWORLDALG_createPrm *params	= (HELLOWORLDALG_createPrm *)algParams;
    HELLOWORLDALG_TI_Obj			*obj	= (HELLOWORLDALG_TI_Obj *)handle;
    HELLOWORLDALG_Params			*helloWorldAlg	= NULL;

    HELLOWORLDALG_Status status;
    U08 i;

    (void)p;	// not used; cast removes compiler warning
    Vps_printf("\n> HELLOWORLDALG: Initializing Hello World Algorithm... ");

    if(memTab[HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM].base != NULL)
    {
        obj->helloWorldAlg = memTab[HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM].base;
        helloWorldAlg = (HELLOWORLDALG_Params *)obj->helloWorldAlg;
    }
    else
    {
        Vps_printf("\n> HELLOWORLDALG: Application memory allocation ==> FAILED FOR memTab #%d!!!!\n", HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM);
        return (HELLOWORLDALG_ERR_MEMORY_INSUFFICIENT);
    }

    // Pass memory allocation addresses for the memory tables
    for (i = 0; i < HELLOWORLDALG_MEM_REC_MAX; i++)
    {
        if (memTab[i].base == NULL)
        {
            status = HELLOWORLDALG_ERR_MEMORY_INSUFFICIENT;
            Vps_printf("\n> HELLOWORLDALG: Application memory allocation ==> FAILED FOR memTab #%d!!!!\n", i);

            return status;
        }

		/*********************************************************************/
        /* Initialize Hello World algorithm buffer pointers here             */
		/*********************************************************************/
		/* 		       
        helloWorldAlg->buf[i]		 = memTab[i].base;
        */

		Vps_printf("\n> HELLOWORLDALG: Application memory allocation ==> memTab #%d", i);
		Vps_printf("\n       Bytes    = %d",   memTab[i].size);
		Vps_printf("\n       Address  = 0x%x", memTab[i].base);
		if (memTab[i].space == IALG_EXTERNAL)
			Vps_printf("\n       Location = EXTERNAL DDR MEM");
		else
			Vps_printf("\n       Location = INTERNAL L1/2 MEM");
    }

    /*********************************************************************/
    /* Initialize HELLOWORLDALG Instance here based on various create    */
    /* time parameter (HELLOWORLDALG_createPrm) set by the user          */
    /*********************************************************************/
	helloWorldAlg->maxWidth  = (S16)params->maxWidth;
	helloWorldAlg->maxHeight = (S16)params->maxHeight;
	helloWorldAlg->maxStride = (S16)params->maxStride;

    return (HELLOWORLDALG_NO_ERROR);
}


/* ===================================================================
*  @func     HELLOWORLDALG_TI_process
*
*  @desc     Hellow world algorithm Process call to process upon the 
*            buffers received at DSP side
*
*  @modif    This function modifies the following structures
*
*  @inputs   This function takes the following inputs
*            <IALG_Handle>
*            Handle to the algoirthm 
*            <U32 chanID>
*            Input Channel id that algorithm need to process
*            <HELLOWORLDALG_Result *pHelloWorldResult>
*            Pointer to output buffer that processed algorithm 
*            can write the results
*
*  @outputs  <argument name>
*            Description of usage
*
*  @return   Status of process call
*  ==================================================================
*/

HELLOWORLDALG_Status
HELLOWORLDALG_TI_process(PTR        handle, 
						 U32        chanID,
                         HELLOWORLDALG_Result *pHelloWorldResult)
{
	HELLOWORLDALG_Status	status = HELLOWORLDALG_NO_ERROR;
//	HELLOWORLDALG_Params	* helloWorldAlg = (HELLOWORLDALG_Params  *)((HELLOWORLDALG_TI_Obj *)handle)->helloWorldAlg;

//	status = HELLOWORLDALG_process(helloWorldAlg, pHelloWorldResult);
    Vps_printf("HELLO WORLD ALG: Process frame\n");

	return status;
}


/* ===================================================================
*  @func     HELLOWORLDALG_TI_setPrms
*
*  @desc     Once DSKT2 memory manager has allocated memory, it makes 
*            this function call. Here algorithm get pointers to those 
*            memory and initializes it's state buffer.
*
*  @modif    This function modifies the following structures
*
*  @inputs   This function takes the following inputs
*            <IALG_Handle>
*            Handle to the algoirthm 
*           <IALG_MemRec>
*            Memory table records with allocated buffer pointers
*            <IALG_Handle>
*            Not used by algorithm. This is to maintain IALG interface
*            protocol. 
*            <IALG_Params>
*            Algorithm create time parameters
*
*  @outputs  <argument name>
*            Description of usage
*
*  @return   Status of process call
*  ==================================================================
*/

HELLOWORLDALG_Status 
HELLOWORLDALG_TI_setPrms(PTR         	handle,
               HELLOWORLDALG_chPrm   *helloWorldAlgPrm,
               U32        chanID)
{
//	HELLOWORLDALG_Params	* helloWorldAlg = (HELLOWORLDALG_Params  *)((HELLOWORLDALG_TI_Obj *)handle)->helloWorldAlg;

    /* All run time dynamic params settings here */

    return HELLOWORLDALG_NO_ERROR;
}

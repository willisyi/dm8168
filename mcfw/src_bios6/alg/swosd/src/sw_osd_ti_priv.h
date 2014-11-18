/*
 *  Copyright 2007 by Texas Instruments Incorporated.
 *
 *  All rights reserved. Property of Texas Instruments Incorporated.
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */

#ifndef _SWOSD_TI_PRIV_
#define _SWOSD_TI_PRIV_

#include <xdc/std.h>
#include <sw_osd.h>

#include <string.h>


#include <ti/xdais/ialg.h>
#include <ti/xdais/ires.h>
#include <ti/sdo/fc/ires/edma3chan/ires_edma3Chan.h>
#include <ti/sdo/fc/ecpy/ecpy.h>
#include <ti/sdo/fc/dskt2/dskt2.h>
#include <ti/sdo/fc/rman/rman.h>


// define's
#define SWOSD_DMA_HNDL_LINKED_DMA   0
#define SWOSD_DMA_HNDL_MEMCPY       1
#define SWOSD_DMA_HNDL_MAX          4//(SWOSD_DMA_HNDL_MEMCPY+1)

#define SWOSD_DMA_CH_IN_A  0x0
#define SWOSD_DMA_CH_IN_B  0x1
#define SWOSD_DMA_CH_ALP   0x2
#define SWOSD_DMA_CH_OUT  0x3
#define SWOSD_DMA_CH_AUX  0x4

#define SWOSD_DMA_CH_MAX   (SWOSD_DMA_CH_AUX+1)

// data structure's

typedef struct SWOSD_DMAObj {
	IRES_EDMA3CHAN2_Handle edma3ResourceHandles[SWOSD_DMA_CH_MAX];
	UInt32 *edma_params_array[SWOSD_DMA_CH_MAX];
	UInt32 edma_tcc_array[SWOSD_DMA_CH_MAX];
	UInt32 edma_phy_to_lgl_map[SWOSD_DMA_CH_MAX];
	UInt32 num_edma_channels;
	UInt32 channel_mask_low;
	UInt32 channel_mask_high;
} SWOSD_DMAObj;

typedef struct SWOSD_TI_Obj {
	IALG_Obj    alg;            /* MUST be first field of all XDAS algs */
	XDAS_Int32  viewState;
	XDAS_Int32  extendedError;
	IRES_EDMA3CHAN2_Handle      edmaHandle[SWOSD_DMA_CH_MAX];
	ECPY_Handle                 ecpyHandle[SWOSD_DMA_CH_MAX];
    IRES_ProtocolRevision       edmaRev;  //{2,0,0} Version of resource;
	IRES_EDMA3CHAN_ProtocolArgs edma3ProtocolArgs[SWOSD_DMA_CH_MAX];
	UInt8 *memLineBuf;
	SWOSD_Obj  swOsdCtrl;
	SWOSD_DMAObj dmaHandle;
} SWOSD_TI_Obj;

typedef struct {

    UInt32 size;
    UInt32 maxWidth;
    UInt32 maxHeight;

} SWOSD_Params;

// extern's

extern IALG_Fxns SWOSD_TI_IALG ;
extern IRES_Fxns SWOSD_TI_IRES;


// functions
Int SWOSD_TI_numAlloc(void);
Int SWOSD_TI_alloc(const IALG_Params *algParams, IALG_Fxns **pf, IALG_MemRec memTab[]);
Int SWOSD_TI_free(IALG_Handle handle, IALG_MemRec memTab[]);
Int SWOSD_TI_initObj(IALG_Handle handle, const IALG_MemRec memTab[], IALG_Handle parent, const IALG_Params *algParams);

int SWOSD_TI_algRun     (SWOSD_TI_Obj *swOsdObj, SWOSD_Obj  *swOsdCtrl);
int SWOSD_TI_algMemcpy2D(SWOSD_TI_Obj *swOsdObj, UInt8 *dest, UInt8 *src, UInt16 width, UInt16 height, UInt16 lineOffsetSrc, UInt16 lineOffsetDest);
void SWOSD_TI_dmaActivate(SWOSD_TI_Obj *swOsdObj);
void SWOSD_TI_dmaDeactivate(SWOSD_TI_Obj *swOsdObj);
void SWOSD_TI_dmaSetup(SWOSD_TI_Obj *swOsdObj, UInt16 width, Bool dmaAlpha);
void SWOSD_TI_dmaStart(SWOSD_TI_Obj *swOsdObj,
                       UInt8 *pInA, UInt8 *pInB,
                       UInt8 *pInAlp, UInt8 *pOut,
                       UInt64 *pLineBufA, UInt64 *pLineBufB,
                       Int32 *pLineBufAlp, UInt64 *pLineBufOut,
                       UInt16 width
                      ) ;
void SWOSD_TI_dmaWait(SWOSD_TI_Obj *swOsdObj);
int SWOSD_TI_algMemcpy2Dv2(SWOSD_TI_Obj *swOsdObj, UInt8 *dest, UInt8 *src, UInt16 width, UInt16 height, UInt16 lineOffsetSrc, UInt16 lineOffsetDest);
#endif
/*
 *  @(#) ti.xdais.dm.examples.vidanalytics_copy; 1, 0, 0,16; 11-25-2007 20:45:08; /db/wtree/library/trees/dais-i23x/src/
 */


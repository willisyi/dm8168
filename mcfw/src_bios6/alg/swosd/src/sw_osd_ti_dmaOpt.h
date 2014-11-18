/*
 *  Copyright 2008
 *  Texas Instruments Incorporated
 *
 *  All rights reserved.  Property of Texas Instruments Incorporated
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */
#ifndef _SWOSD_TI_DMA_OPT_
#define _SWOSD_TI_DMA_OPT_

#include <edma_csl.h>

// defines's
#define MAX_EDMA_TRIGGERS (0x0FFF)
#define CSL_EDMACC_0_REGS (0x09000000)

#define SWOSD_TI_DMA_setTriggerWord(evt_no, lowTriggerWord, highTriggerWord)\
{\
  if(evt_no < 32)\
  {\
	lowTriggerWord |= (CSL_EDMACC_ESR_E0_SET << evt_no);\
  }\
  else\
  {\
	highTriggerWord |= (CSL_EDMACC_ESR_E0_SET << (evt_no - 32));\
  }\
}

#define SWOSD_TI_DMA_ManualUpdate(paramPtr, addr, value)\
{\
	 addr += value;\
   *(paramPtr) = addr;\
}

/** ===========================================================================
*
* @func    SWOSD_TI_DMA_Fast2D2D()
*
* @brief   This function does a simple transfer of bytes from source to
*          destination in a 2D-2D fashion. This function initiates the
*          transfer.
*
* @param   SWOSD_DMAObj  *dmaHandle
*            -> Pointer to SWOSD_DMAObj structure.
*
* @param   UInt32  chan_num
*            -> Channel number upon which DMA is fired.
*
* @param   void *restrict src
*            -> Pointer to source address.
*
* @param   void *restrict dst
*            -> Pointer to destination address.
*
* @param   UInt32 num_bytes
*           -> Number of bytes to be transferred.
*
* @param   UInt32 num_lines
*           -> Number of lines to be transferred.
*
* @param   Int32 src_pitch
*           -> Source pitch.
*
* @param   Int32 dst_pitch
*           -> Destination pitch.
*
* @return  Returns EDMA channel number.
*
* @note    None.
*
* ===========================================================================*/

Int32 SWOSD_TI_DMA_Fast2D2D
(
    SWOSD_DMAObj  *dmaHandle,
	UInt32 chan_num,
	void *restrict src,
	void *restrict dst,
	UInt32 num_bytes,
	UInt32 num_lines,
	Int32 src_pitch,
	Int32 dst_pitch
);

/** ===========================================================================
*
* @func    SWOSD_TI_DMA_Fast2D1D()
*
* @brief   This function does a simple transfer of bytes from source to
*          destination in a 2D-1D fashion. This function does not 
*          initiate the transfer.
*
* @param   SWOSD_DMAObj  *dmaHandle
*            -> Pointer to SWOSD_DMAObj structure.
*
* @param   UInt32  edma_channel_num
*            -> Channel number upon which DMA is fired.
*
* @param   void *restrict source
*            -> Pointer to source address.
*
* @param   void *restrict destination
*            -> Pointer to destination address.
*
* @param   UInt32 num_bytes
*           -> Number of bytes to be transferred.
*
* @param   UInt32 num_triggers
*           -> Total number of triggers.
*
* @param   Int32 source_increment
*           -> Source pitch.
*
* @param   Int32 destination_increment
*           -> Destination pitch.
*
* @param   Int32 source_reset
*           -> Source reset offset value.
*
* @param   Int32 destination_reset
*           -> Destination reset offset value.
*
* @return  Returns EDMA channel number.
*
* @note    None.
*
* ===========================================================================*/
Int32 SWOSD_TI_DMA_Fast2D1D
(
	SWOSD_DMAObj  *dmaHandle,
	UInt32 edma_channel_num,
	void *restrict source,
	void *restrict destination,
	UInt32 num_bytes,
	UInt32 num_triggers,
	Int32 source_increment,
	Int32 destination_increment,
	Int32 source_reset,
	Int32 destination_reset
);

/** ===========================================================================
*
* @func    SWOSD_TI_DMA_FastStart()
*
* @brief   This function initiates the transfer over a single DMA channel.
*
* @param   SWOSD_DMAObj  *dmaHandle
*            -> Pointer to SWOSD_DMAObj structure.
*
* @param   UInt32  chan_num
*            -> Channel number upon which DMA is fired.
*
* @return  None.
*
* @note    None.
*
* ===========================================================================*/
void SWOSD_TI_DMA_FastStart(SWOSD_DMAObj *dmaHandle, UInt32 chan_num);

/** ===========================================================================
*
* @func    SWOSD_TI_DMA_FastWait()
*
* @brief   This function waits on all the previously initiated DMA transfers.
*
* @param   SWOSD_DMAObj  *dmaHandle
*            -> Pointer to SWOSD_DMAObj structure.
*
* @return  None.
*
* @note    None.
*
* ===========================================================================*/
void SWOSD_TI_DMA_FastWait(SWOSD_DMAObj  *dmaHandle);

/** ===========================================================================
*
* @func    SWOSD_TI_DMA_MultiStart()
*
* @brief   This function initiates the transfer over a single DMA channel.
*
* @param   UInt32 lowTriggerWord
*            -> 32-bit word written to ESR register.
*
* @param   UInt32 highTriggerWord
*            -> 32-bit word written to ESRH register
*
* @return  None.
*
* @note    None.
*
* ===========================================================================*/
inline void SWOSD_TI_DMA_MultiStart(SWOSD_DMAObj  *dmaHandle, UInt32 lowTriggerWord, UInt32 highTriggerWord)
{
    volatile CSL_EdmaccRegsOvly edma_cc;
    edma_cc  =  (CSL_EdmaccRegsOvly) CSL_EDMACC_0_REGS;

    edma_cc->ESR  = lowTriggerWord;
    edma_cc->ESRH = highTriggerWord;
    dmaHandle->channel_mask_low  |= lowTriggerWord;	
	dmaHandle->channel_mask_high |= highTriggerWord;		
}

#endif // _SWOSD_TI_DMA_OPT_


/*
 *  Copyright 2008
 *  Texas Instruments Incorporated
 *
 *  All rights reserved.  Property of Texas Instruments Incorporated
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */
#include <sw_osd_ti_priv.h>
#include <sw_osd_ti_dmaOpt.h>

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
)
{
    volatile UInt32  *param_channel;
    volatile UInt32   tcc_chan, opt;
    volatile UInt32   evt_no;

    volatile CSL_EdmaccRegsOvly edma_cc;
    edma_cc  =  (CSL_EdmaccRegsOvly) CSL_EDMACC_0_REGS;

    param_channel   = dmaHandle->edma_params_array[chan_num];
    tcc_chan        = dmaHandle->edma_tcc_array[chan_num];

    /*-----------------------------------------------------------------------*/
    /*   The Options parameter explains the different                        */
    /*   features of the transfer that are being excercised.                 */
    /*   In this case the following values are used for                      */
    /*   each of the individual bits of the "OPT" field.                     */
    /*   We now start setting up the options for 1D -> 1D.                   */
    /*                                                                       */
    /*   BITS   CC      TYPE       DESCRIPTION AND VALUE                     */
    /*   31     PRIV     R         Privelege level, not                      */
    /*                             writeable by wdata.                       */
    /*                                                                       */
    /*   30..28 RSV      R         Reserved                                  */
    /*                                                                       */
    /*   27..24 PRIVID   R         Indicates privelege id                    */
    /*                             not writeable by wdata.                   */
    /*                                                                       */
    /*   23     ITCCCHEN W         Intermediate tranfer                      */
    /*                             completion chaining dis.                  */
    /*                                                                       */
    /*   22     TCCHEN   W         Transfer complete code                    */
    /*                             for chaining disabled.                    */
    /*                                                                       */
    /*   21     ITCINTEN W         Intermediate transfer                     */
    /*                             completion interrupt dis.                 */
    /*                                                                       */
    /*   20     TCINTEN  W         Transfer completion                       */
    /*                             interrupt enabled.                        */
    /*                                                                       */
    /*   19     WIMODE   W         0: Normal operation                       */
    /*                                                                       */
    /*   18     RSV                RESERVED                                  */
    /*                                                                       */
    /*   17-12  TCC      W         Transfer completion code                  */
    /*                             that will be posted IPR.                  */
    /*                                                                       */
    /*   11     TCCM     W         Mode 0:Normal completion                  */
    /*                                                                       */
    /*   10-8   FWID     W         000 FIFO with NA                          */
    /*                                                                       */
    /*   7-4    RSVD     W         0000                                      */
    /*                                                                       */
    /*   3      STATIC   W         1:Static PARAM update.                    */
    /*                                                                       */
    /*   2      SYNCDIM  W         0-AB_sync 1D -> 1D                        */
    /*                                                                       */
    /*   1      DAM      W         0-Incrementing.                           */
    /*                                                                       */
    /*   0      SAM      W         0-Incrementing.                           */
    /*-----------------------------------------------------------------------*/

    opt= CSL_EDMA_OPT_MAKE
         (
            CSL_EDMACC_OPT_ITCCHEN_DISABLE,
            CSL_EDMACC_OPT_TCCHEN_DISABLE,
            CSL_EDMACC_OPT_ITCINTEN_DISABLE,
            CSL_EDMACC_OPT_TCINTEN_ENABLE,
            tcc_chan,
            CSL_EDMACC_OPT_TCCMODE_NORMAL,
            CSL_EDMACC_OPT_FWID_8,
            CSL_EDMACC_OPT_STATIC_STATIC,
            CSL_EDMACC_OPT_SYNCDIM_ABSYNC,
            CSL_EDMACC_OPT_DAM_INCR,
            CSL_EDMACC_OPT_SAM_INCR
         );

    /*-----------------------------------------------------------------------*/
    /* Set up the Param entries in the params space                          */
    /*-----------------------------------------------------------------------*/

    param_channel[0]    = (UInt32)opt;
    param_channel[1]    = (UInt32)src;
    param_channel[2]    = (num_lines<<16)|num_bytes;
    param_channel[3]    = (UInt32)dst;
    param_channel[4]    = (dst_pitch<<16)|src_pitch;
    param_channel[5]    = (0<<16)|0xFFFF;
    param_channel[6]    = 0;
    param_channel[7]    = 0x1;

    /*-----------------------------------------------------------------------*/
    /* Now trigger the EDMA transfer by writing into the appr.               */
    /* location in the ESR register                                          */
    /*-----------------------------------------------------------------------*/

    evt_no  = dmaHandle->edma_phy_to_lgl_map[chan_num];

    if (evt_no < 32)
    {
        edma_cc->ESR  = (CSL_EDMACC_ESR_E0_SET << evt_no);
        dmaHandle->channel_mask_low |= (1<<evt_no);
    }
    else
    {
        edma_cc->ESRH = (CSL_EDMACC_ESR_E0_SET << (evt_no - 32));
        dmaHandle->channel_mask_high |= (1<<(evt_no - 32));
    }

    return (chan_num);

}

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
)
{
  Uint32 total_triggers;

  volatile Uint32  *param_channel;
  volatile Uint32   tcc_chan, opt;
  volatile Uint32   evt_no;
  
/*---------------------------------------------------------------------------*/
/* PONG: ORIGINAL MB DMA SETUP.                                              */
/*---------------------------------------------------------------------------*/

   total_triggers  = MAX_EDMA_TRIGGERS;

   param_channel   = dmaHandle->edma_params_array[edma_channel_num];
   tcc_chan        = dmaHandle->edma_tcc_array[edma_channel_num];

  opt= CSL_EDMA_OPT_MAKE
       (
          CSL_EDMACC_OPT_ITCCHEN_DISABLE,
          CSL_EDMACC_OPT_TCCHEN_DISABLE,
          CSL_EDMACC_OPT_ITCINTEN_ENABLE,
          CSL_EDMACC_OPT_TCINTEN_ENABLE,
          tcc_chan,
          CSL_EDMACC_OPT_TCCMODE_NORMAL,
          CSL_EDMACC_OPT_FWID_8,
          CSL_EDMACC_OPT_STATIC_NORMAL,
          CSL_EDMACC_OPT_SYNCDIM_ASYNC,
          CSL_EDMACC_OPT_DAM_INCR,
          CSL_EDMACC_OPT_SAM_INCR
       );

  /*---------------------------------------------------------------*/
  /* Set up the Param entries in the params space                  */
  /*---------------------------------------------------------------*/
  param_channel[0]    = (Uint32)opt;
  param_channel[1]    = (Uint32)source;
  param_channel[2]    = (num_triggers<<16)|num_bytes;
  param_channel[3]    = (Uint32)destination;
  param_channel[4]    = (destination_increment<<16)|source_increment;
  param_channel[5]    = (num_triggers<<16)|0xFFFF;
  param_channel[6]    = ((destination_reset<<16) | (source_reset & 0xFFFF));
  param_channel[7]    = total_triggers & 0xFFFF;
  
  return(edma_channel_num);
}

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
void SWOSD_TI_DMA_FastStart(SWOSD_DMAObj *dmaHandle, UInt32 chan_num)
{
    volatile CSL_EdmaccRegsOvly edma_cc;
    edma_cc  =  (CSL_EdmaccRegsOvly) CSL_EDMACC_0_REGS;
	UInt32 evt_no;
	
    evt_no  = dmaHandle->edma_phy_to_lgl_map[chan_num];

    if (evt_no < 32)
    {
        edma_cc->ESR  = (CSL_EDMACC_ESR_E0_SET << evt_no);
        dmaHandle->channel_mask_low |= (1<<evt_no);
    }
    else
    {
        edma_cc->ESRH = (CSL_EDMACC_ESR_E0_SET << (evt_no - 32));
        dmaHandle->channel_mask_high |= (1<<(evt_no - 32));
    }
}

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
void SWOSD_TI_DMA_FastWait(SWOSD_DMAObj  *dmaHandle)
{
   UInt32 restore_value;
   UInt32 channel_mask_low  = dmaHandle->channel_mask_low;
   UInt32 channel_mask_high = dmaHandle->channel_mask_high;
   volatile CSL_EdmaccRegsOvly edma_cc;
   edma_cc  =  (CSL_EdmaccRegsOvly) CSL_EDMACC_0_REGS;

   restore_value   = _disable_interrupts();

   while  (!((edma_cc->IPR & channel_mask_low) == channel_mask_low));
   edma_cc->ICR  = channel_mask_low;
   dmaHandle->channel_mask_low = 0;

   while  (!((edma_cc->IPRH & channel_mask_high) == channel_mask_high));
   edma_cc->ICRH  = channel_mask_high;
   dmaHandle->channel_mask_high = 0;

    _restore_interrupts(restore_value);
}

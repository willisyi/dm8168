/* link_codec.cmd */


SECTIONS 
{
  .text_hdvpss_drivers {
    *(.text:Vem_masterISR)
    *(.text:VpsUtils_quePut)
    *(.text:VpsUtils_queGet)
    *(.text:VpsUtils_queue)
    *(.text:VpsUtils_dequeue)
    *(.text:VpsUtils_linkNodeToHead)
    *(.text:VpsUtils_linkNodeToTail)
    *(.text:VpsUtils_unLinkNodeFromHead)
    *(.text:VpsUtils_isListEmpty)
    *(.text:VpsUtils_getHeadNode)
    *(.text:FVID2_queue)
    *(.text:FVID2_dequeue)
    *(.text:FVID2_processFrames)
    *(.text:FVID2_getProcessedFrames)
    *(.text:FVID2_checkFrameList)
    *(.text:FVID2_checkDqFrameList)
    *(.text:FVID2_checkProcessList)
    *(.text:FVID2_copyProcessList)
    *(.text:FVID2_duplicateFrameList)
    *(.text:fdmDriverCbFxn)
    -lhdvpss_drivers.aem3<vpshal_vpdma.oem3>(.text)
    *(.text:Mlm_submit)
    *(.text:mlmSendIntrIsr)
    *(.text:mlmListCompleteIsr)

    *(.text:VpsDlm_updateDescMem)
    *(.text:vpsDlmSendInterruptIsr)
    *(.text:Vps_captLmCompleteIsr)
    *(.text:Vps_captIsrListComplete)
    *(.text:Vps_captTskListUpdate)
    *(.text:Vps_captTskUpdateList)
    *(.text:Vps_captTskSubmitFieldList)
    *(.text:Vps_captTskSubmitFrameList)

    *(.text:VpsMdrv_deiProcessFrms)
    *(.text:VpsMdrv_deiGetProcessedFrms)
    *(.text:VpsMdrv_deiMlmCbFxn)
    *(.text:vpsMdrvDeiProcessReq)
    *(.text:vpsMdrvDeiUpdateChDesc)
    *(.text:vpsMdrvDeiRotateCtxBuf)
    *(.text:VpsMdrv_scProcessFrms)
    *(.text:VpsMdrv_scGetProcessedFrms)
    *(.text:VpsMdrv_scMlmCbFxn)


    *(.text:Vps_nsfProcessFrames)
    *(.text:Vps_nsfReqSubmit)
    *(.text:Vps_nsfGetProcessedFrames)
    *(.text:Vps_nsfReqGetProcessedFrames)

    *(.text:Vps_nsfChannelUpdateDescSet)
    *(.text:Vps_nsfReqGetFreeList)

    *(.text:Vps_nsfReqComplete)
    *(.text:VpsDdrv_queue)
    *(.text:VpsDdrv_dequeue)
    *(.text:vpsDdrvUpdateBufIsr)

    *(.text:vpsDrvGetNextBufState)

    *(.text:VpsHal_vpdmaPostList)
  } load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 


  .text_ipc_bios {
    -lipc.lib(.text)
/*    -lsysbios.lib(.text) */
  } load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 

  
  .text_deiLink {
    -ldvr_rdk_bios6.aem3<deiLink_drv.oem3>(.text)
    -ldvr_rdk_bios6.aem3<deiLink_tsk.oem3>(.text)    
  } load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 
  
  .text_nsfLink {
    -ldvr_rdk_bios6.aem3<nsfLink_drv.oem3>(.text)
    -ldvr_rdk_bios6.aem3<nsfLink_tsk.oem3>(.text)    
  } load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED  
  
  .text_swmsLink {
    -ldvr_rdk_bios6.aem3<swMsLink_drv.oem3>(.text)
    -ldvr_rdk_bios6.aem3<swMsLink_tsk.oem3>(.text)    
  } load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED  

  .text_captureLink {
    -ldvr_rdk_bios6.aem3<captureLink_drv.oem3>(.text)
    -ldvr_rdk_bios6.aem3<captureLink_tsk.oem3>(.text)    
  }  load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 
  
  .text_displayLink {
    -ldvr_rdk_bios6.aem3<displayLink_drv.oem3>(.text)
    -ldvr_rdk_bios6.aem3<displayLink_tsk.oem3>(.text)    
  }  load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 

  .text_dupLink {
    -ldvr_rdk_bios6.aem3<dupLink_tsk.oem3>(.text)    
  }  load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 

  .text_mergeLink {
    -ldvr_rdk_bios6.aem3<mergeLink_tsk.oem3>(.text)    
  }  load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 

  .text_ipcLink {
    -ldvr_rdk_bios6.aem3<ipcFramesOutLink_tsk.oem3>(.text)
    -ldvr_rdk_bios6.aem3<ipcOutM3Link_tsk.oem3>(.text)
    -ldvr_rdk_bios6.aem3<ipcInM3Link_tsk.oem3>(.text)
    -ldvr_rdk_bios6.aem3<ipcFramesInLink_tsk.oem3>(.text)
  }  load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 

  .text_avsyncLink {
    -ldvr_rdk_bios6.aem3<avsync_m3vpss.oem3>(.text)
  }  load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 

  .text_utils {
    -ldvr_rdk_bios6.aem3<utils_que.oem3>(.text)
    -ldvr_rdk_bios6.aem3<utils_mbx.oem3>(.text)
  } load >>  OCMC1_RAM, run >> OCMC1_RAM_MAPPED 
  
}

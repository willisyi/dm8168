

/* link_codec.cmd */
SECTIONS 
{
   .text_h264_vdec {
    -lh264vdec_ti.lib (.text)
  } load >>  L2_SRAM, run >> L2_SRAM_RUN

  .text_h264_venc {
    -lh264enc_ti_host.lib (.text)
  } load >>  L2_SRAM, run >> L2_SRAM_RUN

  .text_ivahd_api_lib {
    -livahd_ti_api_vM3.lib (.text)
  } load >>  L2_SRAM, run >> L2_SRAM_RUN

  .text_ipc_bios {
    -lipc.lib(.text)
/*    -lsysbios.lib(.text) */
  } load >>  OCMC0_RAM, run >> OCMC0_RAM_MAPPED 

  .text_fc_links {
    -lhdvicp2.aem3<iresman_hdvicp2.oem3>(.text)
  } load >>  OCMC0_RAM, run >> OCMC0_RAM_MAPPED 
  
  .text_encLink {
    -ldvr_rdk_bios6.aem3<encLink_common.oem3>(.text)
    -ldvr_rdk_bios6.aem3<encLink_h264.oem3>(.text)    
    -ldvr_rdk_bios6.aem3<encLink_tsk.oem3>(.text)        
  } load >>  OCMC0_RAM, run >> OCMC0_RAM_MAPPED 
  
  .text_decLink {
    -ldvr_rdk_bios6.aem3<decLink_common.oem3>(.text)
    -ldvr_rdk_bios6.aem3<decLink_h264.oem3>(.text)    
    -ldvr_rdk_bios6.aem3<decLink_tsk.oem3>(.text)        
  } load >>  OCMC0_RAM, run >> OCMC0_RAM_MAPPED  
  
  .text_ipcLink {
    -ldvr_rdk_bios6.aem3<ipcBitsInLink_tsk.oem3>(.text)
    -ldvr_rdk_bios6.aem3<ipcBitsOutLink_tsk.oem3>(.text)    
    -ldvr_rdk_bios6.aem3<ipcOutM3Link_tsk.oem3>(.text)
    -ldvr_rdk_bios6.aem3<ipcInM3Link_tsk.oem3>(.text)    
  } load >>  OCMC0_RAM, run >> OCMC0_RAM_MAPPED  
  
  .text_utils {
    -ldvr_rdk_bios6.aem3<utils_que.oem3>(.text)  
    -ldvr_rdk_bios6.aem3<utils_mbx.oem3>(.text)
  } load >>  OCMC0_RAM, run >> OCMC0_RAM_MAPPED 
  
}


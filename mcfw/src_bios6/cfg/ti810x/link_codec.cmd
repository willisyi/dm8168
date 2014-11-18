/* link_codec.cmd */
SECTIONS 
{
   .text_h264_vdec {
    -lh264vdec_ti.lib (.text)
  } load >>  M3_L2_RAM, run >> M3_L2_RAM_MAPPED 

  .text_h264_venc {
    -lh264enc_ti_host.lib<h264enc_ti_process.obj> (.text)
    -lh264enc_ti_host.lib<h264enc_ti_dpb.obj> (.text)
    -lh264enc_ti_host.lib<h264enc_ti_dataSynch.obj> (.text)
    -lh264enc_ti_host.lib<h264enc_ti_inbuf_control.obj> (.text)
    -lh264enc_ti_host.lib<H264BPEnc_ti_ires.obj> (.text)
  } load >>  M3_L2_RAM, run >> M3_L2_RAM_MAPPED 

  .text_ivahd_api_lib {
    -livahd_ti_api_vM3.lib (.text)
  } load >>  M3_L2_RAM, run >> M3_L2_RAM_MAPPED 

  .text_ipc_bios {
    -lipc.lib(.text)
/*  
    -lsysbios.lib(.text) 
*/
  } load >>  VIDEO_M3_CODE_MEM, run >> VIDEO_M3_CODE_MEM 

/*	
  .text_fc_links {
    -lhdvicp2.aem3<iresman_hdvicp2.oem3>(.text)
  } load >>  VIDEO_M3_CODE_MEM, run >> VIDEO_M3_CODE_MEM 
*/
  
  .text_encLink {
    -ldvr_rdk_bios6.aem3<encLink_common.oem3>(.text)
    -ldvr_rdk_bios6.aem3<encLink_h264.oem3>(.text)    
    -ldvr_rdk_bios6.aem3<encLink_tsk.oem3>(.text)        
  } load >>  VIDEO_M3_CODE_MEM, run >> VIDEO_M3_CODE_MEM 
  
  .text_decLink {
    -ldvr_rdk_bios6.aem3<decLink_common.oem3>(.text)
    -ldvr_rdk_bios6.aem3<decLink_h264.oem3>(.text)    
    -ldvr_rdk_bios6.aem3<decLink_tsk.oem3>(.text)        
  } load >>  VIDEO_M3_CODE_MEM, run >> VIDEO_M3_CODE_MEM  
  
  .text_ipcLink {
    -ldvr_rdk_bios6.aem3<ipcBitsInLink_tsk.oem3>(.text)
    -ldvr_rdk_bios6.aem3<ipcBitsOutLink_tsk.oem3>(.text)    
    -ldvr_rdk_bios6.aem3<ipcOutM3Link_tsk.oem3>(.text)
    -ldvr_rdk_bios6.aem3<ipcInM3Link_tsk.oem3>(.text)    
  } load >>  VIDEO_M3_CODE_MEM, run >> VIDEO_M3_CODE_MEM  
  
  .text_utils {
    -ldvr_rdk_bios6.aem3<utils_que.oem3>(.text)  
    -ldvr_rdk_bios6.aem3<utils_mbx.oem3>(.text)
  } load >>  VIDEO_M3_CODE_MEM, run >> VIDEO_M3_CODE_MEM 
  
  .bss_encdec_process_stack {
    -ldvr_rdk_bios6.aem3(.bss:taskStackSection:enc_process)  
    -ldvr_rdk_bios6.aem3(.bss:taskStackSection:dec_process)
  }  >> OCMC0_VIDEO_M3_RAM_MAPPED
  
  .bssNonCached :
  {
    *(.bss:extMemNonCache)
  } > REMOTE_DEBUG_MEM

/*
  .bssBitBufLinks :
  {
    -ldvr_rdk_bios6.aem3<ipcBitsInLink_tsk.oem3>(.bss)
    -ldvr_rdk_bios6.aem3<ipcBitsOutLink_tsk.oem3>(.bss)
  } > VIDEO_M3_BSS_ADDR_RUN, type = NOLOAD
*/
  .bss: 
  {
   *(.bss)
  } > VIDEO_M3_BSS_ADDR_RUN,type = NOLOAD

  .stack: > VIDEO_M3_BSS_ADDR_RUN ,type = NOLOAD



}


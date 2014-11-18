/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_m3video.h"
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/encLink.h>
#include <mcfw/interfaces/link_api/decLink.h>
#include <mcfw/interfaces/link_api/algLink.h>
#include <mcfw/src_bios6/utils/utils_dma.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>


System_VideoObj gSystem_objVideo;

Int32 System_init()
{
    Int32 status = FVID2_SOK;

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Video Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Video Init Done !!!\n", Utils_getCurTimeInMsec());
#endif
    IpcOutM3Link_init();
    IpcInM3Link_init();
    IpcFramesOutLink_init();
    IpcFramesInLink_init();
    IpcBitsInLink_init();
    IpcBitsOutLink_init();

    Utils_encdecInit();

#ifdef SYSTEM_SIMCOP_ENABLE
   Utils_dmaInit();
#endif

#ifdef SYSTEM_SIMCOP_ENABLE
    System_initIss();
#endif

    System_initLinks();

    return status;
}

Int32 System_deInit()
{
    System_deInitLinks();

#ifdef SYSTEM_SIMCOP_ENABLE
	   System_deInitIss();
#endif

#ifdef SYSTEM_SIMCOP_ENABLE
   Utils_dmaDeInit();
#endif

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Video De-Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Video De-Init Done !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Void System_initLinks()
{
    Vps_printf(" %d: SYSTEM  : Initializing Links !!! \r\n", Utils_getCurTimeInMsec());
    System_memPrintHeapStatus();

    EncLink_init();
    DecLink_init();
    //DupLink_init();
    MergeLink_init();

#ifdef SYSTEM_SIMCOP_ENABLE
   AlgLink_init();
#endif

    Vps_printf(" %d: SYSTEM  : Initializing Links ... DONE !!! \r\n",
               Utils_getCurTimeInMsec());
}

Void System_deInitLinks()
{
    Vps_printf(" %d: SYSTEM  : De-Initializing Links !!! \r\n",
               Utils_getCurTimeInMsec());

#ifdef SYSTEM_SIMCOP_ENABLE
	   AlgLink_deInit();
#endif

    IpcOutM3Link_deInit();
    IpcInM3Link_deInit();
    IpcFramesOutLink_deInit();
    IpcFramesInLink_deInit();
    IpcBitsInLink_deInit();
    IpcBitsOutLink_deInit();

    MergeLink_deInit();
    //DupLink_deInit();
    DecLink_deInit();
    EncLink_deInit();

    System_memPrintHeapStatus();

    Vps_printf(" %d: SYSTEM  : De-Initializing Links ... DONE !!! \r\n",
               Utils_getCurTimeInMsec());
}

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <xdc/std.h>
#include "system_priv_c6xdsp.h"
#include <mcfw/interfaces/link_api/algLink.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/src_bios6/links_c6xdsp/utils/utils_dsp.h>
#include <mcfw/interfaces/link_api/helloWorldLink.h>

#ifdef  DSP_RPE_AUDIO_ENABLE
#include "ti/rpe.h"
#endif

System_DspObj gSystem_objDsp;

Int32 System_init()
{
    Int32 status = FVID2_SOK;

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System DSP Init in progress !!!\n",
               Clock_getTicks());
#endif

#ifdef  DSP_RPE_AUDIO_ENABLE
    extern int32_t RpeServer_init (void *heapHdl);

    status = RpeServer_init(Utils_getAlgMemoryHeapHandle());
    Vps_printf(" %d: SYSTEM  : RpeServer_init() done... Ret Val %d!!!\n",
               Clock_getTicks(), status);
#endif

    IpcFramesInLink_init();
    IpcFramesOutLink_init();
    IpcBitsOutLink_init();

    Utils_dspInit();

    System_initLinks();

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System DSP Init Done !!!\n", Clock_getTicks());
#endif
    return status;
}

Int32 System_deInit()
{
#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Dsp De-Init in progress !!!\n",
               Clock_getTicks());
#endif

    IpcFramesInLink_deInit();
    IpcFramesOutLink_deInit();
    IpcBitsOutLink_deInit();

    System_deInitLinks();

    Utils_dspDeInit();
   
#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Dsp De-Init Done !!!\n",
               Clock_getTicks());
#endif
#ifdef  DSP_RPE_AUDIO_ENABLE
    extern int32_t RpeServer_deInit ();

    RpeServer_deInit();
    Vps_printf(" %d: SYSTEM  : RpeServer_deInit() done... !!!\n",
               Clock_getTicks());
#endif
    return FVID2_SOK;
}

Void System_initLinks()
{
    Int32 states=0;
    Vps_printf(" %d: SYSTEM  : Initializing Links !!! %d \r\n", Clock_getTicks(),states);
    System_memPrintHeapStatus();

    AlgLink_init();
    NullLink_init();
    MergeLink_init();
    SelectLink_init();
/*  write by royjwy */
    states=HelloWorldLink_init();
    Vps_printf(" %d: helloworldlink_init done!!!\r\n",Clock_getTicks());


    Vps_printf(" %d: SYSTEM  : Initializing Links ... DONE !!! \r\n",
               Clock_getTicks());
    
}

Void System_deInitLinks()
{
    Vps_printf(" %d: SYSTEM  : De-Initializing Links !!! \r\n",
               Clock_getTicks());

    SelectLink_deInit();
    MergeLink_deInit();
    NullLink_deInit();
    AlgLink_deInit();
/*  write by royjwy */
    HelloWorldLink_deInit();

    Vps_printf(" %d: SYSTEM  : De-Initializing Links ... DONE !!! \r\n",
               Clock_getTicks());
}

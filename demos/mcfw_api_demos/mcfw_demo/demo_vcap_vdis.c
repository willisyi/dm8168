/*==============================================================================
 * @file:       demo_vcap_vdis.c
 *
 * @brief:      Video capture mcfw function definition.
 *
 * @vers:       0.5.0.0 2011-06
 *
 *==============================================================================
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <demo.h>

Void VcapVdis_start()
{
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VDIS_PARAMS_S vdisParams;

    gDemo_info.maxVcapChannels = 16;
    gDemo_info.maxVdisChannels = 16;
    gDemo_info.maxVencChannels = 0;
    gDemo_info.maxVdecChannels = 0;

    vcapParams.numChn = 16;
    vdisParams.numChannels = 16;

    Vsys_params_init(&vsysParams);
    vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_VCAP_VDIS;
    vsysParams.enableCapture = TRUE;
    vsysParams.enableNsf     = TRUE;
    vsysParams.enableNullSrc = TRUE;
    vsysParams.numDeis       = 2;
    vsysParams.numSwMs       = 2;
    vsysParams.numDisplays   = 3;

    printf ("--------------- CHANNEL DETAILS-------------\n");
    printf ("Capture Channels => %d\n", vcapParams.numChn);
    printf ("Disp Channels => %d\n", vdisParams.numChannels);
    printf ("-------------------------------------------\n");

    /* Override the context here as needed */
    Vsys_init(&vsysParams);

    Vcap_params_init(&vcapParams);

    /* Override the context here as needed */
    Vcap_init(&vcapParams);

    Vdis_params_init(&vdisParams);

    /* Override the context here as needed */
    vdisParams.deviceParams[VDIS_DEV_HDMI].resolution   = DEMO_HD_DISPLAY_DEFAULT_STD;
    /* Since HDCOMP and DVO2 are tied together they must have same resolution */
    vdisParams.deviceParams[VDIS_DEV_HDCOMP].resolution = DEMO_HD_DISPLAY_DEFAULT_STD;
    vdisParams.deviceParams[VDIS_DEV_DVO2].resolution   =
                              vdisParams.deviceParams[VDIS_DEV_HDMI].resolution;
    vdisParams.deviceParams[VDIS_DEV_SD].resolution     = VSYS_STD_NTSC;
    Vdis_init(&vdisParams);

 	/* Configure display in order to start grpx before video */
 	Vsys_configureDisplay();

#if USE_FBDEV
    grpx_init(GRPX_FORMAT_RGB565);
#endif

    /* Create Link instances and connects compoent blocks */
    Vsys_create();

    /* Start components in reverse order */
    Vdis_start();
    Vcap_start();

}

Void VcapVdis_stop()
{
    /* Stop components */
    Vcap_stop();
    Vdis_stop();

#if USE_FBDEV
    grpx_exit();
#endif

    Vsys_delete();

	Vsys_deConfigureDisplay();

    /* De-initialize components */
    Vcap_exit();
    Vdis_exit();
    Vsys_exit();
}

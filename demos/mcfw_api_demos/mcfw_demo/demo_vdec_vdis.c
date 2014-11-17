/*==============================================================================
 * @file:       demo_vdec_vdis.c
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
#include <sys/time.h>
#include <demo_vdec_vdis.h>

//#define VDEC_VDIS_ENABLE_IPCFRAMESOUT                 (TRUE)

static Int64 get_current_time_to_msec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((Int64)tv.tv_sec*1000 + tv.tv_usec/1000);
}

Void VdecVdis_start()
{
    VSYS_PARAMS_S vsysParams;
    VDEC_PARAMS_S vdecParams;
    VDIS_PARAMS_S vdisParams;
    VDIS_MOSAIC_S sVdMosaicParam;

    UInt32 i,status;
    Bool forceLowCostScale = FALSE;
    UInt32 startChID;
    UInt64 wallTimeBase;

    VdecVdis_bitsRdInit();

    #ifdef VDEC_VDIS_ENABLE_IPCFRAMESOUT
    VdecVdis_ipcFramesCreate();
    #endif

    gDemo_info.maxVcapChannels = 0;
    gDemo_info.maxVdisChannels = gVdecVdis_config.fileNum;
    gDemo_info.maxVencChannels = 0;
    gDemo_info.maxVdecChannels = gVdecVdis_config.fileNum;

    vdecParams.numChn = gVdecVdis_config.fileNum;
    vdisParams.numChannels = gVdecVdis_config.fileNum;

    Vsys_params_init(&vsysParams);
    vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_VDEC_VDIS;
    vsysParams.enableCapture = FALSE;
    vsysParams.enableNsf     = FALSE;
    vsysParams.enableEncode  = FALSE;
    vsysParams.enableDecode  = TRUE;
    vsysParams.enableNullSrc = FALSE;
    vsysParams.enableAVsync  = TRUE;
    vsysParams.numDeis       = 0;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    vsysParams.numSwMs       = 2;
    vsysParams.numDisplays   = 2;
#else
    vsysParams.numSwMs       = 2;
    vsysParams.numDisplays   = 2;
#endif

    printf ("--------------- CHANNEL DETAILS-------------\n");
    printf ("Dec Channels => %d\n", vdecParams.numChn);
    printf ("Disp Channels => %d\n", vdisParams.numChannels);
    printf ("-------------------------------------------\n");

    /* Override the context here as needed */
    Vsys_init(&vsysParams);

    Vdec_params_init(&vdecParams);
    /* Override the context here as needed */

    vdecParams.numChn = gVdecVdis_config.fileNum;
    vdecParams.forceUseDecChannelParams = TRUE;

    OSA_assert( vdecParams.numChn <= VDEC_CHN_MAX );

    for (i=0; i < vdecParams.numChn; i++) {

        vdecParams.decChannelParams[i].dynamicParam.frameRate = 30; // NOT USED
        vdecParams.decChannelParams[i].dynamicParam.targetBitRate = 2 * 1000 * 1000; // NOT USED
        if (gVdecVdis_config.fileInfo[i].width != 0 && gVdecVdis_config.fileInfo[i].height != 0)
        {
            vdecParams.decChannelParams[i].maxVideoWidth = gVdecVdis_config.fileInfo[i].width;
            vdecParams.decChannelParams[i].maxVideoHeight = gVdecVdis_config.fileInfo[i].height;
        }
        else
        {
            printf(" ERROR: Invalid Decoder width x height !!!\n");
            OSA_assert(0);
        }

        /*If the codec type is missing, by default choose h264*/
        if(strlen(gVdecVdis_config.fileInfo[i].codec) == 0)
            strcpy(gVdecVdis_config.fileInfo[i].codec,"h264");

        if(strcmp(gVdecVdis_config.fileInfo[i].codec,"h264") == 0)
        {
            vdecParams.decChannelParams[i].isCodec = VDEC_CHN_H264;
            printf("ch[%d], h264\n",i);
        }
        else if(strcmp(gVdecVdis_config.fileInfo[i].codec,"mpeg4") == 0)
        {
            vdecParams.decChannelParams[i].isCodec = VDEC_CHN_MPEG4;
            vdecParams.decChannelParams[i].dynamicParam.frameRate = 30;
            vdecParams.decChannelParams[i].dynamicParam.targetBitRate = 2 * 1000 * 1000;

            printf("ch[%d], mpeg4\n",i);
        }
        else if(strcmp(gVdecVdis_config.fileInfo[i].codec,"mjpeg") == 0)
        {
            vdecParams.decChannelParams[i].isCodec = VDEC_CHN_MJPEG;
            vdecParams.decChannelParams[i].dynamicParam.frameRate = 1;
            vdecParams.decChannelParams[i].dynamicParam.targetBitRate = 2 * 1000 * 1000;
            printf("ch[%d], jpeg\n",i);

        }

        vdecParams.decChannelParams[i].displayDelay = gVdecVdis_config.fileInfo[i].displaydelay;
        vdecParams.decChannelParams[i].numBufPerCh = gVdecVdis_config.fileInfo[i].numbuf;

    }

    Vdec_init(&vdecParams);

    Vdis_params_init(&vdisParams);
    /* Override the context here as needed */
    vdisParams.numChannels = gVdecVdis_config.fileNum;

    vdisParams.deviceParams[VDIS_DEV_HDMI].resolution   = DEMO_HD_DISPLAY_DEFAULT_STD;
    /* Since HDCOMP and DVO2 are tied together they must have same resolution */
    vdisParams.deviceParams[VDIS_DEV_HDCOMP].resolution = DEMO_HD_DISPLAY_DEFAULT_STD;
    vdisParams.deviceParams[VDIS_DEV_DVO2].resolution   =
                           vdisParams.deviceParams[VDIS_DEV_HDMI].resolution;
    vdisParams.deviceParams[VDIS_DEV_SD].resolution     = VSYS_STD_NTSC;

    Vdis_tiedVencInit(VDIS_DEV_HDCOMP, VDIS_DEV_DVO2, &vdisParams);



#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    /* set for 2 displays */
    i = 0;
    Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, vdecParams.numChn,
                          DEMO_LAYOUT_MODE_4CH,
                          &vdisParams.mosaicParams[VDIS_DEV_HDMI], forceLowCostScale, gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
    vdisParams.mosaicParams[VDIS_DEV_HDMI].userSetDefaultSWMLayout = TRUE;

    if(vdecParams.numChn < 16)
        startChID = 0;
    else
        startChID = 16;
    i = 1;
    Demo_swMsGenerateLayout(VDIS_DEV_SD, startChID, vdecParams.numChn,
                          DEMO_LAYOUT_MODE_4CH,
                          &vdisParams.mosaicParams[VDIS_DEV_SD], forceLowCostScale, gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_SD].resolution);
    vdisParams.mosaicParams[VDIS_DEV_SD].userSetDefaultSWMLayout = TRUE;
#else
    /* set for 3 displays */
    i = 0;
    Demo_swMsGenerateLayout(VDIS_DEV_HDMI, 0, vdecParams.numChn,
                          DEMO_LAYOUT_MODE_4CH_4CH,
                          &vdisParams.mosaicParams[i], forceLowCostScale,
                          gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_HDMI].resolution);
    vdisParams.mosaicParams[i].userSetDefaultSWMLayout = TRUE;


    if(vdecParams.numChn < 16)
        startChID = 0;
    else
        startChID = 16;
    i = 1;
    Demo_swMsGenerateLayout(VDIS_DEV_HDCOMP, startChID, vdecParams.numChn,
                          DEMO_LAYOUT_MODE_4CH_4CH,
                          &vdisParams.mosaicParams[i], forceLowCostScale,
                          gDemo_info.Type,
                          vdisParams.deviceParams[VDIS_DEV_HDCOMP].resolution);
    vdisParams.mosaicParams[i].userSetDefaultSWMLayout = TRUE;

#endif
    Vdis_init(&vdisParams);

    wallTimeBase = get_current_time_to_msec();
    wallTimeBase = 0;
    Vdis_setWallTimeBase(wallTimeBase);

 	/* Configure display */
	Vsys_configureDisplay();

#if USE_FBDEV
    grpx_init(GRPX_FORMAT_RGB565);
#endif

    /* Create Link instances and connects compoent blocks */
    Vsys_create();

    /* This is done to re-map ch to window mappping when no. of chan are <16 */
    if(vdecParams.numChn < 16)
    {
        status = Vdis_getMosaicParams(1,&sVdMosaicParam);
        status = Vdis_setMosaicParams(1, &sVdMosaicParam);
    }

    /* Start components in reverse order */
    Vdis_start();
    Vdec_start();

    #ifdef VDEC_VDIS_ENABLE_IPCFRAMESOUT
    VdecVdis_ipcFramesStart();
    #endif

    VdecVdis_bitsRdStart();
}

Void VdecVdis_stop()
{
    VdecVdis_bitsRdStop();

    #ifdef VDEC_VDIS_ENABLE_IPCFRAMESOUT
    VdecVdis_ipcFramesStop();
    #endif

    /* Stop components */
    Vdec_stop();
    Vdis_stop();

#if USE_FBDEV
    grpx_exit();
#endif

    Vsys_delete();

 	/* De-configure display */
 	Vsys_deConfigureDisplay();


    /* De-initialize components */
    Vdec_exit();
    Vdis_exit();
    Vsys_exit();

    VdecVdis_bitsRdExit();

    #ifdef VDEC_VDIS_ENABLE_IPCFRAMESOUT
    VdecVdis_ipcFramesDelete();
    #endif

}


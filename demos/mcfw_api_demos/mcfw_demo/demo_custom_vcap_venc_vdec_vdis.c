/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/**
  \file demo_vcap_venc_vdis.c
  \brief
  */

#include <demo_custom_vcap_venc_vdec_vdis.h>


Void VcapVencVdecVdisCustom_setSwMs(UInt32 devId, UInt32 startChId, UInt32 layoutId, UInt32 resolution)
{
    VDIS_MOSAIC_S vdMosaicParam;

    Demo_swMsGenerateLayout(devId, startChId, gDemo_info.maxVdisChannels,
        layoutId, &vdMosaicParam, FALSE, DEMO_TYPE_PROGRESSIVE, resolution);
    Vdis_setMosaicParams(devId,&vdMosaicParam);
}

Void VcapVencVdecVdisCustom_setDefaultSwMs(UInt32 devId, VDIS_PARAMS_S *pContext)
{
    UInt32 row, col, outWidth, outHeight, winId, widthAlign=8, heightAlign=1;
    UInt32 rowMax = 1, colMax = 1;

    VDIS_MOSAIC_S *vdMosaicParam = &pContext->mosaicParams[devId];

    Demo_swMsGetOutSize(pContext->deviceParams[devId].resolution , &outWidth, &outHeight);

    vdMosaicParam->numberOfWindows = rowMax * colMax;
    vdMosaicParam->userSetDefaultSWMLayout = TRUE;
    vdMosaicParam->outputFPS = 30;

    for(row=0; row<rowMax; row++)
    {
        for(col=0; col<colMax; col++)
        {
            winId = row*colMax+col;

            vdMosaicParam->winList[winId].width = VsysUtils_align(outWidth/colMax, widthAlign);
            vdMosaicParam->winList[winId].height = VsysUtils_align(outHeight/rowMax, heightAlign);
            vdMosaicParam->winList[winId].start_X = vdMosaicParam->winList[winId].width*col;
            vdMosaicParam->winList[winId].start_Y = vdMosaicParam->winList[winId].height*row;

            vdMosaicParam->chnMap[winId] = winId;

            vdMosaicParam->useLowCostScaling[winId] = 0;
        }
   }
}

Void VcapVencVdecVdisCustom_start(Bool enableDecode, Bool enableCapture, UInt32 numVipInst)
{
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VENC_PARAMS_S vencParams;
    VDEC_PARAMS_S vdecParams;
    VDIS_PARAMS_S vdisParams;

    UInt32 i;

    gDemo_info.maxVcapChannels = 0;
    gDemo_info.maxVdisChannels = 0;
    gDemo_info.maxVencChannels = 0;
    gDemo_info.maxVdecChannels = 0;

    vcapParams.numChn          = 0;
    vencParams.numPrimaryChn   = 0;
    vencParams.numSecondaryChn = 0;
    vdisParams.numChannels     = 0;

    if(enableDecode)
    {
        VdecVdis_bitsRdInit();

        gDemo_info.maxVdecChannels = 1;
    }
    if(enableCapture)
    {
        if(numVipInst>1)
        {
            gDemo_info.maxVcapChannels = 5;
            gDemo_info.maxVencChannels = 2;
        }
        else
        {
            gDemo_info.maxVcapChannels = 2;
            gDemo_info.maxVencChannels = 2;
        }
    }

    gDemo_info.maxVdisChannels = gDemo_info.maxVcapChannels + gDemo_info.maxVdecChannels;

    /* init all Params */
    Vsys_params_init(&vsysParams);
    Vcap_params_init(&vcapParams);
    Venc_params_init(&vencParams);
    Vdec_params_init(&vdecParams);
    Vdis_params_init(&vdisParams);

    vcapParams.numChn          = gDemo_info.maxVcapChannels;
    vencParams.numPrimaryChn   = gDemo_info.maxVencChannels;
    vdisParams.numChannels     = gDemo_info.maxVdisChannels;

    /* set Vsys params */

    vsysParams.systemUseCase        = VSYS_USECASE_MULTICHN_CUSTOM;
    vsysParams.enableCapture        = enableCapture;
    vsysParams.enableDecode         = enableDecode;

    if(numVipInst>1)
        vsysParams.enableSecondaryOut   = TRUE;
    else
        vsysParams.enableSecondaryOut   = FALSE;


    /* set Vdec params */
    if(enableDecode)
    {
        vdecParams.numChn = gDemo_info.maxVdecChannels;
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
    }

    /* set Vdis params */
    vdisParams.deviceParams[VDIS_DEV_SD].resolution   = VSYS_STD_NTSC;
    VcapVencVdecVdisCustom_setDefaultSwMs(VDIS_DEV_SD, &vdisParams);

    vsysParams.numDeis       = 2;
    /* All params setting done */

    printf ("--------------- CHANNEL DETAILS-------------\n");
    printf ("Capture Channels => %d\n", vcapParams.numChn);
    printf ("Enc Channels => Primary %d, Secondary %d\n", vencParams.numPrimaryChn, vencParams.numSecondaryChn);
    printf ("Dec Channels => %d\n", vdecParams.numChn);
    printf ("Disp Channels => %d\n", vdisParams.numChannels);
    printf ("-------------------------------------------\n");

    /* init sub-systems */
    Vsys_init(&vsysParams);
    Vcap_init(&vcapParams);
    Venc_init(&vencParams);
    Vdec_init(&vdecParams);
    Vdis_init(&vdisParams);

    /* Init the application specific module which will handle bitstream exchange */
    VcapVenc_bitsWriteCreate();

 	/* Configure display */
 	Vsys_configureDisplay();

    /* Create Link instances and connects component blocks */
    Vsys_create();

    /* Start components in reverse order */
    Vdis_start();
    Venc_start();
    Vdec_start();
    Vcap_start();

    /* reset statistics */
    VcapVenc_resetStatistics();

    if(enableDecode)
        VdecVdis_bitsRdStart();

    if(enableCapture)
    {
        VcapVencVdecVdisCustom_setSwMs(VDIS_DEV_SD, 0, DEMO_LAYOUT_MODE_1CH /*DEMO_LAYOUT_MODE_7CH_1CH */, vdisParams.deviceParams[VDIS_DEV_SD].resolution);
    }
	else
    {
        VcapVencVdecVdisCustom_setSwMs(VDIS_DEV_SD, 0, DEMO_LAYOUT_MODE_1CH, vdisParams.deviceParams[VDIS_DEV_SD].resolution);
    }
}

Void VcapVencVdecVdisCustom_stop()
{
    if(gDemo_info.maxVdecChannels>0)
        VdecVdis_bitsRdStop();

    /* Stop sub-systems */
    Vcap_stop();
    Venc_stop();
    Vdec_stop();
    Vdis_stop();

    /* delete bits write */
    VcapVenc_bitsWriteDelete();

    /* system delete */
    Vsys_delete();

	Vsys_deConfigureDisplay();

    /* De-initialize components */
    Vcap_exit();
    Venc_exit();
    Vdec_exit();
    Vdis_exit();
    Vsys_exit();

    if(gDemo_info.maxVdecChannels>0)
        VdecVdis_bitsRdExit();
}



#include <demo.h>
#include <demo_scd_bits_wr.h>
#include <limits.h>


char gDemo_displaySettingsMenu[] = {
    "\r\n ====================="
    "\r\n Display Settings Menu"
    "\r\n ====================="
    "\r\n"
    "\r\n 1: Disable channel"
    "\r\n 2: Enable  channel"
    "\r\n 3: Switch Layout"
    "\r\n 4: Switch Channels"
    "\r\n 5: Change resolution"
    "\r\n 6: Switch Queue(ONLY FOR SD Display)"
    "\r\n 7: Switch Channel(ONLY FOR Enc HD Usecase)"
    "\r\n 8: Switch SDTV channel (ONLY for progressive demo)"
    "\r\n 9: 2x digital zoom in top left"
    "\r\n a: 2x digital zoom in center"
    "\r\n b: Avsync Pause "
    "\r\n c: Avsync Timescale Play "
    "\r\n d: Avsync Step Fwd "
    "\r\n e: Avsync Normal Play "
    "\r\n f: Avsync Seek Play "
    "\r\n g: Avsync Reset Player timer "
    "\r\n h: Avsync Set Player State Play "
    "\r\n i: Avsync Scan Play "
    "\r\n j: SwMS flush frames "
    "\r\n k: Run Grpx fbdev demo"
    "\r\n"
    "\r\n p: Previous Menu"
    "\r\n"
    "\r\n Enter Choice: "
};

char gDemo_displayLayoutMenu[] = {
    "\r\n ====================="
    "\r\n Select Display Layout"
    "\r\n ====================="
    "\r\n"
    "\r\n 1: 1x1 CH"
    "\r\n 2: 2x2 CH"
    "\r\n 3: 3x3 CH"
    "\r\n 4: 4x4 CH"
    "\r\n 5: 2x2 CH + 4CH"
    "\r\n 6: 1CH + 5CH"
    "\r\n 7: 1CH + 7CH"
    "\r\n 8: 1CH + 2CH PIP "
};

char gDemo_displayLayoutMenuDecDemoOnly[] = {
    "\r\n 9: 4x5 CH"
#ifdef TI_816X_BUILD
    "\r\n a: 5x5 CH"
    "\r\n b: 5x6 CH"
    "\r\n c: 6x6 CH"
#endif
};

char gDemo_displayLayoutMenuEnd[] = {
    "\r\n"
    "\r\n Enter Choice: "
};

char gDemo_displayMenu[] = {
    "\r\n ====================="
    "\r\n Select Display"
    "\r\n ====================="
    "\r\n"
    "\r\n 1: ON-Chip HDMI"
    "\r\n 2: VGA / HDCOMP "
    "\r\n 3: OFF-Chip HDMI"
    "\r\n 4: SD "
    "\r\n"
    "\r\n Enter Choice: "

};

char gDemo_ResolutionMenu[] = {
    "\r\n ====================="
    "\r\n Select Display"
    "\r\n ====================="
    "\r\n"
    "\r\n 1: 1080P60"
    "\r\n 2: 720P60"
    "\r\n 3: XGA"
    "\r\n 4: SXGA"
    "\r\n 5: NTSC"
    "\r\n 6: PAL"
    "\r\n 7: 1080P50"
    "\r\n"
    "\r\n Enter Choice: "

};
char gDemo_displayHDDemoChanMenu[] = {
    "\r\n ********                 Channel Mapping                 *******"
    "\r\n Channel 0 - Physical Channel 0   Channel 1 - Physical Channel  4"
    "\r\n Channel 2 - Physical Channel 8   Channel 3 - Physical Channel 12"
    "\r\n"
};

static int layoutId = DEMO_LAYOUT_MODE_7CH_1CH; // Have this static to use layoutId in Demo_displayChangeFpsForLayout() when channel remap alone happens

/*
  * This API is to reset the BIG LIVE channel(s) in a old layout to 30 / 25 fps
  */
Void Demo_displayResetFps(VDIS_MOSAIC_S *vdMosaicParam, UInt32 layoutId)
    {
       /* Set outputFPS for 814x usecases generating 60fps for channels shown as bigger window in some layouts */
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        Int32 currentFrameRate = Demo_swMsGetOutputFPS(vdMosaicParam);
        if (currentFrameRate == 50)
            currentFrameRate = 25;

        if (currentFrameRate == 60)
            currentFrameRate = 30;

        switch (layoutId)
        {
            /* Stream ID 0 refers to live channel for Vcap_setFrameRate */
            case DEMO_LAYOUT_MODE_4CH:
                printf ("4CH Layout, Resetting FPS of CH%d %d %d %d to %d/%dfps\n",
                        vdMosaicParam->chnMap[0],
                        vdMosaicParam->chnMap[1],
                        vdMosaicParam->chnMap[2],
                        vdMosaicParam->chnMap[3],
                        currentFrameRate*2, currentFrameRate
                    );
                Vcap_setFrameRate(vdMosaicParam->chnMap[0], 0, currentFrameRate*2, currentFrameRate);
                Vcap_setFrameRate(vdMosaicParam->chnMap[1], 0, currentFrameRate*2, currentFrameRate);
                Vcap_setFrameRate(vdMosaicParam->chnMap[2], 0, currentFrameRate*2, currentFrameRate);
                Vcap_setFrameRate(vdMosaicParam->chnMap[3], 0, currentFrameRate*2, currentFrameRate);
                break;

            case DEMO_LAYOUT_MODE_1CH:
            case DEMO_LAYOUT_MODE_6CH:
            case DEMO_LAYOUT_MODE_7CH_1CH:
                printf ("1CH Layout, Resetting FPS of CH%d to %d/%dfps\n",
                        vdMosaicParam->chnMap[0],
                        currentFrameRate*2, currentFrameRate
                    );

                Vcap_setFrameRate(vdMosaicParam->chnMap[0], 0, currentFrameRate*2, currentFrameRate);
                break;
        }
#else
    Int32 i=0;
    for (i=0; i<Vcap_getNumChannels(); i++)
        Vcap_setFrameRate(i, 0, 60, 0);
#endif
    }


/*
  * This API is to set have the LIVE channel(s) shown in bigger window <in some layouts> to be rendered
  *  at 60 / 50 fps for some 814x usecase
  */
Void Demo_displayChangeFpsForLayout (VDIS_MOSAIC_S *vdMosaicParam, UInt32 layoutId)
{
   /* Set outputFPS for 814x usecases generating 60fps for channels shown as bigger window in some layouts */
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    Int32 i, currentFrameRate = Demo_swMsGetOutputFPS(vdMosaicParam);
    if (currentFrameRate == 50)
        currentFrameRate = 25;

    if (currentFrameRate == 60)
        currentFrameRate = 30;

    switch (layoutId)
    {
        /* Stream ID 0 refers to live channel for Vcap_setFrameRate */
        case DEMO_LAYOUT_MODE_4CH:
            printf ("4CH Layout, Setting FPS of CH%d %d %d %d to %d/%dfps\n",
                    vdMosaicParam->chnMap[0],
                    vdMosaicParam->chnMap[1],
                    vdMosaicParam->chnMap[2],
                    vdMosaicParam->chnMap[3],
                    currentFrameRate*2, currentFrameRate*2
                );
            /* Reset all capture channels fps */
            for (i=0; i<Vcap_getNumChannels(); i++)
                Vcap_setFrameRate(i, 0, currentFrameRate*2, currentFrameRate);
#ifdef TI_814X_BUILD
            Vcap_setFrameRate(vdMosaicParam->chnMap[0], 0, currentFrameRate*2, currentFrameRate*2);
            Vcap_setFrameRate(vdMosaicParam->chnMap[1], 0, currentFrameRate*2, currentFrameRate*2);
            Vcap_setFrameRate(vdMosaicParam->chnMap[2], 0, currentFrameRate*2, currentFrameRate*2);
            Vcap_setFrameRate(vdMosaicParam->chnMap[3], 0, currentFrameRate*2, currentFrameRate*2);
            Demo_swMsSetOutputFPS(vdMosaicParam, currentFrameRate*2);
#else
            Vcap_setFrameRate(vdMosaicParam->chnMap[0], 0, currentFrameRate * 2, currentFrameRate);
            Vcap_setFrameRate(vdMosaicParam->chnMap[1], 0, currentFrameRate * 2, currentFrameRate);
            Vcap_setFrameRate(vdMosaicParam->chnMap[2], 0, currentFrameRate * 2, currentFrameRate);
            Vcap_setFrameRate(vdMosaicParam->chnMap[3], 0, currentFrameRate * 2, currentFrameRate);
            Demo_swMsSetOutputFPS(vdMosaicParam, currentFrameRate);
#endif
            break;

#ifdef TI_814X_BUILD
        case DEMO_LAYOUT_MODE_1CH:
        case DEMO_LAYOUT_MODE_6CH:
        case DEMO_LAYOUT_MODE_7CH_1CH:

            printf ("1CH Layout, Setting FPS of CH%d to %d/%dfps\n",
                    vdMosaicParam->chnMap[0],
                    currentFrameRate*2, currentFrameRate*2
                );

            /* Reset all capture channels fps */
            for (i=0; i<Vcap_getNumChannels(); i++)
                Vcap_setFrameRate(i, 0, currentFrameRate*2, currentFrameRate);
            Vcap_setFrameRate(vdMosaicParam->chnMap[0], 0, currentFrameRate*2, currentFrameRate*2);
            Demo_swMsSetOutputFPS(vdMosaicParam, currentFrameRate*2);
            break;
#else   // TI_8107_BUILD
        /* DM8107 supports 60 fps S/W mosaic only for 1x1 layout */
        case DEMO_LAYOUT_MODE_1CH:
            printf("1CH Layout, Setting FPS of CH%d to %d/%dfps\n",
                    vdMosaicParam->chnMap[0],
                    currentFrameRate * 2,
                    currentFrameRate * 2);

            /* Reset all capture channels fps */
            for (i = 0; i < Vcap_getNumChannels(); i++)
            {
                Vcap_setFrameRate(i, 0, currentFrameRate * 2, currentFrameRate);
            }
            Vcap_setFrameRate(
                    vdMosaicParam->chnMap[0],
                    0,
                    currentFrameRate * 2,
                    currentFrameRate * 2);
            Demo_swMsSetOutputFPS(vdMosaicParam, currentFrameRate * 2);
            break;
        case DEMO_LAYOUT_MODE_6CH:
        case DEMO_LAYOUT_MODE_7CH_1CH:
            printf("1CH Layout, Setting FPS of CH%d to %d/%dfps\n",
                    vdMosaicParam->chnMap[0],
                    currentFrameRate * 2,
                    currentFrameRate);

            /* Reset all capture channels fps */
            for (i = 0; i < Vcap_getNumChannels(); i++)
            {
                Vcap_setFrameRate(i, 0, currentFrameRate * 2, currentFrameRate);
            }
            Vcap_setFrameRate(
                    vdMosaicParam->chnMap[0],
                    0,
                    currentFrameRate * 2,
                    currentFrameRate);
            Demo_swMsSetOutputFPS(vdMosaicParam, currentFrameRate);
            break;
#endif

        default:
            printf ("NORMAL Layout, Setting FPS of all channels to %d/%dfps\n",
                    currentFrameRate*2, currentFrameRate
                );
            /* swMS fps would be have been modified during layout generation; change only Capture frame rate */
            for (i=0; i<Vcap_getNumChannels(); i++)
                Vcap_setFrameRate(i, 0, currentFrameRate*2, currentFrameRate);
    }
#else
    Int32 i;

    for (i=0; i<vdMosaicParam->numberOfWindows; i++)
    {
        VSYS_PARAMS_S sysContextInfo;
        Vsys_getContext(&sysContextInfo);
        if(sysContextInfo.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
        {
            if((vdMosaicParam->chnMap[i] < Vcap_getNumChannels()) && (vdMosaicParam->useLowCostScaling[i] == FALSE))
                Vcap_setFrameRate(vdMosaicParam->chnMap[i],0,60,30);
        }
        else
            if(vdMosaicParam->chnMap[i] < Vcap_getNumChannels())
                Vcap_setFrameRate(vdMosaicParam->chnMap[i],0,60,30);

    }
#endif
}


int Demo_displaySwitchChn(int devId, int startChId)
{
    UInt32 chMap[VDIS_MOSAIC_WIN_MAX];
    int i;

    for(i=0;i<VDIS_MOSAIC_WIN_MAX;i++)
    {
         if (i < gDemo_info.maxVdisChannels)
            chMap[i] = (i+startChId)%gDemo_info.maxVdisChannels;
         else
            chMap[i] = DEMO_SW_MS_INVALID_ID;

    }
    Vdis_setMosaicChn(devId, chMap);

    /* wait for the info prints to complete */
    OSA_waitMsecs(100);

    return 0;
}

int Demo_displaySwitchQueue(int devId, int queueId)
{
    Vdis_switchActiveQueue(devId,queueId);
    return 0;
}

Void MultiCh_carDVRSwitchDisplayCh(Int displayChId);
int Demo_displaySwitchCarDVRCh(int displayChNum)
{
    MultiCh_carDVRSwitchDisplayCh(displayChNum);
    return 0;
}

int Demo_displaySwitchSDChan(int devId, int chId)
{
    Vdis_switchSDTVChId(devId, chId);
    return 0;
}
int Demo_displaySwitchChannel(int devId, int chId)
{
    Vdis_switchActiveChannel(devId,chId);
    return 0;
}

int Demo_displayChnEnable(int chId, Bool enable)
{
    if(chId >= gDemo_info.maxVdisChannels)
    {
        return -1;
    }

    if(enable)
    {
        Vdis_enableChn(VDIS_DEV_HDMI,chId);
        Vdis_enableChn(VDIS_DEV_HDCOMP,chId);
        Vdis_enableChn(VDIS_DEV_SD,chId);
    }
    else
    {
        Vdis_disableChn(VDIS_DEV_HDMI,chId);
        Vdis_disableChn(VDIS_DEV_HDCOMP,chId);
        Vdis_disableChn(VDIS_DEV_SD,chId);
    }

    /* wait for the info prints to complete */
    OSA_waitMsecs(100);

    return 0;
}

int Demo_displayGetLayoutId(int demoId)
{
    char ch;
    int layoutId = DEMO_LAYOUT_MODE_4CH_4CH;
    Bool done = FALSE;

    while(!done)
    {
        printf(gDemo_displayLayoutMenu);

        if (demoId == DEMO_VDEC_VDIS)
            printf(gDemo_displayLayoutMenuDecDemoOnly);

        printf(gDemo_displayLayoutMenuEnd);

        ch = Demo_getChar();

        done = TRUE;

        switch(ch)
        {
            case '1':
                layoutId = DEMO_LAYOUT_MODE_1CH;
                break;
            case '2':
                layoutId = DEMO_LAYOUT_MODE_4CH;
                break;
            case '3':
                layoutId = DEMO_LAYOUT_MODE_9CH;
                break;
            case '4':
                layoutId = DEMO_LAYOUT_MODE_16CH;
                break;
            case '5':
                layoutId = DEMO_LAYOUT_MODE_4CH_4CH;
                break;
            case '6':
                layoutId = DEMO_LAYOUT_MODE_6CH;
                break;
            case '7':
                layoutId = DEMO_LAYOUT_MODE_7CH_1CH;
                break;
            case '8':
                layoutId = DEMO_LAYOUT_MODE_2CH_PIP;
                break;
            case '9':
                layoutId = DEMO_LAYOUT_MODE_20CH_4X5;
                break;
#if !defined(TI_814X_BUILD) && !defined(TI_8107_BUILD)
            case 'a':
                layoutId = DEMO_LAYOUT_MODE_25CH_5X5;
                break;
            case 'b':
                layoutId = DEMO_LAYOUT_MODE_30CH_5X6;
                break;
            case 'c':
                layoutId = DEMO_LAYOUT_MODE_36CH_6X6;
                break;
#endif
            default:
                done = FALSE;
                break;
        }
    }

    return layoutId;
}

int Demo_displaySetResolution(UInt32 displayId, UInt32 resolution)
{
    VDIS_MOSAIC_S vdisMosaicParams;
#if USE_FBDEV
    UInt32 outWidth, outHeight;
#endif

#if USE_FBDEV
#if defined(TI_814X_BUILD)
    /* Disable graphics through sysfs entries */
    if (displayId == VDIS_DEV_HDMI || displayId == VDIS_DEV_DVO2 ) {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);
    }
#endif

#if defined(TI_8107_BUILD)
    /* Disable graphics through sysfs entries */
    if (displayId == VDIS_DEV_HDMI || displayId == VDIS_DEV_HDCOMP ) {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);
    }
#endif

#ifdef TI_816X_BUILD
    /* Disable graphics through sysfs entries */
    Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);
    Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_OFF);
#endif

    if (displayId == VDIS_DEV_SD ) {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX2, VDIS_OFF);
    }
#endif
    Vdis_stopDrv(displayId);
    memset(&vdisMosaicParams, 0, sizeof(VDIS_MOSAIC_S));
    /* Start with default layout */
    Demo_swMsGenerateLayout(displayId, 0, gDemo_info.maxVdisChannels,
            layoutId,
            &vdisMosaicParams, FALSE,
            gDemo_info.Type,
            resolution);
    Demo_displayResetFps(&vdisMosaicParams,layoutId);
#ifdef TI_816X_BUILD
    Demo_displayChangeFpsForLayout(&vdisMosaicParams,layoutId);
#endif
    Vdis_setMosaicParams(displayId, &vdisMosaicParams);
    Vdis_setResolution(displayId, resolution);
    Vdis_startDrv(displayId);

#if USE_FBDEV
    Demo_swMsGetOutSize(resolution, &outWidth, &outHeight);
#if defined(TI_814X_BUILD)
    if((displayId == VDIS_DEV_HDMI) || (displayId == VDIS_DEV_DVO2))
        grpx_scale(VDIS_DEV_HDMI, 0, 0, outWidth, outHeight);
    if((displayId == VDIS_DEV_SD))
        grpx_scale(VDIS_DEV_SD, 0, 0, outWidth, outHeight);
    /* Enable graphics through sysfs entries */
    if (displayId == VDIS_DEV_HDMI || displayId == VDIS_DEV_DVO2 ) {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_ON);
    }
#endif
#if defined(TI_8107_BUILD)
    if((displayId == VDIS_DEV_HDMI) || (displayId == VDIS_DEV_HDCOMP))
        grpx_scale(VDIS_DEV_HDMI, 0, 0, outWidth, outHeight);
    if((displayId == VDIS_DEV_SD))
        grpx_scale(VDIS_DEV_SD, 0, 0, outWidth, outHeight);
    /* Enable graphics through sysfs entries */
    if (displayId == VDIS_DEV_HDMI || displayId == VDIS_DEV_HDCOMP) {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_ON);
    }

#endif
#ifdef TI_816X_BUILD
    if(displayId==VDIS_DEV_HDMI)
        grpx_scale(VDIS_DEV_HDMI, 0, 0, outWidth, outHeight);

    /* Enable graphics through sysfs entries */
    Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_ON);
    Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_ON);
#endif
    if (displayId == VDIS_DEV_SD ) {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX2, VDIS_ON);
    }

#endif

    return 0;
}

static
Void Demo_displayUpdateSeqId()
{
    gDemo_info.curDisplaySeqId++;
}

UInt32 Demo_displayGetCurSeqId()
{
    return (gDemo_info.curDisplaySeqId);
}

int Demo_displaySettings(int demoId)
{
    Bool done = FALSE;
    char ch;
    static VDIS_MOSAIC_S vdMosaicParam;
    UInt32 chId, startChId, displayId, resolution, displayChId;
    int prevLayoutId;
    Bool validRes = FALSE;
    static Int32 queueNo; // Should be static to retain current queue


    VDIS_DEV devId;
    Bool forceLowCostScale = FALSE;

#if defined(TI_816X_BUILD)
    if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)  ||
            (demoId == DEMO_VCAP_VENC_VDIS))
        layoutId = DEMO_LAYOUT_MODE_16CH;
#endif

    if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) ||
        (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH))
        forceLowCostScale = TRUE;

    if(gDemo_info.maxVdisChannels<=0)
    {
        printf(" \n");
        printf(" WARNING: Display NOT enabled, this menu is NOT valid !!!\n");
        return -1;
    }

    while(!done)
    {
        printf(gDemo_displaySettingsMenu);

        ch = Demo_getChar();

        switch(ch)
        {
            case '1':
                chId = Demo_getChId("DISPLAY", gDemo_info.maxVdisChannels);

                Demo_displayChnEnable(chId, FALSE);
                break;

            case '2':
                chId = Demo_getChId("DISPLAY", gDemo_info.maxVdisChannels);

                Demo_displayChnEnable(chId, TRUE);
                break;

            case '3':
               prevLayoutId = layoutId;
               if (demoId != VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
                {
                layoutId = Demo_displayGetLayoutId(demoId);

                /************* For devID = VDIS_DEV_HDMI *************/
                devId = VDIS_DEV_HDMI;
                startChId = 0;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                forceLowCostScale = FALSE;
#endif

                Demo_swMsGenerateLayout(devId, startChId, gDemo_info.maxVdisChannels,
                    layoutId, &vdMosaicParam, forceLowCostScale, gDemo_info.Type, Vdis_getResolution(devId) );
#ifdef TI_816X_BUILD
                if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)  ||
                    (demoId == DEMO_VCAP_VENC_VDIS))
                {
                    Demo_displayResetFps(&vdMosaicParam,layoutId);
                    Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
                }
#endif

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
#endif
                Vdis_setMosaicParams(devId,&vdMosaicParam);
                /******************************  ***********************/
                /************* For devID = VDIS_DEV_HDCOMP *************/
#ifdef TI_816X_BUILD
                devId = VDIS_DEV_HDCOMP;
                startChId = 0;

                /* if the number of channels being display are more than that can fit in 4x4 then
                    make the other channels appear on the second HD Display.
                    Otherwise show same channels on the other HD Display
                */
                if(gDemo_info.maxVdisChannels>VDIS_MOSAIC_WIN_MAX)
                    startChId = VDIS_MOSAIC_WIN_MAX;

                Demo_swMsGenerateLayout(devId, startChId, gDemo_info.maxVdisChannels,
                    layoutId, &vdMosaicParam, forceLowCostScale, gDemo_info.Type, Vdis_getResolution(devId));

                if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE) ||
                    (demoId == DEMO_VCAP_VENC_VDIS) )
                   Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);

                Vdis_setMosaicParams(devId,&vdMosaicParam);
#endif
                /******************************  ***********************/
                /************* For devID = VDIS_DEV_SD *************/
                devId = VDIS_DEV_SD;
                startChId = 0;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                if (layoutId == DEMO_LAYOUT_MODE_4CH)
                    forceLowCostScale = TRUE;
#endif
                Demo_swMsGenerateLayout(devId, startChId, gDemo_info.maxVdisChannels,
                    layoutId, &vdMosaicParam, forceLowCostScale, gDemo_info.Type, Vdis_getResolution(devId));

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
#endif

#ifdef TI_816X_BUILD
                if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)  ||
                    (demoId == DEMO_VCAP_VENC_VDIS))
                   Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
#endif

                Vdis_setMosaicParams(devId,&vdMosaicParam);

                /* wait for the info prints to complete */
                OSA_waitMsecs(500);
                }
                else
                {
                    printf(" This is not supported in this usecase");
                    OSA_waitMsecs(100);
                }
#if defined (TI_816X_BUILD) || defined (TI_814X_BUILD)
#if DEMO_SCD_ENABLE_MOTION_TRACKING
#if USE_FBDEV
                {
                   VSYS_PARAMS_S sysContextInfo;
                   Bool drawGrid = TRUE;
                   Vsys_getContext(&sysContextInfo);
#if defined (TI_816X_BUILD)
                   if(sysContextInfo.systemUseCase != VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
                   {
                      drawGrid = FALSE;
                   }
#endif

                   if(sysContextInfo.enableScd == TRUE && drawGrid)
                   {
                     Scd_trackLayout(vdMosaicParam.numberOfWindows, vdMosaicParam.chnMap[0]); /* To pass the number of windows on display in current layout  */ 
                   }
                 }
#endif
#endif
#endif
                break;

            case '4':
                if (demoId != VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
                {
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                chId = Demo_getChId("DISPLAY (HDMI)", gDemo_info.maxVdisChannels);

                if (Vdis_getMosaicParams(VDIS_DEV_HDMI,&vdMosaicParam) >= 0)
                {
                    Demo_displayResetFps(&vdMosaicParam,layoutId);
                }
                Demo_displaySwitchChn(VDIS_DEV_HDMI, chId);

                if (Vdis_getMosaicParams(VDIS_DEV_HDMI,&vdMosaicParam) >= 0)
                {
                    Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
                }

                /* wait for the info prints to complete */
                OSA_waitMsecs(500);


                chId = Demo_getChId("DISPLAY (SDTV)", gDemo_info.maxVdisChannels);

                if (Vdis_getMosaicParams(VDIS_DEV_SD,&vdMosaicParam) >= 0)
                {
                    Demo_displayResetFps(&vdMosaicParam,layoutId);
                }

                Demo_displaySwitchChn(VDIS_DEV_SD, chId);

                if (Vdis_getMosaicParams(VDIS_DEV_SD,&vdMosaicParam) >= 0)
                {
                    Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
                }

                /* wait for the info prints to complete */
                OSA_waitMsecs(500);
#else
                chId = Demo_getChId("DISPLAY (HDMI/SDTV)", gDemo_info.maxVdisChannels);
       

                Demo_displaySwitchChn(VDIS_DEV_HDMI, chId);

                if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE) || 
                     (demoId == DEMO_VCAP_VENC_VDIS))
                {
                 Vdis_getMosaicParams(VDIS_DEV_HDMI,&vdMosaicParam);

                 Demo_displayResetFps(&vdMosaicParam,layoutId);

                 Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
                }
                /* wait for the info prints to complete */
                OSA_waitMsecs(500);
#endif       
#if defined (TI_816X_BUILD) || defined (TI_814X_BUILD)
#if DEMO_SCD_ENABLE_MOTION_TRACKING
#if USE_FBDEV
                {
                   VSYS_PARAMS_S sysContextInfo;
                   Bool drawGrid = TRUE;
                   Vsys_getContext(&sysContextInfo);
#if defined (TI_816X_BUILD)
                   if(sysContextInfo.systemUseCase != VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)
                   {
                      drawGrid = FALSE;
                   }
#endif

                   if(sysContextInfo.enableScd == TRUE && drawGrid)
                   {
                     Scd_trackLayout(vdMosaicParam.numberOfWindows, vdMosaicParam.chnMap[0]); /* To pass the number of windows on display in current layout  */ 
                   }
                 }
#endif
#endif
#endif
                /* SDTV if enabled follows HDMI */
                Demo_displaySwitchChn(VDIS_DEV_SD, chId);

                if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE) || 
                     (demoId == DEMO_VCAP_VENC_VDIS))
                {
                 Vdis_getMosaicParams(VDIS_DEV_HDMI,&vdMosaicParam);

                 Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
                }
                /* wait for the info prints to complete */
                OSA_waitMsecs(500);



                chId = Demo_getChId("DISPLAY (HDCOMP/DVO2)", gDemo_info.maxVdisChannels);

                Demo_displaySwitchChn(VDIS_DEV_HDCOMP, chId);

                if ((demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE) || 
                     (demoId == DEMO_VCAP_VENC_VDIS))
                {
                 Vdis_getMosaicParams(VDIS_DEV_HDMI,&vdMosaicParam);


                 Demo_displayChangeFpsForLayout(&vdMosaicParam,layoutId);
                }
                OSA_waitMsecs(500);


                }
                else
                {
                    printf(" This is not supported in this usecase");
                    OSA_waitMsecs(100);
                }
                /* wait for the info prints to complete */
                OSA_waitMsecs(500);

                break;
            case '5':
                if (demoId != VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
                {
                printf(gDemo_displayMenu);
                displayId = Demo_getIntValue("Display Id", 1, 4, 1);
                displayId -= 1;
#if defined(TI_814X_BUILD)
                    if (displayId == VDIS_DEV_HDCOMP) {
                        printf("\nVenc Not supported !!\n");
                    }
                    else {
#endif
#if defined(TI_8107_BUILD)
                    if (displayId == VDIS_DEV_DVO2) {
                        printf("\nVenc Not supported !!\n");
                    }
                    else {
#endif
                        if ((demoId == VSYS_USECASE_MULTICHN_VCAP_VENC) &&
                            (displayId == VDIS_DEV_HDCOMP || displayId == VDIS_DEV_DVO2) )
                        {
                            printf(" This is not supported in this usecase");
                            OSA_waitMsecs(100);
                            break;
                        }
                        printf(gDemo_ResolutionMenu);
                        resolution = Demo_getIntValue("Display Id", 1, 7, 1);
                        switch(resolution) {
                            case 1:
                                if (displayId != VDIS_DEV_SD)
                                {
                                    resolution = VSYS_STD_1080P_60;
                                    validRes   = TRUE;
                                }
                                else
                                {
                                    printf("\n Resolution Not supported !!\n");
                                }
                            break;
                            case 2:
                                if (displayId != VDIS_DEV_SD)
                                {
                                    resolution = VSYS_STD_720P_60;
                                    validRes   = TRUE;
                                }
                                else
                                {
                                    printf("\n Resolution Not supported !!\n");
                                }
                            break;
                            case 3:
                                if (displayId != VDIS_DEV_SD)
                                {
                                    resolution = VSYS_STD_XGA_60;
                                    validRes   = TRUE;
                                }
                                else
                                {
                                    printf("\n Resolution Not supported !!\n");
                                }
                            break;
                            case 4:
                                if (displayId != VDIS_DEV_SD)
                                {
                                    resolution = VSYS_STD_SXGA_60;
                                    validRes   = TRUE;
                                }
                                else
                                {
                                    printf("\n Resolution Not supported !!\n");
                                }
                            break;
                            case 5:
                                if (displayId == VDIS_DEV_SD)
                                {
                                    resolution = VSYS_STD_NTSC;
                                    validRes   = TRUE;
                                }
                                else
                                {
                                    printf("\n Resolution Not supported !!\n");
                                }
                            break;
                            case 6:
                                if (displayId == VDIS_DEV_SD)
                                {
                                    resolution = VSYS_STD_PAL;
                                    validRes   = TRUE;
                                }
                                else
                                {
                                    printf("\n Resolution Not supported !!\n");
                                }
                            break;
                            case 7:
                                if (displayId != VDIS_DEV_SD)
                                {
                                    resolution = VSYS_STD_1080P_50;
                                    validRes   = TRUE;
                                }
                                else
                                {
                                    printf("\n Resolution Not supported !!\n");
                                }
                            break;
                            default:
                                resolution = VSYS_STD_1080P_60;

                        }

                        if (validRes) {
                            Demo_displaySetResolution(displayId, resolution);
                        }
                        validRes = FALSE;
#if defined(TI_814X_BUILD) || defined (TI_8107_BUILD)
                }
#endif
            }
            else
            {
                printf(" This is not supported in this usecase");
                OSA_waitMsecs(100);
            }
            break;
            case '6':
                {
                    queueNo = Demo_getIntValue("DISPLAY ACTIVE QUEUE(0/1) (only for SD)",
                                            0,
                                            1,
                                            0);
                    if (queueNo == 0)
                    {
                        /* Queue 0 has only 1 channel. Reset to Ch 0 while switching to Queue 0*/
                        printf ("Resetting to Ch 0\n");
                        Demo_displaySwitchSDChan(VDIS_DEV_SD,0);
                    }
                    Demo_displaySwitchQueue(VDIS_DEV_SD, queueNo);
                }
                break;
            case '7':
               if(demoId == VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
               {

                    if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                    {
                          printf(gDemo_displayHDDemoChanMenu);
                    }

                   Demo_displaySwitchChannel(
                      Demo_getIntValue("Select DISPLAY HDMI(0/1)",
                                       0,
                                       1,
                                       0),
                      Demo_getIntValue("DISPLAY Channel No. (0-3)",
                                       0,
                                       3,
                                       0));
               }
               else
               {
                  printf("Not Supported in this usecase\n");
               }
                break;
            case '8':
                if (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)
                {
                    Int32 chNo;

                    chNo = Demo_getIntValue("SDTV channel:",
                                             0,
                                             gDemo_info.maxVcapChannels-1,
                                             0);
                    #if 0
                    if (queueNo == 0)
                    {
                        chNo = 0;
                    }
                    #endif
                    Demo_displaySwitchSDChan(VDIS_DEV_SD, chNo);
                }
                if (demoId == DEMO_CARDVR_4CH)
                {
                    Int32 chNo;

                    chNo = Demo_getIntValue("SDTV channel:",
                                             0,
                                             2,
                                             0);
                    Demo_displaySwitchCarDVRCh(chNo);
                }
                break;

            case '9':
            case 'a':
                if (demoId != VSYS_USECASE_MULTICHN_HD_VCAP_VENC)
                {
                    {
                        WINDOW_S chnlInfo, winCrop;
                        UInt32 winId = 0;

                        devId = VDIS_DEV_HDMI;

                        Vdis_getChnlInfoFromWinId(devId, winId,&chnlInfo);
                        winCrop.width = chnlInfo.width/2;
                        winCrop.height = chnlInfo.height/2;

                        if (ch == '9')
                        {
                            winCrop.start_X = chnlInfo.start_X;
                            winCrop.start_Y = chnlInfo.start_Y;
                        }
                        else
                        {
                            winCrop.start_X = chnlInfo.start_X + chnlInfo.width/4;
                            winCrop.start_Y = chnlInfo.start_Y + chnlInfo.height/4;
                        }

                        Vdis_SetCropParam(devId, winId,winCrop);
                    }
                }
                else
                {
                    printf(" This is not supported in this usecase");
                    OSA_waitMsecs(100);
                }
                break;
            case 'b':
                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_pauseMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_pauseChn(devId, displayChId);
                }
                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_pauseMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_pauseChn(devId, displayChId);
                }
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_pauseMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_pauseChn(devId, displayChId);
                }

                break;
            case 'c':
            {
                UInt32 timescale,seqId;

                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                timescale = Demo_getIntValue("Playback Speed x1000",50,2000,1000);
                seqId = Demo_displayGetCurSeqId();
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_mosaicSetPlaybackSpeed(devId,timescale,seqId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_setPlaybackSpeed(devId,displayChId,timescale,seqId);
                }
                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_mosaicSetPlaybackSpeed(devId,timescale,seqId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_setPlaybackSpeed(devId,displayChId,timescale,seqId);
                }
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_mosaicSetPlaybackSpeed(devId,timescale,seqId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_setPlaybackSpeed(devId,displayChId,timescale,seqId);
                }

                break;
            }
            case 'd':
                devId = VDIS_DEV_HDMI;

                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_stepMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_stepChn(devId,displayChId);
                }

                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_stepMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_stepChn(devId,displayChId);
                }
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_stepMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_stepChn(devId,displayChId);
                }

                break;
            case 'e':
                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_resumeMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_resumeChn(devId,displayChId);
                }
                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_resumeMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_resumeChn(devId,displayChId);
                }
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_resumeMosaic(devId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_resumeChn(devId,displayChId);
                }
                break;
            case 'f':
            {
                UInt32 seekVidPTS,seqId;

                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                if (chId == gDemo_info.maxVdecChannels)
                {
                    displayChId = VDIS_CHN_ALL;
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                }
                seekVidPTS = Demo_getIntValue("Seek PTS",0,INT_MAX,0);
                if (VDIS_CHN_ALL == displayChId)
                {
                    Demo_displayUpdateSeqId();
                }
                seqId = Demo_displayGetCurSeqId();
                Vdis_seek(devId,displayChId,AVSYNC_INVALID_PTS,seekVidPTS,seqId);
                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                {
                    displayChId = VDIS_CHN_ALL;
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                }
                Vdis_seek(devId,displayChId,AVSYNC_INVALID_PTS,seekVidPTS,seqId);
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                {
                    displayChId = VDIS_CHN_ALL;
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                }
                Vdis_seek(devId,displayChId,AVSYNC_INVALID_PTS,seekVidPTS,seqId);

                break;
            }
            case 'g':
            {
                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                if (chId == gDemo_info.maxVdecChannels)
                {
                    Vdis_resetMosaicPlayerTime(devId);
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_resetChPlayerTime(devId,displayChId);
                }
                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                {
                    Vdis_resetMosaicPlayerTime(devId);
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_resetChPlayerTime(devId,displayChId);
                }
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                {
                    Vdis_resetMosaicPlayerTime(devId);
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_resetChPlayerTime(devId,displayChId);
                }

                break;
            }
            case 'h':
            {
                UInt32 seqId;

                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                seqId = Demo_displayGetCurSeqId();
                if (chId == gDemo_info.maxVdecChannels)
                {
                    Demo_displayUpdateSeqId();
                    seqId = Demo_displayGetCurSeqId();
                    Vdis_setMosaicPlayerStatePlay(devId,seqId);
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_setChPlayerStatePlay(devId,displayChId,seqId);
                }
                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_setMosaicPlayerStatePlay(devId,seqId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_setChPlayerStatePlay(devId,displayChId,seqId);
                }
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                    Vdis_setMosaicPlayerStatePlay(devId,seqId);
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_setChPlayerStatePlay(devId,displayChId,seqId);
                }

                break;
            }
            case 'i':
            {
                UInt32 frameDisplayDuration;
                UInt32 seqId;

                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels + 1);
                frameDisplayDuration = Demo_getIntValue("Scan Frame Display Duration",100,500,300);
                seqId = Demo_displayGetCurSeqId();
                if (chId == gDemo_info.maxVdecChannels)
                {
                    Demo_displayUpdateSeqId();
                    seqId = Demo_displayGetCurSeqId();
                    Vdis_scanMosaic(devId,frameDisplayDuration,seqId);
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_scanCh(devId,displayChId,frameDisplayDuration,seqId);
                }
                devId = VDIS_DEV_HDCOMP;
                if (chId == gDemo_info.maxVdecChannels)
                {
                    Vdis_scanMosaic(devId,frameDisplayDuration,seqId);
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_scanCh(devId,displayChId,frameDisplayDuration,seqId);
                }
                devId = VDIS_DEV_SD;
                if (chId == gDemo_info.maxVdecChannels)
                {
                    Vdis_scanMosaic(devId,frameDisplayDuration,seqId);
                }
                else
                {
                    Vdec_mapDec2DisplayChId(devId,chId,&displayChId);
                    Vdis_scanCh(devId,displayChId,frameDisplayDuration,seqId);
                }

                break;
            }
            case 'j':
            {
                UInt32 holdLastFrame;

                devId = VDIS_DEV_HDMI;
                chId = Demo_getChId("DISPLAY", gDemo_info.maxVdisChannels + 1);
                if (chId == gDemo_info.maxVdisChannels)
                {
                    chId = VDIS_CHN_ALL;
                }
                holdLastFrame = Demo_getIntValue("SWMS flush : Hold Last Frame (0 - FALSE/1 - TRUE)",0,1,1);
                Vdis_flushSwMs(devId,chId,holdLastFrame);
                break;
            }
            case 'k':
                //grpx_demo();
                break;
            case 'p':
                done = TRUE;
                break;
        }
    }

    return 0;

}





#include <demo.h>
#include <demo_scd_bits_wr.h>

char gDemo_captureSettingsMenu[] = {
    "\r\n ====================="
    "\r\n Capture Settings Menu"
    "\r\n ====================="
    "\r\n"
    "\r\n 1: Disable channel"
    "\r\n 2: Enable  channel"
    "\r\n 3: OSD Window position"
    "\r\n 4: OSD Window size"
    "\r\n 5: OSD Window transparency"
    "\r\n 6: OSD Window global alpha"
    "\r\n 7: OSD Window enable/disable"
    "\r\n 8: Detect video"
    "\r\n 9: Dynamic resolution change"
    "\r\n a: SCD mode selection"
    "\r\n b: SCD Tamper sensitivity"
#if defined(TI_8107_BUILD)
    "\r\n c: SCD ignore lights OFF flag"
    "\r\n d: SCD ignore lights ON flag"
#endif
    "\r\n e: Live Motion Detection (LMD) Configuration"
#if defined(TI_814X_BUILD)|| defined(TI_816X_BUILD)
    "\r\n f: BlindArea/PrivacyMask Configuration"
#endif
    "\r\n g: Print SCD frame status"
    "\r\n h: SCD Enable/Disable Motion Track on Display"
    "\r\n i: SCD Channel to be Tracked on Display"
    "\r\n"
    "\r\n p: Previous Menu"
    "\r\n"
    "\r\n Enter Choice: "
};

char gDemo_captureHDDemoChanMenu[] = {
    "\r\n ********                 Channel Mapping                 *******"
    "\r\n Channel 0 - Physical Channel 0   Channel 1 - Physical Channel  4"
    "\r\n Channel 2 - Physical Channel 8   Channel 3 - Physical Channel 12"
    "\r\n"
};

int  scdTileConfig[SCD_MAX_CHANNELS][SCD_MAX_TILES];

int Demo_captureGetCaptureChNum (int scdChNum)
{
    int newChNum = scdChNum;
    VSYS_PARAMS_S contextInfo;

    Vsys_getContext(&contextInfo);
/*
    if ((contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_VCAP_VENC)
            || (contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
            || (contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH)
            || (contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
        )
*/
    if(scdChNum >= Vcap_getNumChannels())
    {
        newChNum -=   Vcap_getNumChannels();
    }
    /* Please add for other usecase */
    return newChNum;
}

int Demo_captureGetScdChNum (int scdChNum)
{
    int newChNum;
#ifdef TI_816X_BUILD
    VSYS_PARAMS_S contextInfo;

    Vsys_getContext(&contextInfo);
    if (contextInfo.systemUseCase == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)
    {
        /* Only in Netra Progressive demo SCD channels starts from 0 */
        newChNum =   scdChNum;
    }
    else
#endif
    {
        /* In this case  SCD channels are in the secondary path hence need  to add offset */
        newChNum =   scdChNum + Vcap_getNumChannels();
    }
    /* Please add for other usecase */
    return newChNum;
}

int Demo_capturePrintScdFrameStatus()
{
    VCAP_CHN_DYNAMIC_PARAM_S    prm;
    int chId;
    int scdFrameStatus;


    Vcap_getDynamicParamChn(0, &prm, VCAP_SCDGETALLCHFRAMESTATUS);

    if(prm.scdAllChFrameStatus.numCh)
    {
        printf("\n");
        printf(" *** TAMPER DETECT STATUS ***\n");
        printf("\n");

        for(chId=0; chId<prm.scdAllChFrameStatus.numCh; chId++)
        {
            UInt32 chanNum;

            scdFrameStatus = prm.scdAllChFrameStatus.chanFrameResult[chId].frmResult;
            chanNum        = prm.scdAllChFrameStatus.chanFrameResult[chId].chId;

            if(scdFrameStatus==ALG_LINK_SCD_DETECTOR_CHANGE)
            {
                printf(" [TAMPER DETECTED] SCD CH <%d> CAP CH = %d \n", chanNum, Demo_captureGetCaptureChNum(chanNum));
            }
            else
            if(scdFrameStatus==ALG_LINK_SCD_DETECTOR_QUALIFYING)
            {
                printf(" [TAMPER QUALIFY DETECTED] SCD CH <%d> CAP CH = %d \n", chanNum, Demo_captureGetCaptureChNum(chanNum));
            }
            else
            if(scdFrameStatus==ALG_LINK_SCD_DETECTOR_NO_CHANGE)
            {
                printf(" [NO TAMPER] SCD CH <%d> CAP CH = %d \n", chanNum, Demo_captureGetCaptureChNum(chanNum));
            }
            else
            {
                printf(" [TAMPER STATUS UNAVAILABLE] SCD CH <%d> CAP CH = %d \n", chanNum, Demo_captureGetCaptureChNum(chanNum));
            }
        }
        printf("\n");
    }
    else
    {
        printf("\n");
        printf(" *** TAMPER DETECT NOT ENABLED on ANY channels ***\n");
        printf("\n");
    }
    return 0;
}

int Demo_captureGetTamperStatus(Ptr pPrm)
{
    AlgLink_ScdChStatus *pScdStat = (AlgLink_ScdChStatus *)pPrm;

    if(pScdStat->frmResult == ALG_LINK_SCD_DETECTOR_CHANGE)
    {
        printf(" [TAMPER DETECTED] %d: SCD CH <%d> CAP CH = %d \n", OSA_getCurTimeInMsec(), pScdStat->chId, Demo_captureGetCaptureChNum(pScdStat->chId));
    }

    return 0;
}

int Demo_captureGetMotionStatus(Ptr pPrm)
{
    AlgLink_ScdResult *pScdResult = (AlgLink_ScdResult *)pPrm;

    printf(" [MOTION DETECTED] %d: SCD CH <%d> CAP CH = %d \n", OSA_getCurTimeInMsec(), pScdResult->chId, Demo_captureGetCaptureChNum(pScdResult->chId));

    return 0;
}

VSYS_VIDEO_STANDARD_E Demo_captureGetSignalStandard()
{
        VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;
        VCAP_VIDEO_SOURCE_CH_STATUS_S *pChStatus;

        Vcap_getVideoSourceStatus( &videoSourceStatus );

        /* Check status of Channel 0 and use for all channels */
        pChStatus = &videoSourceStatus.chStatus[0];

        if(pChStatus->isVideoDetect)
        {
            if(pChStatus->frameHeight == 288)
            {
                return VSYS_STD_PAL;
            }
            if(pChStatus->frameHeight == 240)
            {
                return VSYS_STD_NTSC;
            }

        }

        return VSYS_STD_MAX;

}

int Demo_captureGetVideoSourceStatus()
{
    UInt32 chId;

    VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;
    VCAP_VIDEO_SOURCE_CH_STATUS_S *pChStatus;

    Vcap_getVideoSourceStatus( &videoSourceStatus );

    printf(" \n");

    for(chId=0; chId<videoSourceStatus.numChannels; chId++)
    {
        pChStatus = &videoSourceStatus.chStatus[chId];

        if(pChStatus->isVideoDetect)
        {
            printf (" DEMO: %2d: Detected video at CH [%d,%d] (%dx%d@%dHz, %d)!!!\n",
                     chId,
                     pChStatus->vipInstId, pChStatus->chId, pChStatus->frameWidth,
                     pChStatus->frameHeight,
                     1000000 / pChStatus->frameInterval,
                     pChStatus->isInterlaced);
        }
        else
        {
            printf (" DEMO: %2d: No video detected at CH [%d,%d] !!!\n",
                 chId, pChStatus->vipInstId, pChStatus->chId);

        }
    }

    printf(" \n");

    return 0;
}

int Demo_captureSettings(int demoId)
{
    Bool done = FALSE;
    char ch;
    int chId, value, winId, palMode = 0, idx;
    int value1 =0;
    VCAP_CHN_DYNAMIC_PARAM_S params;
    VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;

    if(gDemo_info.scdTileConfigInitFlag == FALSE)
    {
        for(chId = 0; chId < SCD_MAX_CHANNELS; chId++)
           for(idx = 0; idx < SCD_MAX_TILES; idx++)
              scdTileConfig[chId][idx] = 0;  /*disabling all the tiles */

        gDemo_info.scdTileConfigInitFlag = TRUE;
    }

    if(gDemo_info.maxVcapChannels<=0)
    {
        printf(" \n");
        printf(" WARNING: Capture NOT enabled, this menu is NOT valid !!!\n");
        return -1;
    }

    while(!done)
    {
        printf(gDemo_captureSettingsMenu);

        ch = Demo_getChar();

        switch(ch)
        {
            case '1':
                if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                {
                      printf(gDemo_captureHDDemoChanMenu);
                }
                chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);

                /* primary stream */
                Vcap_disableChn(chId, 0);

                /* secondary stream */
                Vcap_disableChn(chId, 1);

                if((demoId == DEMO_VCAP_VENC_VDIS) || \
                     (demoId == DEMO_VCAP_VENC_VDIS_HD) || \
                      (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH) || \
                      (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT))
                {
                    /* secondary stream in SD/HD encode demo*/
                    Vcap_disableChn(chId, 2);
                }
                if((demoId == DEMO_VCAP_VENC_VDIS_HD ) ||
                   (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT))
                {
                    /* MJPEG stream in HD encode demo*/
                    Vcap_disableChn(chId, 3);
                }

                /* disable channel on ALL displays as well */
                Demo_displayChnEnable(chId, FALSE);

                /* disable playback channel on display as well, since playback is sync with encode IN this demo */
                /* playback channel ID are after capture channel IDs */
                chId += gDemo_info.maxVcapChannels;

                Demo_displayChnEnable(chId, FALSE);
                break;

            case '2':
                if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                {
                      printf(gDemo_captureHDDemoChanMenu);
                }

                chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);

                /* primary stream */
                Vcap_enableChn(chId, 0);

                /* secondary stream */
                Vcap_enableChn(chId, 1);

                if((demoId == DEMO_VCAP_VENC_VDIS) || \
                     (demoId == DEMO_VCAP_VENC_VDIS_HD) || \
                      (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH) || \
                      (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT))
                {
                    /* secondary stream in SD/HD encode demo*/
                    Vcap_enableChn(chId, 2);
                }
                if((demoId == DEMO_VCAP_VENC_VDIS_HD ) ||
                   (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT))
                {
                    /* MJPEG stream in HD encode demo*/
                    Vcap_enableChn(chId, 3);
                }

                /* enable channel on ALL displays as well */
                Demo_displayChnEnable(chId, TRUE);

                /* enable playback channel on display as well, since playback is sync with encode IN this demo */
                /* playback channel ID are after capture channel IDs */
                chId += gDemo_info.maxVcapChannels;

                Demo_displayChnEnable(chId, TRUE);
                break;


            case '3':
                if(gDemo_info.osdEnable)
                {
                    if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                    {
                          printf(gDemo_captureHDDemoChanMenu);
                    }
                    if(demoId != DEMO_VCAP_VENC_VDIS_HD)
                        chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                    else
                        chId = Demo_getChId("CAPTURE",2 *  gDemo_info.maxVcapChannels);

                    winId = Demo_getIntValue("OSD window number to be updated", 0, g_osdChParam[chId].numWindows - 1, 0);

                    value = Demo_getIntValue("X co-ordinate for 1st OSD Window. Ensure window width W + start pos X  < total width of frame",  0, OSA_floor((720 - g_osdChParam[chId].winPrm[winId].width),32), DEMO_OSD_WIN0_STARTX);

                    value1 = Demo_getIntValue("Y co-ordinate for 1st OSD Window. Ensure window height H + start pos Y  < total height of frame", 0, (480 - g_osdChParam[chId].winPrm[winId].height), DEMO_OSD_WIN0_STARTY);


                    /* max value check below need to be fixed */
                    g_osdChParam[chId].winPrm[winId].startX = value;
                    g_osdChParam[chId].winPrm[winId].startY = value1; //48 lines difference between two windows

                    params.osdChWinPrm = &g_osdChParam[chId];
                    Vcap_setDynamicParamChn(chId, &params, VCAP_OSDWINPRM);
                }
                else
                {
                    printf(" This Option is not Valid for this demo\n");
                }
                break;
#if 0
            case '4':
                if(gDemo_info.osdEnable)
                {
                    if(demoId != DEMO_VCAP_VENC_VDIS_HD)
                    {
                        chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                    }
                    else
                    {
                        chId = Demo_getChId("CAPTURE",2 *  gDemo_info.maxVcapChannels);
                    }
                    winId = Demo_getIntValue("OSD window number to be updated", 0, g_osdChParam[chId].numWindows - 1, 0);

                    /* max value check below need to be fixed below based upon capture max width and height*/
                    value = Demo_getIntValue("OSD window width", 1, 720 - g_osdChParam[chId].winPrm[winId].startX, DEMO_OSD_WIN_WIDTH);
                    value1 = Demo_getIntValue("OSD window height", 1, 480 - g_osdChParam[chId].winPrm[winId].startY, DEMO_OSD_WIN_HEIGHT);
                    value2 = Demo_getIntValue("OSD window stride", 0, DEMO_OSD_WIN_PITCH_H, DEMO_OSD_WIN_PITCH_H);

                    g_osdChParam[chId].winPrm[winId].width = value;
                    g_osdChParam[chId].winPrm[winId].height = value1;
                    g_osdChParam[chId].winPrm[winId].lineOffset = value2;

                    params.osdChWinPrm = &g_osdChParam[chId];
                    Vcap_setDynamicParamChn(chId, &params, VCAP_OSDWINPRM);
                }
                else
                {
                    printf(" This Option is not Valid for this demo\n");
                }

                break;
#else
            case '4':
                printf(" This Option is not valid for this demo. \n Please refer to the source code to get the usage of VCAP_OSDWINPRM \n");
                break;
#endif
            case '5':
                if(gDemo_info.osdEnable)
                {
                    if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                    {
                          printf(gDemo_captureHDDemoChanMenu);
                    }

                    if(demoId != DEMO_VCAP_VENC_VDIS_HD)
                        chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                    else
                        chId = Demo_getChId("CAPTURE",2 *  gDemo_info.maxVcapChannels);

                    memset(&params, 0, sizeof(params));

                    winId = Demo_getIntValue("OSD window number to be updated", 0, g_osdChParam[chId].numWindows - 1, 0);

                    value = Demo_getIntValue("OSD window transparency ON/OFF", 0, 1, 1);

                    g_osdChParam[chId].winPrm[winId].transperencyEnable = value;
                    params.osdChWinPrm = &g_osdChParam[chId];
                    Vcap_setDynamicParamChn(chId, &params, VCAP_OSDWINPRM);
                }
                else
                {
                    printf(" This Option is not Valid for this demo\n");
                }

                break;

            case '6':
                if(gDemo_info.osdEnable)
                {
                    if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                    {
                          printf(gDemo_captureHDDemoChanMenu);
                    }

	                if(demoId != DEMO_VCAP_VENC_VDIS_HD)
	                    chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
	                else
	                    chId = Demo_getChId("CAPTURE",2 *  gDemo_info.maxVcapChannels);

                    memset(&params, 0, sizeof(params));
                    winId = Demo_getIntValue("OSD window number to be updated", 0, g_osdChParam[chId].numWindows - 1, 0);
                    value = Demo_getIntValue("OSD window global alpha (U1Q7 format 0x00 - 0% 0x80 - 100%)", 0, 128, 1);

                    /* max value check below need to be fixed below based upon capture max width and height*/
                    g_osdChParam[chId].winPrm[winId].globalAlpha = value/(winId+1);

                    params.osdChWinPrm = &g_osdChParam[chId];
                    Vcap_setDynamicParamChn(chId, &params, VCAP_OSDWINPRM);
                }
                else
                {
                    printf(" This Option is not Valid for this demo\n");
                }

                break;

            case '7':
                if(gDemo_info.osdEnable)
                {
                    if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                    {
                          printf(gDemo_captureHDDemoChanMenu);
                    }

                    if(demoId != DEMO_VCAP_VENC_VDIS_HD)
                        chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                    else
                        chId = Demo_getChId("CAPTURE",2 *  gDemo_info.maxVcapChannels);

                    memset(&params, 0, sizeof(params));
                    winId = Demo_getIntValue("OSD window number to be updated", 0, g_osdChParam[chId].numWindows - 1, 0);
                    value = Demo_getIntValue("OSD window disable/enable", 0, 1, 1);

                    /* max value check below need to be fixed below based upon capture max width and height*/
                    g_osdChParam[chId].winPrm[winId].enableWin = value;

                    params.osdChWinPrm = &g_osdChParam[chId];
                    Vcap_setDynamicParamChn(chId, &params, VCAP_OSDWINPRM);
                }
                else
                {
                    printf(" This Option is not Valid for this demo\n");
                }

                break;

            case '8':
                Demo_captureGetVideoSourceStatus();
                break;

            case '9':

                if(demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED)
                {
                    printf("******NOTE: This is not supported by this interlace demo now.\n");
                    break;
                }
                memset(&params, 0, sizeof(params));
                if (demoId == DEMO_VCAP_VENC_VDIS)
                {
                    value = Demo_getIntValue("Select datapath (0:preview, 1:primary stream, 2:secondary stream)", 0, 2, 0);
                }
                else if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                   value = Demo_getIntValue("Select datapath (1: Primary stream, 2: Secondary stream, 3: MJPEG stream, 4: A8 Frame Out stream)", 1, 4, 1);
                else
                   value = Demo_getIntValue("Select datapath (1:primary stream, 2:secondary stream)", 1, 2, 1);

                params.chDynamicRes.pathId = (VCAP_PATH_E)value;
                if(demoId == DEMO_VCAP_VENC_VDIS_HD)
                {
                      printf(gDemo_captureHDDemoChanMenu);
                }

                switch (value)
                {
                    case 0:
                        chId = Demo_getChId("PREVIEW", gDemo_info.maxVcapChannels);
                        value = Demo_getIntValue("Select Resolution (0:D1, 1:VGA, 2:Half-D1, 3:CIF, 4:QCIF)", 0, 4, 0);
                        break;
                    case 1:
                        chId = Demo_getChId("PRIMARY STREAM", gDemo_info.maxVcapChannels);
                        value = Demo_getIntValue("Select Resolution (0:D1, 1:VGA, 2:Half-D1, 3:CIF, 4:QCIF)", 0, 4, 0);
                        break;
                    case 2:
                        if(demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)
                        {
                            printf("===================================================\n");
                            printf("NOTE: Secondary stream resolution change is not supported on TI816X 16 CH Progressive App \n");
                            printf("It will result in live preview quality degradation for smaller resolution windows, if still user tries changing resolution \n");
                            printf("===================================================\n");
                        }
                        chId = Demo_getChId("SECONDARY STREAM", gDemo_info.maxVcapChannels);
                        value = Demo_getIntValue("Select Resolution (0:CIF, 1:QCIF)", 0, 1, 0);
                        value = value + 3;
                        break;
                    case 3:
                        chId = Demo_getChId("MJPEG stream", gDemo_info.maxVcapChannels);
                        value = Demo_getIntValue("Select Resolution (0:D1, 1:VGA, 2:Half-D1, 3:CIF, 4:QCIF)", 0, 4, 0);
                        break;
                    case 4:
                        chId = Demo_getChId("A8 Frame Out stream", gDemo_info.maxVcapChannels);
                        value = Demo_getIntValue("Select Resolution (0:D1, 1:VGA, 2:Half-D1, 3:CIF, 4:QCIF)", 0, 4, 0);
                        break;
                    default:
                        printf("NOT SUPPORTED OPTION, USE DEFAULT OPTION!\n");
                        chId = Demo_getChId("PRIMARY STREAM", gDemo_info.maxVcapChannels);
                        value = Demo_getIntValue("Select Resolution (0:D1, 1:VGA, 2:Half-D1, 3:CIF, 4:QCIF)", 0, 4, 0);
                        break;
                }


                if (Vcap_getVideoSourceStatus( &videoSourceStatus ) == ERROR_NONE)
                {
                    // HACK - Assuming ch 0
                    if (videoSourceStatus.chStatus[0].frameHeight == 288)
                    {
                        palMode = 1;
                    }
                    else
                    {
                        palMode = 0;
                    }
                }

                switch (value)
                {
                    case 0: /* D1 */
                        params.chDynamicRes.width = 704;
                        if (palMode)
                            params.chDynamicRes.height = 576;
                        else
                            params.chDynamicRes.height = 480;
                        break;
                    case 1: /* VGA */
                        params.chDynamicRes.width = 640;
                        if (palMode)
                            params.chDynamicRes.height = 480;
                        else
                            params.chDynamicRes.height = 480;
                        break;
                    case 2: /* Half-D1 */
                        params.chDynamicRes.width = 704;
                        if (palMode)
                            params.chDynamicRes.height = 288;
                        else
                            params.chDynamicRes.height = 240;
                        break;
                    case 3: /* CIF */
                        params.chDynamicRes.width = 352;
                        if (palMode)
                            params.chDynamicRes.height = 288;
                        else
                            params.chDynamicRes.height = 240;
                        break;
                    case 4: /* QCIF */
                        params.chDynamicRes.width = 176;
                        if (palMode)
                            params.chDynamicRes.height = 144;
                        else
                            params.chDynamicRes.height = 120;
                        break;
                    default: /* CIF */
                        params.chDynamicRes.width = 352;
                        if (palMode)
                            params.chDynamicRes.height = 288;
                        else
                            params.chDynamicRes.height = 240;
                        break;
                }
                printf ("Capture channel is %s, new resolution for CH %d is %d x %d\n", palMode == 1 ? "PAL" : "NTSC",
                                chId, params.chDynamicRes.width, params.chDynamicRes.height);
                Vcap_setDynamicParamChn(chId, &params, VCAP_RESOLUTION);
                break;


            case 'a':
                if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) || (demoId == DEMO_VCAP_VENC_VDIS_HD))
                {
                    printf("NOTE: This is not supported by this demo now.\n");
                    break;
                }

                chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                chId = Demo_captureGetScdChNum (chId);
                memset(&params, 0, sizeof(params));

                params.scdChPrm.chId = chId;
                params.scdChPrm.mode = (UInt32) ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME;

                printf("SCD modes:\n");
                printf("0 - Disable all,             1 - Enable Tamper detection\n");
                printf("2 - Enable Motion detection, 3 - Enable both Tamper and Motion detection\n\n");
                value = Demo_getIntValue("SCD mode selection", 0, 3, 0);

                params.scdChPrm.mode = value;

                Vcap_setDynamicParamChn(chId, &params, VCAP_SCDMODE);
                break;

            case 'b':
                if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) || (demoId == DEMO_VCAP_VENC_VDIS_HD))
                {
                    printf("NOTE: This is not supported by this demo now.\n");
                    break;
                }

                chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                chId = Demo_captureGetScdChNum (chId);

                memset(&params, 0, sizeof(params));

                params.scdChPrm.chId = chId;
#if defined(TI_814X_BUILD)|| defined(TI_816X_BUILD)
                printf("\n For Tamper detection  valid range of sensitivity is from 1 - 5 \n");
                printf(" The sensitivity value is clipped accordingly to this range.\n\n");
                printf(" For motion detection (LMD), sensitivity range is from 0 - 8. \n");
#endif
                value = Demo_getIntValue("SCD sensitivity", 0, 8, 8);

                params.scdChPrm.frmSensitivity = (UInt32) value;

                Vcap_setDynamicParamChn(chId, &params, VCAP_SCDSENSITIVITY);
                break;
#if defined(TI_8107_BUILD)
            case 'c':
                if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) || (demoId == DEMO_VCAP_VENC_VDIS_HD))
                {
                    printf("NOTE: This is not supported by this demo now.\n");
                    break;
                }
                chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                chId = Demo_captureGetScdChNum (chId);

                memset(&params, 0, sizeof(params));

                params.scdChPrm.chId = chId;

                /* max value check below need to be fixed below based upon capture max width and height*/
                value = Demo_getIntValue("SCD IGNORE LIGHT OFF FLAG", 0, 1, 0);

                params.scdChPrm.frmIgnoreLightsOFF = (UInt32) value;

                Vcap_setDynamicParamChn(chId, &params, VCAP_IGNORELIGHTSOFF);
                break;

            case 'd':
                if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) || (demoId == DEMO_VCAP_VENC_VDIS_HD))
                {
                    printf("NOTE: This is not supported by this demo now.\n");
                    break;
                }
                chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);
                chId = Demo_captureGetScdChNum (chId);

                memset(&params, 0, sizeof(params));

                params.scdChPrm.chId = chId;

                /* max value check below need to be fixed below based upon capture max width and height*/
                value = Demo_getIntValue("SCD IGNORE LIGHT ON FLAG", 0, 1, 0);

                params.scdChPrm.frmIgnoreLightsON = (UInt32) value;

                Vcap_setDynamicParamChn(chId, &params, VCAP_IGNORELIGHTSON);
                break;
#endif
            case 'e':
                if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) || (demoId == DEMO_VCAP_VENC_VDIS_HD))
                {
                    printf("NOTE: This is not supported by this demo now.\n");
                    break;
                }

                printf("\n In the current demo, frame is divided in to a set of 3x3 tiles (Max 9-tiles).\n");
                printf("\n User can select any tile starting from no 0 - 8 (topLeft - bottomRight)\n");
                printf("\n Once the tile is selected, user can update block config of all the blocks within that tile\n");
                chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);

                memset(&params, 0, sizeof(params));
                params.scdChPrm.chId = Demo_captureGetScdChNum (chId);

                {
                     UInt32 tileId,startX, startY, endX, endY;
                     UInt32 maxHorBlks, maxVerBlks, maxHorBlkPerTile, maxVerBlkPerTile;
                     Int32 i, j, sensitivity, flag = 0;
                     if(demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH
                        || demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT
                        || demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1)
                     {
                         maxHorBlks = 6;  /* QCIF Resolution rounded to 32 pixels */
                         maxVerBlks = 12; /* QCIF Resolution*/
                         maxHorBlkPerTile   = 2;
                         maxVerBlkPerTile   = 4;
                     }
                     else
                     {
                         maxHorBlks = 11;  /* CIF Resolution rounded to 32 pixels */
                         maxVerBlks = 24;  /* CIF Resolution*/
                         maxHorBlkPerTile   = 4;
                         maxVerBlkPerTile   = 8;
                     }


                     printf("\n Enabled Tile Nos:\t");
                     for(i = 0; i < SCD_MAX_TILES; i++)
                     {
                        if(scdTileConfig[chId][i] == 1)
                        {
                           flag = 1;
                           printf("%d, ",i);
                        }
                     }
                     if(flag == 0)
                        printf("Currently zero tiles are enabled for LMD for Chan-%d\n",chId);

                     printf("\n");
                     tileId = Demo_getIntValue(" Tile Id/No.", 0, 8, 0);

                     startX = (tileId%3)*maxHorBlkPerTile;
                     startY = (tileId/3)*maxVerBlkPerTile;

                     endX = startX + maxHorBlkPerTile;
                     endY = startY + maxVerBlkPerTile;

                     if(endX > maxHorBlks)
                        endX = maxHorBlks;
                     if(endY > maxVerBlks)
                        endY = maxVerBlks;

                     params.scdChBlkPrm.chId = Demo_captureGetScdChNum (chId);
                     params.scdChBlkPrm.numValidBlock = 0;

                    flag        = Demo_getIntValue("LMD block disable/enable flag", 0, 1, 1);

                    if(flag == 1)
                    {
                       sensitivity = Demo_getIntValue("LMD block sensitivity", 0, 8, 8);
                       scdTileConfig[chId][tileId] = 1;
                    }
                    else
                    {
                       scdTileConfig[chId][tileId] = 0;
                       sensitivity = 0;
                    }


                     for(i = startY; i < endY; i++)
                     {
                         for(j = startX; j < endX; j++)
                         {
                             AlgLink_ScdChBlkConfig * blkConfig;
                             blkConfig = &params.scdChBlkPrm.blkConfig[params.scdChBlkPrm.numValidBlock];

                             blkConfig->blockId      =  j + (i * maxHorBlks)   ;
                             blkConfig->sensitivity  = sensitivity;
                             blkConfig->monitorBlock = flag;
                             params.scdChBlkPrm.numValidBlock++;
                         }
                     }
                     printf("\nNumber of blocks in the tile to be updated %d\n\n",params.scdChBlkPrm.numValidBlock);
                }
                Vcap_setDynamicParamChn(chId, &params, VCAP_SCDBLOCKCONFIG);
                break;
#if defined(TI_814X_BUILD)|| defined(TI_816X_BUILD)
            case 'f':
                if((demoId == DEMO_VCAP_VENC_VDIS) ||
                    (demoId == DEMO_VCAP_VENC_VDIS_HD) ||
                     (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE) ||
                      (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH) ||
                      (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1) ||
                       (demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT))
                {

                    VCAP_CHN_DYNAMIC_PARAM_S params = { 0 };
                    UInt32 winId, enableWin;
                    CaptureLink_BlindInfo * blindInfo;

                    printf("\n User is required to input chanId and MaskWindowId. \n");

                    printf("\n Windows Co-ordinates, Color and Size are default\n");
                    printf("\n Please refer demo_blindArea.c file for default values\n\n");

                    chId = Demo_getChId("CAPTURE", gDemo_info.maxVcapChannels);

                    blindInfo = &g_blindAreaChParam[chId];

                    winId = Demo_getIntValue("Mask window number to be updated:", 0, g_blindAreaChParam[chId].numBlindArea-1, 0);

/* Only enable and disable inputs are taken. Others are default values. Enable else part for detailed UI input*/
#if 1
                    enableWin = Demo_getIntValue("Mask window disable or enable", 0, 1, 0);
                    blindInfo->win[winId].enableWin = enableWin;
#else
                    /* In the current demo UI option input is assume to be 704x240.
                     * User can change it in demo code if needed.
                     * From the default pixel coordinate,  maskWindows of size 50x20 is masked/unMasked
                     */

                    if(g_blindAreaChParam[chId].win[winId].enableWin == TRUE)
                        enableWin = Demo_getIntValue("Mask window already enabled. Now 0-disable or 1-Update Coordinate", 0, 1, 0);
                    else
                        enableWin = Demo_getIntValue("Mask window disable or enable", 0, 1, 0);

                    blindInfo->win[winId].enableWin = enableWin;
                    if(enableWin == TRUE)
                    {
                        UInt32 startX,startY;
                        blindInfo->win[winId].fillColorYUYV= 0x80108010;
                        blindInfo->win[winId].width  = 50;
                        blindInfo->win[winId].height = 20;

                        startX = Demo_getIntValue("Mask window StartX", 0, 704, 0);;
                        startY = Demo_getIntValue("Mask window StartY", 0, 240, 0);;

                        if((startX + blindInfo->win[winId].width)  > 704)
                           startX = 644;
                        if((startY + blindInfo->win[winId].height)  > 240)
                           startY = 210;

                        blindInfo->win[winId].startX = startX;
                        blindInfo->win[winId].startY = startY;

                    }
#endif
                    blindInfo = &params.captureBlindInfo;
                    memcpy(blindInfo,&g_blindAreaChParam[chId],sizeof(CaptureLink_BlindInfo));
                    Vcap_setDynamicParamChn(chId, &params, VCAP_BLINDAREACONFIG);
                }
                else
                {
                    printf(" This Option is not Valid for this demo\n");
                }
                break;
#endif
            case 'g':
                Demo_capturePrintScdFrameStatus();
                break;

            case 'h':
                {
                    Bool enableTrack;
                    if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) || (demoId == DEMO_VCAP_VENC_VDIS_HD)|| (demoId == DEMO_VCAP_VENC_VDIS) )
                    {
                        printf("NOTE: This is not supported by this demo now.\n");
                        break;
                    }
                    printf("\n Enable SCD Block Motion Tracking On Display. (Only On ON-CHIP HDMI)\n\n");
                    enableTrack = Demo_getIntValue("Motion Tracking on Display disable or enable", 0, 1, 0);

                    if(enableTrack)
                    {

                        printf("\n Enter Channel No. to be Tracked\n");
                        chId = Demo_getChId("TRACK", gDemo_info.maxVcapChannels);
                        Scd_trackCh(chId);
                        Scd_enableMotionTracking();
                    }
                    else
                        Scd_disableMotionTracking();

                 }
                 break;
             case 'i':
                if((demoId == DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED) || (demoId == DEMO_VCAP_VENC_VDIS_HD) || (demoId == DEMO_VCAP_VENC_VDIS) )
                {
                    printf("NOTE: This is not supported by this demo now.\n");
                    break;
                }

                printf("\n Enter Channel No. which user wants to track\n");
                chId = Demo_getChId("TRACK", gDemo_info.maxVcapChannels);
                Scd_trackCh(chId); 
                break;

            case 'p':
                done = TRUE;
                break;
        }
    }

    return 0;
}


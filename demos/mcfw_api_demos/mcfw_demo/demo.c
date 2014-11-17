
#include <demo.h>
#include <demo_spl_usecases.h>
#include <demo_crash_dump_analyzer_format.h>

//#define DEMO_ENABLE_SPL_USECASES             (TRUE)

char gDemo_mainMenu[] = {
    "\r\n ========="
    "\r\n Main Menu"
    "\r\n ========="
    "\r\n"
#if defined (TI_814X_BUILD) || defined (TI_8107_BUILD)
    "\r\n 1: 4CH VCAP + VENC + VDEC + VDIS  - Progressive SD Encode + Decode"
    "\r\n 2: 8CH <D1+CIF> VCAP + VENC + VDEC + VDIS  - Progressive SD Encode + Decode"
    "\r\n 3: 16CH <D1+CIF> VCAP + VENC + VDEC + VDIS  - Progressive SD Encode + Decode"
    "\r\n 4: 16CH NRT <D1+CIF> VCAP + VENC + VDEC + VDIS  - Progressive SD Encode + Decode"
    "\r\n 5:               VDEC + VDIS  - SD/HD Decode ONLY"
    "\r\n 6: CUSTOM DEMO - 1Ch D1 Decode"
    "\n"
#ifdef DDR_MEM_256M
    "\r\n In the current 256MB DDR build, only option 1 is supported"
#endif
    "\r\n c: 4CH Car DVR usecase"
#else
    "\r\n 1: VCAP + VENC + VDEC + VDIS  - Progressive SD Encode + Decode"
    "\r\n 2: VCAP + VENC        + VDIS  - SD Encode ONLY"
    "\r\n 3: VCAP + VENC        + VDIS  - HD Encode ONLY"
    "\r\n 4:               VDEC + VDIS  - SD/HD Decode ONLY"
    "\r\n 5: VCAP               + VDIS  - NO Encode or Decode"
#ifdef DEMO_ENABLE_SPL_USECASES
    "\r\n a: 16CH Hybrid DVR usecase"
    "\r\n b: 36CH Hybrid enc usecase"
    "\r\n c: 4CH Car DVR usecase"
#endif

#endif
    "\r\n"
    "\r\n e: Exit"
    "\r\n"
    "\r\n Enter Choice: "
};

char gDemo_runMenu[] = {
    "\r\n ============="
    "\r\n Run-Time Menu"
    "\r\n ============="
    "\r\n"
    "\r\n 1: Capture Settings"
    "\r\n 2: Encode  Settings"
    "\r\n 3: Decode  Settings"
    "\r\n 4: Display Settings"
    "\r\n 5: Audio   Capture / Playback Settings"
#if  !defined(TI_8107_BUILD)
    "\r\n"
    "\r\n 6: Audio Capture <TVP5158> & Encode <AAC-LC> demo"
    "\r\n 7: Audio encode demo <AAC-LC File In/Out>"
    "\r\n 8: Audio decode demo <AAC-LC File In/Out>"
#endif
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
    "\r\n"
    "\r\n c: Change 8CH modes (8CH usecase ONLY!!!!)"
    "\r\n d: Change 16CH modes (16CH usecase ONLY!!!!)"
#endif
    "\r\n"
    "\r\n i: Print detailed system information"
    "\r\n s: Core Status: Active/In-active"
//    "\r\n b: Print link buffers statistics"
#if  defined(TI_816X_BUILD)
    "\r\n f: Switch IVA Channel Map"
#endif
    "\r\n"
    "\r\n e: Stop Demo"
    "\r\n"
    "\r\n Enter Choice: "
};

Demo_Info gDemo_info = 
{
    .curDisplaySeqId = VDIS_DISPLAY_SEQID_DEFAULT,
};

int main()
{
    Bool done = FALSE;
    char ch;

    gDemo_info.audioType = DEMO_AUDIO_TYPE_NONE;
    gDemo_info.audioInitialized = FALSE;

    while(!done)
    {
        printf(gDemo_mainMenu);

        ch = Demo_getChar();

        switch(ch)
        {
#if defined (TI_814X_BUILD) || defined (TI_8107_BUILD)
            case '1':
                Demo_run(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE);
                break;
            case '2':
                Demo_run(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH);
                break;
            case '3':
                Demo_run(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1);
                break;
            case '4':
                Demo_run(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT);
                break;
            case '5':
                Demo_run(DEMO_VDEC_VDIS);
                break;
            case '6':
                Demo_run(DEMO_CUSTOM_2);
                break;
            case 'c':
                Demo_run(DEMO_CARDVR_4CH);
                break;
#else
            case '1':
                Demo_run(DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE);
                break;
            case '2':
                Demo_run(DEMO_VCAP_VENC_VDIS);
                break;
            case '3':
                Demo_run(DEMO_VCAP_VENC_VDIS_HD);
                break;
            case '4':
                Demo_run(DEMO_VDEC_VDIS);
                break;
            case '5':
                Demo_run(DEMO_VCAP_VDIS);
                break;
            case 'a':
                Demo_run(DEMO_HYBRIDDVR_16CH);
                break;
            case 'b':
                Demo_run(DEMO_HYBRIDENC_36CH);
                break;
            case 'c':
                Demo_run(DEMO_CARDVR_4CH);
                break;
#endif
            case 'e':
                done = TRUE;
                break;
        }
    }

    if (gDemo_info.audioInitialized == TRUE)
    {
#if !defined(TI_8107_BUILD)
        Demo_deInitAudioSystem();   /* should be called once in application at the end */
#endif
        gDemo_info.audioInitialized = FALSE;
    }

    return 0;
}

static
Void  Demo_printSlaveCoreExceptionContext(VSYS_SLAVE_CORE_EXCEPTION_INFO_S *excInfo)
{
    FILE *fpCcsCrashDump;
    char crashDumpFileName[100];

    printf("\n\n%d:!!!SLAVE CORE DOWN!!!.EXCEPTION INFO DUMP\n",OSA_getCurTimeInMsec());
    printf("\n !!HW EXCEPTION ACTIVE (0/1): [%d]\n",excInfo->exceptionActive);
    printf("\n !!EXCEPTION CORE NAME      : [%s]\n",excInfo->excCoreName);
    printf("\n !!EXCEPTION TASK NAME      : [%s]\n",excInfo->excTaskName);
    printf("\n !!EXCEPTION LOCATION       : [%s]\n",excInfo->excSiteInfo);
    printf("\n !!EXCEPTION INFO           : [%s]\n",excInfo->excInfo);
    snprintf(crashDumpFileName, sizeof(crashDumpFileName),"CCS_CRASH_DUMP_%s.txt",excInfo->excCoreName);
    fpCcsCrashDump = fopen(crashDumpFileName,"w");
    if (fpCcsCrashDump)
    {
        Demo_CCSCrashDumpFormatSave(excInfo,fpCcsCrashDump);
        fclose(fpCcsCrashDump);
        printf("\n !!EXCEPTION CCS CRASH DUMP FORMAT FILE STORED @ ./%s\n",crashDumpFileName);
    }
}

Int32 Demo_eventHandler(UInt32 eventId, Ptr pPrm, Ptr appData)
{
    if(eventId==VSYS_EVENT_VIDEO_DETECT)
    {
        printf(" \n");
        printf(" DEMO: Received event VSYS_EVENT_VIDEO_DETECT [0x%04x]\n", eventId);

        Demo_captureGetVideoSourceStatus();
    }

    if(eventId==VSYS_EVENT_TAMPER_DETECT)
    {
        Demo_captureGetTamperStatus(pPrm);
    }

    if(eventId==VSYS_EVENT_MOTION_DETECT)
    {
        Demo_captureGetMotionStatus(pPrm);
    }

    if(eventId== VSYS_EVENT_DECODER_ERROR)
    {
        Demo_decodeErrorStatus(pPrm);
    }

    if (eventId == VSYS_EVENT_SLAVE_CORE_EXCEPTION)
    {
        Demo_printSlaveCoreExceptionContext(pPrm);
    }

    return 0;
}

int Demo_run(int demoId)
{
    int status;
    Bool done = FALSE;
    char ch;

    gDemo_info.scdTileConfigInitFlag = FALSE;
    status = Demo_startStop(demoId, TRUE);
    if(status<0)
    {
        printf(" WARNING: This demo is NOT curently supported !!!\n");
        return status;
    }

    while(!done)
    {
        printf(gDemo_runMenu);

        ch = Demo_getChar();

        switch(ch)
        {
            case '1':
                Demo_captureSettings(demoId);
                break;
            case '2':
                Demo_encodeSettings(demoId);
                break;
            case '3':
                Demo_decodeSettings(demoId);
                break;
            case '4':
                Demo_displaySettings(demoId);
                break;
            #ifndef SYSTEM_DISABLE_AUDIO
            case '5':
                Demo_audioSettings(demoId);
                break;
            #endif
            case '6':
                Demo_startStopAudioEncodeDecode(demoId, DEMO_AUDIO_TYPE_CAPTURE, TRUE);
                break;
            case '7':
                Demo_startStopAudioEncodeDecode(demoId, DEMO_AUDIO_TYPE_ENCODE, TRUE);
                break;
            case '8':
                Demo_startStopAudioEncodeDecode(demoId, DEMO_AUDIO_TYPE_DECODE, TRUE);
                break;
            #if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
            case 'c':
                demoId = Demo_change8ChMode(demoId);
                break;
            case 'd':
                Demo_change16ChMode(demoId);
                break;
            #endif
            case 'i':
                Demo_printInfo(demoId);
                break;
            case 's':
            {
                VSYS_CORE_STATUS_TBL_S coreStatusTbl;
                Int i;

                Vsys_getCoreStatus(&coreStatusTbl);
                for (i = 0; i < coreStatusTbl.numSlaveCores; i++)
                {
                    if (FALSE == coreStatusTbl.coreStatus[i].isAlive)
                    {
                        printf("\n\n!!!!CORE ID [%d] DOWN!!!",i);
                        printf("\n\n!!!!CORE ID [%d] DOWN!!!",i);
                    }
                }
                break;
            }
            case 'b':
                Demo_printBuffersInfo();
                break;
            case 'a':
                Demo_printAvsyncInfo();
                break;
            #if defined(TI_816X_BUILD)
            case 'f':
                Demo_switchIVAMap();
                break;
            #endif
            case 'e':
                done = TRUE;
                break;
        }
    }

    Demo_startStop(demoId, FALSE);

    return 0;
}

int Demo_startStop(int demoId, Bool startDemo)
{
    if (gDemo_info.audioInitialized == TRUE)
    {
        /* Stop audio - Doesnt seem to work reliably */
        Demo_startStopAudioEncodeDecode(demoId, gDemo_info.audioType, FALSE);
    }

    if(startDemo)
    {
        gDemo_info.maxVcapChannels = 0;
        gDemo_info.maxVdisChannels = 0;
        gDemo_info.maxVencChannels = 0;
        gDemo_info.maxVdecChannels = 0;

        gDemo_info.audioEnable = FALSE;
        gDemo_info.isAudioPathSet = FALSE;
        gDemo_info.audioCaptureActive = FALSE;
        gDemo_info.audioPlaybackActive = FALSE;
        gDemo_info.audioPlaybackChNum = 0;
        gDemo_info.audioCaptureChNum = 0;
        gDemo_info.osdEnable = FALSE;
        gDemo_info.curDisplaySeqId = VDIS_DISPLAY_SEQID_DEFAULT;
    }

    switch(demoId)
    {
#ifdef TI_816X_BUILD
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE:
            if(startDemo)
            {
        #ifndef SYSTEM_DISABLE_AUDIO
                gDemo_info.audioEnable = TRUE;
        #endif
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVencVdecVdis_start(TRUE, TRUE, demoId);
//                sleep(3);
//                Demo_startStopAudioEncodeDecode(demoId, DEMO_AUDIO_TYPE_CAPTURE, TRUE);
            }
            else
            {
                VcapVencVdecVdis_stop();
            }
            break;
        case DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED:
            if(startDemo)
            {
        #ifndef SYSTEM_DISABLE_AUDIO
                gDemo_info.audioEnable = TRUE;
        #endif
                gDemo_info.Type = DEMO_TYPE_INTERLACED;
                VcapVencVdecVdis_start(FALSE, TRUE, demoId);
            }
            else
            {
                VcapVencVdecVdis_stop();
            }
            break;
        case DEMO_VCAP_VENC_VDIS:
            if(startDemo)
            {
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVenc_start(FALSE);
            }
            else
            {
                VcapVenc_stop();
            }
            break;
        case DEMO_VCAP_VENC_VDIS_HD:
            if(startDemo)
            {
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVenc_start(TRUE);
            }
            else
            {
                 VcapVenc_stop();
            }
            break;
        case DEMO_VCAP_VDIS:
            if(startDemo)
            {
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVdis_start();
            }
            else
            {
                VcapVdis_stop();
            }
            break;

        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_4CH:
            if(startDemo)
            {
    #ifndef SYSTEM_DISABLE_AUDIO
                gDemo_info.audioEnable = TRUE;
    #endif
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVencVdecVdis_start(TRUE, TRUE, demoId);
            }
            else
            {
                VcapVencVdecVdis_stop();
            }
            break;
#else   /* TI_814X_BUILD */
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE:
            if(startDemo)
            {
#ifndef SYSTEM_DISABLE_AUDIO
                gDemo_info.audioEnable = TRUE;
#endif
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVencVdecVdis_start(TRUE, TRUE, demoId);
            }
            else
            {
                VcapVencVdecVdis_stop();
            }
            break;
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH:
            if(startDemo)
            {
                #ifndef SYSTEM_DISABLE_AUDIO
                gDemo_info.audioEnable = TRUE;
                #endif
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVencVdecVdis_start(TRUE, TRUE, demoId);
            }
            else
            {
                VcapVencVdecVdis_stop();
            }
            break;
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1:
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT:
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT:
            if(startDemo)
            {
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VcapVencVdecVdis_start(TRUE, TRUE, demoId);
            }
            else
            {
                VcapVencVdecVdis_stop();
            }
            break;
#endif
        case DEMO_VDEC_VDIS:
            if(startDemo)
            {
                gDemo_info.Type = DEMO_TYPE_PROGRESSIVE;
                VdecVdis_start();
            }
            else
            {
                VdecVdis_stop();
            }
            break;

        case DEMO_CUSTOM_0:
            if(startDemo)
                VcapVencVdecVdisCustom_start(FALSE, TRUE, 1);
            else
                VcapVencVdecVdisCustom_stop();

            break;
        case DEMO_CUSTOM_1:
            if(startDemo)
                VcapVencVdecVdisCustom_start(FALSE, TRUE, 2);
            else
                VcapVencVdecVdisCustom_stop();

            break;
        case DEMO_CUSTOM_2:
            if(startDemo)
                VcapVencVdecVdisCustom_start(TRUE, FALSE, 2);
            else
                VcapVencVdecVdisCustom_stop();

            break;
        case DEMO_HYBRIDDVR_16CH:
        case DEMO_CARDVR_4CH:
        case DEMO_HYBRIDENC_36CH:
            if(startDemo)
                VcapVencVdecVdisSplUsecase_start(demoId);
            else
                VcapVencVdecVdisSplUsecase_stop();

            break;
        default:
            return -1;
    }

    if(startDemo)
    {
        Vsys_registerEventHandler(Demo_eventHandler, NULL);
    }

    return 0;
}

int Demo_printBuffersInfo()
{
    Vsys_printBufferStatistics();
    return 0;
}


int Demo_printAvsyncInfo()
{
    Vdis_printAvsyncStatistics();
    return 0;
}


#ifdef TI_816X_BUILD
int Demo_switchIVAMap()
{
    static VSYS_IVA2CHMAP_TBL_S ivaMapTbl;
    static VSYS_IVA2CHMAP_TBL_S ivaMapTblPrev;

    Vsys_getIVAMap(&ivaMapTblPrev);
    if (ivaMapTblPrev.isPopulated)
    {
        /* Just swap all channel mapping for test */
        ivaMapTbl.isPopulated = TRUE;
        ivaMapTbl.ivaMap[0] = ivaMapTblPrev.ivaMap[1];
        ivaMapTbl.ivaMap[1] = ivaMapTblPrev.ivaMap[2];
        ivaMapTbl.ivaMap[2] = ivaMapTblPrev.ivaMap[0];
        Vsys_setIVAMap(&ivaMapTbl);
    }
    return 0;
}
#endif

char *Demo_getAudioString(int option)
{
    switch(option)
    {
        case DEMO_AUDIO_TYPE_CAPTURE:
            return "CAPTURE";
        case DEMO_AUDIO_TYPE_ENCODE:
            return "ENCODE";
        case DEMO_AUDIO_TYPE_DECODE:
            return "DECODE";
        default:
            return "UNKNOWN";        
    }
}

Int32 Demo_startStopAudioEncodeDecode (Int32 demoId, Int32 option, Bool userOpt)
{
#if defined(TI_8107_BUILD)
    return 0;
#else

    if (gDemo_info.audioInitialized == FALSE)
    {
        Demo_initAudioSystem();
        gDemo_info.audioInitialized = TRUE;
    }

    if ((Demo_IsCaptureActive() == FALSE) && 
            (Demo_IsEncodeActive() == FALSE) &&
            (Demo_IsDecodeActive() == FALSE))
    {
        gDemo_info.audioType = DEMO_AUDIO_TYPE_NONE;
        /* Forced stop & nothing is active */
        if (userOpt == FALSE)
            option = DEMO_AUDIO_TYPE_NONE;
    }

    if (demoId == DEMO_CUSTOM_0 || demoId == DEMO_CUSTOM_1 || demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)
    {
        switch (option)
        {
            case DEMO_AUDIO_TYPE_CAPTURE:
                if (gDemo_info.audioType == DEMO_AUDIO_TYPE_NONE)
                {
                    if (Demo_startAudioCaptureSystem() == TRUE)
                        gDemo_info.audioType = DEMO_AUDIO_TYPE_CAPTURE;
                }
                else if (gDemo_info.audioType == DEMO_AUDIO_TYPE_CAPTURE)
                {
                    if (Demo_stopAudioCaptureSystem(userOpt) == TRUE)
                        gDemo_info.audioType = DEMO_AUDIO_TYPE_NONE;
                }
                else
                {
                    printf ("Stop running %s demo!!!!\n", Demo_getAudioString(gDemo_info.audioType));
                }
                break;
            case DEMO_AUDIO_TYPE_ENCODE:
                if (gDemo_info.audioType == DEMO_AUDIO_TYPE_NONE)
                {
                    if (Demo_startAudioEncodeSystem() == TRUE)
                        gDemo_info.audioType = DEMO_AUDIO_TYPE_ENCODE;
                }
                else if (gDemo_info.audioType == DEMO_AUDIO_TYPE_ENCODE)
                {
                    if (Demo_stopAudioEncodeSystem(userOpt) == TRUE)
                        gDemo_info.audioType = DEMO_AUDIO_TYPE_NONE;
                }
                else
                {
                    printf ("Stop running %s demo!!!!\n", Demo_getAudioString(gDemo_info.audioType));
                }
                break;
            case DEMO_AUDIO_TYPE_DECODE:
                if (gDemo_info.audioType == DEMO_AUDIO_TYPE_NONE)
                {
                    if (Demo_startAudioDecodeSystem() == TRUE)
                        gDemo_info.audioType = DEMO_AUDIO_TYPE_DECODE;
                }
                else if (gDemo_info.audioType == DEMO_AUDIO_TYPE_DECODE)
                {
                    if (Demo_stopAudioDecodeSystem(userOpt) == TRUE)
                        gDemo_info.audioType = DEMO_AUDIO_TYPE_NONE;
                }
                else
                {
                    printf ("Stop running %s demo!!!!\n", Demo_getAudioString(gDemo_info.audioType));
                }
                break;
        }
    }
    return 0;
#endif
}

int Demo_printInfo(int demoId)
{

    Demo_captureGetVideoSourceStatus();
    Vsys_printDetailedStatistics();

    switch(demoId)
    {
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE:
        case DEMO_HYBRIDDVR_16CH:
        case DEMO_CARDVR_4CH:
        case DEMO_HYBRIDENC_36CH:
            VcapVencVdecVdis_printStatistics(TRUE, TRUE);
            Scd_printStatistics(TRUE);
            break;

        case DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED:
            break;

        case DEMO_CUSTOM_0:
        case DEMO_CUSTOM_1:
        case DEMO_CUSTOM_2:
        case DEMO_VCAP_VENC_VDIS:
            VcapVenc_printStatistics(TRUE);
            Scd_printStatistics(TRUE);
            break;
        case DEMO_VCAP_VENC_VDIS_HD:
            VcapVenc_printStatistics(TRUE);
            break;

        case DEMO_VDEC_VDIS:
            break;

        case DEMO_VCAP_VDIS:
            break;


#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1:
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH:
        case DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT:
            VcapVencVdecVdis_printStatistics(TRUE, TRUE);
            if(demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH)
               Scd_printStatistics(TRUE);
            break;
#endif
    }

    if(gDemo_info.audioEnable && gDemo_info.isAudioPathSet)
    {
        #ifndef SYSTEM_DISABLE_AUDIO
        Audio_capturePrintStats();
        Audio_playPrintStats();
        Audio_G711codecPrintStats();
        Audio_pramsPrint();
        #endif
    }

    return 0;
}



char Demo_getChar()
{
    char buffer[MAX_INPUT_STR_SIZE];

    fflush(stdin);
    fgets(buffer, MAX_INPUT_STR_SIZE, stdin);

    return(buffer[0]);
}

int Demo_getChId(char *string, int maxChId)
{
    char inputStr[MAX_INPUT_STR_SIZE];
    int chId;

    printf(" \n");
    printf(" Select %s CH ID [0 .. %d] : ", string, maxChId-1);

    fflush(stdin);
    fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);

    chId = atoi(inputStr);

    if(chId < 0 || chId >= maxChId )
    {
        chId = 0;

        printf(" \n");
        printf(" WARNING: Invalid CH ID specified, defaulting to CH ID = %d \n", chId);
    }
    else
    {
        printf(" \n");
        printf(" Selected CH ID = %d \n", chId);
    }

    printf(" \n");

    return chId;
}

int Demo_getIntValue(char *string, int minVal, int maxVal, int defaultVal)
{
    char inputStr[MAX_INPUT_STR_SIZE];
    int value;

    printf(" \n");
    printf(" Enter %s [Valid values, %d .. %d] : ", string, minVal, maxVal);

    fflush(stdin);
    fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);

    value = atoi(inputStr);

    if(value < minVal || value > maxVal )
    {
        value = defaultVal;
        printf(" \n");
        printf(" WARNING: Invalid value specified, defaulting to value of = %d \n", value);
    }
    else
    {
        printf(" \n");
        printf(" Entered value = %d \n", value);
    }

    printf(" \n");

    return value;
}

Bool Demo_getFileWriteEnable()
{
    char inputStr[MAX_INPUT_STR_SIZE];
    Bool enable;

    printf(" \n");
    printf(" Enable file write (YES - y / NO - n) : ");

    inputStr[0] = 0;

    fflush(stdin);
    fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);

    enable = FALSE;

    if(inputStr[0]=='y' || inputStr[0]=='Y' )
    {
        enable = TRUE;
    }

    printf(" \n");
    if(enable)
        printf(" File write ENABLED !!!\n");
    else
        printf(" File write DISABLED !!!\n");
    printf(" \n");
    return enable;
}

Bool Demo_getMotionTrackEnable()
{
    char inputStr[MAX_INPUT_STR_SIZE];
    Bool enable;

    printf(" \n");
    printf(" Enable Motion Tracking On Display (YES - y / NO - n) : ");

    inputStr[0] = 0;

    fflush(stdin);
    fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);

    enable = FALSE;

    if(inputStr[0]=='y' || inputStr[0]=='Y' )
    {
        enable = TRUE;
    }

    printf(" \n");
    if(enable)
        printf(" Motion Tracking ENABLED !!!\n");
    else
        printf(" Motion Tracking DISABLED !!!\n");
    printf(" \n");
    return enable;
}

Bool Demo_isPathValid( const char* absolutePath )
{

    if(access( absolutePath, F_OK ) == 0 ){

        struct stat status;
        stat( absolutePath, &status );

        return (status.st_mode & S_IFDIR) != 0;
    }
    return FALSE;
}

int Demo_getFileWritePath(char *path, char *defaultPath)
{
    int status=0;

    printf(" \n");
    printf(" Enter file write path : ");

    fflush(stdin);
    fgets(path, MAX_INPUT_STR_SIZE, stdin);

    printf(" \n");

    /* remove \n from the path name */
    path[ strlen(path)-1 ] = 0;

    if(!Demo_isPathValid(path))
    {
        printf(" WARNING: Invalid path [%s], trying default path [%s] ...\n", path, defaultPath);

        strcpy(path, defaultPath);

        if(!Demo_isPathValid(path))
        {
            printf(" WARNING: Invalid default path [%s], file write will FAIL !!! \n", path);

            status = -1;
        }
    }

    if(status==0)
    {
        printf(" Selected file write path [%s] \n", path);
    }

    printf(" \n");

    return 0;


}


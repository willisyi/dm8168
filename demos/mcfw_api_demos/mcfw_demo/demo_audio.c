
#include <demo.h>

#ifdef  DSP_RPE_AUDIO_ENABLE
#include <osa_thr.h>
#include <ti/xdais/dm/iaudio.h>
#endif

char gDemo_audioSettingsMenu[] = {
    "\r\n ==================="
    "\r\n Audio Settings Menu"
    "\r\n ==================="
    "\r\n"
    "\r\n 1: Set   Audio Storage Path"
    "\r\n 2: Start Audio Capture"
    "\r\n 3: Stop  Audio Capture"
    "\r\n 4: Start Audio Playback"
    "\r\n 5: Stop  Audio Playback"
    "\r\n 6: Set Audio Capture Configuration"
    "\r\n"
    "\r\n p: Previous Menu"
    "\r\n"
    "\r\n Enter Choice: "
};


char gDemo_audioParametersSettingsMenu[] = {
    "\r\n ==========================="
    "\r\n Audio Parameters Settings Menu"
    "\r\n ==========================="
    "\r\n"
    "\r\n 1: Set Sampling Frequency"
    "\r\n 2: Set Audio Volume"
    "\r\n 3: Mute Audio Volume"
    "\r\n"
    "\r\n p: Previous Menu"
    "\r\n"
    "\r\n Enter Choice: "
};

#ifdef  DSP_RPE_AUDIO_ENABLE

/****************************************************/
#define AUDIO_BITS_FWRITE_BUFFER_SIZE   (256*1024)

#define ENCODE_TYPE_AAC

#define ENCODE_BITRATE                  24000   /* value really depends on codec guidelines */   
#define AUDIO_TSK_PRI                   17
#define AUDIO_TSK_STACK_SIZE           (10*1024)

#define MAX_INPUT_BUFFER        (4*1024)
#define MAX_OUTPUT_BUFFER       (4*1024)

#define MAX_IN_SAMPLES          (1024)
#define SAMPLE_LEN              2

static UInt8            *gEncodeBuf[ACAP_CHANNELS_MAX];
static UInt16           *gCaptureBuf[ACAP_CHANNELS_MAX];

static OSA_ThrHndl     gAppDecodeThread;
static Bool            gAppDecodeThreadActive = FALSE;
static OSA_ThrHndl     gAppEncodeThread;
static Bool            gAppEncodeThreadActive = FALSE;
static OSA_ThrHndl     gAppCaptureThread;
static Bool            gAppCaptureThreadActive = FALSE;
static Bool            gAppCaptureThreadExitFlag = FALSE;
static Bool            gAppEncodeThreadExitFlag = FALSE;
static Bool            gAppDecodeThreadExitFlag = FALSE;
static Audio_DecInfo   gDecInfo;
static Audio_EncInfo   gEncInfo;
static Audio_CapInfo   gCapInfo;

#endif

int Demo_audioCaptureStop()
{
    if(!gDemo_info.audioEnable)
        return -1;

    printf(" \n");

    if(gDemo_info.isAudioPathSet)
    {
        if(gDemo_info.audioCaptureActive)
        {
            Audio_captureStop();
            printf(" Audio Capture is STOPPED !!! \n");

            gDemo_info.audioCaptureActive = FALSE;
        }
        else
        {
            printf(" Audio Capture is ALREADY STOPPED !!! \n");
        }
    }
    else
    {
        printf(" Audio Capture storage path NOT set !!! \n");
    }

    printf(" \n");

    return 0;
}

int Demo_audioPlaybackStop()
{
    if(!gDemo_info.audioEnable)
        return -1;

    printf(" \n");

    if(gDemo_info.isAudioPathSet)
    {
        if(gDemo_info.audioPlaybackActive)
        {
            Audio_playStop();
            printf(" Audio Playback is STOPPED !!! \n");

            gDemo_info.audioPlaybackActive = FALSE;
        }
        else
        {
            printf(" Audio Playback is ALREADY STOPPED !!! \n");
        }
    }
    else
    {
        printf(" Audio Playback storage path NOT set !!! \n");
    }

    printf(" \n");

    return 0;
}

int Demo_audioCaptureStart(UInt32 chId)
{
    if(!gDemo_info.audioEnable)
        return -1;

    printf(" \n");

    if(gDemo_info.isAudioPathSet)
    {
        gDemo_info.audioCaptureChNum = chId;

        if (gDemo_info.audioCaptureActive)
        {
            Demo_audioCaptureStop();
        }

        if(gDemo_info.audioCaptureChNum == gDemo_info.audioPlaybackChNum && gDemo_info.audioPlaybackActive)
        {
            printf (" Audio playback active on same channel. Stopping Playback ... \n");
            Demo_audioPlaybackStop();
        }
        Audio_captureStart(gDemo_info.audioCaptureChNum);

        gDemo_info.audioCaptureActive = TRUE;
    }
    else
    {
        printf(" Audio Capture storage path NOT set !!! \n");
    }

    printf(" \n");

    return 0;
}

int Demo_audioPlaybackStart(UInt32 chId, UInt32 playbackDevId)
{
    if(!gDemo_info.audioEnable)
        return -1;

    printf(" \n");

    if(gDemo_info.isAudioPathSet)
    {
        gDemo_info.audioPlaybackChNum = chId;

        if (gDemo_info.audioPlaybackActive)
        {
            Demo_audioPlaybackStop();
        }

        if(gDemo_info.audioCaptureChNum == gDemo_info.audioPlaybackChNum && gDemo_info.audioCaptureActive)
        {
            printf (" Audio capture active on same channel. Stopping Capture ... \n");
            Demo_audioCaptureStop();
        }

        Audio_playStart(gDemo_info.audioPlaybackChNum,playbackDevId);
        gDemo_info.audioPlaybackActive = TRUE;
    }
    else
    {
        printf(" Audio Playback storage path NOT set !!! \n");
    }

    printf(" \n");


    return 0;
}

int Demo_audioSettings(int demoId)
{
    Bool done = FALSE;
    char ch;
    char audioPath[256];
    int chId;
    int playbackDevId;

    if(gDemo_info.maxVcapChannels<=0 || !gDemo_info.audioEnable)
    {
        printf(" \n");
        printf(" WARNING: Audio NOT enabled, this menu is NOT valid !!!\n");
        return -1;
    }

    while(!done)
    {
        printf(gDemo_audioSettingsMenu);

        ch = Demo_getChar();

        switch(ch)
        {
            case '1':
                Demo_getFileWritePath(audioPath, "/dev/shm");

                if (Audio_setStoragePath(audioPath) == AUDIO_STATUS_OK)
                {
                    gDemo_info.isAudioPathSet = TRUE;
                }
                else
                {
                    gDemo_info.isAudioPathSet = FALSE;
                }

                break;
            case '2':
                chId = Demo_getChId("AUDIO CAPTURE", gDemo_info.maxVcapChannels);

                Demo_audioCaptureStart(chId);

                break;
            case '3':
                Demo_audioCaptureStop();
                break;
            case '4':
                chId = Demo_getChId("AUDIO PLAYBACK", gDemo_info.maxVcapChannels);
                playbackDevId = Demo_getIntValue("Playback Device 0-AIC3x 1-HDMI Out", 0, 1, 0);
                
                Demo_audioPlaybackStart(chId,playbackDevId);
                break;
            case '5':
                Demo_audioPlaybackStop();
                break;
            case '6':
                Demo_audioCaptureSetParams(TRUE);
                break;
            case 'p':
                done = TRUE;
                break;
        }
    }

    return 0;
}

int Demo_audioEnable(Bool enable)
{
    if(!gDemo_info.audioEnable)
        return -1;

    if(enable)
    {
        Audio_captureCreate();
        Audio_playCreate();
        Demo_audioCaptureSetParams(FALSE);
        gDemo_info.isAudioPathSet = FALSE;
        gDemo_info.audioCaptureActive = FALSE;
        gDemo_info.audioPlaybackActive = FALSE;
        gDemo_info.audioPlaybackChNum = 0;
        gDemo_info.audioCaptureChNum = 0;
    }
    else
    {
        Demo_audioCaptureStop();
        Demo_audioPlaybackStop();

        Audio_captureDelete();
        Audio_playDelete();
    }

    return 0;
}


int Demo_audioCaptureSetParams(Bool set_params)
{
    char ch;
    Bool done = FALSE;
    Uint32 samplingHz;
    Int32 audioVolume;
    if(!gDemo_info.audioEnable)
        return -1;
    if (FALSE == set_params)
    {
        Audio_playSetSamplingFreq(AUDIO_SAMPLE_RATE_DEFAULT,AUDIO_VOLUME_DEFAULT);
        return 0;
    }
    
    Audio_playGetSamplingFreq(&samplingHz,&audioVolume);

    while(!done)
    {
        printf(gDemo_audioParametersSettingsMenu);

        ch = Demo_getChar();

        switch(ch)
        {
            case '1':

                if(Audio_captureIsStart() == 0)
                {
                    printf("\r\n Enter the Frequency [8000 or 16000  Khz]:");
                    scanf("%d",&samplingHz);
                }
                else
                {
                    printf("\r\n Sampling Frquecny can not be changed when Audio Capture is Running ");
                    printf("\r\n Stop Audio Capture");
                }

                break;
            case '2':

                printf("\r\n Enter the Audio Volume[0..8] :");
                scanf("%d",&audioVolume);

                break;
            case '3':

                audioVolume = 0;
                printf("\r\n Audio capture is muted. Increase audio capture volume to unmute");
               
                break;
            case 'p':
                done = TRUE;
                break;
        }
    }

    if ((samplingHz == 8000 || samplingHz == 16000)&&((audioVolume >= 0 || audioVolume <= 8)))
    {
        Vcap_setAudioModeParam(AUDIO_MAX_CHANNELS, samplingHz,audioVolume);
    }
    
    


    return 0;
}

#ifdef  DSP_RPE_AUDIO_ENABLE
static Void writeToFile (FILE *outputFile, ADEC_PROCESS_PARAMS_S *pPrm)
{
    if (pPrm->numSamples <= 0)
        return;

    if(pPrm->channelMode == IAUDIO_1_0)
    {
        fwrite(pPrm->outBuf.dataBuf, pPrm->bytesPerSample, pPrm->numSamples, outputFile);
    }
    else 
    {
        if(pPrm->pcmFormat == IAUDIO_INTERLEAVED)
        {
          fwrite(pPrm->outBuf.dataBuf, pPrm->bytesPerSample, pPrm->numSamples*2, outputFile);
        }
        else
        { 
            UInt32 i;
            UInt8 * outL = (UInt8 *) pPrm->outBuf.dataBuf;
            UInt8 * outR = (UInt8 *)((UInt8 *) pPrm->outBuf.dataBuf + 
                                         (pPrm->numSamples * pPrm->bytesPerSample)) ;
            for(i=0;i<(pPrm->numSamples * pPrm->bytesPerSample); i+= pPrm->bytesPerSample)
            {
              /*--------------------------------------------------------------*/
              /* Write the left anf right channels                            */
              /*--------------------------------------------------------------*/
              fwrite(&outL[i], pPrm->bytesPerSample, 1, outputFile);
              fwrite(&outR[i], pPrm->bytesPerSample, 1, outputFile);
            }
        }
    }
}


static Void *App_allocBuf(Int32 bufSize, Bool fromSharedRegion)
{
    if (fromSharedRegion == TRUE)
        return Audio_allocateSharedRegionBuf(bufSize);
    else
        return malloc(bufSize);
}

static Void App_freeBuf (Void *buf, Int32 bufSize, Bool fromSharedRegion)
{
    if (fromSharedRegion == TRUE)
        return Audio_freeSharedRegionBuf(buf, bufSize);
    else
        return free(buf);
}

static Void *App_encodeTaskFxn(Void * prm)
{
    FILE                  *in = NULL, *out = NULL;
    UInt8                 *inBuf = NULL;
    UInt8                 *outBuf = NULL;
    AENC_PROCESS_PARAMS_S encPrm;
    Int32                 rdIdx, to_read, readBytes;
    Int32                 frameCnt = 0, totalBytes = 0;
    Int32                 inBytes, inBufSize, outBufSize;

    AENC_CREATE_PARAMS_S  aencParams;
    Void                  *encHandle = NULL;
    Audio_EncInfo         *info = prm;
    Bool                  isSharedRegion = FALSE;

    if (!prm)
    {
        printf ("AENC task failed <invalid param>...........\n");
        return NULL;
    }

    aencParams.bitRate = info->bitRate;
    aencParams.encoderType = (info->encodeType == 0) ? AUDIO_CODEC_TYPE_AAC_LC : AUDIO_CODEC_TYPE_G711;
    aencParams.numberOfChannels = info->numChannels;
    aencParams.sampleRate = info->sampleRate;

    encHandle = Aenc_create(&aencParams);
    if (encHandle)
    {
        printf ("AENC Create done...........\n");
    }
    else
    {
        printf ("AENC Create failed...........\n");
        printf ("\n********Encode APP Task Exitting....\n");
        gAppEncodeThreadActive = FALSE;
        gAppEncodeThreadExitFlag = TRUE;
        return NULL;
    }

    gAppEncodeThreadExitFlag = FALSE;

    if (aencParams.encoderType == AUDIO_CODEC_TYPE_AAC_LC)
    {
        inBufSize = (MAX_IN_SAMPLES * SAMPLE_LEN * aencParams.numberOfChannels);
        if (inBufSize < aencParams.minInBufSize)
            inBufSize = aencParams.minInBufSize;
    
        outBufSize = MAX_OUTPUT_BUFFER;
        if (outBufSize < aencParams.minOutBufSize)
            outBufSize = aencParams.minOutBufSize;
    }
    else
    {
        inBufSize = MAX_INPUT_BUFFER;
        outBufSize = MAX_OUTPUT_BUFFER;
    }

    inBytes = inBufSize;

    in = fopen(info->inFile, "rb");
    if (!in)
    {
        printf ("File <%s> Open Error....\n", info->inFile);
    }  
    out = fopen(info->outFile, "wb");  
    if (!in)
    {
        printf ("File <%s> Open Error....\n", info->outFile);
    }  
    if (aencParams.encoderType == AUDIO_CODEC_TYPE_AAC_LC)
        isSharedRegion = TRUE;

    inBuf = App_allocBuf(inBytes, isSharedRegion);
    outBuf = App_allocBuf(outBufSize, isSharedRegion); 

    if (!inBuf || !outBuf)
    {
        printf ("Memory Error....\n");
    }

    if (in && out && inBuf && outBuf)
    {
        printf ("\n\n=============== Starting Encode ===================\n");
        sleep(1);

        rdIdx = 0;
        to_read = inBytes;
        encPrm.outBuf.dataBuf = outBuf;

        while (gAppEncodeThreadActive == TRUE)
        {
            readBytes = fread (inBuf + rdIdx, 1, to_read, in);
            if (readBytes)
            {
                encPrm.inBuf.dataBufSize = readBytes;
                encPrm.inBuf.dataBuf = inBuf;
                encPrm.outBuf.dataBufSize = outBufSize;
                Aenc_process(encHandle, &encPrm);
                if (encPrm.outBuf.dataBufSize <= 0)
                {
                    printf ("ENC: Encoder didnt generate bytes <remaining - %d>... exiting....\n", readBytes);
                    printf ("=============== Encode completed, bytes generated <%d> ================\n",
                            totalBytes);
                    break;
                }
                if (out)
                {
                    fwrite(encPrm.outBuf.dataBuf, 1, encPrm.outBuf.dataBufSize, out);
                }
                {
//                    printf ("ENC - %d, Bytes Generated - %d <total %d>, bytesConsumed - %d\n", 
//                            frameCnt,  encPrm.outBuf.dataBufSize, totalBytes, encPrm.inBuf.dataBufSize);              
                }
                frameCnt++;
                totalBytes += encPrm.outBuf.dataBufSize;
            }
            else
            {
                printf ("=============== Encode completed, bytes generated <%d> ================\n",
                        totalBytes);
                break;
            }
        }
    }
    else
    {
        printf ("\n\n=============== Encode not starting.... file / mem error ============\n");
    }
    prm = prm;
    if (in)
        fclose(in);
    if (out)
        fclose(out);
    if (inBuf)
        App_freeBuf(inBuf, inBufSize, isSharedRegion);
    if (outBuf)
        App_freeBuf(outBuf, outBufSize, isSharedRegion);

    Aenc_delete(encHandle);

    printf ("\n********Encode APP Task Exitting....\n");
    gAppEncodeThreadActive = FALSE;
    gAppEncodeThreadExitFlag = TRUE;

    return NULL;
}

static void Demo_printDecodeStreamParams (ADEC_PROCESS_PARAMS_S *pPrm)
{
    printf ("\n\n------------ Decoded Stream------------\n");

    if(pPrm->channelMode == IAUDIO_1_0)
    {
        printf ("Channel Mode = MONO\n");
    }
    else 
    {
        printf ("Channel Mode = STEREO, ");
        if(pPrm->pcmFormat == IAUDIO_INTERLEAVED)
            printf (" INTERLEAVED\n");
        else
            printf (" PLANAR\n");
    }
    printf ("\n\n");
}

static Void *App_decodeTaskFxn(Void * prm)
{
    FILE                  *in = NULL, *out = NULL;
    UInt8                 *inBuf = NULL;
    UInt8                 *outBuf = NULL;
    ADEC_PROCESS_PARAMS_S decPrm;
    Int32                 rdIdx, to_read, readBytes;
    ADEC_CREATE_PARAMS_S  adecParams;
    Void                  *decHandle = NULL;
    Int32                 frameCnt = 0, totalSamples = 0;
    Int32                 inBufSize, outBufSize;
    Audio_DecInfo         *info = prm;
    Bool                  isSharedRegion = FALSE;

    if (!prm)
    {
        printf ("ADEC task failed <invalid param>...........\n");
        return NULL;
    }

    adecParams.decoderType = (info->decodeType == 0) ? AUDIO_CODEC_TYPE_AAC_LC : AUDIO_CODEC_TYPE_G711;
    adecParams.desiredChannelMode = info->numChannels;

    decHandle = Adec_create(&adecParams);
    if (decHandle)
    {
        printf ("ADEC Create done...........\n");
    }
    else
    {
        printf ("ADEC Create failed...........\n");
        printf ("\n********Decode APP Task Exitting....\n");
        gAppDecodeThreadActive = FALSE;
        gAppDecodeThreadExitFlag = TRUE;
        return NULL;
    }
    gAppDecodeThreadExitFlag = FALSE;

    if (adecParams.decoderType == AUDIO_CODEC_TYPE_AAC_LC)
        isSharedRegion = TRUE;

    inBufSize = MAX_INPUT_BUFFER / 2;
    if (inBufSize < adecParams.minInBufSize)
        inBufSize = adecParams.minInBufSize;

    outBufSize = MAX_OUTPUT_BUFFER;
    if (outBufSize < adecParams.minOutBufSize)
        outBufSize = adecParams.minOutBufSize;

    in = fopen(info->inFile, "rb");
    if (!in)
    {
        printf ("File <%s> Open Error....\n", info->inFile);
    }  

    out = fopen(info->outFile, "wb");    
    if (!out)
    {
        printf ("File <%s> Open Error....\n", info->outFile);
    }  

    inBuf = App_allocBuf(inBufSize, isSharedRegion);
    outBuf = App_allocBuf(outBufSize, isSharedRegion); 

    if (in && out && inBuf && outBuf)
    {
        printf ("\n\n=============== Starting Decode ===================\n");
        sleep(1);

        rdIdx = 0;
        to_read = inBufSize;
        decPrm.outBuf.dataBuf = outBuf;

        while (gAppDecodeThreadActive == TRUE)
        {
            readBytes = fread (inBuf + rdIdx, 1, to_read, in);
            readBytes += rdIdx;
            if (readBytes)
            {
                decPrm.inBuf.dataBufSize = readBytes;
                decPrm.inBuf.dataBuf = inBuf;
                decPrm.outBuf.dataBufSize = outBufSize;
                Adec_process(decHandle, &decPrm);
                if (decPrm.inBuf.dataBufSize <= 0)
                {
                    if (totalSamples <= 0)
                        printf ("DEC: Decoder didnt consume bytes <%d>... exiting....\n", readBytes);
                    printf ("=============== Decode completed, %d samples generated ================\n", totalSamples);
                    break;
                }

                if ((totalSamples == 0) && (adecParams.decoderType == AUDIO_CODEC_TYPE_AAC_LC))
                {
                    Demo_printDecodeStreamParams(&decPrm);
                }

                if (out)
                {
                    writeToFile(out, &decPrm);
                }
                rdIdx = (readBytes-decPrm.inBuf.dataBufSize);
                memmove(inBuf, inBuf+decPrm.inBuf.dataBufSize, rdIdx);
                to_read = decPrm.inBuf.dataBufSize;
                {
//                    printf ("DEC - %d, Samples Generated - %d <total %d>, bytesConsumed - %d\n", 
//                            frameCnt, decPrm.numSamples, totalSamples, decPrm.inBuf.dataBufSize);              
                }
                frameCnt++;
                totalSamples += decPrm.numSamples;
            }
            else
            {
                printf ("=============== Decode completed, %d samples generated ================\n", totalSamples);
                break;
            }
        }
    }
    else
    {
        printf ("\n\n=============== Decode not starting.... file / mem error ============\n");
    }
    prm = prm;
    if (in)
        fclose(in);
    if (out)
        fclose(out);
    if (inBuf)
        App_freeBuf(inBuf, inBufSize, isSharedRegion);
    if (outBuf)
        App_freeBuf(outBuf, outBufSize, isSharedRegion);
    Adec_delete(decHandle);

    printf ("\n********Decode APP Task Exitting....\n");
    gAppDecodeThreadActive = FALSE;
    gAppDecodeThreadExitFlag = TRUE;
    return NULL;
}


static Void *App_captureTaskFxn(Void * prm)
{
    ACAP_PARAMS_S acapParams;
    ACAP_GET_DATA_PARAMS_S dataPrm;
    FILE *fp[ACAP_CHANNELS_MAX];
    UInt32 bytesWritten[ACAP_CHANNELS_MAX];
    Int32 i;
    Int8  file[250];
    UInt8 *buf;
    Int32 bytesAvailable;

    gAppCaptureThreadExitFlag = FALSE;

    Acap_params_init(&acapParams);
    memset(gCaptureBuf, 0, sizeof(gCaptureBuf));
    memset(gEncodeBuf, 0, sizeof(gEncodeBuf));

    acapParams.enableTVP5158 = TRUE;

    printf ("*************Capture Buf Size %d, Enc Buffer Size %d\n", 
            acapParams.chPrm[0].captureBuf.dataBufSize*acapParams.sampleLen,
            acapParams.chPrm[0].encodeBuf.dataBufSize
            );
    for(i=0; i<acapParams.numChannels; i++)
    {
        gCaptureBuf[i] = NULL;
        gEncodeBuf[i] = NULL;

        if (i < gCapInfo.numEncodeChannels)
        {
            acapParams.chPrm[i].enableEncode = TRUE;
            #ifdef  ENCODE_TYPE_AAC
                acapParams.chPrm[i].encodeParam.bitRate = ENCODE_BITRATE;
                acapParams.chPrm[i].encodeParam.encoderType = AUDIO_CODEC_TYPE_AAC_LC;
            #else
                acapParams.chPrm[i].encodeParam.encoderType = AUDIO_CODEC_TYPE_G711;
            #endif

            /* Allocate in shared region for DSP Encode */
            acapParams.chPrm[i].captureBuf.dataBuf = 
                                    gCaptureBuf[i] = 
                                    App_allocBuf(acapParams.chPrm[i].captureBuf.dataBufSize*acapParams.sampleLen, TRUE);
            if (gCaptureBuf[i] == NULL)
            {
                printf ("Capture buffer allocation failed..............\n");
            }

            acapParams.chPrm[i].encodeBuf.dataBuf = gEncodeBuf[i] = App_allocBuf(acapParams.chPrm[i].encodeBuf.dataBufSize, TRUE);
            if (gEncodeBuf[i] == NULL)
            {
                printf ("Encode buffer allocation failed..............\n");
                return NULL;
            }
            /* numberOfChannels & sampleRate will be assigned internally */
        }
        else if (i < ACAP_CHANNELS_MAX)
        {
            acapParams.chPrm[i].enableEncode = FALSE;
            acapParams.chPrm[i].captureBuf.dataBuf = 
                            gCaptureBuf[i] = 
                            App_allocBuf(acapParams.chPrm[i].captureBuf.dataBufSize*acapParams.sampleLen, FALSE);
        }
    }

    if (Acap_init(&acapParams) != 0)
    {
        printf ("\n\n----------- Capture start failed............\n");
        goto CAP_EXIT;
    }

    Acap_start();

    for(i=0; i<ACAP_CHANNELS_MAX; i++)
    {
        fp[i] = NULL;

        if (acapParams.chPrm[i].enableEncode == TRUE)
#ifdef  ENCODE_TYPE_AAC
            sprintf (file, "%s/ch%02d.aac", gCapInfo.outFile, i);
#else
            sprintf (file, "%s/ch%02d.ulaw", gCapInfo.outFile,i);
#endif
        else
            sprintf (file, "%s/ch%02d.pcm", gCapInfo.outFile, i);
        fp[i] = fopen (file, "wb");
        bytesWritten[i] = 0;

        setvbuf(fp[i],
              NULL,
              _IOFBF,
              AUDIO_BITS_FWRITE_BUFFER_SIZE);
    }
   
       
    while (gAppCaptureThreadActive == TRUE)
    {
        if (gAppCaptureThreadActive == FALSE)
            break;

        /* Retrieve fewer channels data */
        for (i=0; i<gCapInfo.numEncodeChannels; i++)
        {
            if (gAppCaptureThreadActive == FALSE)
                break;

            Acap_getData(i, &dataPrm);
            if (acapParams.chPrm[i].enableEncode == TRUE)
            {
                bytesAvailable = dataPrm.encodeDataSize;
                buf = dataPrm.encodeDataBuf;
            }
            else
            {
                bytesAvailable = dataPrm.captureDataSize*2;
                buf = dataPrm.captureDataBuf;
            }
            if (bytesAvailable == 0)
            {
                usleep(1000);
                continue;
            }
            else
            {
                if (fp[i] && bytesAvailable)
                {
                    bytesWritten[i] += fwrite(buf, bytesAvailable, 1, fp[i]);
                    if (!(bytesWritten[i] % (1024)))
                        printf ("*****AUDIO: %d samples written for CH:%d.........\n", bytesWritten[i], i);
                }
                if (acapParams.chPrm[i].enableEncode == TRUE)
                    Acap_setConsumedData(i, dataPrm.captureDataSize, 0);
                else
                    Acap_setConsumedData(i, 0, dataPrm.encodeDataSize);
            }
        }
        if (!(gCapInfo.numEncodeChannels))
        {
            /* Not retrieving capture raw data */
            usleep(100);
        }
    }

    Acap_deInit();

CAP_EXIT:
    gAppCaptureThreadActive = FALSE;
    for(i=0; i<ACAP_CHANNELS_MAX; i++)
    {
        if (fp[i])  
            fclose(fp[i]);
        if (acapParams.chPrm[i].enableEncode == TRUE)
        {
            if (gEncodeBuf[i])
                App_freeBuf(gEncodeBuf[i], acapParams.chPrm[i].encodeBuf.dataBufSize, TRUE);
            if (gCaptureBuf[i])
                App_freeBuf(gCaptureBuf[i], (acapParams.chPrm[i].captureBuf.dataBufSize*acapParams.sampleLen), TRUE);
        }
        else
        {
            if (gCaptureBuf[i])
                App_freeBuf(gCaptureBuf[i], (acapParams.chPrm[i].captureBuf.dataBufSize*acapParams.sampleLen), FALSE);
        }
    }
    printf ("\n********Capture APP Task Exitting....\n");
    gAppCaptureThreadExitFlag = TRUE;
    return NULL;
}

Void Demo_initAudioSystem(Void)
{
    Audio_systemInit();
}

Void Demo_deInitAudioSystem(Void)
{
    Audio_systemDeInit();
}

Bool Demo_startAudioEncodeSystem (Void)
{
    char ch;
    Bool ret = FALSE;

    printf("\r\n\n\n AUDIO: Enable Encode <Y/N>: " );
    ch = Demo_getChar();
    
    if (ch == 'y' || ch == 'Y')
    {
        Int32 status;

        printf("\r\n AUDIO: Enter input PCM file name <absolute path>: " );
        fflush(stdin);
        fgets(gEncInfo.inFile, MAX_INPUT_STR_SIZE, stdin);
        gEncInfo.inFile[ strlen(gEncInfo.inFile)-1 ] = 0;

        printf("\r\n AUDIO: Enter output file name <absolute path>: " );
        fflush(stdin);
        fgets(gEncInfo.outFile, MAX_INPUT_STR_SIZE, stdin);
        gEncInfo.outFile[ strlen(gEncInfo.outFile)-1 ] = 0;

        gEncInfo.encodeType = Demo_getIntValue("encode Type <0 - AAC-LC, 1 - G711>", 0, 1, 0);
        if (gEncInfo.encodeType == 0)
        {
            gEncInfo.bitRate = Demo_getIntValue("AUDIO: bitrate <value as per encode requirement>", 24000, 800000, 24000);

            gEncInfo.numChannels = Demo_getIntValue("AUDIO: num of audio channels", 1, 2, 1);

            gEncInfo.sampleRate = Demo_getIntValue("AUDIO: Sample Rate", 16000, 48000, 16000);
        }

        gAppEncodeThreadActive = TRUE;
        status = OSA_thrCreate(&gAppEncodeThread,
                      App_encodeTaskFxn,
                      AUDIO_TSK_PRI, 
                      AUDIO_TSK_STACK_SIZE, 
                      &gEncInfo);
        if (status != 0)
        {
            gAppEncodeThreadActive = FALSE;
            printf ("AUDIO: App Encode thread create failed...\n");
            return ret;
        }
        ret = TRUE;
    }
    printf ("\r\n\n");
    return ret;
}


Bool Demo_startAudioDecodeSystem (Void)
{
    char ch;
    Bool ret = FALSE;

    printf("\r\n\n\nAUDIO: Enable Decode <Y/N>: " );
    ch = Demo_getChar();
    
    if (ch == 'y' || ch == 'Y')
    {
        Int32 status;

        printf("\r\nAUDIO: Enter input AAC file name <absolute path>: " );
        fflush(stdin);
        fgets(gDecInfo.inFile, MAX_INPUT_STR_SIZE, stdin);
        gDecInfo.inFile[ strlen(gDecInfo.inFile)-1 ] = 0;

        printf("\r\nAUDIO: Enter output file name <absolute path>: " );
        fflush(stdin);
        fgets(gDecInfo.outFile, MAX_INPUT_STR_SIZE, stdin);
        gDecInfo.outFile[ strlen(gDecInfo.outFile)-1 ] = 0;

        gDecInfo.decodeType = Demo_getIntValue("AUDIO: decode Type <0 - AAC-LC, 1 - G711>", 0, 1, 0);
        if (gDecInfo.decodeType == 0)
        {
            gDecInfo.numChannels = Demo_getIntValue("AUDIO: desired audio channels", 1, 2, 1);
        }
        gAppDecodeThreadActive = TRUE;
        status = OSA_thrCreate(&gAppDecodeThread,
                      App_decodeTaskFxn,
                      AUDIO_TSK_PRI, 
                      AUDIO_TSK_STACK_SIZE, 
                      &gDecInfo);
        if (status != 0)
        {
            printf ("AUDIO: App Decode thread create failed...\n");
            gAppDecodeThreadActive = FALSE;
            return ret;
        }
        ret = TRUE;
    }
    printf ("\r\n\n");
    return ret;
}

Bool Demo_startAudioCaptureSystem (Void)
{
    Bool ret = FALSE;
    char ch;

    printf("\r\n\n\nAUDIO: Enable Capture & Encode <Y/N>: " );
    ch = Demo_getChar();
    
    if (ch == 'y' || ch == 'Y')
    {
        Int32 status;

        printf("\r\nAUDIO: Enter Path to store Audio <absolute path>: " );
        fflush(stdin);
        fgets(gCapInfo.outFile, MAX_INPUT_STR_SIZE, stdin);
        gCapInfo.outFile[ strlen(gCapInfo.outFile)-1 ] = 0;

        gCapInfo.numEncodeChannels = Demo_getIntValue("AUDIO: Enter number of channels to be encoded - ", 0, ACAP_CHANNELS_MAX, 4);

        status = OSA_thrCreate(&gAppCaptureThread,
                      App_captureTaskFxn,
                      AUDIO_TSK_PRI, 
                      AUDIO_TSK_STACK_SIZE, 
                      NULL);
        if (status != 0)
        {
            printf ("AUDIO: App capture thread create failed...\n");
            return ret;
        }
        gAppCaptureThreadActive = TRUE;
        ret = TRUE;
    }
    printf ("\r\n\n");
    return ret;
}

Bool Demo_stopAudioDecodeSystem (Bool userOpt)
{
    Bool ret = FALSE;

    if (gAppDecodeThreadActive == TRUE)
    {
        char ch = 'Y';

        if (userOpt == TRUE)
        {
            printf("\r\n\n\nAUDIO: Stop Decode <Y/N>: " );
            ch = Demo_getChar();
        }
            
        if (ch == 'y' || ch == 'Y')
        {
            gAppDecodeThreadActive = FALSE;
            while (gAppDecodeThreadExitFlag == FALSE)
            {
                printf ("**** Waiting for Decode task to exit .....\n");
                OSA_waitMsecs(1000);
            }
            OSA_thrDelete(&gAppDecodeThread);
            printf ("AUDIO:  Decode stopped....\n");
            ret = TRUE;
        }
    }
    printf ("\r\n\n");
    return ret;
}

Bool Demo_stopAudioEncodeSystem (Bool userOpt)
{
    Bool ret = FALSE;

    if (gAppEncodeThreadActive == TRUE)
    {
        char ch = 'Y';

        if (userOpt == TRUE)
        {
            printf("\r\n\n\nAUDIO: Stop Encode <Y/N>: " );
            ch = Demo_getChar();
        }

        if (ch == 'y' || ch == 'Y')
        {
            gAppEncodeThreadActive = FALSE;

            while (gAppEncodeThreadExitFlag == FALSE)
            {
                printf ("**** Waiting for Encode task to exit .....\n");
                OSA_waitMsecs(1000);
            }
            OSA_thrDelete(&gAppEncodeThread);
            printf ("AUDIO:  Encode stopped....\n");
            ret = TRUE;
        }
    }
    printf ("\r\n\n");
    return ret;
}

Bool Demo_stopAudioCaptureSystem (Bool userOpt)
{
    Bool ret = FALSE;

    if (gAppCaptureThreadActive == TRUE)
    {
        char ch = 'Y';

        if (userOpt == TRUE)
        {
            printf("\r\n\n\nAUDIO: Stop Capture[Encode] <Y/N>: " );
            ch = Demo_getChar();
        }

        if (ch == 'y' || ch == 'Y')
        {
            gAppCaptureThreadActive = FALSE;
            while (gAppCaptureThreadExitFlag == FALSE)
            {
                printf ("**** Waiting for capture task to exit .....\n");
                OSA_waitMsecs(1000);
            }

            OSA_thrDelete(&gAppCaptureThread);
            printf ("AUDIO:  Capture stopped....\n");
            ret = TRUE;
        }
    }
    printf ("\r\n\n");
    return ret;
}


Bool Demo_IsCaptureActive(Void)
{
    return gAppCaptureThreadActive;
}

Bool Demo_IsEncodeActive(Void)
{
    return gAppEncodeThreadActive;
}


Bool Demo_IsDecodeActive(Void)
{
    return gAppDecodeThreadActive;
} 

#endif

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/            *
 *                        ALL RIGHTS RESERVED                                 *
 *                                                                             *
 ******************************************************************************/
#include <audio.h>
#include <audio_priv.h>
#include <sys/time.h>


static Int32 deInitAudioPlayback (void);
static Int32 playAudio(Int8 *buffer, Int32 numBytes);
static Int32 InitAudioRenderDevice (Int32 channels, Uint32 sample_rate, UInt32 playbackDevId);
static Void Audio_playMain (Void * arg);


static Int32                playAudioFlag = 0, playChNum = 0;
static snd_pcm_t            *play_handle = NULL;
static Int32                size;
static snd_pcm_uframes_t    frames;
static TaskCtx              playThrHandle;
static Int8                 audioPlaybackBuf[AUDIO_MAX_PLAYBACK_BUF_SIZE];
static Int8                 G711audioPlaybackBuf[AUDIO_MAX_PLAYBACK_BUF_SIZE]; /* Added Support for G711 Codec */


extern UInt32 samplingHz;
extern UInt32 audioVolume;


static AudioStats           playStats;
static int                  audioSamplesPlayed = 0;
static int                  audioPlaybackLen = 0;

AVSyncInfo                  gAVSyncInfo;

extern Int8                 audioPath[];
#ifdef AUDIO_G711CODEC_ENABLE
extern Uint8                audioG711codec;
#endif
Int32 Audio_playCreate (Void)
{
    Int32  status = AUDIO_STATUS_OK;

    status = Audio_thrCreate(
                &playThrHandle,
                (ThrEntryFunc) Audio_playMain,
                AUDIO_PLAY_TSK_PRI,
                AUDIO_PLAY_TSK_STACK_SIZE
                );
    UTILS_assert(  status==AUDIO_STATUS_OK);
    printf ("\n Audio playback task created");
    return  status;
}

Int32 Audio_playSetSamplingFreq (UInt32 freq,Uint32 vol)
{
    samplingHz      = freq;
    audioVolume    = vol;
    return 0;
}

Int32 Audio_playGetSamplingFreq (Uint32 *freq, Int32 *vol)
{
    *freq = samplingHz;
    *vol = audioVolume;
    return 0;
}

static Int Audio_playInitAvsyncInfo(UInt32 chNum,UInt32 playbackDevId)
{
    Int status;
    AvsyncLink_AudQueCreateParams params;
    Int i;

    OSA_queCreate(&gAVSyncInfo.freeAudFrmQue,AUDIO_MAX_QUE_LENGTH);
    for (i = 0; i < OSA_ARRAYSIZE(gAVSyncInfo.audFrames); i++)
    {
        OSA_quePut(&gAVSyncInfo.freeAudFrmQue,
                   (Int32)&gAVSyncInfo.audFrames[i],
                   OSA_TIMEOUT_NONE);
    }
    params.chNum = chNum;
    params.audioDevID = playbackDevId;
    params.maxElements = AUDIO_MAX_QUE_LENGTH;
    status = Avsync_audQueCreate(&params,&gAVSyncInfo.queObj);
    UTILS_assert(  status==AVSYNC_S_OK);
    return status;
}

static Int Audio_playDeInitAvsyncInfo()
{
    Int status;

    status = Avsync_audQueDelete(&gAVSyncInfo.queObj);
    UTILS_assert(  status==AVSYNC_S_OK);
    OSA_queDelete(&gAVSyncInfo.freeAudFrmQue);
    return status;
}


Int32 Audio_playStart (Int8     chNum, UInt32 playbackDevId)
{
    InitAudioRenderDevice(AUDIO_CHANNELS_SUPPORTED, samplingHz,playbackDevId);
    playChNum = chNum;
    Audio_playInitAvsyncInfo(chNum,playbackDevId);
    playAudioFlag = 1;
    sleep(1);
    return 0;
}


Int32 Audio_playStop (Void)
{
    playAudioFlag = 0;
    sleep(1);
    deInitAudioPlayback();
    Audio_playDeInitAvsyncInfo();
    return 0;
}

Int32 Audio_playDelete (Void)
{
    printf ("\n Deleting Audio playback task");
    Audio_thrDelete(&playThrHandle);
    printf (" Audio playback task deleted\n");

    return 0;
}


Int32 Audio_playPrintStats (Void)
{
    printf ("\n============================\n");
    if (playAudioFlag == 1)
        printf ("Playback ACTIVE,  ");
    else
        printf ("Playback NOT ACTIVE,  ");
    printf ("Channel %d, ErrCnt [%d], lastErr [%d, %s]\n", playChNum, playStats.errorCnt, playStats.lastError, snd_strerror(playStats.lastError));
    return 0;
}

static FILE * openFile (Int8 chNum, Int8 idx)
{
    FILE *fp;
    char fname[256];
    Uint8 dirlen;

    if (idx >= AUDIO_MAX_FILES)
    {
        return NULL;
    }

    strcpy(fname, audioPath);
    dirlen = strlen(fname);
    sprintf (&fname[dirlen], "/%02d/%s%02d.pcm", chNum + 1, AUDIO_RECORD_FILE, idx + 1);

    idx ++;

    fp = fopen (fname, "rb" );
    if (fp <= 0)
    {
//      printf ("==== File [%s] Open Error - %s....\n", fname, strerror(errno));
    }
    else
    {
        printf ("opened %s...\n", fname);
    }
    return fp;
}

Uint64 Audio_GetCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((Int64)tv.tv_sec*1000000 + tv.tv_usec)/1000;
}

static Int Audio_enqueDecodedFrame(Int8  *buf,Int32 len, UInt64 framePTS)
{
    Int status;
    AudFrm_Buf *audFrm;

    status = OSA_queGet(&gAVSyncInfo.freeAudFrmQue,(Int32 *)&audFrm,OSA_TIMEOUT_NONE);
    OSA_assert(status == 0);

    audFrm->audBuf = buf;
    audFrm->len    = len;
    audFrm->timestamp = framePTS;

    status = Avsync_audQuePut(&gAVSyncInfo.queObj,audFrm);
    OSA_assert(status == 0);

    return status;
}

static UInt32 Audio_getBackendDelay()
{
    int status;
    snd_pcm_sframes_t delay;
    UInt32 audioBEDelay = 0;

    status = snd_pcm_delay(play_handle,&delay);
    if (status)
    {
        UInt32 freq;
        Int32 vol;

        Audio_playGetSamplingFreq (&freq, &vol);
        audioBEDelay = (delay * 1000) /freq;
        OSA_printf("AUDIO:Calculated Audio BE delay:%d\n",
                   audioBEDelay);
    }
    return audioBEDelay;
}

static Int Audio_dequeDecodedFrame(AudFrm_BufList *freeBufList,
                                   Int8  **buf,Int32 *len ,
                                   UInt32 *muteFlag)
{
    Int status;
    AudFrm_Buf *audFrm;
    ;
    AvsyncLink_AudioBackEndInfo abeInfo;

    abeInfo.aSyncSTC = Avsync_getWallTime();
    abeInfo.audioBackendDelay = Audio_getBackendDelay();
    freeBufList->numFrames = 0;
    status = Avsync_audQueGet(&gAVSyncInfo.queObj,&audFrm,freeBufList,&abeInfo);
    OSA_assert(status == 0);

    if (status == 0)
    {
        status = OSA_quePut(&gAVSyncInfo.freeAudFrmQue,(Int32)audFrm,OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);
    }
    *buf = audFrm->audBuf;
    *len = audFrm->len;
    *muteFlag = audFrm->muteFlag;
    return status;
}


Void Audio_playMain (Void * arg)
{
    FILE            *fp = NULL;
    Int32           hdrLen = sizeof(AudioBkt);
    AudioBkt    bkt;
    Int32        len;
    Int8            *buf;
    TaskCtx         *ctx = arg;
    Int8            fileIdx = 0;
    Int status;
    AudFrm_BufList freeBufList;
    UInt32 muteFlag;

    while (ctx->exitFlag == 0)
    {
        if (playAudioFlag)
        {            
            if (fp == NULL)
            {
                fp = openFile(playChNum, fileIdx);
            }

            if (fp)
            {
#ifdef  AUDIO_STORE_HEADER
                len = fread(&bkt, 1, hdrLen, fp);
#else
                hdrLen = len = AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER * AUDIO_SAMPLE_LEN;
                bkt.data_size = len;
                bkt.bkt_status = BKT_VALID;
                bkt.timestamp  = 0;
#endif                
                if (len == hdrLen)
                {
#ifdef  AUDIO_STORE_HEADER
                    if (bkt.bkt_status != BKT_END)
#endif
                    {
                        len = bkt.data_size;

#ifdef AUDIO_G711CODEC_ENABLE
                    if (audioG711codec==TRUE) /* Added Support for G711 Codec */
                        {
                            audioPlaybackLen = fread(G711audioPlaybackBuf, 1, len, fp);
                            AUDIO_audioDecode(1,(char *)G711audioPlaybackBuf,(short*)audioPlaybackBuf,(Int32)len);
                            len = audioPlaybackLen*2;
                        }
                        else

#endif
                        {
                            audioPlaybackLen = fread(audioPlaybackBuf, 1, len, fp);
                            len = audioPlaybackLen;
                        }
                        buf = audioPlaybackBuf;
                        if (len)
                        {
                            status = Audio_enqueDecodedFrame(buf,len,bkt.timestamp);
                            OSA_assert(status == 0);
                            status = Audio_dequeDecodedFrame(&freeBufList,&buf,&len,&muteFlag);
                            if (muteFlag)
                            {
                                memset(buf,0,len);
                            }
                            if (playAudio(buf, len) != len)
                            {
                                if (strcmp(snd_strerror(playStats.lastError),"Success"))
                                {
                                printf (" AUDIO >>  PLAYBACK ERROR <%d bytes> %s, playback wont continue...\n", len, snd_strerror(playStats.lastError));
                                playAudioFlag = 0;
                                deInitAudioPlayback();
                                fclose(fp);
                                fp = NULL;
                                fileIdx = 0;
                                }
                            }
                            else
                            {
                                audioSamplesPlayed += (len / AUDIO_SAMPLE_LEN);
                            }
                        }
                        else if (audioPlaybackLen <= 0)
                        {
                            printf ("AUDIO >>  PLAYBACK FILE ENDED...opening next...\n");
                            fclose(fp);
                            fp = NULL;
                            fileIdx ++;
                        }
                    }
#ifdef  AUDIO_STORE_HEADER
                    else
                    {
                        printf ("AUDIO >>  PLAYBACK FILE ENDED...opening next...\n");
                        fclose(fp);
                        fp = NULL;
                        fileIdx ++;
                    }
#endif
                }
                else
                {
                    printf ("AUDIO >> PLAYBACK FILE ENDED...opening next...\n");
                    fclose (fp);
                    fp = NULL;
                    fileIdx ++;
                }
            }
            else    /* play zeros */
            {
                memset (audioPlaybackBuf, 0, AUDIO_MAX_PLAYBACK_BUF_SIZE);
                playAudio(audioPlaybackBuf, AUDIO_MAX_PLAYBACK_BUF_SIZE);
            }
        }
        else
        {
            if (fp)
            {
                fclose(fp);
                fp = NULL;
            }
            sleep(2);
            fileIdx = 0;
        }
    }
//  printf ("AUDIO >> Exiting audio play task............\n");
}

Int32 InitAudioRenderDevice (Int32 channels, Uint32 sample_rate, UInt32 playbackDevId)
{
    Int32 rc;
    snd_pcm_hw_params_t *params;
    Uint32 val;
    Int32 dir, ret;
    Int32 resample;

    if(playbackDevId == 0)
    {
        /* Open PCM device for playback. */
        rc = snd_pcm_open(&play_handle, "plughw:0,1",
                         SND_PCM_STREAM_PLAYBACK,
                                0);
        
    }
    else
    {
        /* Open PCM device for playback. */
        rc = snd_pcm_open(&play_handle, "plughw:0,2",
                         SND_PCM_STREAM_PLAYBACK,
                                0);        
    }


    if (rc < 0)
    {
        printf ("AUDIO >> Unable to open pcm device: %s\n", snd_strerror(rc));
        return -1;
    }
//  printf ("AUDIO_PLAY >> opened device\n");

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(play_handle, params);

    /* Set the desired hardware parameters. */
    /* Interleaved mode */
    snd_pcm_hw_params_set_access(play_handle, params,
                   SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(play_handle, params,
                           SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    rc = snd_pcm_hw_params_set_channels(play_handle, params, channels);
    if (rc < 0)
    {
        printf("AUDIO >> Channels count (%i) not available for playbacks: %s\n", channels, snd_strerror(rc));
    }

    dir = 0;
    ret = snd_pcm_hw_params_set_rate_near(play_handle, params,
                               &sample_rate, &dir);
    if (dir != 0)
    {
        printf("AUDIO >> The rate %d Hz is not supported by your hardware.\n ==> Using %d Hz instead.\n", sample_rate, ret);
    }


    /* following setting of period size is done only for AIC3X. Leaving default for HDMI */
    if(playbackDevId == 0)
    {
        resample = 1;
        snd_pcm_hw_params_set_rate_resample(play_handle, params, resample);

        /* Set period size to 1000 frames. */
        frames = 1000;  // do not change this. This matches with period size printed using aplay
        rc = snd_pcm_hw_params_set_period_size_near(play_handle,
                                  params, &frames, &dir);
        if (rc < 0)
        {
            printf("AUDIO >> Unable to set period size %li err: %s\n", frames, snd_strerror(rc));
            return -1;
        }
    }

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(play_handle, params);
    if (rc < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("Unable to set hw parameters: (%s)\n", rc, play_handle);
        play_handle = NULL;
        return -1;
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames,
                                    &dir);

    size = frames * 2; /* 2 bytes/sample, 1 channel */
    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    snd_pcm_hw_params_get_rate(params, &val, &dir);
    return 0;
}


Int32   playAudio(Int8 *buffer, Int32 numBytes)
{
    Int32 samples_played = 0;
    Int32 rc;

    if (play_handle == NULL)
    {
        printf("AUDIO >> Error - Device play_handle is NULL............\n");
        return 0;
    }

    while (numBytes >= size && playAudioFlag)
    {
        rc = snd_pcm_writei(play_handle, buffer + samples_played, frames);
        samples_played += size;
        numBytes -= size;
        if (rc == -EPIPE)
        {
            /* EPIPE means underrun */
            printf("AUDIO >> Underrun occurred\n");
            snd_pcm_prepare(play_handle);
            playStats.errorCnt++;
            playStats.lastError = rc;
            break;
        }
        else if (rc < 0)
        {
            printf("AUDIO >> error from writei: %s\n", snd_strerror(rc));
            playStats.errorCnt++;
            playStats.lastError = rc;
            break;
        }
        else if (rc != (Int32)frames)
        {
            printf("AUDIO >> short write, write %d frames\n", rc);
            playStats.errorCnt++;
            playStats.lastError = rc;
        }
    }
    return samples_played;
}

Int32 deInitAudioPlayback (void)
{
    if (play_handle)
    {
        //snd_pcm_drain(play_handle);
        //snd_pcm_drop(play_handle);
        snd_pcm_close(play_handle);
        play_handle = NULL;
//      printf ("AUDIO_PLAY >> Device closed\n");
    }
    return 0;
}




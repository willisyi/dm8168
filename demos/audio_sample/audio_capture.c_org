/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <audio.h>
#include <audio_priv.h>
#include <mcfw/interfaces/link_api/avsync.h>

#define         MAX_STR_SIZE        256

static Int32    RecordAudio(Uint8 *buffer, Int32 *numSamples);
static Int32    InitAudioCaptureDevice (Int32 channels, Uint32 sample_rate, Int32 driver_buf_size);
static Int32    deInitAudioCaptureDevice(Void);
static Void     deletePreviousRecording (Int8 chNum);
static Void     Audio_recordMain (Void * arg);

static Int32    recordAudioFlag = 0, recordChNum = 0;
static Int32    audioSamplesRecorded = 0;
static Int32    zero_samples_count = 0;
static Uint8    audioRecordBuf[AUDIO_SAMPLES_TO_READ_PER_CHANNEL
                                    * AUDIO_MAX_CHANNELS * AUDIO_SAMPLE_LEN * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER];

 /**< Audio sampling rate in Hz, Valid values: 8000, 16000 
  */
  UInt32 samplingHz=AUDIO_SAMPLE_RATE_DEFAULT;

  /**< Audio volume, Valid values: 0..8. Refer to TVP5158 datasheet for details
   */
  UInt32 audioVolume=AUDIO_VOLUME_DEFAULT;


static TaskCtx  captureThrHandle;
static snd_pcm_t *capture_handle = NULL;

static Int8     audioPhyToDataIndexMap[AUDIO_MAX_CHANNELS];
Int8            audioPath[MAX_STR_SIZE];
static AudioStats captureStats;
#ifdef AUDIO_G711CODEC_ENABLE
Uint8 audioG711codec = TRUE;
#endif


Int32 Audio_captureIsStart()
{
	return recordAudioFlag;
}

Int32 Audio_setStoragePath (Int8 *path)
{
    Int8    i;
    Int8    dirName[256];
    Uint8   len;
    Int8    *ptr;

    printf ("Store path set to %s\n", path);
    /* This path has \n also */
    ptr = strstr(path, "\n");
    if (!ptr)
    ptr = strstr(path, "\r");

    if (ptr)
    {
        Int32 to_copy;

        to_copy = ptr - path;
//      printf ("Removing carriage return, bytes to copy %d\n", to_copy);
        strncpy(audioPath, path, to_copy);
        audioPath[to_copy] = 0;
    }
    else
    {
        strcpy(audioPath, path);
    }

    printf ("Trying to set storage path to %s\n", audioPath);

    if(-1 == access(audioPath, 0))
    {
        remove(audioPath);
        if( 0 != mkdir(audioPath, 0755))
        {
            printf ("\nAudio storage dir \"%s\" - Invalid Entry or File Permission Issue - audio recording not possible.....\n", audioPath);
            return  AUDIO_STATUS_EFAIL;
        }
    }

    for (i=0; i<AUDIO_MAX_CHANNELS; i++)
    {
        strcpy(dirName, audioPath);
        len = strlen(dirName);
        sprintf (&dirName[len], "/%02d", i + 1);
        if(-1 == access((char *) dirName, 0))
        {
            if( 0 != mkdir((char *) dirName, 0755))
            {
                printf ("\nAudio storage dir \"%s\" , subdir \"%s\"  Invalid Entry or File Permission Issue - audio recording not possible.....\n",
                            audioPath, dirName);
                return  AUDIO_STATUS_EFAIL;
            }
        }
    }
    return AUDIO_STATUS_OK;
}

Int32 Audio_captureCreate (Void)
{
    Int32  status = AUDIO_STATUS_OK;
#ifdef  USE_DEFAULT_STORAGE_PATH
    Int8    i;
    Int8 dirName[256];
    Uint8   len;
#endif

    /* TVP5158 CAPTURED AUDIO DATA MAPPING*/
    /* Audio data captured from the TVP5158 are interleaved*/
    /* TVP5158 Daughter card has following configuration for Audio Input (Hardware pins
    --------------------------------------------------------------------------------------
    | AIN15 | AIN13 | AIN11 | AIN9  | AIN7 | AIN5 | AIN3 | AIN1 |
    --------------------------------------------------------------------------------------
    | AIN16 | AIN14 | AIN12 | AIN10 | AIN8 | AIN6 | AIN4 | AIN2 |
    --------------------------------------------------------------------------------------
    */

#if (AUDIO_MAX_CHANNELS==4)
    /*
    Channel Mapping for 4-channels audio capture
    <-----------------64bits----------------->
    <-16bits->
    --------------------------------------------
    | S16-0  | S16-1  | S16-2  | S16-3 |
    --------------------------------------------
    | AIN 3 | AIN 0 | AIN 2 | AIN 1 |
    --------------------------------------------
    */
    audioPhyToDataIndexMap[0]   = 1;
    audioPhyToDataIndexMap[1]   = 3;
    audioPhyToDataIndexMap[2]   = 2;
    audioPhyToDataIndexMap[3]   = 0;

#else

    /*
    Channel Mapping for 16-channels audio capture
    <---------------------------------------------------------------------------------256bits------------------------------------------------------------------------------------>
    <-16bits->
    -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    | S16-0  | S16-1  | S16-2  | S16-3 | S16-4  | S16-5  | S16-6  | S16-7 | S16-8  | S16-9  | S16-10 | S16-11 | S16-12 | S16-13 | S16-14  | S16-15 |
    -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    AIN 16  | AIN 1 | AIN 3 | AIN 5 | AIN 7 | AIN 9 | AIN 11 | AIN 13 | AIN 15 | AIN 2 | AIN 4 | AIN 6 | AIN 8 | AIN 10 | AIN 12 | AIN 14 |
    -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    */

    audioPhyToDataIndexMap[0]   = 1;
    audioPhyToDataIndexMap[1]   = 9;
    audioPhyToDataIndexMap[2]   = 2;
    audioPhyToDataIndexMap[3]   = 10;

    audioPhyToDataIndexMap[4]   = 3;
    audioPhyToDataIndexMap[5]   = 11;
    audioPhyToDataIndexMap[6]   = 4;
    audioPhyToDataIndexMap[7]   = 12;

    audioPhyToDataIndexMap[8]   = 5;
    audioPhyToDataIndexMap[9]   = 13;
    audioPhyToDataIndexMap[10]  = 6;
    audioPhyToDataIndexMap[11]  = 14;

    audioPhyToDataIndexMap[12]  = 7;
    audioPhyToDataIndexMap[13]  = 15;
    audioPhyToDataIndexMap[14]  = 8;
    audioPhyToDataIndexMap[15]  = 0;
#endif
    strcpy(audioPath, "");

#ifdef  USE_DEFAULT_STORAGE_PATH
    strcpy(audioPath, "/home/audio");

    if(-1 == access(audioPath, 0))
    {
        remove(audioPath);
        if( 0 != mkdir(audioPath, 0755))
        {
            printf ("\nAUDIO  [%d] >>  Audio storage dir %s not created - audio recording not possible.....\n", __LINE__, audioPath);
            return AUDIO_STATUS_EFAIL;
        }
    }

    for (i=0; i<AUDIO_MAX_CHANNELS; i++)
    {
        strcpy(dirName, audioPath);
        len = strlen(dirName);
        sprintf (&dirName[len], "/%02d", i + 1);
        if(-1 == access((char *) dirName, 0))
        {
            if( 0 != mkdir((char *) dirName, 0755))
            {
                printf ("\nAUDIO  [%d] >>  Audio storage dir %s not created - audio recording not possible.....\n", __LINE__, dirName);
                return  AUDIO_STATUS_EFAIL;
            }
        }
    }
#endif

   status = Audio_thrCreate(
                &captureThrHandle,
                (ThrEntryFunc) Audio_recordMain,
                AUDIO_CAPTURE_TSK_PRI,
                AUDIO_CAPTURE_TSK_STACK_SIZE
                );
   printf ("\n Audio capture task created");
   UTILS_assert(  status==AUDIO_STATUS_OK);
   return   status;
}

Int32 Audio_captureStart (Int8      chNum)
{
    deletePreviousRecording (chNum);
    InitAudioCaptureDevice (AUDIO_MAX_CHANNELS, samplingHz, AUDIO_BUFFER_SIZE);
    sleep(1);
    recordChNum = chNum;
    recordAudioFlag = 1;
    return 0;
}


Int32 Audio_captureStop (Void)
{
    recordAudioFlag = 0;
    sleep(1);
    deInitAudioCaptureDevice();
    return 0;
}

Int32 Audio_captureDelete (Void)
{
    printf ("\n Deleting Audio capture task");
    Audio_thrDelete(&captureThrHandle);
    printf (" Audio capture task deleted\n");
    return 0;
}


Int32 Audio_capturePrintStats (Void)
{
    printf ("\n============================\n");
    if (recordAudioFlag == 1)
        printf ("Capture ACTIVE,  ");
    else
        printf ("Capture NOT ACTIVE,  ");
    printf ("Channel %d, ErrCnt [%d], lastErr [%d, %s]\n", recordChNum, captureStats.errorCnt, captureStats.lastError, snd_strerror(captureStats.lastError));
    return 0;
}


Int32 Audio_pramsPrint()
{
  printf ("\n===================================================\n");
  printf ("\nSampling Frequency : %d, Audio Volume Level : %d  \n",samplingHz,audioVolume);
  printf ("\n===================================================\n");
  return 0;
}


Void deletePreviousRecording (Int8 chNum)
{
    Int8 idx;
    Int8 fname[256];
    Uint8 len;

    if (chNum<AUDIO_MAX_CHANNELS)
    {
        // Temp - Delete already existing files
        for (idx=0; idx<AUDIO_MAX_FILES; idx++)
        {
            strcpy(fname, audioPath);
            len = strlen(fname);
            sprintf (&fname[len], "/%02d/%s%02d.pcm", chNum + 1, AUDIO_RECORD_FILE, idx + 1);
            printf ("Removing %s \n", fname);
            remove(fname);
        }
    }
}

static UInt32 Audio_getFrontendDelay(Int32 curFrameLen)
{
    snd_pcm_sframes_t delay;
    UInt32 audioFEDelay = 0;

    delay = snd_pcm_avail_update(capture_handle);
    if (delay >= 0)
    {
        delay += curFrameLen;
    }
    else
    {
        delay = curFrameLen;
    }
    audioFEDelay = (delay * 1000) /samplingHz;
    OSA_printf("AUDIO:Calculated Audio FE delay:%d\n",
               audioFEDelay);

    return audioFEDelay;
}

Void  Audio_recordMain (Void * arg)
{
    FILE        *fp = NULL;
    Int8        fname[256];
    Int32       len, idx = 0, fileLen = 0, ii, stored;
    AudioBkt    bkt;
    Uint8       buf[AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_SAMPLE_LEN * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER], *tmp;
    Uint8       buf_g711_encode[AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_SAMPLE_LEN * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER]; /* Added Support for G711 Codec */
    Int32       audioRecordLen = 0;
    TaskCtx     *ctx = arg;
    Uint8       dirlen;
    Int32       err;

    idx = 0;
    strcpy(bkt.id, "AUD_");

    while (ctx->exitFlag == 0)
    {
        if (recordAudioFlag)
        {
            if (fp == NULL)
            {
                strcpy(fname, audioPath);
                dirlen = strlen(fname);
                sprintf (&fname[dirlen], "/%02d/%s%02d.pcm", recordChNum + 1, AUDIO_RECORD_FILE, idx + 1);

                idx ++;
                fp = fopen(fname, "wb");
                printf ("Opened %s for recording..\n", fname);
                fileLen = 0;
                if (idx >= AUDIO_MAX_FILES)
                {
                    idx = 0;
                }
            }

            tmp = audioRecordBuf;
            ii = 0;
            audioRecordLen = 0;
            while (ii < AUDIO_PLAYBACK_SAMPLE_MULTIPLIER && recordAudioFlag)
            {
                len = AUDIO_SAMPLES_TO_READ_PER_CHANNEL;
                err = RecordAudio(tmp, &len);
                if (len == 0 || err < 0)
                {
                    if (strcmp(snd_strerror(captureStats.lastError),"Success"))
                    {
                    printf (" AUDIO >>  CAPTURE ERROR %s, capture wont continue...\n", snd_strerror(captureStats.lastError));
                    recordAudioFlag = 0;
                    deInitAudioCaptureDevice();
                    len = 0;
                    if (fp)
                        fclose(fp);
                    fp = NULL;
                    idx = 0;
                    }
                }
                if (len > 0)
                {
                    audioRecordLen += len;
                    tmp += len * AUDIO_SAMPLE_LEN * AUDIO_MAX_CHANNELS;
                }
                else
                {
                    audioRecordLen = 0;
                    break;
                }
                ii++;
            }

            if (audioRecordLen > 0 && fp)
            {
                UInt64 curTime = Avsync_getWallTime() -
                                 Audio_getFrontendDelay(audioRecordLen);

                if (fileLen > AUDIO_RECORD_FILE_MAX_SIZE)
                {
                    bkt.numChannels = 1;
                    bkt.samplingRate = 16000;
                    bkt.data_size = 0;
                    bkt.timestamp = curTime;  // timestamp from avsync wall time clock.
                    bkt.bkt_status = BKT_END;
#ifdef  AUDIO_STORE_HEADER
                    fwrite(&bkt, sizeof(AudioBkt), 1, fp);
#endif
                    fclose(fp);

                    strcpy(fname, audioPath);
                    dirlen = strlen(fname);
                    sprintf (&fname[dirlen], "/%02d/%s%02d.pcm", recordChNum + 1, AUDIO_RECORD_FILE, idx + 1);
                    printf ("Opened %s for recording..\n", fname);

                    idx ++;
                    fp = fopen(fname, "wb");
                    fileLen = 0;
                    if (idx >= AUDIO_MAX_FILES)
                    {
                        idx = 0;
                    }
                }
                bkt.numChannels = 1;
                bkt.samplingRate = 16000;

                bkt.timestamp = curTime;
                printf (" AUDIO >>  CAPTURE : rec TS = %lld \n", bkt.timestamp);

                bkt.data_size = audioRecordLen * AUDIO_SAMPLE_LEN;
#ifdef AUDIO_G711CODEC_ENABLE
                if (audioG711codec==TRUE) /* Added Support for G711 Codec */
                {
                    bkt.data_size = audioRecordLen * AUDIO_SAMPLE_LEN/2;
                }
#endif
                bkt.bkt_status = BKT_VALID;


#ifdef  AUDIO_STORE_HEADER
                fwrite(&bkt, sizeof(AudioBkt), 1, fp);
#endif
                /* Data is for recordChNum, store only 1 channel data now */
                stored = 0;
                tmp = audioRecordBuf;
#if 0
                tmp += (recordChNum * AUDIO_SAMPLE_LEN);
#else
                tmp += (audioPhyToDataIndexMap[recordChNum] * AUDIO_SAMPLE_LEN);
#endif

                while (stored < (audioRecordLen * AUDIO_SAMPLE_LEN))
                {
                    memcpy(buf + stored, tmp, AUDIO_CHANNEL_SAMPLE_INTERLEAVE_LEN * AUDIO_SAMPLE_LEN);
                    stored += (AUDIO_CHANNEL_SAMPLE_INTERLEAVE_LEN * AUDIO_SAMPLE_LEN);
                    tmp += (AUDIO_CHANNEL_SAMPLE_INTERLEAVE_LEN * AUDIO_SAMPLE_LEN * AUDIO_MAX_CHANNELS);
                }
#ifdef AUDIO_G711CODEC_ENABLE
                if (audioG711codec==TRUE) /* Added Support for G711 Codec */
                {
                    AUDIO_audioEncode(1,(short *)buf_g711_encode,(short *)buf,(Int32)audioRecordLen * AUDIO_SAMPLE_LEN);
                    if (fp)
                        fwrite(buf_g711_encode, audioRecordLen * AUDIO_SAMPLE_LEN/2, 1, fp);
                    fileLen += audioRecordLen * AUDIO_SAMPLE_LEN/2;
                    audioSamplesRecorded += audioRecordLen;
                }
                else
#endif
                {
                    if (fp)
                        fwrite(buf, audioRecordLen * AUDIO_SAMPLE_LEN, 1, fp);
                    fileLen += audioRecordLen * AUDIO_SAMPLE_LEN;
                    audioSamplesRecorded += audioRecordLen;
                }
            }
            else
            {
                zero_samples_count++;
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
            idx = 0;
        }
    }
//  printf ("AUDIO >> Exiting audio capture task............\n");
}

static Int32 InitAudioCaptureDevice (Int32 channels, UInt32 sample_rate, Int32 driver_buf_size)
{
    snd_pcm_hw_params_t *hw_params;
    Int32 err;

    if ((err = snd_pcm_open (&capture_handle, ALSA_CAPTURE_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        fprintf (stderr, "AUDIO >>  cannot open audio device plughw:1,0 (%s)\n", snd_strerror (err));
        return  -1;
    }
//  printf ("AUDIO >>  opened %s device\n", ALSA_CAPTURE_DEVICE);
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot allocate hardware parameter structure (%s)\n", err, capture_handle);
    }

    if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot initialize hardware parameter structure (%s)\n", err, capture_handle);
    }

    if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set access type (%s)\n", err, capture_handle);
    }

    if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set sample format (%s)\n", err, capture_handle);
    }

    if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &sample_rate, 0)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set sample rate (%s)\n", err, capture_handle);
    }

    if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, channels)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set channel count (%s)\n", err, capture_handle);
    }

    if ((err = snd_pcm_hw_params_set_buffer_size (capture_handle, hw_params, driver_buf_size)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set buffer size (%s)\n", err, capture_handle);
    }

    if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set parameters (%s)\n", err, capture_handle);
    }

    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (capture_handle)) < 0)
    {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot prepare audio interface for use (%s)\n", err, capture_handle);
    }
    return 0;
}


static Int32    RecordAudio(Uint8 *buffer, Int32 *numSamples)
{
    Int32 err = -1;

    if (capture_handle)
    {
        if ((err = snd_pcm_readi (capture_handle, buffer, *numSamples)) != *numSamples)
        {
//          printf (" AUDIO >> read from audio interface failed (%s)\n", snd_strerror (err));
            *numSamples = 0;
            captureStats.errorCnt++;
            captureStats.lastError = err;

        }
    }
    else
    {
        *numSamples = 0;
    }
    return err;
}


static Int32 deInitAudioCaptureDevice(Void)
{

    if (capture_handle)
    {
        snd_pcm_drain(capture_handle);
        snd_pcm_close(capture_handle);
        capture_handle = NULL;
//      printf ("AUDIO >> Device closed\n");
    }
    return 0;
}

